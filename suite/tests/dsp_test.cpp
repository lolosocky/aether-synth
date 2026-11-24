// DSP tests for the LGN mixing suite (no JUCE). Each test prints one line; the program fails if any check fails.
#include "Eq.h"
#include "Comp.h"
#include "DeEss.h"
#include "Saturate.h"
#include "Reverb.h"
#include "Delay.h"
#include "Limit.h"
#include "Gravity.h"
#include "Duck.h"
#include "Guitar.h"
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

// Level (RMS dB) of the second half of a processed sine.
static double toneOut(const std::function<void(float* const*, int)>& proc, double f, double amp, double sr, double seconds, double* inDb = nullptr) {
    const int n = (int)(sr * seconds), blk = 256; std::vector<float> L((size_t)n), R((size_t)n);
    for (int i = 0; i < n; i++) L[(size_t)i] = R[(size_t)i] = (float)(amp * std::sin(2 * kPi * f * i / sr));
    for (int o = 0; o < n; o += blk) { float* ch[2] = { L.data() + o, R.data() + o }; proc(ch, std::min(blk, n - o)); }
    double so = 0; for (int i = n / 2; i < n; i++) so += (double)L[(size_t)i] * L[(size_t)i];
    if (inDb) *inDb = gainToDb(amp / std::sqrt(2.0));
    return 10 * std::log10(so / (n - n / 2) + 1e-30);
}

static void testComp() {
    std::printf("Compressor\n");
    const double sr = 48000;
    expect(std::abs(CompDsp::curve(-30, -18, 4, 0) + 30) < 1e-9, "below threshold is untouched");
    expect(std::abs(CompDsp::curve(-6, -18, 4, 0) - (-15)) < 1e-9, "4:1 above threshold: -6 dB in gives -15 dB out");
    expect(std::abs(CompDsp::curve(-18, -18, 4, 6) - (-18 + (0.25 - 1) * 9.0 / 12.0)) < 1e-9, "soft knee is continuous at threshold");
    for (int style = 0; style < 3; style++) {
        CompDsp c; CompParams p; p.thresh = -20; p.ratio = 4; p.kneeDb = 0; p.attackMs = 5; p.releaseMs = 100; p.style = style; c.set(p); c.prepare(sr);
        double in; const double out = toneOut([&](float* const* ch, int n) { c.process(ch, 2, n); }, 1000, 0.5, sr, 1.5, &in);
        // a sine's peak is 3 dB above its RMS; the peak styles see the peak, Glue sees RMS x sqrt2 (= peak for a sine)
        const double peakIn = gainToDb(0.5), want = CompDsp::curve(peakIn, -20, 4, 0) - peakIn;
        const double got = out - in;
        if (style == CompDsp::punch) expect(got < -3 && got > want - 3, "punch (feedback) compresses a loud tone: " + std::to_string(got) + " dB");
        else expect(std::abs(got - want) < 0.8, std::string(style == 0 ? "clean" : "glue") + " gain matches the static curve (" + std::to_string(got) + " vs " + std::to_string(want) + " dB)");
    }
    {   // mix 0 is a perfect bypass
        CompDsp c; CompParams p; p.thresh = -40; p.ratio = 20; p.mix = 0; c.set(p); c.prepare(sr);
        double in; const double out = toneOut([&](float* const* ch, int n) { c.process(ch, 2, n); }, 440, 0.5, sr, 0.5, &in);
        expect(std::abs(out - in) < 0.01, "mix at 0% passes the dry signal");
    }
    {   // release recovers: after a loud burst, quiet signal returns to unity
        CompDsp c; CompParams p; p.thresh = -20; p.ratio = 8; p.releaseMs = 50; c.set(p); c.prepare(sr);
        const int n = 48000; std::vector<float> L((size_t)n), R((size_t)n);
        for (int i = 0; i < n; i++) { const double a = i < 12000 ? 0.8 : 0.02; L[(size_t)i] = R[(size_t)i] = (float)(a * std::sin(2 * kPi * 300 * i / sr)); }
        for (int o = 0; o < n; o += 256) { float* ch[2] = { L.data() + o, R.data() + o }; c.process(ch, 2, std::min(256, n - o)); }
        expect(c.reductionDb() < 0.1, "gain reduction releases after the burst (" + std::to_string(c.reductionDb()) + " dB left)");
    }
}

