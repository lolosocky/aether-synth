// LGN Distance: one knob from in your ear to across the street. Drag the sound around a map of the space.
#include "../common/Suite.h"
#include "../dsp/Distance.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

const juce::Colour kAccent{ 0xff3f8f7a };   // fog green
juce::String metresText(double m) { return m < 1 ? juce::String(juce::roundToInt(m * 100)) + " cm" : m < 10 ? juce::String(m, 1) + " m" : juce::String(juce::roundToInt(m)) + " m"; }
juce::StringArray spaceNames() { juce::StringArray a; for (int i = 0; i < DistanceDsp::numSpaces; i++) a.add(DistanceDsp::space(i).name); return a; }

class DistanceProcessor : public Processor {
public:
    DistanceProcessor() : Processor("LGNDistance", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "distance", 1 }, "Distance", juce::NormalisableRange<float>(0, 1, 0.0001f), 0.35f,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction([](float v, int) { return metresText(DistanceDsp::metres(v)); })
                .withValueFromStringFunction([](const juce::String& t) {
                    const double v = t.getDoubleValue(); return (float)DistanceDsp::knobFor(t.containsIgnoreCase("cm") ? v / 100 : v); })));
        param::add(l, "angle", "Angle", { -90, 90, 0.1f }, 0, "deg", 0);
        param::addChoice(l, "space", "Space", spaceNames(), 1);
        param::addPct(l, "keep", "Keep Level", 50);
        param::add(l, "air", "Air", { 0, 150, 0.1f }, 100, "%", 0);
        param::addPct(l, "doppler", "Doppler", 50);
        param::addPct(l, "width", "Width", 100);
        param::addDb(l, "out", "Output", -12, 12, 0);
        return l;
    }
    DistanceParams params() const {
        DistanceParams d; d.distance = p("distance"); d.angleDeg = p("angle"); d.space = juce::roundToInt(p("space")); d.keepLevel = p("keep") / 100.0;
        d.air = p("air") / 100.0; d.doppler = p("doppler") / 100.0; d.width = p("width") / 100.0; d.outDb = p("out");
        return d;
    }
    void prepareToPlay(double sr, int) override { dsp.set(params()); dsp.prepare(sr); }
    void reset() override { dsp.reset(); }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples(), ch = juce::jmin(2, getTotalNumInputChannels());
        for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); c++) buf.clear(c, 0, n);
        meterInput(buf, n);
        dsp.set(params());
        float* ptrs[2] = { buf.getWritePointer(0), ch > 1 ? buf.getWritePointer(1) : nullptr };
        dsp.process(ptrs, ch, n);
        uiMetres = (float)dsp.currentMetres();
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 4.0; }
    juce::AudioProcessorEditor* createEditor() override;
    std::atomic<float> uiMetres{ 1 };
private:
    DistanceDsp dsp;
};

