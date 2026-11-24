// LGN Saturator: four characters of harmonic drive. The shaper runs on oversampled audio (the plugin oversamples
// 4x); tone, DC removal, auto gain and mix run at the base rate.
//   Tape        soft, symmetric (odd harmonics), rounds peaks
//   Tube        asymmetric (strong 2nd harmonic), warm and thick
//   Transistor  firm knee, closer to a clipper, adds bite
//   Fuzz        rectified and pushed hard, for distortion as an effect
#pragma once
#include "Common.h"

namespace lgn {

struct SatParams { double driveDb = 6, tone = 0, mix = 1, outDb = 0; int mode = 0; bool autoGain = true; };

class SatDsp {
public:
    enum Mode { tape, tube, transistor, fuzz, numModes };

    static double shape(int mode, double x) {
        switch (mode) {
            case tape: return std::tanh(x);
            case tube: {   // biased so positive and negative halves bend differently, then re-centred
                const double b = 0.35, y = std::tanh(x + b) - std::tanh(b);
                return y / (1 - std::tanh(b) * std::tanh(b));
            }
            case transistor: { const double p = 6.0; return x / std::pow(1 + std::pow(std::abs(x), p), 1 / p); }
            default: { const double a = x + 0.45 * std::abs(x); return std::tanh(2.0 * a) * 0.8; }
        }
    }

    // Level compensation so turning drive up changes the colour, not the loudness. Measured on a -18 dBFS sine.
    void buildAutoGain() {
        for (int m = 0; m < numModes; m++)
            for (int d = 0; d <= 48; d++) {
                const double g = dbToGain(d); double si = 0, so = 0;
                for (int i = 0; i < 2048; i++) {
                    const double x = 0.125 * std::sin(2 * kPi * i / 64.0);
                    double y = shape(m, x * g);
                    si += x * x; so += y * y;
                }
                // DC from asymmetric modes is removed later, so measure AC only
                comp[(size_t)m][(size_t)d] = std::sqrt(si / std::max(so, 1e-12));
            }
    }
    double autoGain(int mode, double driveDb) const {
        const double d = clampv(driveDb, 0.0, 48.0); const int i = std::min(47, (int)d); const double t = d - i;
        return comp[(size_t)mode][(size_t)i] * (1 - t) + comp[(size_t)mode][(size_t)i + 1] * t;
    }

    void prepare(double rate) {
        sr = rate; buildAutoGain();
        for (auto& c : dc) c.reset();
        for (auto& c : lowS) c.reset();
        for (auto& c : highS) c.reset();
        driveCur = p.driveDb; out.setTime(sr, 0.02); out.reset(dbToGain(p.outDb) * (p.autoGain ? autoGain(p.mode, driveCur) : 1.0)); toneS.setTime(sr, 0.03); toneS.reset(p.tone);
    }
    void set(const SatParams& np) { p = np; }

    // Oversampled stage: one or two channels at osRate, length n. Drive glides toward its target (20 ms).
    void shapeBlock(float* const* x, int nch, int n, double osRate) {
        const double a = std::exp(-(double)n / (0.02 * osRate));
        const double d0 = driveCur, d1 = p.driveDb + (driveCur - p.driveDb) * a;
        const double g0 = dbToGain(d0), g1 = dbToGain(d1);
        for (int c = 0; c < std::min(nch, 2); c++)
            for (int i = 0; i < n; i++) x[c][i] = (float)shape(p.mode, x[c][i] * (g0 + (g1 - g0) * (i + 1) / n));
        driveCur = d1;
    }
    double currentDriveDb() const { return driveCur; }

    // Base-rate stage after downsampling: DC removal, tilt tone, gain staging.
    void post(float* const* wet, int nch, int n) {
        const double tone = toneS.next(p.tone);
        const double outG = dbToGain(p.outDb) * (p.autoGain ? autoGain(p.mode, driveCur) : 1.0);
        for (int c = 0; c < std::min(nch, 2); c++) {
            dc[(size_t)c].set(Svf::highpass, sr, 8.0, 0.707);
            lowS[(size_t)c].set(Svf::lowShelf, sr, 900, 0.5, -tone * 6.0);
            highS[(size_t)c].set(Svf::highShelf, sr, 1100, 0.5, tone * 6.0);
            float* w = wet[c];
            for (int i = 0; i < n; i++) {
                double y = dc[(size_t)c].process(w[i]);
                if (std::abs(tone) > 1e-3) y = highS[(size_t)c].process(lowS[(size_t)c].process(y));
                w[i] = (float)y;
            }
        }
        const double g0 = out.y, g1 = out.next(outG);
        for (int c = 0; c < std::min(nch, 2); c++) for (int i = 0; i < n; i++) wet[c][i] = (float)(wet[c][i] * (g0 + (g1 - g0) * (i + 1) / n));
    }
    const SatParams& params() const { return p; }
private:
    SatParams p; double sr = 48000;
    std::array<std::array<double, 49>, numModes> comp{};
    std::array<Svf, 2> dc, lowS, highS; Smooth out, toneS; double driveCur = 6;
};
} // namespace lgn