static void testDeEss() {
    std::printf("De-Esser\n");
    const double sr = 48000;
    auto run = [&](double f, double amp, int mode) {
        DeEssDsp d; DeEssParams p; p.freq = 5000; p.thresh = -30; p.rangeDb = 10; p.mode = mode; d.set(p); d.prepare(sr);
        double in; const double out = toneOut([&](float* const* ch, int n) { d.process(ch, 2, n); }, f, amp, sr, 1.0, &in);
        return out - in;
    };
    expect(std::abs(run(500, 0.3, DeEssDsp::split)) < 0.3, "split mode leaves the body of the voice alone (500 Hz)");
    const double s = run(8000, 0.3, DeEssDsp::split);
    expect(s < -6 && s > -10.5, "split mode turns loud sibilance down, within range (" + std::to_string(s) + " dB)");
    expect(std::abs(run(8000, 0.005, DeEssDsp::split)) < 0.3, "quiet sibilance under the threshold is untouched");
    {   // split with no reduction reconstructs the input exactly
        DeEssDsp d; DeEssParams p; p.thresh = 0; d.set(p); d.prepare(sr);
        std::vector<float> L(4096), R(4096); std::mt19937 g(5); std::uniform_real_distribution<float> u(-0.2f, 0.2f);
        for (int i = 0; i < 4096; i++) L[(size_t)i] = R[(size_t)i] = u(g);
        auto L0 = L; float* ch[2] = { L.data(), R.data() }; d.process(ch, 2, 4096);
        double md = 0; for (int i = 0; i < 4096; i++) md = std::max(md, (double)std::abs(L[(size_t)i] - L0[(size_t)i]));
        expect(md < 1e-6, "split mode is transparent when nothing crosses the threshold");
    }
}

static void testSat() {
    std::printf("Saturator\n");
    // harmonic signature of each mode: drive a sine and look at the 2nd and 3rd harmonics
    const double sr = 48000, f0 = 1000;
    for (int m = 0; m < SatDsp::numModes; m++) {
        const int n = 4800; std::vector<double> y((size_t)n);
        for (int i = 0; i < n; i++) y[(size_t)i] = SatDsp::shape(m, 2.0 * 0.5 * std::sin(2 * kPi * f0 * i / sr));
        double mean = 0; for (double v : y) mean += v; mean /= n;
        auto harmonic = [&](int k) { double re = 0, im = 0; for (int i = 0; i < n; i++) { re += (y[(size_t)i] - mean) * std::cos(2 * kPi * k * f0 * i / sr); im += (y[(size_t)i] - mean) * std::sin(2 * kPi * k * f0 * i / sr); } return std::sqrt(re * re + im * im) * 2 / n; };
        const double h1 = harmonic(1), h2 = gainToDb(harmonic(2) / h1), h3 = gainToDb(harmonic(3) / h1);
        const char* names[] = { "tape", "tube", "transistor", "fuzz" };
        bool ok = std::isfinite(h2) && std::isfinite(h3);
        if (m == SatDsp::tape) ok = ok && h2 < -60 && h3 > -40;      // odd only
        if (m == SatDsp::tube || m == SatDsp::fuzz) ok = ok && h2 > -40;   // strong even
        expect(ok, std::string(names[m]) + ": 2nd " + std::to_string((int)h2) + " dB, 3rd " + std::to_string((int)h3) + " dB");
    }
    {   // auto gain keeps loudness steady as drive rises
        SatDsp s; s.buildAutoGain(); double worst = 0;
        for (int m = 0; m < SatDsp::numModes; m++) for (double d : { 0.0, 12.0, 24.0, 36.0 }) {
            double si = 0, so = 0;
            for (int i = 0; i < 4800; i++) { const double x = 0.125 * std::sin(2 * kPi * i / 48.0); const double yv = SatDsp::shape(m, x * dbToGain(d)) * s.autoGain(m, d); si += x * x; so += yv * yv; }
            worst = std::max(worst, std::abs(10 * std::log10(so / si)));
        }
        expect(worst < 1.0, "auto gain holds level within 1 dB across modes and drive (" + std::to_string(worst) + ")");
    }
}

