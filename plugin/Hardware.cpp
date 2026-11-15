#include "Hardware.h"
#include "BinaryData.h"

namespace ui {

const juce::Typeface::Ptr& typeface(int which) {
    static const juce::Typeface::Ptr faces[3] = {
        juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensed_500Medium_ttf, (size_t)BinaryData::BarlowCondensed_500Medium_ttfSize),
        juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensed_700Bold_ttf, (size_t)BinaryData::BarlowCondensed_700Bold_ttfSize),
        juce::Typeface::createSystemTypefaceFor(BinaryData::ShareTechMono_400Regular_ttf, (size_t)BinaryData::ShareTechMono_400Regular_ttfSize) };
    return faces[juce::jlimit(0, 2, which)];
}

void drawScrew(juce::Graphics& g, float cx, float cy, float r, float angle) {
    g.setColour(juce::Colours::black.withAlpha(0.55f)); g.fillEllipse(cx - r - 1, cy - r, (r + 1) * 2, (r + 1) * 2);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff9a9fa6), cx - r, cy - r, juce::Colour(0xff3c4046), cx + r, cy + r, false));
    g.fillEllipse(cx - r, cy - r, r * 2, r * 2);
    juce::Path slot; slot.addRectangle(-r * 0.75f, -r * 0.14f, r * 1.5f, r * 0.28f);
    g.setColour(juce::Colour(0xff1d1f22)); g.fillPath(slot, juce::AffineTransform::rotation(angle).translated(cx, cy));
}

void drawPlate(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, bool screws) {
    // drop shadow, body with a faint vertical sheen, bevel, then a groove around the edge
    g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillRoundedRectangle(r.translated(0, 2), 4);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2b2e33), r.getX(), r.getY(), juce::Colour(0xff202326), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 4);
    // fine brushed lines
    juce::Random rnd(42);
    for (float y = r.getY() + 2; y < r.getBottom() - 2; y += 2.0f) { g.setColour(juce::Colours::white.withAlpha(0.012f + 0.012f * rnd.nextFloat())); g.drawHorizontalLine((int)y, r.getX() + 2, r.getRight() - 2); }
    g.setColour(juce::Colours::white.withAlpha(0.10f)); g.drawLine(r.getX() + 4, r.getY() + 0.5f, r.getRight() - 4, r.getY() + 0.5f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.7f)); g.drawRoundedRectangle(r.reduced(0.5f), 4, 1.0f);
    if (title.isNotEmpty()) {
        const auto t = r.withHeight(22).reduced(12, 0).toNearestInt();
        drawEngraved(g, title.toUpperCase(), t, juce::Justification::centredLeft, 12.5f);
        // a short rule after the title, like printed panel graphics
        const float tw = juce::GlyphArrangement::getStringWidth(juce::Font(font(12.5f, true)).withExtraKerningFactor(0.12f), title.toUpperCase()) + 10;
        g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawHorizontalLine((int)r.getY() + 11, r.getX() + 12 + tw, r.getRight() - (screws ? 24.0f : 12.0f));
        g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawHorizontalLine((int)r.getY() + 12, r.getX() + 12 + tw, r.getRight() - (screws ? 24.0f : 12.0f));
    }
    if (screws) {
        drawScrew(g, r.getX() + 9, r.getY() + 9, 3.6f, 0.6f); drawScrew(g, r.getRight() - 9, r.getY() + 9, 3.6f, 2.1f);
        drawScrew(g, r.getX() + 9, r.getBottom() - 9, 3.6f, 1.2f); drawScrew(g, r.getRight() - 9, r.getBottom() - 9, 3.6f, 0.2f);
    }
}

