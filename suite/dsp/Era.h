// LGN Era: makes audio sound like it came off a specific record, tape, sampler or speaker from a given decade.
// Each era is a recipe of what that medium really did to sound:
//   sampling   sample rate and bit depth (and whether there was an anti-alias filter: the SP-1200 had none,
//              which is where its gritty, aliased top end comes from), mu-law for telephone-grade machines
//   bandwidth  the low and high limits of the medium, a tape head bump, a presence peak
//   saturation tape (soft, symmetric) or tube (asymmetric, warm)
//   motion     wow (slow pitch drift) and flutter (fast)
//   noise      hiss, vinyl crackle and pops, mains hum, cassette dropouts
//   stereo     mono for old media, mono low end for vinyl
//   codec      MP3-style spectral holes (quiet detail thrown away frame by frame)
// Age blends from clean (0) to the full era (1). Plain C++ (no JUCE) so it can be tested on its own.
#pragma once
#include "Common.h"
#include <random>

namespace lgn {

struct EraParams { int era = 2; double age = 0.75, grit = 1, wobble = 1, noise = 1, tone = 0, width = 1, mix = 1, outDb = 0; };

class EraDsp {
public:
    struct Recipe {
        const char* name; const char* year;
        double srHz, bits; bool mulaw, antiAlias;
        double hpHz, lpHz, lpQ, bumpHz, bumpDb, peakHz, peakDb;
        double sat; int satType;            // 0 tape, 1 tube
        double wowMs, wowHz, flutterMs, flutterHz;
        double hissDb, crackle, pops, humDb, dropouts;
        double mono, lowMono; bool codec;
    };
    static constexpr int kEras = 12;
    static const Recipe& recipe(int e) {
        static const Recipe r[kEras] = {
            //  name                year     sr      bits  mu    AA    hp    lp     Q    bump       peak        sat  type wow       flutter     hiss   crk  pops  hum   drop  mono lowM codec
            { "Shellac 78",        "1940s",  0,      0,  false, true, 180, 5200, 0.9, 0, 0,     1800, 3,   0.5, 0, 0.35, 0.5, 0.04, 9,   -38, 9.0, 1.2, -60, 0,    1,   1,   false },
            { "AM Radio",          "1960s",  0,      0,  false, true, 320, 3600, 1.1, 0, 0,     1500, 4,   0.65, 1, 0.08, 0.3, 0,   0,   -44, 0,   0,   -52, 0,    1,   1,   false },
            { "Soul Vinyl",        "1970s",  0,      0,  false, true, 28, 15000, 0.7, 110, 1.5, 3200, 1,   0.3, 0, 0.12, 0.55, 0,  0,   -66, 2.5, 0.15, -72, 0,   0.1, 1,   false },
            { "Cassette",          "1980s",  0,      0,  false, true, 40, 10500, 0.7, 85, 3,    2000, 0.5, 0.4, 0, 0.55, 0.85, 0.06, 7, -52, 0,   0,   -80, 0.25, 0,  0.3, false },
            { "SP-1200",           "1987",   26040, 12,  false, false, 25, 13500, 0.8, 120, 1,   0, 0,     0.2, 1, 0,   0,   0,   0,   -78, 0,   0,   -90, 0,    0,   0,   false },
            { "MPC60",             "1988",   40000, 12,  false, true, 25, 16000, 0.7, 100, 1.5, 0, 0,     0.35, 1, 0,   0,   0,   0,   -82, 0,   0,   -90, 0,    0,   0,   false },
            { "S950",              "1988",   22050, 12,  false, true, 30, 9000, 1.1, 0, 0,      0, 0,     0.25, 1, 0,   0,   0,   0,   -80, 0,   0,   -90, 0,    0,   0,   false },
            { "VHS",               "1990",   0,      0,  false, true, 50, 9500, 0.7, 0, 0,      2500, 1.5, 0.3, 0, 0.45, 0.4, 0.05, 5, -54, 0,   0,   -72, 0.1, 0.2, 0.3, false },
            { "Boombox",           "1990s",  0,      0,  false, true, 75, 12000, 0.7, 0, 0,     1500, 2.5, 0.5, 1, 0.3, 0.7, 0.04, 6,  -58, 0,   0,   -80, 0.15, 0.2, 0.4, false },
            { "MP3 128k",          "2003",   0,      0,  false, true, 20, 16000, 0.7, 0, 0,     0, 0,     0,   0, 0,   0,   0,   0,   -120, 0,  0,   -120, 0,   0,   0,   true },
            { "Phone Speaker",     "2010s",  0,      0,  false, true, 420, 6500, 1.0, 0, 0,     2600, 6,   0.6, 1, 0,   0,   0,   0,   -90, 0,   0,   -100, 0,   1,   1,   false },
            { "Answering Machine", "1990s",  8000,   8,  true,  true, 300, 3400, 0.9, 0, 0,     1800, 2,   0.45, 0, 0.4, 0.7, 0.05, 6, -46, 0,   0,   -64, 0.1, 1,   1,   false } };
        return r[clampv(e, 0, kEras - 1)];
    }
    static constexpr int kCodecN = 512, kCodecH = 128;
    static int latency() { return kCodecN; }   // fixed, so switching eras never changes the plugin's delay

