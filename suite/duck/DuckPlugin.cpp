// LGN Duck: tempo-synced or kick-triggered volume ducking, with a live waveform of the ducking.
// Keeps the plugin code, parameter IDs and state name of the original LGN Duck so saved projects still load.
#include "../common/Suite.h"
#include "../dsp/Duck.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

const juce::StringArray kRates{ "1/1", "1/2", "1/4", "1/8", "1/16" };
constexpr double kRateBeats[5] = { 4.0, 2.0, 1.0, 0.5, 0.25 };

// One column of the waveform display: ~2.5 ms of audio.
struct Column {
    float in = 0, out = 0, gain = 1, sc = 0;   // input / output peak, lowest gain, sidechain peak
    double ppq = 0; bool playing = false, hit = false;
};

// Single-writer (audio thread), single-reader (UI) ring of columns.
class ColumnRing {
public:
    static constexpr int kSize = 4096;
    void push(const Column& c) { const int w = write.load(std::memory_order_relaxed); buf[(size_t)w] = c; write.store((w + 1) & (kSize - 1), std::memory_order_release); }
    // Copies every column written since the reader's last call (at most kSize / 2) into out; returns the count.
    int readNew(int& readPos, Column* out, int maxOut) const {
        const int w = write.load(std::memory_order_acquire);
        int avail = (w - readPos) & (kSize - 1);
        if (avail > kSize / 2) { readPos = (w - kSize / 2) & (kSize - 1); avail = kSize / 2; }   // fell behind: skip ahead
        const int n = std::min(avail, maxOut);
        for (int i = 0; i < n; i++) out[i] = buf[(size_t)((readPos + i) & (kSize - 1))];
        readPos = (readPos + n) & (kSize - 1);
        return n;
    }
    int writePos() const { return write.load(std::memory_order_acquire); }
private:
    std::array<Column, kSize> buf{};
    std::atomic<int> write{ 0 };
};

