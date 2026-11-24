// LGN Delay: tempo-synced or free stereo delay with a tape-style feedback path.
// Changing the time glides (pitch bends like tape) instead of clicking. The feedback loop has low and high cuts,
// warmth (soft saturation, so high feedback blooms instead of exploding) and wow (slow time wobble).
// Duck turns the echoes down while you play and lets them bloom in the gaps.
//   Stereo     left and right echo on their own sides
//   Ping-Pong  echoes bounce left, right, left...
//   Mono       one centred echo
#pragma once
#include "Common.h"

namespace lgn {

struct DelayParams { double timeMs = 375, feedback = 0.35, mix = 0.25, lowCutHz = 120, highCutHz = 9000, warmth = 0.2, wow = 0.1, duck = 0, spreadMs = 0; int mode = 0; };

class DelayDsp {
public:
    enum Mode { stereo, pingPong, mono };
    void prepare(double rate) {
        sr = rate;
        for (auto& l : line) l.prepare((int)(2.1 * sr) + 64);
        for (auto& f : lc) f.reset();
        for (auto& f : hc) f.reset();
        timeS.setTime(sr, 0.12); timeS.reset(p.timeMs);
        mixS.setTime(sr, 0.02); mixS.reset(p.mix);
        duckEnv.set(sr, 0.005, 0.25); duckEnv.env = 0;
        wowPh = 0; counter = 0; filters();
    }
    void set(const DelayParams& np) { p = np; }
    void reset() { for (auto& l : line) l.reset(); for (auto& f : lc) f.reset(); for (auto& f : hc) f.reset(); }

    void process(float* const* ch, int nch, int n) {
        nch = std::min(nch, 2);
        const double fb = clampv(p.feedback, 0.0, 1.0) * 0.98;
        const double drive = 1.0 + p.warmth * 3.0;
        for (int i = 0; i < n; i++) {
            if ((counter++ & 31) == 0) filters();
            const double inL = ch[0][i], inR = nch > 1 ? ch[1][i] : inL;
            const double t = timeS.next(p.timeMs);
            wowPh += 0.6 / sr; if (wowPh >= 1) wowPh -= 1;
            const double wobble = p.wow * 0.004 * sr * (0.7 * std::sin(2 * kPi * wowPh) + 0.3 * std::sin(2 * kPi * wowPh * 3.1 + 1.0));
            const double dL = std::max(2.0, t * 0.001 * sr + wobble), dR = std::max(2.0, (t + (p.mode == stereo ? p.spreadMs : 0)) * 0.001 * sr + wobble);
            const double yl = line[0].read(dL), yr = line[1].read(p.mode == mono ? dL : dR);
            // feedback path: filters, then soft saturation scaled so it is unity for small signals
            auto loop = [&](double x, int c) { x = hc[(size_t)c].process(lc[(size_t)c].process(x)); return std::tanh(x * drive) / drive; };
            const double fl = loop(yl, 0), fr = loop(yr, 1);
            double wl, wr, writeL, writeR;
            switch (p.mode) {
                case pingPong: writeL = 0.5 * (inL + inR) + fr * fb; writeR = fl * fb; wl = yl; wr = yr; break;
                case mono: { const double m = 0.5 * (inL + inR); writeL = writeR = m + fl * fb; wl = wr = yl; break; }
                default: writeL = inL + fl * fb; writeR = inR + fr * fb; wl = yl; wr = yr; break;
            }
            line[0].push((float)writeL); line[1].push((float)writeR);
            const double env = duckEnv.process(std::max(std::abs(inL), std::abs(inR)));
            const double dg = 1.0 - p.duck * std::min(1.0, env * 4.0);
            // equal-power mix: dry stays near unity at typical insert settings (25% mix: dry -0.7 dB)
            const double m = mixS.next(p.mix);
            if (m != lastMix) { lastMix = m; dryG = std::cos(m * 1.5707963267948966); wetG = std::sin(m * 1.5707963267948966); if (m >= 1.0) dryG = 0; }
            ch[0][i] = (float)(inL * dryG + wl * dg * wetG);
            if (nch > 1) ch[1][i] = (float)(inR * dryG + wr * dg * wetG);
            echoPeak = std::max(echoPeak, std::max(std::abs(wl), std::abs(wr)) * dg);
        }
    }
    double takeEchoPeak() { const double v = echoPeak; echoPeak = 0; return v; }
private:
    void filters() { for (int c = 0; c < 2; c++) { lc[(size_t)c].set(Svf::highpass, sr, p.lowCutHz, 0.6); hc[(size_t)c].set(Svf::lowpass, sr, p.highCutHz, 0.6); } }
    DelayParams p; double sr = 48000, wowPh = 0, echoPeak = 0;
    std::array<DelayLine, 2> line; std::array<Svf, 2> lc, hc; double lastMix = -1, dryG = 1, wetG = 0; Smooth timeS, mixS; Follower duckEnv; unsigned counter = 0;
};

// Note lengths for tempo sync, in beats.
inline const std::vector<std::pair<const char*, double>>& delayDivisions() {
    static const std::vector<std::pair<const char*, double>> d = {
        { "1/32", 0.125 }, { "1/16T", 1.0 / 6 }, { "1/16", 0.25 }, { "1/16D", 0.375 }, { "1/8T", 1.0 / 3 }, { "1/8", 0.5 }, { "1/8D", 0.75 },
        { "1/4T", 2.0 / 3 }, { "1/4", 1.0 }, { "1/4D", 1.5 }, { "1/2T", 4.0 / 3 }, { "1/2", 2.0 }, { "1/2D", 3.0 }, { "1/1", 4.0 } };
    return d;
}
} // namespace lgn
