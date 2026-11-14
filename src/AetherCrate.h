// Crate Mode: the preset plays a short chord loop, the loop is "pressed to vinyl" (aged, band-limited, wobbly, crackly),
// then cut into chops that are laid across the keyboard like pads on a sampler.
#pragma once
#include "AetherEngine.h"

namespace aether {

struct CrateProg { const char* name; bool minor; std::vector<std::pair<int, std::vector<int>>> chords; };   // root offset from key, intervals

inline const std::vector<CrateProg>& crateProgs() {
    static const std::vector<CrateProg> v{
        { "Soul ii-V-I", false, { { 2, { 0, 3, 7, 10, 14 } }, { 7, { 0, 4, 10, 14, 21 } }, { 0, { 0, 4, 7, 11, 14 } }, { 9, { 0, 3, 7, 10, 14 } } } },
        { "Minor Drift", true, { { 0, { 0, 3, 7, 10, 14 } }, { 8, { 0, 4, 7, 11 } }, { 5, { 0, 3, 7, 10, 14 } }, { 7, { 0, 3, 7, 10 } } } },
        { "Gospel Turn", false, { { 0, { 0, 4, 7, 11, 14 } }, { 4, { 0, 3, 7, 10 } }, { 5, { 0, 4, 7, 11, 14 } }, { 5, { 0, 3, 7, 10, 14 } } } },
        { "Jazz Cycle", false, { { 4, { 0, 3, 7, 10 } }, { 9, { 0, 3, 7, 10, 14 } }, { 2, { 0, 3, 7, 10, 14 } }, { 7, { 0, 4, 10, 14, 21 } } } },
        { "Dusty Dorian", true, { { 0, { 0, 3, 7, 10, 14 } }, { 5, { 0, 4, 7, 10, 14 } }, { 0, { 0, 3, 7, 10, 14 } }, { 5, { 0, 4, 7, 10, 14 } } } },
        { "Neo Soul", false, { { 5, { 0, 4, 7, 11, 14 } }, { 4, { 0, 3, 7, 10 } }, { 9, { 0, 3, 7, 10, 14 } }, { 7, { 0, 5, 7, 10, 14 } } } },
        { "Boom Bap Minor", true, { { 0, { 0, 3, 7, 10 } }, { 10, { 0, 4, 7, 14 } }, { 8, { 0, 4, 7, 11 } }, { 7, { 0, 4, 7, 10, 13 } } } },
        { "Cinematic", true, { { 0, { 0, 3, 7, 12 } }, { 8, { 0, 4, 7, 12 } }, { 3, { 0, 4, 7, 12 } }, { 10, { 0, 4, 7, 12 } } } },
        { "Go-Go Vamp", true, { { 0, { 0, 3, 7, 10, 14 } }, { 0, { 0, 3, 7, 10, 14 } }, { 5, { 0, 4, 7, 10, 14 } }, { 5, { 0, 4, 7, 10, 14 } } } },
        { "Love Song", false, { { 0, { 0, 4, 7, 14 } }, { 9, { 0, 3, 7, 10 } }, { 5, { 0, 4, 7, 11 } }, { 7, { 0, 5, 7, 10 } } } } };
    return v;
}
inline const std::vector<std::string>& crateRhythms() { static const std::vector<std::string> v{ "Held", "Stabs", "Pushes", "Half-time" }; return v; }

struct CrateSettings { int prog = 0, key = 0, rhythm = 0, chops = 16; double bpm = 90, age = 0.5; uint32_t seed = 1; };
struct CrateBuffer { std::vector<float> L, R; double sr = 48000; int chops = 16, len = 0; std::vector<int> starts; };

inline std::shared_ptr<CrateBuffer> renderCrate(const Patch& patchIn, const Wavetable* const* wts, const Source* const* srcs, const CrateSettings& cs, double sr) {
    const auto& progs = crateProgs(); const CrateProg& pg = progs[(size_t)std::max(0, std::min((int)progs.size() - 1, cs.prog))];
    const double bpm = clampd(cs.bpm, 50, 200), beat = 60.0 / bpm, bar = beat * 4;
    const int bars = 4, N = (int)std::floor(bars * bar * sr + 0.5), tail = (int)std::floor(bar * sr + 0.5);
    // the instrument: the preset itself, with performance helpers off and enough polyphony for chords
    Assets::Bundle b; b.p = patchIn;
    b.p.arp.on = 0; b.p.chord.on = 0; b.p.scale.on = 0; b.p.g.vmode = 0; b.p.g.poly = std::max(b.p.g.poly, 10); b.p.g.glide = 0;
    for (int i = 0; i < 4; i++) { b.wt[i] = wts ? wts[i] : nullptr; b.src[i] = srcs ? srcs[i] : nullptr; }
    auto e = std::make_unique<Engine>(sr); e->setBundle(b, true); e->bpm = bpm; e->rng.seed((double)cs.seed * 7919 + 13);
    Rng hum; hum.seed((double)cs.seed * 104729 + 7);
    struct Ev { double t; bool on; int n; double v; };
    std::vector<Ev> ev;
    const int keyRoot = 48 + ((cs.key % 12) + 12) % 12;
    for (int k = 0; k < bars; k++) {
        const auto& ch = pg.chords[(size_t)k % pg.chords.size()];
        int root = keyRoot + ch.first; if (root > 57) root -= 12;
        std::vector<std::pair<double, double>> hits;   // start (beats), length (beats)
        switch (cs.rhythm) {
            case 1: hits = { { 0, 0.7 }, { 1.5, 0.45 }, { 3, 0.7 } }; break;
            case 2: hits = { { 0, 1.4 }, { 1.5, 0.4 }, { 2.5, 1.3 } }; break;
            case 3: hits = { { 0, 2.6 }, { 3.5, 0.4 } }; break;
            default: hits = { { 0, 3.85 } };
        }
        for (const auto& h : hits) {
            const double t0 = k * bar + h.first * beat;
            for (size_t j = 0; j < ch.second.size(); j++) {
                const int n = std::max(24, std::min(100, root + ch.second[j]));
                const double st = t0 + (j * 0.006 + hum.next() * 0.012) * (cs.rhythm == 0 ? 1.5 : 1), v = 0.62 + hum.next() * 0.2;
                ev.push_back({ st, true, n, v }); ev.push_back({ t0 + h.second * beat, false, n, 0 });
            }
            ev.push_back({ t0, true, root - 12, 0.7 }); ev.push_back({ t0 + h.second * beat, false, root - 12, 0 });
        }
    }
    std::stable_sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& c) { return a.t < c.t || (a.t == c.t && !a.on && c.on); });
    const int total = N + tail;
    std::vector<float> L((size_t)total), R((size_t)total);
    size_t ei = 0;
    for (int off = 0; off < total; off += CB) {
        const int m = std::min(CB, total - off); const double t = off / sr;
        while (ei < ev.size() && ev[ei].t <= t + (double)m / sr) { const Ev& x = ev[ei++]; if (x.on) e->noteIn(x.n, x.v); else e->noteOut(x.n); }
        e->process(L.data() + off, R.data() + off, m);
    }
    // fold the ringing tail back onto the start so the loop wraps cleanly
    for (int i = 0; i < tail && i < N; i++) { L[(size_t)i] += L[(size_t)(N + i)]; R[(size_t)i] += R[(size_t)(N + i)]; }
    L.resize((size_t)N); R.resize((size_t)N);

    // ---- press it to vinyl
    const double age = clampd(cs.age, 0, 1);
    auto onePole = [sr](double hz) { return 1 - std::exp(-TAU * std::min(hz, sr * 0.45) / sr); };
    const double hpA = onePole(35 + age * 120), lpHz = 17000 - age * 11500, lpA = onePole(lpHz);
    const double drive = 1 + age * 2.2, dnorm = 1 / std::tanh(drive);
    std::vector<float> dl(8192 * 2, 0.0f); int w = 0;
    double hl = 0, hr = 0, l1 = 0, l2 = 0, r1 = 0, r2 = 0, wow = hum.next(), flut = hum.next(), click = 0, clickR = 0;
    const double wowD = age * 0.0035 * sr, flutD = age * 0.0005 * sr, base = 0.004 * sr;
    for (int i = 0; i < N; i++) {
        double x = L[(size_t)i], y = R[(size_t)i];
        // narrower, like a record
        const double mid = (x + y) * 0.5, side = (x - y) * 0.5 * (1 - age * 0.55); x = mid + side; y = mid - side;
        // wow and flutter through a short modulated delay
        dl[(size_t)w * 2] = (float)x; dl[(size_t)w * 2 + 1] = (float)y;
        wow += 0.55 / sr; if (wow >= 1) wow -= 1; flut += 6.2 / sr; if (flut >= 1) flut -= 1;
        const double d = base + wowD * (1 + std::sin(TAU * wow)) + flutD * (1 + std::sin(TAU * flut));
        double rp = w - d; while (rp < 0) rp += 8192;
        const int i0 = (int)rp; const double f = rp - i0; const int ia = i0 & 8191, ib = (i0 + 1) & 8191;
        x = dl[(size_t)ia * 2] + f * (dl[(size_t)ib * 2] - dl[(size_t)ia * 2]); y = dl[(size_t)ia * 2 + 1] + f * (dl[(size_t)ib * 2 + 1] - dl[(size_t)ia * 2 + 1]);
        w = (w + 1) & 8191;
        // saturation, then the record's band limits
        x = std::tanh(x * drive) * dnorm; y = std::tanh(y * drive) * dnorm;
        hl += hpA * (x - hl); hr += hpA * (y - hr); x -= hl; y -= hr;
        l1 += lpA * (x - l1); l2 += lpA * (l1 - l2); r1 += lpA * (y - r1); r2 += lpA * (r1 - r2);
        x = l2; y = r2;
        // surface: hiss and crackle
        if (age > 0) {
            const double hiss = (hum.next() * 2 - 1) * age * 0.0035;
            if (hum.next() < age * 2.5 / sr * 40) { const double a = (hum.next() * 2 - 1) * age * 0.25; click += a; clickR += a * (0.5 + hum.next() * 0.5); }
            click *= 0.6; clickR *= 0.6;
            x += hiss + click; y += hiss + clickR;
        }
        L[(size_t)i] = (float)x; R[(size_t)i] = (float)y;
    }
    // level: peak to about -6 dBFS
    double pk = 0; for (int i = 0; i < N; i++) pk = std::max({ pk, (double)std::abs(L[(size_t)i]), (double)std::abs(R[(size_t)i]) });
    const double g = pk > 1e-6 ? 0.5 / pk : 1; for (int i = 0; i < N; i++) { L[(size_t)i] = (float)(L[(size_t)i] * g); R[(size_t)i] = (float)(R[(size_t)i] * g); }
    auto out = std::make_shared<CrateBuffer>();
    out->L = std::move(L); out->R = std::move(R); out->sr = sr; out->len = N; out->chops = cs.chops == 8 ? 8 : 16;
    for (int c = 0; c < out->chops; c++) out->starts.push_back((int)((long long)N * c / out->chops));
    out->starts.push_back(N);
    return out;
}

