// Shared DSP for the LGN mixing suite. Plain C++ (no JUCE) so every algorithm can be tested on its own.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace lgn {

constexpr double kPi = 3.14159265358979323846;
inline double dbToGain(double db) { return std::pow(10.0, db / 20.0); }
inline double gainToDb(double g) { return 20.0 * std::log10(std::max(g, 1e-12)); }
template <typename T> inline T clampv(T x, T lo, T hi) { return std::min(hi, std::max(lo, x)); }

// One-pole smoother toward a target, time constant in seconds.
struct Smooth {
    double y = 0, a = 0;
    void setTime(double sr, double secs) { a = secs <= 0 ? 0 : std::exp(-1.0 / (secs * sr)); }
    void reset(double v) { y = v; }
    double next(double target) { y = target + (y - target) * a; return y; }
};

// Andrew Simper's trapezoidal state-variable filter: stable under fast modulation, and its responses are the
// bilinear transforms of the analog prototypes (the same curves as the RBJ cookbook), so the display can be exact.
struct Svf {
    enum Type { lowpass, highpass, bandpass, bell, lowShelf, highShelf, notch, allpass };
    double ic1 = 0, ic2 = 0;
    double a1 = 1, a2 = 0, a3 = 0, m0 = 1, m1 = 0, m2 = 0;
    void reset() { ic1 = ic2 = 0; }
    void set(Type t, double sr, double freq, double q, double gainDb = 0) {
        freq = clampv(freq, 5.0, sr * 0.4999);
        q = std::max(q, 0.025);
        const double A = std::pow(10.0, gainDb / 40.0);
        double g = std::tan(kPi * freq / sr), k = 1.0 / q;
        switch (t) {
            case lowpass:  m0 = 0; m1 = 0; m2 = 1; break;
            case highpass: m0 = 1; m1 = -k; m2 = -1; break;
            case bandpass: m0 = 0; m1 = k; m2 = 0; break;     // unity gain at the centre
            case notch:    m0 = 1; m1 = -k; m2 = 0; break;
            case allpass:  m0 = 1; m1 = -2 * k; m2 = 0; break;
            case bell:     k = 1.0 / (q * A); m0 = 1; m1 = k * (A * A - 1); m2 = 0; break;
            case lowShelf: g /= std::sqrt(A); m0 = 1; m1 = k * (A - 1); m2 = A * A - 1; break;
            case highShelf: g *= std::sqrt(A); m0 = A * A; m1 = k * (1 - A) * A; m2 = 1 - A * A; break;
        }
        a1 = 1.0 / (1.0 + g * (g + k)); a2 = g * a1; a3 = g * a2;
        gg = g; kk = k;
    }
    // Core step returning the band-pass (v1) and low-pass (v2) states, for callers that mix their own outputs.
    void tick(double v0, double& v1o, double& v2o) {
        const double v3 = v0 - ic2;
        const double v1 = a1 * ic1 + a2 * v3;
        const double v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        v1o = v1; v2o = v2;
    }
    double process(double v0) {
        const double v3 = v0 - ic2;
        const double v1 = a1 * ic1 + a2 * v3;
        const double v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        return m0 * v0 + m1 * v1 + m2 * v2;
    }
    // Exact magnitude of this filter at frequency f (the filter is the bilinear transform of its analog prototype).
    double magnitude(double f, double sr) const {
        const double w = std::tan(kPi * clampv(f, 1.0, sr * 0.4999) / sr) / gg;   // analog frequency normalised to the cutoff
        const std::complex<double> s(0, w), den = s * s + kk * s + 1.0;
        const std::complex<double> h = m0 + (m1 * s + m2) / den;
        return std::abs(h);
    }
    double gg = 1, kk = 1.41421356;
};

// Butterworth Q values for cascaded 2nd-order sections: 12 dB/oct = 1 section, 24 = 2, 48 = 4.
inline double butterQ(int sections, int i) { return 1.0 / (2.0 * std::cos(kPi * (2.0 * i + 1.0) / (4.0 * sections))); }

