// Aether synthesis engine. C++ port of the browser engine (dsp.js); no framework dependencies.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <array>
#include <map>
#include <memory>
#include <mutex>
#include <algorithm>
#include <functional>

namespace aether {

constexpr double TAU = 6.283185307179586;
constexpr double PI = 3.141592653589793;
constexpr int CB = 32, TBL = 2048, TMASK = 2047, NLEV = 18, POOL = 20, NLD = 36, NSRC = 20, NFXD = 11;
constexpr double VOICE_GAIN = 0.3;
constexpr double STEAL_FADE = 0.015;   // seconds; how fast a voice taken for a new note fades out (4 ms was an audible click)

inline double clampd(double x, double a, double b) { return x < a ? a : (x > b ? b : x); }
inline double ftanh(double x) { if (x <= -3) return -1; if (x >= 3) return 1; const double x2 = x * x; return x * (27 + x2) / (27 + 9 * x2); }
inline double mtof(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }
inline int32_t toInt32(double d) { if (!(d == d)) return 0; double m = std::fmod(std::trunc(d), 4294967296.0); if (m < 0) m += 4294967296.0; return (int32_t)(uint32_t)m; }
inline double hash01(double i) {
    uint32_t x = ((uint32_t)toInt32(i) * 0x9E3779B1u) ^ 0x85EBCA6Bu;
    x = (x ^ (x >> 15)) * 0x2C1B3C6Du; x = (x ^ (x >> 12)) * 0x297A2D39u; x ^= x >> 15;
    return x / 4294967296.0;
}
struct Rng {
    uint32_t s = 0x2545F491u;
    inline double next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s / 4294967296.0; }
    void seed(double v) { int32_t i = toInt32(v); s = i ? (uint32_t)i : 0x2545F491u; }
};

// ------------------------------------------------------------------ names
inline const std::vector<std::string>& modeNames() { static const std::vector<std::string> v{ "analog", "wt", "pluck", "modal", "formant", "noise", "gran", "sample" }; return v; }
inline const std::vector<std::string>& ftypeNames() { static const std::vector<std::string> v{ "off", "lp12", "lp24", "ladder", "hp12", "hp24", "bp", "notch", "peak", "comb", "combn", "vowel" }; return v; }
inline const std::vector<std::string>& srcIds() { static const std::vector<std::string> v{ "none", "e1", "e2", "l1", "l2", "l3", "l4", "vel", "key", "mw", "at", "rnd", "m1", "m2", "m3", "m4", "ox", "oy", "alt", "pb" }; return v; }
inline const std::vector<std::string>& ldstNames() { static const std::vector<std::string> v{ "pitch", "shp", "pw", "sync", "pos", "fm", "ring", "fold", "sub", "nz", "det", "spr", "pdmp", "mbri", "vow", "fsh", "brth", "ncol", "gpos", "gsz", "gden", "gspr", "gpit", "sst", "f1c", "f1r", "f1d", "f2c", "f2r", "fbal", "lvl", "pan", "aatk", "adec", "arel", "fdec" }; return v; }
inline const std::vector<std::string>& fxdstNames() { static const std::vector<std::string> v{ "drv.amt", "lofi.mix", "cho.mix", "pha.mix", "gate.depth", "dly.mix", "dly.fb", "rev.mix", "eq.low", "eq.high", "width" }; return v; }
inline const std::vector<std::string>& wtNames() { static const std::vector<std::string> v{ "Classic", "PWM", "Sync Saw", "Harmonic Rise", "Odd Rise", "Vowel", "Choir", "Vocal Pulse", "Drawbars", "Fold", "FM 1:1", "FM 2:1", "FM 3:1", "FM Bell", "Phase Dist", "Growl", "Comb", "Bit Sine", "Digital Saw", "Glass", "Metallic", "Random Spectra", "Hollow", "Reed", "Brass", "Strings", "Octave Stack", "Ghost" }; return v; }
inline const std::vector<std::string>& arpModes() { static const std::vector<std::string> v{ "up", "down", "updown", "downup", "random", "order", "chord" }; return v; }
inline int indexOf(const std::vector<std::string>& v, const std::string& s) { for (size_t i = 0; i < v.size(); i++) if (v[i] == s) return (int)i; return -1; }
inline double divBeats(const std::string& n) {
    static const std::map<std::string, double> m{ { "8 bar", 32 }, { "4 bar", 16 }, { "2 bar", 8 }, { "1 bar", 4 }, { "1/2", 2 }, { "1/2T", 4.0 / 3 }, { "1/4.", 1.5 }, { "1/4", 1 }, { "1/4T", 2.0 / 3 }, { "1/8.", 0.75 }, { "1/8", 0.5 }, { "1/8T", 1.0 / 3 }, { "1/16.", 0.375 }, { "1/16", 0.25 }, { "1/16T", 1.0 / 6 }, { "1/32", 0.125 } };
    auto it = m.find(n); return it == m.end() ? 1.0 : it->second;
}
struct ScaleDef { const char* name; std::vector<int> deg; };
inline const std::vector<ScaleDef>& scales() {
    static const std::vector<ScaleDef> v{ { "Major", { 0, 2, 4, 5, 7, 9, 11 } }, { "Minor", { 0, 2, 3, 5, 7, 8, 10 } }, { "Dorian", { 0, 2, 3, 5, 7, 9, 10 } }, { "Phrygian", { 0, 1, 3, 5, 7, 8, 10 } },
        { "Lydian", { 0, 2, 4, 6, 7, 9, 11 } }, { "Mixolydian", { 0, 2, 4, 5, 7, 9, 10 } }, { "Harmonic Minor", { 0, 2, 3, 5, 7, 8, 11 } }, { "Melodic Minor", { 0, 2, 3, 5, 7, 9, 11 } },
        { "Pentatonic Major", { 0, 2, 4, 7, 9 } }, { "Pentatonic Minor", { 0, 3, 5, 7, 10 } }, { "Blues", { 0, 3, 5, 6, 7, 10 } }, { "Phrygian Dominant", { 0, 1, 4, 5, 7, 8, 10 } },
        { "Hirajoshi", { 0, 2, 3, 7, 8 } }, { "Whole Tone", { 0, 2, 4, 6, 8, 10 } } };
    return v;
}
struct ChordDef { const char* name; std::vector<int> iv; int diatonic; };
inline const std::vector<ChordDef>& chords() {
    static const std::vector<ChordDef> v{ { "Major", { 0, 4, 7 }, 0 }, { "Minor", { 0, 3, 7 }, 0 }, { "Maj7", { 0, 4, 7, 11 }, 0 }, { "Min7", { 0, 3, 7, 10 }, 0 }, { "Dom7", { 0, 4, 7, 10 }, 0 }, { "Min9", { 0, 3, 7, 10, 14 }, 0 },
        { "Maj9", { 0, 4, 7, 11, 14 }, 0 }, { "Min11", { 0, 3, 7, 10, 14, 17 }, 0 }, { "Sus2", { 0, 2, 7 }, 0 }, { "Sus4", { 0, 5, 7 }, 0 }, { "Add9", { 0, 4, 7, 14 }, 0 }, { "Min6", { 0, 3, 7, 9 }, 0 },
        { "Dim", { 0, 3, 6 }, 0 }, { "Dim7", { 0, 3, 6, 9 }, 0 }, { "Aug", { 0, 4, 8 }, 0 }, { "Power", { 0, 7, 12 }, 0 }, { "Octave", { 0, 12 }, 0 },
        { "Diatonic Triad", {}, 3 }, { "Diatonic 7th", {}, 4 }, { "Diatonic 9th", {}, 5 } };
    return v;
}

// ------------------------------------------------------------------ FFT + static tables
inline void fft(double* re, double* im, int n, bool inv) {
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { std::swap(re[i], re[j]); std::swap(im[i], im[j]); }
    }
    for (int len = 2; len <= n; len <<= 1) {
        const int half = len >> 1; const double ang = (inv ? TAU : -TAU) / len, wr = std::cos(ang), wi = std::sin(ang);
        for (int i = 0; i < n; i += len) {
            double cr = 1, ci = 0;
            for (int k = 0; k < half; k++) {
                const int a = i + k, b = a + half;
                const double br = re[b] * cr - im[b] * ci, bi = re[b] * ci + im[b] * cr;
                re[b] = re[a] - br; im[b] = im[a] - bi; re[a] += br; im[a] += bi;
                const double t = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = t;
            }
        }
    }
    if (inv) for (int i = 0; i < n; i++) { re[i] /= n; im[i] /= n; }
}
struct Spec { std::vector<double> re, im; Spec() : re(TBL, 0.0), im(TBL, 0.0) {} };
inline Spec specHarm(const std::vector<double>& amp) {
    Spec s; const int H = TBL / 2;
    for (int h = 1; h < (int)amp.size() && h < H; h++) {
        const double a = amp[h]; if (a == 0) continue;
        s.re[h] = 0; s.im[h] = -H * a; s.re[TBL - h] = 0; s.im[TBL - h] = H * a;
    }
    return s;
}
template <typename Fn> inline Spec specCycle(Fn fn) {
    Spec s;
    for (int i = 0; i < TBL; i++) { const double v = fn((double)i / TBL); s.re[i] = v == v ? v : 0; }
    fft(s.re.data(), s.im.data(), TBL, false);
    return s;
}
template <typename Fn> inline Spec harm(Fn fn) { std::vector<double> a(1024, 0.0); for (int h = 1; h < 1024; h++) { const double v = fn(h); a[h] = (v > 0 || v < 0) ? v : 0; } return specHarm(a); }
inline int miph(int l) { return std::max(1, (int)std::floor(512 * std::pow(2.0, -l / 2.0))); }
inline void writeMips(const Spec& sp, float* out) {
    std::vector<double> fr(TBL), fi(TBL);
    double scale = 1;
    for (int l = 0; l < NLEV; l++) {
        const int M = miph(l);
        std::fill(fr.begin(), fr.end(), 0.0); std::fill(fi.begin(), fi.end(), 0.0);
        for (int h = 1; h <= M; h++) { fr[h] = sp.re[h]; fi[h] = sp.im[h]; fr[TBL - h] = sp.re[TBL - h]; fi[TBL - h] = sp.im[TBL - h]; }
        fft(fr.data(), fi.data(), TBL, true);
        if (l == 0) { double pk = 0; for (int i = 0; i < TBL; i++) pk = std::max(pk, std::abs(fr[i])); scale = pk > 1e-9 ? 1 / pk : 0; }
        float* o = out + l * TBL;
        for (int i = 0; i < TBL; i++) o[i] = (float)(fr[i] * scale);
    }
}
struct Statics {
    std::vector<float> sine, saw, tri;
    Statics() : sine(TBL + 1), saw(NLEV * TBL), tri(NLEV * TBL) {
        for (int i = 0; i <= TBL; i++) sine[i] = (float)std::sin(TAU * i / TBL);
        std::vector<double> a(1024, 0.0), b(1024, 0.0);
        for (int h = 1; h < 1024; h++) {
            a[h] = -2 / (PI * h);
            if (h & 1) b[h] = (8 / (PI * PI)) * ((((h - 1) >> 1) & 1) ? -1 : 1) / ((double)h * h);
        }
        writeMips(specHarm(a), saw.data()); writeMips(specHarm(b), tri.data());
    }
};
inline const Statics& statics() { static const Statics s; return s; }
inline double rdT(const float* t, double p) { const double x = p * TBL; const int i = (int)x; const double f = x - i, a = t[i]; return a + f * (t[(i + 1) & TMASK] - a); }
inline int lvlFor(double freq, double sr) {
    const double mh = 0.5 * sr / (freq > 1 ? freq : 1);
    if (mh >= 512) return 0;
    const int l = (int)std::ceil(2 * std::log2(512 / mh));
    return l > NLEV - 1 ? NLEV - 1 : l;
}

// ------------------------------------------------------------------ vowels
static const double VOWELS[5][2][4] = {
    { { 800, 1150, 2900, 3900 }, { 1, 0.5, 0.25, 0.1 } }, { { 400, 1600, 2700, 3300 }, { 1, 0.3, 0.2, 0.1 } }, { { 270, 2140, 2950, 3900 }, { 1, 0.25, 0.2, 0.1 } },
    { { 450, 800, 2830, 3800 }, { 1, 0.5, 0.15, 0.08 } }, { { 325, 700, 2530, 3500 }, { 1, 0.3, 0.08, 0.04 } } };
static const double VBW[4] = { 80, 90, 120, 130 };
inline void vowelAt(double v, double* F, double* G) {
    const double x = clampd(v, 0, 1) * 4; const int i = std::min(3, (int)x); const double f = x - i;
    for (int k = 0; k < 4; k++) { F[k] = VOWELS[i][0][k] + f * (VOWELS[i + 1][0][k] - VOWELS[i][0][k]); G[k] = VOWELS[i][1][k] + f * (VOWELS[i + 1][1][k] - VOWELS[i][1][k]); }
}
inline double formantAmp(double hz, const double* F, const double* G, double bw) { double s = 0; for (int k = 0; k < 4; k++) { const double d = (hz - F[k]) / (VBW[k] * bw); s += G[k] / (1 + d * d); } return s; }

