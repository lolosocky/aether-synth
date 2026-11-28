// Prints the level offset (dB) each LGN Strike library sound needs so its peak sits at its family's target.
// Built and run by tools/strike_level.py.
#include "../dsp/Strike.h"
#include <cstdio>
#include <map>
namespace lgn {
#include "../strike/Sounds.inc"
}
using namespace lgn;
int main() {
    const std::map<std::string, double> target = { { "Kick", -1.5 }, { "808", -1.5 }, { "Snare", -3.0 }, { "Clap", -4.5 }, { "Hat", -9.0 }, { "Open Hat", -10.0 },
                                                   { "Cymbal", -11.0 }, { "Perc", -7.0 }, { "Tom", -4.5 }, { "FX", -7.0 } };
    const double sr = 48000; std::vector<float> o((size_t)(sr * 4));
    for (auto& s : strikeSounds()) {
        PadParams p; for (int k = 0; k < 15; k++) padSet(p, strikeSoundKeys()[k], s.v[k]);
        p.send = 0; StrikeDsp::preview(p, sr, o.data(), (int)o.size());
        double pk = 0; for (float x : o) pk = std::max(pk, (double)std::abs(x));
        const double t = target.count(s.category) ? target.at(s.category) : -6.0;
        std::printf("%s\t%.2f\n", s.name, t - 20 * std::log10(std::max(pk, 1e-9)));
    }
}
