#include "Hardware.h"
#include "BinaryData.h"
#include <map>

namespace ui {

const juce::Typeface::Ptr& typeface(int which) {
    static const juce::Typeface::Ptr faces[3] = {
        juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensed_500Medium_ttf, (size_t)BinaryData::BarlowCondensed_500Medium_ttfSize),
        juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensed_700Bold_ttf, (size_t)BinaryData::BarlowCondensed_700Bold_ttfSize),
        juce::Typeface::createSystemTypefaceFor(BinaryData::ShareTechMono_400Regular_ttf, (size_t)BinaryData::ShareTechMono_400Regular_ttfSize) };
    return faces[juce::jlimit(0, 2, which)];
}

static float hashf(uint32_t x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return (x & 0xffffff) / 16777215.0f; }

const juce::Image& anodizedTile() {
    static const juce::Image img = [] {
        const int N = 256; juce::Image im(juce::Image::RGB, N, N, false);
        juce::Image::BitmapData bd(im, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
            // fine grain plus a faint horizontal brushing, all periodic so the tile repeats
            const float grain = hashf((uint32_t)(x * 7919 + y * 104729)) - 0.5f;
            const float row = hashf((uint32_t)(y * 2654435761u)) - 0.5f;
            const float v = 38.0f + grain * 7.0f + row * 4.0f;
            bd.setPixelColour(x, y, juce::Colour::fromRGB((juce::uint8)juce::jlimit(0.0f, 255.0f, v), (juce::uint8)juce::jlimit(0.0f, 255.0f, v + 2), (juce::uint8)juce::jlimit(0.0f, 255.0f, v + 5)));
        }
        return im;
    }();
    return img;
}

juce::Image brushed(int w, int h, juce::Colour base) {
    w = juce::jmax(1, w); h = juce::jmax(1, h);
    juce::Image im(juce::Image::RGB, w, h, false);
    juce::Image::BitmapData bd(im, juce::Image::BitmapData::writeOnly);
    std::vector<float> row((size_t)w);
    for (int y = 0; y < h; y++) {
        // long horizontal streaks: random walk along the row, smoothed
        float acc = 0, sm = 0;
        for (int x = 0; x < w; x++) { acc = acc * 0.985f + (hashf((uint32_t)(y * 92821 + x * 31)) - 0.5f) * 0.35f; sm += (acc - sm) * 0.2f; row[(size_t)x] = sm; }
        const float rowTone = (hashf((uint32_t)(y * 15485863u)) - 0.5f) * 0.05f;
        for (int x = 0; x < w; x++) {
            const float sheen = 0.06f * std::sin(juce::MathConstants<float>::pi * (float)x / (float)w * 1.0f);   // soft broad highlight
            const float k = 1.0f + rowTone + row[(size_t)x] * 0.09f + sheen + (hashf((uint32_t)(x * 7 + y * 131)) - 0.5f) * 0.03f;
            bd.setPixelColour(x, y, juce::Colour::fromFloatRGBA(juce::jlimit(0.0f, 1.0f, base.getFloatRed() * k), juce::jlimit(0.0f, 1.0f, base.getFloatGreen() * k), juce::jlimit(0.0f, 1.0f, base.getFloatBlue() * k), 1.0f));
        }
    }
    return im;
}

juce::Image walnut(int w, int h) {
    w = juce::jmax(1, w); h = juce::jmax(1, h);
    juce::Image im(juce::Image::RGB, w, h, false);
    juce::Image::BitmapData bd(im, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        // grain runs vertically; rings wobble with a slow noise along y
        const float wob = 3.0f * std::sin(y * 0.013f) + 1.5f * std::sin(y * 0.041f + 1.3f) + 0.8f * std::sin(y * 0.11f);
        const float ring = std::sin((x + wob) * 0.9f + std::sin(y * 0.02f) * 2.0f);
        const float fleck = hashf((uint32_t)(x * 131 + y * 7)) - 0.5f;
        const float t = 0.5f + 0.25f * ring + 0.12f * std::sin(y * 0.6f + x) * 0.2f + fleck * 0.1f;
        const juce::Colour dark(0xff2a170c), light(0xff6b3f22);
        bd.setPixelColour(x, y, dark.interpolatedWith(light, juce::jlimit(0.0f, 1.0f, t)));
    }
    return im;
}

