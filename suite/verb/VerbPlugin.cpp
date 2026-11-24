// LGN Reverb: eight-line FDN with Room, Plate, Hall and Space characters, freeze, width and movement.
#include "../common/Suite.h"
#include "../dsp/Reverb.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

class VerbProcessor : public Processor {
public:
    VerbProcessor() : Processor("LGNVerb", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        param::addPct(l, "size", "Size", 60);
        param::add(l, "decay", "Decay", param::skewRange(0.2f, 20.0f, 2.5f), 2.4f, "s");
        param::addMs(l, "predelay", "Pre-delay", 0, 250, 20, 40);
        param::addHz(l, "damp", "Damping", 1000, 20000, 7000);
        param::addHz(l, "lowcut", "Low Cut", 20, 1000, 120);
        param::addPct(l, "width", "Width", 100);
        param::addPct(l, "mod", "Movement", 30);
        param::addPct(l, "mix", "Mix", 25);
        param::addChoice(l, "character", "Character", { "Room", "Plate", "Hall", "Space" }, 2);
        param::addBool(l, "freeze", "Freeze", false);
        return l;
    }
    VerbParams params() const {
        VerbParams v; v.sizePct = p("size"); v.decayS = p("decay"); v.preMs = p("predelay"); v.dampHz = p("damp"); v.lowCutHz = p("lowcut");
        v.widthPct = p("width"); v.modPct = p("mod"); v.mix = p("mix") / 100.0; v.character = juce::roundToInt(p("character")); v.freeze = p("freeze") > 0.5f;
        return v;
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
        const float w = (float)dsp.takeWetPeak(); float o = uiWet.load(); while (w > o && !uiWet.compare_exchange_weak(o, w)) {}
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return p("freeze") > 0.5f ? 60.0 : p("decay") * 1.6 + p("predelay") * 0.001; }
    juce::AudioProcessorEditor* createEditor() override;
    std::atomic<float> uiWet{ 0 };
private:
    VerbDsp dsp;
};

// Decay shape: pre-delay gap, build-up, then the tail; low frequencies (dark) and highs (accent) decay differently.
class DecayView : public juce::Component {
public:
    DecayView(VerbProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    float wet = 0;
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        const auto in = r.reduced(14, 12).withTrimmedBottom(10);
        const auto v = proc.params();
        const double span = juce::jmax(1.0, juce::jmin(24.0, v.decayS * 1.4 + v.preMs * 0.001));
        for (int i = 1; i < 4; i++) { const float x = in.getX() + in.getWidth() * i / 4; g.setColour(hairline.withAlpha(0.8f)); g.drawVerticalLine((int)x, in.getY(), in.getBottom()); }
        g.setFont(station::mono(9.0f)); g.setColour(faint);
        for (int i = 0; i <= 4; i++) g.drawText((span < 4 ? juce::String(span * i / 4, 1) : juce::String(juce::roundToInt(span * i / 4))) + " s", juce::Rectangle<float>(in.getX() + in.getWidth() * i / 4 - 20, in.getBottom() + 1, 40, 11), juce::Justification::centred);
        auto shape = [&](double hfFactor) {
            juce::Path p; p.startNewSubPath(in.getX(), in.getBottom());
            const double pre = v.preMs * 0.001, build = v.character == 1 ? 0.004 : 0.012 + 0.04 * v.sizePct / 100.0;
            for (int i = 0; i <= 200; i++) {
                const double t = span * i / 200; double e;
                if (t < pre) e = 0;
                else { const double tt = t - pre; e = (1 - std::exp(-tt / build)) * std::pow(10.0, -3.0 * tt / (v.freeze ? 1e9 : v.decayS * hfFactor) / 2); }
                p.lineTo(in.getX() + in.getWidth() * (float)(i / 200.0), in.getBottom() - in.getHeight() * (float)juce::jlimit(0.0, 1.0, e));
            }
            p.lineTo(in.getRight(), in.getBottom()); p.closeSubPath(); return p;
        };
        const double hf = juce::jlimit(0.25, 1.0, std::log(v.dampHz / 800.0) / std::log(20000.0 / 800.0));
        g.setColour(juce::Colour(0xffe2e6eb)); g.fillPath(shape(1.0));
        g.setColour(accent.withAlpha(0.22f)); g.fillPath(shape(hf));
        g.setColour(accent); g.strokePath(shape(hf), juce::PathStrokeType(1.6f));
        // live wet level
        const float lv = juce::jlimit(0.0f, 1.0f, ((float)gainToDb(wet) + 60) / 60);
        g.setColour(accent.withAlpha(0.9f)); g.fillRoundedRectangle(in.getRight() - 4, in.getBottom() - in.getHeight() * lv, 4, in.getHeight() * lv, 2);
        if (v.freeze) station::drawCaps(g, "Frozen", juce::Rectangle<float>(in.getX() + 6, in.getY(), 80, 16), juce::Justification::centredLeft, 10.0f, accent);
    }
private:
    VerbProcessor& proc; juce::Colour accent;
};

class VerbEditor : public Editor {
public:
    explicit VerbEditor(VerbProcessor& p)
        : Editor(p, "Reverb", "Room / Plate / Hall / Space", station::accents::verb, 920, 530), vp(p), decay(p, accent),
          character(p.apvts, "character"), freeze(p.apvts, "freeze", "Freeze"), width(p.apvts, "width", "Width", true), mod(p.apvts, "mod", "Movement", true),
          size(p.apvts, "size", "Size"), time(p.apvts, "decay", "Decay"), pre(p.apvts, "predelay", "Pre-delay"), damp(p.apvts, "damp", "Damping"),
          lowcut(p.apvts, "lowcut", "Low cut"), mix(p.apvts, "mix", "Mix") {
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &decay, &character, &freeze, &width, &mod, &size, &time, &pre, &damp, &lowcut, &mix }) body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        auto top = r.removeFromTop(250);
        charArea = top.removeFromRight(330); top.removeFromRight(12); decayArea = top;
        decay.setBounds(decayArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        auto c = charArea.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        character.setBounds(c.removeFromTop(36)); c.removeFromTop(12);
        freeze.setBounds(c.removeFromTop(26)); c.removeFromTop(4);
        width.setBounds(c.removeFromLeft(c.getWidth() / 2)); mod.setBounds(c);
        r.removeFromTop(12);
        controlsArea = r;
        auto k = r.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        const int w = k.getWidth() / 6;
        for (auto* d : { &size, &time, &pre, &damp, &lowcut, &mix }) d->setBounds(k.removeFromLeft(w));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, decayArea, "Decay", "RV-01");
        module(g, charArea, "Character", "RV-02");
        module(g, controlsArea, "Controls", "RV-03");
    }
    void tick() override { decay.wet = vp.uiWet.exchange(0); decay.repaint(); }
private:
    VerbProcessor& vp; DecayView decay; Segments character; Switch freeze; Dial width, mod, size, time, pre, damp, lowcut, mix;
    juce::Rectangle<int> decayArea, charArea, controlsArea;
};

juce::AudioProcessorEditor* VerbProcessor::createEditor() { return new VerbEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VerbProcessor(); }