// ------------------------------------------------------------------ wavetables
struct Wavetable { int F = 16; std::vector<float> data; };
inline Spec wtFrame(int id, double t) {
    static const int DB_H[9] = { 1, 3, 2, 4, 6, 8, 10, 12, 16 };
    static const double DB_REG[7][9] = { { 0, 0, 8, 0, 0, 0, 0, 0, 0 }, { 8, 8, 8, 0, 0, 0, 0, 0, 0 }, { 8, 8, 8, 8, 0, 0, 0, 0, 0 }, { 6, 8, 8, 6, 0, 0, 0, 0, 0 }, { 8, 8, 8, 8, 8, 8, 0, 0, 0 }, { 8, 6, 8, 8, 6, 8, 4, 4, 8 }, { 8, 8, 8, 8, 8, 8, 8, 8, 8 } };
    static const int METAL[11] = { 1, 3, 7, 11, 13, 17, 23, 29, 31, 37, 41 };
    double F[4], G[4];
    switch (id) {
        case 0: return specCycle([t](double p) {
            const double s = std::sin(TAU * p), tri = p < 0.25 ? 4 * p : p < 0.75 ? 2 - 4 * p : 4 * p - 4, saw = 2 * p - 1, sq = p < 0.5 ? 1 : -1, x = t * 4;
            if (x < 1) return s + (tri - s) * x;
            if (x < 2) return tri + (saw - tri) * (x - 1);
            if (x < 3) return saw + (sq - saw) * (x - 2);
            return p < 0.5 - 0.42 * (x - 3) ? 1.0 : -1.0; });
        case 1: return specCycle([t](double p) { return p < 0.5 - 0.46 * t ? 1.0 : -1.0; });
        case 2: return specCycle([t](double p) { const double q = std::fmod(p * (1 + 7 * t), 1.0); return 2 * q - 1; });
        case 3: { const double N = 1 + 63 * t * t; return harm([N](int h) { return h <= N ? 1.0 / h : (h < N + 1 ? (N + 1 - h) / h : 0.0); }); }
        case 4: { const double N = 1 + 63 * t * t; return harm([N](int h) { return (h & 1) ? (h <= N ? 1.0 / h : (h < N + 2 ? (N + 2 - h) / (2.0 * h) : 0.0)) : 0.0; }); }
        case 5: vowelAt(t, F, G); return harm([&](int h) { return formantAmp(h * 110.0, F, G, 1) / std::sqrt((double)h); });
        case 6: vowelAt(0.75 - 0.75 * t, F, G); return harm([&](int h) { return formantAmp(h * 130.0, F, G, 1.6) * std::exp(-h / 40.0) / std::sqrt((double)h); });
        case 7: vowelAt(t, F, G); return harm([&](int h) { return std::abs(std::sin(PI * h * 0.3)) / h * formantAmp(h * 120.0, F, G, 1.2); });
        case 8: {
            const double x = t * 6; const int j = std::min(5, (int)x); const double f = x - j; double lev[9];
            for (int k = 0; k < 9; k++) lev[k] = DB_REG[j][k] + f * (DB_REG[j + 1][k] - DB_REG[j][k]);
            return harm([&](int h) { double s = 0; for (int k = 0; k < 9; k++) if (DB_H[k] == h) { const double L = lev[k]; if (L > 0.01) s += std::pow(10.0, (L - 8) * 0.15) * std::min(1.0, L); } return s; });
        }
        case 9: return specCycle([t](double p) { return std::sin(PI / 2 * std::sin(TAU * p) * (1 + 9 * t)); });
        case 10: return specCycle([t](double p) { return std::sin(TAU * p + 6 * t * std::sin(TAU * p)); });
        case 11: return specCycle([t](double p) { return std::sin(TAU * p + 5 * t * std::sin(TAU * 2 * p)); });
        case 12: return specCycle([t](double p) { return std::sin(TAU * p + 4 * t * std::sin(TAU * 3 * p)); });
        case 13: return specCycle([t](double p) { return std::sin(TAU * p + 3 * t * std::sin(TAU * 7 * p) + 1.5 * t * std::sin(TAU * 2 * p)); });
        case 14: return specCycle([t](double p) { return (1 - p) * std::sin(TAU * p * (1 + 14 * t)); });
        case 15: { const double hc = 2 + 40 * t * t, Q = 5; return harm([=](int h) { const double x = h / hc; return std::pow(1 / std::sqrt((1 - x * x) * (1 - x * x) + (x / Q) * (x / Q)), 1.5) / h; }); }
        case 16: return harm([t](int h) { return (1.0 / h) * (0.5 + 0.5 * std::cos(TAU * h * (0.05 + 0.25 * t))); });
        case 17: return specCycle([t](double p) { const double lv = std::max(1.0, std::floor(std::pow(2.0, 5 - 4.5 * t) + 0.5)); return std::floor(std::sin(TAU * p) * lv + 0.5) / lv; });
        case 18: return specCycle([t](double p) { const double st = std::floor(64 - 58 * t + 0.5); return 2 * std::floor(p * st) / st - 1; });
        case 19: { const double c1 = 3 + 8 * t, c2 = 7 + 18 * t; return harm([=](int h) { return (h == 1 ? 1.0 : 0.0) + 0.7 * std::exp(-((h - c1) * (h - c1)) / 2) + 0.5 * std::exp(-((h - c2) * (h - c2)) / 3); }); }
        case 20: return harm([t](int h) { int i = -1; for (int k = 0; k < 11; k++) if (METAL[k] == h) i = k; if (i < 0) return 0.0; return (1 - t) * std::pow(0.7, i) + t * std::pow(0.85, std::abs(i - 5)); });
        case 21: {
            const double x = t * 3; const int j = std::min(2, (int)x); const double f = x - j;
            return harm([=](int h) { if (h > 96) return 0.0; const double a = std::pow(hash01(j * 977.0 + h * 31.0), 2) / std::pow((double)h, 0.6), b = std::pow(hash01((j + 1) * 977.0 + h * 31.0), 2) / std::pow((double)h, 0.6); return a * (1 - f) + b * f; });
        }
        case 22: return harm([t](int h) { return (h & 1) ? 1.0 / h : t * 0.8 / h; });
        case 23: return harm([t](int h) { return ((h & 1) ? 1.0 / h : (0.15 + 0.5 * t) / h) * std::exp(-h / (6 + 30 * t)); });
        case 24: return harm([t](int h) { return std::exp(-h / (1.5 + 25 * t)) / std::sqrt((double)h); });
        case 25: return harm([t](int h) { const double c = 4 + 4 * t; return (1.0 / h) * (1 + 1.5 * std::exp(-((h - c) * (h - c)) / 6)) * std::exp(-h / (20 + 60 * t)); });
        case 26: return harm([t](int h) { return h == 1 ? 1.0 : h == 2 ? 0.8 * t : h == 4 ? 0.6 * t * t : h == 8 ? 0.45 * t * t * t : h == 16 ? 0.3 * t * t * t * t : 0.0; });
        default: return harm([t](int h) { return h == 1 ? 1.0 : (h < 64 ? t * 0.18 * hash01(h * 7.0 + 3) / std::sqrt((double)h) : 0.0); });
    }
}
inline std::shared_ptr<Wavetable> buildWT(int id) {
    auto w = std::make_shared<Wavetable>(); w->F = 16; w->data.resize((size_t)16 * NLEV * TBL);
    for (int i = 0; i < 16; i++) writeMips(wtFrame(id, i / 15.0), w->data.data() + (size_t)i * NLEV * TBL);
    return w;
}

// ------------------------------------------------------------------ modal models
struct Model { const char* name; std::vector<double> r, a, d; };
inline const std::vector<Model>& models() {
    static const std::vector<Model> v{
        { "Marimba", { 1, 3.99, 10.65, 18.8 }, { 1, 0.35, 0.12, 0.04 }, { 1, 0.4, 0.22, 0.12 } },
        { "Vibes", { 1, 3.98, 9.2, 15.6 }, { 1, 0.45, 0.15, 0.06 }, { 1, 0.55, 0.3, 0.18 } },
        { "Glock", { 1, 2.756, 5.404, 8.933, 13.344, 19.93 }, { 1, 0.55, 0.35, 0.22, 0.14, 0.08 }, { 1, 0.75, 0.55, 0.4, 0.3, 0.22 } },
        { "Bell", { 0.5, 1, 1.183, 1.506, 2, 2.514, 2.662, 3.011, 4.166, 5.433, 6.796, 8.215 }, { 0.55, 1, 0.65, 0.4, 0.65, 0.3, 0.28, 0.3, 0.22, 0.15, 0.1, 0.07 }, { 1.5, 1.1, 0.9, 0.75, 0.85, 0.55, 0.5, 0.45, 0.35, 0.28, 0.22, 0.18 } },
        { "Tine", { 1, 2, 3, 4, 7.15, 11.3 }, { 1, 0.3, 0.1, 0.05, 0.18, 0.06 }, { 1, 0.55, 0.4, 0.3, 0.08, 0.05 } },
        { "Kalimba", { 1, 5.95, 13.1, 2.01 }, { 1, 0.25, 0.07, 0.05 }, { 1, 0.25, 0.12, 0.5 } },
        { "Glass", { 1, 2.32, 4.25, 6.63, 9.38, 12.53 }, { 1, 0.5, 0.35, 0.25, 0.16, 0.1 }, { 1, 0.9, 0.8, 0.7, 0.6, 0.5 } },
        { "Steel Pan", { 1, 2, 3, 4.03, 5.08, 6.1 }, { 1, 0.7, 0.4, 0.22, 0.12, 0.07 }, { 1, 0.8, 0.55, 0.4, 0.3, 0.22 } },
        { "Gong", { 1, 1.42, 1.83, 2.27, 2.71, 3.2, 3.72, 4.29, 4.9, 5.6, 6.3, 7.1 }, { 1, 0.8, 0.75, 0.6, 0.55, 0.5, 0.4, 0.35, 0.3, 0.25, 0.2, 0.15 }, { 1, 0.95, 0.9, 0.85, 0.8, 0.75, 0.7, 0.62, 0.55, 0.5, 0.45, 0.4 } },
        { "Drum", { 1, 1.593, 2.136, 2.296, 2.653, 2.918, 3.156, 3.501 }, { 1, 0.8, 0.65, 0.6, 0.5, 0.42, 0.35, 0.3 }, { 1, 0.6, 0.5, 0.45, 0.4, 0.36, 0.32, 0.28 } },
        { "Harmonic", { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 }, { 1, 0.5, 0.33, 0.25, 0.2, 0.17, 0.14, 0.12, 0.11, 0.1, 0.09, 0.08 }, { 1, 0.85, 0.72, 0.62, 0.54, 0.47, 0.41, 0.36, 0.32, 0.28, 0.25, 0.22 } },
        { "Piano", { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 1, 0.6, 0.35, 0.25, 0.18, 0.12, 0.09, 0.07, 0.05, 0.04 }, { 1, 0.8, 0.62, 0.5, 0.4, 0.33, 0.28, 0.24, 0.2, 0.17 } },
        { "Music Box", { 1, 6.27, 17.55, 2 }, { 1, 0.3, 0.1, 0.08 }, { 1, 0.3, 0.12, 0.6 } },
        { "Crystal", { 1, 2.01, 3.03, 4.08, 5.15, 6.24 }, { 1, 0.6, 0.45, 0.3, 0.2, 0.12 }, { 1, 0.95, 0.9, 0.85, 0.8, 0.75 } },
        { "Plate", { 1, 1.71, 2.47, 3.12, 3.94, 4.73, 5.51, 6.3 }, { 1, 0.7, 0.6, 0.5, 0.42, 0.35, 0.3, 0.25 }, { 1, 0.8, 0.7, 0.6, 0.52, 0.45, 0.4, 0.35 } },
        { "Wood", { 1, 2.57, 4.9, 7.6 }, { 1, 0.4, 0.2, 0.1 }, { 1, 0.3, 0.15, 0.08 } } };
    return v;
}
inline int modelIndex(const std::string& n) { const auto& m = models(); for (size_t i = 0; i < m.size(); i++) if (n == m[i].name) return (int)i; return 3; }

