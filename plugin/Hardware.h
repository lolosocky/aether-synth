// Aether's visual language: the shared Station look (white modules, hairlines, soft shadows) with Aether's
// mission-orange accent. The ui:: names are kept so every page draws through one place.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Station.h"

namespace ui {
// palette
const juce::Colour bg = station::hull,          // window
    panel = station::module,                    // raised modules
    line = station::hairline,                   // edges and separators
    text = station::ink,                        // primary text
    dim = station::dim,                         // secondary text
    accent = station::accents::aether,          // mission orange
    accent2 = station::graphite,                // waveforms and secondary marks
    lcdBg = station::well, lcdText = station::ink, lcdDim = station::faint;

inline juce::FontOptions font(float h, bool bold = false) { return station::sans(h, bold ? station::bold : station::medium); }
inline juce::FontOptions mono(float h) { return station::mono(h); }

inline void drawPlate(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title = {}, bool = false) { station::drawModule(g, r, title); }
inline void drawRecess(juce::Graphics& g, juce::Rectangle<float> r, float radius = 8.0f) { station::drawWell(g, r, juce::jmax(radius, 6.0f)); }
inline void drawLcd(juce::Graphics& g, juce::Rectangle<float> r) { station::drawWell(g, r, 10.0f); }
inline void drawLed(juce::Graphics& g, juce::Rectangle<float> r, bool on, juce::Colour c = accent) { station::drawDot(g, r.getCentre(), juce::jmin(r.getWidth(), r.getHeight()) * 0.5f + 1, on, c); }
inline void drawEngraved(juce::Graphics& g, const juce::String& s, juce::Rectangle<int> r, juce::Justification j, float size = 12.0f) { station::drawCaps(g, s, r.toFloat(), j, size * 0.85f, dim); }
inline void drawPrinted(juce::Graphics& g, const juce::String& s, juce::Rectangle<int> r, juce::Justification j, float size, juce::Colour ink) { station::drawCaps(g, s, r.toFloat(), j, size, ink); }
}

class AetherLook : public station::Look {
public:
    AetherLook() : station::Look(ui::accent) {}
};