class DuckProcessor : public Processor {
public:
    DuckProcessor()
        : Processor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                                     .withInput("Sidechain (Kick)", juce::AudioChannelSet::stereo(), true),   // on by default so hosts feed it
                    "LGNDuck", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        param::addPct(l, "amount", "Amount", 75);
        param::addPct(l, "release", "Release", 50);
        param::addChoice(l, "rate", "Rate", kRates, 2);
        param::addChoice(l, "mode", "Mode", { "Tempo", "Kick" }, 0);
        return l;
    }
    bool isBusesLayoutSupported(const BusesLayout& b) const override {
        const auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
        if (in != out || (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())) return false;
        if (b.inputBuses.size() > 1) {
            const auto sc = b.getChannelSet(true, 1);
            if (!sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo()) return false;
        }
        return true;
    }
    void prepareToPlay(double sr, int) override {
        rate = sr; dsp.prepare(sr); fallbackPpq = 0;
        colLen = juce::jmax(16, juce::roundToInt(sr * 0.0025)); colCount = 0; col = {};
    }
    void reset() override { dsp.reset(); }
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals nd;
        const int n = buffer.getNumSamples();
        auto main = getBusBuffer(buffer, true, 0);
        for (int c = getMainBusNumInputChannels(); c < getMainBusNumOutputChannels(); ++c) main.clear(c, 0, n);
        meterInput(buffer, n);

        double bpmNow = 120, ppq = fallbackPpq; bool playing = false;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition()) {
                if (auto b = pos->getBpm()) bpmNow = juce::jlimit(20.0, 999.0, *b);
                playing = pos->getIsPlaying();
                if (auto q = pos->getPpqPosition()) ppq = *q;
            }
        const double ppqPerSample = bpmNow / 60.0 / rate;
        fallbackPpq = ppq + n * ppqPerSample;

        dsp.amount = p("amount") / 100.0;
        dsp.release = p("release") / 100.0;
        dsp.mode = juce::roundToInt(p("mode"));
        dsp.rateBeats = kRateBeats[juce::jlimit(0, 4, juce::roundToInt(p("rate")))];

        const float* sc[2] = { nullptr, nullptr }; int numSc = 0;
        if (getBusCount(true) > 1 && getBus(true, 1)->isEnabled()) {
            auto scBuf = getBusBuffer(buffer, true, 1);
            numSc = juce::jmin(2, scBuf.getNumChannels());
            for (int c = 0; c < numSc; ++c) sc[c] = scBuf.getReadPointer(c);
        }
        const int nch = juce::jmin(2, main.getNumChannels());
        float* chp[2] = { main.getWritePointer(0), nch > 1 ? main.getWritePointer(1) : nullptr };

        // run in short chunks so the per-sample scratch buffers never need to grow on the audio thread
        float scPeakBlock = 0;
        for (int o = 0; o < n; o += kChunk) {
            const int m = juce::jmin(kChunk, n - o);
            for (int i = 0; i < m; i++) {
                float a = std::abs(chp[0][o + i]); if (nch > 1) a = juce::jmax(a, std::abs(chp[1][o + i]));
                inAbs[(size_t)i] = a;
                float s = 0; for (int c = 0; c < numSc; c++) s = juce::jmax(s, std::abs(sc[c][o + i]));
                scAbs[(size_t)i] = s; scPeakBlock = juce::jmax(scPeakBlock, s);
            }
            float* cp[2] = { chp[0] + o, nch > 1 ? chp[1] + o : nullptr };
            const float* sp[2] = { sc[0] ? sc[0] + o : nullptr, sc[1] ? sc[1] + o : nullptr };
            dsp.process(cp, nch, m, ppq + o * ppqPerSample, bpmNow, playing, numSc ? sp : nullptr, numSc, gains.data(), hits.data());
            for (int i = 0; i < m; i++) {
                if (colCount == 0) { col = {}; col.ppq = ppq + (o + i) * ppqPerSample; col.playing = playing; }
                col.in = juce::jmax(col.in, inAbs[(size_t)i]);
                col.out = juce::jmax(col.out, inAbs[(size_t)i] * gains[(size_t)i]);
                col.gain = juce::jmin(col.gain, gains[(size_t)i]);
                col.sc = juce::jmax(col.sc, scAbs[(size_t)i]);
                col.hit = col.hit || hits[(size_t)i];
                if (++colCount >= colLen) { columns.push(col); colCount = 0; }
            }
        }
        meterOutput(buffer, n);

        uiReduction = (float)dsp.reduction;
        uiHitAge = (float)dsp.lastHitAge;
        uiPlaying = playing;
        uiBpm = (float)bpmNow;
        uiPpq = ppq + n * ppqPerSample;
        uiSidechainLevel = juce::jmax(scPeakBlock, uiSidechainLevel.load() * 0.9f);
        uiSidechainBus = numSc > 0;
    }
    double getTailLengthSeconds() const override { return 0.0; }
    juce::AudioProcessorEditor* createEditor() override;

    ColumnRing columns;
    std::atomic<float> uiReduction{ 0 }, uiHitAge{ 1e9f }, uiBpm{ 120 }, uiSidechainLevel{ 0 };
    std::atomic<double> uiPpq{ 0 };
    std::atomic<bool> uiPlaying{ false }, uiSidechainBus{ false };
private:
    static constexpr int kChunk = 256;
    DuckDsp dsp;
    double fallbackPpq = 0, rate = 48000;
    std::array<float, kChunk> inAbs{}, scAbs{}, gains{};
    std::array<uint8_t, kChunk> hits{};
    Column col; int colLen = 120, colCount = 0;
};

// ---------------------------------------------------------------- the waveform
// Tempo mode (song playing): a window of one or two bars pinned to the song position, so each pump sits in
// place on the beat grid and a playhead sweeps across. Kick mode, or a stopped song: the last couple of
// seconds scroll from right to left, with every detected kick marked.
class WaveView : public juce::Component {
public:
    WaveView(DuckProcessor& p, juce::Colour a) : proc(p), accent(a) { readPos = p.columns.writePos(); }

