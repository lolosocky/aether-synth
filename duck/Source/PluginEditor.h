#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

struct DuckLook : juce::LookAndFeel_V4 {
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
};

class DuckEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit DuckEditor(DuckProcessor&);
    ~DuckEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void setChoice(const juce::String& id, int index);
    void drawCurve(juce::Graphics&, juce::Rectangle<float>);
    DuckProcessor& proc;
    DuckLook look;
    juce::Slider amount, release;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAtt, releaseAtt;
    juce::TextButton rateBtn[5], modeBtn[2];
    juce::Rectangle<float> curveArea;
    float shownReduction = 0;
};
