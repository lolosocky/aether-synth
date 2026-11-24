#include "Station.h"
#include "StationFontData.h"

namespace station {

const juce::Typeface::Ptr& typeface(Face f) {
    static const juce::Typeface::Ptr faces[5] = {
        juce::Typeface::createSystemTypefaceFor(StationFontData::SpaceGrotesk_400Regular_ttf, (size_t)StationFontData::SpaceGrotesk_400Regular_ttfSize),
        juce::Typeface::createSystemTypefaceFor(StationFontData::SpaceGrotesk_500Medium_ttf, (size_t)StationFontData::SpaceGrotesk_500Medium_ttfSize),
        juce::Typeface::createSystemTypefaceFor(StationFontData::SpaceGrotesk_700Bold_ttf, (size_t)StationFontData::SpaceGrotesk_700Bold_ttfSize),
        juce::Typeface::createSystemTypefaceFor(StationFontData::IBMPlexMono_400Regular_ttf, (size_t)StationFontData::IBMPlexMono_400Regular_ttfSize),
        juce::Typeface::createSystemTypefaceFor(StationFontData::IBMPlexMono_500Medium_ttf, (size_t)StationFontData::IBMPlexMono_500Medium_ttfSize) };
    return faces[juce::jlimit(0, 4, (int)f)];
}

juce::Font caps(float h, Face f, float tracking) { return juce::Font(sans(h, f)).withExtraKerningFactor(tracking); }

void drawCaps(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, juce::Justification j, float size, juce::Colour c, Face f) {
    g.setFont(caps(size, f)); g.setColour(c);
    g.drawText(s.toUpperCase(), r, j, true);
}

void drawShadow(juce::Graphics& g, juce::Rectangle<float> r, float radius, float k) {
    // two soft layers: a tight contact shadow and a wide ambient one
    for (int i = 6; i >= 1; i--) {
        g.setColour(juce::Colour(0xff1a2030).withAlpha(0.012f * k * (7 - i) / 3.0f));
        g.fillRoundedRectangle(r.translated(0, i * 0.8f).expanded(i * 0.9f), radius + i);
    }
    g.setColour(juce::Colour(0xff1a2030).withAlpha(0.05f * k));
    g.fillRoundedRectangle(r.translated(0, 1.0f), radius);
}

void drawModule(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, juce::Colour outline, const juce::String& code) {
    const float rad = 12.0f;
    drawShadow(g, r, rad);
    g.setColour(module); g.fillRoundedRectangle(r, rad);
    const bool tinted = !outline.isTransparent();
    g.setColour(tinted ? outline.withAlpha(0.85f) : hairline);
    g.drawRoundedRectangle(r.reduced(0.5f), rad, tinted ? 1.25f : 1.0f);
    if (title.isNotEmpty()) {
        auto t = r.withHeight((float)kModuleTitle).reduced(16, 0);
        if (tinted) {   // a short accent tick before the title
            g.setColour(outline); g.fillRoundedRectangle(t.getX(), t.getCentreY() - 1.0f, 8.0f, 2.0f, 1.0f);
            t.removeFromLeft(14);
        }
        drawCaps(g, title, t, juce::Justification::centredLeft, 10.5f, dim);
    }
    if (code.isNotEmpty()) {
        g.setFont(mono(10.0f)); g.setColour(faint);
        g.drawText(code.toUpperCase(), r.withHeight((float)kModuleTitle).reduced(16, 0), juce::Justification::centredRight, false);
    }
}

void drawWell(juce::Graphics& g, juce::Rectangle<float> r, float radius) {
    g.setColour(well); g.fillRoundedRectangle(r, radius);
    // a faint inner shade along the top edge sells the recess without any dark tones
    g.setGradientFill(juce::ColourGradient(juce::Colour(0x0a1a2030), r.getX(), r.getY(), juce::Colour(0x001a2030), r.getX(), r.getY() + 6, false));
    g.fillRoundedRectangle(r, radius);
    g.setColour(wellLine); g.drawRoundedRectangle(r.reduced(0.5f), radius, 1.0f);
}

void drawDot(juce::Graphics& g, juce::Point<float> c, float rad, bool on, juce::Colour a) {
    if (on) { g.setColour(a.withAlpha(0.18f)); g.fillEllipse(c.x - rad * 2.2f, c.y - rad * 2.2f, rad * 4.4f, rad * 4.4f); }
    g.setColour(on ? a : track); g.fillEllipse(c.x - rad, c.y - rad, rad * 2, rad * 2);
}

void drawWordmark(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& name, const juce::String& subtitle, juce::Colour a) {
    // accent mark: a small filled square, like a module status tile
    const float m = 10.0f;
    g.setColour(a); g.fillRoundedRectangle(r.getX(), r.getCentreY() - m / 2, m, m, 2.5f);
    auto t = r.withTrimmedLeft(m + 12);
    g.setFont(caps(11.0f, medium, 0.22f)); g.setColour(dim);
    const float lgnW = juce::GlyphArrangement::getStringWidth(caps(11.0f, medium, 0.22f), "LGN") + 10;
    g.drawText("LGN", t.withWidth(lgnW), juce::Justification::centredLeft, false);
    t.removeFromLeft(lgnW);
    const juce::Font nf = caps(22.0f, bold, 0.10f);
    g.setFont(nf); g.setColour(ink);
    const float nw = juce::GlyphArrangement::getStringWidth(nf, name.toUpperCase()) + 18;
    g.drawText(name.toUpperCase(), t.withWidth(nw), juce::Justification::centredLeft, false);
    t.removeFromLeft(nw);
    if (subtitle.isNotEmpty()) {
        g.setColour(hairline); g.fillRect(t.getX(), t.getCentreY() - 8, 1.0f, 16.0f);
        g.setFont(mono(11.0f)); g.setColour(dim);
        g.drawText(subtitle.toUpperCase(), t.withTrimmedLeft(14), juce::Justification::centredLeft, true);
    }
}

// ---------------------------------------------------------------- look and feel
Look::Look(juce::Colour a) : accent(a) {
    setColour(juce::ResizableWindow::backgroundColourId, hull);
    setColour(juce::Label::textColourId, ink);
    setColour(juce::TextEditor::backgroundColourId, well);
    setColour(juce::TextEditor::outlineColourId, wellLine);
    setColour(juce::TextEditor::focusedOutlineColourId, a);
    setColour(juce::TextEditor::textColourId, ink);
    setColour(juce::TextEditor::highlightColourId, a.withAlpha(0.22f));
    setColour(juce::TextEditor::highlightedTextColourId, ink);
    setColour(juce::CaretComponent::caretColourId, a);
    setColour(juce::ComboBox::backgroundColourId, well);
    setColour(juce::ComboBox::outlineColourId, wellLine);
    setColour(juce::ComboBox::textColourId, ink);
    setColour(juce::ComboBox::arrowColourId, dim);
    setColour(juce::PopupMenu::backgroundColourId, module);
    setColour(juce::PopupMenu::textColourId, ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, well);
    setColour(juce::PopupMenu::highlightedTextColourId, ink);
    setColour(juce::PopupMenu::headerTextColourId, dim);
    setColour(juce::ListBox::backgroundColourId, module);
    setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextButton::buttonColourId, module);
    setColour(juce::TextButton::buttonOnColourId, graphite);
    setColour(juce::TextButton::textColourOffId, ink);
    setColour(juce::TextButton::textColourOnId, ink);
    setColour(juce::ScrollBar::thumbColourId, faint);
    setColour(juce::Slider::textBoxTextColourId, ink);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, a.withAlpha(0.22f));
    setColour(juce::ToggleButton::textColourId, ink);
    setColour(juce::ToggleButton::tickColourId, a);
    setColour(juce::TooltipWindow::backgroundColourId, graphite);
    setColour(juce::TooltipWindow::textColourId, module);
    setColour(juce::TooltipWindow::outlineColourId, graphite);
}