    void update() {
        const int mode = juce::roundToInt(proc.p("mode"));
        const bool tempo = mode == 0 && proc.uiPlaying.load();
        const int rate = juce::jlimit(0, 4, juce::roundToInt(proc.p("rate")));
        const double window = rate == 0 ? 8.0 : 4.0;   // beats shown in tempo mode
        if (tempo != tempoView || window != windowBeats) { tempoView = tempo; windowBeats = window; clearBins(); }
        auto* fresh = freshBuf.data();
        const int n = proc.columns.readNew(readPos, fresh, (int)freshBuf.size());
        for (int i = 0; i < n; i++) {
            const auto& c = fresh[i];
            scale = juce::jmax(scale * 0.9995f, c.in);
            if (tempoView) {
                if (!c.playing) continue;
                const int nb = (int)bins.size();
                double f = std::fmod(c.ppq, windowBeats) / windowBeats; if (f < 0) f += 1;
                const int b = juce::jlimit(0, nb - 1, (int)(f * nb));
                if (lastBin >= 0 && b != lastBin) {
                    // the playhead moved on: bins it skipped get this column (slow tempos have fewer columns than
                    // bins), unless it jumped (a loop or a locate), in which case they are cleared
                    const int gap = (b - lastBin + nb) % nb;
                    for (int k = (lastBin + 1) % nb; k != b; k = (k + 1) % nb) bins[(size_t)k] = gap <= 8 ? Bin(c, true) : Bin();
                    bins[(size_t)b] = {};
                }
                lastBin = b;
                auto& d = bins[(size_t)b];
                if (!d.used) d = Bin(c, true);
                else { d.in = juce::jmax(d.in, c.in); d.out = juce::jmax(d.out, c.out); d.gain = juce::jmin(d.gain, c.gain); d.sc = juce::jmax(d.sc, c.sc); d.hit |= c.hit; }
            } else {
                history[(size_t)histPos] = { c, true }; histPos = (histPos + 1) % kHistory;
            }
        }
        repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        auto plot = r.reduced(14, 12);
        const bool kick = juce::roundToInt(proc.p("mode")) == 1;
        auto lane = kick ? plot.removeFromBottom(26) : juce::Rectangle<float>();
        if (kick) plot.removeFromBottom(8);
        plot.removeFromBottom(12);   // time labels
        const float mid = plot.getCentreY(), half = plot.getHeight() * 0.5f;
        const float norm = 0.92f / juce::jmax(0.03f, scale);

        // grid
        g.setFont(station::mono(9.0f));
        if (tempoView) {
            const int beats = (int)windowBeats;
            for (int k = 0; k <= beats * 4; k++) {
                const float x = plot.getX() + plot.getWidth() * (float)k / (beats * 4);
                const bool beat = k % 4 == 0, bar = k % 16 == 0;
                g.setColour(bar ? station::hairlineHi : beat ? hairline : hairline.withAlpha(0.45f));
                g.drawVerticalLine((int)x, plot.getY(), plot.getBottom());
                if (beat && k < beats * 4) {
                    g.setColour(faint);
                    g.drawText(juce::String(k / 16 + 1) + "." + juce::String(k / 4 % 4 + 1), juce::Rectangle<float>(x + 3, plot.getBottom() + 1, 40, 11), juce::Justification::centredLeft);
                }
            }
        } else {
            const double secs = kHistory * 0.0025;
            for (int k = 0; k <= 4; k++) {
                const float x = plot.getRight() - plot.getWidth() * k / 4.0f;
                g.setColour(hairline); g.drawVerticalLine((int)x, plot.getY(), plot.getBottom());
                g.setColour(faint);
                const auto box = k == 4 ? juce::Rectangle<float>(x + 3, plot.getBottom() + 1, 44, 11) : juce::Rectangle<float>(x - 47, plot.getBottom() + 1, 44, 11);
                g.drawText(k == 0 ? juce::String("now") : "-" + juce::String(secs * k / 4, 1) + " s", box, k == 4 ? juce::Justification::centredLeft : juce::Justification::centredRight);
            }
        }
        g.setColour(hairline); g.drawHorizontalLine((int)mid, plot.getX(), plot.getRight());

        // gather the columns to draw, left to right
        const int W = juce::jmax(1, (int)plot.getWidth());
        std::vector<Bin> cols((size_t)W);
        if (tempoView) {
            for (int x = 0; x < W; x++) {
                const int b0 = x * (int)bins.size() / W, b1 = juce::jmax(b0 + 1, (x + 1) * (int)bins.size() / W);
                for (int b = b0; b < b1; b++) merge(cols[(size_t)x], bins[(size_t)b]);
            }
        } else {
            for (int x = 0; x < W; x++) {
                const int h0 = (int)((int64_t)x * kHistory / W), h1 = juce::jmax(h0 + 1, (int)((int64_t)(x + 1) * kHistory / W));
                for (int h = h0; h < h1; h++) merge(cols[(size_t)x], history[(size_t)((histPos + h) % kHistory)]);
            }
        }

        // input (what it would have been) in grey, output (ducked) in the accent colour
        auto wave = [&](bool out) {
            juce::Path p; bool started = false; int firstX = 0, lastX = 0;
            for (int x = 0; x < W; x++) {
                const auto& c = cols[(size_t)x]; if (!c.used) continue;
                const float a = juce::jmin(1.0f, (out ? c.out : c.in) * norm) * half;
                const float px = plot.getX() + (float)x;
                if (!started) { p.startNewSubPath(px, mid - a); started = true; firstX = x; } else p.lineTo(px, mid - a);
                lastX = x;
            }
            if (!started) return p;
            for (int x = lastX; x >= firstX; x--) {
                const auto& c = cols[(size_t)x]; if (!c.used) continue;
                p.lineTo(plot.getX() + (float)x, mid + juce::jmin(1.0f, (out ? c.out : c.in) * norm) * half);
            }
            p.closeSubPath(); return p;
        };
        g.setColour(juce::Colour(0xffdde2e8)); g.fillPath(wave(false));
        g.setColour(accent.withAlpha(0.85f)); g.fillPath(wave(true));

        // the volume line: full height = full volume
        juce::Path gl; bool gs = false;
        for (int x = 0; x < W; x++) {
            const auto& c = cols[(size_t)x]; if (!c.used) { gs = false; continue; }
            const float y = plot.getBottom() - plot.getHeight() * c.gain, px = plot.getX() + (float)x;
            if (!gs) { gl.startNewSubPath(px, y); gs = true; } else gl.lineTo(px, y);
        }
        g.setColour(ink.withAlpha(0.75f)); g.strokePath(gl, juce::PathStrokeType(1.5f));

        // kick lane: the sidechain signal, with a mark on every detected hit
        if (kick) {
            g.setColour(station::track); g.fillRoundedRectangle(lane, 4);
            float scMax = 0.001f; for (auto& c : cols) scMax = juce::jmax(scMax, c.sc);
            for (int x = 0; x < W; x++) {
                const auto& c = cols[(size_t)x]; if (!c.used) continue;
                const float h = (lane.getHeight() - 6) * juce::jmin(1.0f, c.sc / scMax);
                if (h >= 1.0f) { g.setColour(station::graphite.withAlpha(0.55f)); g.fillRect(lane.getX() + x, lane.getCentreY() - h / 2, 1.0f, h); }
                if (c.hit) {
                    const float px = plot.getX() + x;
                    g.setColour(accent.withAlpha(0.35f)); g.drawVerticalLine((int)px, plot.getY(), plot.getBottom());
                    g.setColour(accent); g.fillEllipse(px - 3.5f, lane.getY() - 4.5f, 7, 7);
                }
            }
            station::drawCaps(g, "Kick in", lane.reduced(8, 0).withWidth(80), juce::Justification::centredLeft, 8.5f, dim);
        }

        // playhead in tempo mode
        if (tempoView) {
            double f = std::fmod(proc.uiPpq.load(), windowBeats) / windowBeats; if (f < 0) f += 1;
            const float x = plot.getX() + plot.getWidth() * (float)f;
            g.setColour(ink); g.fillRect(x - 0.75f, plot.getY(), 1.5f, plot.getHeight());
        }

        // readout
        const float red = proc.uiReduction.load();
        g.setFont(station::mono(11.0f, true)); g.setColour(ink);
        const juce::String db = red > 0.0005f ? juce::String(juce::Decibels::gainToDecibels(1.0f - red, -60.0f), 1) + " dB" : juce::String("0.0 dB");
        g.drawText(db, juce::Rectangle<float>(plot.getRight() - 120, plot.getY() + 2, 112, 14), juce::Justification::right);
        // legend: swatch, then its label
        const float ly = plot.getY() + 4, lx = plot.getX() + 8;
        g.setColour(juce::Colour(0xffcfd5dc)); g.fillRect(lx, ly + 3.5f, 9.0f, 5.0f);
        station::drawCaps(g, "Before", juce::Rectangle<float>(lx + 14, ly, 60, 12), juce::Justification::centredLeft, 8.5f, dim);
        g.setColour(accent); g.fillRect(lx + 72, ly + 3.5f, 9.0f, 5.0f);
        station::drawCaps(g, "After", juce::Rectangle<float>(lx + 86, ly, 50, 12), juce::Justification::centredLeft, 8.5f, dim);
        g.setColour(ink); g.fillRect(lx + 136, ly + 5.25f, 9.0f, 1.5f);
        station::drawCaps(g, "Volume", juce::Rectangle<float>(lx + 150, ly, 60, 12), juce::Justification::centredLeft, 8.5f, dim);
    }

private:
    struct Bin : Column { bool used = false; Bin() = default; Bin(const Column& c, bool u) : Column(c), used(u) {} };
    static void merge(Bin& d, const Bin& s) {
        if (!s.used) return;
        if (!d.used) { d = s; return; }
        d.in = juce::jmax(d.in, s.in); d.out = juce::jmax(d.out, s.out); d.gain = juce::jmin(d.gain, s.gain); d.sc = juce::jmax(d.sc, s.sc); d.hit |= s.hit;
    }
    void clearBins() { for (auto& b : bins) b = {}; lastBin = -1; }
    static constexpr int kHistory = 900;   // 2.25 s of columns
    DuckProcessor& proc; juce::Colour accent;
    int readPos = 0;
    std::vector<Column> freshBuf = std::vector<Column>(ColumnRing::kSize / 2);
    std::array<Bin, 1200> bins{}; int lastBin = -1;
    std::array<Bin, kHistory> history{}; int histPos = 0;
    bool tempoView = false; double windowBeats = 4; float scale = 0.1f;
};