    void prepare(double rate) {
        sr = rate;
        for (int c = 0; c < 2; c++) {
            for (auto& f : aa[c]) f.reset();
            for (auto& f : band[c]) f.reset();
            recon[c].reset(); hissF[c].reset(); crackleF[c].reset();
            wowLine[c].prepare((int)(0.03 * sr) + 8); dryLine[c].prepare(kCodecN + 8);
            held[c] = 0; heldPrev[c] = 0;
            inF[c].assign((size_t)kCodecN, 0.0); outF[c].assign((size_t)kCodecH, 0.0); acc[c].assign((size_t)kCodecN * 2, 0.0); X[c].assign((size_t)kCodecN, {});
        }
        sideLp.reset(); toneLo.reset(); toneHi.reset(); toneLo2.reset(); toneHi2.reset();
        fft.init(9); win.resize((size_t)kCodecN);
        for (int i = 0; i < kCodecN; i++) win[(size_t)i] = 0.5 - 0.5 * std::cos(2 * kPi * i / kCodecN);
        double s2 = 0; for (double w : win) s2 += w * w; olaScale = (double)kCodecH / s2;
        rover = kCodecN - kCodecH;
        shPhase = 0; wowPh = 0; flutPh = 0; dropGain = 1; dropLeft = 0; crackleEnv[0] = crackleEnv[1] = 0; humPh = 0;
        mixS.setTime(sr, 0.02); mixS.reset(p.mix);
        update();
    }
    void set(const EraParams& np) { const bool changed = np.era != p.era || np.age != p.age || np.grit != p.grit || np.tone != p.tone || np.wobble != p.wobble || np.noise != p.noise; p = np; if (changed) dirty = true; }