void drawRecess(juce::Graphics& g, juce::Rectangle<float> r, float radius) {
    g.setColour(juce::Colour(0xff0f1012)); g.fillRoundedRectangle(r, radius);
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.6f), r.getX(), r.getY(), juce::Colours::transparentBlack, r.getX(), r.getY() + 6, false));
    g.fillRoundedRectangle(r, radius);
    g.setColour(juce::Colours::black); g.drawRoundedRectangle(r.reduced(0.5f), radius, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawLine(r.getX() + radius, r.getBottom() + 0.5f, r.getRight() - radius, r.getBottom() + 0.5f, 1.0f);
}

void drawLed(juce::Graphics& g, juce::Rectangle<float> r, bool on, juce::Colour c) {
    if (on) { g.setColour(c.withAlpha(0.28f)); g.fillRoundedRectangle(r.expanded(3, 2.5f), 3); }
    g.setColour(juce::Colours::black); g.fillRoundedRectangle(r.expanded(1), 1.5f);
    g.setGradientFill(juce::ColourGradient(on ? c.brighter(0.5f) : c.darker(2.2f).withSaturation(0.5f), r.getX(), r.getY(), on ? c : juce::Colour(0xff2a1a10), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 1);
}

void drawLcd(juce::Graphics& g, juce::Rectangle<float> r) {
    drawRecess(g, r.expanded(3), 5);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff22140a), r.getX(), r.getY(), lcdBg, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 3);
    g.setColour(juce::Colours::black.withAlpha(0.22f));
    for (float y = r.getY(); y < r.getBottom(); y += 3) g.drawHorizontalLine((int)y, r.getX(), r.getRight());   // scanlines
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.06f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX(), r.getCentreY(), false));
    g.fillRoundedRectangle(r, 3);                                                                                    // glass
}

void drawEngraved(juce::Graphics& g, const juce::String& s, juce::Rectangle<int> r, juce::Justification j, float size) {
    const juce::Font f = juce::Font(font(size, true)).withExtraKerningFactor(0.12f);
    g.setFont(f);
    g.setColour(juce::Colours::black.withAlpha(0.7f)); g.drawText(s, r.translated(0, 1), j, false);
    g.setColour(dim.brighter(0.25f)); g.drawText(s, r, j, false);
}
} // namespace ui

// ------------------------------------------------------------------ look and feel
AetherLook::AetherLook() {
    using namespace ui;
    setColour(juce::ResizableWindow::backgroundColourId, bg);
    setColour(juce::Label::textColourId, text);
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0f1012));
    setColour(juce::TextEditor::outlineColourId, line);
    setColour(juce::TextEditor::focusedOutlineColourId, accent);
    setColour(juce::TextEditor::textColourId, lcdText);
    setColour(juce::TextEditor::highlightColourId, accent.withAlpha(0.35f));
    setColour(juce::CaretComponent::caretColourId, accent);
    setColour(juce::ComboBox::backgroundColourId, panel);
    setColour(juce::ComboBox::outlineColourId, line);
    setColour(juce::ComboBox::textColourId, lcdText);
    setColour(juce::ComboBox::arrowColourId, accent);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1c1e21));
    setColour(juce::PopupMenu::textColourId, text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour(juce::PopupMenu::headerTextColourId, dim);
    setColour(juce::ListBox::backgroundColourId, lcdBg);
    setColour(juce::TextButton::buttonColourId, cap);
    setColour(juce::TextButton::textColourOffId, text);
    setColour(juce::TextButton::textColourOnId, text);
    setColour(juce::ScrollBar::thumbColourId, capHi);
    setColour(juce::Slider::textBoxTextColourId, lcdText);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ToggleButton::textColourId, text);
    setColour(juce::ToggleButton::tickColourId, accent);
}

juce::Typeface::Ptr AetherLook::getTypefaceForFont(const juce::Font& f) {
    const auto name = f.getTypefaceName();
    if (name.isEmpty() || name == juce::Font::getDefaultSansSerifFontName()) return ui::typeface(f.isBold() ? 1 : 0);
    return LookAndFeel_V4::getTypefaceForFont(f);
}