// One pump: the volume over one beat division (tempo) or after one kick, with a dot where it is now.
class ShapeView : public juce::Component {
public:
    ShapeView(DuckProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        auto plot = r.reduced(14, 14).withTrimmedBottom(10);
        const int mode = juce::roundToInt(proc.p("mode")), rate = juce::jlimit(0, 4, juce::roundToInt(proc.p("rate")));
        const double amt = proc.p("amount") / 100.0, rel = proc.p("release") / 100.0;
        const double span = mode == 0 ? 1.0 : 0.7, len = mode == 0 ? DuckDsp::tempoLength(rel) : DuckDsp::kickSeconds(rel);
        for (int k = 1; k < 4; k++) { g.setColour(hairline); g.drawHorizontalLine((int)(plot.getY() + plot.getHeight() * k / 4), plot.getX(), plot.getRight()); }
        juce::Path line, fill;
        for (int i = 0; i <= 160; ++i) {
            const double t = span * i / 160.0, gn = 1 - amt * DuckDsp::curve(t / len);
            const float x = plot.getX() + plot.getWidth() * (float)i / 160.0f, y = plot.getBottom() - plot.getHeight() * (float)gn;
            if (i == 0) { line.startNewSubPath(x, y); fill.startNewSubPath(x, plot.getBottom()); } else line.lineTo(x, y);
            fill.lineTo(x, y);
        }
        fill.lineTo(plot.getRight(), plot.getBottom()); fill.closeSubPath();
        g.setColour(accent.withAlpha(0.14f)); g.fillPath(fill);
        g.setColour(accent); g.strokePath(line, juce::PathStrokeType(2.0f));
        g.setFont(station::mono(9.0f)); g.setColour(faint);
        const double bpm = proc.uiBpm.load();
        const juce::String lenText = mode == 0
            ? "one " + kRates[rate] + " note  " + juce::String(juce::roundToInt(kRateBeats[rate] * 60000.0 / bpm)) + " ms"
            : "dip " + juce::String(juce::roundToInt(len * 1000)) + " ms after each kick";
        g.drawText(lenText, juce::Rectangle<float>(plot.getX(), plot.getBottom() + 2, plot.getWidth(), 11), juce::Justification::centredRight);
        // live dot
        float x = -1;
        if (mode == 0 && proc.uiPlaying.load()) {
            double ph = std::fmod(proc.uiPpq.load(), kRateBeats[rate]) / kRateBeats[rate]; if (ph < 0) ph += 1;
            x = plot.getX() + plot.getWidth() * (float)ph;
        } else if (mode == 1 && proc.uiHitAge.load() < span) x = plot.getX() + plot.getWidth() * proc.uiHitAge.load() / (float)span;
        if (x >= 0) {
            const float y = plot.getBottom() - plot.getHeight() * (1 - proc.uiReduction.load());
            g.setColour(accent.withAlpha(0.2f)); g.fillEllipse(x - 8, y - 8, 16, 16);
            g.setColour(accent); g.fillEllipse(x - 4, y - 4, 8, 8);
        }
    }
private:
    DuckProcessor& proc; juce::Colour accent;
};

