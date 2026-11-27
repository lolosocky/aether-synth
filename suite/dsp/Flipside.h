// LGN Flipside: a tempo-synced time and volume shaper (the Gross Beat idea).
//
// Two curves repeat in time with the song:
//   Time curve:   how far back in time to play from, in beats (0 = live). A flat curve holds a delay, a rising
//                 curve slows the sound down (rising at 0.5 beat per beat = half speed), a falling one speeds it
//                 up, a jump repeats or skips. Stutters, tape stops, reverses, scratches and half-time all come
//                 from this one idea.
//   Volume curve: gain from 0 to 1 (gates, pumps, swells, mutes).
// Jumps in the time curve are crossfaded over 4 ms so they never click.
// The library is generated from families of musical moves, each in many sizes and lengths.
// Plain C++ (no JUCE) so it can be tested on its own.
#pragma once
#include "Common.h"
#include <string>
#include <functional>

namespace lgn {

// A curve over one cycle: points (x 0..1 across the cycle, y value). Two points at the same x make a jump.
struct FlipCurve {
    static constexpr int kMax = 256;
    std::array<float, kMax> x{}, y{}; int n = 0;
    float lengthBeats = 4;
    void clear() { n = 0; }
    void add(double px, double py) { if (n < kMax) { x[(size_t)n] = (float)clampv(px, 0.0, 1.0); y[(size_t)n] = (float)py; n++; } }
    // value at position p (0..1); `hint` speeds up the search for monotonic playback
    double eval(double p, int& hint) const {
        if (n == 0) return 0;
        if (p <= x[0]) return y[0];
        if (p >= x[(size_t)n - 1]) return y[(size_t)n - 1];
        int i = clampv(hint, 0, n - 2);
        if (x[(size_t)i] > p) i = 0;
        while (i < n - 2 && x[(size_t)i + 1] <= p) i++;
        hint = i;
        const double x0 = x[(size_t)i], x1 = x[(size_t)i + 1];
        if (x1 - x0 < 1e-9) return y[(size_t)i + 1];
        const double t = (p - x0) / (x1 - x0);
        return y[(size_t)i] + (y[(size_t)i + 1] - y[(size_t)i]) * t;
    }
};

struct FlipEntry { std::string category, name; FlipCurve curve; };

// ------------------------------------------------------------------ the library
class FlipBank {
public:
    std::vector<FlipEntry> time, vol;
    struct Combo { std::string category, name; int time, vol; };
    std::vector<Combo> combos;

    static const FlipBank& get() { static const FlipBank b; return b; }
    int findTime(const std::string& n) const { for (size_t i = 0; i < time.size(); i++) if (time[i].name == n) return (int)i; return 0; }
    int findVol(const std::string& n) const { for (size_t i = 0; i < vol.size(); i++) if (vol[i].name == n) return (int)i; return 0; }

private:
    using Fn = std::function<double(double /*beat within cycle*/)>;
    // Sample a function of beat position into a curve; jumps (big changes between samples) become exact steps
    // found by bisection, so stutters land precisely on the grid.
    static FlipCurve sample(double L, const Fn& f, int samples = 192, double jumpTol = 0.004) {
        FlipCurve c; c.lengthBeats = (float)L;
        double prevB = 0, prevY = f(0); c.add(0, prevY);
        for (int k = 1; k <= samples; k++) {
            const double b = L * k / samples, yv = f(std::min(b, L * 0.999999));
            const double mid = f(0.5 * (prevB + b)), lin = 0.5 * (prevY + yv);
            if (std::abs(mid - lin) > jumpTol * std::max(1.0, L / 4) || std::abs(yv - prevY) > 0.06 * std::max(1.0, L / 4)) {
                // find a discontinuity between prevB and b: each test point is compared with the straight line
                // coming from the left and the one coming from the right, and goes to the side it fits
                const double h = (b - prevB) * 1e-3;
                const double yL0 = prevY, sL = (f(prevB + h) - prevY) / h, yR1 = f(std::min(b, L * 0.999999) - 0.0), sR = (yR1 - f(b - h)) / h;
                double lo = prevB, hi = b;
                for (int it = 0; it < 40; it++) {
                    const double m = 0.5 * (lo + hi), ym = f(m);
                    const double eL = std::abs(ym - (yL0 + sL * (m - prevB))), eR = std::abs(ym - (yR1 - sR * (b - m)));
                    if (eL <= eR) lo = m; else hi = m;
                }
                const double fl = f(lo), fh = f(std::min(hi, L * 0.999999));
                if (std::abs(fh - fl) > 1e-4 && hi < b - 1e-12) { c.add(lo / L, fl); c.add(lo / L, fh); }
            }
            // a jump exactly on this sample point: put both sides of it here
            const double left = f(std::max(0.0, b - 1e-9 * L));
            if (std::abs(left - yv) > 1e-5 && b < L) c.add(b / L, left);
            c.add(b / L, yv);
            prevB = b; prevY = yv;
        }
        // drop points on straight lines (keeps curves editable and light)
        FlipCurve s; s.lengthBeats = c.lengthBeats;
        for (int i = 0; i < c.n; i++) {
            if (s.n >= 2 && i < c.n - 1) {
                const double x0 = s.x[(size_t)s.n - 2], y0 = s.y[(size_t)s.n - 2], x1 = s.x[(size_t)s.n - 1], y1 = s.y[(size_t)s.n - 1];
                const double x2 = c.x[(size_t)i], y2 = c.y[(size_t)i];
                if (x1 - x0 > 1e-9 && x2 - x1 > 1e-9) {
                    const double pred = y0 + (y1 - y0) * (x2 - x0) / (x1 - x0);
                    if (std::abs(pred - y2) < 1e-4) { s.x[(size_t)s.n - 1] = (float)x2; s.y[(size_t)s.n - 1] = (float)y2; continue; }
                }
            }
            s.add(c.x[(size_t)i], c.y[(size_t)i]);
        }
        return s;
    }
    void T(const std::string& cat, const std::string& name, double L, const Fn& f) { time.push_back({ cat, name, sample(L, [f](double b) { return std::max(0.0, f(b)); }) }); }
    void V(const std::string& cat, const std::string& name, double L, const Fn& f) { vol.push_back({ cat, name, sample(L, [f](double b) { return clampv(f(b), 0.0, 1.0); }, 256, 0.02) }); }

