// LGN Reverb: an eight-line feedback delay network.
// Pre-delay -> input diffusion (four allpasses per side) -> 8 modulated delay lines mixed by a Householder matrix,
// each with a damping filter and a gain set from the decay time, so RT60 is what the knob says at low frequencies.
// Characters change the line lengths, diffusion and movement:
//   Room   short, dense, little movement     Plate  bright and instantly dense
//   Hall   long and smooth                   Space  huge, slowly drifting, for washes and ambience
#pragma once
#include "Common.h"

namespace lgn {

struct VerbParams { double sizePct = 60, decayS = 2.4, preMs = 20, dampHz = 7000, lowCutHz = 120, widthPct = 100, mix = 0.25, modPct = 30; int character = 2; bool freeze = false; };

class VerbDsp {
public:
    enum Character { room, plate, hall, space };
    static constexpr int kLines = 8;

    void prepare(double rate) {
        sr = rate;
        pre.prepare((int)(0.26 * sr) + 8);
        for (auto& d : lines) d.prepare((int)(0.22 * sr * 1.6) + 64);
        for (auto& side : diff) for (auto& a : side) a.line.prepare((int)(0.02 * sr) + 8);
        for (auto& f : damp) f = 0;
        for (auto& f : lowCut) f.reset();
        for (int i = 0; i < kLines; i++) lfoPhase[i] = i / (double)kLines;
        sizeS.setTime(sr, 0.25); sizeS.reset(p.sizePct); mixS.setTime(sr, 0.02); mixS.reset(p.mix);
        preS.setTime(sr, 0.05); preS.reset(p.preMs);
        fade = p.freeze ? 1.0 : 0.0;
        counter = 0; update(true);
    }
    void set(const VerbParams& np) { p = np; }
    void reset() { pre.reset(); for (auto& d : lines) d.reset(); for (auto& side : diff) for (auto& a : side) a.line.reset(); for (auto& f : damp) f = 0; for (auto& f : lowCut) f.reset(); }