// ------------------------------------------------------------------ patch
struct Layer {
    int on = 0, mode = 0; double lvl = 0.8, pan = 0, crs = 0, fin = 0; int kt = 1; double vs = 0.6;
    double shp = 0.667, pw = 0.5, sync = 0; int wt = 0; double pos = 0, fm = 0, fmr = 1, ring = 0, rngr = 2, fold = 0, sub = 0, nz = 0;
    int uni = 1; double det = 15, spr = 0.5, drift = 0.15;
    double pdec = 2, pdmp = 0.4, ppos = 0.2, pexc = 0.6;
    int mdl = 3; double mbri = 0.5, mdec = 1.5, mhard = 0.5, minh = 0, mbow = 0;
    double vow = 0, fsh = 0.5, brth = 0.1; int ntyp = 0; double ncol = 0.6;
    int src = 0; double gpos = 0.3, gspr = 0.1, gsz = 0.45, gden = 0.55, gpit = 0, grev = 0, gscan = 0, sst = 0; int sloop = 1;
    int f1t = 2; double f1c = 0.8, f1r = 0.1, f1d = 0, f1k = 0.5, f1e = 0, f1v = 0;
    int f2t = 0; double f2c = 0.6, f2r = 0.1, f2d = 0, f2k = 0.5, f2e = 0, f2v = 0; int frt = 0; double fbal = 0.5;
    double aa = 0.005, ad = 0.5, as = 1, ar = 0.3, fa = 0.005, fd = 0.5, fs = 0.3, fr = 0.5;
};
struct ModEnvP { double a = 0.005, d = 0.5, s = 0, r = 0.5; int loop = 0; };
struct LfoP { int shp = 0; double rate = 1; int sync = 0; double div = 1, fade = 0; int rtg = 0; double ph = 0; };
struct ModRoute { int s = 0, v = 0; double a = 0; int tgt = 0, pi = 0; };   // tgt 0-3 layer, 4 all, 5 lfo rate, 6 fx
struct Patch {
    Layer L[4];
    struct { int poly = 8, vmode = 0; double glide = 0; int gmode = 0; double bend = 2, vol = 0.8, trim = 1; } g;
    ModEnvP me[2]; LfoP lfo[4];
    ModRoute mods[24]; int nmods = 0;
    double mac[4] = { 0, 0, 0, 0 };
    struct { double x = 0, y = 0, ch = 0, sp = 0.3; } orb;
    struct { int on = 0, mode = 0; double div = 0.25; int oct = 1; double gate = 0.6, swing = 0; int len = 16, latch = 0; double vel[16]; int tr[16]; } arp;
    struct { int on = 0, type = 3, spread = 0; } chord;
    struct { int on = 0, root = 0, type = 1; } scale;
    struct {
        struct { int on = 0, type = 0; double amt = 0.3, tone = 0.75, mix = 1; } drv;
        struct { int on = 0; double bits = 12, rate = 1, wow = 0.25, flut = 0.15, noise = 0.15, tone = 0.7, mix = 1; } lofi;
        struct { int on = 0; double div = 0.25, depth = 1, smooth = 0.25; int pat[16]; } gate;
        struct { int on = 0, mode = 0; double rate = 0.5, depth = 0.5, fb = 0, mix = 0.4; } cho;
        struct { int on = 0; double rate = 0.25, depth = 0.7, fb = 0.4, mix = 0.5; } pha;
        struct { int on = 0; double low = 0, mid = 0, mf = 1000, high = 0; } eq;
        struct { int on = 0, sync = 1; double div = 0.75, time = 0.35, fb = 0.35; int pp = 1; double lc = 250, hc = 5000, mod = 0.15, mix = 0.2; } dly;
        struct { int on = 1, type = 0; double size = 0.6, decay = 3, damp = 0.5, pre = 0.02, lc = 180, mix = 0.22; } rev;
        struct { int on = 0; double thr = -18, ratio = 3, gain = 3; } comp;
        double width = 1;
    } fx;
    Patch() {
        L[0].on = 1;
        me[1] = ModEnvP{ 0.5, 1, 0.5, 1, 0 };
        const double rates[4] = { 5, 0.3, 0.15, 0.4 };
        for (int i = 0; i < 4; i++) { lfo[i].rate = rates[i]; lfo[i].rtg = i == 0 ? 1 : 0; }
        for (int i = 0; i < 16; i++) { arp.vel[i] = 1; arp.tr[i] = 0; fx.gate.pat[i] = (i & 1) ? 0 : 1; }
    }
};

// ------------------------------------------------------------------ sources (sampled material rendered by the engine itself)
struct Source { std::vector<float> data; double sr = 48000, root = 60; int ls = 0, le = 0; };
struct SourceDef { std::string name; double root, len, ls, le, flat; Patch patch; std::vector<std::array<double, 4>> ev; };
inline const std::vector<SourceDef>& sourceDefs();
inline const std::vector<std::string>& sourceNames();
inline int sourceIndex(const std::string& n) { return indexOf(sourceNames(), n); }

// Shared, lazily built wavetables and sources. Build from any non-audio thread before handing a patch to an engine.
class Assets {
public:
    static Assets& get() { static Assets a; return a; }
    std::shared_ptr<Wavetable> table(int id) {
        id = std::max(0, std::min((int)wtNames().size() - 1, id));
        std::lock_guard<std::mutex> g(m);
        if (!wt[(size_t)id]) wt[(size_t)id] = buildWT(id);
        return wt[(size_t)id];
    }
    std::shared_ptr<Source> source(int id);
    // A patch plus the tables and sources it needs. Assets are cached for the life of the process, so the raw pointers stay valid.
    struct Bundle { Patch p; const Wavetable* wt[4] = { nullptr, nullptr, nullptr, nullptr }; const Source* src[4] = { nullptr, nullptr, nullptr, nullptr }; };
    void fill(Bundle& b) {
        for (int i = 0; i < 4; i++) {
            const Layer& L = b.p.L[i];
            b.wt[i] = L.mode == 1 ? table(L.wt).get() : nullptr;
            b.src[i] = (L.mode == 6 || L.mode == 7) ? source(L.src).get() : nullptr;
        }
    }
private:
    Assets() : wt(wtNames().size()), src(32) { statics(); }
    std::mutex m, ms;
    std::vector<std::shared_ptr<Wavetable>> wt;
    std::vector<std::shared_ptr<Source>> src;
};

// ------------------------------------------------------------------ filters
struct Filt {
    double a = 0, b = 0, c = 0, d = 0, s1 = 0, s2 = 0, s3 = 0, s4 = 0, g = -1; int w = 0; double v[8] = { 0 };
    std::vector<float> buf;
    void reset() { a = b = c = d = s1 = s2 = s3 = s4 = 0; g = -1; if (!buf.empty()) std::fill(buf.begin(), buf.end(), 0.0f); std::fill(v, v + 8, 0.0); }
};
inline void fl0(double& x) { if (x < 1e-18 && x > -1e-18) x = 0; }
inline double cutHz(double c, double sr) { const double hz = 20 * std::pow(2.0, c * 9.9658); return hz > sr * 0.45 ? sr * 0.45 : hz; }
inline void svf1(Filt& f, float* buf, int n, double g0, double g1, double k, int mode) {
    double ic1 = f.a, ic2 = f.b, g = g0; const double dg = (g1 - g0) / n;
    for (int i = 0; i < n; i++) {
        g += dg;
        const double a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const double v0 = buf[i], v3 = v0 - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        buf[i] = (float)(mode == 0 ? v2 : mode == 1 ? v0 - k * v1 - v2 : mode == 2 ? k * v1 : mode == 3 ? v0 - k * v1 : 2 * v2 - v0 + k * v1);
    }
    f.a = ic1; f.b = ic2;
}
inline void svf2(Filt& f, float* buf, int n, double g0, double g1, double k1, double k2, bool hp) {
    double a1s = f.a, a2s = f.b, b1s = f.c, b2s = f.d, g = g0; const double dg = (g1 - g0) / n;
    for (int i = 0; i < n; i++) {
        g += dg;
        double a1 = 1 / (1 + g * (g + k1)), a2 = g * a1, a3 = g * a2;
        double v0 = buf[i], v3 = v0 - a2s, v1 = a1 * a1s + a2 * v3, v2 = a2s + a2 * a1s + a3 * v3;
        a1s = 2 * v1 - a1s; a2s = 2 * v2 - a2s;
        const double y1 = hp ? v0 - k1 * v1 - v2 : v2;
        a1 = 1 / (1 + g * (g + k2)); a2 = g * a1; a3 = g * a2;
        v0 = y1; v3 = v0 - b2s; v1 = a1 * b1s + a2 * v3; v2 = b2s + a2 * b1s + a3 * v3;
        b1s = 2 * v1 - b1s; b2s = 2 * v2 - b2s;
        buf[i] = (float)(hp ? v0 - k2 * v1 - v2 : v2);
    }
    f.a = a1s; f.b = a2s; f.c = b1s; f.d = b2s;
}
inline void ladder(Filt& f, float* buf, int n, double g0, double g1, double k) {
    double s1 = f.s1, s2 = f.s2, s3 = f.s3, s4 = f.s4, g = g0; const double dg = (g1 - g0) / n, comp = 1 + k * 0.42;
    for (int i = 0; i < n; i++) {
        g += dg;
        const double G = g / (1 + g), b = 1 / (1 + g), G2 = G * G, G3 = G2 * G, G4 = G3 * G;
        const double S = G3 * b * s1 + G2 * b * s2 + G * b * s3 + b * s4;
        const double x = buf[i] * comp, y4e = (G4 * x + S) / (1 + k * G4), u = ftanh(x - k * y4e);
        double v = (u - s1) * G; const double y1 = v + s1; s1 = y1 + v;
        v = (y1 - s2) * G; const double y2 = v + s2; s2 = y2 + v;
        v = (y2 - s3) * G; const double y3 = v + s3; s3 = y3 + v;
        v = (y3 - s4) * G; const double y4 = v + s4; s4 = y4 + v;
        buf[i] = (float)y4;
    }
    f.s1 = s1; f.s2 = s2; f.s3 = s3; f.s4 = s4;
}
inline void combF(Filt& f, float* buf, int n, double D, double fb) {
    if (f.buf.empty()) return;
    float* B = f.buf.data(); const double att = 1 - std::abs(fb) * 0.6; int w = f.w;
    for (int i = 0; i < n; i++) {
        double rp = w - D; if (rp < 0) rp += 4096;
        const int i0 = (int)rp; const double fr = rp - i0, a = B[i0], d = a + fr * (B[(i0 + 1) & 4095] - a);
        double y = buf[i] + fb * d;
        if (y > 4) y = 4; else if (y < -4) y = -4;
        B[w] = (float)y; w = (w + 1) & 4095;
        buf[i] = (float)(y * att);
    }
    f.w = w;
}
inline void vowelF(Filt& f, float* buf, int n, double pos, double res, double sr, double shift) {
    double F[4], G[4], va[12], vw[4]; vowelAt(pos, F, G);
    double* v = f.v; const double bw = 1.6 - res * 1.2, sh = shift != 0 ? shift : 1;
    for (int k = 0; k < 4; k++) {
        const double hz = std::min(F[k] * sh, sr * 0.45), g = std::tan(PI * hz / sr), kk = std::min(1.9, VBW[k] * bw * sh / hz);
        const double a1 = 1 / (1 + g * (g + kk));
        va[k * 3] = a1; va[k * 3 + 1] = g * a1; va[k * 3 + 2] = g * g * a1; vw[k] = G[k] * kk;
    }
    for (int i = 0; i < n; i++) {
        const double x = buf[i]; double out = 0;
        for (int k = 0; k < 4; k++) {
            const int j = k * 2; const double ic1 = v[j], ic2 = v[j + 1], a1 = va[k * 3], a2 = va[k * 3 + 1], a3 = va[k * 3 + 2];
            const double v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
            v[j] = 2 * v1 - ic1; v[j + 1] = 2 * v2 - ic2;
            out += vw[k] * v1;
        }
        buf[i] = (float)(out * 1.7);
    }
    for (int j = 0; j < 8; j++) fl0(v[j]);
}
inline void filt(Filt& f, int type, float* buf, int n, double cut, double res, double drv, double sr) {
    if (drv > 0.001) { const double dg = 1 + drv * 7, nm = 1 / std::sqrt(dg); for (int i = 0; i < n; i++) buf[i] = (float)(ftanh(buf[i] * dg) * nm); }
    const double hz = cutHz(cut, sr);
    if (type >= 1 && type <= 8) {
        const double g1 = std::tan(PI * hz / sr), g0 = f.g < 0 ? g1 : f.g;
        f.g = g1;
        switch (type) {
            case 1: svf1(f, buf, n, g0, g1, 1.414 - 1.374 * res, 0); break;
            case 2: svf2(f, buf, n, g0, g1, 1.848, 0.765 - 0.725 * res, false); break;
            case 3: ladder(f, buf, n, g0, g1, res * 3.9); break;
            case 4: svf1(f, buf, n, g0, g1, 1.414 - 1.374 * res, 1); break;
            case 5: svf2(f, buf, n, g0, g1, 1.848, 0.765 - 0.725 * res, true); break;
            case 6: svf1(f, buf, n, g0, g1, 1.5 - 1.45 * res, 2); break;
            case 7: svf1(f, buf, n, g0, g1, 2 - 1.9 * res, 3); break;
            case 8: svf1(f, buf, n, g0, g1, 1.414 - 1.36 * res, 4); break;
        }
        fl0(f.a); fl0(f.b); fl0(f.c); fl0(f.d); fl0(f.s1); fl0(f.s2); fl0(f.s3); fl0(f.s4);
    } else if (type == 9 || type == 10) combF(f, buf, n, clampd(sr / hz, 2, 4000), (type == 9 ? 1 : -1) * res * 0.96);
    else if (type == 11) vowelF(f, buf, n, cut, res, sr, 1);
}