static void testVerb() {
    std::printf("Reverb\n");
    const double sr = 48000;
    for (int c = 0; c < 4; c++) {
        VerbDsp v; VerbParams p; p.character = c; p.decayS = 2.0; p.mix = 1.0; p.lowCutHz = 20; p.dampHz = 18000; p.preMs = 0; v.set(p); v.prepare(sr);
        const int n = (int)(sr * 5); std::vector<float> L((size_t)n, 0.0f), R((size_t)n, 0.0f); L[100] = R[100] = 1.0f;
        for (int o = 0; o < n; o += 256) { float* ch[2] = { L.data() + o, R.data() + o }; v.process(ch, 2, std::min(256, n - o)); }
        // energy decay: compare 50 ms windows 0.5 s and 1.5 s after the impulse -> should fall ~30 dB per second for RT60 = 2 s
        auto e = [&](double t) { double s2 = 0; const int a = (int)(t * sr); for (int i = a; i < a + 2400; i++) s2 += (double)L[(size_t)i] * L[(size_t)i] + (double)R[(size_t)i] * R[(size_t)i]; return 10 * std::log10(s2 + 1e-30); };
        const double slope = e(0.5) - e(1.5);
        bool finite = true; for (float x : L) finite = finite && std::isfinite(x);
        const char* names[] = { "room", "plate", "hall", "space" };
        expect(finite && slope > 22 && slope < 38, std::string(names[c]) + ": decays " + std::to_string(slope) + " dB per second (RT60 2 s wants 30)");
    }
    {   // freeze holds the tail
        VerbDsp v; VerbParams p; p.mix = 1; p.decayS = 1; v.set(p); v.prepare(sr);
        const int n = (int)(sr * 3); std::vector<float> L((size_t)n), R((size_t)n);
        std::mt19937 g(2); std::uniform_real_distribution<float> u(-0.3f, 0.3f);
        for (int i = 0; i < 24000; i++) L[(size_t)i] = R[(size_t)i] = u(g);
        for (int o = 0; o < n; o += 256) { if (o >= 24000 && !p.freeze) { p.freeze = true; v.set(p); } float* ch[2] = { L.data() + o, R.data() + o }; v.process(ch, 2, std::min(256, n - o)); }
        auto e = [&](double t) { double s2 = 0; const int a = (int)(t * sr); for (int i = a; i < a + 4800; i++) s2 += (double)L[(size_t)i] * L[(size_t)i]; return 10 * std::log10(s2 + 1e-30); };
        expect(std::abs(e(1.5) - e(2.8)) < 3, "freeze holds the tail steady (" + std::to_string(e(1.5) - e(2.8)) + " dB change)");
    }
}

static void testDelay() {
    std::printf("Delay\n");
    const double sr = 48000;
    {
        DelayDsp d; DelayParams p; p.timeMs = 250; p.feedback = 0.5; p.mix = 1; p.lowCutHz = 20; p.highCutHz = 20000; p.warmth = 0; p.wow = 0; d.set(p); d.prepare(sr);
        const int n = (int)sr; std::vector<float> L((size_t)n, 0.0f), R((size_t)n, 0.0f);
        L[0] = R[0] = 0.5f;
        std::vector<float> L2((size_t)n, 0.0f), R2((size_t)n, 0.0f);   // a 1 kHz tone burst for level checks
        for (int i = 0; i < 2400; i++) L2[(size_t)i] = R2[(size_t)i] = (float)(0.1 * std::sin(2 * kPi * 1000 * i / sr));
        for (int o = 0; o < n; o += 128) { float* ch[2] = { L.data() + o, R.data() + o }; d.process(ch, 2, std::min(128, n - o)); }
        int pk = 0; for (int i = 1; i < 18000; i++) if (std::abs(L[(size_t)i]) > std::abs(L[(size_t)pk])) pk = i;
        expect(std::abs(pk - 12000) <= 2, "first echo lands at 250 ms (sample " + std::to_string(pk) + ")");
        DelayDsp d2; d2.set(p); d2.prepare(sr);
        for (int o = 0; o < n; o += 128) { float* ch[2] = { L2.data() + o, R2.data() + o }; d2.process(ch, 2, std::min(128, n - o)); }
        auto rms = [&](int a) { double s2 = 0; for (int i = a + 400; i < a + 2000; i++) s2 += (double)L2[(size_t)i] * L2[(size_t)i]; return s2; };
        const double ratio = 10 * std::log10(rms(24000) / rms(12000));
        expect(std::abs(ratio - gainToDb(0.49)) < 0.5, "each repeat is quieter by the feedback amount (" + std::to_string(ratio) + " dB, want " + std::to_string(gainToDb(0.49)) + ")");
    }
    {   // full feedback with warmth stays bounded
        DelayDsp d; DelayParams p; p.timeMs = 30; p.feedback = 1.0; p.mix = 1; p.warmth = 1; d.set(p); d.prepare(sr);
        const int n = (int)(sr * 4); std::vector<float> L((size_t)n), R((size_t)n);
        std::mt19937 g(4); std::uniform_real_distribution<float> u(-0.9f, 0.9f);
        for (int i = 0; i < n; i++) L[(size_t)i] = R[(size_t)i] = i < 48000 ? u(g) : 0.0f;
        for (int o = 0; o < n; o += 256) { float* ch[2] = { L.data() + o, R.data() + o }; d.process(ch, 2, std::min(256, n - o)); }
        float pk = 0; bool fin = true; for (float x : L) { pk = std::max(pk, std::abs(x)); fin = fin && std::isfinite(x); }
        expect(fin && pk < 4.0f, "maximum feedback stays bounded (peak " + std::to_string(pk) + ")");
    }
}