juce::Typeface::Ptr Look::getTypefaceForFont(const juce::Font& f) {
    const auto name = f.getTypefaceName();
    if (name.isEmpty() || name == juce::Font::getDefaultSansSerifFontName()) return typeface(f.isBold() ? bold : medium);
    if (name == juce::Font::getDefaultMonospacedFontName()) return typeface(monoRegular);
    return LookAndFeel_V4::getTypefaceForFont(f);
}

// White disc with a hairline edge and a soft shadow, a thin track arc around it and the value in the accent colour.
void Look::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) {
    const auto r = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(3);
    const float R = juce::jmin(r.getWidth(), r.getHeight()) / 2, cx = r.getCentreX(), cy = r.getCentreY();
    const bool bipolar = (bool)s.getProperties().getWithDefault("bipolar", s.getMinimum() < 0 && s.getMaximum() > 0);
    const bool enabled = s.isEnabled();
    const float ang = a0 + pos * (a1 - a0), zero = bipolar ? (a0 + a1) / 2 : a0;
    const juce::Colour ac = s.getProperties().contains("accent") ? juce::Colour((juce::uint32)(juce::int64)s.getProperties()["accent"]) : accent;
    const float stroke = R > 34 ? 3.0f : 2.5f, ringR = R - stroke / 2;
    juce::Path trackArc; trackArc.addCentredArc(cx, cy, ringR, ringR, 0, a0, a1, true);
    g.setColour(track); g.strokePath(trackArc, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (enabled && std::abs(ang - zero) > 0.001f) {
        juce::Path v; v.addCentredArc(cx, cy, ringR, ringR, 0, juce::jmin(zero, ang), juce::jmax(zero, ang), true);
        g.setColour(ac); g.strokePath(v, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    // knob body
    const float kr = R - stroke - juce::jmax(4.0f, R * 0.14f);
    const juce::Rectangle<float> body(cx - kr, cy - kr, kr * 2, kr * 2);
    for (int i = 5; i >= 1; i--) { g.setColour(juce::Colour(0xff1a2030).withAlpha(0.018f * (6 - i))); g.fillEllipse(body.translated(0, i * 0.7f).expanded(i * 0.5f)); }
    g.setGradientFill(juce::ColourGradient(juce::Colours::white, cx, cy - kr, juce::Colour(0xfff3f5f7), cx, cy + kr, false));
    g.fillEllipse(body);
    g.setColour(s.isMouseOverOrDragging() && enabled ? hairlineHi : hairline); g.drawEllipse(body.reduced(0.5f), 1.0f);
    // pointer
    const float p0 = kr * 0.30f, p1 = kr * 0.80f;
    g.setColour(enabled ? ink : faint);
    g.drawLine(cx + p0 * std::sin(ang), cy - p0 * std::cos(ang), cx + p1 * std::sin(ang), cy - p1 * std::cos(ang), R > 34 ? 2.2f : 1.8f);
}

void Look::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle style, juce::Slider& s) {
    const bool vert = style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical;
    const float t = 4.0f;
    juce::Rectangle<float> tr = vert ? juce::Rectangle<float>(x + w * 0.5f - t / 2, (float)y + 6, t, (float)h - 12)
                                     : juce::Rectangle<float>((float)x + 6, y + h * 0.5f - t / 2, (float)w - 12, t);
    g.setColour(track); g.fillRoundedRectangle(tr, t / 2);
    const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0;
    if (s.isEnabled()) {
        g.setColour(accent);
        if (vert) {
            const float z = bipolar ? tr.getCentreY() : tr.getBottom();
            g.fillRoundedRectangle(tr.getX(), juce::jmin(z, pos), t, std::abs(z - pos), t / 2);
        } else {
            const float z = bipolar ? tr.getCentreX() : tr.getX();
            g.fillRoundedRectangle(juce::jmin(z, pos), tr.getY(), std::abs(pos - z), t, t / 2);
        }
    }
    const float tw = 14.0f;
    const auto th = vert ? juce::Rectangle<float>(tr.getCentreX() - tw / 2, pos - tw / 2, tw, tw) : juce::Rectangle<float>(pos - tw / 2, tr.getCentreY() - tw / 2, tw, tw);
    for (int i = 3; i >= 1; i--) { g.setColour(juce::Colour(0xff1a2030).withAlpha(0.03f * (4 - i))); g.fillEllipse(th.translated(0, i * 0.6f).expanded(i * 0.4f)); }
    g.setColour(module); g.fillEllipse(th);
    g.setColour(s.isMouseOverOrDragging() ? hairlineHi : hairline); g.drawEllipse(th.reduced(0.5f), 1.0f);
    g.setColour(s.isEnabled() ? accent : faint); g.fillEllipse(th.reduced(4.5f));
}

juce::Label* Look::createSliderTextBox(juce::Slider& s) {
    auto* l = LookAndFeel_V4::createSliderTextBox(s);
    l->setFont(mono(12.0f)); l->setColour(juce::Label::textColourId, ink);
    l->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour(juce::Label::backgroundWhenEditingColourId, well);
    l->setColour(juce::Label::outlineWhenEditingColourId, accent);
    return l;
}

void Look::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) {
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    const float rad = juce::jmin(8.0f, r.getHeight() / 2);
    const bool on = b.getToggleState(), led = (bool)b.getProperties().getWithDefault("led", false);
    const bool accentTone = b.getProperties().getWithDefault("tone", "").toString() == "accent";
    if (led) {
        if (on) {
            g.setColour(graphite); g.fillRoundedRectangle(r, rad);
            drawDot(g, { r.getX() + 12.0f, r.getCentreY() }, 2.5f, true, accent);
        } else {
            if (over) { g.setColour(juce::Colour(0x0a1a2030)); g.fillRoundedRectangle(r, rad); }
        }
        return;
    }
    if (!down) drawShadow(g, r, rad, 0.5f);
    juce::Colour fill = down ? well : module;
    if (accentTone) fill = down ? accent.darker(0.15f) : (over ? accent.brighter(0.08f) : accent);
    g.setColour(fill); g.fillRoundedRectangle(r, rad);
    if (on && !accentTone) { g.setColour(accent.withAlpha(0.10f)); g.fillRoundedRectangle(r, rad); }
    g.setColour(accentTone ? fill.darker(0.1f) : (on ? accent : (over ? hairlineHi : hairline)));
    g.drawRoundedRectangle(r.reduced(0.5f), rad, on ? 1.2f : 1.0f);
}

juce::Font Look::getTextButtonFont(juce::TextButton&, int h) { return caps(juce::jmin(12.5f, h * 0.42f), medium, 0.10f); }

void Look::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) {
    const bool led = (bool)b.getProperties().getWithDefault("led", false), on = b.getToggleState();
    const bool accentTone = b.getProperties().getWithDefault("tone", "").toString() == "accent";
    auto r = b.getLocalBounds().reduced(6, 2);
    if (led && on) r = r.withTrimmedLeft(10);
    g.setFont(getTextButtonFont(b, b.getHeight()));
    juce::Colour c = led ? (on ? module : dim) : (accentTone ? module : ink);
    if (!b.isEnabled()) c = faint;
    g.setColour(c);
    g.drawFittedText(b.getButtonText().toUpperCase(), r, juce::Justification::centred, 1, 0.75f);
}

