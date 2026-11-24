// LGN Gravity: pulls every resonance in a sound toward the notes of a key.
//
// Short-time Fourier analysis (4x overlap). Each frame:
//   1. find the spectral peaks of the mid signal above the Floor, and each peak's true frequency from its phase advance
//   2. give every peak the region of bins around it (split at the minima between peaks)
//   3. the target for a peak is the nearest note of the chosen key and scale; Pull sets how far toward it the peak
//      moves (as a ratio), Drift sets how quickly it gets there (a slow drift makes sounds glide and sing)
//   4. move each region to its new place and continue its phase as a sinusoid at the new frequency, keeping the
//      phase relationships inside the region (phase-locked vocoder), so tones stay clean instead of going "phasey"
//   5. Mass shapes the spectrum around the notes, gathering energy into the wells (noise starts to ring in key)
// Bins below the Floor pass through untouched, and with Pull and Mass at zero the whole thing is transparent.
#pragma once
#include "Common.h"
#include <atomic>

namespace lgn {

struct GravityParams { int key = 0, scale = 1; double pull = 0.7, driftMs = 80, mass = 0.2, floorHz = 150, mix = 1.0; };

class GravityDsp {
public:
    struct PeakView { float fromHz, toHz, level; };
    static constexpr int kMaxView = 96;

    static int orderFor(double sr) { return sr <= 50000 ? 11 : sr <= 100000 ? 12 : 13; }

    void prepare(double rate) {
        sr = rate; order = orderFor(sr); N = 1 << order; H = N / 4; NB = N / 2 + 1;
        fft.init(order);
        win.resize((size_t)N);
        for (int i = 0; i < N; i++) win[(size_t)i] = 0.5 - 0.5 * std::cos(2 * kPi * i / N);
        double s2 = 0; for (int i = 0; i < N; i++) s2 += win[(size_t)i] * win[(size_t)i];
        olaScale = (double)H / s2;   // Hann analysis x Hann synthesis at hop N/4
        for (int c = 0; c < 2; c++) {
            inF[c].assign((size_t)N, 0.0); outF[c].assign((size_t)H, 0.0); acc[c].assign((size_t)N * 2, 0.0);
            X[c].assign((size_t)N, {}); Y[c].assign((size_t)N, {}); if (c == 0) Z2.assign((size_t)N, {});

            dry[c].assign((size_t)latency(), 0.0f);
        }
        midNow.assign((size_t)NB, {}); midPrev.assign((size_t)NB, {});
        for (int c = 0; c < 2; c++) { synY[c].assign((size_t)NB, {}); synPh[c].assign((size_t)NB, 0.0); synPending[c].assign((size_t)NB, 0); } sLog.assign((size_t)NB, 0.0); mag.assign((size_t)NB, 0.0); trueBin.assign((size_t)NB, 0.0);
        wellW.assign((size_t)NB, 1.0); regionTarget.assign((size_t)NB, 0.0);
        rover = N - H; dpos = 0; lastKey = -1; lastScale = -1; massGain = 1;
        mixS.setTime(sr, 0.02); mixS.reset(p.mix);
        viewCount = 0;
    }
    int latency() const { return N; }   // a full analysis frame
    void set(const GravityParams& np) { p = np; }

    void process(float* const* ch, int nch, int n) {
        nch = std::min(nch, 2);
        for (int i = 0; i < n; i++) {
            const double inL = ch[0][i], inR = nch > 1 ? ch[1][i] : inL;
            inF[0][(size_t)rover] = inL; inF[1][(size_t)rover] = inR;
            const double wl = outF[0][(size_t)(rover - (N - H))], wr = outF[1][(size_t)(rover - (N - H))];
            // dry path delayed by the same latency
            const double dl = dry[0][(size_t)dpos], dr = dry[1][(size_t)dpos];
            dry[0][(size_t)dpos] = (float)inL; dry[1][(size_t)dpos] = (float)inR; dpos = (dpos + 1) % (int)dry[0].size();
            const double m = mixS.next(p.mix);
            ch[0][i] = (float)(dl * (1 - m) + wl * m);
            if (nch > 1) ch[1][i] = (float)(dr * (1 - m) + wr * m);
            if (++rover >= N) { frame(2); rover = N - H; }
        }
    }

    // Latest peak movements for the display (from the audio thread; the UI copies what it finds).
    int peaks(PeakView* out) const { const int n = viewCount.load(std::memory_order_acquire); for (int i = 0; i < n; i++) out[i] = view[(size_t)i]; return n; }