static void testLimit() {
    std::printf("Limiter\n");
    const double sr = 48000;
    for (bool tp : { false, true }) {
        LimitDsp l; LimitParams p; p.inputDb = 12; p.ceilingDb = -1.0; p.truePeak = tp; l.set(p); l.prepare(sr);
        const int n = (int)(sr * 3); std::vector<float> L((size_t)n), R((size_t)n);
        std::mt19937 g(9); std::normal_distribution<float> u(0, 0.3f);
        // loud noise (band-limited at 20 kHz, like real program material) plus drum hits, driven 12 dB into the limiter
        Svf f1[2], f2[2]; for (int c = 0; c < 2; c++) { f1[c].set(Svf::lowpass, sr, 20000, 0.54); f2[c].set(Svf::lowpass, sr, 20000, 1.31); }
        for (int i = 0; i < n; i++) {
            const double drum = std::exp(-std::fmod(i / sr, 0.25) * 20) * std::sin(2 * kPi * 60 * i / sr);
            L[(size_t)i] = (float)(f2[0].process(f1[0].process(u(g) * 0.5)) + drum); R[(size_t)i] = (float)(f2[1].process(f1[1].process(u(g) * 0.5)) - drum * 0.8);
        }
        for (int o = 0; o < n; o += 300) { float* ch[2] = { L.data() + o, R.data() + o }; l.process(ch, 2, std::min(300, n - o)); }
        double sp = 0; for (int i = 0; i < n; i++) sp = std::max({ sp, (double)std::abs(L[(size_t)i]), (double)std::abs(R[(size_t)i]) });
        // true peak: 8x sinc-interpolate the output and find the largest value
        double tpk = 0;
        for (int i = 64; i < n - 64; i += 1) for (int ph = 1; ph < 8; ph++) {
            double s2 = 0; for (int k = -32; k <= 32; k++) { const double tt = k - ph / 8.0; const double w = 0.5 + 0.5 * std::cos(kPi * tt / 33); s2 += L[(size_t)(i + k)] * std::sin(kPi * tt) / (kPi * tt) * w; }
            tpk = std::max(tpk, std::abs(s2));
        }
        expect(gainToDb(sp) <= -1.0 + 1e-4, std::string(tp ? "true-peak" : "sample-peak") + " mode: sample peaks stay under the ceiling (" + std::to_string(gainToDb(sp)) + " dBFS)");
        if (tp) expect(gainToDb(tpk) <= -0.9, "true-peak mode: intersample peaks stay under the ceiling (" + std::to_string(gainToDb(tpk)) + " dBTP)");
        else std::printf("       sample-peak mode intersample peak: %.2f dBTP\n", gainToDb(tpk));
    }
    {   // under the ceiling: an exact delay
        LimitDsp l; LimitParams p; p.inputDb = 0; p.ceilingDb = 0; p.truePeak = false; l.set(p); l.prepare(sr);
        std::vector<float> L(8192), R(8192); std::mt19937 g(1); std::uniform_real_distribution<float> u(-0.3f, 0.3f);
        for (int i = 0; i < 8192; i++) L[(size_t)i] = R[(size_t)i] = u(g);
        auto L0 = L; float* ch[2] = { L.data(), R.data() }; l.process(ch, 2, 8192);
        double md = 0; const int D = l.latency(); for (int i = D; i < 8192; i++) md = std::max(md, (double)std::abs(L[(size_t)i] - L0[(size_t)(i - D)]));
        expect(md < 1e-6, "below the ceiling it is a clean delay of " + std::to_string(D) + " samples (latency reported correctly)");
    }
}

// Frequency of the strongest component in x (Hz), with parabolic interpolation.
static double dominantHz(const std::vector<float>& x, int start, int len, double sr) {
    Fft f(15); const int n = 1 << 15; std::vector<std::complex<double>> b((size_t)n);
    for (int i = 0; i < n; i++) { const double w = i < len ? 0.5 - 0.5 * std::cos(2 * kPi * i / len) : 0; b[(size_t)i] = { i < len ? x[(size_t)(start + i)] * w : 0.0, 0.0 }; }
    f.transform(b.data(), false);
    int best = 1; for (int k = 2; k < n / 2 - 1; k++) if (std::abs(b[(size_t)k]) > std::abs(b[(size_t)best])) best = k;
    const double a = std::log(std::abs(b[(size_t)best - 1]) + 1e-30), c = std::log(std::abs(b[(size_t)best]) + 1e-30), d = std::log(std::abs(b[(size_t)best + 1]) + 1e-30);
    return (best + 0.5 * (a - d) / (a - 2 * c + d)) * sr / n;
}

