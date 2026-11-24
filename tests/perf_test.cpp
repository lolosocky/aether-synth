// Stress test for every preset: CPU cost and audible glitches under real playing, not just a single held chord.
// Plays overlapping chords (long releases pile up voices) and fast repeated notes (strums, re-triggers), then reports
//   - average and worst-block CPU, as a fraction of real time
//   - voices cut off while still audible (a hard cut is a click)
//   - sample-to-sample jumps in the output that no note-on explains (crackle)
// Usage: aether_perf_test presets.json [first] [count] [block]
#include "../src/AetherPatch.h"
#include "../src/AetherPost.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#if defined(__SSE__) || defined(_M_X64)
#include <xmmintrin.h>
#endif
using namespace aether;
static std::string slurp(const std::string& p) { std::ifstream f(p, std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }

struct Ev { double t; int note; bool on; };

static std::vector<Ev> score() {
    std::vector<Ev> ev;
    // four 4-note chords, each held 1.2 s, next one starting before the last is released, played twice
    const int ch[4][4] = { { 48, 55, 60, 63 }, { 44, 51, 56, 60 }, { 46, 53, 58, 62 }, { 43, 50, 55, 59 } };
    double t = 0;
    for (int r = 0; r < 2; r++) for (int c = 0; c < 4; c++) {
        for (int k = 0; k < 4; k++) { ev.push_back({ t + k * 0.012, ch[c][k], true }); ev.push_back({ t + 1.25, ch[c][k], false }); }
        t += 1.2;
    }
    // fast repeated notes and strums (16ths at 140 bpm), two voices moving
    const double s16 = 60.0 / 140 / 4;
    for (int i = 0; i < 24; i++) {
        const double tt = t + i * s16; const int n = (i % 8 < 4) ? 60 : 64;
        ev.push_back({ tt, n, true }); ev.push_back({ tt + s16 * 0.8, n, false });
        if (i % 4 == 0) for (int k = 0; k < 3; k++) { ev.push_back({ tt + k * 0.008, 48 + k * 7, true }); ev.push_back({ tt + s16 * 3.5, 48 + k * 7, false }); }
    }
    t += 24 * s16 + 1.5;
    ev.push_back({ t, -1, false });  // end marker
    std::stable_sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.t < b.t; });
    return ev;
}

static double voiceLevel(const Voice& v, const Patch& P) {
    double m = 0;
    for (int i = 0; i < 4; i++) if (!v.lay[i].done && P.L[i].on) m = std::max(m, v.lay[i].aenv.v * P.L[i].lvl);
    return v.fading ? 0 : m;
}

