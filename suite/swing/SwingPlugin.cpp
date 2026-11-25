// LGN Swing: gives any audio a swing or a hand-shaped groove, locked to the song's grid.
#include "../common/Suite.h"
#include "../dsp/Swing.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

const juce::Colour kAccent{ 0xffd0533b };   // brick
juce::String stepId(int s) { return "s" + juce::String(s + 1); }

struct Column { float in = 0, out = 0; double ppqIn = 0, ppqOut = 0; bool playing = false; };
class ColumnRing {
public:
    static constexpr int kSize = 4096;
    void push(const Column& c) { const int w = write.load(std::memory_order_relaxed); buf[(size_t)w] = c; write.store((w + 1) & (kSize - 1), std::memory_order_release); }
    int readNew(int& rp, Column* out, int maxOut) const {
        const int w = write.load(std::memory_order_acquire); int avail = (w - rp) & (kSize - 1);
        if (avail > kSize / 2) { rp = (w - kSize / 2) & (kSize - 1); avail = kSize / 2; }
        const int n = std::min(avail, maxOut);
        for (int i = 0; i < n; i++) out[i] = buf[(size_t)((rp + i) & (kSize - 1))];
        rp = (rp + n) & (kSize - 1); return n;
    }
    int writePos() const { return write.load(std::memory_order_acquire); }
private:
    std::array<Column, kSize> buf{}; std::atomic<int> write{ 0 };
};

class SwingProcessor : public Processor {
public:
    SwingProcessor() : Processor("LGNSwing", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        param::addChoice(l, "grid", "Grid", { "1/8", "1/16" }, 1);
        param::add(l, "swing", "Swing", { 50, 80, 0.1f }, 58, "%", 1);
        param::addMs(l, "humanize", "Humanize", 0, 20, 0, 6);
        param::addChoice(l, "tail", "Gaps", { "Repeat", "Fade" }, 0);
        for (int s = 0; s < 16; s++) param::add(l, stepId(s).toRawUTF8(), ("Step " + juce::String(s + 1)).toRawUTF8(), { -30, 30, 0.1f }, 0, "%", 1);
        return l;
    }
    SwingParams params() const {
        SwingParams s; s.grid = juce::roundToInt(p("grid")); s.swingPct = p("swing"); s.humanizeMs = p("humanize"); s.tail = juce::roundToInt(p("tail"));
        for (int i = 0; i < 16; i++) s.step[(size_t)i] = p(ids[(size_t)i].toRawUTF8()) / 100.0;
        return s;
    }
    void prepareToPlay(double sr, int) override {
        rate = sr; dsp.set(params()); dsp.prepare(sr); setLatencySamples(dsp.latency());
        colLen = juce::jmax(16, juce::roundToInt(sr * 0.0025)); colCount = 0; col = {}; fallbackPpq = 0;
    }
    void reset() override { dsp.reset(); }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples(), ch = juce::jmin(2, getTotalNumInputChannels());
        for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); c++) buf.clear(c, 0, n);
        double bpmNow = 120, ppq = fallbackPpq; bool playing = false;
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) {
            if (auto b = pos->getBpm()) bpmNow = juce::jlimit(20.0, 999.0, *b);
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }
        const double pps = bpmNow / 60.0 / rate;
        fallbackPpq = ppq + n * pps;
        meterInput(buf, n);
        // input peaks per sample, before the move
        for (int i = 0; i < n && i < (int)inAbs.size(); i++) { float a = std::abs(buf.getSample(0, i)); if (ch > 1) a = juce::jmax(a, std::abs(buf.getSample(1, i))); inAbs[(size_t)i] = a; }
        dsp.set(params());
        float* ptrs[2] = { buf.getWritePointer(0), ch > 1 ? buf.getWritePointer(1) : nullptr };
        dsp.process(ptrs, ch, n, ppq, bpmNow, playing);
        // the display's columns: input at its own song position, output at the position it will be heard
        // (the host plays it back early by the plugin's latency, so it lines up with the input's timeline)
        const double outShift = dsp.latency() * pps;
        for (int i = 0; i < n; i++) {
            float o = std::abs(buf.getSample(0, i)); if (ch > 1) o = juce::jmax(o, std::abs(buf.getSample(1, i)));
            if (colCount == 0) { col = {}; col.ppqIn = ppq + i * pps; col.ppqOut = ppq + i * pps - outShift; col.playing = playing; }
            col.in = juce::jmax(col.in, i < (int)inAbs.size() ? inAbs[(size_t)i] : 0.0f); col.out = juce::jmax(col.out, o);
            if (++colCount >= colLen) { columns.push(col); colCount = 0; }
        }
        uiPpq = ppq + n * pps; uiBpm = bpmNow; uiPlaying = playing;
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 0.0; }
    juce::AudioProcessorEditor* createEditor() override;
    double offsetMs(int s) const { return dsp.offsetMs(s, uiBpm.load()); }
    double stepBeats() const { return juce::roundToInt(p("grid")) == 0 ? 0.5 : 0.25; }

    ColumnRing columns; std::atomic<double> uiPpq{ 0 }, uiBpm{ 120 }; std::atomic<bool> uiPlaying{ false };
