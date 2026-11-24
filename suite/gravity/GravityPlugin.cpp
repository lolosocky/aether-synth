// LGN Gravity: pulls every resonance in a sound toward the notes of your key.
#include "../common/Suite.h"
#include "../dsp/Gravity.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

juce::StringArray keyNames() { juce::StringArray a; for (int i = 0; i < 12; i++) a.add(noteNames()[i]); return a; }
juce::StringArray scaleNames() { juce::StringArray a; for (auto& s : scales()) a.add(s.name); return a; }

class GravityProcessor : public Processor {
public:
    GravityProcessor() : Processor("LGNGravity", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        param::addChoice(l, "key", "Key", keyNames(), 0);
        param::addChoice(l, "scale", "Scale", scaleNames(), 1);
        param::addPct(l, "pull", "Pull", 70);
        param::addMs(l, "drift", "Drift", 0, 2000, 80, 200);
        param::addPct(l, "mass", "Mass", 20);
        param::addHz(l, "floor", "Floor", 40, 2000, 150);
        param::addPct(l, "mix", "Mix", 100);
        return l;
    }
    GravityParams params() const {
        GravityParams g; g.key = juce::roundToInt(p("key")); g.scale = juce::roundToInt(p("scale")); g.pull = p("pull") / 100.0; g.driftMs = p("drift");
        g.mass = p("mass") / 100.0; g.floorHz = p("floor"); g.mix = p("mix") / 100.0;
        return g;
    }
    void prepareToPlay(double sr, int) override { dsp.set(params()); dsp.prepare(sr); setLatencySamples(dsp.latency()); }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples(), ch = juce::jmin(2, getTotalNumInputChannels());
        for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); c++) buf.clear(c, 0, n);
        meterInput(buf, n);
        dsp.set(params());
        float* ptrs[2] = { buf.getWritePointer(0), ch > 1 ? buf.getWritePointer(1) : nullptr };
        dsp.process(ptrs, ch, n);
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 0.1; }
    juce::AudioProcessorEditor* createEditor() override;
    GravityDsp dsp;
};

// The field: the key's notes as wells along a log frequency axis, the output spectrum, and the sound's peaks
// with a trail from where they were to where gravity has taken them.
class FieldView : public juce::Component {
public:
    explicit FieldView(GravityProcessor& p) : proc(p) { ax.lo = 60; ax.hi = 12000; }
    Spectrum spectrum{ 12 };
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        const auto in = r.reduced(1).withTrimmedBottom(18);
        const auto gp = proc.params();
        // floor: everything to the left passes untouched
        const float fx = ax.x((float)gp.floorHz, in);
        if (fx > in.getX()) {
            g.setColour(juce::Colour(0xffe9ecf0)); g.fillRect(juce::Rectangle<float>(in.getX(), in.getY(), fx - in.getX(), in.getHeight()));
            g.setColour(faint); g.drawVerticalLine((int)fx, in.getY(), in.getBottom());
            if (fx - in.getX() > 70) station::drawCaps(g, "Untouched", juce::Rectangle<float>(in.getX() + 10, in.getY() + 8, fx - in.getX() - 14, 12), juce::Justification::centredLeft, 9.0f, faint);
        }
        // spectrum
        g.setColour(juce::Colour(0xffe4e8ec)); g.fillPath(spectrum.path(in, ax, -96, 0, true));
        // wells
        for (double f : GravityDsp::noteFreqs(gp.key, gp.scale, ax.lo, ax.hi)) {
            const float x = ax.x((float)f, in);
            const int midi = juce::roundToInt(69 + 12 * std::log2(f / 440.0));
            const bool root = ((midi - gp.key) % 12 + 12) % 12 == 0;
            const float h = in.getHeight() * (root ? 0.9f : 0.62f);
            g.setGradientFill(juce::ColourGradient(ink.withAlpha(root ? 0.75f : 0.32f), x, in.getBottom(), ink.withAlpha(0.0f), x, in.getBottom() - h, false));
            g.fillRect(x - (root ? 1.0f : 0.6f), in.getBottom() - h, root ? 2.0f : 1.2f, h);
            if (root) {
                g.setFont(station::mono(9.0f)); g.setColour(dim);
                g.drawText(juce::String(noteNames()[midi % 12]) + juce::String(midi / 12 - 1), juce::Rectangle<float>(x - 16, in.getBottom() + 3, 32, 12), juce::Justification::centred);
            }
        }
        // peaks: trail from the original frequency to where they are now
        GravityDsp::PeakView pv[GravityDsp::kMaxView];
        const int n = proc.dsp.peaks(pv);
        for (int i = 0; i < n; i++) {
            if (pv[i].toHz < ax.lo || pv[i].toHz > ax.hi) continue;
            const float lv = juce::jlimit(0.0f, 1.0f, ((float)gainToDb(pv[i].level) + 48) / 48);
            const float y = in.getBottom() - in.getHeight() * (0.12f + 0.8f * lv);
            const float x0 = ax.x(juce::jlimit(ax.lo, ax.hi, pv[i].fromHz), in), x1 = ax.x(pv[i].toHz, in);
            g.setColour(ink.withAlpha(0.18f + 0.3f * lv)); g.drawLine(x0, y, x1, y, 1.2f);
            const float d = 3.0f + 4.0f * lv;
            g.setColour(ink.withAlpha(0.55f + 0.45f * lv)); g.fillEllipse(x1 - d / 2, y - d / 2, d, d);
        }
    }
