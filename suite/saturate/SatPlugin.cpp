// LGN Saturator: Tape, Tube, Transistor and Fuzz, 4x oversampled, with auto gain, tilt tone and mix.
#include "../common/Suite.h"
#include "../dsp/Saturate.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

const char* const kModeText[4] = {
    "Soft, symmetric rounding. Odd harmonics, glues and fattens without getting harsh.",
    "Asymmetric warmth. A strong second harmonic that thickens vocals, bass and keys.",
    "A firmer knee closer to a clipper. Adds bite and presence, keeps transients forward.",
    "Rectified and pushed hard. Distortion as an effect: grit, fuzz and attitude." };

class SatProcessor : public Processor {
public:
    SatProcessor() : Processor("LGNSaturate", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        param::addDb(l, "drive", "Drive", 0, 36, 9);
        param::addChoice(l, "mode", "Character", { "Tape", "Tube", "Transistor", "Fuzz" }, 0);
        param::add(l, "tone", "Tone", { -1, 1, 0.01f }, 0, "", 2);
        param::addPct(l, "mix", "Mix", 100);
        param::addDb(l, "out", "Output", -24, 12, 0);
        param::addBool(l, "autogain", "Auto Gain", true);
        return l;
    }
    SatParams params() const {
        SatParams s; s.driveDb = p("drive"); s.mode = juce::roundToInt(p("mode")); s.tone = p("tone"); s.mix = p("mix") / 100.0; s.outDb = p("out"); s.autoGain = p("autogain") > 0.5f;
        return s;
    }
    void prepareToPlay(double sr, int maxBlock) override {
        os = std::make_unique<juce::dsp::Oversampling<float>>(2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        os->initProcessing((size_t)maxBlock);
        latency = juce::roundToInt(os->getLatencyInSamples());
        setLatencySamples(latency);
        dry.setSize(2, maxBlock); dryDelay.assign(2, std::vector<float>((size_t)latency + 1, 0.0f)); dPos = 0;
        dsp.set(params()); dsp.prepare(sr);
        mixS.setTime(sr, 0.02); mixS.reset(params().mix);
    }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples(), ch = juce::jmin(2, getTotalNumInputChannels());
        for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); c++) buf.clear(c, 0, n);
        meterInput(buf, n);
        const auto sp = params(); dsp.set(sp);
        if (dry.getNumSamples() < n) dry.setSize(2, n, false, false, true);
        // dry path, delayed to line up with the oversampled wet path
        for (int c = 0; c < ch; c++) {
            const float* x = buf.getReadPointer(c); float* d = dry.getWritePointer(c); auto& line = dryDelay[(size_t)c];
            int w = dPos;
            for (int i = 0; i < n; i++) { line[(size_t)w] = x[i]; w = (w + 1) % (int)line.size(); d[i] = line[(size_t)w]; }
        }
        dPos = (dPos + n) % (int)dryDelay[0].size();
        // wet: up, shape, down
        juce::dsp::AudioBlock<float> block(buf.getArrayOfWritePointers(), (size_t)ch, (size_t)n);
        auto up = os->processSamplesUp(block);
        float* ups[2] = { up.getChannelPointer(0), ch > 1 ? up.getChannelPointer(1) : nullptr };
        dsp.shapeBlock(ups, ch, (int)up.getNumSamples(), getSampleRate() * 4);
        os->processSamplesDown(block);
        dsp.post(buf.getArrayOfWritePointers(), ch, n);
        // mix
        for (int i = 0; i < n; i++) {
            const float m = (float)mixS.next(sp.mix);
            for (int c = 0; c < ch; c++) { float* w = buf.getWritePointer(c); w[i] = dry.getSample(c, i) * (1 - m) + w[i] * m; }
        }
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 0.0; }
    juce::AudioProcessorEditor* createEditor() override;
private:
    SatDsp dsp; std::unique_ptr<juce::dsp::Oversampling<float>> os; int latency = 0;
    juce::AudioBuffer<float> dry; std::vector<std::vector<float>> dryDelay; int dPos = 0; Smooth mixS;
};