// Plays chops. Runs on the audio thread; no allocation.
struct CratePlayer {
    struct V { bool on = false, release = false, choked = false; int chop = 0, note = 0; double pos = 0, end = 0, gain = 0, fade = 1, step = 1; };
    V v[12];
    void trigger(const CrateBuffer& b, int chop, int note, double vel, double rate, bool choke) {
        if (choke) for (auto& x : v) if (x.on) x.choked = true;
        V* s = nullptr; for (auto& x : v) if (!x.on) { s = &x; break; }
        if (!s) { s = &v[0]; for (auto& x : v) if (x.pos - x.end > s->pos - s->end) s = &x; }
        s->on = true; s->release = false; s->choked = false; s->chop = chop; s->note = note; s->pos = b.starts[(size_t)chop]; s->end = b.starts[(size_t)chop + 1];
        s->gain = 0.25 + 0.75 * vel; s->fade = 0; s->step = rate;
    }
    void noteOff(int note) { for (auto& x : v) if (x.on && x.note == note) x.release = true; }
    void stopAll() { for (auto& x : v) x.on = false; }
    void render(const CrateBuffer& b, float* L, float* R, int n, double hostSr, bool oneShot) {
        const double ratio = b.sr / hostSr; const double fin = 1.0 / (0.002 * hostSr), fout = 1.0 / (0.006 * hostSr);
        for (auto& x : v) {
            if (!x.on) continue;
            const double inc = x.step * ratio;
            for (int i = 0; i < n; i++) {
                const bool ending = x.pos + 0.006 * b.sr * x.step >= x.end;
                const bool fadingOut = x.choked || ending || (x.release && !oneShot);
                if (fadingOut) { x.fade -= fout; if (x.fade <= 0) { x.on = false; break; } }
                else if (x.fade < 1) x.fade = std::min(1.0, x.fade + fin);
                const int i0 = (int)x.pos; if (i0 + 1 >= b.len) { x.on = false; break; }
                const double f = x.pos - i0, g = x.gain * x.fade;
                L[i] += (float)((b.L[(size_t)i0] + f * (b.L[(size_t)i0 + 1] - b.L[(size_t)i0])) * g);
                R[i] += (float)((b.R[(size_t)i0] + f * (b.R[(size_t)i0 + 1] - b.R[(size_t)i0])) * g);
                x.pos += inc;
            }
        }
    }
};

} // namespace aether
