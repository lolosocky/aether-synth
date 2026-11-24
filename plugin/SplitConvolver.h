// Reverb convolution that keeps the audio thread's load flat.
//
// The impulse response is split in two:
//   head  [0, 2P)   JUCE's zero-latency convolver (short, so cheap at any host block size)
//   tail  [2P, end) uniform partitions of P samples. All the work for one output block of the tail (the forward FFT of
//                   the newest input block, the multiply-accumulates for every partition, the inverse FFTs) is spread
//                   evenly over the P samples before that block plays. Because the tail starts 2P into the response,
//                   every input it needs is already complete when that work begins.
// Output equals plain convolution with the full response (within float rounding), with no added latency.
//
// Why: JUCE's convolver partitions the whole response at the host block size and runs every partition each block.
// Aether's reverbs are 5-15 s long, so at small buffers that cost a large, constant slice of a core per instance,
// and when a host prepared a large block but called with smaller ones, a whole partition's work landed in one callback.
//
// Threads: setImpulse() and collectGarbage() on the message thread; prepare(), reset() and process() on the audio side
// (prepare() must not run concurrently with process()).
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>

class SplitConvolver {
public:
    // sampleRate and the largest block process() will see. Allocates for responses up to maxSeconds.
    void prepare(double sampleRate, int maxBlock, double maxSeconds = 15.5) {
        const juce::ScopedLock cl(cfgLock);
        sr = sampleRate;
        P = sampleRate <= 50000 ? 2048 : sampleRate <= 100000 ? 4096 : 8192;
        N = 2 * P; NB = P + 1;
        fft = std::make_unique<juce::dsp::FFT>(juce::roundToInt(std::log2((double)N)));
        Kmax = (int)std::ceil(maxSeconds * sampleRate / P) + 1;
        for (int c = 0; c < 2; c++) {
            fdlRe[c].assign((size_t)Kmax * NB, 0.0f); fdlIm[c].assign((size_t)Kmax * NB, 0.0f);
            for (auto& b : in[c]) b.assign((size_t)P, 0.0f);
            for (auto& o : out[c]) o.assign((size_t)P, 0.0f);
            for (int j = 0; j < 2; j++) { accRe[j][c].assign((size_t)NB, 0.0f); accIm[j][c].assign((size_t)NB, 0.0f); }
            tailOut[c].assign((size_t)juce::jmax(1, maxBlock), 0.0f);
        }
        work.assign((size_t)N * 2, 0.0f); work2.assign((size_t)N * 2, 0.0f); fade.assign((size_t)P, 0.0f);
        for (int i = 0; i < P; i++) fade[(size_t)i] = (float)(0.5 - 0.5 * std::cos(juce::MathConstants<double>::pi * (i + 0.5) / P));
        // The head runs in slices of at most 256 samples: JUCE sizes its FFTs from the prepared block, so a host that
        // prepares 4096 and then calls with 256 would otherwise pay for 8192-point FFTs on every call.
        headBlock = juce::jlimit(1, 256, maxBlock);
        head.prepare({ sampleRate, (juce::uint32)headBlock, 2 });
        { const juce::SpinLock::ScopedLockType sl(lock); retired.reserve(16); }
        reset();
        for (int c = 0; c < 2; c++) { std::fill(fdlRe[c].begin(), fdlRe[c].end(), 0.0f); std::fill(fdlIm[c].begin(), fdlIm[c].end(), 0.0f); }
    }

    // Audio-thread safe: forgets the input history without touching the (large) delay line, which is only read
    // back as far as it has been refilled.
    void reset() {
        for (int c = 0; c < 2; c++) {
            for (auto& b : in[c]) std::fill(b.begin(), b.end(), 0.0f);
            for (auto& o : out[c]) std::fill(o.begin(), o.end(), 0.0f);
        }
        clearAcc();
        fdlNewest = 0; fdlCount = 0; pos = 0; unit = 0; wDone = 0; fill = 2; outCur = 0; firstJob = true;
        if (fadingOut) bury(fadingOut);
        head.reset();
    }

