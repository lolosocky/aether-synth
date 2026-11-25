// LGN Swing: re-times audio to a swing feel in real time, locked to the song's grid.
//
// The audio is cut at every grid step and each step is moved later (or earlier) by its own amount, at normal speed
// so nothing changes pitch. Moves happen with a short crossfade placed just *before* each step's start, so the
// hit at the start of the step is never touched:
//   a step moved later leaves a gap behind it, filled by repeating the end of the step before (Repeat) or by
//   letting it fade (Fade); a step moved earlier cuts the end of the step before short.
// Swing moves every second step (the MPC way: 50% straight, 66% triplet feel, 75% hard swing); each of the 16 steps
// can also be pushed or dragged by hand, and Humanize adds a small, repeatable random drift per step.
// Pushing a step earlier needs to look ahead, so the plugin runs a fixed 40 ms behind (the host makes up for it).
// Plain C++ (no JUCE) so it can be tested on its own.
#pragma once
#include "Common.h"

namespace lgn {

struct SwingParams {
    int grid = 1;                  // 0: 1/8 notes, 1: 1/16 notes
    double swingPct = 58;          // 50..80
    double humanizeMs = 0;
    int tail = 0;                  // 0 repeat, 1 fade
    std::array<double, 16> step{}; // per-step offset, -0.3..0.3 of a step
};

class SwingDsp {
public:
    static constexpr double kLookaheadS = 0.040, kXfadeS = 0.003;
    int latency() const { return L; }
    double stepBeats() const { return p.grid == 0 ? 0.5 : 0.25; }

    void prepare(double rate) {
        sr = rate; L = (int)std::lround(kLookaheadS * sr); X = std::max(16, (int)std::lround(kXfadeS * sr));
        for (auto& d : line) d.prepare((int)(sr * 1.2) + L + 64);
        dCur = dNew = L; xfLeft = 0; queueN = 0; lastStep = INT64_MIN; gapUntil = -1; gapFrom = -1; now = 0;
        lastD = L;
    }
    void set(const SwingParams& np) { p = np; }
    void reset() { for (auto& d : line) d.reset(); dCur = dNew = L; xfLeft = 0; queueN = 0; lastStep = INT64_MIN; }

    // Offset in samples for step index k (absolute, counted from the song start), as a delay >= the crossfade.
    double delayFor(int64_t k, double samplesPerStep) const {
        const int s = (int)(((k % 16) + 16) % 16);
        double off = 0;
        if (s % 2 == 1) off += (clampv(p.swingPct, 50.0, 80.0) / 50.0 - 1.0) * samplesPerStep;   // swing: every second step
        off += clampv(p.step[(size_t)s], -0.3, 0.3) * samplesPerStep;
        if (p.humanizeMs > 0) off += (hash01(k) * 2 - 1) * p.humanizeMs * 0.001 * sr;
        return clampv(L + off, (double)X + 1, sr * 1.1);
    }
    // the delay each of the 16 steps gets right now (display)
    double offsetMs(int s, double bpm) const {
        const double sps = stepBeats() * 60.0 / std::max(20.0, bpm) * sr;
        double off = 0;
        if (s % 2 == 1) off += (clampv(p.swingPct, 50.0, 80.0) / 50.0 - 1.0) * sps;
        off += clampv(p.step[(size_t)s], -0.3, 0.3) * sps;
        return std::max(off, -(double)(L - X - 1)) / sr * 1000.0;
    }