static void testGravity() {
    std::printf("Gravity\n");
    const double sr = 48000;
    {   // pull and mass at zero: an exact delay
        GravityDsp g; GravityParams p; p.pull = 0; p.mass = 0; g.set(p); g.prepare(sr);
        const int n = 48000; std::vector<float> L((size_t)n), R((size_t)n); std::mt19937 r(3); std::uniform_real_distribution<float> u(-0.3f, 0.3f);
        for (int i = 0; i < n; i++) { L[(size_t)i] = u(r) + (float)(0.2 * std::sin(2 * kPi * 330 * i / sr)); R[(size_t)i] = u(r); }
        auto L0 = L; for (int o = 0; o < n; o += 512) { float* ch[2] = { L.data() + o, R.data() + o }; g.process(ch, 2, std::min(512, n - o)); }
        const int D = g.latency(); double md = 0; for (int i = D + 4096; i < n; i++) md = std::max(md, (double)std::abs(L[(size_t)i] - L0[(size_t)(i - D)]));
        expect(md < 1e-4, "with no pull it is transparent, a delay of " + std::to_string(D) + " samples (max diff " + std::to_string(md) + ")");
    }
    auto pullTone = [&](double inHz, double pull, int key, int scale, double drift) {
        GravityDsp g; GravityParams p; p.pull = pull; p.mass = 0; p.key = key; p.scale = scale; p.driftMs = drift; p.floorHz = 100; g.set(p); g.prepare(sr);
        const int n = 96000; std::vector<float> L((size_t)n), R((size_t)n);
        for (int i = 0; i < n; i++) L[(size_t)i] = R[(size_t)i] = (float)(0.3 * std::sin(2 * kPi * inHz * i / sr));
        for (int o = 0; o < n; o += 256) { float* ch[2] = { L.data() + o, R.data() + o }; g.process(ch, 2, std::min(256, n - o)); }
        return dominantHz(L, 48000, 32768, sr);
    };
    {
        const double f = pullTone(452, 1.0, 9, 1, 0);   // A minor: 452 Hz falls into A (440)
        expect(std::abs(f - 440) < 1.0, "full pull lands a 452 Hz tone on A 440 (" + std::to_string(f) + " Hz)");
        const double h = pullTone(452, 0.5, 9, 1, 0);
        const double want = 452 * std::sqrt(440.0 / 452.0);
        expect(std::abs(h - want) < 1.0, "half pull goes halfway, in pitch (" + std::to_string(h) + " Hz, want " + std::to_string(want) + ")");
        const double c = pullTone(1000, 1.0, 0, 0, 30);  // C major: 1000 Hz -> B5 987.8 Hz
        expect(std::abs(c - 987.77) < 1.5, "1 kHz in C major settles on B (" + std::to_string(c) + " Hz)");
    }
    {   // mass gathers noise into the scale: energy within 25 cents of a note goes up
        auto inScaleShare = [&](double mass) {
            GravityDsp g; GravityParams p; p.pull = 0; p.mass = mass; p.key = 0; p.scale = 9; p.floorHz = 200; g.set(p); g.prepare(sr);
            const int n = 96000; std::vector<float> L((size_t)n), R((size_t)n); std::mt19937 r(8); std::uniform_real_distribution<float> u(-0.3f, 0.3f);
            for (int i = 0; i < n; i++) L[(size_t)i] = R[(size_t)i] = u(r);
            for (int o = 0; o < n; o += 512) { float* ch[2] = { L.data() + o, R.data() + o }; g.process(ch, 2, std::min(512, n - o)); }
            Fft f(15); const int m = 1 << 15; std::vector<std::complex<double>> b((size_t)m);
            for (int i = 0; i < m; i++) b[(size_t)i] = { L[(size_t)(48000 + i)] * (0.5 - 0.5 * std::cos(2 * kPi * i / m)), 0.0 };
            f.transform(b.data(), false);
            double in = 0, all = 0;
            for (int k = (int)(400.0 * m / sr); k < (int)(8000.0 * m / sr); k++) {
                const double hz = k * sr / m, e = std::norm(b[(size_t)k]);
                all += e; if (std::abs(1200 * std::log2(hz / GravityDsp::nearestNote(hz, 0, 9))) < 25) in += e;
            }
            return in / all;
        };
        const double a = inScaleShare(0), b = inScaleShare(1);
        expect(b > a * 1.8, "mass gathers noise into the octave wells (" + std::to_string(a * 100) + "% -> " + std::to_string(b * 100) + "% of energy near notes)");
    }
    {   // stays finite and bounded on wild input and settings
        GravityDsp g; GravityParams p; g.set(p); g.prepare(sr);
        const int n = 96000; std::vector<float> L((size_t)n), R((size_t)n); std::mt19937 r(5); std::uniform_real_distribution<float> u(-1, 1);
        for (int i = 0; i < n; i++) { L[(size_t)i] = u(r); R[(size_t)i] = (i / 4000) % 2 ? 0.0f : u(r); }
        float pk = 0; bool fin = true;
        for (int o = 0; o < n; o += 333) {
            p.pull = (o / 3330 % 5) / 4.0; p.mass = (o / 7000 % 3) / 2.0; p.key = o / 9000 % 12; p.scale = o / 11000 % 11; p.driftMs = (o / 5000 % 4) * 200; g.set(p);
            float* ch[2] = { L.data() + o, R.data() + o }; g.process(ch, 2, std::min(333, n - o));
        }
        for (int i = 0; i < n; i++) { fin = fin && std::isfinite(L[(size_t)i]) && std::isfinite(R[(size_t)i]); pk = std::max(pk, std::abs(L[(size_t)i])); }
        expect(fin && pk < 3, "stays finite and bounded with settings changing under it (peak " + std::to_string(pk) + ")");
    }
}

