// Song Sections: every preset carries four voicings of itself (Intro, Verse, Hook, Outro).
// A section is a set of offsets applied on top of the patch; moving between sections morphs smoothly.
#pragma once
#include "AetherEngine.h"
#include "../third_party/json.hpp"

namespace aether {

struct SectionState {
    double cut = 0, res = 0, rev = 0, dly = 0, wid = 1, pocket = 0, gain = 0, drive = 0, rel = 1;
    double lvl[4] = { 1, 1, 1, 1 };
};

inline const char* sectionName(int i) { static const char* n[4] = { "Intro", "Verse", "Hook", "Outro" }; return n[std::max(0, std::min(3, i))]; }

// Musical defaults by category: verses leave a pocket for vocals, hooks open up, intros and outros are filtered and roomy.
inline nlohmann::json defaultSections(const std::string& cat, const std::string& name) {
    const bool low = cat == "Synth Bass" || cat == "808 & Sub";
    const bool fx = cat == "Sweeps & FX" || cat == "Textures & Noise";
    const bool lead = cat == "Leads" || cat == "Brass & Winds";
    double h = 0; for (char c : name) h = h * 31 + (unsigned char)c; h = hash01(std::fmod(h, 1e9));
    auto st = [](double cut, double res, double rev, double dly, double wid, double pocket, double gain, double drive, double rel, std::array<double, 4> lvl) {
        return nlohmann::json{ { "cut", cut }, { "res", res }, { "rev", rev }, { "dly", dly }, { "wid", wid }, { "pocket", pocket }, { "gain", gain }, { "drive", drive }, { "rel", rel }, { "lvl", lvl } };
    };
    nlohmann::json s = nlohmann::json::array();
    if (low) {
        s.push_back(st(-0.14, 0, 0, 0, 1, 0, -3, 0, 1, { 1, 0.6, 0.6, 0.6 }));
        s.push_back(st(-0.03, 0, 0, 0, 1, 0, -0.5, 0, 1, { 1, 1, 1, 1 }));
        s.push_back(st(0.05, 0.05, 0, 0, 1, 0, 0.5, 0.12, 1, { 1, 1, 1, 1 }));
        s.push_back(st(-0.2, 0, 0.05, 0, 1, 0, -3, 0, 1.4, { 1, 0.5, 0.5, 0.5 }));
    } else if (fx) {
        s.push_back(st(-0.1, 0, 0.1, 0.05, 1.1, 0, -2, 0, 1.2, { 1, 0.8, 0.8, 0.8 }));
        s.push_back(st(-0.05, 0, -0.05, 0, 0.9, 0.5, -2, 0, 1, { 1, 1, 1, 1 }));
        s.push_back(st(0.05, 0, 0.05, 0.05, 1.3, 0, 0, 0, 1, { 1, 1, 1, 1 }));
        s.push_back(st(-0.18, 0, 0.2, 0.1, 1.2, 0, -3, 0, 1.6, { 1, 0.6, 0.6, 0.6 }));
    } else {
        const double v = 0.85 + 0.3 * h;   // a little variety between presets
        s.push_back(st(-0.17 * v, 0, 0.12, 0.06, 0.75, 0, -1.5, 0, 1.3, { 1, 0.55, 0.55, 0.55 }));
        s.push_back(st(-0.07 * v, 0, -0.06, -0.03, 0.9, lead ? 0.45 : 0.8, -1.5, 0, 0.9, { 1, 0.8, 0.8, 0.8 }));
        s.push_back(st(0.08 * v, 0.03, 0.04, 0.05, 1.3, 0, 0.5, 0.08, 1, { 1, 1, 1, 1 }));
        s.push_back(st(-0.24 * v, 0, 0.22, 0.1, 1.15, 0, -2.5, 0, 1.6, { 1, 0.6, 0.6, 0.6 }));
    }
    return s;
}

inline SectionState sectionFromJson(const nlohmann::json& j) {
    SectionState s;
    if (!j.is_object()) return s;
    auto num = [&j](const char* k, double d) { auto it = j.find(k); return (it != j.end() && it->is_number()) ? it->get<double>() : d; };
    s.cut = num("cut", 0); s.res = num("res", 0); s.rev = num("rev", 0); s.dly = num("dly", 0); s.wid = num("wid", 1);
    s.pocket = num("pocket", 0); s.gain = num("gain", 0); s.drive = num("drive", 0); s.rel = num("rel", 1);
    if (j.contains("lvl") && j["lvl"].is_array()) for (size_t i = 0; i < 4 && i < j["lvl"].size(); i++) if (j["lvl"][i].is_number()) s.lvl[i] = j["lvl"][i].get<double>();
    return s;
}

struct Sections {
    SectionState st[4];
    void load(const nlohmann::json& arr) { for (int i = 0; i < 4; i++) st[i] = (arr.is_array() && arr.size() > (size_t)i) ? sectionFromJson(arr[(size_t)i]) : SectionState(); }
    // pos in [0, 3]: whole numbers are pure sections, fractions blend neighbours
    SectionState at(double pos) const {
        pos = clampd(pos, 0, 3); const int a = std::min(2, (int)pos); const double f = pos - a;
        const SectionState& A = st[a]; const SectionState& B = st[a + 1]; SectionState o;
        auto mix = [f](double x, double y) { return x + (y - x) * f; };
        o.cut = mix(A.cut, B.cut); o.res = mix(A.res, B.res); o.rev = mix(A.rev, B.rev); o.dly = mix(A.dly, B.dly); o.wid = mix(A.wid, B.wid);
        o.pocket = mix(A.pocket, B.pocket); o.gain = mix(A.gain, B.gain); o.drive = mix(A.drive, B.drive); o.rel = mix(A.rel, B.rel);
        for (int i = 0; i < 4; i++) o.lvl[i] = mix(A.lvl[i], B.lvl[i]);
        return o;
    }
};

// Applies a section voicing to a live patch. Returns the vocal-pocket depth (0-1) and output gain in dB through the out params.
inline void applySection(Patch& p, const SectionState& s) {
    for (int i = 0; i < 4; i++) {
        Layer& L = p.L[i];
        L.f1c = clampd(L.f1c + s.cut, 0, 1); L.f2c = clampd(L.f2c + s.cut, 0, 1);
        L.f1r = clampd(L.f1r + s.res, 0, 1);
        L.lvl *= std::max(0.0, s.lvl[i]);
        L.ar *= std::max(0.05, s.rel);
    }
    if (std::abs(s.rev) > 1e-4) { const double m = clampd((p.fx.rev.on ? p.fx.rev.mix : 0) + s.rev, 0, 1); p.fx.rev.on = m > 0.0005; p.fx.rev.mix = m; }
    if (std::abs(s.dly) > 1e-4) { const double m = clampd((p.fx.dly.on ? p.fx.dly.mix : 0) + s.dly, 0, 1); p.fx.dly.on = m > 0.0005; p.fx.dly.mix = m; }
    p.fx.width = clampd(p.fx.width * s.wid, 0, 2);
    if (s.drive > 1e-4) { if (!p.fx.drv.on) { p.fx.drv.on = 1; p.fx.drv.amt = 0; p.fx.drv.type = 1; } p.fx.drv.amt = clampd(p.fx.drv.amt + s.drive, 0, 1); }
}

// "I8 V16 H8 O8" style arrangement: returns the section index for a bar position. Bars past the end repeat the last
// two entries (verse/hook alternate) unless the map ends with an outro, which then holds.
struct Arrangement {
    std::vector<std::pair<int, int>> parts;   // section, bars
    int total = 0;
    static Arrangement parse(const std::string& text) {
        Arrangement a; std::string tok;
        auto flush = [&a](const std::string& t) {
            if (t.size() < 2) return;
            const char c = (char)std::toupper((unsigned char)t[0]); const int sec = c == 'I' ? 0 : c == 'V' ? 1 : c == 'H' || c == 'C' ? 2 : c == 'O' ? 3 : -1;
            const int bars = std::atoi(t.c_str() + 1);
            if (sec >= 0 && bars > 0 && bars < 1000) { a.parts.push_back({ sec, bars }); a.total += bars; }
        };
        for (char c : text) { if (std::isspace((unsigned char)c) || c == ',' || c == '-') { flush(tok); tok.clear(); } else tok += c; }
        flush(tok);
        return a;
    }
    int sectionAt(double bar) const {
        if (parts.empty()) return 1;
        if (bar < 0) bar = 0;
        if (bar >= total) {
            if (parts.back().first == 3 || parts.size() < 2) return parts.back().first;
            const auto& x = parts[parts.size() - 2]; const auto& y = parts.back();
            const double cyc = x.second + y.second, r = std::fmod(bar - total, cyc);
            return r < x.second ? x.first : y.first;
        }
        double acc = 0;
        for (const auto& p : parts) { acc += p.second; if (bar < acc) return p.first; }
        return parts.back().first;
    }
};

} // namespace aether