// The shaping curve for the current character and drive (after auto gain), with a faint unity line.
class CurveView : public juce::Component {
public:
    CurveView(SatProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        const auto in = r.reduced(14);
        g.setColour(hairline); g.drawHorizontalLine((int)in.getCentreY(), in.getX(), in.getRight()); g.drawVerticalLine((int)in.getCentreX(), in.getY(), in.getBottom());
        g.setColour(hairline.withAlpha(0.8f)); g.drawLine(in.getX(), in.getBottom(), in.getRight(), in.getY(), 1.0f);
        // input -1..1 (full scale) through the drive, normalised to the curve's own peak so the bend is what you see
        const auto sp = proc.params();
        const double dg = dbToGain(sp.driveDb);
        double top = 1e-9; for (int i = 0; i <= 200; i++) top = std::max(top, std::abs(SatDsp::shape(sp.mode, (-1 + i / 100.0) * dg)));
        juce::Path p;
        for (int i = 0; i <= 200; i++) {
            const double x = -1 + i / 100.0;
            const double y = (SatDsp::shape(sp.mode, x * dg) / top) * sp.mix + x * (1 - sp.mix);
            const float px = in.getX() + in.getWidth() * (float)((x + 1) / 2), py = in.getCentreY() - in.getHeight() / 2 * (float)y;
            i ? p.lineTo(px, py) : p.startNewSubPath(px, py);
        }
        g.setColour(accent); g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
private:
    SatProcessor& proc; juce::Colour accent;
};

// Harmonic fingerprint: relative level of harmonics 2 to 9 for the current character and drive.
class HarmonicsView : public juce::Component {
public:
    HarmonicsView(SatProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    void paint(juce::Graphics& g) override {
        const auto sp = proc.params();
        if (sp.mode != lastMode || std::abs(sp.driveDb - lastDrive) > 0.2) compute(sp);
        auto r = getLocalBounds().toFloat();
        const float bw = r.getWidth() / 8;
        for (int k = 0; k < 8; k++) {
            auto col = juce::Rectangle<float>(r.getX() + k * bw, r.getY(), bw, r.getHeight()).reduced(5, 0);
            auto bar = col.withTrimmedBottom(16);
            g.setColour(station::well); g.fillRoundedRectangle(bar, 4);
            const float t = juce::jlimit(0.0f, 1.0f, (h[k] + 72) / 72);
            g.setColour(k % 2 == 0 ? accent : accent.withAlpha(0.55f)); g.fillRoundedRectangle(bar.withTop(bar.getBottom() - bar.getHeight() * t), 4);
            g.setFont(station::mono(9.5f)); g.setColour(faint); g.drawText("H" + juce::String(k + 2), col.removeFromBottom(14), juce::Justification::centred);
        }
    }
private:
    void compute(const SatParams& sp) {
        lastMode = sp.mode; lastDrive = sp.driveDb;
        const int n = 2048; std::vector<double> y((size_t)n); const double g = dbToGain(sp.driveDb);
        for (int i = 0; i < n; i++) y[(size_t)i] = SatDsp::shape(sp.mode, 0.25 * g * std::sin(2 * kPi * 8 * i / n));
        auto mag = [&](int k) { double re = 0, im = 0; for (int i = 0; i < n; i++) { re += y[(size_t)i] * std::cos(2 * kPi * 8 * k * i / n); im += y[(size_t)i] * std::sin(2 * kPi * 8 * k * i / n); } return std::sqrt(re * re + im * im); };
        const double h1 = mag(1);
        for (int k = 0; k < 8; k++) h[k] = (float)gainToDb(mag(k + 2) / std::max(h1, 1e-12));
    }
    SatProcessor& proc; juce::Colour accent; float h[8] = {}; int lastMode = -1; double lastDrive = -1;
};

class SatEditor : public Editor {
public:
    explicit SatEditor(SatProcessor& p)
        : Editor(p, "Saturator", "Harmonic drive", station::accents::saturate, 900, 560), sp(p), curve(p, accent), harmonics(p, accent),
          mode(p.apvts, "mode"), autoGain(p.apvts, "autogain", "Auto gain"),
          drive(p.apvts, "drive", "Drive"), tone(p.apvts, "tone", "Tone"), mix(p.apvts, "mix", "Mix"), out(p.apvts, "out", "Output") {
        tone.slider.getProperties().set("bipolar", true);
        drive.maxKnob = 108;
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &curve, &harmonics, &mode, &autoGain, &drive, &tone, &mix, &out }) body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        auto top = r.removeFromTop(262);
        curveArea = top.removeFromLeft(262); top.removeFromLeft(12); charArea = top;
        curve.setBounds(curveArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        auto c = charArea.reduced(18, 12).withTrimmedTop(station::kModuleTitle - 8);
        mode.setBounds(c.removeFromTop(38)); c.removeFromTop(10);
        textArea = c.removeFromTop(36);
        c.removeFromTop(8);
        harmonics.setBounds(c);
        r.removeFromTop(12);
        controlsArea = r;
        auto k = r.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        auto right = k.removeFromRight(190);
        autoGain.setBounds(right.withSizeKeepingCentre(right.getWidth(), 26));
        drive.setBounds(k.removeFromLeft(170));
        k.removeFromLeft(10);
        const int w = 130;
        for (auto* d : { &tone, &mix, &out }) d->setBounds(k.removeFromLeft(w));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, curveArea, "Curve", "SA-01");
        module(g, charArea, "Character", "SA-02");
        module(g, controlsArea, "Controls", "SA-03");
        g.setFont(station::sans(13.0f, station::regular)); g.setColour(dim);
        g.drawFittedText(kModeText[juce::jlimit(0, 3, juce::roundToInt(sp.p("mode")))], textArea, juce::Justification::centredLeft, 2);
    }
    void tick() override {
        const int m = juce::roundToInt(sp.p("mode")); const float d = sp.p("drive"), t = sp.p("tone"), x = sp.p("mix");
        if (m != lastMode || d != lastDrive || t != lastTone || x != lastMix) { lastMode = m; lastDrive = d; lastTone = t; lastMix = x; curve.repaint(); harmonics.repaint(); body.repaint(textArea); }
    }
private:
    SatProcessor& sp; CurveView curve; HarmonicsView harmonics; Segments mode; Switch autoGain; Dial drive, tone, mix, out;
    juce::Rectangle<int> curveArea, charArea, controlsArea, textArea;
    int lastMode = -1; float lastDrive = -1, lastTone = -9, lastMix = -1;
};

juce::AudioProcessorEditor* SatProcessor::createEditor() { return new SatEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SatProcessor(); }