static void testDuck() {
    std::printf("Duck\n");
    const double sr = 48000, bpm = 120; const int B = 256, N = (int)(sr * 4);   // 4 s = 8 beats at 120
    const int beat = (int)(sr * 60 / bpm);
    auto runTempo = [&](double amount, double rate, bool playing, int len) {
        DuckDsp d; d.prepare(sr); d.amount = amount; d.release = 0.5; d.rateBeats = rate;
        std::vector<float> L((size_t)len, 1.0f);
        for (int o = 0; o < len; o += B) { float* ch[1] = { &L[(size_t)o] }; d.process(ch, 1, std::min(B, len - o), playing ? o * bpm / 60 / sr : 0, bpm, playing, nullptr, 0); }
        return L;
    };
    {
        auto L = runTempo(1.0, 1.0, true, N);
        double minv = 1, maxStep = 0;
        for (int i = 1; i < N; ++i) { minv = std::min(minv, (double)L[(size_t)i]); maxStep = std::max(maxStep, (double)std::abs(L[(size_t)i] - L[(size_t)i - 1])); }
        expect(L[(size_t)(2 * beat + (int)(0.003 * sr))] < 0.05, "tempo: volume is down right after the beat");
        expect(L[(size_t)(2 * beat + (int)(0.9 * beat))] > 0.99, "tempo: volume is back up before the next beat");
        expect(minv >= 0 && minv < 0.02, "tempo: dips all the way at 100% and never goes negative");
        expect(maxStep < 0.03, "tempo: no clicks (largest per-sample gain step " + std::to_string(maxStep) + ")");
        auto H = runTempo(0.5, 1.0, true, B * 40); double hmin = 1; for (float v : H) hmin = std::min(hmin, (double)v);
        expect(std::abs(hmin - 0.5) < 0.03, "tempo: 50% amount dips to half volume");
        auto E = runTempo(1.0, 0.5, true, N);
        expect(E[(size_t)(beat + beat / 2 + (int)(0.003 * sr))] < 0.05, "tempo: 1/8 rate also dips on the off-beat");
        auto S = runTempo(0.75, 1.0, false, B * 20); double smin = 1; for (float v : S) smin = std::min(smin, (double)v);
        expect(smin > 0.999, "tempo: does nothing while the song is stopped");
    }
    {   // kick mode: kicks at 0.5 s, 1.5 s, 2.25 s; the reported hits and gains match what happened to the audio
        std::vector<float> sc((size_t)N, 0.0f);
        for (double t : { 0.5, 1.5, 2.25 }) for (int i = 0; i < (int)(0.12 * sr); ++i) sc[(size_t)((int)(t * sr) + i)] = (float)(0.8 * std::exp(-i / (0.03 * sr)) * std::sin(2 * M_PI * 55 * i / sr));
        for (float level : { 1.0f, 0.02f }) {
            DuckDsp k; k.prepare(sr); k.mode = 1; k.amount = 1; k.release = 0.4;
            std::vector<float> K((size_t)N, 1.0f), gains((size_t)N); std::vector<uint8_t> hits((size_t)N);
            std::vector<float> s2v((size_t)N); for (int i = 0; i < N; i++) s2v[(size_t)i] = sc[(size_t)i] * level;
            for (int o = 0; o < N; o += B) { float* ch[1] = { &K[(size_t)o] }; const float* s2[1] = { &s2v[(size_t)o] }; k.process(ch, 1, B, 0, bpm, true, s2, 1, &gains[(size_t)o], &hits[(size_t)o]); }
            int nh = 0; for (auto h : hits) nh += h;
            double gd = 0; for (int i = 0; i < N; i++) gd = std::max(gd, (double)std::abs(gains[(size_t)i] - K[(size_t)i]));
            const std::string tag = level < 1 ? " (quiet kick, -34 dB)" : "";
            expect(K[(size_t)(0.25 * sr)] > 0.999, "kick: untouched before the first kick" + tag);
            expect(K[(size_t)(0.505 * sr)] < 0.1 && K[(size_t)(1.505 * sr)] < 0.1 && K[(size_t)(2.255 * sr)] < 0.1, "kick: dips on every kick" + tag);
            expect(K[(size_t)(1.2 * sr)] > 0.99, "kick: recovers between kicks" + tag);
            expect(nh == 3, "kick: detects exactly 3 hits, no double triggers (" + std::to_string(nh) + ")" + tag);
            expect(gd < 1e-6, "kick: reported gain matches the audio exactly" + tag);
        }
    }
}

