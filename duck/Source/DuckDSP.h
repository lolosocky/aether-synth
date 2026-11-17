#pragma once
// LGN Duck - the whole sidechain engine. No JUCE here, so it can be tested on its own.
//   Tempo mode: volume dips on every beat division (1/1 ... 1/16), locked to the host's song position.
//   Kick mode:  volume dips every time a hit arrives on the sidechain input (route your kick there).
#include <cmath>
#include <algorithm>

namespace lgn {

struct Duck {
    // ---- controls (set from the UI every block) ----
    double amount = 0.75;    // 0..1  how far the volume dips
    double release = 0.5;    // 0..1  how long it takes to come back up
    int mode = 0;            // 0 = Tempo, 1 = Kick
    double rateBeats = 1.0;  // length of one pump in quarter notes (1/4 note = 1.0)

    // ---- read-only state for the UI ----
    double reduction = 0;    // 0..1, how much the volume is down right now
    double lastHitAge = 1e9; // seconds since the last kick was detected (Kick mode)

    void prepare(double sampleRate) { sr = sampleRate; reset(); }
    void reset() { gain = 1; env = 0; peakHold = 0; armed = true; since = 1e9; reduction = 0; }

    // Shape of one pump: x = 0 at the hit, 1 when fully recovered. Holds low briefly, then eases back.
    static double curve(double x) {
        if (x <= 0) return 1;
        if (x >= 1) return 0;
        const double u = 1 - x;
        return u * u * (3 - 2 * u);
    }
    // Fraction of each beat division that the dip lasts, from the Release knob.
    static double tempoLength(double release) { return 0.15 + 0.8 * release; }
    // Dip length in seconds for Kick mode, from the Release knob (40-600 ms).
    static double kickSeconds(double release) { return 0.04 + 0.56 * std::pow(release, 1.5); }

    // Process one block in place. ppqStart = song position in quarter notes at the first sample, bpm = host tempo,
    // playing = transport running. sc = sidechain channels (may be null in Tempo mode).
    void process(float* const* ch, int numCh, int n, double ppqStart, double bpm, bool playing, const float* const* sc, int numSc) {
        const double ppqPerSample = bpm / 60.0 / sr;
        const double smooth = 1 - std::exp(-1.0 / (0.0008 * sr));   // ~0.8 ms de-click on gain moves
        const double envAtt = 1 - std::exp(-1.0 / (0.0005 * sr)), envRel = 1 - std::exp(-1.0 / (0.03 * sr));
        const double holdRel = std::exp(-1.0 / (2.0 * sr));
        const double kickLen = kickSeconds(release) * sr, len = tempoLength(release);
        for (int i = 0; i < n; ++i) {
            double red = 0;
            if (mode == 0) {
                if (playing && rateBeats > 0) {
                    const double ppq = ppqStart + i * ppqPerSample;
                    double pos = std::fmod(ppq, rateBeats) / rateBeats; if (pos < 0) pos += 1;
                    red = curve(pos / len);
                }
            } else {
                double a = 0;
                for (int c = 0; c < numSc; ++c) if (sc && sc[c]) a = std::max(a, (double)std::abs(sc[c][i]));
                env += (a > env ? envAtt : envRel) * (a - env);
                peakHold = std::max(env, peakHold * holdRel);                // tracks how loud the kick is
                const double thr = std::max(0.003, 0.45 * peakHold);        // so the trigger level sets itself
                if (armed && env > thr) { armed = false; since = 0; }
                else if (!armed && env < 0.6 * thr) armed = true;
                red = curve(since / kickLen);
                since += 1;
            }
            const double target = 1 - amount * red;
            gain += smooth * (target - gain);
            for (int c = 0; c < numCh; ++c) ch[c][i] = (float)(ch[c][i] * gain);
        }
        reduction = 1 - gain;
        lastHitAge = mode == 1 ? since / sr : 1e9;
    }
private:
    double sr = 48000, gain = 1, env = 0, peakHold = 0, since = 1e9; bool armed = true;
};

} // namespace lgn