// Envelope follower with separate attack and release, in the linear domain.
struct Follower {
    double env = 0, att = 0, rel = 0;
    void set(double sr, double attSecs, double relSecs) { att = std::exp(-1.0 / (std::max(attSecs, 1e-6) * sr)); rel = std::exp(-1.0 / (std::max(relSecs, 1e-6) * sr)); }
    double process(double x) { const double c = x > env ? att : rel; env = x + (env - x) * c; return env; }
};

// Delay line with 4-point Hermite interpolation; length rounded up to a power of two.
struct DelayLine {
    std::vector<float> buf; int mask = 0, w = 0;
    void prepare(int maxSamples) { int n = 1; while (n < maxSamples + 4) n <<= 1; buf.assign((size_t)n, 0.0f); mask = n - 1; w = 0; }
    void reset() { std::fill(buf.begin(), buf.end(), 0.0f); w = 0; }
    void push(float x) { buf[(size_t)w] = x; w = (w + 1) & mask; }
    float tap(int d) const { return buf[(size_t)((w - 1 - d) & mask)]; }   // d = 0: newest sample
    float read(double d) const {                                         // fractional delay, d >= 1
        const int i = (int)d; const float t = (float)(d - i);
        const float y0 = tap(i - 1), y1 = tap(i), y2 = tap(i + 1), y3 = tap(i + 2);
        const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }
};

// In-place radix-2 complex FFT (size a power of two). Small and dependency-free; fast enough for the suite's sizes.
class Fft {
public:
    explicit Fft(int order = 11) { init(order); }
    void init(int order) {
        n = 1 << order; rev.resize((size_t)n); tw.resize((size_t)n / 2);
        for (int i = 0; i < n; i++) { int r = 0; for (int b = 0; b < order; b++) if (i & (1 << b)) r |= 1 << (order - 1 - b); rev[(size_t)i] = r; }
        for (int i = 0; i < n / 2; i++) tw[(size_t)i] = std::polar(1.0, -2 * kPi * i / n);
    }
    int size() const { return n; }
    void transform(std::complex<double>* x, bool inverse) const {
        for (int i = 0; i < n; i++) if (i < rev[(size_t)i]) std::swap(x[i], x[rev[(size_t)i]]);
        for (int len = 2; len <= n; len <<= 1) {
            const int half = len / 2, step = n / len;
            for (int i = 0; i < n; i += len) {
                { auto& u = x[i], & v = x[i + half]; const auto t = v; v = u - t; u = u + t; }   // j = 0: twiddle is exactly 1
                for (int j = 1; j < half; j++) {
                    // complex multiply written out: std::complex's operator* goes through a slow library call
                    // (__muldc3, which re-checks for infinities); for finite numbers this is the same arithmetic
                    const auto w = tw[(size_t)(j * step)];
                    const double wr = w.real(), wi = inverse ? -w.imag() : w.imag();
                    const auto b = x[i + j + half];
                    const double vr = b.real() * wr - b.imag() * wi, vi = b.real() * wi + b.imag() * wr;
                    const auto u = x[i + j];
                    x[i + j] = { u.real() + vr, u.imag() + vi }; x[i + j + half] = { u.real() - vr, u.imag() - vi };
                }
            }
        }
        if (inverse) for (int i = 0; i < n; i++) x[i] /= (double)n;
    }
private:
    int n = 0; std::vector<int> rev; std::vector<std::complex<double>> tw;
};

// Note names and scales, shared by Gravity and anything else that needs a key.
inline const char* const* noteNames() { static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return n; }
struct Scale { const char* name; int mask; };   // bit i set: semitone i above the root is in the scale
inline const std::vector<Scale>& scales() {
    static const std::vector<Scale> s = {
        { "Major", 0b101010110101 }, { "Minor", 0b010110101101 }, { "Harmonic Minor", 0b100110101101 },
        { "Dorian", 0b011010101101 }, { "Phrygian", 0b010110101011 }, { "Pentatonic Major", 0b001010010101 },
        { "Pentatonic Minor", 0b010010101001 }, { "Blues", 0b010011101001 }, { "Chromatic", 0b111111111111 },
        { "Root + Fifth", 0b000010000001 }, { "Octaves", 0b000000000001 } };
    return s;
}
} // namespace lgn
