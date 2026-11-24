// LGN Limiter: lookahead brickwall with true-peak detection and auto release.
#include "../common/Suite.h"
#include "../dsp/Limit.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

class LimitProcessor : public Processor {
public:
    LimitProcessor() : Processor("LGNLimit", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        param::addDb(l, "input", "Input", 0, 24, 0);
        param::addDb(l, "ceiling", "Ceiling", -12, 0, -0.3f);
        param::addMs(l, "release", "Release", 1, 1000, 80, 80);
        param::addBool(l, "truepeak", "True Peak", true);
        param::addBool(l, "autorel", "Auto Release", true);
        return l;
    }
    LimitParams params() const {
        LimitParams q; q.inputDb = p("input"); q.ceilingDb = p("ceiling"); q.releaseMs = p("release"); q.truePeak = p("truepeak") > 0.5f; q.autoRelease = p("autorel") > 0.5f; q.lookaheadMs = 1.5;
        return q;
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
        const float gr = (float)dsp.reductionDb(); float o = uiGr.load(); while (gr > o && !uiGr.compare_exchange_weak(o, gr)) {}
        float pk = 0; for (int c = 0; c < ch; c++) pk = juce::jmax(pk, buf.getMagnitude(c, 0, n));
        o = uiOut.load(); while (pk > o && !uiOut.compare_exchange_weak(o, pk)) {}
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 0.0; }
    juce::AudioProcessorEditor* createEditor() override;
    std::atomic<float> uiGr{ 0 }, uiOut{ 0 };
private:
    LimitDsp dsp;
};

// Big readouts: current gain reduction and output peak.
class Readout : public juce::Component {
public:
    Readout(const juce::String& l, juce::Colour a) : label(l), accent(a) {}
    void set(const juce::String& v, float bar01) { value = v; bar = bar01; repaint(); }
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        station::drawCaps(g, label, r.removeFromTop(16), juce::Justification::centredLeft, 9.5f, dim);
        g.setFont(juce::Font(station::mono(34.0f, true))); g.setColour(ink);
        g.drawText(value, r.removeFromTop(44), juce::Justification::centredLeft, false);
        auto b = r.removeFromTop(6);
        g.setColour(station::track); g.fillRoundedRectangle(b, 3);
        g.setColour(accent); g.fillRoundedRectangle(b.withWidth(b.getWidth() * juce::jlimit(0.0f, 1.0f, bar)), 3);
    }
private:
    juce::String label, value{ "0.0" }; juce::Colour accent; float bar = 0;
};

class LimitEditor : public Editor {
public:
    explicit LimitEditor(LimitProcessor& p)
        : Editor(p, "Limiter", "True-peak brickwall", station::accents::limit, 900, 540), lp(p), history(accent, 12.0f),
          grRead("Gain reduction  dB", accent), outRead("Output peak  dBFS", station::graphite),
          truePeak(p.apvts, "truepeak", "True peak"), autoRel(p.apvts, "autorel", "Auto release"),
          input(p.apvts, "input", "Input"), ceiling(p.apvts, "ceiling", "Ceiling"), release(p.apvts, "release", "Release") {
        input.maxKnob = 100;
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &history, &grRead, &outRead, &truePeak, &autoRel, &input, &ceiling, &release }) body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        auto top = r.removeFromTop(250);
        readArea = top.removeFromRight(250); top.removeFromRight(12); histArea = top;
        history.setBounds(histArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        auto c = readArea.reduced(18, 12).withTrimmedTop(station::kModuleTitle - 8);
        grRead.setBounds(c.removeFromTop(80)); c.removeFromTop(16); outRead.setBounds(c.removeFromTop(80));
        r.removeFromTop(12);
        controlsArea = r;
        auto k = r.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        auto right = k.removeFromRight(200);
        truePeak.setBounds(right.removeFromTop(right.getHeight() / 2).withSizeKeepingCentre(right.getWidth(), 26).translated(0, 10));
        autoRel.setBounds(right.withSizeKeepingCentre(right.getWidth(), 26).translated(0, -10));
        input.setBounds(k.removeFromLeft(180)); k.removeFromLeft(20);
        ceiling.setBounds(k.removeFromLeft(140)); release.setBounds(k.removeFromLeft(140));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, histArea, "Activity", "LM-01");
        module(g, readArea, "Readout", "LM-02");
        module(g, controlsArea, "Controls", "LM-03");
    }
    void tick() override {
        const float gr = lp.uiGr.exchange(0), out = lp.uiOut.exchange(0);
        grShown = gr > grShown ? gr : grShown + (gr - grShown) * 0.15f;
        history.push((float)gainToDb(out), gr);
        grRead.set(juce::String(-grShown, 1), grShown / 12.0f);
        if (out > outHold || ++holdAge > 40) { outHold = out; holdAge = 0; }
        outRead.set(out > 0 ? juce::String(gainToDb(outHold), 1) : juce::String("-inf"), juce::jlimit(0.0f, 1.0f, ((float)gainToDb(outHold) + 30) / 30));
    }
private:
    LimitProcessor& lp; History history; Readout grRead, outRead; Switch truePeak, autoRel; Dial input, ceiling, release;
    juce::Rectangle<int> histArea, readArea, controlsArea;
    float grShown = 0, outHold = 0; int holdAge = 0;
};

juce::AudioProcessorEditor* LimitProcessor::createEditor() { return new LimitEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LimitProcessor(); }
