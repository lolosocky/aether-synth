// Output effects after the voice engine: EQ, stereo delay, reverb send/return, compressor, width, limiter, soft clip.
// Mirrors the browser version's native chain. The reverb convolution itself is supplied by the host wrapper.
#pragma once
#include "AetherEngine.h"

namespace aether {

struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] = { 0, 0 }, z2[2] = { 0, 0 };
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    void set(double B0, double B1, double B2, double A0, double A1, double A2) { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
    void lowShelf(double f, double dB, double sr) { const double A = std::pow(10.0, dB / 40), w = TAU * clampd(f, 10, sr * 0.49) / sr, c = std::cos(w), k = 2 * std::sqrt(A) * std::sin(w) * 0.5 * std::sqrt(2.0);
        set(A * ((A + 1) - (A - 1) * c + k), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - k), (A + 1) + (A - 1) * c + k, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - k); }
    void highShelf(double f, double dB, double sr) { const double A = std::pow(10.0, dB / 40), w = TAU * clampd(f, 10, sr * 0.49) / sr, c = std::cos(w), k = 2 * std::sqrt(A) * std::sin(w) * 0.5 * std::sqrt(2.0);
        set(A * ((A + 1) + (A - 1) * c + k), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - k), (A + 1) - (A - 1) * c + k, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - k); }
    void peak(double f, double q, double dB, double sr) { const double A = std::pow(10.0, dB / 40), w = TAU * clampd(f, 10, sr * 0.49) / sr, c = std::cos(w), al = std::sin(w) / (2 * q);
        set(1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A); }
    void lowPass(double f, double qdB, double sr) { const double w = TAU * clampd(f, 10, sr * 0.49) / sr, c = std::cos(w), al = std::sin(w) / (2 * std::pow(10.0, qdB / 20));
        set((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al); }
    void highPass(double f, double qdB, double sr) { const double w = TAU * clampd(f, 10, sr * 0.49) / sr, c = std::cos(w), al = std::sin(w) / (2 * std::pow(10.0, qdB / 20));
        set((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al); }
    inline double tick(double x, int ch) { const double y = b0 * x + z1[ch]; z1[ch] = b1 * x - a1 * y + z2[ch]; z2[ch] = b2 * x - a2 * y; return y; }
    void flush() { for (int c = 0; c < 2; c++) { fl0(z1[c]); fl0(z2[c]); } }
};

// Feed-forward compressor with a soft knee and the same style of automatic make-up gain a browser compressor applies.
struct Dyn {
    double thr = 0, ratio = 1, knee = 0, att = 0.01, rel = 0.2, gr = 0, makeup = 1;
    double curve(double dB) const {
        if (ratio <= 1.0001) return dB;
        if (knee > 0.01 && dB > thr && dB < thr + knee) { const double x = dB - thr; return dB + (1 / ratio - 1) * x * x / (2 * knee); }
        if (dB >= thr + knee) return thr + knee * 0.5 * (1 + 1 / ratio) + (dB - thr - knee) / ratio;
        return dB;
    }
    void set(double t, double r, double k) { thr = t; ratio = r; knee = k; makeup = std::pow(10.0, -curve(0) * 0.6 / 20); }
    void process(float* L, float* R, int n, double sr) {
        if (ratio <= 1.0001 && gr > -0.01) { gr = 0; return; }
        const double ca = 1 - std::exp(-1 / (att * sr)), cr = 1 - std::exp(-1 / (rel * sr));
        double g = gr;
        for (int i = 0; i < n; i++) {
            const double a = std::max(std::abs((double)L[i]), std::abs((double)R[i]));
            const double dB = a > 1e-6 ? 20 * std::log10(a) : -120, target = curve(dB) - dB;
            g += (target - g) * (target < g ? ca : cr);
            const double lin = std::pow(10.0, g / 20) * makeup;
            L[i] = (float)(L[i] * lin); R[i] = (float)(R[i] * lin);
        }
        gr = (g > -1e-6) ? 0 : g;
    }
};

// Reverb impulse response, identical to the browser version's generator, already scaled the way a normalising convolver would.
inline void makeIR(int type, double decayIn, double sizeIn, double dampIn, double sr, std::vector<float>& outL, std::vector<float>& outR) {
    const double decay = clampd(decayIn > 0 ? decayIn : 2, 0.2, 14), size = clampd(sizeIn, 0, 1), damp = clampd(dampIn, 0, 1);
    const int len = std::max(256, (int)std::floor(std::min(decay * 1.3 + 0.15, 15.0) * sr + 0.5));
    uint32_t s = (uint32_t)(0x1234567 + type * 7919 + (int)std::floor(size * 1000 + 0.5));
    auto rnd = [&s]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s / 4294967296.0) * 2 - 1; };
    const double build = type == 3 ? 0.18 + size * 0.35 : type == 2 ? 0.003 : 0.008 + size * 0.03;
    const int erN = type == 1 ? 16 : type == 0 ? 10 : 0;
    const double k = std::exp(-6.9078 / (decay * sr));
    double power = 0;
    for (int ch = 0; ch < 2; ch++) {
        std::vector<float>& d = ch ? outR : outL; d.assign((size_t)len, 0.0f);
        double env = 1, lp = 0, lp2 = 0, a = 0.9;
        for (int i = 0; i < len; i++) {
            const double t = i / sr;
            if ((i & 63) == 0) {
                const double prog = std::min(1.0, t / (decay * 0.7));
                const double fc = (type == 2 ? 15000.0 : 12000.0) * std::pow(1 - 0.88 * damp, prog) + 300;
                a = 1 - std::exp(-TAU * std::min(fc, sr * 0.45) / sr);
            }
            lp += a * (rnd() - lp); lp2 += a * (lp - lp2);
            const double b = t < build ? std::sin((t / build) * PI / 2) : 1;
            d[(size_t)i] = (float)((type == 2 ? lp : lp2) * env * b);
            env *= k;
        }
        for (int e = 0; e < erN; e++) {
            const double tt = 0.003 + (0.01 + size * 0.07) * (e + 0.5 + rnd() * 0.4) / erN; const int idx = (int)std::floor(tt * sr + 0.5);
            const double sign = rnd() > 0 ? 1 : -1, mag = 0.6 + 0.4 * std::abs(rnd());
            if (idx < len) d[(size_t)idx] = (float)(d[(size_t)idx] + sign * 0.55 * (1 - (double)e / erN) * mag);
        }
        for (int i = 0; i < len; i++) power += (double)d[(size_t)i] * d[(size_t)i];
    }
    // equal-power normalisation (calibrated at 44.1 kHz), as browsers do for convolver buffers
    power = std::sqrt(power / (2.0 * len)); if (!(power > 1e-6)) power = 1e-6;
    const double scale = (1 / power) * 0.00125 * (44100.0 / sr);
    for (int i = 0; i < len; i++) { outL[(size_t)i] = (float)(outL[(size_t)i] * scale); outR[(size_t)i] = (float)(outR[(size_t)i] * scale); }
}