int main(int argc, char** argv) {
#if defined(__SSE__) || defined(_M_X64)
    _mm_setcsr(_mm_getcsr() | 0x8040);  // flush denormals, as the plugin does
#endif
    const json lib = json::parse(slurp(argc > 1 ? argv[1] : "tests/ref/presets.json"));
    const auto& presets = lib["presets"];
    const size_t first = argc > 2 ? (size_t)std::atoi(argv[2]) : 0, count = argc > 3 ? (size_t)std::atoi(argv[3]) : presets.size();
    const int B = argc > 4 ? std::atoi(argv[4]) : 256; const double SR = 48000;
    const auto ev = score();
    std::cout << std::fixed << std::setprecision(4) << std::unitbuf;
    for (size_t idx = first; idx < std::min(presets.size(), first + count); idx++) {
        Assets::Bundle b; b.p = patchFromJson(presets[idx]); Assets::get().fill(b);
        auto e = std::make_unique<Engine>(SR); e->setBundle(b, true); e->rng.seed(1000.0 + (double)idx);
        PostFX post; post.prepare(SR);
        const int N = (int)(ev.back().t * SR);
        std::vector<float> L((size_t)B), R((size_t)B), sL((size_t)B), sR((size_t)B);
        std::vector<double> blocks; size_t ei = 0; int steals = 0, fades = 0, glitches = 0, maxVoices = 0; double worst = 0, total = 0, quietFrom = 0;
        double prev[2][2] = { { 0, 0 }, { 0, 0 } }, d2rms = 1e-6; int lastOn = -100000;
        for (int off = 0; off < N; off += B) {
            const int n = std::min(B, N - off);
            const auto t0 = std::chrono::steady_clock::now();
            int done = 0;
            while (ei < ev.size() && ev[ei].note >= 0 && ev[ei].t * SR < off + n) {
                const int at = std::max(done, (int)(ev[ei].t * SR) - off);
                if (at > done) { e->process(L.data() + done, R.data() + done, at - done); done = at; }
                double before[POOL]; int notes[POOL]; bool wasFading[POOL];
                for (int v = 0; v < POOL; v++) { before[v] = e->voices[v].active ? voiceLevel(e->voices[v], e->patch) : 0; notes[v] = e->voices[v].note; wasFading[v] = e->voices[v].fading; }
                if (ev[ei].on) { e->noteIn(ev[ei].note, 0.8); lastOn = off + at; } else e->noteOut(ev[ei].note);
                for (int v = 0; v < POOL; v++) {
                    const Voice& x = e->voices[v];
                    const bool cut = before[v] > 0.02 && (!x.active || (x.age == 0 && x.note != notes[v]));
                    if (cut && e->patch.g.vmode == 0) steals++;
                    else if (before[v] > 0.05 && !wasFading[v] && x.fading && e->patch.g.vmode == 0) fades++;
                }
                ei++;
            }
            if (n > done) e->process(L.data() + done, R.data() + done, n - done);
            post.stage1(e->patch, e->fxm, e->bpm, L.data(), R.data(), sL.data(), sR.data(), n);
            post.stage2(e->patch, e->fxm, 0.8, L.data(), R.data(), sL.data(), sR.data(), n);
            const double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            if (const char* dump = std::getenv("AETHER_DUMP")) {   // raw float32 stereo, for listening or analysis
                static FILE* f = nullptr; static size_t which = (size_t)-1;
                if (which != idx) { if (f) std::fclose(f); f = std::fopen((std::string(dump) + "_" + std::to_string(idx) + ".f32").c_str(), "wb"); which = idx; }
                for (int i = 0; i < n; i++) { const float fr[2] = { L[(size_t)i], R[(size_t)i] }; std::fwrite(fr, 4, 2, f); }
            }
            total += dt; worst = std::max(worst, dt / (n / SR)); blocks.push_back(dt / (n / SR));
            maxVoices = std::max(maxVoices, e->activeVoices());
            // crackle detector: second difference far above its recent level, away from note starts
            for (int i = 0; i < n; i++) {
                const float* ch[2] = { L.data(), R.data() };
                for (int c = 0; c < 2; c++) {
                    const double x = ch[c][i], d2 = std::abs(x - 2 * prev[c][0] + prev[c][1]);
                    if (off + i - lastOn > (int)(0.004 * SR) && d2 > 0.03 && d2 > 12 * d2rms && off + i > quietFrom) { glitches++; quietFrom = off + i + SR * 0.01; }
                    d2rms += (d2 - d2rms) * 0.002;
                    prev[c][1] = prev[c][0]; prev[c][0] = x;
                }
            }
        }
        const double rt = total / (N / SR);
        std::sort(blocks.begin(), blocks.end()); const double p99 = blocks[(size_t)(blocks.size() * 0.99)];
        std::cout << idx << "\t" << presets[idx]["cat"].get<std::string>() << "\t" << presets[idx]["name"].get<std::string>() << "\tavg " << rt << "\tp99 " << p99 << "\tworst " << worst
                  << "\tvoices " << maxVoices << "\tsteals " << steals << "\tfadecuts " << fades << "\tglitches " << glitches << "\n";
    }
    return 0;
}