// ------------------------------------------------------------------ envelopes, LFOs
struct Env {
    int st = 0; double v = 0;
    void on() { st = 1; }
    void off() { if (st != 0) st = 4; }
    double tick(double a, double d, double s, double r, int n, double sr, int loop) {
        switch (st) {
            case 1:
                if (a <= 0.0005) { v = 1; st = 2; }
                else { v += (1.3 - v) * (1 - std::exp(-n * 1.466 / (a * sr))); if (v >= 1) { v = 1; st = 2; } }
                break;
            case 2:
                v += (s - v) * (1 - std::exp(-n * 4 / ((d > 0.001 ? d : 0.001) * sr)));
                if (std::abs(v - s) < 1e-3) { v = s; if (loop) st = 1; else if (s < 1e-4) { st = 0; v = 0; } else st = 3; }
                break;
            case 3: v = s; if (s < 1e-4) { st = 0; v = 0; } break;
            case 4: v -= v * (1 - std::exp(-n * 5 / ((r > 0.001 ? r : 0.001) * sr))); if (v < 1e-4) { v = 0; st = 0; } break;
        }
        return v;
    }
};
struct Lfo {
    double ph = 0, v = 0, sh = 0, r0 = 0, r1 = 0, t = 0, k = -1;
    void reset(const LfoP& p, Rng& rng) { ph = p.ph; t = 0; sh = rng.next() * 2 - 1; r0 = sh; r1 = rng.next() * 2 - 1; k = -1; }
    double tick(const LfoP& p, int id, double hz, int n, double sr, double gtime, Rng& rng) {
        double phv;
        if (p.rtg) {
            ph += hz * n / sr;
            if (ph >= 1) { ph -= std::floor(ph); sh = rng.next() * 2 - 1; r0 = r1; r1 = rng.next() * 2 - 1; }
            phv = ph;
        } else {
            const double x = gtime * hz + p.ph, kk = std::floor(x);
            phv = x - kk;
            if (kk != k) { k = kk; sh = hash01(kk * 7 + id * 1013) * 2 - 1; r0 = hash01(kk * 13 + id * 7919) * 2 - 1; r1 = hash01((kk + 1) * 13 + id * 7919) * 2 - 1; }
        }
        t += (double)n / sr;
        double val;
        switch (p.shp) {
            case 0: val = statics().sine[(size_t)(int)(phv * TBL) & 2047]; break;
            case 1: val = phv < 0.25 ? 4 * phv : phv < 0.75 ? 2 - 4 * phv : 4 * phv - 4; break;
            case 2: val = 2 * phv - 1; break;
            case 3: val = 1 - 2 * phv; break;
            case 4: val = phv < 0.5 ? 1 : -1; break;
            case 5: val = sh; break;
            case 6: { const double s = 0.5 - 0.5 * std::cos(PI * phv); val = r0 + (r1 - r0) * s; } break;
            default: val = 0;
        }
        if (p.fade > 0.001) { const double f = t / p.fade; if (f < 1) val *= f; }
        return (v = val);
    }
};

class Engine;
struct LayerVoice {
    Env aenv, fenv;
    double ph[8] = { 0 }, mph[8] = { 0 }, fph[8] = { 0 }, rph[8] = { 0 }, drift[8] = { 0 }, incs[8] = { 0 }, gl[8] = { 0 }, gr[8] = { 0 };
    double subph = 0, g0l = 0, g0r = 0;
    Filt f1, f1b, f2, f2b, fo, fob;
    bool done = true; int mode = -1, quiet = 0;
    std::vector<float> str[3]; int sw = 0; double slp[3] = { 0 };
    double my1[12] = { 0 }, my2[12] = { 0 }, mc1[12], mc2[12], mg[12], mb[12], mpl[12], mpr[12];
    double mkey = -1; int mn = 0; double imp = 0, exn = 0, exd = 0;
    double pk[8] = { 0 }, brn[2] = { 0 }, hold[2] = { 0 }, clk[2] = { 0 }, hp[2] = { 0 };
    double nph = 0, wc = 0.5, wv = 0;
    double gpos[32], ginc[32], gage[32], gdur[32], ggl[32], ggr[32]; int gn = 0; double gcnt = 0, head = 0;
    double spos = 0; bool sdone = false, sfirst = false;
    LayerVoice() {
        for (auto& s : str) s.assign(4096, 0.0f);
        f1.buf.assign(4096, 0.0f); f1b.buf.assign(4096, 0.0f); f2.buf.assign(4096, 0.0f); f2b.buf.assign(4096, 0.0f);
    }
};
inline double herm(const float* d, double p, int len) {
    const int i = (int)p; const double f = p - i, x0 = d[i > 0 ? i - 1 : 0], x1 = d[i], x2 = d[i + 1 < len ? i + 1 : len - 1], x3 = d[i + 2 < len ? i + 2 : len - 1];
    const double c = (x2 - x0) * 0.5, v = x1 - x2, w = c + v, a = w + v + (x3 - x1) * 0.5, bb = w + a;
    return ((a * f - bb) * f + c) * f + x1;
}

struct Voice {
    Engine* e = nullptr;
    bool active = false, gate = false, fading = false; double fadeG = 1, fadeT = 0.004;
    int note = 60; double vel = 0.8; long age = 0; double pitch = 60, target = 60, rand = 0, alt = 1;
    LayerVoice lay[4]; Env m0, m1; Lfo lfo[4];
    double S[NSRC] = { 0 }, M[4 * NLD] = { 0 }, gm[4] = { 0 };
    void start(int n, double v, bool keep);
    void startLayer(int li, const Layer& L, LayerVoice& lv, bool cont);
    void excitePluck(const Layer& L, LayerVoice& lv, double freq, double sr);
    void legatoTo(int n);
    void release() { gate = false; m0.off(); m1.off(); for (auto& lv : lay) { lv.aenv.off(); lv.fenv.off(); } }
    void startFade(double secs = 0.004) { fading = true; fadeG = 1; fadeT = secs; gate = false; }
    double level() const;   // current loudness estimate (envelope x layer level), for picking which voice to steal
    void kill() { active = false; fading = false; gate = false; for (auto& lv : lay) lv.done = true; }
    void render(int off, int n);
    bool renderLayer(int li, const Layer& L, LayerVoice& lv, int n);
    bool oscAW(const Layer& L, LayerVoice& lv, int li, double freq, int n, int b);
    bool oscPluck(const Layer& L, LayerVoice& lv, double freq, int n, int b);
    bool oscModal(const Layer& L, LayerVoice& lv, double freq, int n, int b);
    bool oscFormant(const Layer& L, LayerVoice& lv, double freq, int n, int b);
    bool oscNoise(const Layer& L, LayerVoice& lv, double freq, int n, int b);
    bool oscGran(const Layer& L, LayerVoice& lv, int li, double semis, int n, int b);
    bool oscSample(const Layer& L, LayerVoice& lv, int li, double semis, int n, int b);
};

struct ArpNote { int n; double v; };
struct ArpPlay { int n; double off; };
struct Arp {
    Engine* e = nullptr;
    std::vector<ArpNote> held; std::vector<ArpPlay> play;
    bool running = false; long k = 0, i = 0; double nb = 0, next = 0; int cur = -1, phys = 0;
    Arp() { held.reserve(160); play.reserve(160); }
    void reset() { stop(); held.clear(); phys = 0; }
    void on(int n, double v); void off(int n); void stop(); void tick(double t0, int n); void doStep(double stepLen);
};

struct MasterFX {
    double sr = 48000, mod[NFXD] = { 0 };
    double dl[2] = { 0 };
    std::vector<float> lb[2], ld[2], cb[2];
    int lw = 0; double hc = 0, hold[2] = { 0 }, wph = 0, fph = 0, lt[2] = { 0 }, ck[2] = { 0 }, gv = 1;
    int cw = 0; double cph = 0, cph2 = 0, cfl[2] = { 0 };
    double pst[2][12] = { { 0 } }, plast[2] = { 0 }, pph = 0;
    MasterFX() { for (int c = 0; c < 2; c++) { lb[c].assign(8192, 0.0f); ld[c].assign(8192, 0.0f); cb[c].assign(8192, 0.0f); } }
    void process(Engine& e, float* L, float* R, int n);
};

class Engine {
public:
    explicit Engine(double sampleRate = 48000) { setSampleRate(sampleRate); for (auto& v : voices) v.e = this; arp.e = this; mono.reserve(160); mixL.assign(8192, 0.0f); mixR.assign(8192, 0.0f); }
    void setSampleRate(double s) { sr = s; fx.sr = s; }
    // Call from the audio thread (or while the engine is not processing). Build the bundle elsewhere with Assets::fill.
    void setBundle(const Assets::Bundle& b, bool resetVoices) {
        patch = b.p; for (int i = 0; i < 4; i++) { lcWt[i] = b.wt[i]; lcSrc[i] = b.src[i]; }
        if (resetVoices) { for (auto& v : voices) if (v.active) v.startFade(); arp.reset(); for (auto& m : inMap) m.count = -1; std::fill(cnt, cnt + 128, 0); mono.clear(); std::fill(susSet, susSet + 128, false); }
        else if (!patch.arp.on && arp.running) arp.stop();
    }
    void noteIn(int n, double v); void noteOut(int n);
    void vOn(int n, double v); void vOff(int n);
    void setSus(bool on) { sus = on; if (!on) { for (int x = 0; x < 128; x++) if (susSet[x]) { if (cnt[x] == 0) vOff(x); susSet[x] = false; } } }
    void panic() { for (auto& v : voices) v.kill(); arp.reset(); for (auto& m : inMap) m.count = -1; std::fill(cnt, cnt + 128, 0); mono.clear(); std::fill(susSet, susSet + 128, false); }
    void process(float* outL, float* outR, int n);
    int activeVoices() const { int c = 0; for (const auto& v : voices) if (v.active && !v.fading) c++; return c; }

    double sr = 48000; Patch patch; Rng rng;
    Voice voices[POOL]; Arp arp; MasterFX fx;
    double time = 0, gtime = 0, bpm = 90, bps = 1.5;
    double mw = 0, mwT = 0, at = 0, atT = 0, pb = 0, pbT = 0, bendSemis = 0;
    double mac[4] = { 0, 0, 0, 0 }, ox = 0, oy = 0, orbT = 0;
    double fxm[NFXD] = { 0 };          // global-source modulation of effect parameters, refreshed every block
    int lastNote = -1; double altF = 1, gateStart = 0, monoVel = 0.8;
    bool fxModEnabled = true;          // the port test turns this off to compare with the bare browser engine
    bool sus = false, susSet[128] = { false };
    struct InRec { int count = -1; int notes[8]; bool arp = false; } inMap[128];
    int cnt[128] = { 0 }; std::vector<int> mono;
    const Wavetable* lcWt[4] = { nullptr, nullptr, nullptr, nullptr }; const Source* lcSrc[4] = { nullptr, nullptr, nullptr, nullptr };
    float sL[CB], sR[CB], sL2[CB], sR2[CB], vL[CB], vR[CB];
    std::vector<float> mixL, mixR;
private:
    int snap(int n) const; int chordOf(int base, int* out) const; void tickGlobal(int n);
};

// ================================================================== implementation
inline void Voice::start(int n, double v, bool keep) {
    Engine& E = *e; const Patch& P = E.patch;
    const bool was = active && !fading;
    age = 0; active = true; fading = false; fadeG = 1; gate = true; note = n; vel = v;
    const bool glideOn = P.g.glide > 0.001 && (P.g.gmode == 0 || keep) && E.lastNote >= 0;
    pitch = glideOn ? (was ? pitch : E.lastNote) : n;
    target = n; E.lastNote = n;
    rand = E.rng.next() * 2 - 1; E.altF = -E.altF; alt = E.altF;
    m0.on(); m1.on();
    if (!was) { m0.v = 0; m1.v = 0; }
    for (int k = 0; k < 4; k++) { if (P.lfo[k].rtg) lfo[k].reset(P.lfo[k], E.rng); else lfo[k].t = 0; }
    for (int li = 0; li < 4; li++) {
        const Layer& L = P.L[li]; LayerVoice& lv = lay[li];
        if (!L.on) { lv.done = true; continue; }
        startLayer(li, L, lv, was && !lv.done);
    }
}
inline void Voice::startLayer(int, const Layer& L, LayerVoice& lv, bool cont) {
    Engine& E = *e; const double sr = E.sr;
    const bool fresh = !cont || lv.mode != L.mode;
    lv.done = false; lv.quiet = 0; lv.aenv.on(); lv.fenv.on();
    if (fresh) {
        lv.f1.reset(); lv.f1b.reset(); lv.f2.reset(); lv.f2b.reset(); lv.fo.reset(); lv.fob.reset();
        lv.g0l = 0; lv.g0r = 0; lv.aenv.v = 0; lv.fenv.v = 0; lv.mode = L.mode;
        std::fill(lv.drift, lv.drift + 8, 0.0); lv.subph = 0; lv.gn = 0;
        std::fill(lv.my1, lv.my1 + 12, 0.0); std::fill(lv.my2, lv.my2 + 12, 0.0); std::fill(lv.pk, lv.pk + 8, 0.0);
        lv.brn[0] = lv.brn[1] = lv.clk[0] = lv.clk[1] = lv.hp[0] = lv.hp[1] = 0;
        for (int u = 0; u < 8; u++) { const double r = L.uni > 1 ? E.rng.next() : 0; lv.ph[u] = r; lv.mph[u] = r; lv.fph[u] = 0; lv.rph[u] = 0; }
        for (auto& s : lv.str) std::fill(s.begin(), s.end(), 0.0f);
        lv.slp[0] = lv.slp[1] = lv.slp[2] = 0;
    }
    const double freq = mtof((L.kt ? pitch : 60) + L.crs + L.fin * 0.01);
    switch (L.mode) {
        case 2: excitePluck(L, lv, freq, sr); break;
        case 3: lv.imp = 1; lv.exn = L.mhard * 0.6; lv.exd = std::exp(-1 / (0.0025 * sr)); lv.mkey = -1; break;
        case 6: lv.gcnt = 0; lv.head = 0; break;
        case 7: lv.sfirst = true; lv.sdone = false; break;
        default: break;
    }
}
inline void Voice::excitePluck(const Layer& L, LayerVoice& lv, double freq, double sr) {
    Rng& rng = e->rng;
    const int ns = std::max(1, std::min(3, L.uni)); const double c = 0.04 + 0.96 * L.pexc * L.pexc;
    float ex[4096];
    for (int s = 0; s < ns; s++) {
        const double off = ns > 1 ? ((double)s / (ns - 1)) * 2 - 1 : 0;
        const double f = freq * std::pow(2.0, L.det * off * 0.3 / 1200);
        const int P = (int)clampd(std::floor(sr / f + 0.5), 2, 4000), pp = std::max(1, (int)std::floor(L.ppos * P + 0.5));
        double lp = 0, pk = 0;
        for (int j = 0; j < P; j++) { lp += c * ((rng.next() * 2 - 1) - lp); ex[j] = (float)lp; }
        for (int j = P - 1; j >= pp; j--) ex[j] -= ex[j - pp];
        for (int j = 0; j < P; j++) { const double a = std::abs(ex[j]); if (a > pk) pk = a; }
        const double nm = pk > 1e-9 ? 0.9 / pk : 0; float* B = lv.str[s].data();
        for (int j = 0; j < P; j++) { const int idx = (lv.sw - P + j) & 4095; B[idx] = (float)(B[idx] + ex[j] * nm); }
    }
}
inline double Voice::level() const {
    double m = 0; const Patch& P = e->patch;
    for (int i = 0; i < 4; i++) if (!lay[i].done && P.L[i].on) m = std::max(m, lay[i].aenv.v * P.L[i].lvl);
    return m;
}
inline void Voice::legatoTo(int n) { note = n; target = n; if (e->patch.g.glide <= 0.001) pitch = n; e->lastNote = n; }