    // Frequencies of the scale's notes between lo and hi Hz (for the display and tests).
    static std::vector<double> noteFreqs(int key, int scale, double lo, double hi) {
        std::vector<double> f; const int mask = scales()[(size_t)clampv(scale, 0, (int)scales().size() - 1)].mask;
        for (int midi = 0; midi < 136; midi++) {
            if (!((mask >> (((midi - key) % 12 + 12) % 12)) & 1)) continue;
            const double hz = 440.0 * std::pow(2.0, (midi - 69) / 12.0);
            if (hz >= lo && hz <= hi) f.push_back(hz);
        }
        return f;
    }
    // Nearest note of the scale to f (Hz).
    static double nearestNote(double f, int key, int scale) {
        const int mask = scales()[(size_t)clampv(scale, 0, (int)scales().size() - 1)].mask;
        const double midi = 69 + 12 * std::log2(std::max(f, 1.0) / 440.0);
        double best = f, bd = 1e9;
        const int m0 = (int)std::floor(midi) - 12, m1 = (int)std::ceil(midi) + 12;
        for (int m = m0, pc = ((m0 - key) % 12 + 12) % 12; m <= m1; m++, pc = pc == 11 ? 0 : pc + 1) {   // pc: pitch class above the key
            if (!((mask >> pc) & 1)) continue;
            const double d = std::abs(m - midi);
            if (d < bd) { bd = d; best = noteHz(m); }
        }
        return best;
    }
    // 440 * 2^((m - 69) / 12), from a table for the usual range (same values, no pow per call)
    static double noteHz(int m) {
        static const std::array<double, 256> t = [] { std::array<double, 256> a{}; for (int i = 0; i < 256; i++) a[(size_t)i] = 440.0 * std::pow(2.0, (i - 128 - 69) / 12.0); return a; }();
        return m >= -128 && m < 128 ? t[(size_t)(m + 128)] : 440.0 * std::pow(2.0, (m - 69) / 12.0);
    }

private:
    void buildWells() {
        lastKey = p.key; lastScale = p.scale;
        for (int k = 0; k < NB; k++) {
            const double f = std::max(1.0, k * sr / N);
            const double cents = 1200 * std::log2(f / nearestNote(f, p.key, p.scale));
            wellW[(size_t)k] = std::exp(-0.5 * (cents / 28.0) * (cents / 28.0));
        }
    }

