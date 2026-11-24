// LGN Delay: synced or free stereo delay with tape-style feedback, ping-pong, warmth, wow and ducking.
#include "../common/Suite.h"
#include "../dsp/Delay.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

juce::StringArray divisionNames() { juce::StringArray a; for (auto& d : delayDivisions()) a.add(d.first); return a; }

class DelayProcessor : public Processor {
public:
    DelayProcessor() : Processor("LGNDelay", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        param::addBool(l, "sync", "Sync", true);
        param::addChoice(l, "division", "Division", divisionNames(), 6);   // 1/8 dotted
        param::addMs(l, "time", "Time", 1, 2000, 375, 300);
        param::addPct(l, "feedback", "Feedback", 35);
        param::addPct(l, "mix", "Mix", 25);
        param::addHz(l, "lowcut", "Low Cut", 20, 2000, 150);
        param::addHz(l, "highcut", "High Cut", 1000, 20000, 8000);
        param::addPct(l, "warmth", "Warmth", 25);
        param::addPct(l, "wow", "Wow", 10);
        param::addPct(l, "duck", "Duck", 0);
        param::addMs(l, "spread", "Spread", 0, 40, 0, 10);
        param::addChoice(l, "mode", "Mode", { "Stereo", "Ping-Pong", "Mono" }, 1);
        return l;
    }
    double timeMs() const {
        if (p("sync") > 0.5f) { const auto& d = delayDivisions()[(size_t)juce::jlimit(0, (int)delayDivisions().size() - 1, juce::roundToInt(p("division")))]; return juce::jlimit(1.0, 2000.0, d.second * 60000.0 / bpm.load()); }
        return p("time");
    }
    DelayParams params() const {
        DelayParams d; d.timeMs = timeMs(); d.feedback = p("feedback") / 100.0; d.mix = p("mix") / 100.0; d.lowCutHz = p("lowcut"); d.highCutHz = p("highcut");
        d.warmth = p("warmth") / 100.0; d.wow = p("wow") / 100.0; d.duck = p("duck") / 100.0; d.spreadMs = p("spread"); d.mode = juce::roundToInt(p("mode"));
        return d;
    }
    void prepareToPlay(double sr, int) override { dsp.set(params()); dsp.prepare(sr); }
    void reset() override { dsp.reset(); }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples(), ch = juce::jmin(2, getTotalNumInputChannels());
        for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); c++) buf.clear(c, 0, n);
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) bpm = juce::jlimit(20.0, 400.0, *b);
        meterInput(buf, n);
        dsp.set(params());
        float* ptrs[2] = { buf.getWritePointer(0), ch > 1 ? buf.getWritePointer(1) : nullptr };
        dsp.process(ptrs, ch, n);
        const float e = (float)dsp.takeEchoPeak(); float o = uiEcho.load(); while (e > o && !uiEcho.compare_exchange_weak(o, e)) {}
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 2.0 / juce::jmax(0.02, 1.0 - p("feedback") / 100.0 * 0.98); }
    juce::AudioProcessorEditor* createEditor() override;
    std::atomic<double> bpm{ 120.0 }; std::atomic<float> uiEcho{ 0 };
private:
    DelayDsp dsp;
};