private:
    SwingDsp dsp; double rate = 48000, fallbackPpq = 0;
    std::array<juce::String, 16> ids = [] { std::array<juce::String, 16> a; for (int i = 0; i < 16; i++) a[(size_t)i] = stepId(i); return a; }();
    std::array<float, 8192> inAbs{};
    Column col; int colLen = 120, colCount = 0;
};

// One pattern (16 steps) of your audio on the grid: before (grey, top) and after (brick, bottom), the straight
// grid in hairlines, and a handle on every step where it now lands. Drag a handle to push or drag that step.
class GrooveView : public juce::Component {
public:
    GrooveView(SwingProcessor& p, juce::Colour a) : proc(p), accent(a) { readPos = p.columns.writePos(); }
    void update() {
        const double len = 16 * proc.stepBeats();
        if (len != windowBeats) { windowBeats = len; for (auto& b : before) b = 0; for (auto& b : after) b = 0; }
        Column fresh[512];
        int n;
        while ((n = proc.columns.readNew(readPos, fresh, 512)) > 0)
            for (int i = 0; i < n; i++) {
                if (!fresh[i].playing) continue;
                scale = juce::jmax(scale * 0.9995f, fresh[i].in);
                put(before, lastIn, fresh[i].ppqIn, fresh[i].in);
                put(after, lastOut, fresh[i].ppqOut, fresh[i].out);
            }
        repaint();
    }
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        auto area = r.reduced(14, 12); area.removeFromBottom(12);
        const auto top = area.removeFromTop(area.getHeight() * 0.5f).reduced(0, 6), bottom = area.reduced(0, 6);
        lanes = { top, bottom };
        // grid
        for (int k = 0; k <= 16; k++) {
            const float x = top.getX() + top.getWidth() * k / 16.0f;
            g.setColour(k % 4 == 0 ? station::hairlineHi : hairline);
            g.drawVerticalLine((int)x, top.getY(), bottom.getBottom());
            if (k < 16 && k % 4 == 0) { g.setFont(station::mono(9.0f)); g.setColour(faint); g.drawText(juce::String(k / 4 + 1), juce::Rectangle<float>(x + 3, bottom.getBottom() + 1, 20, 11), juce::Justification::centredLeft); }
        }
        auto wave = [&](const std::array<float, kBins>& b, juce::Rectangle<float> lane, juce::Colour c) {
            const float norm = 0.95f / juce::jmax(0.03f, scale), mid = lane.getCentreY(), half = lane.getHeight() * 0.5f;
            juce::Path p; p.startNewSubPath(lane.getX(), mid);
            for (int i = 0; i < kBins; i++) p.lineTo(lane.getX() + lane.getWidth() * i / (float)(kBins - 1), mid - juce::jmin(1.0f, b[(size_t)i] * norm) * half);
            for (int i = kBins - 1; i >= 0; i--) p.lineTo(lane.getX() + lane.getWidth() * i / (float)(kBins - 1), mid + juce::jmin(1.0f, b[(size_t)i] * norm) * half);
            p.closeSubPath(); g.setColour(c); g.fillPath(p);
        };
        wave(before, top, juce::Colour(0xffcfd5dc));
        wave(after, bottom, accent.withAlpha(0.85f));
        station::drawCaps(g, "Before", juce::Rectangle<float>(top.getX() + 6, top.getY(), 60, 12), juce::Justification::centredLeft, 8.5f, dim);
        station::drawCaps(g, "After", juce::Rectangle<float>(bottom.getX() + 6, bottom.getY(), 60, 12), juce::Justification::centredLeft, 8.5f, dim);
        // where each step lands
        const double stepMs = proc.stepBeats() * 60000.0 / juce::jmax(20.0, proc.uiBpm.load());
        for (int s = 0; s < 16; s++) {
            const float x0 = top.getX() + top.getWidth() * s / 16.0f;
            const float x1 = x0 + top.getWidth() / 16.0f * (float)(proc.offsetMs(s) / stepMs);
            g.setColour(accent.withAlpha(0.35f)); g.drawLine(x0, top.getBottom(), x1, bottom.getY(), 1.0f);
            const float d = s == dragging ? 7.0f : 5.5f;
            juce::Path dia; dia.addQuadrilateral(x1, bottom.getY() - d, x1 + d, bottom.getY(), x1, bottom.getY() + d, x1 - d, bottom.getY());
            g.setColour(s % 2 ? accent : station::graphite); g.fillPath(dia);
        }
        // playhead
        if (proc.uiPlaying.load()) {
            double f = std::fmod(proc.uiPpq.load(), windowBeats) / windowBeats; if (f < 0) f += 1;
            const float x = top.getX() + top.getWidth() * (float)f;
            g.setColour(ink); g.fillRect(x - 0.75f, top.getY(), 1.5f, bottom.getBottom() - top.getY());
        } else station::drawCaps(g, "Press play: Swing follows your song's grid", juce::Rectangle<float>(r.getX(), r.getCentreY() - 7, r.getWidth(), 14), juce::Justification::centred, 9.5f, dim);
    }
    void mouseDown(const juce::MouseEvent& e) override {
        dragging = -1; float best = 14;
        const auto& bottom = lanes[1];
        const double stepMs = proc.stepBeats() * 60000.0 / juce::jmax(20.0, proc.uiBpm.load());
        for (int s = 0; s < 16; s++) {
            const float x = bottom.getX() + bottom.getWidth() * s / 16.0f + bottom.getWidth() / 16.0f * (float)(proc.offsetMs(s) / stepMs);
            const float d = std::hypot(e.position.x - x, e.position.y - bottom.getY());
            if (d < best) { best = d; dragging = s; }
        }
        if (dragging >= 0) { prm(dragging)->beginChangeGesture(); dragX = e.position.x; startVal = prm(dragging)->convertFrom0to1(prm(dragging)->getValue()); }
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragging < 0) return;
        const float stepW = lanes[1].getWidth() / 16.0f;
        const float v = juce::jlimit(-30.0f, 30.0f, startVal + (e.position.x - dragX) / stepW * 100.0f);
        prm(dragging)->setValueNotifyingHost(prm(dragging)->convertTo0to1(v));
    }
    void mouseUp(const juce::MouseEvent&) override { if (dragging >= 0) prm(dragging)->endChangeGesture(); dragging = -1; }
    void mouseDoubleClick(const juce::MouseEvent& e) override {   // double-click a handle: back to the swing setting
        mouseDown(e); if (dragging >= 0) { prm(dragging)->setValueNotifyingHost(prm(dragging)->convertTo0to1(0)); prm(dragging)->endChangeGesture(); dragging = -1; }
    }