    static double fr(double v) { return v - std::floor(v); }
    static std::string len(double b) {
        // lengths in beats, named as note values
        static const std::pair<double, const char*> names[] = { { 0.0625, "1/64" }, { 0.125, "1/32" }, { 0.25, "1/16" }, { 0.5, "1/8" }, { 1, "1/4" }, { 2, "1/2" },
                                                                 { 4, "1 Bar" }, { 8, "2 Bars" }, { 1.0 / 6, "1/16T" }, { 1.0 / 3, "1/8T" }, { 2.0 / 3, "1/4T" }, { 0.75, "1/8D" }, { 1.5, "1/4D" }, { 3, "3/4 Bar" } };
        for (auto& nm : names) if (std::abs(b - nm.first) < 1e-9) return nm.second;
        return std::to_string(b);
    }
    static uint32_t hash(uint32_t x) { x ^= x >> 16; x *= 0x7feb352d; x ^= x >> 15; x *= 0x846ca68b; x ^= x >> 16; return x; }

    // ---- gate shapes for volume steps
    static double gateShape(int shape, double t /*0..1 within step*/) {
        switch (shape) {
            case 0: return t < 0.92 ? 1.0 : (1 - t) / 0.08;                        // hard (short release)
            case 1: return std::min(1.0, t / 0.18) * (t < 0.7 ? 1.0 : (1 - t) / 0.3); // soft
            default: return std::exp(-t * 4.0);                                     // pluck
        }
    }
    void gate(const std::string& cat, const std::string& name, const std::string& pat, double step, int shape) {
        const int N = (int)pat.size(); const double L = N * step;
        V(cat, name, L, [pat, step, shape, N](double b) {
            const int k = std::min(N - 1, (int)(b / step)); const double t = b / step - k;
            if (pat[(size_t)k] == 'x') return gateShape(shape, t);
            if (pat[(size_t)k] == 'o') return 0.45 * gateShape(shape, t);   // ghost
            if (pat[(size_t)k] == '-') return 1.0;                          // tie (keeps the previous note open)
            return 0.0; });
    }

    FlipBank() { buildTime(); buildVolume(); buildCombos(); }