inline void Voice::render(int off, int n) {
    Engine& E = *e; const Patch& P = E.patch; const double sr = E.sr;
    age += n;
    if (pitch != target) {
        const double gt = P.g.glide;
        if (gt <= 0.001) pitch = target;
        else { pitch += (target - pitch) * (1 - std::exp(-n * 3 / (gt * sr))); if (std::abs(target - pitch) < 0.002) pitch = target; }
    }
    m0.tick(P.me[0].a, P.me[0].d, P.me[0].s, P.me[0].r, n, sr, P.me[0].loop);
    m1.tick(P.me[1].a, P.me[1].d, P.me[1].s, P.me[1].r, n, sr, P.me[1].loop);
    for (int k = 0; k < 4; k++) {
        const LfoP& lp = P.lfo[k];
        double hz = lp.sync ? E.bps / lp.div : lp.rate;
        if (gm[k] != 0) hz *= std::pow(2.0, gm[k] * 4);
        S[3 + k] = lfo[k].tick(lp, k + 1, hz, n, sr, E.gtime, E.rng);
    }
    S[1] = m0.v; S[2] = m1.v; S[7] = vel; S[8] = (note - 60) / 24.0; S[9] = E.mw; S[10] = E.at; S[11] = rand;
    S[12] = E.mac[0]; S[13] = E.mac[1]; S[14] = E.mac[2]; S[15] = E.mac[3]; S[16] = E.ox; S[17] = E.oy; S[18] = alt; S[19] = E.pb;
    std::fill(M, M + 4 * NLD, 0.0); gm[0] = gm[1] = gm[2] = gm[3] = 0;
    for (int r = 0; r < P.nmods; r++) {
        const ModRoute& m = P.mods[r];
        if (m.tgt > 5) continue;
        const double val = S[m.s] * m.a * (m.v ? S[m.v] : 1);
        if (val == 0) continue;
        if (m.tgt < 4) M[m.tgt * NLD + m.pi] += val;
        else if (m.tgt == 4) { M[m.pi] += val; M[NLD + m.pi] += val; M[2 * NLD + m.pi] += val; M[3 * NLD + m.pi] += val; }
        else gm[m.pi] += val;
    }
    float* vL = E.vL; float* vR = E.vR;
    for (int i = 0; i < n; i++) { vL[i] = 0; vR[i] = 0; }
    bool live = false;
    for (int li = 0; li < 4; li++) {
        const Layer& L = P.L[li]; LayerVoice& lv = lay[li];
        if (lv.done) continue;
        if (!L.on) { lv.done = true; continue; }
        if (renderLayer(li, L, lv, n)) live = true;
    }
    float* mL = E.mixL.data() + off; float* mR = E.mixR.data() + off;
    if (fading) {
        const double g0 = fadeG; fadeG = std::max(0.0, g0 - n / (fadeT * sr));
        const double dg = (fadeG - g0) / n; double g = g0;
        for (int i = 0; i < n; i++) { g += dg; mL[i] = (float)(mL[i] + vL[i] * g); mR[i] = (float)(mR[i] + vR[i] * g); }
        if (fadeG <= 0) live = false;
    } else for (int i = 0; i < n; i++) { mL[i] += vL[i]; mR[i] += vR[i]; }
    if (!live) kill();
}

inline bool Voice::renderLayer(int li, const Layer& L, LayerVoice& lv, int n) {
    Engine& E = *e; const double sr = E.sr; const int b = li * NLD;
    const double ta = L.aa * (M[b + 32] != 0 ? std::pow(2.0, M[b + 32] * 4) : 1);
    const double td = L.ad * (M[b + 33] != 0 ? std::pow(2.0, M[b + 33] * 4) : 1);
    const double trl = L.ar * (M[b + 34] != 0 ? std::pow(2.0, M[b + 34] * 4) : 1);
    lv.aenv.tick(ta, td, L.as, trl, n, sr, 0);
    lv.fenv.tick(L.fa, L.fd * (M[b + 35] != 0 ? std::pow(2.0, M[b + 35] * 4) : 1), L.fs, L.fr, n, sr, 0);
    const double semis = (L.kt ? pitch : 60) + L.crs + L.fin * 0.01 + M[b] * 24 + E.bendSemis;
    const double freq = 440 * std::pow(2.0, (semis - 69) / 12);
    float* bL = E.sL; float* bR = E.sR; bool stereo = false;
    switch (L.mode) {
        case 0: case 1: stereo = oscAW(L, lv, li, freq, n, b); break;
        case 2: stereo = oscPluck(L, lv, freq, n, b); break;
        case 3: stereo = oscModal(L, lv, freq, n, b); break;
        case 4: stereo = oscFormant(L, lv, freq, n, b); break;
        case 5: stereo = oscNoise(L, lv, freq, n, b); break;
        case 6: stereo = oscGran(L, lv, li, semis, n, b); break;
        default: stereo = oscSample(L, lv, li, semis, n, b); break;
    }
    const double kn = (pitch - 60) / 119.6, fe = lv.fenv.v;
    const double c1 = clampd(L.f1c + M[b + 24] + fe * L.f1e + L.f1k * kn + L.f1v * (vel - 1) * 0.4, 0, 1);
    const double r1 = clampd(L.f1r + M[b + 25], 0, 1), d1 = clampd(L.f1d + M[b + 26], 0, 1);
    const double c2 = clampd(L.f2c + M[b + 27] + fe * L.f2e + L.f2k * kn + L.f2v * (vel - 1) * 0.4, 0, 1);
    const double r2 = clampd(L.f2r + M[b + 28], 0, 1), d2 = clampd(L.f2d, 0, 1);
    if (!L.frt) {
        if (L.f1t) { filt(lv.f1, L.f1t, bL, n, c1, r1, d1, sr); if (stereo) filt(lv.f1b, L.f1t, bR, n, c1, r1, d1, sr); }
        if (L.f2t) { filt(lv.f2, L.f2t, bL, n, c2, r2, d2, sr); if (stereo) filt(lv.f2b, L.f2t, bR, n, c2, r2, d2, sr); }
    } else if (L.f1t || L.f2t) {
        float* pL = E.sL2; float* pR = E.sR2;
        for (int i = 0; i < n; i++) { pL[i] = bL[i]; pR[i] = bR[i]; }
        if (L.f1t) { filt(lv.f1, L.f1t, bL, n, c1, r1, d1, sr); if (stereo) filt(lv.f1b, L.f1t, bR, n, c1, r1, d1, sr); }
        if (L.f2t) { filt(lv.f2, L.f2t, pL, n, c2, r2, d2, sr); if (stereo) filt(lv.f2b, L.f2t, pR, n, c2, r2, d2, sr); }
        const double bal = clampd(L.fbal + M[b + 29], 0, 1), ba = 1 - bal;
        for (int i = 0; i < n; i++) { bL[i] = (float)(bL[i] * ba + pL[i] * bal); bR[i] = (float)(bR[i] * ba + pR[i] * bal); }
    }
    const double vg = 1 - L.vs + L.vs * std::pow(vel, 1.6);
    const double lm = M[b + 30] != 0 ? std::max(0.0, 1 + M[b + 30]) : 1;
    const double amp = lv.aenv.v * L.lvl * vg * lm * VOICE_GAIN;
    const double pan = clampd(L.pan + M[b + 31], -1, 1);
    const double gl1 = amp * (pan > 0 ? 1 - pan : 1), gr1 = amp * (pan < 0 ? 1 + pan : 1);
    const double dl = (gl1 - lv.g0l) / n, dr = (gr1 - lv.g0r) / n; float* vL = E.vL; float* vR = E.vR;
    double gl = lv.g0l, gr = lv.g0r, pk = 0;
    for (int i = 0; i < n; i++) {
        gl += dl; gr += dr; const double a = bL[i], c = stereo ? bR[i] : a;
        vL[i] = (float)(vL[i] + a * gl); vR[i] = (float)(vR[i] + c * gr);
        const double q = a < 0 ? -a : a; if (q > pk) pk = q;
    }
    lv.g0l = gl1; lv.g0r = gr1;
    if (lv.aenv.st == 0 || lv.sdone) { lv.done = true; return false; }
    if ((L.mode == 2 || L.mode == 3) && lv.aenv.st >= 2) { if (pk < 3e-5) { if (++lv.quiet > 40) { lv.done = true; return false; } } else lv.quiet = 0; }
    return true;
}

inline bool Voice::oscAW(const Layer& L, LayerVoice& lv, int li, double freq, int n, int b) {
    Engine& E = *e; const double sr = E.sr; float* bL = E.sL; float* bR = E.sR; Rng& rng = E.rng;
    const Statics& ST = statics(); const float* SINE = ST.sine.data();
    const int uni = std::max(1, std::min(7, L.uni));
    const double det = std::max(0.0, L.det + M[b + 10] * 100), spr = clampd(L.spr + M[b + 11], 0, 1);
    const double base = freq / sr; const int lo = lvlFor(freq * (uni > 1 ? std::pow(2.0, det / 1200) : 1), sr) * TBL;
    const double dr = L.drift, nrm = 1 / std::sqrt((double)uni);
    for (int u = 0; u < uni; u++) {
        if (dr > 0) lv.drift[u] += (rng.next() - 0.5) * 0.6 * dr - lv.drift[u] * 0.002;
        const double o = uni > 1 ? ((double)u / (uni - 1)) * 2 - 1 : 0;
        lv.incs[u] = base * std::pow(2.0, (det * o + lv.drift[u] * 4 * dr) / 1200);
        const double a = (o * spr + 1) * PI / 4;
        lv.gl[u] = std::cos(a) * nrm * 1.41421; lv.gr[u] = std::sin(a) * nrm * 1.41421;
    }
    const double fmA = clampd(L.fm + M[b + 5], 0, 1), pmS = fmA * fmA * 12 / TAU, fmr = L.fmr;
    const double ring = clampd(L.ring + M[b + 6], 0, 1), rr = L.rngr, ring1 = 1 - ring;
    const double fold = clampd(L.fold + M[b + 7], 0, 1), fg = 0.25 + fold * 2.5, fmix = std::min(1.0, fold * 4);
    const double sync = clampd(L.sync + M[b + 3], 0, 1), sratio = 1 + sync * 7;
    const bool useFM = pmS > 0, useRing = ring > 0, useFold = fold > 0, useSync = sync > 0.001;
    double* ph = lv.ph; double* mph = lv.mph; double* fph = lv.fph; double* rph = lv.rph;
    const Wavetable* T = E.lcWt[li];
    const bool wtMode = L.mode == 1 && T != nullptr;
    const float* SAWT = ST.saw.data() + lo; const float* TRIT = ST.tri.data() + lo;
    const double shp = clampd(L.shp + M[b + 1], 0, 1), x = shp * 3; const int seg = x >= 2 ? 2 : (int)x; const double w = x - seg, w1 = 1 - w;
    const double pw = clampd(L.pw + M[b + 2], 0.02, 0.98);
    const float* DA = nullptr; const float* DB = nullptr; double ww = 0, ww1 = 1;
    if (wtMode) {
        const double pos = clampd(L.pos + M[b + 4], 0, 1) * (T->F - 1); const int f0 = std::min(T->F - 2, (int)pos);
        ww = pos - f0; ww1 = 1 - ww; DA = T->data.data() + (size_t)f0 * NLEV * TBL + lo; DB = DA + NLEV * TBL;
    }
    for (int i = 0; i < n; i++) {
        double sl = 0, sr2 = 0;
        for (int u = 0; u < uni; u++) {
            const double inc = lv.incs[u]; double p;
            if (useSync) { double m = mph[u] + inc; if (m >= 1) { m -= 1; p = m * sratio; } else p = ph[u] + inc * sratio; mph[u] = m; p -= std::floor(p); }
            else { p = ph[u] + inc; if (p >= 1) p -= 1; }
            ph[u] = p;
            double q = p;
            if (useFM) { double f = fph[u] + inc * fmr; f -= std::floor(f); fph[u] = f; q += pmS * SINE[(int)(f * TBL)]; q -= std::floor(q); }
            double v;
            if (wtMode) {
                const double xq = q * TBL; const int iq = (int)xq & TMASK; const double fq = xq - (int)xq; const int i1 = (iq + 1) & TMASK;
                const double a0 = DA[iq], b0 = DB[iq];
                v = (a0 + fq * (DA[i1] - a0)) * ww1 + (b0 + fq * (DB[i1] - b0)) * ww;
            } else if (L.mode == 1) v = 0;
            else if (seg == 0) v = rdT(SINE, q) * w1 + rdT(TRIT, q) * w;
            else if (seg == 1) v = rdT(TRIT, q) * w1 + rdT(SAWT, q) * w;
            else { double q2 = q + pw; if (q2 >= 1) q2 -= 1; const double s = rdT(SAWT, q); v = s * w1 + (s - rdT(SAWT, q2)) * w; }
            if (useRing) { double r = rph[u] + inc * rr; r -= std::floor(r); rph[u] = r; v *= ring1 + ring * SINE[(int)(r * TBL)]; }
            if (useFold) { const double xx = v * fg; v += fmix * (rdT(SINE, xx - std::floor(xx)) - v); }
            sl += v * lv.gl[u]; sr2 += v * lv.gr[u];
        }
        bL[i] = (float)sl; bR[i] = (float)sr2;
    }
    const double sub = clampd(L.sub + M[b + 8], 0, 1), nz = clampd(L.nz + M[b + 9], 0, 1);
    if (sub > 0.001) {
        double sp = lv.subph; const double si = base * 0.5;
        for (int i = 0; i < n; i++) { sp += si; if (sp >= 1) sp -= 1; const double s = rdT(SINE, sp) * sub; bL[i] = (float)(bL[i] + s); bR[i] = (float)(bR[i] + s); }
        lv.subph = sp;
    }
    if (nz > 0.001) { const double a = nz * 0.7; for (int i = 0; i < n; i++) { const double wn = (rng.next() * 2 - 1) * a; bL[i] = (float)(bL[i] + wn); bR[i] = (float)(bR[i] + wn); } }
    return uni > 1 && spr > 0.01;
}

