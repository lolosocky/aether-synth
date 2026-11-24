// Station: the visual language shared by Aether and the LGN mixing suite.
// White modules on a pale hull, hairline edges, soft shadows, one accent colour per product.
// Everything draws through these helpers and station::Look so every LGN plugin reads as one family.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace station {

// ---------------------------------------------------------------- palette
const juce::Colour hull{ 0xffeef0f3 },     // window background
    module{ 0xffffffff },                   // raised white panels
    hairline{ 0xffdfe3e8 },                 // panel and control edges
    hairlineHi{ 0xffc8ced6 },               // hovered edges
    well{ 0xfff4f6f8 },                     // recessed areas: displays, inputs, tracks
    wellLine{ 0xffe4e8ec },
    track{ 0xffe4e7eb },                    // unlit arcs and slider tracks
    ink{ 0xff15181d },                      // primary text
    dim{ 0xff7a828d },                      // secondary text
    faint{ 0xffb3b9c1 },                    // tertiary text, grid lines
    graphite{ 0xff2a2e35 };                 // meters, black keys

// One accent per product.
namespace accents {
const juce::Colour aether{ 0xffff5b1a },    // mission orange
    eq{ 0xff00a3c4 },                       // cyan
    comp{ 0xffe6a000 },                     // amber
    deess{ 0xffef3b8c },                    // pink
    saturate{ 0xffe5352e },                 // red
    verb{ 0xff2d66f5 },                     // blue
    delay{ 0xff1fae55 },                    // green
    limit{ 0xff7b55f5 },                    // violet
    gravity{ 0xff121417 },                  // void
    duck{ 0xff0fb5a5 };                     // teal
}

// ---------------------------------------------------------------- type
enum Face { regular = 0, medium = 1, bold = 2, monoRegular = 3, monoMedium = 4 };
const juce::Typeface::Ptr& typeface(Face);
inline juce::FontOptions sans(float h, Face f = medium) { return juce::FontOptions(typeface(f)).withHeight(h); }
inline juce::FontOptions mono(float h, bool medium = false) { return juce::FontOptions(typeface(medium ? monoMedium : monoRegular)).withHeight(h); }
// Small uppercase labels with open tracking, the signature of the look.
juce::Font caps(float h, Face f = medium, float tracking = 0.14f);

// ---------------------------------------------------------------- drawing
// A white panel. Title is printed small and spaced at the top left, code (e.g. "M-01") faintly at the top right.
// With an outline colour, the panel edge is drawn in that colour instead of a grey hairline.
void drawModule(juce::Graphics&, juce::Rectangle<float>, const juce::String& title = {}, juce::Colour outline = {}, const juce::String& code = {});
// Height a module's title row takes, so content can start below it.
constexpr int kModuleTitle = 30;
// Recessed area: displays, pads, inputs.
void drawWell(juce::Graphics&, juce::Rectangle<float>, float radius = 8.0f);
// Status light: a small dot, lit in the accent colour with a soft halo.
void drawDot(juce::Graphics&, juce::Point<float> centre, float radius, bool on, juce::Colour accent);
// Product wordmark: "LGN" small, name large, optional subtitle, accent mark.
void drawWordmark(juce::Graphics&, juce::Rectangle<float>, const juce::String& name, const juce::String& subtitle, juce::Colour accent);
// Uppercase spaced text.
void drawCaps(juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, float size, juce::Colour, Face f = medium);
// Soft drop shadow under a rounded rectangle.
void drawShadow(juce::Graphics&, juce::Rectangle<float>, float radius, float strength = 1.0f);

// ---------------------------------------------------------------- look and feel
class Look : public juce::LookAndFeel_V4 {
public:
    explicit Look(juce::Colour accent);
    juce::Colour accent;

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font&) override;
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
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
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* textColour) override;
    void fillTextEditorBackground(juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline(juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawLabel(juce::Graphics&, juce::Label&) override;
    void drawScrollbar(juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool over, bool down) override;
    int getDefaultScrollbarWidth() override { return 8; }
    void drawTooltip(juce::Graphics&, const juce::String& text, int w, int h) override;
    juce::Rectangle<int> getTooltipBounds(const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;
    void drawCornerResizer(juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;
};

// Button properties understood by Look (set with button.getProperties().set(...)):
//   "led"  true: a tab / mode button. On = graphite pill with white text and an accent dot.
//   "tone" "accent": an action button drawn with the accent colour.
}