    void buildTime() {
        // --- Slow Down: half/quarter/two-thirds speed over a cycle, then catch up at the jump
        for (double L : { 1.0, 2.0, 4.0, 8.0 })
            for (auto sp : std::vector<std::pair<double, std::string>>{ { 0.5, "Half Speed" }, { 0.25, "Quarter Speed" }, { 2.0 / 3, "2/3 Speed" }, { 0.75, "3/4 Speed" } })
                T("Slow Down", sp.second + " " + len(L), L, [sp, L](double b) { (void)L; return (1 - sp.first) * b; });
        // --- Half-time halves: normal for the first half, half speed for the second (and the reverse)
        for (double L : { 2.0, 4.0, 8.0 }) {
            T("Slow Down", "Half-Time Second Half " + len(L), L, [L](double b) { return b < L / 2 ? 0.0 : 0.5 * (b - L / 2); });
            T("Slow Down", "Half-Time Last Beat " + len(L), L, [L](double b) { return b < L - 1 ? 0.0 : 0.5 * (b - (L - 1)); });
        }
        // --- Tape: stops, starts, brakes, power-downs
        for (double L : { 1.0, 2.0, 4.0, 8.0 })
            for (double f : { 0.25, 0.5, 1.0 }) {
                const double D = f * L, s0 = L - D;
                T("Tape", "Tape Stop " + len(L) + (f < 1 ? " (last " + std::to_string((int)(f * 100)) + "%)" : ""), L,
                  [s0, D](double b) { if (b < s0) return 0.0; const double t = b - s0; return t * t / (2 * D); });
            }
        for (double L : { 1.0, 2.0, 4.0 }) {
            // speed ramps from 0 up to normal: read position tau^2/2L, so the delay is tau - tau^2/2L
            T("Tape", "Tape Start " + len(L), L, [L](double b) { return b - b * b / (2 * L); });
            // speed (1 - t)^3: a fast vinyl brake
            T("Tape", "Vinyl Brake " + len(L), L, [L](double b) { const double t = b / L; return b - L / 4 * (1 - std::pow(1 - t, 4)); });
            // speed e^(-3t): a power-down that never quite stops
            T("Tape", "Power Down " + len(L), L, [L](double b) { return b - L / 3 * (1 - std::exp(-3 * b / L)); });
            // stop over the first half, start again over the second
            T("Tape", "Stop + Restart " + len(L), L, [L](double b) { const double h = L / 2; if (b < h) return b * b / (2 * h); const double t = b - h; return h / 2 + t - t * t / (2 * h); });
        }
        // --- Stutter: repeat the first part of each segment
        for (double seg : { 0.25, 0.5, 1.0, 2.0, 4.0 })
            for (int k : { 2, 4, 8 }) {
                const double r = seg / k; if (r < 0.0625) continue;
                T("Stutter", "Stutter " + len(r) + " in " + len(seg), std::max(seg, 1.0), [seg, r](double b) { const double t = std::fmod(b, seg); return t - std::fmod(t, r); });
            }
        for (double r : { 0.0625, 0.125, 0.25, 0.5, 1.0 / 6, 1.0 / 3 }) {
            T("Stutter", "Last Beat Stutter " + len(r), 4, [r](double b) { if (b < 3) return 0.0; const double t = b - 3; return t - std::fmod(t, r); });
            T("Stutter", "Last Half Stutter " + len(r), 4, [r](double b) { if (b < 2) return 0.0; const double t = b - 2; return t - std::fmod(t, r); });
        }
        // --- Rolls: repeats that get faster across the cycle (snare-roll style)
        for (double L : { 2.0, 4.0, 8.0 })
            for (int steps : { 3, 4, 5 }) {
                T("Rolls", "Roll Build x" + std::to_string(steps) + " " + len(L), L, [L, steps](double b) {
                    const int st = std::min(steps - 1, (int)(b / L * steps)); const double r = 1.0 / std::pow(2.0, st), s0 = L * st / steps, t = b - s0;
                    return t - std::fmod(t, r); });
                T("Rolls", "Roll Down x" + std::to_string(steps) + " " + len(L), L, [L, steps](double b) {
                    const int st = std::min(steps - 1, (int)(b / L * steps)); const double r = 1.0 / std::pow(2.0, steps - 1 - st), s0 = L * st / steps, t = b - s0;
                    return t - std::fmod(t, r); });
            }
        for (double r : { 0.125, 1.0 / 6, 0.25 }) T("Rolls", "Steady Roll " + len(r), 1, [r](double b) { return b - std::fmod(b, r); });
        // --- Reverse: play each segment backwards (from its start, going back)
        for (double s : { 0.125, 0.25, 0.5, 1.0, 2.0, 4.0 }) {
            const double L = std::max(1.0, s * 2);
            T("Reverse", "Reverse " + len(s), std::max(1.0, s), [s](double b) { return 2 * std::fmod(b, s); });
            T("Reverse", "Reverse Every Other " + len(s), L, [s](double b) { const int k = (int)(b / s); return k % 2 ? 2 * std::fmod(b, s) : 0.0; });
        }
        for (double s : { 0.25, 0.5, 1.0 }) {
            T("Reverse", "Reverse Last Beat (" + len(s) + " pieces)", 4, [s](double b) { return b < 3 ? 0.0 : 2 * std::fmod(b - 3, s); });
            T("Reverse", "Reverse Fill 2 Bars (" + len(s) + ")", 8, [s](double b) { return b < 6 ? 0.0 : 2 * std::fmod(b - 6, s); });
        }
        T("Reverse", "Backspin 1 Beat", 4, [](double b) { if (b < 3) return 0.0; const double t = b - 3; return t + t * t * 1.6; });
        T("Reverse", "Backspin 2 Beats", 4, [](double b) { if (b < 2) return 0.0; const double t = b - 2; return t + t * t * 0.9; });
        // --- Repeat: MPC-style beat repeats (play an earlier beat again)
        struct Rep { const char* name; double from, to, back; };
        for (auto r : std::vector<Rep>{ { "Repeat Beat 1 on 3", 2, 3, 2 }, { "Repeat Beat 2 on 4", 3, 4, 2 }, { "Repeat Beats 1-2 on 3-4", 2, 4, 2 }, { "Repeat Last 1/8", 3.5, 4, 0.5 },
                                          { "Repeat Beat 3 on 4", 3, 4, 1 }, { "Echo Back 1/8 on 4", 3, 4, 0.5 }, { "Echo Back 1/4 on 2+4", 1, 2, 1 }, { "Double Beat 1", 1, 2, 1 } })
            T("Repeat", r.name, 4, [r](double b) { return b >= r.from && b < r.to ? r.back : 0.0; });
        for (double d : { 0.25, 0.5, 0.75, 1.0 / 3, 2.0 / 3 }) {
            T("Repeat", "Offbeat Echo " + len(d), 2, [d](double b) { return std::fmod(b, 1.0) >= 0.5 ? d : 0.0; });
            T("Repeat", "Pushed Late " + len(d), 4, [d](double b) { (void)b; return d; });
        }
        T("Repeat", "Bar Repeat", 8, [](double b) { return b >= 4 ? 4.0 : 0.0; });
        T("Repeat", "Half Bar Repeat", 4, [](double b) { return b >= 2 ? 2.0 : 0.0; });
        // --- Scratch: back-and-forth movement
        for (double P : { 0.25, 0.5, 1.0 })
            for (double A : { 0.125, 0.25, 0.5 }) {
                if (A > P * 0.5) continue;   // deeper than half the period is ~13x speed: noise, not a scratch
                T("Scratch", "Scratch " + len(P) + " depth " + len(A), std::max(1.0, P), [P, A](double b) { return A * (1 - std::cos(2 * kPi * std::fmod(b, P) / P)); });
            }
        T("Scratch", "Baby Scratch", 1, [](double b) { const double t = std::fmod(b, 0.5) / 0.5; return t < 0.5 ? t * 0.5 : (1 - t) * 0.5; });
        T("Scratch", "Chirp", 1, [](double b) { const double t = std::fmod(b, 0.5) / 0.5; return t < 0.3 ? t * 1.2 : std::max(0.0, 0.36 - (t - 0.3) * 0.5); });
        T("Scratch", "Tear", 2, [](double b) { const double t = std::fmod(b, 1.0); return t < 0.25 ? t * 2 : t < 0.5 ? 0.5 - (t - 0.25) * 1.2 : std::max(0.0, 0.2 + (t - 0.5) * 0.3); });
        T("Scratch", "Rewind 1 Bar", 4, [](double b) { return b < 3 ? 0.0 : (b - 3) * (b - 3) * 2.2 + (b - 3); });
        T("Scratch", "Rewind + Snap", 4, [](double b) { return b < 2.5 ? 0.0 : b < 3.75 ? (b - 2.5) * 2.4 : 0.0; });
        // --- Groove: swing and late notes by repeating a sliver (the time-curve way of swinging)
        for (double step : { 0.5, 0.25 })
            for (int pct : { 54, 58, 62, 66, 70, 75 }) {
                const double off = (pct / 50.0 - 1.0) * step;
                T("Groove", "Swing " + std::to_string(pct) + "% " + len(step), 1, [step, off](double b) { return std::fmod(b, 2 * step) >= step ? off : 0.0; });
            }
        T("Groove", "Dilla Drag", 4, [](double b) { const double s = std::fmod(b, 1.0); return (s >= 0.25 && s < 0.5 ? 0.06 : s >= 0.75 ? 0.08 : 0.0) + (b >= 1 && b < 2 ? 0.05 : 0) + (b >= 3 ? 0.06 : 0); });
        T("Groove", "Lazy Snares", 4, [](double b) { return (b >= 1 && b < 2) || b >= 3 ? 0.09 : 0.0; });
        T("Groove", "Slow Then Rush", 4, [](double b) { return b < 2 ? 0.08 * b : std::max(0.0, 0.16 - 0.08 * (b - 2)); });
        T("Groove", "Stumble", 4, [](double b) { return b >= 1.5 && b < 2 ? 0.12 : b >= 3.25 && b < 3.5 ? 0.1 : 0.0; });
        T("Groove", "Drunk Bar", 4, [](double b) { return 0.05 + 0.05 * std::sin(2 * kPi * b / 4) + 0.03 * std::sin(2 * kPi * b * 1.37); });
        T("Groove", "Behind the Beat", 4, [](double b) { (void)b; return 0.06; });
        // --- Pitch: wobble, octave down/up moves
        for (double rate : { 0.5, 1.0, 2.0, 4.0 })
            for (double depth : { 0.01, 0.03 })
                T("Pitch", std::string(depth < 0.02 ? "Wow " : "Seasick ") + len(rate), std::max(1.0, rate), [rate, depth](double b) { return depth * (1 - std::cos(2 * kPi * b / rate)) * 2; });
        for (double s : { 0.5, 1.0, 2.0 }) {
            T("Pitch", "Octave Down " + len(s), std::max(1.0, s), [s](double b) { return 0.5 * std::fmod(b, s); });
            T("Pitch", "Octave Up " + len(s), std::max(1.0, s), [s](double b) { return s - std::fmod(b, s); });
        }
        T("Pitch", "Octave Down Beat 4", 4, [](double b) { return b < 3 ? 0.0 : 0.5 * (b - 3); });
        T("Pitch", "Fifth Down Bar", 4, [](double b) { return b / 3.0; });
        T("Pitch", "Dive Last 1/8", 4, [](double b) { return b < 3.5 ? 0.0 : (b - 3.5) * (b - 3.5) * 2; });
        T("Pitch", "Slow Slide", 8, [](double b) { return b * b / 64.0 * 1.5; });
        // --- Freeze: a tiny piece held (granular freeze)
        for (double r : { 0.0625, 0.125, 0.25 }) {
            T("Freeze", "Freeze " + len(r), 4, [r](double b) { return b - std::fmod(b, r); });   // the bar's first grain, held
            T("Freeze", "Freeze Last Beat " + len(r), 4, [r](double b) { return b < 3 ? 0.0 : (b - 3) - std::fmod(b - 3, r); });
            T("Freeze", "Freeze Beat 2+4 " + len(r), 4, [r](double b) { const double t = std::fmod(b, 2.0); return t < 1 ? 0.0 : (t - 1) - std::fmod(t - 1, r); });
        }
        // --- Trap & Rage
        T("Trap & Rage", "Opium Drop (last bar half)", 8, [](double b) { return b < 4 ? 0.0 : 0.5 * (b - 4); });
        T("Trap & Rage", "Rage 1/32 Rolls", 4, [](double b) { const double s = std::fmod(b, 2.0); return s < 1.5 ? 0.0 : (s - 1.5) - std::fmod(s - 1.5, 0.125); });
        T("Trap & Rage", "Triplet Stutter Fill", 4, [](double b) { return b < 3 ? 0.0 : (b - 3) - std::fmod(b - 3, 1.0 / 6); });
        T("Trap & Rage", "808 Slide Down", 4, [](double b) { return b < 3 ? 0.0 : (b - 3) * (b - 3) * 0.4; });
        T("Trap & Rage", "Quarter Speed Drop", 4, [](double b) { return b < 2 ? 0.0 : 0.75 * (b - 2); });
        T("Trap & Rage", "Stutter + Reverse", 4, [](double b) { if (b < 2) return 0.0; if (b < 3) { const double t = b - 2; return t - std::fmod(t, 0.25); } return 2 * std::fmod(b - 3, 1.0); });
        T("Trap & Rage", "Hi-Hat Roll 1/16T", 2, [](double b) { return b < 1 ? 0.0 : (b - 1) - std::fmod(b - 1, 1.0 / 6); });
        T("Trap & Rage", "Pluggnb Bounce", 4, [](double b) { const double t = std::fmod(b, 1.0); return t >= 0.75 ? 0.25 : 0.0; });
        T("Trap & Rage", "Dark Half-Time 2 Bars", 8, [](double b) { return 0.5 * b; });
        T("Trap & Rage", "Glitch Gate Stutter", 4, [](double b) { const int k = (int)(b / 0.25); return (k % 3 == 2) ? std::fmod(b, 0.25) + 0.25 : 0.0; });
        T("Trap & Rage", "Rage Rewind", 4, [](double b) { return b < 3.5 ? 0.0 : (b - 3.5) * 3; });
        T("Trap & Rage", "Detroit Skip", 4, [](double b) { const double t = std::fmod(b, 1.0); return t >= 0.5 && t < 0.75 ? 0.5 : 0.0; });
        // --- Glitch: seeded random patterns per 1/16 (repeat, reverse, freeze, skip back)
        for (int seed = 1; seed <= 160; seed++) {
            const double L = seed % 4 == 0 ? 8.0 : 4.0, g = seed > 80 ? 0.125 : 0.25;   // 1/16 grid, then 1/32
            T("Glitch", "Glitch " + std::to_string(seed) + (g < 0.2 ? " 1/32" : "") + (L > 4 ? " (2 Bars)" : ""), L, [seed, g](double b) {
                const int k = (int)(b / g); const double t = std::fmod(b, g);
                const uint32_t h = hash((uint32_t)(seed * 977 + k * 131));
                switch (h % 9) {
                    case 0: return 2 * t;                                // reverse
                    case 1: return t - std::fmod(t, g / 4);              // tiny stutter
                    case 2: return t;                                    // freeze start of step
                    case 3: return g;                                    // repeat previous step
                    case 4: return 0.5 * t;                              // half speed
                    default: return 0.0;                                 // normal
                } });
        }
    }

