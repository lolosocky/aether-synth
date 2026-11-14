// Renders every factory preset with the C++ engine and compares against reference renders from the browser engine.
#include "../src/AetherPatch.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
using namespace aether;
static std::string slurp(const std::string& p) { std::ifstream f(p, std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "tests/ref";
    const json lib = json::parse(slurp(dir + "/presets.json")); const json ref = json::parse(slurp(dir + "/ref.json"));
    const auto& presets = lib["presets"]; const double SR = 48000;
    int bad = 0, exact = 0, close = 0, n = 0; double worstDb = 0, worstRt = 0; std::string worstName, worstRtName;
    for (size_t idx = 0; idx < presets.size(); idx++) {
        const json& r = ref[idx];
        Assets::Bundle b; b.p = patchFromJson(presets[idx]); Assets::get().fill(b);
        auto e = std::make_unique<Engine>(SR); e->setBundle(b, true); e->fxModEnabled = false; e->rng.seed(1000.0 + (double)idx);
        std::vector<int> notes = r["notes"].get<std::vector<int>>();
        for (int nn : notes) e->noteIn(nn, 0.8);
        const int N = (int)(2.5 * SR); float L[128], R[128]; double sum = 0, pk = 0; bool offDone = false; int nan = 0;
        std::vector<float> head, all; all.reserve((size_t)N * 2);
        const auto t0 = std::chrono::steady_clock::now();
        for (int off = 0; off < N; off += 128) {
            if (!offDone && off >= 2 * SR) { for (int nn : notes) e->noteOut(nn); offDone = true; }
            e->process(L, R, 128);
            for (int i = 0; i < 128 && off + i < N; i++) { if (!std::isfinite(L[i]) || !std::isfinite(R[i])) nan++; sum += ((double)L[i] * L[i] + (double)R[i] * R[i]) * 0.5; pk = std::max({ pk, (double)std::abs(L[i]), (double)std::abs(R[i]) }); all.push_back(L[i]); all.push_back(R[i]); }
            if (off < 128 * 8) for (int i = 0; i < 128; i += 16) head.push_back(L[i]);
        }
        const double rt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() / 2.5;
        if (rt > worstRt) { worstRt = rt; worstRtName = r["name"]; }
        const double rms = std::sqrt(sum / N), rr = r["rms"].get<double>();
        const double db = 20 * std::log10((rms + 1e-9) / (rr + 1e-9));
        double hd = 0; const auto& rh = r["head"]; for (size_t i = 0; i < head.size() && i < rh.size(); i++) hd = std::max(hd, std::abs((double)head[i] - rh[i].get<double>()));
        n++;
        if (hd < 1e-4) exact++;
        if (std::abs(db) < 0.5) close++;
        if (std::abs(db) > std::abs(worstDb)) { worstDb = db; worstName = r["name"]; }
        if (nan || std::abs(db) > 0.2 || hd >= 1e-4) { bad++; if (bad <= 40) std::cout << "DIFF " << idx << " " << r["name"] << " [" << r["cat"] << "] rms " << rms << " vs " << rr << " (" << db << " dB) head " << hd << " nan " << nan << "\n"; }
        std::ifstream f(dir + "/" + std::to_string(idx) + ".f32", std::ios::binary);
        if (f) { std::vector<float> w((size_t)N * 2); f.read((char*)w.data(), (std::streamsize)(w.size() * 4)); double md = 0; size_t first = 0; for (size_t i = 0; i < w.size(); i++) { const double d = std::abs((double)w[i] - all[i]); if (d > md) md = d; if (!first && d > 1e-3) first = i; } std::cout << "full " << idx << " " << r["name"] << " max diff " << md << " first>1e-3 at " << first / 2 << "\n"; }
    }
    std::cout << "presets " << n << " | head-exact " << exact << " | rms within 0.5 dB " << close << " | off by >1.5 dB or NaN " << bad << " | worst " << worstName << " " << worstDb << " dB | heaviest " << worstRtName << " " << worstRt << " x realtime\n";
    return bad ? 1 : 0;
}
