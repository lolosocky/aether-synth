// Renders the editor's pages to PNG so the design can be reviewed without a host.
// Usage: aether_snapshot <out dir> [scale] [width height]
#include "../plugin/PluginProcessor.h"
#include "../plugin/PluginEditor.h"
#include <iostream>

static void collect(juce::Component& c, juce::Array<juce::TextButton*>& out) {
    for (auto* ch : c.getChildren()) { if (auto* b = dynamic_cast<juce::TextButton*>(ch)) out.add(b); collect(*ch, out); }
}

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File dir(juce::File::getCurrentWorkingDirectory().getChildFile(argc > 1 ? argv[1] : "snapshots"));
    dir.createDirectory();
    const float scale = argc > 2 ? (float)std::atof(argv[2]) : 2.0f;
    const int W = argc > 4 ? std::atoi(argv[3]) : 1120, H = argc > 4 ? std::atoi(argv[4]) : 720;
    std::unique_ptr<juce::AudioProcessor> base(createPluginFilter());
    auto& p = dynamic_cast<AetherProcessor&>(*base);
    p.setPlayConfigDetails(0, 2, 48000, 256); p.prepareToPlay(48000, 256);
    p.loadPreset(0);
    juce::AudioBuffer<float> buf(2, 256); juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
    for (int i = 0; i < 40; i++) { p.processBlock(buf, midi); midi.clear(); }
    std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());
    ed->setSize(W, H);
    juce::Array<juce::TextButton*> buttons; collect(*ed, buttons);
    const char* pages[] = { "Play", "Layers", "Mod", "FX", "Arp", "Sections", "Crate" };
    for (auto* name : pages) {
        for (auto* b : buttons) if (b->getButtonText() == name && b->onClick) { b->onClick(); break; }
        ed->resized();
        const auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, scale);
        const auto f = dir.getChildFile(juce::String("aether-") + juce::String(name).toLowerCase() + ".png");
        f.deleteFile();
        juce::FileOutputStream os(f); juce::PNGImageFormat().writeImageToStream(img, os);
        std::cout << f.getFullPathName() << "\n";
    }
    ed.reset();
    return 0;
}