    void buildVolume() {
        // --- Gates: named rhythms (x = hit, o = ghost, - = hold, . = off), 16 steps of 1/16 unless noted
        struct Pat { const char* cat; const char* name; const char* p; double step; };
        const std::vector<Pat> pats = {
            { "Basic Gates", "1/4 Notes", "x...x...x...x...", 0.25 }, { "Basic Gates", "1/8 Notes", "x.x.x.x.x.x.x.x.", 0.25 }, { "Basic Gates", "1/16 Notes", "xxxxxxxxxxxxxxxx", 0.25 },
            { "Basic Gates", "Offbeats", "..x...x...x...x.", 0.25 }, { "Basic Gates", "Off 16ths", ".x.x.x.x.x.x.x.x", 0.25 }, { "Basic Gates", "Dotted 8ths", "x..x..x..x..x..x", 0.25 },
            { "Basic Gates", "Dotted Quarter", "x.....x.....x...", 0.25 }, { "Basic Gates", "Half Notes", "x-------x-------", 0.25 }, { "Basic Gates", "Whole + Cut", "x-------------..", 0.25 },
            { "Basic Gates", "Gallop", "x.xxx.xxx.xxx.xx", 0.25 }, { "Basic Gates", "Reverse Gallop", "xx.xxx.xxx.xxx.x", 0.25 }, { "Basic Gates", "Triplet 8ths", "xxxxxxxxxxxx", 1.0 / 3 },
            { "Basic Gates", "Triplet 16ths", "xxxxxxxxxxxxxxxxxxxxxxxx", 1.0 / 6 }, { "Basic Gates", "Triplet Skip", "x.xx.xx.xx.x", 1.0 / 3 }, { "Basic Gates", "Every Third 16th", "x..x..x..x..x..x", 0.25 },
            { "Basic Gates", "Long-Short", "x-.xx-.xx-.xx-.x", 0.25 },
            { "Genre Rhythms", "Tresillo", "x..x..x.x..x..x.", 0.25 }, { "Genre Rhythms", "Son Clave", "x..x..x...x.x...", 0.25 }, { "Genre Rhythms", "Rumba Clave", "x..x...x..x.x...", 0.25 },
            { "Genre Rhythms", "Dembow", "x..x..x.x..x..x.", 0.25 }, { "Genre Rhythms", "Reggaeton Snare", "...x..x....x..x.", 0.25 }, { "Genre Rhythms", "Amapiano Log", "x..x..x...x..x.x", 0.25 },
            { "Genre Rhythms", "Jersey Club", "x.x.x...x.x.x...", 0.25 }, { "Genre Rhythms", "Jersey Kicks", "x..x..x.x.x.x...", 0.25 }, { "Genre Rhythms", "UK Drill", "x.....x...x..x..", 0.25 },
            { "Genre Rhythms", "NY Drill Slide", "x..x....x.x..x..", 0.25 }, { "Genre Rhythms", "Afrobeats", "x..x..x...x.x...", 0.25 }, { "Genre Rhythms", "Afro Shaker", "xoxxoxxoxoxxoxxo", 0.25 },
            { "Genre Rhythms", "Bossa Nova", "x..x..x...x..x..", 0.25 }, { "Genre Rhythms", "3-3-2", "x..x..x.x..x..x.", 0.25 }, { "Genre Rhythms", "Baile Funk", "x..x..x.x.x..x..", 0.25 },
            { "Genre Rhythms", "Moombahton", "x..x..x.x..x..x.", 0.25 }, { "Genre Rhythms", "Footwork", "x..x..x...x..x..x..x..x...x..x..", 0.25 }, { "Genre Rhythms", "Garage 2-Step", "x......x..x.....", 0.25 },
            { "Genre Rhythms", "Dancehall", "x..x..x.x..x..x.", 0.25 }, { "Genre Rhythms", "Trap Hats", "x.x.x.x.xxx.x.xx", 0.25 }, { "Genre Rhythms", "Trap Hat Rolls", "x.x.x.x.xxxxxxxx", 0.25 },
            { "Genre Rhythms", "Boom Bap Kick", "x......xx.x.....", 0.25 }, { "Genre Rhythms", "Boom Bap Snare", "....x.......x...", 0.25 }, { "Genre Rhythms", "Funky Drummer", "x.x...x..x.x..x.", 0.25 },
            { "Genre Rhythms", "Go-Go Swing", "x..x.x..x..x.x..", 0.25 }, { "Genre Rhythms", "Phonk Cowbell", "x.xx.x.xx.x.x.xx", 0.25 }, { "Genre Rhythms", "Memphis", "x..x..x...x..x..", 0.25 },
            { "Genre Rhythms", "Rage Synth", "x.x.xx.x.x.xx.x.", 0.25 }, { "Genre Rhythms", "Plugg Chords", "x-.x-.x-x-.x-.x-", 0.25 }, { "Genre Rhythms", "Detroit Bounce", "x..xx..x..xx.x..", 0.25 },
            { "Genre Rhythms", "Baltimore Club", "x...x...x.x.x...", 0.25 }, { "Genre Rhythms", "Hyperpop Stutter", "xxxx..xx.xxx..xx", 0.25 }, { "Genre Rhythms", "House Offbeat", "..x...x...x...x.", 0.25 },
            { "Genre Rhythms", "Techno Gate", "xx.xxx.xxx.xxx.x", 0.25 }, { "Genre Rhythms", "DnB Two-Step", "x.........x.....", 0.125 }, { "Genre Rhythms", "Breakbeat", "x.x...x..xx...x.", 0.25 },
            { "Genre Rhythms", "Lo-Fi Shuffle", "x..x.x..x..x.x..", 0.25 }, { "Genre Rhythms", "Neo-Soul Chops", "x.-x..x.-.x-..x.", 0.25 }, { "Genre Rhythms", "Gqom", "x..x..x...x..x..", 0.25 },
            { "Chops", "Chop 1+3", "x---....x---....", 0.25 }, { "Chops", "Chop 2+4", "....x---....x---", 0.25 }, { "Chops", "Stab 1/8 Ends", "x.......x......x", 0.25 },
            { "Chops", "Sample Chop A", "x-..x-x-..x-x...", 0.25 }, { "Chops", "Sample Chop B", "x--.x.x--.x.x--.", 0.25 }, { "Chops", "Sample Chop C", "x.x--.x.x.x--.x.", 0.25 },
            { "Chops", "MPC Chop 1", "x--x--x-x--x--x-", 0.25 }, { "Chops", "MPC Chop 2", "x-.x-.x.x-.x-...", 0.25 }, { "Chops", "MPC Chop 3", "x---x-x-x---x-.x", 0.25 },
            { "Chops", "Vocal Chop", "x.x.x---x.x.x---", 0.25 }, { "Chops", "Soul Flip", "x--x--x---x-x---", 0.25 }, { "Chops", "Stutter Ends", "x-------x---xxxx", 0.25 },
            { "Chops", "Call + Response", "x---x---........", 0.25 }, { "Chops", "Breath", "x-----------....", 0.25 }, { "Chops", "Dropouts", "x-------..x-----", 0.25 },
            { "Chops", "Morse", "x.x.xxx.xxx.x.x.", 0.25 },
        };
        for (auto& pt : pats) {
            gate(pt.cat, pt.name, pt.p, pt.step, 0);
            gate(pt.cat, std::string(pt.name) + " (Soft)", pt.p, pt.step, 1);
            gate(pt.cat, std::string(pt.name) + " (Pluck)", pt.p, pt.step, 2);
        }
        // --- Euclidean rhythms: k hits spread evenly over n steps, plus a rotation
        for (int n : { 8, 16 })
            for (int k = 3; k < n - 1; k++) {
                if (n == 8 && k > 6) continue;
                std::string p((size_t)n, '.'); for (int i = 0; i < n; i++) if ((i * k) % n < k) p[(size_t)i] = 'x';
                const std::string nm = "E(" + std::to_string(k) + "," + std::to_string(n) + ")";
                gate("Euclidean", nm, p, n == 8 ? 0.5 : 0.25, 0);
                std::string rot = p.substr(1) + p.substr(0, 1);
                gate("Euclidean", nm + " Shifted", rot, n == 8 ? 0.5 : 0.25, 1);
            }
        // --- Sidechain pumps
        for (double d : { 0.25, 0.5, 1.0, 2.0, 4.0 })
            for (double depth : { 0.5, 0.75, 1.0 }) {
                V("Pump", "Pump " + len(d) + " " + std::to_string((int)(depth * 100)) + "%", std::max(1.0, d), [d, depth](double b) { const double t = std::fmod(b, d) / d; return 1 - depth * std::pow(1 - std::min(1.0, t / 0.7), 2.2); });
                V("Pump", "Hard Pump " + len(d) + " " + std::to_string((int)(depth * 100)) + "%", std::max(1.0, d), [d, depth](double b) { const double t = std::fmod(b, d) / d; return 1 - depth * (t < 0.12 ? 1.0 : std::max(0.0, 1 - (t - 0.12) / 0.4)); });
            }
        // --- Tremolo
        struct Shape { const char* name; std::function<double(double)> f; };
        const std::vector<Shape> shapes = {
            { "Sine", [](double t) { return 0.5 + 0.5 * std::cos(2 * kPi * t); } }, { "Triangle", [](double t) { return std::abs(1 - 2 * t); } },
            { "Square", [](double t) { return t < 0.5 ? 1.0 : 0.0; } }, { "Saw Down", [](double t) { return 1 - t; } }, { "Saw Up", [](double t) { return t; } },
            { "Pulse 25%", [](double t) { return t < 0.25 ? 1.0 : 0.0; } } };
        for (auto& sh : shapes)
            for (double d : { 1.0 / 6, 0.25, 1.0 / 3, 0.5, 1.0 }) {
                auto f = sh.f;
                V("Tremolo", std::string(sh.name) + " " + len(d), std::max(1.0, d), [f, d](double b) { return 0.15 + 0.85 * f(std::fmod(b, d) / d); });
            }
        // --- Swells and fades
        for (double L : { 1.0, 2.0, 4.0, 8.0 }) {
            V("Swells", "Swell " + len(L), L, [L](double b) { const double t = b / L; return t * t; });
            V("Swells", "Reverse Swell " + len(L), L, [L](double b) { const double t = b / L; return t < 0.92 ? std::pow(t / 0.92, 2.5) : 0.0; });
            V("Swells", "Fade Out " + len(L), L, [L](double b) { const double t = b / L; return std::pow(1 - t, 1.6); });
            V("Swells", "Breathe " + len(L), L, [L](double b) { const double t = b / L; return 0.25 + 0.75 * std::sin(kPi * t); });
            V("Swells", "Duck + Recover " + len(L), L, [L](double b) { const double t = b / L; return std::min(1.0, t * 1.4); });
        }
        // --- Builds
        V("Builds", "Gate Build 2 Bars", 8, [](double b) { const int st = (int)(b / 2); const double r = 1.0 / std::pow(2.0, st); return std::fmod(b, r) < r * 0.5 ? 1.0 : 0.0; });
        V("Builds", "Snare Roll Gate", 4, [](double b) { const double r = b < 2 ? 0.5 : b < 3 ? 0.25 : b < 3.5 ? 0.125 : 0.0625; return std::fmod(b, r) < r * 0.55 ? 0.4 + 0.6 * b / 4 : 0.0; });
        V("Builds", "Riser Gate", 8, [](double b) { return (std::fmod(b, 0.25) < 0.14 ? 1.0 : 0.0) * (0.2 + 0.8 * b / 8); });
        V("Builds", "Pre-Drop Gap", 4, [](double b) { return b < 3.5 ? 1.0 : 0.0; });
        V("Builds", "Pre-Drop Gap 1 Beat", 4, [](double b) { return b < 3 ? 1.0 : 0.0; });
        V("Builds", "Heartbeat", 4, [](double b) { const double t = std::fmod(b, 1.0); return std::exp(-t * 14) + 0.7 * std::exp(-std::max(0.0, t - 0.22) * 14) * (t > 0.22); });
        V("Builds", "Long Duck", 8, [](double b) { return std::min(1.0, b / 7); });
        V("Builds", "Accelerating Pulse", 4, [](double b) { const double ph = b * b / 2; return std::fmod(ph, 1.0) < 0.5 ? 1.0 : 0.0; });
        V("Builds", "Slowing Pulse", 4, [](double b) { const double ph = 4 * b - b * b / 2; return std::fmod(ph, 1.0) < 0.5 ? 1.0 : 0.0; });
        V("Builds", "Tension Swell 2 Bars", 8, [](double b) { return 0.1 + 0.9 * std::pow(b / 8, 3); });
        // --- Mutes
        for (int beat = 1; beat <= 4; beat++) V("Mutes", "Mute Beat " + std::to_string(beat), 4, [beat](double b) { return (int)b == beat - 1 ? 0.0 : 1.0; });
        for (double d : { 0.125, 0.25, 0.5, 1.0, 2.0 }) V("Mutes", "Mute Last " + len(d), 4, [d](double b) { return b >= 4 - d ? 0.0 : 1.0; });
        V("Mutes", "Mute 2nd Bar", 8, [](double b) { return b >= 4 ? 0.0 : 1.0; });
        V("Mutes", "Mute Last 2 Beats of 2 Bars", 8, [](double b) { return b >= 6 ? 0.0 : 1.0; });
        V("Mutes", "Stop on 4", 4, [](double b) { return b >= 3 && b < 3.25 ? 1.0 : b >= 3 ? 0.0 : 1.0; });
        V("Mutes", "Hits on 1 Only", 4, [](double b) { return b < 0.5 ? 1.0 : 0.0; });
        V("Mutes", "Silence", 4, [](double) { return 0.0; });
        V("Mutes", "Full", 4, [](double) { return 1.0; });
        // --- Trance gates: 32-step patterns at 1/32 and 1/16
        for (int seed = 1; seed <= 60; seed++) {
            std::string p(32, '.'); for (int i = 0; i < 32; i++) p[(size_t)i] = (hash((uint32_t)(seed * 7919 + i * 31)) % 100) < 58 ? 'x' : '.';
            p[0] = 'x';
            gate("Trance Gates", "Trance Gate " + std::to_string(seed), p, seed % 2 ? 0.125 : 0.25, seed % 3 == 0 ? 1 : 0);
        }
        // --- Random: seeded gates with ghosts and ties
        for (int seed = 1; seed <= 150; seed++) {
            std::string p(16, '.');
            for (int i = 0; i < 16; i++) { const uint32_t h = hash((uint32_t)(seed * 104729 + i * 7)) % 100; p[(size_t)i] = h < 40 ? 'x' : h < 55 ? 'o' : h < 70 ? '-' : '.'; }
            p[0] = 'x';
            gate("Random", "Random " + std::to_string(seed), p, seed % 5 == 0 ? 0.125 : 0.25, (int)(seed % 3));
        }
    }

