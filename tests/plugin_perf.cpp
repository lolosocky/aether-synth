// CPU of the real plugin processor (engine + reverb convolution + output chain) for each preset, under the same playing
// as the engine stress test: overlapping chords, then fast repeated notes. Reports average and 99th-percentile block cost
// as a fraction of real time. A block over 1.0 is a dropout (crackle) on that machine.
// Usage: aether_plugin_perf [category filter] [stride] [block] [rate] [prepared max block]
// Hosts often prepare with a large maximum block and then call with smaller, varying ones (FL Studio does); pass a
// prepared size above the block size to test that.
#include "../plugin/PluginProcessor.h"
#include <iostream>
#include <iomanip>
#include <chrono>

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String filter = argc > 1 ? juce::String(argv[1]) : juce::String();
    const int stride = argc > 2 ? juce::jmax(1, std::atoi(argv[2])) : 1;
    const int B = argc > 3 ? std::atoi(argv[3]) : 256;
    const double SR = argc > 4 ? std::atof(argv[4]) : 48000;
    const int prepared = argc > 5 ? std::atoi(argv[5]) : B;
    std::unique_ptr<juce::AudioProcessor> base(createPluginFilter());
    auto& p = dynamic_cast<AetherProcessor&>(*base);
    p.setPlayConfigDetails(0, 2, SR, prepared);
    p.prepareToPlay(SR, prepared);
    juce::AudioBuffer<float> buf(2, B); juce::MidiBuffer midi;
    struct Ev { double t; int n; bool on; };
    std::vector<Ev> ev;
    const int ch[4][4] = { { 48, 55, 60, 63 }, { 44, 51, 56, 60 }, { 46, 53, 58, 62 }, { 43, 50, 55, 59 } };
    double t = 0;
    for (int r = 0; r < 2; r++) for (int c = 0; c < 4; c++) { for (int k = 0; k < 4; k++) { ev.push_back({ t + k * 0.012, ch[c][k], true }); ev.push_back({ t + 1.25, ch[c][k], false }); } t += 1.2; }
    const double s16 = 60.0 / 140 / 4;
    for (int i = 0; i < 24; i++) {
        const double tt = t + i * s16; const int n = (i % 8 < 4) ? 60 : 64;
        ev.push_back({ tt, n, true }); ev.push_back({ tt + s16 * 0.8, n, false });
        if (i % 4 == 0) for (int k = 0; k < 3; k++) { ev.push_back({ tt + k * 0.008, 48 + k * 7, true }); ev.push_back({ tt + s16 * 3.5, 48 + k * 7, false }); }
    }
    t += 24 * s16 + 1.5;
    std::stable_sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.t < b.t; });
    const auto& lib = Library::get();
    std::cout << std::fixed << std::setprecision(4) << std::unitbuf;
    double sumAvg = 0; int count = 0;
    for (int i = 0; i < (int)lib.entries.size(); i += stride) {
        if (filter.isNotEmpty() && !juce::String(lib.entries[(size_t)i].cat).containsIgnoreCase(filter)) continue;
        p.loadPreset(i);
        for (int k = 0; k < 8; k++) { midi.clear(); p.processBlock(buf, midi); }
        juce::Thread::sleep(150);   // let the convolver swap in this preset's impulse response
        for (int k = 0; k < 8; k++) { midi.clear(); p.processBlock(buf, midi); }
        const int N = (int)(t * SR); size_t e = 0; std::vector<double> blocks; double total = 0;
        for (int off = 0; off < N; off += B) {
            midi.clear();
            while (e < ev.size() && ev[e].t * SR < off + B) {
                const int at = juce::jlimit(0, B - 1, (int)(ev[e].t * SR) - off);
                midi.addEvent(ev[e].on ? juce::MidiMessage::noteOn(1, ev[e].n, (juce::uint8)100) : juce::MidiMessage::noteOff(1, ev[e].n), at);
                e++;
            }
            const auto t0 = std::chrono::steady_clock::now();
            p.processBlock(buf, midi);
            const double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() / (B / SR);
            blocks.push_back(dt); total += dt;
            if (const char* dump = std::getenv("AETHER_DUMP")) {   // raw float32 stereo per preset, for comparing builds
                static FILE* f = nullptr; static int which = -1;
                if (which != i) { if (f) std::fclose(f); f = std::fopen((std::string(dump) + "_" + std::to_string(i) + ".f32").c_str(), "wb"); which = i; }
                for (int k = 0; k < B; k++) { const float fr[2] = { buf.getSample(0, k), buf.getSample(1, k) }; std::fwrite(fr, 4, 2, f); }
            }
        }
        std::sort(blocks.begin(), blocks.end());
        const double avg = total / blocks.size(), p99 = blocks[(size_t)(blocks.size() * 0.99)], worst = blocks.back();
        sumAvg += avg; count++;
        std::cout << i << "\t" << lib.entries[(size_t)i].cat << "\t" << lib.entries[(size_t)i].name << "\tavg " << avg << "\tp99 " << p99 << "\tworst " << worst << "\n";
        for (int k = 0; k < 400; k++) { midi.clear(); p.processBlock(buf, midi); }   // let tails die before the next preset
    }
    std::cout << "presets " << count << " mean avg " << (count ? sumAvg / count : 0) << "\n";
    return 0;
}
