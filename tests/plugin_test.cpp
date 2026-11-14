// Drives the real plugin processor headlessly: preset loads, MIDI, automation, state save/restore, CPU.
#include "../plugin/PluginProcessor.h"
#include <iostream>
#include <chrono>
int main() {
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> base(createPluginFilter());
    auto& p = dynamic_cast<AetherProcessor&>(*base);
    p.setPlayConfigDetails(0, 2, 44100, 512);
    p.prepareToPlay(44100, 512);
    juce::AudioBuffer<float> buf(2, 512); juce::MidiBuffer midi;
    auto run = [&](double secs, std::vector<int> on, std::vector<int> off) {
        double sum = 0, pk = 0; long n = 0; bool nan = false;
        const int blocks = (int)(secs * 44100 / 512);
        for (int b = 0; b < blocks; b++) {
            midi.clear();
            if (b == 0) for (int x : on) midi.addEvent(juce::MidiMessage::noteOn(1, x, (juce::uint8)100), 17);
            if (b == blocks - 1) for (int x : off) midi.addEvent(juce::MidiMessage::noteOff(1, x), 300);
            p.processBlock(buf, midi);
            for (int c = 0; c < 2; c++) for (int i = 0; i < 512; i++) { const float v = buf.getSample(c, i); if (!std::isfinite(v)) nan = true; sum += v * v; pk = std::max(pk, (double)std::abs(v)); n++; }
        }
        return std::make_tuple(20 * std::log10(std::sqrt(sum / n) + 1e-9), 20 * std::log10(pk + 1e-9), nan);
    };
    const auto& lib = Library::get();
    std::cout << "library " << lib.entries.size() << " presets, " << lib.cats.size() << " categories\n";
    int fails = 0; double worstPk = -200, minRms = 100; std::string worstName, quietName;
    const auto t0 = std::chrono::steady_clock::now(); double audio = 0;
    for (int i = 0; i < (int)lib.entries.size(); i += 7) {
        p.loadPreset(i);
        const auto [rms, pk, nan] = run(1.2, { 48, 55, 60, 63 }, { 48, 55, 60, 63 }); audio += 1.2;
        run(0.3, {}, {}); audio += 0.3;
        if (nan || rms < -60 || pk > 0.01) { fails++; std::cout << "  ! " << lib.entries[(size_t)i].name << " rms " << rms << " pk " << pk << " nan " << nan << "\n"; }
        if (pk > worstPk) { worstPk = pk; worstName = lib.entries[(size_t)i].name; }
        if (rms < minRms) { minRms = rms; quietName = lib.entries[(size_t)i].name; }
    }
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::cout << "sweep: fails " << fails << " | hottest peak " << worstPk << " dBFS (" << worstName << ") | quietest " << minRms << " dB (" << quietName << ") | CPU " << secs / audio * 100 << "% of one core incl. preset loads\n";
    // automation + state round trip
    p.loadPreset(42);
    p.apvts.getParameter("mac2")->setValueNotifyingHost(0.73f);
    p.apvts.getParameter("cutoff")->setValueNotifyingHost(0.25f);
    juce::MemoryBlock state; p.getStateInformation(state);
    std::unique_ptr<juce::AudioProcessor> other(createPluginFilter());
    auto& q = dynamic_cast<AetherProcessor&>(*other);
    q.setStateInformation(state.getData(), (int)state.getSize());
    std::cout << "state: " << state.getSize() << " bytes | restored '" << q.info().name << "' idx " << q.currentIndex() << " (want '" << lib.entries[42].name << "' 42) | mac2 "
              << q.apvts.getParameter("mac2")->getValue() << " cutoff " << q.apvts.getRawParameterValue("cutoff")->load() << "\n";
    const bool ok = q.info().name.toStdString() == lib.entries[42].name && q.currentIndex() == 42 && std::abs(q.apvts.getParameter("mac2")->getValue() - 0.73f) < 1e-3;
    // sample-rate change
    p.prepareToPlay(96000, 128); buf.setSize(2, 512); p.loadPreset(10);
    const auto [r96, pk96, nan96] = run(1.0, { 60 }, { 60 });
    std::cout << "96k: rms " << r96 << " nan " << nan96 << "\n";
    return (fails || !ok || nan96) ? 1 : 0;
}