    void buildCombos() {
        // Named pairings for the main preset menu
        struct C { const char* cat; const char* name; const char* t; const char* v; };
        const std::vector<C> named = {
            { "Signature", "Off (Clean)", "Pushed Late 1/16", "Full" },
            { "Signature", "Half-Time Melody", "Half Speed 1 Bar", "Full" },
            { "Signature", "Opium Drop", "Opium Drop (last bar half)", "Pre-Drop Gap" },
            { "Signature", "Tape Stop Ending", "Tape Stop 1 Bar (last 50%)", "Full" },
            { "Signature", "Rage Stutter Lead", "Rage 1/32 Rolls", "Rage Synth" },
            { "Signature", "Dilla Chop", "Dilla Drag", "Soul Flip" },
            { "Signature", "Boom Bap Sample Flip", "Pushed Late 1/16", "MPC Chop 1" },
            { "Signature", "Pluggnb Bounce", "Pluggnb Bounce", "Plugg Chords" },
            { "Signature", "808 Slide Bar", "808 Slide Down", "Full" },
            { "Signature", "Backspin Into Drop", "Backspin 1 Beat", "Pre-Drop Gap 1 Beat" },
            { "Signature", "Vinyl Brake + Fade", "Vinyl Brake 1 Bar", "Fade Out 1 Bar" },
            { "Signature", "Detroit Skip Chops", "Detroit Skip", "Detroit Bounce" },
            { "Signature", "Jersey Club Chops", "Stutter 1/16 in 1/4", "Jersey Club" },
            { "Signature", "Baltimore Club Bounce", "Offbeat Echo 1/8", "Baltimore Club" },
            { "Signature", "Drill Slide Melody", "808 Slide Down", "UK Drill (Soft)" },
            { "Signature", "Reverse Swell Fill", "Reverse Last Beat (1/4 pieces)", "Reverse Swell 1/4" },
            { "Signature", "Snare Roll Build", "Roll Build x4 1 Bar", "Snare Roll Gate" },
            { "Signature", "Pump + Half-Time", "Half-Time Second Half 1 Bar", "Pump 1/4 75%" },
            { "Signature", "Scratch Hook", "Baby Scratch", "Full" },
            { "Signature", "Rewind Snap", "Rewind + Snap", "Full" },
            { "Signature", "Freeze Fill", "Freeze Last Beat 1/16", "Fade Out 1/4" },
            { "Signature", "Seasick Tape Loop", "Seasick 1/2", "Breathe 1 Bar" },
            { "Signature", "Phonk Cowbell Gate", "Pushed Late 1/32", "Phonk Cowbell" },
            { "Signature", "Amapiano Log Drum", "Pushed Late 1/32", "Amapiano Log (Pluck)" },
            { "Signature", "Afro Shaker Melody", "Swing 58% 1/16", "Afro Shaker (Soft)" },
        };
        for (auto& c : named) combos.push_back({ c.cat, c.name, findTime(c.t), findVol(c.v) });
        // Systematic pairings: every time move with a matching volume feel, so each family has ready-made combos
        const std::vector<std::pair<std::string, std::string>> partner = {
            { "Slow Down", "Full" }, { "Tape", "Full" }, { "Stutter", "1/16 Notes (Soft)" }, { "Rolls", "Snare Roll Gate" }, { "Reverse", "Swell 1/4" },
            { "Repeat", "MPC Chop 2" }, { "Scratch", "Off 16ths (Soft)" }, { "Groove", "Lo-Fi Shuffle (Soft)" }, { "Pitch", "Breathe 1 Bar" },
            { "Freeze", "Fade Out 1/4" }, { "Trap & Rage", "Trap Hats" }, { "Glitch", "Random 1" } };
        for (auto& pp : partner) {
            const int v = findVol(pp.second); int count = 0;
            for (size_t i = 0; i < time.size(); i++)
                if (time[i].category == pp.first) { combos.push_back({ "With " + pp.first, time[i].name + " + " + vol[(size_t)v].name, (int)i, v }); count++; }
        }
        // Lab: 300 seeded random pairings to dig through
        for (int k = 1; k <= 300; k++) {
            const int ti = (int)(hash((uint32_t)k * 2654435761u) % time.size());
            int vi = (int)(hash((uint32_t)k * 40503u + 17) % vol.size());
            if (vol[(size_t)vi].name == "Silence") vi = findVol("Full");
            char nm[16]; std::snprintf(nm, sizeof nm, "Lab %03d", k);
            combos.push_back({ "Lab", nm, ti, vi });
        }
        // and every volume pattern on its own with the time curve clean
        const int clean = findTime("Pushed Late 1/16");
        for (size_t i = 0; i < vol.size(); i++)
            if (vol[i].category == "Genre Rhythms" || vol[i].category == "Chops")
                if (vol[i].name.find('(') == std::string::npos) combos.push_back({ "Gates Only: " + vol[i].category, vol[i].name, clean, (int)i });
    }
};

// ------------------------------------------------------------------ the engine
struct FlipParams { double timeMix = 1, volAmount = 1, mix = 1, smoothMs = 2, rate = 1, outDb = 0; bool timeOn = true, volOn = true; };

class FlipsideDsp {
public:
    static constexpr double kMaxBack = 8.0;   // seconds of audio kept for the time curve
    void prepare(double rate) {
        sr = rate; X = std::max(32, (int)(0.004 * sr));
        for (auto& b : buf) b.assign((size_t)(sr * kMaxBack) + 16, 0.0f);
        w = 0; dPrev = 0; xfLeft = 0; gS = 1; hintT = hintV = 0;
        outS.setTime(sr, 0.02); outS.reset(dbToGain(p.outDb));
    }
    void set(const FlipParams& np) { p = np; }
    void setCurves(const FlipCurve& t, const FlipCurve& v) { timeC = t; volC = v; }   // call from the audio thread
    void reset() { for (auto& b : buf) std::fill(b.begin(), b.end(), 0.0f); dPrev = 0; xfLeft = 0; }