    // ppq = song position (quarter notes) of the first sample; playing = transport running.
    void process(float* const* ch, int nch, int n, double ppq, double bpm, bool playing) {
        nch = std::min(nch, 2);
        const double ppqPerSample = std::max(20.0, bpm) / 60.0 / sr;
        const double sb = stepBeats(), sps = sb / ppqPerSample;
        for (int i = 0; i < n; i++, now++) {
            for (int c = 0; c < 2; c++) line[(size_t)c].push(ch[std::min(c, nch - 1)][i]);
            // a new grid step starts at this input sample: schedule the move to its delay
            if (playing) {
                const double q = ppq + i * ppqPerSample;
                const int64_t k = (int64_t)std::floor(q / sb + 1e-9);
                if (k != lastStep) {
                    const double dk = delayFor(k, sps);
                    // the step before keeps playing until its own end, unless this step needs to start sooner
                    const double prev = lastD;
                    const double switchAt = (double)now + std::min(prev, dk);
                    schedule((int64_t)std::lround(switchAt) - X, dk, prev < dk ? (int64_t)std::lround((double)now + prev) : -1, (int64_t)std::lround((double)now + dk));
                    lastD = dk; lastStep = k;
                }
            } else if (lastStep != INT64_MIN || std::abs(lastD - L) > 0.5) {
                // stopped: go back to the plain latency
                schedule(now + 1, L, -1, -1); lastD = L; lastStep = INT64_MIN;
            }
            // start any move that is due
            if (xfLeft == 0 && queueN > 0 && queue[0].start <= now) {
                dNew = queue[0].delay; xfLeft = X;
                if (p.tail == 1 && queue[0].gapFrom >= 0) { gapFrom = queue[0].gapFrom; gapUntil = queue[0].gapUntil; }
                for (int q2 = 1; q2 < queueN; q2++) queue[(size_t)q2 - 1] = queue[(size_t)q2];
                queueN--;
            }
            double outL, outR;
            if (xfLeft > 0) {
                const double w = 1.0 - (double)xfLeft / X, a = std::cos(w * 0.5 * kPi), b = std::sin(w * 0.5 * kPi);
                outL = line[0].read(dCur) * a + line[0].read(dNew) * b;
                outR = line[1].read(dCur) * a + line[1].read(dNew) * b;
                if (--xfLeft == 0) dCur = dNew;
            } else { outL = line[0].read(dCur); outR = line[1].read(dCur); }
            // Fade mode: the gap left by a later step goes quiet instead of repeating
            if (gapUntil >= 0 && p.tail == 1) {
                const int64_t ramp = X;
                double g = 1;
                if (now >= gapFrom && now < gapUntil) {
                    const double into = (double)(now - gapFrom), left = (double)(gapUntil - now);
                    g = std::min(1.0, std::max(0.0, 1.0 - into / ramp)) + std::max(0.0, 1.0 - left / ramp);
                    g = clampv(g, 0.0, 1.0);
                } else if (now >= gapUntil) gapUntil = -1;
                outL *= g; outR *= g;
            }
            ch[0][i] = (float)outL;
            if (nch > 1) ch[1][i] = (float)outR;
        }
    }

private:
    struct Move { int64_t start; double delay; int64_t gapFrom, gapUntil; };
    void schedule(int64_t start, double delay, int64_t gapFrom, int64_t gapUntil) {
        if (queueN >= (int)queue.size()) { for (int q = 1; q < queueN; q++) queue[(size_t)q - 1] = queue[(size_t)q]; queueN--; }
        queue[(size_t)queueN++] = { std::max(start, now), delay, gapFrom, gapUntil };
    }
    static double hash01(int64_t k) {   // repeatable per step, so a looped bar drifts the same way each time
        uint64_t x = (uint64_t)k * 0x9E3779B97F4A7C15ull; x ^= x >> 31; x *= 0xBF58476D1CE4E5B9ull; x ^= x >> 29;
        return (double)(x & 0xFFFFFF) / (double)0xFFFFFF;
    }

    SwingParams p; double sr = 48000; int L = 1920, X = 144;
    std::array<DelayLine, 2> line;
    double dCur = 1920, dNew = 1920, lastD = 1920; int xfLeft = 0;
    std::array<Move, 16> queue{}; int queueN = 0;
    int64_t lastStep = INT64_MIN, now = 0, gapFrom = -1, gapUntil = -1;
};
} // namespace lgn