    void process(float* const* ch, int nch, int n) {
        nch = std::min(nch, 2);
        if (dirty) update();
        const auto& R = recipe(p.era);
        const double a = clampv(p.age, 0.0, 1.0);
        for (int i = 0; i < n; i++) {
            double x[2] = { ch[0][i], nch > 1 ? ch[1][i] : ch[0][i] };
            // the dry signal, delayed by the fixed latency so the mix lines up
            double dry[2];
            for (int c = 0; c < 2; c++) { dryLine[c].push((float)x[c]); dry[c] = dryLine[c].tap(kCodecN); }
            // codec stage first (it needs the latency); the other stages run on its output
            if (R.codec && a > 0.001) codecTick(x);
            else for (int c = 0; c < 2; c++) { inF[c][(size_t)rover] = x[c]; x[c] = dryLine[c].tap(kCodecN); }
            if (!(R.codec && a > 0.001) && ++rover >= kCodecN) rover = kCodecN - kCodecH;
            // mono / mono low end
            {
                const double mid = 0.5 * (x[0] + x[1]); double side = 0.5 * (x[0] - x[1]);
                side *= 1 - R.mono * a;
                if (R.lowMono > 0) side -= sideLp.process(side) * R.lowMono * a;   // remove side below ~150 Hz
                side *= p.width;
                x[0] = mid + side; x[1] = mid - side;
            }
            // wow and flutter (a modulated delay; both channels move together, like one tape)
            const bool motion = wowDepth > 0 || flutDepth > 0;
            if (motion) {
                wowPh += R.wowHz / sr; if (wowPh >= 1) wowPh -= 1;
                flutPh += R.flutterHz / sr; if (flutPh >= 1) flutPh -= 1;
                const double d = 2 + (wowDepth + flutDepth) + wowDepth * std::sin(2 * kPi * wowPh) + flutDepth * std::sin(2 * kPi * flutPh) * (0.7 + 0.3 * std::sin(2 * kPi * wowPh * 3.3));
                for (int c = 0; c < 2; c++) { wowLine[c].push((float)x[c]); x[c] = wowLine[c].read(d); }
            }
            // sampler: anti-alias (if the machine had one), sample-and-hold to its rate, quantise to its bits
            if (srEff > 0) {
                for (int c = 0; c < 2; c++) if (R.antiAlias) for (auto& f : aa[c]) x[c] = f.process(x[c]);
                shPhase += srEff / sr;
                if (shPhase >= 1) {
                    shPhase -= std::floor(shPhase);
                    for (int c = 0; c < 2; c++) held[c] = quantise(x[c], R.mulaw);
                }
                x[0] = held[0]; x[1] = held[1];
            } else if (bitsEff < 23.5) for (int c = 0; c < 2; c++) x[c] = quantise(x[c], R.mulaw);
            // the medium's bandwidth and colour
            for (int c = 0; c < 2; c++) for (auto& f : band[c]) x[c] = f.process(x[c]);
            // saturation
            if (satEff > 0.001) for (int c = 0; c < 2; c++) {
                const double g = 1 + satEff * 4;
                x[c] = R.satType == 1 ? (std::tanh(g * x[c] + 0.2 * satEff) - std::tanh(0.2 * satEff)) / g * (1 + satEff * 0.8) : std::tanh(g * x[c]) / g * (1 + satEff * 0.8);
            }
            // tone tilt
            if (std::abs(p.tone) > 0.01) for (int c = 0; c < 2; c++) x[c] = c == 0 ? toneHi.process(toneLo.process(x[c])) : toneHi2.process(toneLo2.process(x[c]));
            // noise: hiss, crackle and pops, hum
            const double nz = p.noise * a;
            if (nz > 0.001) {
                for (int c = 0; c < 2; c++) {
                    const double white = rnd() * 2 - 1;
                    const double hiss = hissF[c].process(white) * hissGain;
                    double crk = 0;
                    if (R.crackle > 0 && rnd() < R.crackle * nz / sr) crackleEnv[c] = (0.3 + 0.7 * rnd()) * (rnd() < 0.5 ? -1 : 1);
                    if (R.pops > 0 && rnd() < R.pops * nz / sr) crackleEnv[c] = (rnd() < 0.5 ? -1 : 1) * 3.0;
                    if (crackleEnv[c] != 0) { crk = crackleEnv[c] * (rnd() * 0.6 + 0.4); crackleEnv[c] *= 0.82; if (std::abs(crackleEnv[c]) < 0.01) crackleEnv[c] = 0; }
                    crk = crackleF[c].process(crk) * 0.04;
                    x[c] += (hiss + crk) * (R.mono > 0.5 ? 1.0 : (c ? 0.9 : 1.0));
                }
                if (humGain > 0) { humPh += 60.0 / sr; if (humPh >= 1) humPh -= 1; const double h = humGain * (std::sin(2 * kPi * humPh) + 0.5 * std::sin(6 * kPi * humPh)); x[0] += h; x[1] += h; }
            }
            // cassette dropouts: brief dips in level
            if (R.dropouts > 0 && a > 0.001) {
                if (dropLeft <= 0 && rnd() < R.dropouts * a / sr) { dropLeft = (int)(sr * (0.03 + 0.09 * rnd())); dropDepth = 0.25 + 0.45 * rnd(); }
                const double target = dropLeft > 0 ? dropDepth : 1.0; if (dropLeft > 0) dropLeft--;
                dropGain += (target - dropGain) * 0.004;
                x[0] *= dropGain; x[1] *= dropGain;
            }
            const double m = mixS.next(p.mix), og = outGain;
            ch[0][i] = (float)((dry[0] * (1 - m) + x[0] * m) * og);
            if (nch > 1) ch[1][i] = (float)((dry[1] * (1 - m) + x[1] * m) * og);
        }
    }
    // what the display shows about the current settings
    double effectiveRate() const { return srEff; }
    double effectiveBits() const { return bitsEff; }

private:
    double rnd() { return std::uniform_real_distribution<double>(0, 1)(rng); }
    double quantise(double x, bool mulaw) const {
        if (bitsEff >= 23.5) return x;
        const double q = std::pow(2.0, bitsEff - 1);
        if (mulaw) {   // compress, quantise, expand (telephone-grade)
            const double mu = 255, s = x < 0 ? -1 : 1, ax = std::min(1.0, std::abs(x));
            double y = s * std::log1p(mu * ax) / std::log1p(mu);
            y = std::round(y * q) / q;
            return (y < 0 ? -1 : 1) * (std::pow(1 + mu, std::abs(y)) - 1) / mu;
        }
        return std::round(x * q) / q;
    }
    // MP3-style artifacts: in each frame, detail more than a set amount below the loudest part of its band is
    // thrown away, and everything above ~16 kHz is gone. What's left "swirls" from frame to frame.
    void codecTick(double* x) {
        double y[2];
        for (int c = 0; c < 2; c++) { inF[c][(size_t)rover] = x[c]; y[c] = outF[c][(size_t)(rover - (kCodecN - kCodecH))]; }
        if (++rover >= kCodecN) { codecFrame(); rover = kCodecN - kCodecH; }
        x[0] = y[0]; x[1] = y[1];
    }
    void codecFrame() {
        const int N = kCodecN, NB = N / 2 + 1;
        const double thrDb = -36 + 22 * p.age * p.grit;   // more age, more thrown away
        const int topBin = (int)std::min((double)NB - 1, (16000.0 - 3000.0 * p.age) * N / sr);
        for (int c = 0; c < 2; c++) {
            auto& Xc = X[c];
            for (int i = 0; i < N; i++) Xc[(size_t)i] = { inF[c][(size_t)i] * win[(size_t)i], 0.0 };
            fft.transform(Xc.data(), false);
            // 24 bands, roughly log spaced
            int k0 = 1;
            for (int b = 0; b < 24 && k0 < NB; b++) {
                const int k1 = std::min(NB, std::max(k0 + 1, (int)(1.0 * std::pow((double)NB, (b + 1) / 24.0))));
                double mx = 0; for (int k = k0; k < k1; k++) mx = std::max(mx, std::norm(Xc[(size_t)k]));
                const double thr = mx * std::pow(10.0, thrDb / 10.0);
                for (int k = k0; k < k1; k++) if (std::norm(Xc[(size_t)k]) < thr || k > topBin) Xc[(size_t)k] = 0;
                k0 = k1;
            }
            for (int k = 1; k < N / 2; k++) Xc[(size_t)(N - k)] = std::conj(Xc[(size_t)k]);
            Xc[0] = { Xc[0].real(), 0 }; Xc[(size_t)N / 2] = { Xc[(size_t)N / 2].real(), 0 };
            fft.transform(Xc.data(), true);
            for (int i = 0; i < N; i++) acc[c][(size_t)i] += Xc[(size_t)i].real() * win[(size_t)i] * olaScale;
            for (int i = 0; i < kCodecH; i++) outF[c][(size_t)i] = acc[c][(size_t)i];
            std::copy(acc[c].begin() + kCodecH, acc[c].begin() + kCodecH + N, acc[c].begin());
            std::fill(acc[c].begin() + N, acc[c].end(), 0.0);
            std::copy(inF[c].begin() + kCodecH, inF[c].end(), inF[c].begin());
        }
    }
    void update() {
        dirty = false;
        const auto& R = recipe(p.era);
        const double a = clampv(p.age, 0.0, 1.0), g = clampv(p.grit, 0.0, 2.0);
        // sampling: rate and bits move from transparent toward the machine's as Age (and Grit) rise
        const double ga = clampv(a * g, 0.0, 2.0);
        srEff = R.srHz > 0 ? std::max(2000.0, std::min(sr, R.srHz * std::pow(sr / R.srHz, 1 - std::min(1.0, ga)) / (ga > 1 ? 1 + (ga - 1) * 0.6 : 1.0))) : 0.0;
        if (srEff >= sr * 0.999) srEff = 0;
        bitsEff = R.bits > 0 ? std::max(4.0, 24 - (24 - R.bits) * ga) : 24.0;
        for (int c = 0; c < 2; c++) {
            const double aaHz = srEff > 0 ? srEff * 0.45 : sr * 0.45;
            aa[c][0].set(Svf::lowpass, sr, aaHz, butterQ(2, 0)); aa[c][1].set(Svf::lowpass, sr, aaHz, butterQ(2, 1));
            // bandwidth: interpolate (in log frequency) from full range to the medium's limits
            const double lp = std::exp(std::log(20000.0) * (1 - a) + std::log(R.lpHz) * a), hp = std::exp(std::log(15.0) * (1 - a) + std::log(R.hpHz) * a);
            band[c][0].set(Svf::highpass, sr, hp, 0.707);
            band[c][1].set(Svf::lowpass, sr, std::min(lp, sr * 0.45), R.lpQ);
            band[c][2].set(Svf::bell, sr, R.bumpHz > 0 ? R.bumpHz : 100, 1.2, R.bumpDb * a);
            band[c][3].set(Svf::bell, sr, R.peakHz > 0 ? R.peakHz : 2000, 0.9, R.peakDb * a);
            hissF[c].set(Svf::lowpass, sr, 9000, 0.5);
            crackleF[c].set(Svf::bandpass, sr, 3500, 0.8);
        }
        sideLp.set(Svf::lowpass, sr, 150, 0.707);
        satEff = R.sat * a * clampv(p.grit, 0.0, 2.0);
        wowDepth = R.wowMs * 0.001 * sr * a * p.wobble;
        flutDepth = R.flutterMs * 0.001 * sr * a * p.wobble;
        hissGain = dbToGain(R.hissDb) * 2.5;
        humGain = R.humDb > -95 ? dbToGain(R.humDb) * p.noise * a : 0;
        toneLo.set(Svf::lowShelf, sr, 900, 0.5, -p.tone * 5); toneHi.set(Svf::highShelf, sr, 900, 0.5, p.tone * 5);
        toneLo2.set(Svf::lowShelf, sr, 900, 0.5, -p.tone * 5); toneHi2.set(Svf::highShelf, sr, 900, 0.5, p.tone * 5);
        outGain = dbToGain(p.outDb);
    }

    EraParams p; double sr = 48000; bool dirty = true;
    std::array<std::array<Svf, 2>, 2> aa; std::array<std::array<Svf, 4>, 2> band;
    std::array<Svf, 2> recon, hissF, crackleF; Svf sideLp, toneLo, toneHi, toneLo2, toneHi2;
    std::array<DelayLine, 2> wowLine, dryLine;
    double held[2] = {}, heldPrev[2] = {}, shPhase = 0, wowPh = 0, flutPh = 0, humPh = 0, crackleEnv[2] = {};
    double srEff = 0, bitsEff = 24, satEff = 0, wowDepth = 0, flutDepth = 0, hissGain = 0, humGain = 0, outGain = 1;
    double dropGain = 1, dropDepth = 0.5; int dropLeft = 0;
    // codec
    Fft fft{ 9 }; std::vector<double> win; double olaScale = 1; int rover = kCodecN - kCodecH;
    std::vector<double> inF[2], outF[2], acc[2]; std::vector<std::complex<double>> X[2];
    Smooth mixS; std::mt19937 rng{ 777 };
};
} // namespace lgn
