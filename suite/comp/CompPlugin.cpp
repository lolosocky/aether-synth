// LGN Compressor: three styles (Clean, Glue, Punch), transfer curve and gain-reduction history.
#include "../common/Suite.h"
#include "../dsp/Comp.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

class CompProcessor : public Processor {
public:
    CompProcessor() : Processor("LGNComp", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        param::addDb(l, "thresh", "Threshold", -60, 0, -18);
        param::add(l, "ratio", "Ratio", param::skewRange(1, 20, 4), 4, ":1");
        param::addMs(l, "attack", "Attack", 0.05f, 200, 10, 10);
        param::addMs(l, "release", "Release", 5, 2000, 120, 150);
        param::addDb(l, "knee", "Knee", 0, 24, 6);
        param::addDb(l, "makeup", "Makeup", -12, 24, 0);
        param::addPct(l, "mix", "Mix", 100);
        param::addHz(l, "schpf", "Sidechain HPF", 20, 500, 20);
        param::addChoice(l, "style", "Style", { "Clean", "Glue", "Punch" }, 0);
        param::addBool(l, "automakeup", "Auto Makeup", false);
        return l;
    }
    CompParams params() const {
        CompParams c; c.thresh = p("thresh"); c.ratio = p("ratio"); c.attackMs = p("attack"); c.releaseMs = p("release"); c.kneeDb = p("knee");
        c.makeupDb = p("makeup"); c.mix = p("mix") / 100.0; c.scHpf = p("schpf"); c.style = juce::roundToInt(p("style")); c.autoMakeup = p("automakeup") > 0.5f;
        return c;
    }
    void prepareToPlay(double sr, int) override { dsp.set(params()); dsp.prepare(sr); }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples(), ch = juce::jmin(2, getTotalNumInputChannels());
        for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); c++) buf.clear(c, 0, n);
        meterInput(buf, n);
        float in = 0; for (int c = 0; c < ch; c++) in = juce::jmax(in, buf.getMagnitude(c, 0, n));
        dsp.set(params());
        // run in short slices so the meter sees the deepest reduction in the block
        float deepest = 0;
        for (int o = 0; o < n; o += 32) {
            float* ptrs[2] = { buf.getWritePointer(0, o), ch > 1 ? buf.getWritePointer(1, o) : nullptr };
            dsp.process(ptrs, ch, juce::jmin(32, n - o));
            deepest = juce::jmax(deepest, (float)dsp.reductionDb());
        }
        raiseTo(uiGr, deepest); raiseTo(uiIn, in);
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 0.0; }
    juce::AudioProcessorEditor* createEditor() override;
    std::atomic<float> uiGr{ 0 }, uiIn{ 0 };
private:
    static void raiseTo(std::atomic<float>& a, float v) { float o = a.load(); while (v > o && !a.compare_exchange_weak(o, v)) {} }
    CompDsp dsp;
};

// Static curve with the live input level as a dot.
class TransferView : public juce::Component {
public:
    TransferView(CompProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    float level = -90;
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        const auto in = r.reduced(12);
        auto px = [&](double db) { return in.getX() + in.getWidth() * (float)((db + 60) / 60); };
        auto py = [&](double db) { return in.getBottom() - in.getHeight() * (float)((db + 60) / 60); };
        for (int db = -48; db < 0; db += 12) { g.setColour(hairline.withAlpha(0.8f)); g.drawVerticalLine((int)px(db), in.getY(), in.getBottom()); g.drawHorizontalLine((int)py(db), in.getX(), in.getRight()); }
        const auto c = proc.params();
        g.setColour(hairline); g.drawLine(in.getX(), in.getBottom(), in.getRight(), in.getY(), 1.0f);
        g.setColour(accent.withAlpha(0.25f)); g.drawVerticalLine((int)px(c.thresh), in.getY(), in.getBottom());
        juce::Path p; for (int i = 0; i <= 120; i++) { const double x = -60 + i * 0.5, y = CompDsp::curve(x, c.thresh, c.ratio, c.kneeDb); i ? p.lineTo(px(x), py(y)) : p.startNewSubPath(px(x), py(y)); }
        g.setColour(accent); g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (level > -60) {
            const float x = px(level), y = py(CompDsp::curve(level, c.thresh, c.ratio, c.kneeDb));
            g.setColour(accent.withAlpha(0.18f)); g.fillEllipse(x - 9, y - 9, 18, 18);
            g.setColour(accent); g.fillEllipse(x - 4.5f, y - 4.5f, 9, 9);
        }
        g.setFont(station::mono(9.0f)); g.setColour(faint);
        g.drawText("IN", juce::Rectangle<float>(in.getRight() - 22, in.getBottom() - 12, 20, 12), juce::Justification::right);
        g.drawText("OUT", juce::Rectangle<float>(in.getX() + 2, in.getY(), 30, 12), juce::Justification::left);
    }
private:
    CompProcessor& proc; juce::Colour accent;
};

class CompEditor : public Editor {
public:
    explicit CompEditor(CompProcessor& p)
        : Editor(p, "Compressor", "Clean / Glue / Punch", station::accents::comp, 920, 550), cp(p), transfer(p, accent), history(accent), grMeter(Meter::reduction, accent),
          style(p.apvts, "style"), autoMk(p.apvts, "automakeup", "Auto makeup"),
          thresh(p.apvts, "thresh", "Threshold"), ratio(p.apvts, "ratio", "Ratio"), attack(p.apvts, "attack", "Attack"), release(p.apvts, "release", "Release"),
          knee(p.apvts, "knee", "Knee"), makeup(p.apvts, "makeup", "Makeup"), mix(p.apvts, "mix", "Mix"), schpf(p.apvts, "schpf", "SC Filter") {
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &transfer, &history, &grMeter, &style, &autoMk, &thresh, &ratio, &attack, &release, &knee, &makeup, &mix, &schpf })
            body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        auto top = r.removeFromTop(262);
        transferArea = top.removeFromLeft(262); top.removeFromLeft(12); historyArea = top;
        transfer.setBounds(transferArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        auto h = historyArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12);
        grMeter.setBounds(h.removeFromRight(10)); h.removeFromRight(10);
        history.setBounds(h);
        r.removeFromTop(12);
        controlsArea = r;
        auto c = r.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        auto left = c.removeFromLeft(220);
        style.setBounds(left.removeFromTop(36)); left.removeFromTop(16);
        autoMk.setBounds(left.removeFromTop(26));
        c.removeFromLeft(16);
        const int w = c.getWidth() / 8;
        for (auto* d : { &thresh, &ratio, &attack, &release, &knee, &makeup, &mix, &schpf }) d->setBounds(c.removeFromLeft(w));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, transferArea, "Transfer", "CP-01");
        module(g, historyArea, "Gain reduction", "CP-02");
        module(g, controlsArea, "Controls", "CP-03");
    }
    void tick() override {
        const float gr = cp.uiGr.exchange(0), in = cp.uiIn.exchange(0);
        const float inDb = (float)gainToDb(in);
        history.push(inDb, gr); grMeter.setValue(gr);
        transfer.level = inDb; transfer.repaint();
    }
private:
    CompProcessor& cp; TransferView transfer; History history; Meter grMeter;
    Segments style; Switch autoMk; Dial thresh, ratio, attack, release, knee, makeup, mix, schpf;
    juce::Rectangle<int> transferArea, historyArea, controlsArea;
};

juce::AudioProcessorEditor* CompProcessor::createEditor() { return new CompEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CompProcessor(); }
