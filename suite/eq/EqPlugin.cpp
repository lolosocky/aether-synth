// LGN EQ: seven-band equaliser with a live spectrum and a curve you drag directly.
#include "../common/Suite.h"
#include "../dsp/Eq.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

const char* const kPre[EqDsp::kBands] = { "lc", "ls", "b1", "b2", "b3", "hs", "hc" };
const char* const kName[EqDsp::kBands] = { "Low Cut", "Low Shelf", "Bell 1", "Bell 2", "Bell 3", "High Shelf", "High Cut" };
const char* const kChip[EqDsp::kBands] = { "LC", "LS", "1", "2", "3", "HS", "HC" };
const float kDefFreq[EqDsp::kBands] = { 30, 100, 250, 1000, 4000, 8000, 18000 };
const float kDefQ[EqDsp::kBands] = { 0.707f, 0.707f, 1.0f, 1.0f, 1.0f, 0.707f, 0.707f };
juce::String pid(int b, const char* what) { return juce::String(kPre[b]) + "_" + what; }

class EqProcessor : public Processor {
public:
    EqProcessor() : Processor("LGNEQ", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        for (int b = 0; b < EqDsp::kBands; b++) {
            const juce::String n(kName[b]);
            param::addBool(l, pid(b, "on").toRawUTF8(), (n + " On").toRawUTF8(), !EqDsp::isCut(b));
            param::addHz(l, pid(b, "freq").toRawUTF8(), (n + " Freq").toRawUTF8(), 10, 22000, kDefFreq[b]);
            if (EqDsp::isCut(b)) param::addChoice(l, pid(b, "slope").toRawUTF8(), (n + " Slope").toRawUTF8(), { "12 dB", "24 dB", "48 dB" }, 1);
            else {
                param::addDb(l, pid(b, "gain").toRawUTF8(), (n + " Gain").toRawUTF8(), -18, 18, 0);
                param::add(l, pid(b, "q").toRawUTF8(), (n + " Q").toRawUTF8(), param::logRange(0.1f, 18.0f), kDefQ[b], "", 2);
            }
        }
        param::addDb(l, "out", "Output", -18, 18, 0);
        param::addBool(l, "spectrum", "Spectrum", true);
        return l;
    }
    EqBandParams band(int b) const {
        EqBandParams p;
        p.on = p_(pid(b, "on")) > 0.5f; p.freq = p_(pid(b, "freq"));
        if (EqDsp::isCut(b)) p.slope = juce::roundToInt(p_(pid(b, "slope"))); else { p.gain = p_(pid(b, "gain")); p.q = p_(pid(b, "q")); }
        return p;
    }
    void prepareToPlay(double sr, int) override { pull(); dsp.prepare(sr, 2); }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples(), ch = juce::jmin(2, getTotalNumInputChannels());
        for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); c++) buf.clear(c, 0, n);
        meterInput(buf, n);
        pull();
        dsp.process(buf.getArrayOfWritePointers(), ch, n);
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 0.0; }
    juce::AudioProcessorEditor* createEditor() override;
private:
    float p_(const juce::String& id) const { return apvts.getRawParameterValue(id)->load(std::memory_order_relaxed); }
    void pull() { for (int b = 0; b < EqDsp::kBands; b++) dsp.setBand(b, band(b)); dsp.setOutputDb(p_("out")); }
    EqDsp dsp;
};

// ---------------------------------------------------------------- curve display
class EqDisplay : public juce::Component {
public:
    EqDisplay(EqProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    std::function<void(int)> onSelect;
    int selected = 2;
    Spectrum spectrum{ 12 };
    static constexpr float kRange = 18.0f;

    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced(1); }
    float yFor(float db) const { const auto r = plot(); return juce::jmap(db, -kRange, kRange, r.getBottom() - 6, r.getY() + 6); }
    float dbFor(float y) const { const auto r = plot(); return juce::jmap(y, r.getBottom() - 6, r.getY() + 6, -kRange, kRange); }
    juce::Point<float> node(int b) const {
        const auto p = proc.band(b);
        return { ax.x((float)p.freq, plot()), EqDsp::isCut(b) ? yFor(0) : yFor((float)p.gain) };
    }