const juce::Image& spunCap(int d) {
    static std::map<int, juce::Image> cache;
    d = juce::jlimit(8, 400, d);
    auto it = cache.find(d); if (it != cache.end()) return it->second;
    juce::Image im(juce::Image::ARGB, d, d, true);
    {
        juce::Image::BitmapData bd(im, juce::Image::BitmapData::writeOnly);
        const float c = (d - 1) * 0.5f, R = d * 0.5f;
        for (int y = 0; y < d; y++) for (int x = 0; x < d; x++) {
            const float dx = x - c, dy = y - c, r = std::sqrt(dx * dx + dy * dy);
            if (r > R) continue;
            const float a = std::atan2(dy, dx);
            // turned metal: anisotropic highlight bands at +/-45 degrees, fine concentric rings, darker rim
            const float band = 0.5f + 0.5f * std::cos(2.0f * (a + 0.785f));
            const float rings = 0.03f * std::sin(r * 2.2f) + (hashf((uint32_t)(int)(r * 8)) - 0.5f) * 0.05f;
            const float rim = juce::jlimit(0.0f, 1.0f, (R - r) / 2.0f);
            float v = 0.55f + 0.38f * band * band + rings - 0.16f * (r / R) * (r / R);
            v = juce::jlimit(0.0f, 1.0f, v);
            bd.setPixelColour(x, y, juce::Colour::fromFloatRGBA(v * 0.97f, v * 0.98f, v, rim));
        }
    }
    return cache.emplace(d, im).first->second;
}

void drawBrushedPlate(juce::Graphics& g, juce::Rectangle<float> r, bool screws) {
    static juce::Image cached; static juce::Rectangle<int> size;
    if (size != r.toNearestInt().withZeroOrigin()) { size = r.toNearestInt().withZeroOrigin(); cached = brushed(size.getWidth(), size.getHeight(), juce::Colour(0xffb9bcc0)); }
    g.setColour(juce::Colours::black.withAlpha(0.55f)); g.fillRoundedRectangle(r.translated(0, 3), 5);
    juce::Path clip; clip.addRoundedRectangle(r, 5);
    { juce::Graphics::ScopedSaveState ss(g); g.reduceClipRegion(clip); g.drawImageAt(cached, (int)r.getX(), (int)r.getY()); }
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.25f), r.getX(), r.getY(), juce::Colours::black.withAlpha(0.18f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 5);
    g.setColour(juce::Colours::white.withAlpha(0.6f)); g.drawLine(r.getX() + 5, r.getY() + 0.7f, r.getRight() - 5, r.getY() + 0.7f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRoundedRectangle(r.reduced(0.5f), 5, 1.0f);
    if (screws) {
        drawScrew(g, r.getX() + 11, r.getY() + 11, 4.2f, 0.6f); drawScrew(g, r.getRight() - 11, r.getY() + 11, 4.2f, 2.1f);
        drawScrew(g, r.getX() + 11, r.getBottom() - 11, 4.2f, 1.2f); drawScrew(g, r.getRight() - 11, r.getBottom() - 11, 4.2f, 0.2f);
    }
}

void drawWoodCheek(juce::Graphics& g, juce::Rectangle<float> r) {
    static juce::Image cached; static juce::Rectangle<int> size;
    if (size != r.toNearestInt().withZeroOrigin()) { size = r.toNearestInt().withZeroOrigin(); cached = walnut(size.getWidth(), size.getHeight()); }
    g.drawImageAt(cached, (int)r.getX(), (int)r.getY());
    // satin finish: soft vertical highlight and darker edges
    juce::ColourGradient cg(juce::Colours::black.withAlpha(0.45f), r.getX(), 0, juce::Colours::black.withAlpha(0.45f), r.getRight(), 0, false);
    cg.addColour(0.4, juce::Colours::white.withAlpha(0.10f));
    g.setGradientFill(cg); g.fillRect(r);
}