// Machined knob: shadow, knurled skirt, brushed cap with an indicator line, and an LED ring showing the value.
void AetherLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) {
    using namespace ui;
    const auto r = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(2);
    const float R = juce::jmin(r.getWidth(), r.getHeight()) / 2, cx = r.getCentreX(), cy = r.getCentreY();
    const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0;
    const float ang = a0 + pos * (a1 - a0), zero = bipolar ? (a0 + a1) / 2 : a0;
    const bool enabled = s.isEnabled();
    // LED ring
    const int nLed = R > 30 ? 25 : (R > 20 ? 19 : 15);
    const float ledR = R - 2.5f, dot = R > 30 ? 2.6f : 2.1f;
    for (int i = 0; i < nLed; i++) {
        const float a = a0 + (a1 - a0) * i / (nLed - 1);
        const bool lit = enabled && (bipolar ? ((a >= juce::jmin(zero, ang) - 0.01f) && (a <= juce::jmax(zero, ang) + 0.01f)) : a <= ang + 0.01f);
        const float lx = cx + ledR * std::sin(a), ly = cy - ledR * std::cos(a);
        if (lit) { g.setColour(accent.withAlpha(0.25f)); g.fillEllipse(lx - dot * 2, ly - dot * 2, dot * 4, dot * 4); }
        g.setColour(lit ? accent.brighter(0.25f) : juce::Colour(0xff3a2414));
        g.fillEllipse(lx - dot, ly - dot, dot * 2, dot * 2);
    }
    // knob body
    const float kr = R - (R > 30 ? 9.0f : 7.5f);
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.fillEllipse(cx - kr - 1, cy - kr + 2, (kr + 1) * 2, (kr + 1) * 2);
    // knurled skirt
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3c4046), cx, cy - kr, juce::Colour(0xff15171a), cx, cy + kr, false));
    g.fillEllipse(cx - kr, cy - kr, kr * 2, kr * 2);
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    const int teeth = (int)(kr * 1.6f);
    for (int i = 0; i < teeth; i++) {
        const float a = juce::MathConstants<float>::twoPi * i / teeth + ang;
        g.drawLine(cx + (kr - 3.2f) * std::sin(a), cy - (kr - 3.2f) * std::cos(a), cx + kr * std::sin(a), cy - kr * std::cos(a), 1.0f);
    }
    // cap
    const float cr = kr * 0.74f;
    g.setGradientFill(juce::ColourGradient(capHi, cx - cr, cy - cr, capLo, cx + cr, cy + cr, true));
    g.fillEllipse(cx - cr, cy - cr, cr * 2, cr * 2);
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.18f), cx, cy - cr, juce::Colours::transparentWhite, cx, cy + cr * 0.2f, false));
    g.fillEllipse(cx - cr, cy - cr, cr * 2, cr * 2);
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.drawEllipse(cx - cr, cy - cr, cr * 2, cr * 2, 1.0f);
    // indicator
    juce::Path tick; tick.addRoundedRectangle(-1.4f, -kr + 1.5f, 2.8f, kr * 0.55f, 1.2f);
    g.setColour(enabled ? text : dim); g.fillPath(tick, juce::AffineTransform::rotation(ang).translated(cx, cy));
}

void AetherLook::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) {
    using namespace ui;
    const float cy = y + h * 0.5f;
    drawRecess(g, { (float)x, cy - 3, (float)w, 6 }, 3);
    g.setColour(accent); g.fillRoundedRectangle((float)x + 1, cy - 1.5f, juce::jmax(0.0f, pos - x - 1), 3, 1.5f);
    const juce::Rectangle<float> capR(pos - 6, cy - 9, 12, 18);
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillRoundedRectangle(capR.translated(0, 2), 2);
    g.setGradientFill(juce::ColourGradient(capHi, capR.getX(), capR.getY(), capLo, capR.getX(), capR.getBottom(), false)); g.fillRoundedRectangle(capR, 2);
    g.setColour(text); g.drawHorizontalLine((int)cy, capR.getX() + 2, capR.getRight() - 2);
}