    void frame(int nch) {
        if (p.key != lastKey || p.scale != lastScale) buildWells();
        const int kFloor = std::max(2, (int)std::ceil(p.floorHz * N / sr));
        const double expected = 2 * kPi * H / N;
        // analysis
        // both channels in one complex FFT (left in the real part, right in the imaginary), then separated:
        // X_L[k] = (Z[k] + conj Z[N-k]) / 2,  X_R[k] = (Z[k] - conj Z[N-k]) / 2i
        {
            auto& Z = Y[0];
            for (int i = 0; i < N; i++) Z[(size_t)i] = { inF[0][(size_t)i] * win[(size_t)i], inF[1][(size_t)i] * win[(size_t)i] };
            fft.transform(Z.data(), false);
            for (int k = 0; k < NB; k++) {
                const auto a = Z[(size_t)k], b = std::conj(Z[(size_t)((N - k) & (N - 1))]);
                X[0][(size_t)k] = { 0.5 * (a.real() + b.real()), 0.5 * (a.imag() + b.imag()) };
                X[1][(size_t)k] = { 0.5 * (a.imag() - b.imag()), -0.5 * (a.real() - b.real()) };
            }
        }
        double maxM = 0;
        for (int k = 0; k < NB; k++) {
            const auto mid = X[0][(size_t)k] + X[1][(size_t)k];
            midNow[(size_t)k] = mid;
            mag[(size_t)k] = std::sqrt(mid.real() * mid.real() + mid.imag() * mid.imag()); maxM = std::max(maxM, mag[(size_t)k]);
        }
        // true frequency from the phase advance since the last frame; only peaks need it, so only peaks pay the atan2s
        auto trueBinAt = [&](int k) {
            double d = std::arg(midNow[(size_t)k]) - std::arg(midPrev[(size_t)k]) - k * expected;
            d -= 2 * kPi * std::round(d / (2 * kPi));
            return k + d * N / (2 * kPi * H);
        };
        // peaks above the floor
        peakList.clear();
        const double gate = std::max(maxM * std::pow(10.0, -70.0 / 20), 1e-7 * N);
        for (int k = std::max(kFloor, 2); k < NB - 2; k++)
            if (mag[(size_t)k] > gate && mag[(size_t)k] > mag[(size_t)k - 1] && mag[(size_t)k] >= mag[(size_t)k + 1]) { peakList.push_back(k); trueBin[(size_t)k] = trueBinAt(k); }
        std::swap(midNow, midPrev);
        // regions: from the minimum before each peak to the minimum after it
        regionStart.assign(peakList.size(), kFloor); regionEnd.assign(peakList.size(), NB - 1);
        for (size_t i = 0; i + 1 < peakList.size(); i++) {
            int best = peakList[i];
            for (int k = peakList[i] + 1; k < peakList[i + 1]; k++) if (mag[(size_t)k] < mag[(size_t)best]) best = k;
            regionEnd[i] = best; regionStart[i + 1] = best + 1;
        }
        // drift: smooth each bin's log ratio toward its region's target
        const double alpha = p.driftMs <= 0.5 ? 1.0 : 1 - std::exp(-(double)H / (p.driftMs * 0.001 * sr));
        std::fill(regionTarget.begin(), regionTarget.end(), 0.0);
        for (size_t i = 0; i < peakList.size(); i++) {
            const double f = trueBin[(size_t)peakList[i]] * sr / N;
            const double target = f > 1 ? std::log(nearestNote(f, p.key, p.scale) / f) * p.pull : 0.0;
            for (int k = regionStart[i]; k <= regionEnd[i]; k++) regionTarget[(size_t)k] = target;
        }
        for (int k = 0; k < NB; k++) sLog[(size_t)k] += (regionTarget[(size_t)k] - sLog[(size_t)k]) * alpha;
        // synthesis spectrum
        for (int c = 0; c < nch; c++) {
            auto& Yc = Y[c]; auto& Xc = X[c];
            std::fill(Yc.begin(), Yc.end(), std::complex<double>{});
            for (int k = 0; k < std::min(kFloor, NB); k++) Yc[(size_t)k] = Xc[(size_t)k];
            for (size_t i = 0; i < peakList.size(); i++) {
                const int pk = peakList[i]; const double s = sLog[(size_t)pk];
                if (std::abs(s) < 1e-5) { for (int k = regionStart[i]; k <= regionEnd[i]; k++) Yc[(size_t)k] += Xc[(size_t)k]; continue; }
                const double r = std::exp(s), newBin = trueBin[(size_t)pk] * r;
                const int dst = (int)std::lround(newBin), shift = dst - pk;
                if (dst < 1 || dst >= NB - 1) continue;
                const double theta = synPhaseAt(c, dst) + newBin * expected;
                const double peakPh = std::arg(Xc[(size_t)pk]);
                // every bin in the region turns by the same angle (keeps its phase relative to the peak), so one
                // rotation per region replaces abs + arg + polar on every bin
                const double rc = std::cos(theta - peakPh), rs = std::sin(theta - peakPh);
                for (int k = regionStart[i]; k <= regionEnd[i]; k++) {
                    const int k2 = k + shift; if (k2 < kFloor || k2 >= NB) continue;
                    const auto x = Xc[(size_t)k];
                    Yc[(size_t)k2] += std::complex<double>(x.real() * rc - x.imag() * rs, x.real() * rs + x.imag() * rc);
                }
            }
            // bins not covered by any region (above the floor but below the first peak, etc.) pass through
            if (peakList.empty()) for (int k = kFloor; k < NB; k++) Yc[(size_t)k] = Xc[(size_t)k];
            else { for (int k = kFloor; k < regionStart[0]; k++) Yc[(size_t)k] += Xc[(size_t)k]; }
            // each bin's output phase for the next frame: arg(Y) where the bin has output, otherwise the old phase
            // advanced by one hop. The arg is only worked out when it is needed (a later read, or the bin going
            // silent), which gives exactly the same numbers with far fewer atan2 calls.
            for (int k = 0; k < NB; k++) {
                const auto y = Yc[(size_t)k];
                if (y.real() != 0 || y.imag() != 0) { synY[c][(size_t)k] = y; synPending[c][(size_t)k] = 1; }
                else {
                    if (synPending[c][(size_t)k]) { synPh[c][(size_t)k] = std::arg(synY[c][(size_t)k]); synPending[c][(size_t)k] = 0; }
                    synPh[c][(size_t)k] = synPh[c][(size_t)k] + k * expected;
                }
            }
        }
        // mass: gather energy into the wells, keeping the overall level
        if (p.mass > 0.001) {
            double eb = 0, ea = 0;
            for (int c = 0; c < nch; c++) for (int k = kFloor; k < NB; k++) {
                const double a = std::norm(Y[c][(size_t)k]); eb += a;
                const double w = (1 - p.mass) + p.mass * wellW[(size_t)k] * 2.0;
                Y[c][(size_t)k] *= w; ea += a * w * w;
            }
            const double g = ea > 1e-20 ? clampv(std::sqrt(eb / ea), 0.5, 2.0) : 1.0;
            massGain += (g - massGain) * 0.3;
            for (int c = 0; c < nch; c++) for (int k = kFloor; k < NB; k++) Y[c][(size_t)k] *= massGain;
        }
        // energy guard: shifted regions can land on the same bins and add up; never let a frame come out louder
        // than it went in (above the floor) by more than 1.5 dB
        {
            double ein = 0, eout = 0;
            for (int c = 0; c < nch; c++) for (int k = kFloor; k < NB; k++) { ein += std::norm(X[c][(size_t)k]); eout += std::norm(Y[c][(size_t)k]); }
            const double cap = ein * 1.41;
            if (eout > cap && eout > 1e-20) { const double g = std::sqrt(cap / eout); for (int c = 0; c < nch; c++) for (int k = kFloor; k < NB; k++) Y[c][(size_t)k] *= g; }
        }
        // inverse, window, overlap-add: both channels in one inverse FFT, W = Y_L + i Y_R (each conjugate-symmetric),
        // so the left output comes back in the real part and the right in the imaginary part
        for (int c = 0; c < 2; c++) { Y[c][0] = { Y[c][0].real(), 0 }; Y[c][(size_t)N / 2] = { Y[c][(size_t)N / 2].real(), 0 }; }
        {
            auto& W = Z2;
            for (int k = 0; k < NB; k++) {
                const auto l = Y[0][(size_t)k], r = Y[1][(size_t)k];
                W[(size_t)k] = { l.real() - r.imag(), l.imag() + r.real() };
                if (k > 0 && k < N / 2) W[(size_t)(N - k)] = { l.real() + r.imag(), -l.imag() + r.real() };
            }
            fft.transform(W.data(), true);
            for (int i = 0; i < N; i++) { Y[0][(size_t)i] = { W[(size_t)i].real(), 0 }; Y[1][(size_t)i] = { W[(size_t)i].imag(), 0 }; }
        }
        for (int c = 0; c < nch; c++) {
            auto& Yc = Y[c];
            for (int i = 0; i < N; i++) acc[c][(size_t)i] += Yc[(size_t)i].real() * win[(size_t)i] * olaScale;
            for (int i = 0; i < H; i++) outF[c][(size_t)i] = acc[c][(size_t)i];
            std::copy(acc[c].begin() + H, acc[c].begin() + H + N, acc[c].begin());
            std::fill(acc[c].begin() + N, acc[c].end(), 0.0);
            std::copy(inF[c].begin() + H, inF[c].end(), inF[c].begin());
        }
        // display data: the strongest peaks, where they came from and where they went
        int nv = 0;
        for (size_t i = 0; i < peakList.size() && nv < kMaxView; i++) {
            const int pk = peakList[i]; if (mag[(size_t)pk] < maxM * 0.01) continue;
            const double f = trueBin[(size_t)pk] * sr / N;
            view[(size_t)nv++] = { (float)f, (float)(f * std::exp(sLog[(size_t)pk])), (float)(mag[(size_t)pk] / std::max(maxM, 1e-12)) };
        }
        viewCount.store(nv, std::memory_order_release);
    }

    GravityParams p; double sr = 48000; int order = 11, N = 2048, H = 512, NB = 1025, rover = 1536, dpos = 0, lastKey = -1, lastScale = -1;
    Fft fft{ 11 }; std::vector<double> win; double olaScale = 1, massGain = 1;
    std::vector<double> inF[2], outF[2], acc[2];
    std::vector<std::complex<double>> X[2], Y[2], Z2;
    std::vector<float> dry[2];
    std::vector<std::complex<double>> midNow, midPrev, synY[2]; std::vector<double> synPh[2]; std::vector<uint8_t> synPending[2];
    double synPhaseAt(int c, int k) const { return synPending[c][(size_t)k] ? std::arg(synY[c][(size_t)k]) : synPh[c][(size_t)k]; }
    std::vector<double> sLog, mag, trueBin, wellW, regionTarget;
    std::vector<int> peakList, regionStart, regionEnd;
    std::array<PeakView, kMaxView> view{}; std::atomic<int> viewCount{ 0 };
    Smooth mixS;
};
} // namespace lgn
