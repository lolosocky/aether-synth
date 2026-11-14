// Patch loading from JSON, plus the built-in sampled sources (rendered by the engine itself).
#pragma once
#include "AetherEngine.h"
#include "../third_party/json.hpp"
#include "AetherSourceData.h"

namespace aether {
using json = nlohmann::json;

inline double jnum(const json& o, const char* k, double def) {
    if (!o.is_object()) return def;
    auto it = o.find(k); if (it == o.end()) return def;
    if (it->is_number()) return it->get<double>();
    if (it->is_boolean()) return it->get<bool>() ? 1 : 0;
    return def;
}
inline int jint(const json& o, const char* k, int def) { return (int)std::lround(jnum(o, k, def)); }
inline int jflag(const json& o, const char* k, int def) { return jnum(o, k, def) != 0 ? 1 : 0; }
inline std::string jstr(const json& o, const char* k, const std::string& def) {
    if (!o.is_object()) return def;
    auto it = o.find(k); return (it != o.end() && it->is_string()) ? it->get<std::string>() : def;
}
// A named choice may arrive as its name or as an index.
inline int jenum(const json& o, const char* k, const std::vector<std::string>& names, int def) {
    if (!o.is_object()) return def;
    auto it = o.find(k); if (it == o.end()) return def;
    if (it->is_string()) { const int i = indexOf(names, it->get<std::string>()); return i < 0 ? def : i; }
    if (it->is_number()) return std::max(0, std::min((int)names.size() - 1, (int)it->get<double>()));
    return def;
}
inline double jdiv(const json& o, const char* k, double def) {
    if (!o.is_object()) return def;
    auto it = o.find(k); if (it == o.end()) return def;
    if (it->is_string()) return divBeats(it->get<std::string>());
    if (it->is_number()) return it->get<double>();
    return def;
}
inline const json& jobj(const json& o, const char* k) { static const json empty = json::object(); if (!o.is_object()) return empty; auto it = o.find(k); return (it != o.end() && it->is_object()) ? *it : empty; }

inline void layerFromJson(const json& j, Layer& L) {
    if (!j.is_object()) return;
#define N(f) L.f = jnum(j, #f, L.f);
#define I(f) L.f = jint(j, #f, L.f);
#define B(f) L.f = jflag(j, #f, L.f);
    B(on) L.mode = jenum(j, "mode", modeNames(), L.mode);
    N(lvl) N(pan) N(crs) N(fin) B(kt) N(vs)
    N(shp) N(pw) N(sync) L.wt = jenum(j, "wt", wtNames(), L.wt); N(pos) N(fm) N(fmr) N(ring) N(rngr) N(fold) N(sub) N(nz)
    I(uni) N(det) N(spr) N(drift)
    N(pdec) N(pdmp) N(ppos) N(pexc)
    { auto it = j.find("mdl"); if (it != j.end()) { if (it->is_string()) L.mdl = modelIndex(it->get<std::string>()); else if (it->is_number()) L.mdl = std::max(0, std::min((int)models().size() - 1, (int)it->get<double>())); } }
    N(mbri) N(mdec) N(mhard) N(minh) N(mbow)
    N(vow) N(fsh) N(brth) I(ntyp) N(ncol)
    { auto it = j.find("src"); if (it != j.end()) { if (it->is_string()) L.src = sourceIndex(it->get<std::string>()); else if (it->is_number()) L.src = (int)it->get<double>(); } }
    N(gpos) N(gspr) N(gsz) N(gden) N(gpit) N(grev) N(gscan) N(sst) B(sloop)
    L.f1t = jenum(j, "f1t", ftypeNames(), L.f1t); N(f1c) N(f1r) N(f1d) N(f1k) N(f1e) N(f1v)
    L.f2t = jenum(j, "f2t", ftypeNames(), L.f2t); N(f2c) N(f2r) N(f2d) N(f2k) N(f2e) N(f2v) B(frt) N(fbal)
    N(aa) N(ad) N(as) N(ar) N(fa) N(fd) N(fs) N(fr)
#undef N
#undef I
#undef B
}

// Fills a Patch from a (possibly partial) patch object; anything missing keeps its default.
inline Patch patchFromJson(const json& j) {
    Patch p;
    if (!j.is_object()) return p;
    if (j.contains("L") && j["L"].is_array()) { const auto& a = j["L"]; for (size_t i = 0; i < a.size() && i < 4; i++) layerFromJson(a[i], p.L[i]); }
    { const json& g = jobj(j, "g"); p.g.poly = jint(g, "poly", 8); p.g.vmode = jint(g, "vmode", 0); p.g.glide = jnum(g, "glide", 0); p.g.gmode = jint(g, "gmode", 0); p.g.bend = jnum(g, "bend", 2); p.g.vol = jnum(g, "vol", 0.8); p.g.trim = jnum(g, "trim", 1); }
    if (j.contains("me") && j["me"].is_array()) { const auto& a = j["me"]; for (size_t i = 0; i < a.size() && i < 2; i++) { ModEnvP& m = p.me[i]; m.a = jnum(a[i], "a", m.a); m.d = jnum(a[i], "d", m.d); m.s = jnum(a[i], "s", m.s); m.r = jnum(a[i], "r", m.r); m.loop = jflag(a[i], "loop", m.loop); } }
    if (j.contains("lfo") && j["lfo"].is_array()) { const auto& a = j["lfo"]; for (size_t i = 0; i < a.size() && i < 4; i++) { LfoP& l = p.lfo[i]; l.shp = jint(a[i], "shp", l.shp); l.rate = jnum(a[i], "rate", l.rate); l.sync = jflag(a[i], "sync", l.sync); l.div = jdiv(a[i], "div", l.div); l.fade = jnum(a[i], "fade", l.fade); l.rtg = jflag(a[i], "rtg", l.rtg); l.ph = jnum(a[i], "ph", l.ph); } }
    if (j.contains("mods") && j["mods"].is_array()) {
        for (const auto& m : j["mods"]) {
            if (!m.is_object() || p.nmods >= 24) continue;
            const double a = jnum(m, "a", 0); if (a == 0) continue;
            const int s = indexOf(srcIds(), jstr(m, "s", "")); if (s <= 0) continue;
            const int v = std::max(0, indexOf(srcIds(), jstr(m, "v", "")));
            const std::string d = jstr(m, "d", ""); const size_t dot = d.find('.'); if (dot == std::string::npos) continue;
            const std::string t = d.substr(0, dot), pn = d.substr(dot + 1);
            int tgt, pi;
            if (t == "G") { tgt = 5; pi = pn.size() == 3 && pn[0] == 'l' && pn[2] == 'r' ? pn[1] - '1' : -1; if (pi > 3) pi = -1; }
            else if (t == "FX") { tgt = 6; pi = indexOf(fxdstNames(), pn); }
            else { tgt = t == "*" ? 4 : (t.size() == 1 && t[0] >= 'A' && t[0] <= 'D' ? t[0] - 'A' : -1); pi = indexOf(ldstNames(), pn); }
            if (tgt < 0 || pi < 0) continue;
            p.mods[p.nmods++] = ModRoute{ s, v, a, tgt, pi };
        }
    }
    if (j.contains("mac") && j["mac"].is_array()) { const auto& a = j["mac"]; for (size_t i = 0; i < a.size() && i < 4; i++) p.mac[i] = a[i].is_number() ? a[i].get<double>() : jnum(a[i], "v", 0); }
    { const json& o = jobj(j, "orb"); p.orb.x = jnum(o, "x", 0); p.orb.y = jnum(o, "y", 0); p.orb.ch = jnum(o, "ch", 0); p.orb.sp = jnum(o, "sp", 0.3); }
    {
        const json& a = jobj(j, "arp"); p.arp.on = jflag(a, "on", 0); p.arp.mode = jenum(a, "mode", arpModes(), 0); p.arp.div = jdiv(a, "div", 0.25); p.arp.oct = jint(a, "oct", 1);
        p.arp.gate = jnum(a, "gate", 0.6); p.arp.swing = jnum(a, "swing", 0); p.arp.len = jint(a, "len", 16); p.arp.latch = jflag(a, "latch", 0);
        if (a.contains("vel") && a["vel"].is_array()) for (size_t i = 0; i < a["vel"].size() && i < 16; i++) if (a["vel"][i].is_number()) p.arp.vel[i] = a["vel"][i].get<double>();
        if (a.contains("tr") && a["tr"].is_array()) for (size_t i = 0; i < a["tr"].size() && i < 16; i++) if (a["tr"][i].is_number()) p.arp.tr[i] = (int)a["tr"][i].get<double>();
    }
    {
        const json& c = jobj(j, "chord"); p.chord.on = jflag(c, "on", 0); p.chord.spread = jint(c, "spread", 0);
        const std::string ct = jstr(c, "type", "Min7"); p.chord.type = 3; for (size_t i = 0; i < chords().size(); i++) if (ct == chords()[i].name) p.chord.type = (int)i;
        const json& s = jobj(j, "scale"); p.scale.on = jflag(s, "on", 0); p.scale.root = jint(s, "root", 0);
        const std::string stp = jstr(s, "type", "Minor"); p.scale.type = 1; for (size_t i = 0; i < scales().size(); i++) if (stp == scales()[i].name) p.scale.type = (int)i;
    }
    {
        const json& f = jobj(j, "fx");
        { const json& o = jobj(f, "drv"); auto& d = p.fx.drv; d.on = jflag(o, "on", d.on); d.type = jint(o, "type", d.type); d.amt = jnum(o, "amt", d.amt); d.tone = jnum(o, "tone", d.tone); d.mix = jnum(o, "mix", d.mix); }
        { const json& o = jobj(f, "lofi"); auto& d = p.fx.lofi; d.on = jflag(o, "on", d.on); d.bits = jnum(o, "bits", d.bits); d.rate = jnum(o, "rate", d.rate); d.wow = jnum(o, "wow", d.wow); d.flut = jnum(o, "flut", d.flut); d.noise = jnum(o, "noise", d.noise); d.tone = jnum(o, "tone", d.tone); d.mix = jnum(o, "mix", d.mix); }
        { const json& o = jobj(f, "gate"); auto& d = p.fx.gate; d.on = jflag(o, "on", d.on); d.div = jdiv(o, "div", d.div); d.depth = jnum(o, "depth", d.depth); d.smooth = jnum(o, "smooth", d.smooth);
          if (o.contains("pat") && o["pat"].is_array()) for (size_t i = 0; i < o["pat"].size() && i < 16; i++) { const auto& x = o["pat"][i]; d.pat[i] = x.is_number() ? (x.get<double>() != 0) : (x.is_boolean() && x.get<bool>()); } }
        { const json& o = jobj(f, "cho"); auto& d = p.fx.cho; d.on = jflag(o, "on", d.on); d.mode = jint(o, "mode", d.mode); d.rate = jnum(o, "rate", d.rate); d.depth = jnum(o, "depth", d.depth); d.fb = jnum(o, "fb", d.fb); d.mix = jnum(o, "mix", d.mix); }
        { const json& o = jobj(f, "pha"); auto& d = p.fx.pha; d.on = jflag(o, "on", d.on); d.rate = jnum(o, "rate", d.rate); d.depth = jnum(o, "depth", d.depth); d.fb = jnum(o, "fb", d.fb); d.mix = jnum(o, "mix", d.mix); }
        { const json& o = jobj(f, "eq"); auto& d = p.fx.eq; d.on = jflag(o, "on", d.on); d.low = jnum(o, "low", d.low); d.mid = jnum(o, "mid", d.mid); d.mf = jnum(o, "mf", d.mf); d.high = jnum(o, "high", d.high); }
        { const json& o = jobj(f, "dly"); auto& d = p.fx.dly; d.on = jflag(o, "on", d.on); d.sync = jflag(o, "sync", d.sync); d.div = jdiv(o, "div", d.div); d.time = jnum(o, "time", d.time); d.fb = jnum(o, "fb", d.fb); d.pp = jflag(o, "pp", d.pp); d.lc = jnum(o, "lc", d.lc); d.hc = jnum(o, "hc", d.hc); d.mod = jnum(o, "mod", d.mod); d.mix = jnum(o, "mix", d.mix); }
        { const json& o = jobj(f, "rev"); auto& d = p.fx.rev; d.on = jflag(o, "on", d.on); d.type = jint(o, "type", d.type); d.size = jnum(o, "size", d.size); d.decay = jnum(o, "decay", d.decay); d.damp = jnum(o, "damp", d.damp); d.pre = jnum(o, "pre", d.pre); d.lc = jnum(o, "lc", d.lc); d.mix = jnum(o, "mix", d.mix); }
        { const json& o = jobj(f, "comp"); auto& d = p.fx.comp; d.on = jflag(o, "on", d.on); d.thr = jnum(o, "thr", d.thr); d.ratio = jnum(o, "ratio", d.ratio); d.gain = jnum(o, "gain", d.gain); }
        p.fx.width = jnum(f, "width", 1);
    }
    return p;
}

inline const json& sourcesJson() { static const json all = json::parse(kAetherSourcesJson, nullptr, false); return all; }
inline const std::vector<std::string>& sourceNames() {
    static const std::vector<std::string> v = [] { std::vector<std::string> n; if (sourcesJson().is_array()) for (const auto& s : sourcesJson()) n.push_back(jstr(s, "name", "")); return n; }();
    return v;
}
inline const std::vector<SourceDef>& sourceDefs() {
    static const std::vector<SourceDef> v = [] {
        std::vector<SourceDef> out;
        if (!sourcesJson().is_array()) return out;
        for (const auto& s : sourcesJson()) {
            SourceDef d; d.name = jstr(s, "name", ""); d.root = jnum(s, "root", 60); d.len = jnum(s, "len", 3); d.ls = jnum(s, "ls", 0); d.le = jnum(s, "le", 0); d.flat = jnum(s, "flat", 0);
            d.patch = patchFromJson(jobj(s, "p"));
            if (s.contains("ev")) for (const auto& e : s["ev"]) { std::array<double, 4> a{ 0, 0, 60, 0 }; for (size_t k = 0; k < e.size() && k < 4; k++) a[k] = e[k].get<double>(); d.ev.push_back(a); }
            out.push_back(d);
        }
        return out;
    }();
    return v;
}

inline std::shared_ptr<Source> renderSource(int id, double sr) {
    const auto& defs = sourceDefs();
    if (id < 0 || id >= (int)defs.size()) return nullptr;
    const SourceDef& d = defs[(size_t)id];
    auto e = std::make_unique<Engine>(sr);
    e->rng.seed(hash01((double)d.name.size() * 131 + (unsigned char)d.name[0]) * 1e9);
    Assets::Bundle b; b.p = d.patch;
    for (int i = 0; i < 4; i++) b.wt[i] = b.p.L[i].mode == 1 ? Assets::get().table(b.p.L[i].wt).get() : nullptr;   // sources never use other sources
    e->setBundle(b, false);
    const int N = (int)std::floor(d.len * sr + 0.5);
    auto out = std::make_shared<Source>(); out->data.assign((size_t)N, 0.0f); out->sr = sr; out->root = d.root;
    float L[128], R[128];
    auto ev = d.ev; std::stable_sort(ev.begin(), ev.end(), [](const std::array<double, 4>& a, const std::array<double, 4>& c) { return a[0] < c[0]; });
    size_t ei = 0; float* o = out->data.data();
    for (int off = 0; off < N; off += 128) {
        const double t = off / sr;
        while (ei < ev.size() && ev[ei][0] <= t) { const auto& x = ev[ei++]; if (x[1] != 0) e->noteIn((int)x[2], x[3] != 0 ? x[3] : 0.8); else e->noteOut((int)x[2]); }
        e->process(L, R, 128);
        const int m = std::min(128, N - off);
        for (int i = 0; i < m; i++) o[off + i] = (L[i] + R[i]) * 0.5f;
    }
    if (d.flat > 0) {
        const int W = (int)std::floor(0.03 * sr + 0.5); std::vector<float> env((size_t)N);
        double acc = 0;
        for (int i = 0; i < N; i++) { acc += (double)o[i] * o[i]; if (i >= W) acc -= (double)o[i - W] * o[i - W]; env[(size_t)i] = (float)std::sqrt(std::max(acc, 0.0) / W); }
        double em = 0, sm = 0; for (int i = 0; i < N; i++) if (env[(size_t)i] > em) em = env[(size_t)i];
        const double c = 1 - std::exp(-1 / (0.05 * sr));
        for (int i = 0; i < N; i++) { const double ev2 = env[(size_t)i]; if (ev2 > sm) sm = ev2; else sm += (ev2 - sm) * c; const double g2 = std::min(30.0, std::pow(em / std::max(sm, em * 1e-3), d.flat)); o[i] = (float)(o[i] * g2); }
    }
    double pk = 0; for (int i = 0; i < N; i++) pk = std::max(pk, (double)std::abs(o[i]));
    const double g = pk > 1e-6 ? 0.9 / pk : 1; const int fi = (int)std::floor(0.002 * sr + 0.5), fo = (int)std::floor(0.05 * sr + 0.5);
    for (int i = 0; i < N; i++) { double w = g; if (i < fi) w *= (double)i / fi; if (i > N - fo) w *= (double)(N - i) / fo; o[i] = (float)(o[i] * w); }
    out->ls = (int)std::floor(d.ls * sr + 0.5); out->le = (int)std::floor(d.le * sr + 0.5);
    return out;
}

inline std::shared_ptr<Source> Assets::source(int id) {
    if (id < 0 || id >= (int)src.size() || id >= (int)sourceDefs().size()) return nullptr;
    std::lock_guard<std::mutex> g(ms);
    if (!src[(size_t)id]) src[(size_t)id] = renderSource(id, 48000.0);
    return src[(size_t)id];
}

} // namespace aether