    // Message thread. Builds the tail partitions here; the audio thread picks them up at its next partition boundary
    // and crossfades from the previous response over one partition.
    void setImpulse(const std::vector<float>& L, const std::vector<float>& R) {
        const juce::ScopedLock cl(cfgLock);
        if (!fft) return;
        const int len = (int)juce::jmin(L.size(), R.size());
        const int headLen = juce::jmin(len, 2 * P);
        juce::AudioBuffer<float> hb(2, juce::jmax(1, headLen));
        hb.clear();
        if (headLen > 0) { hb.copyFrom(0, 0, L.data(), headLen); hb.copyFrom(1, 0, R.data(), headLen); }
        head.loadImpulseResponse(std::move(hb), sr, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);

        auto t = std::make_shared<Tail>();
        t->K = juce::jlimit(0, Kmax, (len - 2 * P + P - 1) / P);
        std::vector<float> w((size_t)N * 2);
        for (int c = 0; c < 2; c++) {
            const float* src = c ? R.data() : L.data();
            t->re[c].assign((size_t)t->K * NB, 0.0f); t->im[c].assign((size_t)t->K * NB, 0.0f);
            for (int k = 0; k < t->K; k++) {
                std::fill(w.begin(), w.end(), 0.0f);
                const int s0 = 2 * P + k * P, cnt = juce::jmin(P, len - s0);
                for (int i = 0; i < cnt; i++) w[(size_t)i] = src[s0 + i];
                fft->performRealOnlyForwardTransform(w.data(), true);
                float* gr = t->re[c].data() + (size_t)k * NB; float* gi = t->im[c].data() + (size_t)k * NB;
                for (int b = 0; b < NB; b++) { gr[b] = w[(size_t)2 * b]; gi[b] = w[(size_t)2 * b + 1]; }
            }
        }
        const juce::SpinLock::ScopedLockType sl(lock);
        pending = std::move(t);
    }

    // Message thread: frees responses the audio thread has finished with.
    void collectGarbage() { const juce::SpinLock::ScopedLockType sl(lock); retired.clear(); }

    // Audio thread. In place, stereo; n must not exceed the maxBlock given to prepare().
    void process(float* L, float* R, int n) {
        if (!fft || n <= 0) return;
        jassert(n <= (int)tailOut[0].size());
        float* io[2] = { L, R };
        for (int i = 0; i < n;) {
            const int m = juce::jmin(n - i, P - pos);
            for (int c = 0; c < 2; c++) {
                std::copy(io[c] + i, io[c] + i + m, in[c][fill].begin() + pos);
                std::copy(out[c][outCur].begin() + pos, out[c][outCur].begin() + pos + m, tailOut[c].begin() + i);
            }
            pos += m; i += m;
            work_until(pos == P ? 1.0 : (double)pos / P);
            if (pos == P) boundary();
        }
        for (int i = 0; i < n; i += headBlock) {
            float* ch[2] = { L + i, R + i };
            juce::dsp::AudioBlock<float> block(ch, 2, (size_t)juce::jmin(headBlock, n - i));
            head.process(juce::dsp::ProcessContextReplacing<float>(block));
        }
        for (int c = 0; c < 2; c++) juce::FloatVectorOperations::add(io[c], tailOut[c].data(), n);
    }

    int partition() const { return P; }

private:
    struct Tail { int K = 0; std::vector<float> re[2], im[2]; };
    static constexpr double kFftWeight = 6;   // one FFT channel costs roughly this many partition multiply-accumulates

    // A job is a list of units: [forward FFT L, forward FFT R, MAC partition 0..K-1, inverse FFT L, inverse FFT R].
    int jobK() const { return juce::jmax(active ? active->K : 0, fadingOut ? fadingOut->K : 0); }
    double weight(int u, int K) const { return (u < 2 || u >= K + 2) ? kFftWeight : 1.0; }

    void work_until(double frac) {
        const int K = jobK(), U = K + 4;
        const double total = 4 * kFftWeight + K;
        const double target = frac >= 1.0 ? 1e300 : total * frac;
        while (unit < U && wDone + weight(unit, K) <= target + 1e-9) { runUnit(unit, K); wDone += weight(unit, K); unit++; }
    }

    void runUnit(int u, int K) {
        if (u < 2) { forward(u); return; }
        if (u < K + 2) { mac(u - 2); return; }
        inverse(u - (K + 2));
    }

    // FFT of the two newest complete input blocks into the delay line (skipped before any input block is complete).
    void forward(int c) {
        if (firstJob) return;
        const int older = (fill + 1) % 3, newer = (fill + 2) % 3;
        float* w = work.data();
        std::copy(in[c][older].begin(), in[c][older].end(), w); std::copy(in[c][newer].begin(), in[c][newer].end(), w + P);
        std::fill(w + N, w + 2 * N, 0.0f);
        fft->performRealOnlyForwardTransform(w, true);
        float* xr = fdlRe[c].data() + (size_t)fdlNewest * NB; float* xi = fdlIm[c].data() + (size_t)fdlNewest * NB;
        for (int b = 0; b < NB; b++) { xr[b] = w[2 * b]; xi[b] = w[2 * b + 1]; }
    }