    // ppq = song position at the first sample; when the song is stopped the curves rest at their start
    void process(float* const* ch, int nch, int n, double ppq, double bpm, bool playing) {
        nch = std::min(nch, 2);
        const int size = (int)buf[0].size();
        const double spb = 60.0 / std::max(20.0, bpm) * sr, ppqPerSample = 1.0 / spb;
        const double gA = 1 - std::exp(-1.0 / (std::max(0.05, p.smoothMs) * 0.001 * sr));
        const double og = dbToGain(p.outDb);
        for (int i = 0; i < n; i++) {
            const float inL = ch[0][i], inR = nch > 1 ? ch[1][i] : inL;
            buf[0][(size_t)w] = inL; buf[1][(size_t)w] = inR;
            double d = 0, g = 1;
            if (playing) {
                const double q = (ppq + i * ppqPerSample) * p.rate;
                if (p.timeOn && timeC.n > 0) { const double L = timeC.lengthBeats; d = timeC.eval(frac(q / L), hintT) * spb / p.rate; }
                if (p.volOn && volC.n > 0) { const double L = volC.lengthBeats; g = volC.eval(frac(q / L), hintV); }
            }
            d = clampv(d, 0.0, (double)size - 8);
            // a jump: fade from where we were reading (continuing at normal speed) to the new spot
            if (std::abs(d - dPrev) > 24) { xfFrom = dPrev; xfLeft = X; }
            double yL, yR;
            if (xfLeft > 0) {
                // raised-cosine fade (gains sum to 1): no level bump when both heads carry similar audio
                const double t = 1.0 - (double)xfLeft / X, b = 0.5 - 0.5 * std::cos(t * kPi), a = 1 - b;
                yL = read(0, xfFrom) * a + read(0, d) * b; yR = read(1, xfFrom) * a + read(1, d) * b;
                xfLeft--;
            } else { yL = read(0, d); yR = read(1, d); }
            dPrev = d;
            const double tm = p.timeOn ? p.timeMix : 0.0;
            yL = inL * (1 - tm) + yL * tm; yR = inR * (1 - tm) + yR * tm;
            const double gt = 1 - p.volAmount * (1 - g);
            gS += gA * (gt - gS);
            yL *= gS; yR *= gS;
            const double o = outS.next(og);
            ch[0][i] = (float)((inL * (1 - p.mix) + yL * p.mix) * o);
            if (nch > 1) ch[1][i] = (float)((inR * (1 - p.mix) + yR * p.mix) * o);
            if (++w >= size) w = 0;
        }
        lastGain = gS; lastDelayBeats = dPrev / spb;
    }
    double lastGain = 1, lastDelayBeats = 0;

private:
    static double frac(double v) { return v - std::floor(v); }
    double read(int c, double d) const {   // d samples ago (d >= 0), cubic interpolation
        const int size = (int)buf[0].size();
        double pos = w - d; while (pos < 0) pos += size;
        const int i = (int)pos; const double t = pos - i;
        auto at = [&](int k) { int j = i + k; while (j < 0) j += size; while (j >= size) j -= size; return (double)buf[(size_t)c][(size_t)j]; };
        const double y0 = at(-1), y1 = at(0), y2 = at(1 <= d ? 1 : 0), y3 = at(2 <= d ? 2 : 0);
        const double c0 = y1, c1 = 0.5 * (y2 - y0), c2 = y0 - 2.5 * y1 + 2 * y2 - 0.5 * y3, c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + c0;
    }
    FlipParams p; double sr = 48000; int X = 192;
    std::array<std::vector<float>, 2> buf; int w = 0;
    FlipCurve timeC, volC; int hintT = 0, hintV = 0;
    double dPrev = 0, xfFrom = 0, gS = 1; int xfLeft = 0;
    Smooth outS;
};
} // namespace lgn
