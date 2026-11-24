// LGN EQ: seven bands (low cut, low shelf, three bells, high shelf, high cut).
// Parameters are smoothed per sample block of 16 so sweeps and automation never zipper or click.
#pragma once
#include "Common.h"

namespace lgn {

struct EqBandParams { bool on = false; double freq = 1000, gain = 0, q = 0.707; int slope = 1; };   // slope: 0 = 12, 1 = 24, 2 = 48 dB/oct

class EqDsp {
public:
    static constexpr int kBands = 7;
    enum Band { lowCut, lowShelf, bell1, bell2, bell3, highShelf, highCut };
    static int sectionsFor(int slope) { return slope <= 0 ? 1 : slope == 1 ? 2 : 4; }
    static bool isCut(int b) { return b == lowCut || b == highCut; }

    void prepare(double rate, int chans) {
        sr = rate; nch = std::min(chans, 2);
        for (auto& st : state) { for (auto& c : st.svf) for (auto& f : c) f.reset(); st.sections = sectionsFor(st.target.slope); }
        out.setTime(sr / kBlock, 0.03); out.reset(outTarget);
        coeffs(true);
    }
    void setBand(int b, const EqBandParams& p) { state[(size_t)b].target = p; }
    void setOutputDb(double db) { outTarget = dbToGain(db); }

    void process(float* const* ch, int chans, int n) {
        for (int off = 0; off < n; off += kBlock) {
            const int m = std::min(kBlock, n - off);
            coeffs(false);
            const double g0 = out.y, g1 = out.next(outTarget);
            for (int c = 0; c < std::min(chans, nch); c++) {
                float* x = ch[c] + off;
                for (int bi = 0; bi < kBands; bi++) {
                    auto& st = state[(size_t)bi];
                    if (!st.active) continue;
                    auto& f = st.svf[(size_t)c];
                    if (isCut(bi)) {
                        const int ns = st.sections; const double w0 = st.wPrev, w1 = st.w.y;
                        for (int i = 0; i < m; i++) {
                            double y = x[i];
                            for (int s = 0; s < ns; s++) y = f[(size_t)s].process(y);
                            const double w = w0 + (w1 - w0) * (i + 1) / m;
                            x[i] = (float)(x[i] + w * (y - x[i]));
                        }
                    } else {
                        for (int i = 0; i < m; i++) x[i] = (float)f[0].process(x[i]);
                    }
                }
                for (int i = 0; i < m; i++) x[i] = (float)(x[i] * (g0 + (g1 - g0) * (i + 1) / m));
            }
        }
    }

    // Magnitude of one band (as currently set by its parameters) at frequency f. Used by the display.
    static double bandMagnitude(int b, const EqBandParams& p, double f, double rate) {
        if (!p.on) return 1.0;
        Svf s;
        if (isCut(b)) {
            double m = 1; const int ns = sectionsFor(p.slope);
            for (int i = 0; i < ns; i++) { s.set(b == lowCut ? Svf::highpass : Svf::lowpass, rate, p.freq, butterQ(ns, i)); m *= s.magnitude(f, rate); }
            return m;
        }
        s.set(typeFor(b), rate, p.freq, p.q, p.gain);
        return s.magnitude(f, rate);
    }

private:
    static constexpr int kBlock = 16;
    static Svf::Type typeFor(int b) { return b == lowShelf ? Svf::lowShelf : b == highShelf ? Svf::highShelf : Svf::bell; }
    struct State {
        EqBandParams target;
        Smooth f, g, q, w;                  // frequency (log), gain dB, Q (log), cut wet amount
        double wPrev = 0; int sections = 1; bool active = false;
        std::array<std::array<Svf, 4>, 2> svf;
    };
    void coeffs(bool snap) {
        const double t = snap ? 0 : 0.03;   // 30 ms glide
        for (int bi = 0; bi < kBands; bi++) {
            auto& st = state[(size_t)bi];
            for (auto* s : { &st.f, &st.g, &st.q, &st.w }) s->setTime(sr / kBlock, t);
            const auto& p = st.target;
            const double lf = std::log(std::max(p.freq, 10.0)), lq = std::log(std::max(p.q, 0.05));
            if (snap) { st.f.reset(lf); st.q.reset(lq); st.g.reset(p.on ? p.gain : 0); st.w.reset(p.on ? 1 : 0); }
            const double fr = std::exp(st.f.next(lf)), q = std::exp(st.q.next(lq));
            if (isCut(bi)) {
                st.wPrev = st.w.y; st.w.next(p.on ? 1.0 : 0.0);
                const int ns = sectionsFor(p.slope);
                if (ns != st.sections) { st.sections = ns; for (auto& c : st.svf) for (auto& f : c) f.reset(); }
                st.active = st.w.y > 1e-4 || st.wPrev > 1e-4;
                if (!st.active) { for (auto& c : st.svf) for (auto& f : c) f.reset(); continue; }
                for (auto& c : st.svf) for (int s = 0; s < ns; s++) c[(size_t)s].set(bi == lowCut ? Svf::highpass : Svf::lowpass, sr, fr, butterQ(ns, s));
            } else {
                const double g = st.g.next(p.on ? p.gain : 0.0);
                st.active = std::abs(g) > 1e-4 || std::abs(p.on ? p.gain : 0.0) > 1e-4;
                if (!st.active) { for (auto& c : st.svf) c[0].reset(); continue; }
                for (auto& c : st.svf) c[0].set(typeFor(bi), sr, fr, q, g);
            }
        }
    }
    double sr = 48000; int nch = 2;
    std::array<State, kBands> state;
    Smooth out; double outTarget = 1;
};
} // namespace lgn