static void testGuitar() {
    std::printf("Guitar\n");
    const double sr = 48000;
    auto render = [&](GuitarParams p, std::vector<std::pair<int, int>> ons, double secs, std::vector<std::pair<int, int>> offs = {}) {
        GuitarDsp g; g.set(p); g.prepare(sr);
        const int n = (int)(secs * sr); std::vector<float> L((size_t)n), R((size_t)n);
        for (int o = 0; o < n; o += 256) {
            const int m = std::min(256, n - o);
            for (auto& e : ons) if (e.second >= o && e.second < o + m) g.noteOn(e.first, 0.8, e.second - o);
            for (auto& e : offs) if (e.second >= o && e.second < o + m) g.noteOff(e.first, e.second - o);
            g.process(L.data() + o, R.data() + o, m);
        }
        for (int i = 0; i < n; i++) L[(size_t)i] = 0.5f * (L[(size_t)i] + R[(size_t)i]);
        return L;
    };
    auto pitch = [&](const std::vector<float>& x, size_t from, size_t len, double expect) {
        double best = 0, bf = expect;
        for (double c = -50; c <= 50; c += 0.25) {
            const double f = expect * std::pow(2.0, c / 1200); double re = 0, im = 0;
            for (size_t i = 0; i < len; i++) { const double w = 0.5 - 0.5 * std::cos(2 * kPi * i / len); re += x[from + i] * w * std::cos(2 * kPi * f * i / sr); im -= x[from + i] * w * std::sin(2 * kPi * f * i / sr); }
            if (re * re + im * im > best) { best = re * re + im * im; bf = f; }
        }
        return 1200 * std::log2(bf / expect);
    };
    {   // in tune across the neck, with bright/stiff and dark settings
        double worst = 0;
        for (int cfg = 0; cfg < 3; cfg++) {
            GuitarParams p; p.body = 0; p.verbMix = 0;
            if (cfg == 1) { p.bright = 1; p.stiffness = 1; } if (cfg == 2) { p.bright = 0; p.decayS = 1; }
            for (int note : { 40, 52, 64, 76, 88 }) { auto x = render(p, { { note, 0 } }, 0.5); worst = std::max(worst, std::abs(pitch(x, (size_t)(0.1 * sr), (size_t)(0.25 * sr), GuitarDsp::noteHz(note)))); }
        }
        expect(worst <= 1.0, "in tune from E2 to E6 across bright, stiff and dark strings (worst " + std::to_string(worst) + " cents)");
    }
    {   // strum: six notes arrive together and are spread over the strum time, low to high on a down-strum
        GuitarParams p; p.strumMs = 60; p.strumDir = 0; p.humanize = 0; p.body = 0; p.verbMix = 0;
        std::vector<std::pair<int, int>> chord; for (int n : { 40, 47, 52, 56, 59, 64 }) chord.push_back({ n, 0 });
        auto x = render(p, chord, 0.2);
        int first = -1, last = -1; double pk = 0; for (float v : x) pk = std::max(pk, (double)std::abs(v));
        for (size_t i = 0; i < x.size(); i++) if (std::abs(x[i]) > pk * 0.02) { first = (int)i; break; }
        // the last string starts 60 ms after the first: the signal keeps gaining new energy until then
        double e1 = 0, e2 = 0; for (int i = 0; i < (int)(0.03 * sr); i++) e1 += x[(size_t)i] * x[(size_t)i];
        for (int i = (int)(0.06 * sr); i < (int)(0.09 * sr); i++) e2 += x[(size_t)i] * x[(size_t)i];
        (void)last;
        expect(first >= 0 && first < (int)(0.003 * sr), "a strum starts straight away");
        expect(e2 > e1 * 0.8, "and is still adding strings 60 ms in (spread across the strum time)");
    }
    {   // a new note on the same string chokes the old one (guitar mode); free mode lets both ring
        GuitarParams p; p.body = 0; p.verbMix = 0; p.decayS = 8;
        auto choke = render(p, { { 64, 0 }, { 65, (int)(0.3 * sr) } }, 0.8);
        p.mode = 1; auto free = render(p, { { 64, 0 }, { 65, (int)(0.3 * sr) } }, 0.8);
        const size_t at = (size_t)(0.5 * sr), len = (size_t)(0.25 * sr);
        auto level = [&](const std::vector<float>& x, double f) { double re = 0, im = 0; for (size_t i = 0; i < len; i++) { re += x[at + i] * std::cos(2 * kPi * f * i / sr); im += x[at + i] * std::sin(2 * kPi * f * i / sr); } return std::sqrt(re * re + im * im); };
        const double e4 = GuitarDsp::noteHz(64);
        expect(level(choke, e4) < level(free, e4) * 0.1, "guitar mode: a new note on the high E string stops the E that was ringing");
    }
    {   // releasing a note damps it; holding lets it ring
        GuitarParams p; p.body = 0; p.verbMix = 0; p.decayS = 8; p.releaseS = 0.1;
        auto held = render(p, { { 52, 0 } }, 1.0), rel = render(p, { { 52, 0 } }, 1.0, { { 52, (int)(0.3 * sr) } });
        double eh = 0, er = 0; for (int i = (int)(0.7 * sr); i < (int)(0.9 * sr); i++) { eh += held[(size_t)i] * held[(size_t)i]; er += rel[(size_t)i] * rel[(size_t)i]; }
        expect(er < eh * 1e-3, "letting go of a key damps the string (release)");
    }
    {   // nothing blows up: every choice, extreme knobs, fast notes, bends and sustain
        bool ok = true; double pk = 0;
        for (int amp = 0; amp <= 4; amp++) for (int course = 0; course <= 2; course++) {
            GuitarParams p; p.amp = amp; p.course = course; p.cab = amp % 4; p.pickup = course + 1; p.body = (amp + course) % 6; p.driveDb = 40; p.bright = 1; p.stiffness = 1;
            p.decayS = 20; p.releaseS = 5; p.chorus = 1; p.delayMix = 1; p.delayFb = 1; p.verbMix = 1; p.age = 1; p.mode = (amp + course) % 3; p.bassDb = 12; p.trebleDb = 12;
            GuitarDsp g; g.set(p); g.prepare(sr);
            std::vector<float> L(256), R(256);
            for (int b = 0; b < 400; b++) {
                if (b % 3 == 0) for (int k = 0; k < 4; k++) g.noteOn(28 + (b * 7 + k * 5) % 72, 1.0, k * 10);
                if (b % 3 == 1) for (int k = 0; k < 4; k++) g.noteOff(28 + ((b - 1) * 7 + k * 5) % 72, 5);
                g.pitchBend(std::sin(b * 0.1)); g.modWheel(0.5 + 0.5 * std::sin(b * 0.05)); g.sustainPedal(b % 50 < 25);
                std::fill(L.begin(), L.end(), 0.0f); std::fill(R.begin(), R.end(), 0.0f);
                g.process(L.data(), R.data(), 256);
                for (int i = 0; i < 256; i++) { ok = ok && std::isfinite(L[(size_t)i]) && std::isfinite(R[(size_t)i]); pk = std::max(pk, (double)std::abs(L[(size_t)i])); }
            }
        }
        expect(ok && pk < 8, "stays finite and bounded at every extreme (peak " + std::to_string(pk) + ")");
    }
}

int main() {
    testEq(); testComp(); testDeEss(); testSat(); testVerb(); testDelay(); testLimit(); testGravity(); testDuck(); testGuitar();
    std::printf(fails ? "FAILED (%d)\n" : "ALL PASS\n", fails);
    return fails ? 1 : 0;
}