class PostFX {
public:
    void prepare(double sampleRate) {
        sr = sampleRate; dlen = 1; while (dlen < (int)(4.2 * sr)) dlen <<= 1;
        for (auto& d : dbuf) d.assign((size_t)dlen, 0.0f);
        plen = 1; while (plen < (int)(0.6 * sr)) plen <<= 1;
        for (auto& d : pbuf) d.assign((size_t)plen, 0.0f);
        reset(); first = true;
    }
    void reset() {
        for (auto& d : dbuf) std::fill(d.begin(), d.end(), 0.0f);
        for (auto& d : pbuf) std::fill(d.begin(), d.end(), 0.0f);
        eqL.reset(); eqM.reset(); eqH.reset(); lp.reset(); hp.reset(); rlc.reset(); comp.gr = 0; lim.gr = 0; fbL = fbR = 0; dw = pw = 0;
    }
    // Stage 1: EQ and delay in place; writes the pre-delayed reverb send. fxm = engine's effect modulation offsets.
    void stage1(const Patch& P, const double* fxm, double bpm, float* L, float* R, float* sendL, float* sendR, int n) {
        const auto& F = P.fx; const double k = first ? 1 : 1 - std::exp(-n / (0.025 * sr));
        const double lo = F.eq.on ? clampd(F.eq.low + fxm[8] * 12, -18, 18) : 0, mi = F.eq.on ? clampd(F.eq.mid, -18, 18) : 0, hi = F.eq.on ? clampd(F.eq.high + fxm[9] * 12, -18, 18) : 0;
        sLo += (lo - sLo) * k; sMi += (mi - sMi) * k; sHi += (hi - sHi) * k; sMf += (clampd(F.eq.mf, 100, 12000) - sMf) * k;
        const bool eqActive = std::abs(sLo) > 0.01 || std::abs(sMi) > 0.01 || std::abs(sHi) > 0.01;
        if (eqActive) {
            eqL.lowShelf(120, sLo, sr); eqM.peak(sMf, 0.8, sMi, sr); eqH.highShelf(7000, sHi, sr);
            for (int i = 0; i < n; i++) { L[i] = (float)eqH.tick(eqM.tick(eqL.tick(L[i], 0), 0), 0); R[i] = (float)eqH.tick(eqM.tick(eqL.tick(R[i], 1), 1), 1); }
            eqL.flush(); eqM.flush(); eqH.flush();
        }
        const auto& d = F.dly;
        const double mixT = d.on ? clampd(d.mix + fxm[5], 0, 1) : 0, fbT = clampd(d.fb + fxm[6], 0, 0.95);
        const double timeT = clampd(d.sync ? d.div * 60 / (bpm > 1 ? bpm : 90) : d.time, 0.01, 3.9);
        const double mix0 = sMix, fb0 = sFb, pp0 = sPP, t0 = sTime;
        sMix += (mixT - sMix) * k; sFb += (fbT - sFb) * k; sPP += ((d.pp ? 1.0 : 0.0) - sPP) * k; sTime += (timeT - sTime) * k;
        sHc += (clampd(d.hc, 500, 18000) - sHc) * k; sLc += (clampd(d.lc, 20, 2000) - sLc) * k; sMod += (clampd(d.mod, 0, 1) * 0.004 - sMod) * k;
        const bool dlyActive = sMix > 1e-4 || mix0 > 1e-4;
        if (!dlyActive && !dclear) { for (auto& b : dbuf) std::fill(b.begin(), b.end(), 0.0f); fbL = fbR = 0; lp.reset(); hp.reset(); dclear = true; }
        if (dlyActive) {
            dclear = false;
            lp.lowPass(sHc, 0.5, sr); hp.highPass(sLc, 0.5, sr);
            float* BL = dbuf[0].data(); float* BR = dbuf[1].data(); const int mask = dlen - 1; int w = dw; double fl = fbL, fr = fbR;
            for (int i = 0; i < n; i++) {
                const double a = (double)(i + 1) / n, mix = mix0 + (sMix - mix0) * a, fb = fb0 + (sFb - fb0) * a, pp = pp0 + (sPP - pp0) * a;
                lfo += 0.35 / sr; if (lfo >= 1) lfo -= 1;
                const double tm = clampd(t0 + (sTime - t0) * a + sMod * std::sin(TAU * lfo), 0.001, 4.0), ds = tm * sr;
                const double xl = L[i], xr = R[i];
                // ping-pong: both inputs feed the left line at half level and the lines feed each other
                BL[w] = (float)(xl * (1 - 0.5 * pp) + xr * 0.5 * pp + fb * (fl * (1 - pp) + fr * pp));
                BR[w] = (float)(xr * (1 - pp) + fb * (fr * (1 - pp) + fl * pp));
                double rp = w - ds; while (rp < 0) rp += dlen;
                const int i0 = (int)rp; const double f = rp - i0;
                double yl = BL[i0 & mask] + f * (BL[(i0 + 1) & mask] - BL[i0 & mask]), yr = BR[i0 & mask] + f * (BR[(i0 + 1) & mask] - BR[i0 & mask]);
                yl = hp.tick(lp.tick(yl, 0), 0); yr = hp.tick(lp.tick(yr, 1), 1);
                fl = yl; fr = yr; w = (w + 1) & mask;
                L[i] = (float)(xl + mix * yl); R[i] = (float)(xr + mix * yr);
            }
            dw = w; fl0(fl); fl0(fr); fbL = fl; fbR = fr; lp.flush(); hp.flush();
        }
        // reverb send through the pre-delay
        const auto& r = F.rev; const int pmask = plen - 1; const int pd = (int)(clampd(r.pre, 0, 0.5) * sr);
        float* PL = pbuf[0].data(); float* PR = pbuf[1].data(); int w = pw;
        for (int i = 0; i < n; i++) { PL[w] = L[i]; PR[w] = R[i]; const int rd = (w - pd) & pmask; sendL[i] = PL[rd]; sendR[i] = PR[rd]; w = (w + 1) & pmask; }
        pw = w;
        first = false;
    }
    // Stage 2: mixes the convolved send back in, then dynamics, width, level and the final soft clip.
    void stage2(const Patch& P, const double* fxm, double master, float* L, float* R, float* wetL, float* wetR, int n) {
        const auto& F = P.fx; const double k = 1 - std::exp(-n / (0.025 * sr));
        const auto& r = F.rev; const double rmix = r.on ? clampd(r.mix + fxm[7], 0, 1) : 0;
        const double wetT = rmix * 1.8 * std::pow(clampd(r.decay > 0 ? r.decay : 3, 0.2, 14) / 3, -0.36), dryT = 1 - rmix * 0.35;
        const double wet0 = sWet, dry0 = sDry; sWet += (wetT - sWet) * k; sDry += (dryT - sDry) * k;
        rlc.highPass(clampd(r.lc, 20, 2000), 0.7, sr);
        for (int i = 0; i < n; i++) {
            const double a = (double)(i + 1) / n, wg = wet0 + (sWet - wet0) * a, dg = dry0 + (sDry - dry0) * a;
            L[i] = (float)(L[i] * dg + rlc.tick(wetL[i], 0) * wg); R[i] = (float)(R[i] * dg + rlc.tick(wetR[i], 1) * wg);
        }
        rlc.flush();
        const auto& c = F.comp;
        comp.att = 0.01; comp.rel = 0.2; comp.set(c.on ? clampd(c.thr, -60, 0) : 0, c.on ? clampd(c.ratio, 1, 20) : 1, 6);
        if (!c.on) comp.makeup = 1;
        comp.process(L, R, n, sr);
        const double cgT = c.on ? std::pow(10.0, clampd(c.gain, -12, 24) / 20) : 1, wT = clampd(F.width + fxm[10], 0, 2);
        const double cg0 = sCg, w0 = sW, m0 = sMaster; sCg += (cgT - sCg) * k; sW += (wT - sW) * k; sMaster += (master - sMaster) * k;
        for (int i = 0; i < n; i++) {
            const double a = (double)(i + 1) / n, cg = cg0 + (sCg - cg0) * a, w = w0 + (sW - w0) * a;
            const double l = L[i] * cg, rr = R[i] * cg, p = (1 + w) * 0.5, q = (1 - w) * 0.5;
            L[i] = (float)(l * p + rr * q); R[i] = (float)(rr * p + l * q);
        }
        lim.att = 0.001; lim.rel = 0.12; lim.set(-2, 20, 0);
        lim.process(L, R, n, sr);
        for (int i = 0; i < n; i++) {
            const double a = (double)(i + 1) / n, m = m0 + (sMaster - m0) * a;
            L[i] = (float)clip(L[i] * m); R[i] = (float)clip(R[i] * m);
        }
    }
    static inline double clip(double x) { const double a = std::abs(x); if (a < 0.85) return x; const double y = 0.85 + 0.15 * std::tanh((std::min(a, 2.0) - 0.85) / 0.15); return x < 0 ? -y : y; }
    double sr = 48000;
private:
    Biquad eqL, eqM, eqH, lp, hp, rlc; Dyn comp, lim;
    std::vector<float> dbuf[2], pbuf[2]; int dlen = 1, plen = 1, dw = 0, pw = 0; bool first = true, dclear = true;
    double fbL = 0, fbR = 0, lfo = 0;
    double sLo = 0, sMi = 0, sHi = 0, sMf = 1000, sMix = 0, sFb = 0.35, sPP = 1, sTime = 0.35, sHc = 5000, sLc = 250, sMod = 0, sWet = 0, sDry = 1, sCg = 1, sW = 1, sMaster = 0.8;
};

} // namespace aether