// A small switch: grey track when off, accent when on, white thumb.
void Look::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool) {
    const float h = (float)b.getHeight(), th = juce::jmin(16.0f, h - 6), tw = th * 1.75f;
    const juce::Rectangle<float> tr(2, (h - th) / 2, tw, th);
    const bool on = b.getToggleState();
    g.setColour(on ? accent : (over ? hairlineHi : track)); g.fillRoundedRectangle(tr, th / 2);
    const float d = th - 4;
    const juce::Rectangle<float> knob(on ? tr.getRight() - d - 2 : tr.getX() + 2, tr.getY() + 2, d, d);
    g.setColour(juce::Colour(0xff1a2030).withAlpha(0.15f)); g.fillEllipse(knob.translated(0, 0.8f));
    g.setColour(module); g.fillEllipse(knob);
    g.setFont(caps(11.5f, medium, 0.08f));
    g.setColour(b.isEnabled() ? ink : faint);
    g.drawFittedText(b.getButtonText().toUpperCase(), b.getLocalBounds().withTrimmedLeft((int)tw + 10), juce::Justification::centredLeft, 2, 0.8f);
}

void Look::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) {
    const juce::Rectangle<float> r(0.5f, 0.5f, (float)w - 1, (float)h - 1);
    drawWell(g, r, juce::jmin(8.0f, r.getHeight() / 2));
    if (box.hasKeyboardFocus(true) || box.isMouseOver(true)) { g.setColour(hairlineHi); g.drawRoundedRectangle(r.reduced(0.5f), juce::jmin(8.0f, r.getHeight() / 2), 1.0f); }
    const float cx = r.getRight() - 14, cy = r.getCentreY();
    juce::Path chev; chev.startNewSubPath(cx - 4, cy - 2); chev.lineTo(cx, cy + 2); chev.lineTo(cx + 4, cy - 2);
    g.setColour(box.isEnabled() ? dim : faint); g.strokePath(chev, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
juce::Font Look::getComboBoxFont(juce::ComboBox&) { return juce::Font(sans(13.5f, medium)); }
void Look::positionComboBoxText(juce::ComboBox& box, juce::Label& l) { l.setBounds(8, 1, box.getWidth() - 30, box.getHeight() - 2); l.setFont(getComboBoxFont(box)); }
juce::Font Look::getPopupMenuFont() { return juce::Font(sans(14.0f, regular)); }
void Look::drawPopupMenuBackground(juce::Graphics& g, int w, int h) {
    g.fillAll(module);
    g.setColour(hairline); g.drawRect(0, 0, w, h, 1);
}
void Look::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool sep, bool active, bool hi, bool ticked, bool sub,
                             const juce::String& text, const juce::String& shortcut, const juce::Drawable*, const juce::Colour*) {
    if (sep) { g.setColour(hairline); g.fillRect(area.reduced(10, 0).withHeight(1).withY(area.getCentreY())); return; }
    auto r = area.reduced(4, 1);
    if (hi && active) { g.setColour(well); g.fillRoundedRectangle(r.toFloat(), 6); }
    if (ticked) drawDot(g, { (float)r.getX() + 10, (float)r.getCentreY() }, 3.0f, true, accent);
    g.setFont(getPopupMenuFont()); g.setColour(active ? ink : faint);
    g.drawFittedText(text, r.withTrimmedLeft(22).withTrimmedRight(18), juce::Justification::centredLeft, 1);
    if (shortcut.isNotEmpty()) { g.setFont(mono(11.0f)); g.setColour(dim); g.drawText(shortcut, r.withTrimmedRight(8), juce::Justification::centredRight); }
    if (sub) { const float cx = (float)r.getRight() - 10, cy = (float)r.getCentreY(); juce::Path p; p.startNewSubPath(cx - 2, cy - 4); p.lineTo(cx + 2, cy); p.lineTo(cx - 2, cy + 4); g.setColour(dim); g.strokePath(p, juce::PathStrokeType(1.4f)); }
}
void Look::fillTextEditorBackground(juce::Graphics& g, int w, int h, juce::TextEditor&) { drawWell(g, { 0.5f, 0.5f, (float)w - 1, (float)h - 1 }, juce::jmin(8.0f, h / 2.0f)); }
void Look::drawTextEditorOutline(juce::Graphics& g, int w, int h, juce::TextEditor& e) {
    if (e.hasKeyboardFocus(true)) { g.setColour(accent); g.drawRoundedRectangle(0.5f, 0.5f, (float)w - 1, (float)h - 1, juce::jmin(8.0f, h / 2.0f), 1.2f); }
}
void Look::drawLabel(juce::Graphics& g, juce::Label& l) {
    if (l.isBeingEdited()) return;
    const bool upper = (bool)l.getProperties().getWithDefault("caps", false);
    juce::Font f = getLabelFont(l);
    if (upper) f = f.withExtraKerningFactor(0.12f);
    g.setFont(f);
    g.setColour(l.findColour(juce::Label::textColourId).withMultipliedAlpha(l.isEnabled() ? 1.0f : 0.5f));
    g.drawFittedText(upper ? l.getText().toUpperCase() : l.getText(), l.getBorderSize().subtractedFrom(l.getLocalBounds()), l.getJustificationType(), 2, l.getMinimumHorizontalScale());
}
void Look::drawScrollbar(juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int ts, int tsz, bool over, bool down) {
    const juce::Rectangle<float> trk((float)x + 2, (float)y + 2, (float)w - 4, (float)h - 4);
    const auto th = vertical ? juce::Rectangle<float>(trk.getX(), (float)ts, trk.getWidth(), (float)tsz) : juce::Rectangle<float>((float)ts, trk.getY(), (float)tsz, trk.getHeight());
    g.setColour(over || down ? dim : faint.withAlpha(0.8f)); g.fillRoundedRectangle(th.reduced(0.5f), 2.5f);
}
void Look::drawTooltip(juce::Graphics& g, const juce::String& text, int w, int h) {
    g.setColour(graphite); g.fillRoundedRectangle(0, 0, (float)w, (float)h, 6);
    g.setFont(sans(12.5f, regular)); g.setColour(module);
    g.drawFittedText(text, juce::Rectangle<int>(w, h).reduced(10, 4), juce::Justification::centredLeft, 4);
}
juce::Rectangle<int> Look::getTooltipBounds(const juce::String& tip, juce::Point<int> p, juce::Rectangle<int> parent) {
    const int w = juce::jmin(320, juce::roundToInt(juce::GlyphArrangement::getStringWidth(juce::Font(sans(12.5f, regular)), tip)) + 22), h = w >= 320 ? 46 : 26;
    return juce::Rectangle<int>(p.x > parent.getCentreX() ? p.x - (w + 12) : p.x + 24, p.y > parent.getCentreY() ? p.y - (h + 6) : p.y + 6, w, h).constrainedWithin(parent);
}
void Look::drawCornerResizer(juce::Graphics& g, int w, int h, bool over, bool drag) {
    g.setColour(over || drag ? dim : faint);
    for (int i = 0; i < 3; i++) { const float o = 4.0f + i * 4.0f; g.drawLine((float)w - o, (float)h - 2, (float)w - 2, (float)h - o, 1.2f); }
}
} // namespace station