inline bool Voice::oscPluck(const Layer& L, LayerVoice& lv, double freq, int n, int b) {
    Engine& E = *e; const double sr = E.sr; float* bL = E.sL; float* bR = E.sR;
    const int ns = std::max(1, std::min(3, L.uni)); const double damp = clampd(L.pdmp + M[b + 12], 0, 1), a = 1 - 0.88 * damp;
    const double T60 = std::max(0.05, L.pdec) * std::pow(2.0, -(pitch - 60) / 36), spr = clampd(L.spr + M[b + 11], 0, 1);
    double accL[CB] = { 0 }, accR[CB] = { 0 };
    const int w0 = lv.sw;
    for (int s = 0; s < ns; s++) {
        const double off = ns > 1 ? ((double)s / (ns - 1)) * 2 - 1 : 0;
        const double f = freq * std::pow(2.0, L.det * off * 0.3 / 1200);
        const double D = clampd(sr / f - (1 - a) / a, 2, 4000), g = std::pow(10.0, -3 / (T60 * f));
        const double pa = (off * spr + 1) * PI / 4, gl = std::cos(pa) * 1.41421 / std::sqrt((double)ns), gr = std::sin(pa) * 1.41421 / std::sqrt((double)ns);
        float* B = lv.str[s].data(); int w = w0; double lp = lv.slp[s];
        for (int i = 0; i < n; i++) {
            double rp = w - D; if (rp < 0) rp += 4096;
            const int i0 = (int)rp; const double fr = rp - i0, x0 = B[i0], y = x0 + fr * (B[(i0 + 1) & 4095] - x0);
            lp += a * (y - lp);
            B[w] = (float)(lp * g); w = (w + 1) & 4095;
            accL[i] += y * gl * 2; accR[i] += y * gr * 2;
        }
        lv.slp[s] = (lp < 1e-18 && lp > -1e-18) ? 0 : lp;
    }
    for (int i = 0; i < n; i++) { bL[i] = (float)accL[i]; bR[i] = (float)accR[i]; }
    lv.sw = (w0 + n) & 4095;
    return ns > 1 && spr > 0.01;
}

inline bool Voice::oscModal(const Layer& L, LayerVoice& lv, double freq, int n, int b) {
    Engine& E = *e; const double sr = E.sr; float* bL = E.sL; float* bR = E.sR; Rng& rng = E.rng;
    const Model& md = models()[(size_t)std::max(0, std::min((int)models().size() - 1, L.mdl))];
    const double bri = clampd(L.mbri + M[b + 13], 0, 1), spr = clampd(L.spr + M[b + 11], 0, 1);
    const double key = freq * 1.000137 + bri * 7.1 + L.mdec * 13.3 + L.minh * 17.7 + L.mhard * 3.3 + spr * 2.9 + L.mbow * 5.1 + L.mdl * 1009.0;
    const int np = (int)md.r.size();
    if (key != lv.mkey) {
        lv.mkey = key;
        const double B = L.minh * 0.002, ks = std::pow(2.0, -(pitch - 60) / 24), tilt = (bri - 0.5) * 2, hard = -(1 - L.mhard) * 0.8;
        int m = 0;
        for (int k = 0; k < np; k++) {
            const double r0 = md.r[(size_t)k], rk = r0 * std::sqrt(1 + B * r0 * r0), wv = TAU * freq * rk / sr;
            if (wv > 2.9) continue;
            const double T60 = std::max(0.02, L.mdec * md.d[(size_t)k] * ks), R = std::exp(-6.9078 / (T60 * sr));
            const double amp = md.a[(size_t)k] * std::pow(rk, tilt + hard);
            lv.mc1[m] = 2 * R * std::cos(wv); lv.mc2[m] = R * R; lv.mg[m] = amp * std::sin(wv); lv.mb[m] = amp * std::sin(wv) * std::sqrt(2 * (1 - R * R)) * 2.5;
            const double pn = ((k & 1) ? -1 : 1) * spr * std::min(1.0, k * 0.5), pa = (pn + 1) * PI / 4;
            lv.mpl[m] = std::cos(pa) * 1.41421; lv.mpr[m] = std::sin(pa) * 1.41421;
            m++;
        }
        lv.mn = m;
    }
    const int nm = lv.mn;
    const double bow = L.mbow > 0.001 ? L.mbow * 0.5 * (0.3 + 0.7 * lv.aenv.v) : 0;
    double imp = lv.imp, exn = lv.exn; const double exd = lv.exd;
    for (int i = 0; i < n; i++) {
        double x = imp; imp = 0;
        if (exn > 1e-5) { x += (rng.next() * 2 - 1) * exn; exn *= exd; }
        const double bw = bow > 0 ? (rng.next() * 2 - 1) * bow : 0;
        double sl = 0, sr2 = 0;
        for (int k = 0; k < nm; k++) {
            const double y = lv.mc1[k] * lv.my1[k] - lv.mc2[k] * lv.my2[k] + lv.mg[k] * x + lv.mb[k] * bw;
            lv.my2[k] = lv.my1[k]; lv.my1[k] = y; sl += y * lv.mpl[k]; sr2 += y * lv.mpr[k];
        }
        bL[i] = (float)sl; bR[i] = (float)sr2;
    }
    for (int k = 0; k < nm; k++) { fl0(lv.my1[k]); fl0(lv.my2[k]); }
    lv.imp = imp; lv.exn = exn;
    return spr > 0.01;
}

inline bool Voice::oscFormant(const Layer& L, LayerVoice& lv, double freq, int n, int b) {
    Engine& E = *e; const double sr = E.sr; float* bL = E.sL; float* bR = E.sR; Rng& rng = E.rng;
    const int uni = std::max(1, std::min(7, L.uni)); const double det = std::max(0.0, L.det + M[b + 10] * 100), spr = clampd(L.spr + M[b + 11], 0, 1);
    const double base = freq / sr; const int lo = lvlFor(freq * std::pow(2.0, det / 1200), sr) * TBL; const double dr = std::max(L.drift, 0.2), nrm = 1 / std::sqrt((double)uni);
    for (int u = 0; u < uni; u++) {
        lv.drift[u] += (rng.next() - 0.5) * 0.6 * dr - lv.drift[u] * 0.002;
        const double o = uni > 1 ? ((double)u / (uni - 1)) * 2 - 1 : 0;
        lv.incs[u] = base * std::pow(2.0, (det * o + lv.drift[u] * 4 * dr) / 1200);
        const double a = (o * spr + 1) * PI / 4;
        lv.gl[u] = std::cos(a) * nrm * 1.41421; lv.gr[u] = std::sin(a) * nrm * 1.41421;
    }
    const double shp = clampd(L.shp + M[b + 1], 0, 1), brth = clampd(L.brth + M[b + 16], 0, 1) * 1.6;
    const float* SAWT = statics().saw.data() + lo;
    for (int i = 0; i < n; i++) {
        double sl = 0, sr2 = 0;
        for (int u = 0; u < uni; u++) {
            double p = lv.ph[u] + lv.incs[u]; if (p >= 1) p -= 1; lv.ph[u] = p;
            double q2 = p + 0.5; if (q2 >= 1) q2 -= 1;
            const double s = rdT(SAWT, p), v = s + shp * (-rdT(SAWT, q2));
            sl += v * lv.gl[u]; sr2 += v * lv.gr[u];
        }
        if (brth > 0) { const double w = (rng.next() * 2 - 1) * brth; sl += w; sr2 += w * 0.9 + (rng.next() * 2 - 1) * brth * 0.1; }
        bL[i] = (float)sl; bR[i] = (float)sr2;
    }
    const double vow = clampd(L.vow + M[b + 14], 0, 1), sh = std::pow(2.0, (clampd(L.fsh + M[b + 15], 0, 1) - 0.5) * 1.4);
    vowelF(lv.fo, bL, n, vow, 0.5, sr, sh); vowelF(lv.fob, bR, n, vow, 0.5, sr, sh);
    for (int i = 0; i < n; i++) { bL[i] *= 1.5f; bR[i] *= 1.5f; }
    return true;
}

inline bool Voice::oscNoise(const Layer& L, LayerVoice& lv, double freq, int n, int b) {
    Engine& E = *e; const double sr = E.sr; Rng& rng = E.rng;
    const double col = clampd(L.ncol + M[b + 17], 0, 1); const int type = L.ntyp; const double spr = clampd(L.spr + M[b + 11], 0, 1); const bool st = spr > 0.01;
    if (type == 4) { lv.wv += (rng.next() - 0.5) * 0.08; lv.wv *= 0.995; lv.wc = clampd(lv.wc + lv.wv * 0.02, 0, 1); if (lv.wc <= 0 || lv.wc >= 1) lv.wv = -lv.wv; }
    for (int c = 0; c < (st ? 2 : 1); c++) {
        float* out = c ? E.sR : E.sL; double* pk = lv.pk; const int o = c * 4;
        double b0 = pk[o], b1 = pk[o + 1], b2 = pk[o + 2], br = lv.brn[c], cl = lv.clk[c], hp = lv.hp[c];
        switch (type) {
            case 0:
                for (int i = 0; i < n; i++) {
                    const double w = rng.next() * 2 - 1;
                    b0 = 0.99765 * b0 + w * 0.099046; b1 = 0.963 * b1 + w * 0.2965164; b2 = 0.57 * b2 + w * 1.0526913;
                    const double pink = (b0 + b1 + b2 + w * 0.1848) * 0.22;
                    br = (br + 0.02 * w) / 1.02; const double brown = br * 3.5;
                    out[i] = (float)((col < 0.5 ? brown + (pink - brown) * col * 2 : pink + (w * 0.6 - pink) * (col - 0.5) * 2) * 0.8);
                }
                break;
            case 1: {
                const double prob = 0.0004 + col * 0.006;
                for (int i = 0; i < n; i++) {
                    const double w = rng.next() * 2 - 1;
                    br = (br + 0.02 * w) / 1.02;
                    if (rng.next() < prob) { const double r = rng.next(); cl += (rng.next() < 0.5 ? -1 : 1) * (0.25 + r * r * 0.75); }
                    cl *= 0.62;
                    out[i] = (float)((cl + br * 0.25 + w * 0.012) * 3);
                }
            } break;
            case 2: {
                const double prob = 0.004 + col * 0.03;
                for (int i = 0; i < n; i++) {
                    const double w = rng.next() * 2 - 1;
                    b0 = 0.99765 * b0 + w * 0.099046; b1 = 0.963 * b1 + w * 0.2965164; b2 = 0.57 * b2 + w * 1.0526913;
                    if (rng.next() < prob) { const double r = rng.next(); cl += (rng.next() < 0.5 ? -1 : 1) * r * r * 0.8; }
                    const double x = cl; cl *= 0.35;
                    hp += 0.15 * (x - hp);
                    out[i] = (float)((x - hp) * 1.3 + (b0 + b1 + b2) * 0.03);
                }
            } break;
            case 3: {
                double nph = lv.nph, hold = lv.hold[c]; const double inc = freq / sr;
                for (int i = 0; i < n; i++) { nph += inc; if (nph >= 1) { nph -= std::floor(nph); hold = rng.next() * 2 - 1; } out[i] = (float)((hold * (0.4 + col * 0.4) + (rng.next() * 2 - 1) * (1 - col) * 0.15) * 1.5); }
                if (!c || !st) lv.nph = nph;
                lv.hold[c] = hold;
            } break;
            default: {
                const double hz = std::min(250 * std::pow(2.0, lv.wc * 3.5 + (col - 0.5) * 2), sr * 0.4), g = std::tan(PI * hz / sr), k = 0.3, a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
                double i1 = cl, i2 = hp;
                for (int i = 0; i < n; i++) {
                    const double w = rng.next() * 2 - 1;
                    b0 = 0.99765 * b0 + w * 0.099046; b1 = 0.963 * b1 + w * 0.2965164; b2 = 0.57 * b2 + w * 1.0526913;
                    const double x = (b0 + b1 + b2 + w * 0.1848) * 0.22, v3 = x - i2, v1 = a1 * i1 + a2 * v3, v2 = i2 + a2 * i1 + a3 * v3;
                    i1 = 2 * v1 - i1; i2 = 2 * v2 - i2;
                    out[i] = (float)(k * v1 * 3 + x * 0.08);
                }
                cl = i1; hp = i2;
            }
        }
        pk[o] = b0; pk[o + 1] = b1; pk[o + 2] = b2; lv.brn[c] = br; lv.clk[c] = cl; lv.hp[c] = hp;
    }
    return st;
}