private:
    static constexpr int kBins = 480;
    juce::RangedAudioParameter* prm(int s) const { return proc.apvts.getParameter(stepId(s)); }
    void put(std::array<float, kBins>& b, int& last, double ppq, float v) {
        double f = std::fmod(ppq, windowBeats) / windowBeats; if (f < 0) f += 1;
        const int k = juce::jlimit(0, kBins - 1, (int)(f * kBins));
        if (last >= 0 && k != last) { for (int j = (last + 1) % kBins; j != k; j = (j + 1) % kBins) b[(size_t)j] = v; b[(size_t)k] = 0; }
        last = k; b[(size_t)k] = juce::jmax(b[(size_t)k], v);
    }
    SwingProcessor& proc; juce::Colour accent; int readPos = 0, lastIn = -1, lastOut = -1, dragging = -1; float dragX = 0, startVal = 0, scale = 0.1f;
    double windowBeats = 4; std::array<float, kBins> before{}, after{}; std::array<juce::Rectangle<float>, 2> lanes;
};

class SwingEditor : public Editor {
public:
    explicit SwingEditor(SwingProcessor& p)
        : Editor(p, "Swing", "Groove for any audio", kAccent, 940, 560), sp(p), groove(p, accent),
          grid(p.apvts, "grid"), tail(p.apvts, "tail"), swing(p.apvts, "swing", "Swing"), humanize(p.apvts, "humanize", "Humanize") {
        swing.maxKnob = 104;
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &groove, &grid, &tail, &swing, &humanize }) body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        grooveArea = r.removeFromTop(290);
        groove.setBounds(grooveArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        r.removeFromTop(12);
        feelArea = r.removeFromLeft(560); r.removeFromLeft(12); gridArea = r;
        auto f = feelArea.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        swing.setBounds(f.removeFromLeft(f.getWidth() / 2)); humanize.setBounds(f);
        auto g2 = gridArea.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        gridLabel = g2.removeFromTop(14); g2.removeFromTop(4);
        grid.setBounds(g2.removeFromTop(36)); g2.removeFromTop(16);
        tailLabel = g2.removeFromTop(14); g2.removeFromTop(4);
        tail.setBounds(g2.removeFromTop(36));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, grooveArea, "Groove", "SW-01");
        module(g, feelArea, "Feel", "SW-02");
        module(g, gridArea, "Grid", "SW-03");
        station::drawCaps(g, "Swing on", gridLabel.toFloat(), juce::Justification::centredLeft, 9.5f, dim);
        station::drawCaps(g, "Gaps between moved steps", tailLabel.toFloat(), juce::Justification::centredLeft, 9.5f, dim);
    }
    void tick() override { groove.update(); }
private:
    SwingProcessor& sp; GrooveView groove; Segments grid, tail; Dial swing, humanize;
    juce::Rectangle<int> grooveArea, feelArea, gridArea, gridLabel, tailLabel;
};

juce::AudioProcessorEditor* SwingProcessor::createEditor() { return new SwingEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SwingProcessor(); }
