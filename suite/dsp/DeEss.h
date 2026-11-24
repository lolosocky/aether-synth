// LGN De-Esser. Detects energy above a chosen frequency and turns it down only while it is over the threshold.
//   Split  a dynamic high shelf at the frequency: only the top end dips, the body of the voice is untouched
//   Wide   reduces the whole signal, for gentler, more natural de-essing
//   Listen outputs the detection band so you can hear exactly what is being caught
#pragma once
#include "Common.h"

namespace lgn {

struct DeEssParams { double freq = 6500, thresh = -28, rangeDb = 8, smoothMs = 60; int mode = 0; bool listen = false; };

class DeEssDsp {
public:
    enum Mode { split, wide };
    void prepare(double rate) { sr = rate; for (auto& f : hp) f.reset(); for (auto& f : shelf) f.reset(); env = 0; grDb = 0; detDb = -120; freqS.setTime(sr / 16, 0.03); freqS.reset(std::log(p.freq)); counter = 0; }
    void set(const DeEssParams& np) { p = np; }
    double reductionDb() const { return grDb; }
    double detectorDb() const { return detDb; }

    void process(float* const* ch, int nch, int n) {
        nch = std::min(nch, 2);
        const double att = std::exp(-1.0 / (0.0005 * sr)), rel = std::exp(-1.0 / (std::max(p.smoothMs, 5.0) * 0.001 * sr));
        const double ratio = 6.0, knee = 6.0;
        double maxDet = 0;
        for (int i = 0; i < n; i++) {
            if ((counter++ & 15) == 0) {
                const double f = std::exp(freqS.next(std::log(p.freq)));
                for (auto& h : hp) h.set(Svf::highpass, sr, f, 0.707);
                for (auto& h : shelf) h.set(Svf::highpass, sr, f, 0.707);   // only the core coefficients are used
            }
            double hi[2] = { 0, 0 }, pk = 0;
            for (int c = 0; c < nch; c++) { hi[c] = hp[(size_t)c].process(ch[c][i]); pk = std::max(pk, std::abs(hi[c])); }
            env = pk > env ? pk + (env - pk) * att : pk + (env - pk) * rel;
            maxDet = std::max(maxDet, env);
            const double lvl = gainToDb(env), over = lvl - p.thresh;
            double gr = 0;
            if (2 * over > knee) gr = over * (1 - 1 / ratio);
            else if (2 * over > -knee) gr = (1 - 1 / ratio) * (over + knee / 2) * (over + knee / 2) / (2 * knee);
            gr = std::min(gr, p.rangeDb);
            grDb = gr;
            const double g = dbToGain(-gr);
            // dynamic high shelf: H = (A^2 s^2 + k A s + 1) / (s^2 + k s + 1), A^2 = g. With g = 1 it is exactly flat.
            const double A = std::sqrt(g), k = 1.41421356;
            for (int c = 0; c < nch; c++) {
                const double x = ch[c][i]; double v1, v2; shelf[(size_t)c].tick(x, v1, v2);
                if (p.listen) ch[c][i] = (float)hi[c];
                else if (p.mode == split) ch[c][i] = (float)(g * x + k * (1 - A) * A * v1 + (1 - g) * v2);
                else ch[c][i] = (float)(x * g);
            }
        }
        detDb = gainToDb(maxDet);
    }
private:
    DeEssParams p; double sr = 48000, env = 0, grDb = 0, detDb = -120; std::array<Svf, 2> hp, shelf; Smooth freqS; unsigned counter = 0;
};
} // namespace lgn
