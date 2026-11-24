// LGN De-Esser: dynamic high shelf (Split) or full-band (Wide) reduction of sibilance, with a listen mode.
#include "../common/Suite.h"
#include "../dsp/DeEss.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

class DeEssProcessor : public Processor {
public:
    DeEssProcessor() : Processor("LGNDeEss", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        param::addHz(l, "freq", "Frequency", 2000, 16000, 6500);
        param::addDb(l, "thresh", "Threshold", -60, 0, -28);
        param::addDb(l, "range", "Range", 0, 24, 8);
        param::addMs(l, "smooth", "Smooth", 10, 250, 60, 60);
        param::addChoice(l, "mode", "Mode", { "Split", "Wide" }, 0);
        param::addBool(l, "listen", "Listen", false);
        return l;
    }
    DeEssParams params() const {
        DeEssParams d; d.freq = p("freq"); d.thresh = p("thresh"); d.rangeDb = p("range"); d.smoothMs = p("smooth"); d.mode = juce::roundToInt(p("mode")); d.listen = p("listen") > 0.5f;
        return d;
    }
    void prepareToPlay(double sr, int) override { dsp.set(params()); dsp.prepare(sr); }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples(), ch = juce::jmin(2, getTotalNumInputChannels());
        for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); c++) buf.clear(c, 0, n);
        meterInput(buf, n);
        dsp.set(params());
        float deepest = 0, det = -120;
        for (int o = 0; o < n; o += 32) {
            float* ptrs[2] = { buf.getWritePointer(0, o), ch > 1 ? buf.getWritePointer(1, o) : nullptr };
            dsp.process(ptrs, ch, juce::jmin(32, n - o));
            deepest = juce::jmax(deepest, (float)dsp.reductionDb()); det = juce::jmax(det, (float)dsp.detectorDb());
        }
        float o = uiGr.load(); while (deepest > o && !uiGr.compare_exchange_weak(o, deepest)) {}
        o = uiDet.load(); while (det > o && !uiDet.compare_exchange_weak(o, det)) {}
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 0.0; }
    juce::AudioProcessorEditor* createEditor() override;
    std::atomic<float> uiGr{ 0 }, uiDet{ -120 };
private:
    DeEssDsp dsp;
};

// Input spectrum with the detection band shaded; drag anywhere to move the frequency.
class SibilanceView : public juce::Component {
public:
    SibilanceView(DeEssProcessor& p, juce::Colour a) : proc(p), accent(a) { ax.lo = 1000; ax.hi = 20000; }
    Spectrum spectrum{ 11 }; float detDb = -120, grDb = 0;
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        const auto in = r.reduced(1);
        const float fx = ax.x(proc.p("freq"), in);
        g.setColour(accent.withAlpha(0.07f)); g.fillRect(juce::Rectangle<float>(fx, in.getY(), in.getRight() - fx, in.getHeight()));
        g.setColour(juce::Colour(0xffe6e9ee)); g.fillPath(spectrum.path(in, ax, -96, 0, true));
        g.setColour(juce::Colour(0xffd3d8de)); g.strokePath(spectrum.path(in, ax, -96, 0, false), juce::PathStrokeType(1.0f));
        ax.drawGrid(g, in);
        // threshold and live detector level, on the same dB scale as the spectrum
        auto yFor = [&](float db) { return juce::jmap(juce::jlimit(-96.0f, 0.0f, db), -96.0f, 0.0f, in.getBottom(), in.getY()); };
        const float ty = yFor(proc.p("thresh"));
        g.setColour(accent.withAlpha(0.55f));
        for (float x = fx; x < in.getRight(); x += 8) g.drawLine(x, ty, juce::jmin(in.getRight(), x + 4), ty, 1.2f);
        g.setFont(station::mono(9.0f)); g.setColour(accent); g.drawText("THRESHOLD", juce::Rectangle<float>(in.getRight() - 84, ty - 13, 80, 11), juce::Justification::right);
        if (detDb > -96) { const float dy = yFor(detDb); g.setColour(accent); g.fillRoundedRectangle(fx + 6, dy - 1.5f, 26, 3, 1.5f); }
        g.setColour(accent); g.fillRect(fx - 1, in.getY(), 2.0f, in.getHeight());
        g.setColour(juce::Colours::white); g.fillEllipse(fx - 7, in.getCentreY() - 7, 14, 14);
        g.setColour(accent); g.drawEllipse(fx - 6.5f, in.getCentreY() - 6.5f, 13, 13, 1.5f);
        g.setFont(station::mono(10.0f)); g.setColour(ink);
        g.drawText(param::formatHz(proc.p("freq")), juce::Rectangle<float>(fx + 10, in.getY() + 8, 80, 12), juce::Justification::left);
    }
    void mouseDown(const juce::MouseEvent& e) override { prm()->beginChangeGesture(); mouseDrag(e); }
    void mouseDrag(const juce::MouseEvent& e) override {
        const auto in = getLocalBounds().toFloat().reduced(1);
        prm()->setValueNotifyingHost(prm()->convertTo0to1(juce::jlimit(2000.0f, 16000.0f, ax.f(juce::jlimit(in.getX(), in.getRight(), e.position.x), in))));
        repaint();
    }
    void mouseUp(const juce::MouseEvent&) override { prm()->endChangeGesture(); }
    FreqAxis ax;
private:
    juce::RangedAudioParameter* prm() { return proc.apvts.getParameter("freq"); }
    DeEssProcessor& proc; juce::Colour accent;
};

class DeEssEditor : public Editor {
public:
    explicit DeEssEditor(DeEssProcessor& p)
        : Editor(p, "De-Esser", "Sibilance control", station::accents::deess, 900, 540), dp(p), view(p, accent), grMeter(Meter::reduction, accent),
          mode(p.apvts, "mode"), listen(p.apvts, "listen", "Listen"),
          freq(p.apvts, "freq", "Frequency"), thresh(p.apvts, "thresh", "Threshold"), range(p.apvts, "range", "Range"), smooth(p.apvts, "smooth", "Smooth") {
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &view, &grMeter, &mode, &listen, &freq, &thresh, &range, &smooth }) body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        viewArea = r.removeFromTop(250);
        auto v = viewArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12);
        grMeter.setBounds(v.removeFromRight(10)); v.removeFromRight(10);
        view.setBounds(v);
        r.removeFromTop(12);
        controlsArea = r;
        auto c = r.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        auto left = c.removeFromLeft(220);
        mode.setBounds(left.removeFromTop(36)); left.removeFromTop(16);
        listen.setBounds(left.removeFromTop(26));
        c.removeFromLeft(24);
        const int w = juce::jmin(130, c.getWidth() / 4);
        for (auto* d : { &freq, &thresh, &range, &smooth }) d->setBounds(c.removeFromLeft(w));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, viewArea, "Detection", "DS-01");
        module(g, controlsArea, "Controls", "DS-02");
    }
    void tick() override {
        view.spectrum.update(dp.scopeIn, dp.getSampleRate() > 0 ? dp.getSampleRate() : 48000);
        const float gr = dp.uiGr.exchange(0), det = dp.uiDet.exchange(-120);
        view.detDb = det; view.grDb = gr; view.repaint(); grMeter.setValue(gr);
    }
private:
    DeEssProcessor& dp; SibilanceView view; Meter grMeter; Segments mode; Switch listen; Dial freq, thresh, range, smooth;
    juce::Rectangle<int> viewArea, controlsArea;
};

juce::AudioProcessorEditor* DeEssProcessor::createEditor() { return new DeEssEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DeEssProcessor(); }