void drawScrew(juce::Graphics& g, float cx, float cy, float r, float angle) {
    g.setColour(juce::Colours::black.withAlpha(0.55f)); g.fillEllipse(cx - r - 1, cy - r, (r + 1) * 2, (r + 1) * 2);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff9a9fa6), cx - r, cy - r, juce::Colour(0xff3c4046), cx + r, cy + r, false));
    g.fillEllipse(cx - r, cy - r, r * 2, r * 2);
    g.setColour(juce::Colours::white.withAlpha(0.5f)); g.fillEllipse(cx - r * 0.55f, cy - r * 0.7f, r * 0.6f, r * 0.45f);
    juce::Path slot; slot.addRectangle(-r * 0.7f, -r * 0.13f, r * 1.4f, r * 0.26f); slot.addRectangle(-r * 0.13f, -r * 0.7f, r * 0.26f, r * 1.4f);
    g.setColour(juce::Colour(0xff17191b)); g.fillPath(slot, juce::AffineTransform::rotation(angle).translated(cx, cy));
    g.setColour(juce::Colours::white.withAlpha(0.25f)); g.fillPath(slot, juce::AffineTransform::rotation(angle).translated(cx + 0.6f, cy + 0.6f));
    g.setColour(juce::Colour(0xff17191b)); g.fillPath(slot, juce::AffineTransform::rotation(angle).translated(cx, cy));
}

void drawPlate(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, bool screws) {
    // drop shadow, body with a faint vertical sheen, bevel, then a groove around the edge
    g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillRoundedRectangle(r.translated(0, 2), 4);
    for (int i = 4; i >= 1; i--) { g.setColour(juce::Colours::black.withAlpha(0.07f)); g.fillRoundedRectangle(r.translated(0, (float)i).expanded((float)i * 0.6f), 4 + i * 0.5f); }   // soft shadow
    g.setTiledImageFill(anodizedTile(), 0, 0, 1.0f); g.fillRoundedRectangle(r, 4);
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.05f), r.getX(), r.getY(), juce::Colours::black.withAlpha(0.25f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 4);
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
    for (int i = 0; i < 5; i++) { g.setColour(juce::Colours::black.withAlpha(0.16f - i * 0.03f)); g.drawRoundedRectangle(r.reduced((float)i + 0.5f).withTrimmedBottom(-(float)i), radius, 1.0f); }
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.7f), r.getX(), r.getY(), juce::Colours::transparentBlack, r.getX(), r.getY() + 8, false));
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
    // backlight bloom from the centre
    g.setGradientFill(juce::ColourGradient(juce::Colour(0x30ff9a30), r.getCentreX(), r.getCentreY(), juce::Colour(0x00ff9a30), r.getX(), r.getY(), true));
    g.fillRoundedRectangle(r, 3);
    // pixel matrix
    g.setColour(juce::Colours::black.withAlpha(0.28f));
    for (float y = r.getY(); y < r.getBottom(); y += 2) g.drawHorizontalLine((int)y, r.getX(), r.getRight());
    g.setColour(juce::Colours::black.withAlpha(0.14f));
    for (float x = r.getX(); x < r.getRight(); x += 2) g.drawVerticalLine((int)x, r.getY(), r.getBottom());
    // glass: diagonal reflection and inner edge shadow
    juce::Path refl; refl.startNewSubPath(r.getX(), r.getY()); refl.lineTo(r.getX() + r.getWidth() * 0.55f, r.getY()); refl.lineTo(r.getX() + r.getWidth() * 0.35f, r.getBottom()); refl.lineTo(r.getX(), r.getBottom()); refl.closeSubPath();
    g.setColour(juce::Colours::white.withAlpha(0.035f)); g.fillPath(refl);
    for (int i = 0; i < 4; i++) { g.setColour(juce::Colours::black.withAlpha(0.35f - i * 0.08f)); g.drawRoundedRectangle(r.reduced((float)i), 3, 1.0f); }
}