    void mac(int k) {
        if (k >= fdlCount) return;
        const int slot = (fdlNewest - k + Kmax) % Kmax;
        for (int j = 0; j < 2; j++) {
            const Tail* t = j == 0 ? active.get() : fadingOut.get();
            if (!t || k >= t->K) continue;
            for (int c = 0; c < 2; c++) {
                const float* xr = fdlRe[c].data() + (size_t)slot * NB; const float* xi = fdlIm[c].data() + (size_t)slot * NB;
                const float* gr = t->re[c].data() + (size_t)k * NB; const float* gi = t->im[c].data() + (size_t)k * NB;
                float* ar = accRe[j][c].data(); float* ai = accIm[j][c].data();
                for (int b = 0; b < NB; b++) { ar[b] += xr[b] * gr[b] - xi[b] * gi[b]; ai[b] += xr[b] * gi[b] + xi[b] * gr[b]; }
            }
        }
    }

    void toTime(int j, int c, float* dst) {
        float* w = work2.data(); const float* ar = accRe[j][c].data(); const float* ai = accIm[j][c].data();
        for (int b = 0; b < NB; b++) { w[2 * b] = ar[b]; w[2 * b + 1] = ai[b]; }
        for (int b = NB; b < N; b++) { w[2 * b] = ar[N - b]; w[2 * b + 1] = -ai[N - b]; }   // conjugate-symmetric upper half
        fft->performRealOnlyInverseTransform(w);
        std::copy(w + P, w + N, dst);   // overlap-save: the second half is the valid output
    }

    // Inverse FFT of the finished sum into the back output buffer; crossfades from a previous response if one is fading.
    void inverse(int c) {
        float* d = out[c][1 - outCur].data();
        if (active) toTime(0, c, d); else std::fill(d, d + P, 0.0f);
        if (fadingOut) {
            float* o = work.data();
            toTime(1, c, o);
            for (int i = 0; i < P; i++) d[i] = d[i] * fade[(size_t)i] + o[i] * (1.0f - fade[(size_t)i]);
        }
    }

    void boundary() {
        outCur = 1 - outCur;                 // the block computed during this partition plays during the next one
        if (fadingOut) bury(fadingOut);
        {
            const juce::SpinLock::ScopedTryLockType sl(lock);
            if (sl.isLocked()) {
                for (auto& g : graves) if (g && retired.size() < retired.capacity()) retired.push_back(std::move(g));
                if (pending) { if (active) fadingOut = std::move(active); active = std::move(pending); pending.reset(); }
            }
        }
        // the block just filled becomes the newest input; its spectrum is computed at the start of the next job
        fill = (fill + 1) % 3;
        fdlNewest = (fdlNewest + 1) % Kmax; fdlCount = juce::jmin(Kmax, fdlCount + 1);
        firstJob = false;
        clearAcc();
        pos = 0; unit = 0; wDone = 0;
    }

    void clearAcc() { for (int j = 0; j < 2; j++) for (int c = 0; c < 2; c++) { std::fill(accRe[j][c].begin(), accRe[j][c].end(), 0.0f); std::fill(accIm[j][c].begin(), accIm[j][c].end(), 0.0f); } }

    // Never free on the audio thread: park finished responses until the message thread can take them.
    void bury(std::shared_ptr<const Tail>& t) {
        for (auto& g : graves) if (!g) { g = std::move(t); break; }
        t.reset();   // only frees here with every grave full (eight response changes before one garbage collection)
    }

    double sr = 48000;
    int P = 2048, N = 4096, NB = 2049, Kmax = 1, headBlock = 256;
    std::unique_ptr<juce::dsp::FFT> fft;
    juce::dsp::Convolution head;
    // in: three rotating input blocks per channel (older, newer, filling); out: front (playing) and back (being built)
    std::vector<float> fdlRe[2], fdlIm[2], in[2][3], out[2][2], accRe[2][2], accIm[2][2], work, work2, fade, tailOut[2];
    int fdlNewest = 0, fdlCount = 0, pos = 0, unit = 0, fill = 2, outCur = 0; double wDone = 0; bool firstJob = true;
    std::shared_ptr<const Tail> active, fadingOut, pending, graves[8];
    std::vector<std::shared_ptr<const Tail>> retired;
    juce::SpinLock lock;
    juce::CriticalSection cfgLock;   // prepare() vs setImpulse(); never taken on the audio thread
};
