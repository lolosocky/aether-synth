// Hardware-style visual language for Aether: anodised panels, machined knobs with LED rings, latching buttons with LEDs,
// an amber display. Every page draws through these helpers so the whole plugin reads as one instrument.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui {
// palette
const juce::Colour bg{ 0xff17191c },        // chassis
    panel{ 0xff24272b },                    // raised plate
    line{ 0xff0b0c0e },                     // grooves and outlines
    text{ 0xffebe7de },                     // silkscreen
    dim{ 0xff8f8b83 },                      // secondary print
    accent{ 0xffff7a1a },                   // LED orange
    accent2{ 0xffffb54a },                  // amber
    lcdBg{ 0xff140c04 }, lcdText{ 0xffffb54a }, lcdDim{ 0xff6e4a1e },
    cap{ 0xff3a3e44 }, capHi{ 0xff5b6068 }, capLo{ 0xff202326 };

const juce::Typeface::Ptr& typeface(int which);   // 0 condensed medium, 1 condensed bold, 2 mono
inline juce::FontOptions font(float h, bool bold = false) { return juce::FontOptions(typeface(bold ? 1 : 0)).withHeight(h); }
inline juce::FontOptions mono(float h) { return juce::FontOptions(typeface(2)).withHeight(h); }

void drawScrew(juce::Graphics&, float cx, float cy, float r, float angle);
void drawPlate(juce::Graphics&, juce::Rectangle<float>, const juce::String& title = {}, bool screws = false);
void drawRecess(juce::Graphics&, juce::Rectangle<float>, float radius = 3.0f);
void drawLed(juce::Graphics&, juce::Rectangle<float>, bool on, juce::Colour c = accent);
void drawLcd(juce::Graphics&, juce::Rectangle<float>);
void drawEngraved(juce::Graphics&, const juce::String&, juce::Rectangle<int>, juce::Justification, float size = 12.0f);
}

class AetherLook : public juce::LookAndFeel_V4 {
public:
    AetherLook();
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font&) override;
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Label* createSliderTextBox(juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getTextButtonFont(juce::TextButton&, int h) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox(juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground(juce::Graphics&, int w, int h) override;
    void fillTextEditorBackground(juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline(juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawLabel(juce::Graphics&, juce::Label&) override;
    void drawScrollbar(juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool over, bool down) override;
    int getDefaultScrollbarWidth() override { return 10; }
};