void drawEngraved(juce::Graphics& g, const juce::String& s, juce::Rectangle<int> r, juce::Justification j, float size) {
    const juce::Font f = juce::Font(font(size, true)).withExtraKerningFactor(0.12f);
    g.setFont(f);
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.drawText(s, r.translated(0, 1), j, false);
    g.setColour(text.withAlpha(0.78f)); g.drawText(s, r, j, false);
}
void drawPrinted(juce::Graphics& g, const juce::String& s, juce::Rectangle<int> r, juce::Justification j, float size, juce::Colour ink) {
    g.setFont(juce::Font(font(size, true)).withExtraKerningFactor(0.12f));
    g.setColour(ink); g.drawText(s, r, j, false);
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
    // knob body: soft contact shadow, black rubber skirt with grip ridges, turned aluminium cap
    const float kr = R - (R > 30 ? 9.0f : 7.5f);
    for (int k = 6; k >= 1; k--) { g.setColour(juce::Colours::black.withAlpha(0.06f)); g.fillEllipse(cx - kr - k * 0.6f, cy - kr + k * 0.9f, (kr + k * 0.6f) * 2, (kr + k * 0.6f) * 2); }
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff34373b), cx, cy - kr, juce::Colour(0xff0d0e10), cx, cy + kr, false));
    g.fillEllipse(cx - kr, cy - kr, kr * 2, kr * 2);
    const int teeth = juce::jmax(18, (int)(kr * 1.9f));
    for (int i = 0; i < teeth; i++) {
        const float a = juce::MathConstants<float>::twoPi * i / teeth + ang;
        const float lit = 0.5f + 0.5f * std::cos(a + 0.8f);       // ridges catch light on the upper left
        g.setColour(juce::Colours::white.withAlpha(0.05f + 0.10f * lit));
        g.drawLine(cx + (kr - 3.5f) * std::sin(a), cy - (kr - 3.5f) * std::cos(a), cx + (kr - 0.6f) * std::sin(a), cy - (kr - 0.6f) * std::cos(a), 1.1f);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        const float b = a + juce::MathConstants<float>::pi / teeth;
        g.drawLine(cx + (kr - 3.5f) * std::sin(b), cy - (kr - 3.5f) * std::cos(b), cx + (kr - 0.6f) * std::sin(b), cy - (kr - 0.6f) * std::cos(b), 1.0f);
    }
    g.setColour(juce::Colours::black.withAlpha(0.8f)); g.drawEllipse(cx - kr, cy - kr, kr * 2, kr * 2, 1.0f);
    const float cr = kr * 0.72f;
    g.setColour(juce::Colours::black.withAlpha(0.55f)); g.fillEllipse(cx - cr - 1, cy - cr - 0.5f, (cr + 1) * 2, (cr + 1) * 2 + 1);   // bevel under the cap
    const int dia = (int)std::round(cr * 2);
    g.drawImage(spunCap(dia), juce::Rectangle<float>(cx - cr, cy - cr, cr * 2, cr * 2), juce::RectanglePlacement::stretchToFit);
    if (!enabled) { g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillEllipse(cx - cr, cy - cr, cr * 2, cr * 2); }
    // engraved indicator on the cap, and a painted line on the skirt
    juce::Path tick; tick.addRoundedRectangle(-1.3f, -cr + 2.0f, 2.6f, cr * 0.6f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.75f)); g.fillPath(tick, juce::AffineTransform::rotation(ang).translated(cx, cy));
    juce::Path mark; mark.addRoundedRectangle(-1.2f, -kr + 0.8f, 2.4f, kr - cr - 0.8f, 0.8f);
    g.setColour(enabled ? accent.brighter(0.2f) : dim); g.fillPath(mark, juce::AffineTransform::rotation(ang).translated(cx, cy));
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
    juce::Colour top = over ? juce::Colour(0xff3d4045) : juce::Colour(0xff35383c), bot = juce::Colour(0xff1c1e21);
    if (down) { top = juce::Colour(0xff222427); bot = juce::Colour(0xff2c2f33); }
    g.setGradientFill(juce::ColourGradient(top, body.getX(), body.getY(), bot, body.getX(), body.getBottom(), false));
    g.fillRoundedRectangle(body, 4);
    // rubber texture and a soft top gloss
    { juce::Graphics::ScopedSaveState ss(g); juce::Path cl; cl.addRoundedRectangle(body, 4); g.reduceClipRegion(cl);
      g.setTiledImageFill(ui::anodizedTile(), 0, 0, 0.35f); g.fillRect(body);
      if (!down) { g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.13f), body.getX(), body.getY(), juce::Colours::transparentWhite, body.getX(), body.getY() + body.getHeight() * 0.5f, false)); g.fillRect(body); } }
    g.setColour(juce::Colours::white.withAlpha(down ? 0.02f : 0.18f)); g.drawLine(body.getX() + 4, body.getY() + 0.8f, body.getRight() - 4, body.getY() + 0.8f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.7f)); g.drawRoundedRectangle(body, 4, 1.0f);
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