class DuckEditor : public Editor {
public:
    explicit DuckEditor(DuckProcessor& p)
        : Editor(p, "Duck", "Sidechain / volume pump", station::accents::duck, 940, 560), dp(p), wave(p, accent), shape(p, accent),
          mode(p.apvts, "mode"), rate(p.apvts, "rate"), amount(p.apvts, "amount", "Amount"), release(p.apvts, "release", "Release") {
        amount.maxKnob = 104;
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &wave, &shape, &mode, &rate, &amount, &release }) body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        waveArea = r.removeFromTop(250);
        wave.setBounds(waveArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        r.removeFromTop(12);
        modeArea = r.removeFromLeft(340); r.removeFromLeft(12);
        shapeArea = r.removeFromLeft(260); r.removeFromLeft(12);
        controlsArea = r;
        auto m = modeArea.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        mode.setBounds(m.removeFromTop(36)); m.removeFromTop(22);
        rateLabel = m.removeFromTop(14); m.removeFromTop(4);
        rate.setBounds(m.removeFromTop(36)); m.removeFromTop(10);
        statusArea = m;
        shape.setBounds(shapeArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        auto c = controlsArea.reduced(12, 12).withTrimmedTop(station::kModuleTitle - 8);
        amount.setBounds(c.removeFromLeft(c.getWidth() * 3 / 5)); release.setBounds(c);
    }
    void paintBody(juce::Graphics& g) override {
        module(g, waveArea, "Waveform", "DK-01");
        module(g, modeArea, "Trigger", "DK-02");
        module(g, shapeArea, "One pump", "DK-03");
        module(g, controlsArea, "Duck", "DK-04");
        const bool kick = juce::roundToInt(dp.p("mode")) == 1;
        station::drawCaps(g, kick ? "Rate (tempo mode only)" : "Rate", rateLabel.toFloat(), juce::Justification::centredLeft, 9.5f, kick ? faint : dim);
        // status: what Duck is doing, or what it needs from you
        juce::String status; bool warn = false;
        if (!kick) {
            if (dp.uiPlaying.load()) status = "Pumping in time with your song";
            else { status = "Press play: Duck pumps in time with your song"; warn = true; }
        } else if (!dp.uiSidechainBus.load()) { status = "This host gave Duck no sidechain input"; warn = true; }
        else if (dp.uiHitAge.load() > 3.0f) {
            status = dp.uiSidechainLevel.load() > 0.0005f ? "Listening for a kick..." : "No kick coming in. Send your kick to Duck's sidechain";
            warn = true;
        } else status = "Ducking to your kick";
        auto s = statusArea.toFloat();
        g.setColour(warn ? juce::Colour(0xffe0832b) : accent); g.fillEllipse(s.getX(), s.getY() + 6, 7, 7);
        g.setFont(station::sans(12.5f)); g.setColour(warn ? ink : dim);
        g.drawFittedText(status, s.withTrimmedLeft(14).toNearestInt(), juce::Justification::topLeft, 2);
    }
    void tick() override {
        wave.update(); shape.repaint();
        const bool kick = juce::roundToInt(dp.p("mode")) == 1;
        rate.setAlpha(kick ? 0.4f : 1.0f); rate.setEnabled(!kick);
        body.repaint(modeArea);
    }
private:
    DuckProcessor& dp; WaveView wave; ShapeView shape; Segments mode, rate; Dial amount, release;
    juce::Rectangle<int> waveArea, modeArea, shapeArea, controlsArea, rateLabel, statusArea;
};

juce::AudioProcessorEditor* DuckProcessor::createEditor() { return new DuckEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DuckProcessor(); }