inline bool Voice::oscGran(const Layer& L, LayerVoice& lv, int li, double semis, int n, int b) {
    Engine& E = *e; const double sr = E.sr; float* bL = E.sL; float* bR = E.sR; Rng& rng = E.rng;
    const Source* src = E.lcSrc[li];
    if (!src || src->data.size() < 8) { for (int i = 0; i < n; i++) { bL[i] = 0; bR[i] = 0; } return false; }
    const float* data = src->data.data(); const int len = (int)src->data.size(); const double rsr = src->sr / sr;
    const double pos = clampd(L.gpos + M[b + 18], 0, 1), spray = clampd(L.gspr + M[b + 21], 0, 1);
    const double sizeS = 0.005 * std::pow(200.0, clampd(L.gsz + M[b + 19], 0, 1)), dens = std::pow(200.0, clampd(L.gden + M[b + 20], 0, 1));
    const double scat = clampd(L.gpit + M[b + 22], 0, 1) * 12, spr = clampd(L.spr + M[b + 11], 0, 1);
    const double st = L.kt ? semis - src->root : semis - 60;
    lv.head += L.gscan * n * rsr / len; lv.head -= std::floor(lv.head);
    const double interval = sr / dens, norm = 1.25 / std::sqrt(std::max(1.0, dens * sizeS)), durS = std::max(16.0, sizeS * sr);
    const float* SINE = statics().sine.data();
    int gn = lv.gn; double cnt = lv.gcnt;
    for (int i = 0; i < n; i++) {
        if (--cnt <= 0) {
            cnt += interval * (0.5 + rng.next());
            if (gn < 32) {
                double p = pos + lv.head + (rng.next() * 2 - 1) * spray * 0.5; p -= std::floor(p);
                lv.gpos[gn] = p * (len - 1); lv.gdur[gn] = durS; lv.gage[gn] = 0;
                double inc = rsr * std::pow(2.0, (st + (rng.next() * 2 - 1) * scat) / 12);
                if (L.grev > 0 && rng.next() < L.grev) inc = -inc;
                lv.ginc[gn] = inc;
                const double pa = ((rng.next() * 2 - 1) * spr + 1) * PI / 4; lv.ggl[gn] = std::cos(pa) * 1.41421; lv.ggr[gn] = std::sin(pa) * 1.41421;
                gn++;
            }
        }
        double sl = 0, sr2 = 0;
        for (int k = 0; k < gn; k++) {
            double p = lv.gpos[k];
            if (p >= len - 1) p -= len - 1; else if (p < 0) p += len - 1;
            if (p < 0 || p >= len - 1) p = 0;
            const int i0 = (int)p; const double f = p - i0, a = data[i0], s = a + f * (data[i0 + 1] - a);
            const double t = lv.gage[k] / lv.gdur[k], win = 0.5 - 0.5 * SINE[(int)((t + 0.25) * TBL) & TMASK];
            const double v = s * win; sl += v * lv.ggl[k]; sr2 += v * lv.ggr[k];
            lv.gpos[k] = p + lv.ginc[k];
            if (++lv.gage[k] >= lv.gdur[k]) { gn--; lv.gpos[k] = lv.gpos[gn]; lv.ginc[k] = lv.ginc[gn]; lv.gage[k] = lv.gage[gn]; lv.gdur[k] = lv.gdur[gn]; lv.ggl[k] = lv.ggl[gn]; lv.ggr[k] = lv.ggr[gn]; k--; }
        }
        bL[i] = (float)(sl * norm); bR[i] = (float)(sr2 * norm);
    }
    lv.gn = gn; lv.gcnt = cnt;
    return true;
}

inline bool Voice::oscSample(const Layer& L, LayerVoice& lv, int li, double semis, int n, int b) {
    Engine& E = *e; const double sr = E.sr; float* bL = E.sL;
    const Source* src = E.lcSrc[li];
    if (!src || src->data.size() < 8) { for (int i = 0; i < n; i++) bL[i] = 0; return false; }
    const float* data = src->data.data(); const int len = (int)src->data.size();
    if (lv.sfirst) { lv.sfirst = false; lv.spos = clampd(L.sst + M[b + 23], 0, 0.99) * len; }
    const double st = L.kt ? semis - src->root : semis - 60;
    const double inc = src->sr / sr * std::pow(2.0, st / 12);
    const bool loop = L.sloop && src->le > src->ls + 256; const double le = src->le, ll = src->le - src->ls, xf = std::min(ll * 0.25, src->sr * 0.03);
    double p = lv.spos;
    for (int i = 0; i < n; i++) {
        if (p >= (loop ? le : len - 3)) {
            if (loop) { p -= ll; if (p >= le) p = src->ls; }
            else { lv.sdone = true; for (int j = i; j < n; j++) bL[j] = 0; break; }
        }
        double s = herm(data, p, len);
        if (loop && p > le - xf) { const double t = (le - p) / xf; s = s * t + herm(data, std::max(0.0, p - ll), len) * (1 - t); }
        bL[i] = (float)s; p += inc;
    }
    lv.spos = p;
    return false;
}

// ------------------------------------------------------------------ arp
inline void Arp::on(int n, double v) {
    const auto& A = e->patch.arp;
    if (A.latch && phys == 0) held.clear();
    phys++;
    for (size_t j = 0; j < held.size(); j++) if (held[j].n == n) { held.erase(held.begin() + (long)j); break; }
    if (held.size() < 128) held.push_back({ n, v });
    if (!running) { running = true; k = 0; i = 0; nb = e->time; next = e->time; }
}
inline void Arp::off(int n) {
    phys = std::max(0, phys - 1);
    if (e->patch.arp.latch) return;
    for (size_t j = 0; j < held.size(); j++) if (held[j].n == n) { held.erase(held.begin() + (long)j); break; }
    if (held.empty()) stop();
}
inline void Arp::stop() { running = false; cur = -1; for (const auto& p : play) e->vOff(p.n); play.clear(); }
inline void Arp::tick(double t0, int n) {
    for (int j = (int)play.size() - 1; j >= 0; j--) if (play[(size_t)j].off < t0 + n) { e->vOff(play[(size_t)j].n); play.erase(play.begin() + j); }
    if (!running) return;
    const double stepLen = 60 / e->bpm * e->patch.arp.div * e->sr;
    int guard = 0;
    while (next < t0 + n && guard++ < 8) {
        doStep(stepLen);
        k++; nb += stepLen;
        next = nb + ((k & 1) ? clampd(e->patch.arp.swing, 0, 1) * stepLen * 0.33 : 0);
    }
}
inline void Arp::doStep(double stepLen) {
    const auto& A = e->patch.arp; const int len = std::max(1, std::min(16, A.len)), s = (int)(k % len);
    cur = s;
    const double vs = A.vel[s];
    if (!(vs > 0) || held.empty()) return;
    const int tr = A.tr[s]; const double gl = std::max(0.05, A.gate) * stepLen;
    ArpNote picks[128]; int np = 0;
    const int oct = std::max(1, std::min(4, A.oct));
    if (A.mode == 6) { for (const auto& h : held) picks[np++] = h; }
    else {
        ArpNote base[128]; int nb2 = 0; for (const auto& h : held) base[nb2++] = h;
        if (A.mode != 5) std::stable_sort(base, base + nb2, [](const ArpNote& x, const ArpNote& y) { return x.n < y.n; });
        const int L = nb2 * oct; int idx = 0;
        switch (A.mode) {
            case 1: idx = L - 1 - (int)(i % L); break;
            case 2: { const int per = std::max(1, 2 * L - 2), kk = (int)(i % per); idx = kk < L ? kk : per - kk; } break;
            case 3: { const int per = std::max(1, 2 * L - 2), kk = (int)(i % per); idx = L - 1 - (kk < L ? kk : per - kk); } break;
            case 4: idx = (int)(e->rng.next() * L); break;
            default: idx = (int)(i % L);
        }
        i++;
        idx = std::max(0, std::min(L - 1, idx));
        picks[np++] = { base[idx % nb2].n + 12 * (idx / nb2), base[idx % nb2].v };
    }
    for (int q = 0; q < np; q++) {
        const int note = std::max(0, std::min(127, picks[q].n + tr)); const double vel = clampd(picks[q].v * (0.35 + 0.65 * vs), 0.05, 1);
        for (int j = (int)play.size() - 1; j >= 0; j--) if (play[(size_t)j].n == note) play.erase(play.begin() + j);
        e->vOn(note, vel);
        if (play.size() < 150) play.push_back({ note, next + gl });
    }
}