    void paint(juce::Graphics& g) override {
        const auto r = plot();
        station::drawWell(g, getLocalBounds().toFloat(), 10);
        // live spectrum first, so the grid stays readable on top of it
        if (proc.apvts.getRawParameterValue("spectrum")->load() > 0.5f) {
            g.setColour(juce::Colour(0xffe9ecf0)); g.fillPath(spectrum.path(r, ax, -90, 6, true));
            g.setColour(juce::Colour(0xffd5dae0)); g.strokePath(spectrum.path(r, ax, -90, 6, false), juce::PathStrokeType(1.0f));
        }
        ax.drawGrid(g, r);
        for (float db : { -12.0f, -6.0f, 6.0f, 12.0f }) {
            g.setColour(hairline.withAlpha(0.7f)); g.drawHorizontalLine((int)yFor(db), r.getX(), r.getRight());
            g.setFont(station::mono(9.0f)); g.setColour(faint);
            g.drawText((db > 0 ? "+" : "") + juce::String((int)db), juce::Rectangle<float>(r.getRight() - 30, yFor(db) - 12, 26, 11), juce::Justification::right);
        }
        g.setColour(station::hairlineHi); g.drawHorizontalLine((int)yFor(0), r.getX(), r.getRight());
        const double sr = proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000;
        EqBandParams bp[EqDsp::kBands]; for (int b = 0; b < EqDsp::kBands; b++) bp[b] = proc.band(b);
        // selected band on its own, lightly
        {
            juce::Path sel; bool st = false;
            for (float x = r.getX(); x <= r.getRight(); x += 2) {
                const float db = (float)gainToDb(EqDsp::bandMagnitude(selected, bp[selected], ax.f(x, r), sr));
                const float y = yFor(juce::jlimit(-kRange - 2, kRange + 2, db));
                if (!st) { sel.startNewSubPath(x, yFor(0)); sel.lineTo(x, y); st = true; } else sel.lineTo(x, y);
            }
            sel.lineTo(r.getRight(), yFor(0)); sel.closeSubPath();
            g.setColour(accent.withAlpha(bp[selected].on ? 0.12f : 0.0f)); g.fillPath(sel);
        }
        // the combined curve
        juce::Path curve; bool st = false;
        for (float x = r.getX(); x <= r.getRight(); x += 1.5f) {
            double m = 1; const float f = ax.f(x, r);
            for (int b = 0; b < EqDsp::kBands; b++) m *= EqDsp::bandMagnitude(b, bp[b], f, sr);
            const float y = yFor(juce::jlimit(-kRange - 2, kRange + 2, (float)gainToDb(m)));
            if (!st) { curve.startNewSubPath(x, y); st = true; } else curve.lineTo(x, y);
        }
        g.setColour(accent); g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // nodes
        for (int b = 0; b < EqDsp::kBands; b++) {
            const auto c = node(b); const bool on = bp[b].on, sel = b == selected;
            const float rad = sel ? 10.0f : 8.5f;
            if (on) station::drawShadow(g, { c.x - rad, c.y - rad, rad * 2, rad * 2 }, rad, 0.6f);
            g.setColour(sel && on ? accent : juce::Colours::white); g.fillEllipse(c.x - rad, c.y - rad, rad * 2, rad * 2);
            g.setColour(on ? (sel ? accent : accent.withAlpha(0.9f)) : faint); g.drawEllipse(c.x - rad + 0.5f, c.y - rad + 0.5f, rad * 2 - 1, rad * 2 - 1, on ? 1.4f : 1.0f);
            g.setFont(juce::Font(station::sans(9.5f, station::bold))); g.setColour(sel && on ? juce::Colours::white : (on ? ink : faint));
            g.drawText(kChip[b], juce::Rectangle<float>(c.x - rad, c.y - rad, rad * 2, rad * 2), juce::Justification::centred);
        }
    }

    int hit(juce::Point<float> p) const {
        int best = -1; float bd = 15;
        for (int b = 0; b < EqDsp::kBands; b++) { const float d = node(b).getDistanceFrom(p); if (d < bd) { bd = d; best = b; } }
        return best;
    }
    void mouseDown(const juce::MouseEvent& e) override {
        const int b = hit(e.position);
        if (b < 0) return;
        select(b);
        dragging = b;
        auto* on = proc.apvts.getParameter(pid(b, "on"));
        if (on->getValue() < 0.5f) { on->beginChangeGesture(); on->setValueNotifyingHost(1); on->endChangeGesture(); }
        for (auto* p : dragParams(b)) p->beginChangeGesture();
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragging < 0) return;
        const auto r = plot();
        auto* f = proc.apvts.getParameter(pid(dragging, "freq"));
        f->setValueNotifyingHost(f->convertTo0to1(juce::jlimit(10.0f, 22000.0f, ax.f(juce::jlimit(r.getX(), r.getRight(), e.position.x), r))));
        if (!EqDsp::isCut(dragging)) { auto* gp = proc.apvts.getParameter(pid(dragging, "gain")); gp->setValueNotifyingHost(gp->convertTo0to1(juce::jlimit(-18.0f, 18.0f, dbFor(e.position.y)))); }
        repaint();
    }
    void mouseUp(const juce::MouseEvent&) override { if (dragging >= 0) for (auto* p : dragParams(dragging)) p->endChangeGesture(); dragging = -1; }
    void mouseDoubleClick(const juce::MouseEvent& e) override {
        const int b = hit(e.position); if (b < 0) return;
        auto* on = proc.apvts.getParameter(pid(b, "on"));
        on->beginChangeGesture(); on->setValueNotifyingHost(on->getValue() > 0.5f ? 0.0f : 1.0f); on->endChangeGesture();
    }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        const int b = hit(e.position) >= 0 ? hit(e.position) : selected;
        if (EqDsp::isCut(b)) {
            auto* s = proc.apvts.getParameter(pid(b, "slope"));
            const int v = juce::jlimit(0, 2, juce::roundToInt(s->convertFrom0to1(s->getValue())) + (w.deltaY > 0 ? 1 : -1));
            s->beginChangeGesture(); s->setValueNotifyingHost(s->convertTo0to1((float)v)); s->endChangeGesture();
        } else {
            auto* q = proc.apvts.getParameter(pid(b, "q"));
            q->beginChangeGesture(); q->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, q->getValue() + w.deltaY * 0.15f)); q->endChangeGesture();
        }
        select(b);
    }
    void select(int b) { if (b != selected) { selected = b; if (onSelect) onSelect(b); } repaint(); }
