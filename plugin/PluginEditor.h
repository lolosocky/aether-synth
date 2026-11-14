#pragma once
#include "PluginProcessor.h"

namespace edit { class Page; class Scroller; }

namespace ui {
const juce::Colour bg{ 0xff0d0f14 }, panel{ 0xff161a22 }, line{ 0xff262c38 }, text{ 0xffe8ebf2 }, dim{ 0xff8a93a6 }, accent{ 0xff7c9cff }, accent2{ 0xffc58bff };
}

class AetherLook : public juce::LookAndFeel_V4 {
public:
    AetherLook();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
};

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
    juce::MidiKeyboardComponent keys;
    juce::Random random;
    static constexpr int kPages = 6;
    juce::TextButton pageTabs[kPages + 1];
    std::unique_ptr<edit::Page> pages[kPages];
    std::unique_ptr<edit::Scroller> scrollers[kPages];
    int page = 0, lastGen = -1;
    juce::Rectangle<int> centre;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AetherEditor)
};
