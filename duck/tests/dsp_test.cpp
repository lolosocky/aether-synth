// Offline checks for the Duck engine: dips land on the beat, recover, follow the kick, never click or blow up.
#include "../Source/DuckDSP.h"
#include <cstdio>
#include <vector>
#include <cmath>
static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf("FAIL: %s\n", msg); ++fails; } else std::printf("ok:   %s\n", msg); } while (0)
int main() {
    const double sr = 48000, bpm = 120; const int B = 256, N = (int)(sr * 4);   // 4 s = 8 beats at 120
    // ---- Tempo mode, 1/4 notes, 100% amount ----
    lgn::Duck d; d.prepare(sr); d.amount = 1.0; d.release = 0.5; d.rateBeats = 1.0;
    std::vector<float> L(N, 1.0f), R(N, 1.0f);
    for (int o = 0; o < N; o += B) { float* ch[2] = { &L[o], &R[o] }; d.process(ch, 2, B, o * bpm / 60 / sr, bpm, true, nullptr, 0); }
    const int beat = (int)(sr * 60 / bpm);
    double atHit = L[2 * beat + (int)(0.003 * sr)], late = L[2 * beat + (int)(0.9 * beat)], minv = 1, maxStep = 0;
    for (int i = 1; i < N; ++i) { minv = std::min(minv, (double)L[i]); maxStep = std::max(maxStep, (double)std::abs(L[i] - L[i - 1])); }
    CHECK(atHit < 0.05, "tempo: volume is down right after the beat");
    CHECK(late > 0.99, "tempo: volume is back up before the next beat");
    CHECK(minv >= 0 && minv < 0.02, "tempo: dips all the way at 100% and never goes negative");
    CHECK(maxStep < 0.03, "tempo: no clicks (largest per-sample gain jump < 0.03)");
    // amount scales the dip
    lgn::Duck h; h.prepare(sr); h.amount = 0.5; h.rateBeats = 1.0; std::vector<float> H(B * 40, 1.0f);
    for (int o = 0; o < (int)H.size(); o += B) { float* ch[1] = { &H[o] }; h.process(ch, 1, B, o * bpm / 60 / sr, bpm, true, nullptr, 0); }
    double hmin = 1; for (float v : H) hmin = std::min(hmin, (double)v);
    CHECK(std::abs(hmin - 0.5) < 0.03, "tempo: 50% amount dips to half volume");
    // 1/8 rate pumps twice per beat
    lgn::Duck e; e.prepare(sr); e.amount = 1; e.rateBeats = 0.5; std::vector<float> E(N, 1.0f);
    for (int o = 0; o < N; o += B) { float* ch[1] = { &E[o] }; e.process(ch, 1, B, o * bpm / 60 / sr, bpm, true, nullptr, 0); }
    CHECK(E[beat + beat / 2 + (int)(0.003 * sr)] < 0.05, "tempo: 1/8 rate also dips on the off-beat");
    // stopped transport = no ducking
    lgn::Duck s; s.prepare(sr); std::vector<float> S(B * 20, 1.0f);
    for (int o = 0; o < (int)S.size(); o += B) { float* ch[1] = { &S[o] }; s.process(ch, 1, B, 0, bpm, false, nullptr, 0); }
    double smin = 1; for (float v : S) smin = std::min(smin, (double)v);
    CHECK(smin > 0.999, "tempo: does nothing while the song is stopped");
    // ---- Kick mode: kick hits at 0.5 s and 1.5 s ----
    lgn::Duck k; k.prepare(sr); k.mode = 1; k.amount = 1; k.release = 0.4;
    std::vector<float> K(N, 1.0f), sc(N, 0.0f);
    for (double t : {0.5, 1.5, 2.25}) for (int i = 0; i < (int)(0.12 * sr); ++i) { int j = (int)(t * sr) + i; sc[j] = (float)(0.8 * std::exp(-i / (0.03 * sr)) * std::sin(2 * M_PI * 55 * i / sr)); }
    for (int o = 0; o < N; o += B) { float* ch[1] = { &K[o] }; const float* s2[1] = { &sc[o] }; k.process(ch, 1, B, 0, bpm, true, s2, 1); }
    CHECK(K[(int)(0.25 * sr)] > 0.999, "kick: untouched before the first kick");
    CHECK(K[(int)(0.505 * sr)] < 0.1, "kick: dips when the kick hits");
    CHECK(K[(int)(1.2 * sr)] > 0.99, "kick: recovers between kicks");
    CHECK(K[(int)(1.505 * sr)] < 0.1 && K[(int)(2.255 * sr)] < 0.1, "kick: retriggers on every kick");
    // quiet kicks still trigger (self-setting threshold)
    lgn::Duck q; q.prepare(sr); q.mode = 1; q.amount = 1; std::vector<float> Q(N, 1.0f), qs(N, 0.0f);
    for (int i = 0; i < N; ++i) qs[i] = sc[i] * 0.02f;   // about -34 dB quieter
    for (int o = 0; o < N; o += B) { float* ch[1] = { &Q[o] }; const float* s2[1] = { &qs[o] }; q.process(ch, 1, B, 0, bpm, true, s2, 1); }
    CHECK(Q[(int)(0.505 * sr)] < 0.1, "kick: works with a quiet kick too");
    std::printf(fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