    void process(float* const* ch, int nch, int n) {
        nch = std::min(nch, 2);
        for (int i = 0; i < n; i++) {
            if ((counter++ & 31) == 0) update(false);
            const double inL = ch[0][i], inR = nch > 1 ? ch[1][i] : inL;
            // freeze: the input fades out and the loop holds
            const double inGain = 1.0 - fade;
            pre.push((float)(0.5 * (inL + inR) * inGain));
            const double pd = std::max(1.0, preS.next(p.preMs) * 0.001 * sr);
            const double mono = pre.read(pd);
            double side[2] = { mono + (inL - inR) * 0.25 * inGain, mono - (inL - inR) * 0.25 * inGain };
            for (int s = 0; s < 2; s++) for (auto& a : diff[(size_t)s]) side[s] = a.process(side[s], diffusion);
            // read the delay lines (modulated), mix with the Householder matrix, feed back
            double y[kLines], sum = 0;
            for (int k = 0; k < kLines; k++) {
                lfoPhase[k] += lfoRate[k] / sr; if (lfoPhase[k] >= 1) lfoPhase[k] -= 1;
                const double d = len[k] + modDepth * std::sin(2 * kPi * lfoPhase[k]);
                y[k] = lines[(size_t)k].read(std::max(4.0, d));
                sum += y[k];
            }
            const double h = sum * (2.0 / kLines);
            double outL = 0, outR = 0;
            for (int k = 0; k < kLines; k++) {
                double v = y[k] - h;
                // damping: one-pole low-pass in the loop, blended toward flat when frozen
                damp[k] += dampC * (v - damp[k]);
                v = damp[k] * (1 - fade) + v * fade;
                const double fb = v * (g[k] * (1 - fade) + fade);
                const double inj = (k & 1) ? side[1] : side[0];
                lines[(size_t)k].push((float)(fb + inj * 0.35 * (k < 4 ? 1.0 : -1.0)));
                const double tap = y[k];
                outL += tap * outSignL[k]; outR += tap * outSignR[k];
            }
            outL *= 0.42; outR *= 0.42;
            // width (mid/side) and low cut on the wet signal
            const double mid = 0.5 * (outL + outR), sid = 0.5 * (outL - outR) * (p.widthPct / 100.0);
            double wl = lowCut[0].process(mid + sid), wr = lowCut[1].process(mid - sid);
            // equal-power mix: dry stays near unity at typical insert settings (25% mix: dry -0.7 dB)
            const double m = mixS.next(p.mix);
            if (m != lastMix) { lastMix = m; dryG = std::cos(m * 1.5707963267948966); wetG = std::sin(m * 1.5707963267948966); if (m >= 1.0) dryG = 0; }
            ch[0][i] = (float)(inL * dryG + wl * wetG);
            if (nch > 1) ch[1][i] = (float)(inR * dryG + wr * wetG);
            wetPeak = std::max(wetPeak, std::max(std::abs(wl), std::abs(wr)));
        }
    }
    double takeWetPeak() { const double v = wetPeak; wetPeak = 0; return v; }

private:
    struct Allpass {
        DelayLine line; int d = 100;
        double process(double x, double gain) {
            const double z = line.tap(d - 1);
            const double v = x + gain * z;
            line.push((float)v);
            return z - gain * v;
        }
    };
    void update(bool snap) {
        static const double baseMs[4][kLines] = {
            { 11.3, 13.7, 16.1, 17.9, 19.3, 21.7, 23.9, 26.3 },        // room
            { 17.1, 19.9, 22.3, 24.7, 27.1, 29.3, 31.7, 34.1 },        // plate
            { 29.7, 37.1, 41.1, 43.7, 53.3, 59.9, 67.7, 73.1 },        // hall
            { 47.3, 57.1, 67.9, 79.3, 89.9, 101.3, 113.9, 127.1 } };   // space
        static const double diffMs[4] = { 4.77, 3.59, 12.73, 9.31 };
        const int c = clampv(p.character, 0, 3);
        const double size = snap ? p.sizePct : sizeS.next(p.sizePct);
        if (!snap) for (int k = 0; k < 31; k++) sizeS.next(p.sizePct);   // update runs every 32 samples
        const double scale = 0.35 + 1.05 * size / 100.0;
        for (int k = 0; k < kLines; k++) {
            len[k] = baseMs[c][k] * scale * 0.001 * sr;
            // decay: each line loses 60 dB over RT60 seconds
            g[k] = std::pow(10.0, -3.0 * (len[k] / sr) / std::max(0.05, p.decayS));
            lfoRate[k] = (c == space ? 0.07 : 0.23) + 0.11 * k / kLines;
        }
        if (snap) for (int s = 0; s < 2; s++) for (int a = 0; a < 4; a++) diff[(size_t)s][(size_t)a].d = std::max(2, (int)(diffMs[a] * (1 + 0.13 * s) * 0.001 * sr));
        static const double diffAmt[4] = { 0.62, 0.72, 0.68, 0.7 }, modMul[4] = { 0.4, 0.5, 1.0, 2.6 };
        diffusion = diffAmt[c];
        modDepth = p.modPct / 100.0 * 0.0016 * sr * modMul[c];
        dampC = 1 - std::exp(-2 * kPi * clampv(p.dampHz, 500.0, sr * 0.45) / sr);
        fade += ((p.freeze ? 1.0 : 0.0) - fade) * (snap ? 1.0 : 0.02);
        for (auto& f : lowCut) f.set(Svf::highpass, sr, p.lowCutHz, 0.707);
    }
    VerbParams p; double sr = 48000;
    DelayLine pre; std::array<DelayLine, kLines> lines; std::array<std::array<Allpass, 4>, 2> diff;
    double len[kLines] = {}, g[kLines] = {}, damp[kLines] = {}, lfoPhase[kLines] = {}, lfoRate[kLines] = {};
    double diffusion = 0.7, modDepth = 0, dampC = 0.5, fade = 0, wetPeak = 0;
    const double outSignL[kLines] = { 1, -1, 1, 1, -1, 1, -1, -1 }, outSignR[kLines] = { 1, 1, -1, 1, 1, -1, -1, 1 };
    std::array<Svf, 2> lowCut; double lastMix = -1, dryG = 1, wetG = 0; Smooth sizeS, mixS, preS; unsigned counter = 0;
};
} // namespace lgn
