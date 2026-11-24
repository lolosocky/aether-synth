// LGN Limiter: lookahead brickwall with optional true-peak detection.
// Gain needed per sample = ceiling / peak. A sliding-window minimum over the lookahead (a monotonic queue, O(1)),
// an instant-attack / smooth-release envelope, then a moving average over the same window: the gain ramps down
// before each peak arrives and the sample peaks of the delayed output never pass the ceiling.
// True peak: the detector also looks at 4x-interpolated points between samples, so peaks that only appear after
// a converter's reconstruction filter are caught too.
#pragma once
#include "Common.h"
#include <deque>

namespace lgn {

constexpr double kTpMargin = 0.15;   // dB of headroom kept in true-peak mode for what 4x detection cannot see

struct LimitParams { double inputDb = 0, ceilingDb = -0.3, releaseMs = 80, lookaheadMs = 2; bool truePeak = true, autoRelease = true; };

class LimitDsp {
public:
    static constexpr int kTaps = 24;    // interpolator half-length (samples each side)

    void prepare(double rate) {
        sr = rate;
        W = std::max(8, (int)std::lround(p.lookaheadMs * 0.001 * sr));
        // interpolation filter: windowed sinc for phases 1/4, 2/4, 3/4
        for (int ph = 1; ph < 4; ph++)
            for (int k = 0; k < 2 * kTaps; k++) {
                const double t = (k - kTaps + 1) - ph / 4.0;   // tap k multiplies x[m - kTaps + 1 + k]
                const double sinc = std::abs(t) < 1e-9 ? 1.0 : std::sin(kPi * t) / (kPi * t);
                const double w = 0.42 + 0.5 * std::cos(kPi * t / kTaps) + 0.08 * std::cos(2 * kPi * t / kTaps);   // Blackman
                interp[(size_t)ph - 1][(size_t)k] = sinc * w;
            }
        // history of exactly 2 * kTaps samples, written twice so any window is contiguous (no wrap in the inner loop)
        xin[0].assign((size_t)(4 * kTaps), 0.0); xin[1].assign((size_t)(4 * kTaps), 0.0); hpos = 0;
        delay = W + kTaps;   // audio waits for the interpolator's look-ahead plus the gain ramp
        for (auto& d : dl) d.assign((size_t)delay, 0.0f);
        dpos = 0;
        mins.assign((size_t)W, 1.0); sum = W; mpos = 0; env = 1; slow = 1;
        qv.assign((size_t)W + 4, 1.0); qi.assign((size_t)W + 4, 0); qh = qt = qn = 0; t = 0;
        inS.setTime(sr, 0.02); inS.reset(dbToGain(p.inputDb));
        grDb = 0;
    }
    int latency() const { return delay; }
    void set(const LimitParams& np) { p = np; }
    double reductionDb() const { return grDb; }

    void process(float* const* ch, int nch, int n) {
        nch = std::min(nch, 2);
        const double ceil = dbToGain(p.ceilingDb) * (p.truePeak ? dbToGain(-kTpMargin) : 1.0);
        const double relC = 1 - std::exp(-1.0 / (std::max(1.0, p.releaseMs) * 0.001 * sr));
        const double relSlow = 1 - std::exp(-1.0 / (std::max(1.0, p.releaseMs) * 6 * 0.001 * sr));
        const int Q = W + 4, hist = 2 * kTaps;
        double deepest = 1;
        const double inTarget = dbToGain(p.inputDb);   // constant for the block: computed once, not per sample
        for (int i = 0; i < n; i++) {
            const double gin = inS.next(inTarget);
            double x[2] = { ch[0][i] * gin, (nch > 1 ? ch[1][i] : ch[0][i]) * gin };
            // detection on the newest sample and the interpolated points just before the sample kTaps back
            for (int c = 0; c < 2; c++) xin[(size_t)c][(size_t)hpos] = xin[(size_t)c][(size_t)(hpos + hist)] = x[c];
            if (++hpos == hist) hpos = 0;
            double a = 0;
            for (int c = 0; c < nch; c++) {
                const double* w = xin[(size_t)c].data() + hpos;   // oldest .. newest
                const double centre = w[kTaps - 1];
                a = std::max(a, std::abs(centre));
                if (p.truePeak)
                    for (int ph = 0; ph < 3; ph++) {
                        double s = 0;
                        const double* h = interp[(size_t)ph].data();
                        for (int k = 0; k < 2 * kTaps; k++) s += h[k] * w[k];
                        a = std::max(a, std::abs(s));
                    }
            }
            const double req = a > ceil ? ceil / a : 1.0;
            // sliding-window minimum over the last W + 1 requests
            while (qn > 0) { const int b = (qt + Q - 1) % Q; if (qv[(size_t)b] >= req) { qt = b; qn--; } else break; }
            qv[(size_t)qt] = req; qi[(size_t)qt] = t; qt = (qt + 1) % Q; qn++;
            while (qi[(size_t)qh] <= t - (W + 1)) { qh = (qh + 1) % Q; qn--; }
            t++;
            const double m = std::min(1.0, qv[(size_t)qh]);
            // release: with auto release, sustained limiting recovers slowly and short peaks recover fast
            if (m < env) env = m; else env += (1 - env) * relC;
            if (p.autoRelease) { if (m < slow) slow += (m - slow) * 0.002; else slow += (1 - slow) * relSlow; env = std::min(env, std::max(m, slow)); }
            if (env > m) env = m;
            sum += env - mins[(size_t)mpos]; mins[(size_t)mpos] = env;
            const double gg = std::min(1.0, sum / W);
            mpos = (mpos + 1) % W; if (mpos == 0) { sum = 0; for (double v : mins) sum += v; }
            // delayed audio out
            for (int c = 0; c < nch; c++) {
                auto& d = dl[(size_t)c];
                const float old = d[(size_t)dpos]; d[(size_t)dpos] = (float)x[c];
                double y = old * gg;
                y = clampv(y, -ceil / (p.truePeak ? dbToGain(-kTpMargin) : 1.0), ceil / (p.truePeak ? dbToGain(-kTpMargin) : 1.0));   // last-resort safety, should never act
                ch[c][i] = (float)y;
            }
            dpos = (dpos + 1) % delay;
            deepest = std::min(deepest, gg);
        }
        grDb = -gainToDb(deepest);
    }
private:
    LimitParams p; double sr = 48000;
    int W = 96, delay = 112, dpos = 0, hpos = 0, mpos = 0, qh = 0, qt = 0, qn = 0; long long t = 0;
    std::array<std::array<double, 2 * kTaps>, 3> interp{};
    std::array<std::vector<double>, 2> xin;
    std::array<std::vector<float>, 2> dl;
    std::vector<double> mins, qv; std::vector<long long> qi; double sum = 0, env = 1, slow = 1, grDb = 0;
    Smooth inS;
};
} // namespace lgn