// Top-down map: you at the bottom, the sound anywhere in front. Distance runs outward on a perceptual (log) scale,
// angle is the direction. Drag the sound to move it; rings mark 1, 5, 20 and 100 m.
class MapView : public juce::Component {
public:
    MapView(DistanceProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    void tick() { level = juce::jmax(proc.outMeter.peak.load(), level * 0.85f); repaint(); }
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        geo();
        const auto sp = DistanceDsp::space(juce::roundToInt(proc.p("space")));
        // the space: walls for indoor spaces (sized to the room), a horizon for outdoor ones
        if (sp.size > 0 && juce::roundToInt(proc.p("space")) != DistanceDsp::street) {
            const float w = radiusFor(sp.size * 0.9), d = radiusFor(sp.size * 0.7);
            g.setColour(hairline); g.drawRoundedRectangle(juce::Rectangle<float>(centre.x - w, centre.y - d, w * 2, d + 18), 8, 1.2f);
        }
        station::drawCaps(g, sp.name, juce::Rectangle<float>(r.getX() + 14, r.getY() + 10, 160, 14), juce::Justification::centredLeft, 9.5f, dim);
        // rings
        for (double m : { 1.0, 5.0, 20.0, 100.0 }) {
            const float rad = radiusFor(m);
            juce::Path arc; arc.addCentredArc(centre.x, centre.y, rad, rad, 0, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
            g.setColour(hairline); g.strokePath(arc, juce::PathStrokeType(1.0f));
            g.setFont(station::mono(9.0f)); g.setColour(faint);
            g.drawText(metresText(m), juce::Rectangle<float>(centre.x + 4, centre.y - rad - 13, 50, 12), juce::Justification::centredLeft);
        }
        // the listener
        g.setColour(station::graphite); g.fillEllipse(centre.x - 9, centre.y - 9, 18, 18);
        g.setColour(juce::Colours::white); g.fillEllipse(centre.x - 3, centre.y - 6, 6, 4);
        g.setColour(station::graphite); g.fillEllipse(centre.x - 12, centre.y - 3, 4, 7); g.fillEllipse(centre.x + 8, centre.y - 3, 4, 7);
        station::drawCaps(g, "You", juce::Rectangle<float>(centre.x - 20, centre.y + 11, 40, 12), juce::Justification::centred, 8.5f, faint);
        // the sound: where it is (smoothed, so movement shows) with rings that pulse with its level
        const auto pos = pointFor(proc.uiMetres.load(), proc.p("angle"));
        g.setColour(accent.withAlpha(0.25f)); g.drawLine(centre.x, centre.y, pos.x, pos.y, 1.0f);
        const float pulse = 10 + 26 * juce::jlimit(0.0f, 1.0f, level * 1.4f);
        g.setColour(accent.withAlpha(0.12f)); g.fillEllipse(pos.x - pulse, pos.y - pulse, pulse * 2, pulse * 2);
        g.setColour(accent); g.fillEllipse(pos.x - 8, pos.y - 8, 16, 16);
        g.setColour(juce::Colours::white); g.fillEllipse(pos.x - 3, pos.y - 3, 6, 6);
        g.setFont(station::mono(11.0f, true)); g.setColour(ink);
        g.drawText(metresText(proc.uiMetres.load()) + "   " + juce::String(juce::roundToInt(proc.p("angle"))) + juce::String::fromUTF8("\xc2\xb0"),
                   juce::Rectangle<float>(r.getRight() - 170, r.getY() + 8, 156, 16), juce::Justification::right);
    }
    void mouseDown(const juce::MouseEvent& e) override { for (auto* id : { "distance", "angle" }) prm(id)->beginChangeGesture(); mouseDrag(e); }
    void mouseUp(const juce::MouseEvent&) override { for (auto* id : { "distance", "angle" }) prm(id)->endChangeGesture(); }
    void mouseDrag(const juce::MouseEvent& e) override {
        geo();
        const auto v = e.position - centre;
        const double rad = juce::jmax(0.0, (double)std::hypot(v.x, v.y));
        const double knob = juce::jlimit(0.0, 1.0, rad / maxR);
        double ang = std::atan2((double)v.x, (double)-v.y) * 180.0 / juce::MathConstants<double>::pi;
        ang = juce::jlimit(-90.0, 90.0, ang);
        prm("distance")->setValueNotifyingHost(prm("distance")->convertTo0to1((float)knob));
        prm("angle")->setValueNotifyingHost(prm("angle")->convertTo0to1((float)ang));
    }
private:
    juce::RangedAudioParameter* prm(const char* id) const { return proc.apvts.getParameter(id); }
    void geo() { const auto r = getLocalBounds().toFloat(); centre = { r.getCentreX(), r.getBottom() - 30 }; maxR = juce::jmin(r.getHeight() - 44, r.getWidth() * 0.5f - 20); }
    float radiusFor(double m) const { return (float)(DistanceDsp::knobFor(m) * maxR); }
    juce::Point<float> pointFor(double m, double angDeg) const {
        const float rad = radiusFor(m); const double a = angDeg * juce::MathConstants<double>::pi / 180.0;
        return { centre.x + rad * (float)std::sin(a), centre.y - rad * (float)std::cos(a) };
    }
    DistanceProcessor& proc; juce::Colour accent; juce::Point<float> centre; float maxR = 100, level = 0;
};

class DistanceEditor : public Editor {
public:
    explicit DistanceEditor(DistanceProcessor& p)
        : Editor(p, "Distance", "Place it near or far", kAccent, 900, 600), dp(p), map(p, accent),
          space(p.apvts, "space"), distance(p.apvts, "distance", "Distance"), angle(p.apvts, "angle", "Angle"),
          keep(p.apvts, "keep", "Keep level"), air(p.apvts, "air", "Air"), doppler(p.apvts, "doppler", "Doppler"), width(p.apvts, "width", "Width"), out(p.apvts, "out", "Output") {
        distance.maxKnob = 100;
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &map, &space, &distance, &angle, &keep, &air, &doppler, &width, &out }) body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        auto top = r.removeFromTop(290);
        placeArea = top.removeFromRight(240); top.removeFromRight(12); mapArea = top;
        map.setBounds(mapArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        auto c = placeArea.reduced(14, 12).withTrimmedTop(station::kModuleTitle - 8);
        distance.setBounds(c.removeFromTop(150)); c.removeFromTop(4); angle.setBounds(c);
        r.removeFromTop(12);
        controlsArea = r;
        auto k = r.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        space.setBounds(k.removeFromTop(36)); k.removeFromTop(8);
        const int w = k.getWidth() / 5;
        for (auto* d : { &keep, &air, &doppler, &width, &out }) d->setBounds(k.removeFromLeft(w));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, mapArea, "Map", "DS-01");
        module(g, placeArea, "Place", "DS-02");
        module(g, controlsArea, "Space", "DS-03");
    }
    void tick() override { map.tick(); }
private:
    DistanceProcessor& dp; MapView map; Segments space; Dial distance, angle, keep, air, doppler, width, out;
    juce::Rectangle<int> mapArea, placeArea, controlsArea;
};

juce::AudioProcessorEditor* DistanceProcessor::createEditor() { return new DistanceEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DistanceProcessor(); }
