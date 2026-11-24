#pragma once
#include "PluginProcessor.h"

namespace edit { class Page; class Scroller; }

#include "Hardware.h"

// XY pad for the Orb: drag to steer two morph axes the preset maps to its sound.
class OrbPad : public juce::Component, private juce::Timer {
public:
    explicit OrbPad(AetherProcessor& p);
    ~OrbPad() override { stopTimer(); }
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent& e) override { drag(e); }
    void mouseDrag(const juce::MouseEvent& e) override { drag(e); }
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
private:
    void drag(const juce::MouseEvent&);
    void timerCallback() override { repaint(); }
    AetherProcessor& proc; juce::RangedAudioParameter* px; juce::RangedAudioParameter* py; bool held = false;
};

// Keyboard in the Station look: white keys with hairline edges, graphite black keys, accent when held.
class HardwareKeys : public juce::MidiKeyboardComponent {
public:
    using juce::MidiKeyboardComponent::MidiKeyboardComponent;
    void drawWhiteNote(int note, juce::Graphics&, juce::Rectangle<float> area, bool down, bool over, juce::Colour, juce::Colour) override;
    void drawBlackNote(int note, juce::Graphics&, juce::Rectangle<float> area, bool down, bool over, juce::Colour) override;
};

struct Knob : juce::Component {
    Knob(juce::AudioProcessorValueTreeState& s, const char* id, const juce::String& label);
    void resized() override;
    juce::Slider slider; juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class AetherEditor : public juce::AudioProcessorEditor, private juce::ChangeListener, private juce::Timer, private juce::ListBoxModel {
public:
    explicit AetherEditor(AetherProcessor&);
    ~AetherEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override { refreshInfo(); }
    void timerCallback() override;
    int getNumRows() override { return (int)shown.size(); }
    void paintListBoxItem(int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void selectedRowsChanged(int) override {}
    void refilter();
    void refreshInfo();
    void showPage(int i);
    void layoutPlay(juce::Rectangle<int> r);

    AetherProcessor& proc;
    AetherLook look;
    juce::TextEditor search;
    juce::ComboBox catBox;
    juce::ListBox list{ "presets", this };
    std::vector<int> shown;
    juce::TextButton prev{ "<" }, next{ ">" }, rnd{ "Random" };
    juce::Label nameLabel, catLabel, statusLabel;
    OrbPad orb;
    std::unique_ptr<Knob> macros[4], perf[6], layers[4], volume;
    juce::Label secMacro, secPerf, secLayers, secOrb;
    HardwareKeys keys;
    juce::Random random;
    static constexpr int kPages = 6;
    juce::TextButton pageTabs[kPages + 1];
    std::unique_ptr<edit::Page> pages[kPages];
    std::unique_ptr<edit::Scroller> scrollers[kPages];
    int page = 0, lastGen = -1;
    juce::Rectangle<int> header, centre, lcdArea, browserPlate, playPlates[4];
    juce::Label sectionLcd;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AetherEditor)
};
