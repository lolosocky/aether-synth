// DSP tests for the LGN mixing suite (no JUCE). Each test prints one line; the program fails if any check fails.
#include "Eq.h"
#include <cstdio>
#include <functional>
#include <random>
#include <string>

using namespace lgn;
static int fails = 0;
static void expect(bool ok, const std::string& what) { std::printf("  %s %s\n", ok ? "ok  " : "FAIL", what.c_str()); if (!ok) fails++; }

// Steady-state gain of a processor at frequency f, in dB (stereo buffers, measured on the left channel).
static double sineGainDb(const std::function<void(float* const*, int)>& proc, double f, double sr, double seconds = 1.0) {
    const int n = (int)(sr * seconds), blk = 256; std::vector<float> L((size_t)n), R((size_t)n);
    for (int i = 0; i < n; i++) L[(size_t)i] = R[(size_t)i] = (float)(0.25 * std::sin(2 * kPi * f * i / sr));
    for (int o = 0; o < n; o += blk) { float* ch[2] = { L.data() + o, R.data() + o }; proc(ch, std::min(blk, n - o)); }
    double si = 0, so = 0; for (int i = n / 2; i < n; i++) { const double x = 0.25 * std::sin(2 * kPi * f * i / sr); si += x * x; so += (double)L[(size_t)i] * L[(size_t)i]; }
    return 10 * std::log10(so / si);
}

static void testEq() {
    std::printf("EQ\n");
    const double sr = 48000;
    {   // flat settings are an exact pass-through
        EqDsp eq; for (int b = 0; b < EqDsp::kBands; b++) { EqBandParams p; p.on = !EqDsp::isCut(b); p.freq = 1000; eq.setBand(b, p); }
        eq.prepare(sr, 2);
        std::vector<float> L(4096), R(4096); std::mt19937 g(1); std::uniform_real_distribution<float> u(-1, 1);
        for (int i = 0; i < 4096; i++) L[(size_t)i] = R[(size_t)i] = u(g);
        auto L0 = L; float* ch[2] = { L.data(), R.data() }; eq.process(ch, 2, 4096);
        double md = 0; for (int i = 0; i < 4096; i++) md = std::max(md, (double)std::abs(L[(size_t)i] - L0[(size_t)i]));
        expect(md < 1e-6, "flat EQ is transparent (max diff " + std::to_string(md) + ")");
    }
    // each band type: measured sine gain matches the display's magnitude
    struct Case { int band; EqBandParams p; };
    std::vector<Case> cases = {
        { EqDsp::bell1, { true, 1000, 6, 1.0, 1 } }, { EqDsp::bell2, { true, 3000, -9, 4.0, 1 } },
        { EqDsp::lowShelf, { true, 120, 5, 0.707, 1 } }, { EqDsp::highShelf, { true, 6000, -4, 0.9, 1 } },
        { EqDsp::lowCut, { true, 100, 0, 0.707, 0 } }, { EqDsp::lowCut, { true, 100, 0, 0.707, 2 } }, { EqDsp::highCut, { true, 8000, 0, 0.707, 1 } } };
    double worst = 0;
    for (auto& c : cases) {
        for (double f : { 40.0, 100.0, 300.0, 1000.0, 2500.0, 3000.0, 6000.0, 12000.0 }) {
            EqDsp eq; for (int b = 0; b < EqDsp::kBands; b++) { EqBandParams off; off.on = false; eq.setBand(b, off); }
            eq.setBand(c.band, c.p); eq.prepare(sr, 2);
            const double meas = sineGainDb([&](float* const* ch, int n) { eq.process(ch, 2, n); }, f, sr, 0.5);
            const double want = gainToDb(EqDsp::bandMagnitude(c.band, c.p, f, sr));
            if (want > -60) worst = std::max(worst, std::abs(meas - want));
        }
    }
    expect(worst < 0.05, "measured response matches the displayed curve (worst " + std::to_string(worst) + " dB)");
    {
        EqBandParams p{ true, 1000, 6, 1.0, 1 };
        expect(std::abs(gainToDb(EqDsp::bandMagnitude(EqDsp::bell1, p, 1000, sr)) - 6) < 0.01, "bell peak is +6 dB at its centre");
        EqBandParams cut{ true, 100, 0, 0.707, 1 };
        expect(std::abs(gainToDb(EqDsp::bandMagnitude(EqDsp::lowCut, cut, 100, sr)) + 3.01) < 0.05, "24 dB low cut is -3 dB at its corner");
        expect(gainToDb(EqDsp::bandMagnitude(EqDsp::lowCut, cut, 25, sr)) < -47, "24 dB low cut is down 48 dB two octaves below");
    }
    {   // a fast sweep must not click: compare against the smooth output's local behaviour
        EqDsp eq; for (int b = 0; b < EqDsp::kBands; b++) { EqBandParams off; off.on = false; eq.setBand(b, off); }
        eq.prepare(sr, 2);
        const int n = 48000; std::vector<float> L((size_t)n), R((size_t)n);
        for (int i = 0; i < n; i++) L[(size_t)i] = R[(size_t)i] = (float)(0.3 * std::sin(2 * kPi * 220 * i / sr));
        double jump = 0; float prev = 0;
        for (int o = 0; o < n; o += 64) {
            EqBandParams p{ (o / 4800) % 2 == 0, 200 + 4000.0 * o / n, (o / 2400) % 2 ? 12.0 : -12.0, 2.0, 1 };   // toggles and jumps
            eq.setBand(EqDsp::bell1, p);
            float* ch[2] = { L.data() + o, R.data() + o }; eq.process(ch, 2, std::min(64, n - o));
            for (int i = o; i < std::min(n, o + 64); i++) { jump = std::max(jump, (double)std::abs(L[(size_t)i] - prev)); prev = L[(size_t)i]; }
        }
        expect(jump < 0.12, "parameter jumps glide without clicks (largest step " + std::to_string(jump) + ")");
    }
}

int main() {
    testEq();
    std::printf(fails ? "FAILED (%d)\n" : "ALL PASS\n", fails);
    return fails ? 1 : 0;
}