// Echo map: each repeat as a bar at its time, sized by the feedback, placed left/right for ping-pong.
class TapView : public juce::Component {
public:
    TapView(DelayProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    float echo = 0;
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        const auto in = r.reduced(16, 14).withTrimmedBottom(10);
        const auto d = proc.params();
        const double span = 2000.0;
        g.setColour(hairline); g.drawHorizontalLine((int)in.getCentreY(), in.getX(), in.getRight());
        for (int i = 0; i <= 4; i++) {
            const float x = in.getX() + in.getWidth() * i / 4;
            g.setColour(hairline.withAlpha(0.8f)); g.drawVerticalLine((int)x, in.getY(), in.getBottom());
            g.setFont(station::mono(9.0f)); g.setColour(faint); g.drawText(juce::String((int)(span * i / 4)) + " ms", juce::Rectangle<float>(x - 30, in.getBottom() + 1, 60, 11), juce::Justification::centred);
        }
        station::drawCaps(g, "L", juce::Rectangle<float>(in.getX() + 4, in.getY(), 12, 12), juce::Justification::centredLeft, 9.0f, faint);
        station::drawCaps(g, "R", juce::Rectangle<float>(in.getX() + 4, in.getBottom() - 12, 12, 12), juce::Justification::centredLeft, 9.0f, faint);
        // the dry hit
        g.setColour(station::graphite); g.fillRoundedRectangle(in.getX() - 2, in.getY() + in.getHeight() * 0.1f, 4, in.getHeight() * 0.8f, 2);
        double amp = 1.0; const double fb = d.feedback * 0.98;
        for (int k = 1; k <= 40; k++) {
            const double t = d.timeMs * k; if (t > span) break;
            const float x = in.getX() + in.getWidth() * (float)(t / span);
            const float h = in.getHeight() * 0.45f * (float)amp;
            if (h < 1) break;
            juce::Rectangle<float> bar;
            if (d.mode == DelayDsp::pingPong) bar = (k % 2) ? juce::Rectangle<float>(x - 3, in.getCentreY() - h, 6, h) : juce::Rectangle<float>(x - 3, in.getCentreY(), 6, h);
            else bar = juce::Rectangle<float>(x - 3, in.getCentreY() - h, 6, h * 2);
            g.setColour(accent.withAlpha(0.25f + 0.75f * (float)amp)); g.fillRoundedRectangle(bar, 3);
            amp *= fb;
        }
        const float lv = juce::jlimit(0.0f, 1.0f, ((float)gainToDb(echo) + 60) / 60);
        g.setColour(accent); g.fillRoundedRectangle(in.getRight() - 4, in.getBottom() - in.getHeight() * lv, 4, in.getHeight() * lv, 2);
        g.setFont(station::mono(11.0f, true)); g.setColour(ink);
        g.drawText(juce::String(d.timeMs, 1) + " ms" + (proc.p("sync") > 0.5f ? "   @ " + juce::String(proc.bpm.load(), 1) + " BPM" : ""), juce::Rectangle<float>(in.getRight() - 230, in.getY(), 222, 14), juce::Justification::right);
    }
private:
    DelayProcessor& proc; juce::Colour accent;
};

class DelayEditor : public Editor {
public:
    explicit DelayEditor(DelayProcessor& p)
        : Editor(p, "Delay", "Tape-style echoes", station::accents::delay, 940, 540), dp(p), taps(p, accent),
          mode(p.apvts, "mode"), sync(p.apvts, "sync", "Tempo sync"), division(p.apvts, "division", "Division"), time(p.apvts, "time", "Time", true),
          feedback(p.apvts, "feedback", "Feedback"), mix(p.apvts, "mix", "Mix"), lowcut(p.apvts, "lowcut", "Low cut"), highcut(p.apvts, "highcut", "High cut"),
          warmth(p.apvts, "warmth", "Warmth"), wow(p.apvts, "wow", "Wow"), duck(p.apvts, "duck", "Duck"), spread(p.apvts, "spread", "Spread") {
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &taps, &mode, &sync, &division, &time, &feedback, &mix, &lowcut, &highcut, &warmth, &wow, &duck, &spread })
            body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        auto top = r.removeFromTop(250);
        timeArea = top.removeFromRight(300); top.removeFromRight(12); tapArea = top;
        taps.setBounds(tapArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        auto c = timeArea.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        mode.setBounds(c.removeFromTop(36)); c.removeFromTop(12);
        sync.setBounds(c.removeFromTop(26)); c.removeFromTop(8);
        auto row = c;
        division.setBounds(row.removeFromLeft(row.getWidth() / 2).withSizeKeepingCentre(row.getWidth() / 2 - 10, 54));
        time.setBounds(row);
        r.removeFromTop(12);
        controlsArea = r;
        auto k = r.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        const int w = k.getWidth() / 8;
        for (auto* d : { &feedback, &mix, &lowcut, &highcut, &warmth, &wow, &duck, &spread }) d->setBounds(k.removeFromLeft(w));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, tapArea, "Echoes", "DL-01");
        module(g, timeArea, "Time", "DL-02");
        module(g, controlsArea, "Controls", "DL-03");
    }
    void tick() override {
        const bool s = dp.p("sync") > 0.5f;
        division.setEnabled(s); division.setAlpha(s ? 1.0f : 0.4f); time.setEnabled(!s); time.setAlpha(s ? 0.4f : 1.0f);
        spread.setAlpha(juce::roundToInt(dp.p("mode")) == DelayDsp::stereo ? 1.0f : 0.4f);
        taps.echo = dp.uiEcho.exchange(0); taps.repaint();
    }
private:
    DelayProcessor& dp; TapView taps; Segments mode; Switch sync; Choice division; Dial time, feedback, mix, lowcut, highcut, warmth, wow, duck, spread;
    juce::Rectangle<int> tapArea, timeArea, controlsArea;
};

juce::AudioProcessorEditor* DelayProcessor::createEditor() { return new DelayEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DelayProcessor(); }