juce::Label* AetherLook::createSliderTextBox(juce::Slider& s) {
    auto* l = LookAndFeel_V4::createSliderTextBox(s);
    l->setFont(ui::mono(12.0f)); l->setColour(juce::Label::textColourId, ui::lcdText);
    return l;
}

// Rubber/plastic machine button. Buttons flagged "led" carry a status LED; toggled-on ones light it.
void AetherLook::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) {
    using namespace ui;
    auto r = b.getLocalBounds().toFloat().reduced(1.5f, 1.5f);
    const bool on = b.getToggleState();
    const bool led = (bool)b.getProperties().getWithDefault("led", false);
    // well the button sits in
    g.setColour(juce::Colours::black.withAlpha(0.85f)); g.fillRoundedRectangle(r.expanded(1.5f), 4);
    if (!down) { g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillRoundedRectangle(r.translated(0, 2), 3); }
    const auto body = down ? r.translated(0, 1) : r;
    juce::Colour top = over ? juce::Colour(0xff4a4f56) : juce::Colour(0xff41454b), bot = juce::Colour(0xff26292d);
    if (down) std::swap(top, bot);
    g.setGradientFill(juce::ColourGradient(top, body.getX(), body.getY(), bot, body.getX(), body.getBottom(), false));
    g.fillRoundedRectangle(body, 3);
    g.setColour(juce::Colours::white.withAlpha(down ? 0.03f : 0.16f)); g.drawLine(body.getX() + 3, body.getY() + 0.8f, body.getRight() - 3, body.getY() + 0.8f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRoundedRectangle(body, 3, 1.0f);
    if (led) {
        const float lw = juce::jmin(16.0f, body.getWidth() * 0.3f);
        drawLed(g, { body.getCentreX() - lw / 2, body.getY() + 4, lw, 3 }, on);
    } else if (on) {
        g.setColour(accent.withAlpha(0.18f)); g.fillRoundedRectangle(body, 3);
        g.setColour(accent); g.drawRoundedRectangle(body, 3, 1.2f);
    }
}

juce::Font AetherLook::getTextButtonFont(juce::TextButton&, int h) { return juce::Font(ui::font(juce::jmin(16.0f, h * 0.55f), true)).withExtraKerningFactor(0.08f); }

void AetherLook::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool down) {
    const bool led = (bool)b.getProperties().getWithDefault("led", false);
    auto r = b.getLocalBounds().reduced(4, 2).translated(0, down ? 1 : 0);
    if (led) r = r.withTrimmedTop(6);
    g.setFont(getTextButtonFont(b, b.getHeight()));
    g.setColour(b.getToggleState() && led ? ui::text : (b.isEnabled() ? ui::text.withAlpha(0.88f) : ui::dim));
    g.drawFittedText(b.getButtonText().toUpperCase(), r, juce::Justification::centred, 1, 0.8f);
}

// Latching switch: a small square button with an LED, label printed beside it.
void AetherLook::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool down) {
    using namespace ui;
    const float h = (float)b.getHeight(), sz = juce::jmin(20.0f, h - 6);
    const juce::Rectangle<float> key(3, (h - sz) / 2, sz, sz);
    g.setColour(juce::Colours::black.withAlpha(0.85f)); g.fillRoundedRectangle(key.expanded(1.5f), 3);
    const auto body = down ? key.translated(0, 1) : key;
    g.setGradientFill(juce::ColourGradient(over ? juce::Colour(0xff4a4f56) : juce::Colour(0xff41454b), body.getX(), body.getY(), juce::Colour(0xff26292d), body.getX(), body.getBottom(), false));
    g.fillRoundedRectangle(body, 2.5f);
    drawLed(g, body.reduced(sz * 0.3f, sz * 0.38f), b.getToggleState());
    g.setFont(juce::Font(font(14.0f, true)).withExtraKerningFactor(0.06f));
    g.setColour(b.isEnabled() ? text : dim);
    g.drawFittedText(b.getButtonText().toUpperCase(), b.getLocalBounds().withTrimmedLeft((int)sz + 10), juce::Justification::centredLeft, 2, 0.8f);
}

