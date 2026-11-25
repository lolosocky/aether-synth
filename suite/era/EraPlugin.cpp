// LGN Era: pick a decade or a machine and your sound comes out like it was recorded, sampled or played back on it.
#include "../common/Suite.h"
#include "../dsp/Era.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

const juce::Colour kAccent{ 0xffc2873a };   // faded gold
juce::StringArray eraNames() { juce::StringArray a; for (int i = 0; i < EraDsp::kEras; i++) a.add(EraDsp::recipe(i).name); return a; }

class EraProcessor : public Processor {
public:
    EraProcessor() : Processor("LGNEra", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        param::addChoice(l, "era", "Era", eraNames(), 2);
        param::addPct(l, "age", "Age", 75);
        param::add(l, "grit", "Grit", { 0, 200, 0.1f }, 100, "%", 0);
        param::add(l, "wobble", "Wobble", { 0, 200, 0.1f }, 100, "%", 0);
        param::add(l, "noise", "Noise", { 0, 200, 0.1f }, 100, "%", 0);
        param::add(l, "tone", "Tone", { -1, 1, 0.01f }, 0, "", 2);
        param::addPct(l, "width", "Width", 100);
        param::addPct(l, "mix", "Mix", 100);
        param::addDb(l, "out", "Output", -12, 12, 0);
        return l;
    }
    EraParams params() const {
        EraParams e; e.era = juce::roundToInt(p("era")); e.age = p("age") / 100.0; e.grit = p("grit") / 100.0; e.wobble = p("wobble") / 100.0;
        e.noise = p("noise") / 100.0; e.tone = p("tone"); e.width = p("width") / 100.0; e.mix = p("mix") / 100.0; e.outDb = p("out");
        return e;
    }
    void prepareToPlay(double sr, int) override { dsp.set(params()); dsp.prepare(sr); setLatencySamples(EraDsp::latency()); }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples(), ch = juce::jmin(2, getTotalNumInputChannels());
        for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); c++) buf.clear(c, 0, n);
        meterInput(buf, n);
        dsp.set(params());
        float* ptrs[2] = { buf.getWritePointer(0), ch > 1 ? buf.getWritePointer(1) : nullptr };
        dsp.process(ptrs, ch, n);
        uiRate = (float)dsp.effectiveRate(); uiBits = (float)dsp.effectiveBits();
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 0.05; }
    juce::AudioProcessorEditor* createEditor() override;
    std::atomic<float> uiRate{ 0 }, uiBits{ 24 };
private:
    EraDsp dsp;
};