private:
    GravityProcessor& proc; FreqAxis ax;
};

class GravityEditor : public Editor {
public:
    explicit GravityEditor(GravityProcessor& p)
        : Editor(p, "Gravity", "Pulls any sound into key", station::accents::gravity, 960, 580), gp(p), field(p),
          key(p.apvts, "key"), scale(p.apvts, "scale", "Scale"),
          pull(p.apvts, "pull", "Pull"), drift(p.apvts, "drift", "Drift"), mass(p.apvts, "mass", "Mass"), floor(p.apvts, "floor", "Floor"), mix(p.apvts, "mix", "Mix") {
        pull.maxKnob = 96;
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &field, &key, &scale, &pull, &drift, &mass, &floor, &mix }) body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        fieldArea = r.removeFromTop(272);
        field.setBounds(fieldArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        r.removeFromTop(12);
        keyArea = r.removeFromLeft(440); r.removeFromLeft(12); controlsArea = r;
        auto k = keyArea.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        key.setBounds(k.removeFromTop(36)); k.removeFromTop(14);
        scale.setBounds(k.removeFromTop(50).withWidth(220));
        notesArea = k.reduced(0, 4);
        auto c = controlsArea.reduced(12, 12).withTrimmedTop(station::kModuleTitle - 8);
        pull.setBounds(c.removeFromLeft(130));
        const int w = c.getWidth() / 4;
        for (auto* d : { &drift, &mass, &floor, &mix }) d->setBounds(c.removeFromLeft(w));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, fieldArea, "Field", "GV-01");
        module(g, keyArea, "Key", "GV-02");
        module(g, controlsArea, "Gravity", "GV-03");
        // the notes in the chosen key, as a reminder of what the wells are
        const auto p = gp.params(); const int mask = scales()[(size_t)juce::jlimit(0, (int)scales().size() - 1, p.scale)].mask;
        juce::String notes; for (int i = 0; i < 12; i++) if ((mask >> i) & 1) notes << noteNames()[(p.key + i) % 12] << "  ";
        station::drawCaps(g, "Notes", notesArea.removeFromTop(14).toFloat(), juce::Justification::centredLeft, 9.5f, dim);
        g.setFont(station::mono(13.0f, true)); g.setColour(ink); g.drawText(notes.trim(), notesArea.toFloat(), juce::Justification::centredLeft, true);
    }
    void tick() override {
        field.spectrum.update(gp.scopeOut, gp.getSampleRate() > 0 ? gp.getSampleRate() : 48000);
        field.repaint();
        const int k = juce::roundToInt(gp.p("key")), s = juce::roundToInt(gp.p("scale"));
        if (k != lastKey || s != lastScale) { lastKey = k; lastScale = s; body.repaint(keyArea); }
    }
private:
    GravityProcessor& gp; FieldView field; Segments key; Choice scale; Dial pull, drift, mass, floor, mix;
    juce::Rectangle<int> fieldArea, keyArea, controlsArea, notesArea; int lastKey = -1, lastScale = -1;
};

juce::AudioProcessorEditor* GravityProcessor::createEditor() { return new GravityEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GravityProcessor(); }