// ------------------------------------------------------------------ master FX
inline double rdR(const float* B, int w, double d) { double rp = w - d; if (rp < 0) rp += 8192; const int i0 = (int)rp; const double f = rp - i0, a = B[i0]; return a + f * (B[(i0 + 1) & 8191] - a); }
inline void MasterFX::process(Engine& e, float* L, float* R, int n) {
    const auto& F = e.patch.fx; Rng& rng = e.rng; const float* SINE = statics().sine.data();
    static const double TUBE_B = ftanh(0.25);
    if (F.drv.on) {
        const double amt = clampd(F.drv.amt + mod[0], 0, 1);
        if (amt > 0.002) {
            const double pre = 1 + amt * amt * 24, post = 1 / std::sqrt(pre), mix = clampd(F.drv.mix, 0, 1); const int type = F.drv.type;
            const double lc = 1 - std::exp(-TAU * std::min(600 * std::pow(2.0, F.drv.tone * 5), sr * 0.45) / sr);
            for (int c = 0; c < 2; c++) {
                float* B = c ? R : L; double lp = dl[c];
                for (int i = 0; i < n; i++) {
                    const double x = B[i], u = x * pre; double y;
                    switch (type) {
                        case 1: y = ftanh(u + 0.25) - TUBE_B; break;
                        case 2: y = u > 1 ? 1 : (u < -1 ? -1 : u); break;
                        case 3: y = (u > 0 ? 1 - std::exp(-u * 1.5) : std::exp(u * 1.5) - 1) * 1.1; break;
                        case 4: y = std::sin(u * 1.5707963); break;
                        default: y = ftanh(u);
                    }
                    lp += lc * (y * post - lp);
                    B[i] = (float)(x + mix * (lp - x));
                }
                dl[c] = (lp < 1e-18 && lp > -1e-18) ? 0 : lp;
            }
        }
    }
    if (F.lofi.on) {
        const double mix = clampd(F.lofi.mix + mod[1], 0, 1);
        if (mix > 0.001) {
            const double bits = clampd(F.lofi.bits, 2, 16), q = bits >= 15.9 ? 0 : 2 / std::pow(2.0, bits), rate = clampd(F.lofi.rate, 1, 40);
            const double wow = clampd(F.lofi.wow, 0, 1), flut = clampd(F.lofi.flut, 0, 1), nz = clampd(F.lofi.noise, 0, 1);
            const double lc = 1 - std::exp(-TAU * std::min(400 * std::pow(2.0, F.lofi.tone * 5.5), sr * 0.45) / sr);
            float* B0 = lb[0].data(); float* B1 = lb[1].data(); float* D0 = ld[0].data(); float* D1 = ld[1].data();
            int w = lw; double hcv = hc, h0 = hold[0], h1 = hold[1], wp = wph, fp = fph, l0 = lt[0], l1 = lt[1], c0 = ck[0], c1 = ck[1];
            const double wi = 0.5 / sr, fi = 6.5 / sr, wd = wow * 0.0028 * sr, fd = flut * 0.00035 * sr; const int base = (int)std::floor(0.004 * sr + 0.5);
            for (int i = 0; i < n; i++) {
                const double xl = L[i], xr = R[i];
                D0[w] = (float)xl; D1[w] = (float)xr;
                hcv += 1; if (hcv >= rate) { hcv -= rate; h0 = xl; h1 = xr; }
                double yl = rate > 1.01 ? h0 : xl, yr = rate > 1.01 ? h1 : xr;
                if (q > 0) { yl = std::floor(yl / q + 0.5) * q; yr = std::floor(yr / q + 0.5) * q; }
                B0[w] = (float)yl; B1[w] = (float)yr;
                wp += wi * (1 + 0.25 * SINE[(int)(std::fmod(wp * 0.37, 1.0) * TBL)]); if (wp >= 1) wp -= 1;
                fp += fi; if (fp >= 1) fp -= 1;
                const double dd = base + wd * (1 + SINE[(int)(wp * TBL)]) + fd * (1 + SINE[(int)(fp * TBL)]);
                yl = rdR(B0, w, dd); yr = rdR(B1, w, dd);
                const int di = (w - base) & 8191; const double dxl = D0[di], dxr = D1[di];
                w = (w + 1) & 8191;
                if (nz > 0) {
                    const double hs = (rng.next() * 2 - 1) * nz * 0.012;
                    if (rng.next() < nz * 0.0005) { const double a = (rng.next() * 2 - 1) * nz * 0.6; c0 += a; c1 += a * (0.6 + rng.next() * 0.4); }
                    c0 *= 0.55; c1 *= 0.55; yl += hs + c0; yr += hs + c1;
                }
                l0 += lc * (yl - l0); l1 += lc * (yr - l1);
                L[i] = (float)(dxl + mix * (l0 - dxl)); R[i] = (float)(dxr + mix * (l1 - dxr));
            }
            lw = w; hc = hcv; hold[0] = h0; hold[1] = h1; wph = wp; fph = fp;
            fl0(l0); fl0(l1); fl0(c0); fl0(c1); lt[0] = l0; lt[1] = l1; ck[0] = c0; ck[1] = c1;
        }
    }
    if (F.gate.on) {
        const double depth = clampd(F.gate.depth + mod[4], 0, 1);
        const double stepLen = 60 / e.bpm * F.gate.div * sr;
        const double coef = 1 - std::exp(-1 / (sr * (0.0008 + clampd(F.gate.smooth, 0, 1) * 0.04)));
        const double t0 = e.time - e.gateStart; double g = gv;
        for (int i = 0; i < n; i++) {
            const int st = (int)((long long)std::floor((t0 + i) / stepLen) & 15);
            g += ((F.gate.pat[st] ? 1 : 1 - depth) - g) * coef; L[i] = (float)(L[i] * g); R[i] = (float)(R[i] * g);
        }
        gv = g;
    }
    if (F.cho.on) {
        const double mix = clampd(F.cho.mix + mod[2], 0, 1);
        if (mix > 0.001) {
            const int mode = F.cho.mode; float* B0 = cb[0].data(); float* B1 = cb[1].data();
            int w = cw; double ph = cph, ph2 = cph2, fl = cfl[0], fr = cfl[1];
            const double rate = clampd(F.cho.rate, 0.02, 10), dep = clampd(F.cho.depth, 0, 1), fb = mode == 2 ? clampd(F.cho.fb, -0.95, 0.95) : 0;
            const double pi = rate / sr, pi2 = 5.7 / sr; double base, depS;
            if (mode == 0) { base = 0.0065 * sr; depS = dep * 0.0045 * sr; }
            else if (mode == 1) { base = 0.009 * sr; depS = dep * 0.0032 * sr; }
            else { base = 0.0006 * sr; depS = dep * 0.0045 * sr; }
            const double vib = 0.00022 * sr;
            auto S = [SINE](double p) { return (double)SINE[(int)(std::fmod(p, 1.0) * TBL)]; };
            for (int i = 0; i < n; i++) {
                const double xl = L[i], xr = R[i];
                B0[w] = (float)(xl + fb * fl); B1[w] = (float)(xr + fb * fr);
                ph += pi; if (ph >= 1) ph -= 1; ph2 += pi2; if (ph2 >= 1) ph2 -= 1;
                double wl, wr;
                if (mode == 1) {
                    const double d1 = base + depS * S(ph) + vib * S(ph2), d2 = base + depS * S(ph + 0.3333) + vib * S(ph2 + 0.3333), d3 = base + depS * S(ph + 0.6667) + vib * S(ph2 + 0.6667);
                    wl = (rdR(B0, w, d1) + rdR(B0, w, d2) * 0.7 + rdR(B1, w, d3) * 0.5) * 0.6;
                    wr = (rdR(B1, w, d2) + rdR(B1, w, d3) * 0.7 + rdR(B0, w, d1) * 0.5) * 0.6;
                } else {
                    const double dl2 = base + depS * (0.5 + 0.5 * S(ph)), dr2 = base + depS * (0.5 + 0.5 * S(ph + 0.25));
                    wl = rdR(B0, w, dl2); wr = rdR(B1, w, dr2);
                }
                fl = wl; fr = wr;
                w = (w + 1) & 8191;
                L[i] = (float)(xl + mix * (wl - xl)); R[i] = (float)(xr + mix * (wr - xr));
            }
            cw = w; cph = ph; cph2 = ph2; fl0(fl); fl0(fr); cfl[0] = fl; cfl[1] = fr;
        }
    }
    if (F.pha.on) {
        const double mix = clampd(F.pha.mix + mod[3], 0, 1);
        if (mix > 0.001) {
            const double rate = clampd(F.pha.rate, 0.01, 10), dep = clampd(F.pha.depth, 0, 1), fb = clampd(F.pha.fb, -0.95, 0.95);
            double ph = pph;
            for (int o = 0; o < n; o += CB) {
                const int m = std::min(CB, n - o);
                ph += rate * m / sr; if (ph >= 1) ph -= 1;
                for (int c = 0; c < 2; c++) {
                    const double lfv = 0.5 + 0.5 * SINE[(int)(std::fmod(ph + c * 0.25, 1.0) * TBL)];
                    const double fc = std::min(160 * std::pow(2.0, dep * 5.6 * lfv), sr * 0.4), t = std::tan(PI * fc / sr), a = (t - 1) / (t + 1);
                    float* B = c ? R : L; double* s = pst[c]; double last = plast[c];
                    for (int i = o; i < o + m; i++) {
                        double x = B[i] + fb * last;
                        for (int k = 0; k < 6; k++) { const double y = a * (x - s[2 * k + 1]) + s[2 * k]; s[2 * k] = x; s[2 * k + 1] = y; x = y; }
                        last = x;
                        B[i] = (float)(B[i] * (1 - 0.5 * mix) + x * 0.5 * mix);
                    }
                    fl0(last); plast[c] = last;
                    for (int k = 0; k < 12; k++) fl0(s[k]);
                }
            }
            pph = ph;
        }
    }
}

// ------------------------------------------------------------------ engine
inline int Engine::snap(int n) const {
    const auto& deg = scales()[(size_t)std::max(0, std::min((int)scales().size() - 1, patch.scale.type))].deg;
    const int rel = (((n - patch.scale.root) % 12) + 12) % 12; int best = 0;
    for (int d : deg) if (d <= rel) best = d;
    return n - (rel - best);
}
inline int Engine::chordOf(int base, int* out) const {
    const auto& def = chords()[(size_t)std::max(0, std::min((int)chords().size() - 1, patch.chord.type))];
    int iv[8]; int n = 0;
    if (def.diatonic) {
        const auto& deg = scales()[(size_t)std::max(0, std::min((int)scales().size() - 1, patch.scale.type))].deg;
        const int bb = snap(base), rel = (((bb - patch.scale.root) % 12) + 12) % 12; int idx = 0;
        for (size_t j = 0; j < deg.size(); j++) if (deg[j] == rel) idx = (int)j;
        const int Ln = (int)deg.size();
        for (int j = 0; j < def.diatonic; j++) { const int k = idx + j * 2, o = k / Ln; iv[n++] = deg[(size_t)(k % Ln)] + 12 * o - deg[(size_t)idx]; }
        base = bb;
    } else for (int x : def.iv) iv[n++] = x;
    if (patch.chord.spread >= 1 && n >= 3) iv[1] += 12;
    if (patch.chord.spread >= 2 && n >= 4) iv[3] += 12;
    for (int j = 0; j < n; j++) out[j] = std::max(0, std::min(127, base + iv[j]));
    return n;
}
inline void Engine::noteIn(int n, double v) {
    if (n < 0 || n > 127) return;
    if (inMap[n].count >= 0) noteOut(n);
    const int base = patch.scale.on ? snap(n) : n;
    InRec& rec = inMap[n];
    if (patch.chord.on) rec.count = chordOf(base, rec.notes); else { rec.count = 1; rec.notes[0] = base; }
    rec.arp = patch.arp.on != 0;
    bool anyActive = false; for (const auto& x : voices) if (x.active && x.gate) { anyActive = true; break; }
    if (!anyActive && !arp.running) gateStart = time;
    for (int j = 0; j < rec.count; j++) {
        const int x = rec.notes[j];
        if (rec.arp) arp.on(x, v);
        else { cnt[x]++; susSet[x] = false; vOn(x, v); }
    }
}
inline void Engine::noteOut(int n) {
    if (n < 0 || n > 127) return;
    InRec& rec = inMap[n]; if (rec.count < 0) return;
    const int c = rec.count; rec.count = -1;
    for (int j = 0; j < c; j++) {
        const int x = rec.notes[j];
        if (rec.arp) arp.off(x);
        else { cnt[x] = std::max(0, cnt[x] - 1); if (cnt[x] == 0) { if (sus) susSet[x] = true; else vOff(x); } }
    }
}
inline void Engine::vOn(int n, double v) {
    const auto& g = patch.g;
    if (g.vmode > 0) {
        auto it = std::find(mono.begin(), mono.end(), n); if (it != mono.end()) mono.erase(it);
        if (mono.size() < 128) mono.push_back(n);
        Voice& vc = voices[0];
        for (int k = 1; k < POOL; k++) if (voices[k].active && !voices[k].fading) voices[k].startFade(STEAL_FADE);
        if (vc.active && vc.gate && !vc.fading && g.vmode == 2) vc.legatoTo(n);
        else vc.start(n, v, vc.active && vc.gate);
        monoVel = v;
        return;
    }
    Voice* vc = nullptr;
    for (auto& x : voices) if (x.active && !x.fading && x.note == n) { vc = &x; break; }
    if (!vc) {
        // Over the polyphony limit, the quietest released voice (else the oldest held one) fades out over STEAL_FADE.
        // The new note takes a free slot; only if the whole pool is busy does it take the voice furthest into its fade.
        int count = 0; Voice* free = nullptr; Voice* quietR = nullptr; Voice* old = nullptr; Voice* faded = nullptr; double qLvl = 0;
        for (auto& x : voices) {
            if (!x.active) { if (!free) free = &x; continue; }
            if (x.fading) { if (!faded || x.fadeG < faded->fadeG) faded = &x; continue; }
            count++;
            if (!x.gate) { const double l = x.level(); if (!quietR || l < qLvl) { quietR = &x; qLvl = l; } }
            if (!old || x.age > old->age) old = &x;
        }
        const int poly = std::max(1, std::min(16, g.poly));
        Voice* victim = nullptr;
        if (count >= poly) { victim = quietR ? quietR : old; if (victim) victim->startFade(STEAL_FADE); }
        vc = free ? free : (faded ? faded : victim ? victim : (quietR ? quietR : old));
        if (vc && vc->active) vc->kill();
    }
    if (vc) vc->start(n, v, false);
}
inline void Engine::vOff(int n) {
    const auto& g = patch.g;
    if (g.vmode > 0) {
        auto it = std::find(mono.begin(), mono.end(), n); if (it != mono.end()) mono.erase(it);
        Voice& vc = voices[0];
        if (!vc.active || vc.fading || vc.note != n) return;
        if (!mono.empty()) { const int nn = mono.back(); if (g.vmode == 2) vc.legatoTo(nn); else vc.start(nn, monoVel > 0 ? monoVel : 0.8, true); }
        else vc.release();
        return;
    }
    for (auto& x : voices) if (x.active && x.gate && x.note == n) x.release();
}
inline void Engine::tickGlobal(int n) {
    gtime = time / sr; bps = bpm / 60;
    for (int k = 0; k < 4; k++) mac[k] += (patch.mac[k] - mac[k]) * 0.12;
    mw += (mwT - mw) * 0.15; at += (atT - at) * 0.15; pb += (pbT - pb) * 0.3;
    bendSemis = pb * patch.g.bend;
    const auto& o = patch.orb;
    orbT += n / sr * (0.04 + o.sp * 1.1);
    const double t = orbT, chs = o.ch;
    const double cx = chs * (0.55 * std::sin(t * 1.3 + std::sin(t * 0.37) * 2) + 0.35 * std::sin(t * 2.9 + 1.1));
    const double cy = chs * (0.55 * std::cos(t * 0.91 + std::sin(t * 0.53) * 1.7) + 0.35 * std::sin(t * 2.3 + 0.4));
    ox += (clampd(o.x + cx, -1, 1) - ox) * 0.2; oy += (clampd(o.y + cy, -1, 1) - oy) * 0.2;
    if (patch.arp.on || !arp.play.empty()) arp.tick(time, n);
}
inline void Engine::process(float* outL, float* outR, int n) {
    if ((int)mixL.size() < n) { mixL.resize((size_t)n); mixR.resize((size_t)n); }
    float* mL = mixL.data(); float* mR = mixR.data();
    for (int i = 0; i < n; i++) { mL[i] = 0; mR[i] = 0; }
    for (int off = 0; off < n; off += CB) {
        const int len = std::min(CB, n - off);
        tickGlobal(len);
        for (auto& vc : voices) if (vc.active) vc.render(off, len);
        time += len;
    }
    // effect modulation from global sources
    double gs[NSRC] = { 0 }; gs[9] = mw; gs[10] = at; gs[12] = mac[0]; gs[13] = mac[1]; gs[14] = mac[2]; gs[15] = mac[3]; gs[16] = ox; gs[17] = oy; gs[19] = pb;
    std::fill(fxm, fxm + NFXD, 0.0);
    if (fxModEnabled) for (int r = 0; r < patch.nmods; r++) { const ModRoute& m = patch.mods[r]; if (m.tgt == 6) fxm[m.pi] += gs[m.s] * m.a * (m.v ? gs[m.v] : 1); }
    for (int k = 0; k < 5; k++) fx.mod[k] = fxm[k];
    const double gain = clampd(patch.g.vol, 0, 2) * clampd(patch.g.trim, 0.05, 8);
    for (int i = 0; i < n; i++) { mL[i] = (float)(mL[i] * gain); mR[i] = (float)(mR[i] * gain); }
    fx.process(*this, mL, mR, n);
    for (int i = 0; i < n; i++) { float a = mL[i], b = mR[i]; if (!(a == a)) a = 0; if (!(b == b)) b = 0; outL[i] = a; outR[i] = b; }
}

} // namespace aether