// Twelve tiles, one per era: the year above, the name below. Click to switch.
class EraTiles : public juce::Component, private juce::Timer {
public:
    EraTiles(EraProcessor& p, juce::Colour a) : proc(p), accent(a) { startTimerHz(15); setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    void paint(juce::Graphics& g) override {
        const int cur = juce::roundToInt(proc.p("era"));
        for (int i = 0; i < EraDsp::kEras; i++) {
            const auto t = tile(i).toFloat().reduced(3);
            const bool on = i == cur, over = i == hover;
            if (on) { g.setColour(station::graphite); g.fillRoundedRectangle(t, 8); }
            else { station::drawWell(g, t, 8); if (over) { g.setColour(juce::Colour(0x0a1a2030)); g.fillRoundedRectangle(t, 8); } }
            const auto& R = EraDsp::recipe(i);
            g.setFont(station::mono(9.5f)); g.setColour(on ? accent : faint);
            g.drawText(R.year, t.reduced(10, 0).withHeight(t.getHeight() * 0.45f).withY(t.getY() + 5), juce::Justification::bottomLeft);
            g.setFont(station::sans(12.5f)); g.setColour(on ? juce::Colours::white : ink);
            g.drawFittedText(R.name, t.reduced(10, 0).withTrimmedTop(t.getHeight() * 0.48f).toNearestInt(), juce::Justification::topLeft, 1, 0.8f);
            if (on) { g.setColour(accent); g.fillEllipse(t.getRight() - 14, t.getY() + 9, 6, 6); }
        }
    }
    void mouseMove(const juce::MouseEvent& e) override { const int h = at(e.getPosition()); if (h != hover) { hover = h; repaint(); } }
    void mouseExit(const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseUp(const juce::MouseEvent& e) override {
        const int i = at(e.getPosition()); if (i < 0) return;
        auto* prm = proc.apvts.getParameter("era");
        prm->beginChangeGesture(); prm->setValueNotifyingHost(prm->convertTo0to1((float)i)); prm->endChangeGesture();
        repaint();
    }
private:
    void timerCallback() override { const int c = juce::roundToInt(proc.p("era")); if (c != shown) { shown = c; repaint(); } }
    juce::Rectangle<int> tile(int i) const {
        const int cols = 4, rows = 3, w = getWidth() / cols, h = getHeight() / rows;
        return { (i % cols) * w, (i / cols) * h, w, h };
    }
    int at(juce::Point<int> pt) const { for (int i = 0; i < EraDsp::kEras; i++) if (tile(i).contains(pt)) return i; return -1; }
    EraProcessor& proc; juce::Colour accent; int hover = -1, shown = -1;
};

// What the era is doing: the spectrum before (grey) and after (gold), and the machine's numbers.
class EraScope : public juce::Component {
public:
    EraScope(EraProcessor& p, juce::Colour a) : proc(p), accent(a) { ax.lo = 30; ax.hi = 20000; }
    void tick() {
        const double sr = proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000;
        in.update(proc.scopeIn, sr); out.update(proc.scopeOut, sr); repaint();
    }
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        auto plot = r.reduced(1).withTrimmedBottom(16);
        ax.drawGrid(g, plot, true);
        g.setColour(juce::Colour(0xffdde2e8)); g.fillPath(in.path(plot, ax, -90, 0, true));
        g.setColour(accent.withAlpha(0.25f)); g.fillPath(out.path(plot, ax, -90, 0, true));
        g.setColour(accent); g.strokePath(out.path(plot, ax, -90, 0, false), juce::PathStrokeType(1.5f));
        // the machine's numbers
        const auto& R = EraDsp::recipe(juce::roundToInt(proc.p("era")));
        const float rate = proc.uiRate.load(), bits = proc.uiBits.load();
        juce::String spec = rate > 0 ? juce::String(rate / 1000.0f, 1) + " kHz" : juce::String("full rate");
        spec << "  /  " << (bits < 23.5f ? juce::String(juce::roundToInt(bits)) + "-bit" : juce::String("full depth"));
        spec << "  /  " << (R.mono > 0.5 ? "mono" : R.lowMono > 0.5 ? "mono lows" : "stereo");
        g.setFont(station::mono(11.0f, true)); g.setColour(ink);
        g.drawText(spec, juce::Rectangle<float>(r.getX() + 12, r.getY() + 8, r.getWidth() - 24, 14), juce::Justification::right);
    }
private:
    EraProcessor& proc; juce::Colour accent; FreqAxis ax; Spectrum in{ 12 }, out{ 12 };
};

class EraEditor : public Editor {
public:
    explicit EraEditor(EraProcessor& p)
        : Editor(p, "Era", "Records, tapes, samplers", kAccent, 980, 620), ep(p), tiles(p, accent), scope(p, accent),
          age(p.apvts, "age", "Age"), grit(p.apvts, "grit", "Grit"), wobble(p.apvts, "wobble", "Wobble"), noise(p.apvts, "noise", "Noise"),
          tone(p.apvts, "tone", "Tone"), width(p.apvts, "width", "Width"), mix(p.apvts, "mix", "Mix"), out(p.apvts, "out", "Output") {
        age.maxKnob = 100;
        tone.slider.getProperties().set("bipolar", true);
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &tiles, &scope, &age, &grit, &wobble, &noise, &tone, &width, &mix, &out }) body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        auto top = r.removeFromTop(300);
        tilesArea = top.removeFromLeft(560); top.removeFromLeft(12); scopeArea = top;
        tiles.setBounds(tilesArea.reduced(12).withTrimmedTop(station::kModuleTitle - 10));
        scope.setBounds(scopeArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        r.removeFromTop(12); controlsArea = r;
        auto k = r.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        age.setBounds(k.removeFromLeft(170));
        const int w = k.getWidth() / 7;
        for (auto* d : { &grit, &wobble, &noise, &tone, &width, &mix, &out }) d->setBounds(k.removeFromLeft(w));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, tilesArea, "Era", "ER-01");
        module(g, scopeArea, "Signal", "ER-02");
        module(g, controlsArea, "Character", "ER-03");
    }
    void tick() override { scope.tick(); }
private:
    EraProcessor& ep; EraTiles tiles; EraScope scope; Dial age, grit, wobble, noise, tone, width, mix, out;
    juce::Rectangle<int> tilesArea, scopeArea, controlsArea;
};

juce::AudioProcessorEditor* EraProcessor::createEditor() { return new EraEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EraProcessor(); }