private:
    std::vector<juce::RangedAudioParameter*> dragParams(int b) {
        std::vector<juce::RangedAudioParameter*> v{ proc.apvts.getParameter(pid(b, "freq")) };
        if (!EqDsp::isCut(b)) v.push_back(proc.apvts.getParameter(pid(b, "gain")));
        return v;
    }
    EqProcessor& proc; juce::Colour accent; FreqAxis ax; int dragging = -1;
};

// ---------------------------------------------------------------- editor
class EqEditor : public Editor {
public:
    explicit EqEditor(EqProcessor& p) : Editor(p, "EQ", "Seven band equaliser", station::accents::eq, 940, 600), eq(p), display(p, accent),
                                       out(p.apvts, "out", "Output"), spec(p.apvts, "spectrum", "Spectrum") {
        body.addAndMakeVisible(display);
        for (int b = 0; b < EqDsp::kBands; b++) {
            auto* c = chips.add(new juce::TextButton(kChip[b]));
            c->getProperties().set("led", true);
            c->onClick = [this, b] { display.select(b); };
            body.addAndMakeVisible(c);
        }
        display.onSelect = [this](int b) { showBand(b); };
        for (auto* c : { (juce::Component*)&out, (juce::Component*)&spec }) body.addAndMakeVisible(c);
        showBand(display.selected);
        setReady();
    }
    void showBand(int b) {
        for (int i = 0; i < chips.size(); i++) chips[i]->setToggleState(i == b, juce::dontSendNotification);
        bandOn = std::make_unique<Switch>(eq.apvts, pid(b, "on").toRawUTF8(), "On");
        freq = std::make_unique<Dial>(eq.apvts, pid(b, "freq").toRawUTF8(), "Frequency");
        if (EqDsp::isCut(b)) { gain.reset(); q.reset(); slope = std::make_unique<Segments>(eq.apvts, pid(b, "slope").toRawUTF8()); }
        else { slope.reset(); gain = std::make_unique<Dial>(eq.apvts, pid(b, "gain").toRawUTF8(), "Gain"); q = std::make_unique<Dial>(eq.apvts, pid(b, "q").toRawUTF8(), "Q"); }
        for (juce::Component* c : { (juce::Component*)bandOn.get(), (juce::Component*)freq.get(), (juce::Component*)gain.get(), (juce::Component*)q.get(), (juce::Component*)slope.get() })
            if (c) body.addAndMakeVisible(c);
        bandTitle = kName[b];
        resized(); body.repaint();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        displayArea = r.removeFromTop(318);
        display.setBounds(displayArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        r.removeFromTop(12);
        outArea = r.removeFromRight(220); r.removeFromRight(12);
        bandArea = r;
        auto inner = bandArea.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 10);
        auto chipRow = inner.removeFromTop(34);
        const int cw = chipRow.getWidth() / EqDsp::kBands;
        for (auto* c : chips) c->setBounds(chipRow.removeFromLeft(cw).reduced(2, 0));
        inner.removeFromTop(10);
        auto left = inner.removeFromLeft(150);
        if (bandOn) bandOn->setBounds(left.withSizeKeepingCentre(left.getWidth(), 28).translated(0, -4));
        const int dw = 104;
        if (freq) freq->setBounds(inner.removeFromLeft(dw));
        if (gain) gain->setBounds(inner.removeFromLeft(dw));
        if (q) q->setBounds(inner.removeFromLeft(dw));
        if (slope) slope->setBounds(inner.removeFromLeft(260).withSizeKeepingCentre(260, 36));
        auto o = outArea.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 10);
        spec.setBounds(o.removeFromBottom(26));
        out.setBounds(o.withSizeKeepingCentre(104, o.getHeight()));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, displayArea, "Response", "EQ-01");
        module(g, bandArea, "Band  /  " + bandTitle, "EQ-02");
        module(g, outArea, "Output", "EQ-03");
    }
    void tick() override {
        if (eq.apvts.getRawParameterValue("spectrum")->load() > 0.5f) display.spectrum.update(eq.scopeOut, eq.getSampleRate() > 0 ? eq.getSampleRate() : 48000);
        display.repaint();
    }
private:
    EqProcessor& eq;
    EqDisplay display;
    juce::OwnedArray<juce::TextButton> chips;
    std::unique_ptr<Switch> bandOn; std::unique_ptr<Dial> freq, gain, q; std::unique_ptr<Segments> slope;
    Dial out; Switch spec;
    juce::String bandTitle;
    juce::Rectangle<int> displayArea, bandArea, outArea;
};

juce::AudioProcessorEditor* EqProcessor::createEditor() { return new EqEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EqProcessor(); }
