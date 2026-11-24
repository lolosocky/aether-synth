// LGN Compressor. Three styles share one gain computer (soft knee, Giannoulis/Massberg/Reiss):
//   Clean  feed-forward peak detector, smooth decoupled attack/release in the dB domain
//   Glue   feed-forward RMS detector with a program-dependent (dual) release, for bus glue
//   Punch  feedback topology (detects its own output), for vintage-style snap on drums
// Stereo is always linked so the image never shifts.
#pragma once
#include "Common.h"

namespace lgn {

struct CompParams {
    double thresh = -18, ratio = 4, attackMs = 10, releaseMs = 120, kneeDb = 6, makeupDb = 0, mix = 1, scHpf = 20;
    bool autoMakeup = false; int style = 0;
};

class CompDsp {
public:
    enum Style { clean, glue, punch };

    // Static curve: output level for an input level, both in dB.
    static double curve(double x, double T, double R, double W) {
        const double d = x - T;
        if (2 * d < -W) return x;
        if (W > 0 && 2 * std::abs(d) <= W) return x + (1.0 / R - 1.0) * (d + W / 2) * (d + W / 2) / (2 * W);
        return T + d / R;
    }
    // Estimate of the level lost on typical mix material (peaks around -6 dBFS, body around -12 dBFS):
    // the reduction a signal at -12 dBFS would get, scaled down because only its loudest parts reach it.
    static double autoMakeupDb(double T, double R) { return std::max(0.0, (-12.0 - T) * (1.0 - 1.0 / R) * 0.4); }

    void prepare(double rate) {
        sr = rate; grSlow = grFast = grDb = 0; ms = 0; fbLevel = 0;
        for (auto& f : hpf) f.reset();
        makeup.setTime(sr, 0.02); makeup.reset(dbToGain(makeupTarget()));
        mixS.setTime(sr, 0.02); mixS.reset(p.mix);
    }
    void set(const CompParams& np) { p = np; }
    double reductionDb() const { return grDb; }

    void process(float* const* ch, int nch, int n) {
        nch = std::min(nch, 2);
        const double aA = coef(p.attackMs), aR = coef(p.releaseMs);
        const double aAslow = coef(p.attackMs * 8 + 20), aRslow = coef(p.releaseMs * 5);
        const double rmsC = coef(std::max(5.0, p.attackMs));
        const bool hp = p.scHpf > 21;
        if (hp) for (auto& f : hpf) f.set(Svf::highpass, sr, p.scHpf, 0.707);
        const double mk = makeupTarget();
        for (int i = 0; i < n; i++) {
            // detector input (linked)
            double det = 0;
            if (p.style == punch) det = fbLevel;
            else for (int c = 0; c < nch; c++) { double x = ch[c][i]; if (hp) x = hpf[(size_t)c].process(x); det = std::max(det, std::abs(x)); }
            if (p.style == punch && hp) for (int c = 0; c < nch; c++) hpf[(size_t)c].process(ch[c][i]);   // keep filter state warm
            double lvl;
            if (p.style == glue) { ms = det * det + (ms - det * det) * rmsC; lvl = gainToDb(std::sqrt(ms) * 1.41421356); }
            else lvl = gainToDb(det);
            const double target = lvl - curve(lvl, p.thresh, p.ratio, p.kneeDb);   // dB of reduction wanted, >= 0
            // smoothing in the dB domain
            grFast = target > grFast ? target + (grFast - target) * aA : target + (grFast - target) * aR;
            if (p.style == glue) { grSlow = target > grSlow ? target + (grSlow - target) * aAslow : target + (grSlow - target) * aRslow; grDb = std::max(grFast, grSlow * 0.7); }
            else grDb = grFast;
            const double g = dbToGain(-grDb) * makeup.next(dbToGain(mk));
            const double m = mixS.next(p.mix);
            double outPk = 0;
            for (int c = 0; c < nch; c++) {
                const double x = ch[c][i], y = x * g;
                const double o = x + (y - x) * m;
                ch[c][i] = (float)o;
                outPk = std::max(outPk, std::abs(y));
            }
            fbLevel = outPk;
        }
    }
private:
    double coef(double ms_) const { return std::exp(-1.0 / (std::max(ms_, 0.01) * 0.001 * sr)); }
    double makeupTarget() const { return p.makeupDb + (p.autoMakeup ? autoMakeupDb(p.thresh, p.ratio) : 0.0); }
    CompParams p; double sr = 48000;
    double grFast = 0, grSlow = 0, grDb = 0, ms = 0, fbLevel = 0;
    std::array<Svf, 2> hpf; Smooth makeup, mixS;
};
} // namespace lgn