void AetherLook::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) {
    using namespace ui;
    const juce::Rectangle<float> r(1, 1, (float)w - 2, (float)h - 2);
    drawRecess(g, r, 3);
    // arrow on a small button cap
    const juce::Rectangle<float> cap(r.getRight() - 22, r.getY() + 3, 19, r.getHeight() - 6);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff41454b), cap.getX(), cap.getY(), juce::Colour(0xff26292d), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle(cap, 2);
    juce::Path tri; tri.addTriangle(cap.getCentreX() - 4, cap.getCentreY() - 2, cap.getCentreX() + 4, cap.getCentreY() - 2, cap.getCentreX(), cap.getCentreY() + 3);
    g.setColour(box.isEnabled() ? accent : dim); g.fillPath(tri);
}
juce::Font AetherLook::getComboBoxFont(juce::ComboBox&) { return juce::Font(ui::mono(14.0f)); }
void AetherLook::positionComboBoxText(juce::ComboBox& box, juce::Label& l) { l.setBounds(6, 1, box.getWidth() - 30, box.getHeight() - 2); l.setFont(getComboBoxFont(box)); }
juce::Font AetherLook::getPopupMenuFont() { return juce::Font(ui::font(16.0f)); }
void AetherLook::drawPopupMenuBackground(juce::Graphics& g, int w, int h) {
    g.fillAll(findColour(juce::PopupMenu::backgroundColourId));
    g.setColour(ui::accent.withAlpha(0.6f)); g.drawRect(0, 0, w, h, 1);
}
void AetherLook::fillTextEditorBackground(juce::Graphics& g, int w, int h, juce::TextEditor&) { ui::drawRecess(g, { 0.5f, 0.5f, (float)w - 1, (float)h - 1 }, 3); }
void AetherLook::drawTextEditorOutline(juce::Graphics& g, int w, int h, juce::TextEditor& e) {
    if (e.hasKeyboardFocus(true)) { g.setColour(ui::accent.withAlpha(0.7f)); g.drawRoundedRectangle(0.5f, 0.5f, (float)w - 1, (float)h - 1, 3, 1.0f); }
}
void AetherLook::drawLabel(juce::Graphics& g, juce::Label& l) {
    if (l.isBeingEdited()) return;
    const bool caps = (bool)l.getProperties().getWithDefault("caps", false);
    juce::Font f = getLabelFont(l);
    if (caps) f = f.withExtraKerningFactor(0.08f);
    g.setFont(f);
    g.setColour(l.findColour(juce::Label::textColourId).withMultipliedAlpha(l.isEnabled() ? 1.0f : 0.5f));
    g.drawFittedText(caps ? l.getText().toUpperCase() : l.getText(), l.getBorderSize().subtractedFrom(l.getLocalBounds()), l.getJustificationType(), 2, l.getMinimumHorizontalScale());
}
void AetherLook::drawScrollbar(juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int ts, int tsz, bool over, bool down) {
    const juce::Rectangle<float> track((float)x + 2, (float)y + 2, (float)w - 4, (float)h - 4);
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.fillRoundedRectangle(track, 3);
    const auto th = vertical ? juce::Rectangle<float>(track.getX(), (float)ts, track.getWidth(), (float)tsz) : juce::Rectangle<float>((float)ts, track.getY(), (float)tsz, track.getHeight());
    g.setColour(over || down ? ui::accent.withAlpha(0.8f) : ui::capHi); g.fillRoundedRectangle(th.reduced(0.5f), 3);
}
