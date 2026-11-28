// LGN Strike: a synthesized drum machine, eight pads, 25 drum models, vintage and modern machine characters.
#include "../common/Suite.h"
#include "../dsp/Strike.h"
#include <deque>

namespace {
#include "Presets.inc"
}
namespace lgn {
#include "Sounds.inc"
}

namespace {
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

const juce::Colour kAccent{ 0xffe8a10c };   // amber
constexpr int kPads = StrikeDsp::kPads;
const int kDefaultNotes[kPads] = { 36, 38, 39, 42, 46, 45, 37, 49 };
const char* const kDefaultSounds[kPads] = { "808 Kick Classic", "808 Snare Classic", "808 Clap", "808 Hat Closed", "808 Open Hat", "808 Cowbell", "808 Rim", "808 Cymbal" };

const StrikeSound* findSound(const char* name) { for (auto& s : strikeSounds()) if (juce::String(s.name) == name) return &s; return nullptr; }
juce::String pid(int pad, const char* key) { return "p" + juce::String(pad + 1) + key; }

class StrikeProcessor : public Processor {
public:
    StrikeProcessor() : Processor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true), "LGNStrike", layout()) {
        setPresets(factoryPresets(), presetKeep());
    }
    static param::Layout layout() {
        using namespace param;
        Layout l;
        static std::deque<std::string> keep;
        auto K = [&](int pad, const char* key) { keep.push_back(("p" + std::to_string(pad + 1) + key)); return keep.back().c_str(); };
        auto N = [&](int pad, const char* name) { keep.push_back("Pad " + std::to_string(pad + 1) + " " + name); return keep.back().c_str(); };
        juce::StringArray models; for (int m = 0; m < (int)DrumModel::count; m++) models.add(drumModel(m).name);
        for (int i = 0; i < kPads; i++) {
            const auto* snd = findSound(kDefaultSounds[i]);
            auto def = [&](const char* key, float fallback) { if (!snd) return fallback; for (int k = 0; k < 15; k++) if (juce::String(strikeSoundKeys()[k]) == key) return snd->v[k]; return fallback; };
            addChoice(l, K(i, "model"), N(i, "Model"), models, (int)def("model", 0));
            for (auto& pk : padKeys()) {
                const auto range = pk.skewMid > 0 ? skewRange(pk.lo, pk.hi, pk.skewMid) : juce::NormalisableRange<float>(pk.lo, pk.hi, 0.01f);
                add(l, K(i, pk.key), N(i, pk.name), range, def(pk.key, pk.def), pk.unit, juce::String(pk.unit) == "dB" ? 1 : 0);
            }
            addChoice(l, K(i, "choke"), N(i, "Choke Group"), { "Off", "1", "2", "3" }, (i == 3 || i == 4) ? 1 : 0);
            add(l, K(i, "note"), N(i, "Note"), { 0, 127, 1 }, (float)kDefaultNotes[i], "", 0);
        }
        addChoice(l, "character", "Machine", fromC(strikeCharacters(), 8), 1);
        addPct(l, "amount", "Machine Amount", 30);
        addPct(l, "glue", "Glue", 20);
        addPct(l, "clip", "Clip", 20);
        addPct(l, "vmix", "Reverb", 100);
        addPct(l, "vsize", "Room Size", 35);
        addPct(l, "humanize", "Humanize", 15);
        add(l, "width", "Width", { 0, 200, 0.1f }, 100, "%", 0);
        addDb(l, "volume", "Volume", -24, 12, 0);
        addChoice(l, "melodic", "Melodic Pad", { "Off", "Pad 1", "Pad 2", "Pad 3", "Pad 4", "Pad 5", "Pad 6", "Pad 7", "Pad 8" }, 0);
        return l;
    }
    static juce::StringArray fromC(const char* const* n, int c) { juce::StringArray a; for (int i = 0; i < c; i++) a.add(n[i]); return a; }
    float pv(int pad, const char* key) const { return p(pid(pad, key).toRawUTF8()); }
    PadParams padParams(int i) const {
        PadParams pp;
        padSet(pp, "model", pv(i, "model"));
        for (auto& pk : padKeys()) padSet(pp, pk.key, pv(i, pk.key));
        pp.choke = juce::roundToInt(pv(i, "choke")); pp.note = juce::roundToInt(pv(i, "note"));
        return pp;
    }
    bool isBusesLayoutSupported(const BusesLayout& b) const override {
        const auto out = b.getMainOutputChannelSet();
        return b.inputBuses.isEmpty() && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
    }
    bool acceptsMidi() const override { return true; }
    void prepareToPlay(double sr, int) override { dsp.prepare(sr); sync(); }
    void reset() override { dsp.allOff(); }
    void sync() {
        for (int i = 0; i < kPads; i++) dsp.setPad(i, padParams(i));
        StrikeGlobal g; g.character = juce::roundToInt(p("character")); g.amount = p("amount") / 100.0; g.glue = p("glue") / 100.0; g.clip = p("clip") / 100.0;
        g.verbMix = p("vmix") / 100.0; g.verbSize = p("vsize") / 100.0; g.humanize = p("humanize") / 100.0; g.width = p("width") / 100.0; g.volumeDb = p("volume");
        g.melodicPad = juce::roundToInt(p("melodic")) - 1;
        dsp.setGlobal(g);
    }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples();
        buf.clear();
        sync();
        for (const auto m : midi) {
            const auto msg = m.getMessage();
            if (msg.isNoteOn()) dsp.noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), juce::jlimit(0, n - 1, m.samplePosition));
            else if (msg.isAllNotesOff() || msg.isAllSoundOff()) dsp.allOff();
        }
        // pads clicked in the editor
        const int a = audition.exchange(0);
        for (int i = 0; i < kPads; i++) if (a & (1 << i)) dsp.trigger(i, 0.85);
        float* L = buf.getWritePointer(0);
        if (buf.getNumChannels() > 1) dsp.process(L, buf.getWritePointer(1), n);
        else { scratch.resize((size_t)n); dsp.process(L, scratch.data(), n); for (int i = 0; i < n; i++) L[i] = 0.5f * (L[i] + scratch[(size_t)i]); }
        for (int i = 0; i < kPads; i++) if (dsp.flash[(size_t)i] > 0) { lights[(size_t)i] = 1.0f; dsp.flash[(size_t)i] = 0; }
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 4.0; }
    juce::AudioProcessorEditor* createEditor() override;

    // set a pad to a library sound (as one undoable gesture per control)
    void loadSound(int pad, const StrikeSound& s) {
        for (int k = 0; k < 15; k++) {
            if (auto* q = apvts.getParameter(pid(pad, strikeSoundKeys()[k]))) { q->beginChangeGesture(); q->setValueNotifyingHost(q->convertTo0to1(s.v[k])); q->endChangeGesture(); }
        }
    }
    // the library sound this pad currently matches (level, pan, send and velocity may differ), or nullptr
    const StrikeSound* matchSound(int pad) const {
        for (auto& s : strikeSounds()) {
            bool ok = true;
            for (int k = 0; k < 15 && ok; k++) {
                const juce::String key = strikeSoundKeys()[k];
                if (key == "level" || key == "pan" || key == "send" || key == "vel") continue;
                ok = std::abs(pv(pad, key.toRawUTF8()) - s.v[k]) < 0.02f * (1 + std::abs(s.v[k]));
            }
            if (ok) return &s;
        }
        return nullptr;
    }
    std::atomic<int> audition{ 0 };
    std::array<std::atomic<float>, kPads> lights{};
    int selected = 0;
private:
    StrikeDsp dsp; std::vector<float> scratch;
};

// ---------------------------------------------------------------- the eight pads
class PadStrip : public juce::Component {
public:
    PadStrip(StrikeProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    std::function<void(int)> onSelect;
    void paint(juce::Graphics& g) override {
        for (int i = 0; i < kPads; i++) {
            auto r = cell(i);
            const float lit = shown[(size_t)i];
            const bool sel = i == proc.selected;
            station::drawShadow(g, r, 10, sel ? 1.0f : 0.6f);
            g.setColour(juce::Colours::white); g.fillRoundedRectangle(r, 10);
            if (lit > 0.01f) { g.setColour(accent.withAlpha(0.55f * lit)); g.fillRoundedRectangle(r, 10); }
            g.setColour(sel ? accent : hairline); g.drawRoundedRectangle(r.reduced(0.5f), 10, sel ? 2.0f : 1.0f);
            auto t = r.reduced(12, 10);
            station::drawCaps(g, "Pad " + juce::String(i + 1), t.removeFromTop(12), juce::Justification::centredLeft, 8.5f, sel ? accent.darker(0.3f) : dim);
            const int note = juce::roundToInt(proc.pv(i, "note"));
            g.setFont(station::mono(9.0f)); g.setColour(dim); g.drawText(juce::MidiMessage::getMidiNoteName(note, true, true, 3), t.withHeight(12).translated(0, -12), juce::Justification::centredRight);
            t.removeFromTop(6);
            const auto* s = proc.matchSound(i);
            g.setFont(station::sans(13.5f, station::medium)); g.setColour(ink);
            g.drawFittedText(s ? juce::String(s->name) : juce::String("Custom"), t.removeFromTop(36).toNearestInt(), juce::Justification::topLeft, 2);
            g.setFont(station::mono(9.5f)); g.setColour(dim);
            g.drawText(drumModel(juce::roundToInt(proc.pv(i, "model"))).name, t.removeFromBottom(14), juce::Justification::bottomLeft);
            const int ch = juce::roundToInt(proc.pv(i, "choke"));
            if (ch > 0) { g.setColour(accent.darker(0.2f)); g.drawText("choke " + juce::String(ch), t.removeFromBottom(14), juce::Justification::bottomRight); }
        }
    }
    void tick() {
        bool dirty = false;
        for (int i = 0; i < kPads; i++) {
            float& v = shown[(size_t)i]; const float in = proc.lights[(size_t)i].exchange(0.0f);
            const float nv = in > 0 ? 1.0f : v * 0.8f;
            if (std::abs(nv - v) > 0.003f) { v = nv; dirty = true; } else if (v > 0 && nv < 0.01f) { v = 0; dirty = true; }
        }
        if (dirty || ++slow % 8 == 0) repaint();
    }
    void mouseDown(const juce::MouseEvent& e) override {
        for (int i = 0; i < kPads; i++) if (cell(i).contains(e.position)) {
            proc.audition.fetch_or(1 << i);
            if (proc.selected != i) { proc.selected = i; if (onSelect) onSelect(i); }
            repaint(); return;
        }
    }
private:
    juce::Rectangle<float> cell(int i) const { const float w = getWidth() / (float)kPads; return { i * w + 5, 4, w - 10, getHeight() - 8.0f }; }
    StrikeProcessor& proc; juce::Colour accent; std::array<float, kPads> shown{}; int slow = 0;
};

// ---------------------------------------------------------------- the selected pad's waveform
class PadView : public juce::Component {
public:
    PadView(StrikeProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    void tick() {
        const auto pp = proc.padParams(proc.selected);
        const juce::String key = juce::String(proc.selected) + "|" + juce::String(pp.model) + "|" + juce::String(pp.tune) + juce::String(pp.decay) + juce::String(pp.pitch) + juce::String(pp.pdecay) + juce::String(pp.tone)
                               + juce::String(pp.snap) + juce::String(pp.ndecay) + juce::String(pp.color) + juce::String(pp.drive) + juce::String(pp.click) + juce::String(pp.level);
        if (key == lastKey) return;
        lastKey = key;
        auto q = pp; q.send = 0;
        len = juce::jlimit(0.08, 3.0, (std::max(pp.decay, pp.ndecay) * 1.15 + 20) / 1000.0);
        wave.resize((size_t)(48000 * len)); StrikeDsp::preview(q, 48000, wave.data(), (int)wave.size());
        repaint();
    }
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat(); station::drawWell(g, r, 10);
        auto a = r.reduced(14, 12);
        station::drawCaps(g, "Pad " + juce::String(proc.selected + 1) + " - first " + juce::String(juce::roundToInt(len * 1000)) + " ms", a.removeFromTop(12), juce::Justification::centredLeft, 8.5f, dim);
        a.removeFromTop(6);
        g.setColour(hairline); g.drawHorizontalLine((int)a.getCentreY(), a.getX(), a.getRight());
        if (wave.empty()) return;
        const int W = (int)a.getWidth(); const size_t N = wave.size();
        juce::Path top, bot;
        for (int x = 0; x < W; x++) {
            const size_t s0 = N * (size_t)x / (size_t)W, s1 = std::max(s0 + 1, N * (size_t)(x + 1) / (size_t)W);
            float lo = 0, hi = 0; for (size_t s = s0; s < s1 && s < N; s++) { lo = std::min(lo, wave[s]); hi = std::max(hi, wave[s]); }
            const float px = a.getX() + (float)x;
            const float yh = a.getCentreY() - juce::jlimit(-1.0f, 1.0f, hi) * a.getHeight() * 0.48f, yl = a.getCentreY() - juce::jlimit(-1.0f, 1.0f, lo) * a.getHeight() * 0.48f;
            if (x == 0) { top.startNewSubPath(px, yh); bot.startNewSubPath(px, yl); } else { top.lineTo(px, yh); bot.lineTo(px, yl); }
        }
        g.setColour(accent.withAlpha(0.25f));
        for (int x = 0; x < W; x += 1) {
            const size_t s0 = N * (size_t)x / (size_t)W, s1 = std::max(s0 + 1, N * (size_t)(x + 1) / (size_t)W);
            float lo = 0, hi = 0; for (size_t s = s0; s < s1 && s < N; s++) { lo = std::min(lo, wave[s]); hi = std::max(hi, wave[s]); }
            g.drawVerticalLine((int)(a.getX() + x), a.getCentreY() - juce::jlimit(-1.0f, 1.0f, hi) * a.getHeight() * 0.48f, a.getCentreY() - juce::jlimit(-1.0f, 1.0f, lo) * a.getHeight() * 0.48f + 1);
        }
        g.setColour(accent.darker(0.2f)); g.strokePath(top, juce::PathStrokeType(1.2f)); g.strokePath(bot, juce::PathStrokeType(1.2f));
    }
private:
    StrikeProcessor& proc; juce::Colour accent; std::vector<float> wave; double len = 0.5; juce::String lastKey;
};

// ---------------------------------------------------------------- sound browser for the selected pad
class SoundBrowser : public juce::Component, private juce::Timer {
public:
    SoundBrowser(StrikeProcessor& p) : proc(p) {
        for (auto& s : strikeSounds()) if (!cats.contains(s.category)) cats.add(s.category);
        int id = 1; for (auto& c : cats) { int cnt = 0; for (auto& s : strikeSounds()) cnt += juce::String(s.category) == c; category.addItem(c + "  (" + juce::String(cnt) + ")", id++); }
        category.onChange = [this] { if (updating) return; const int ci = category.getSelectedItemIndex(); for (auto& s : strikeSounds()) if (juce::String(s.category) == cats[ci]) { load(&s); break; } };
        prev.onClick = [this] { step(-1); }; next.onClick = [this] { step(1); };
        dice.onClick = [this] {
            const auto cat = currentCategory(); std::vector<const StrikeSound*> in; for (auto& s : strikeSounds()) if (juce::String(s.category) == cat) in.push_back(&s);
            if (!in.empty()) load(in[(size_t)juce::Random::getSystemRandom().nextInt((int)in.size())]);
        };
        name.setFont(station::sans(15.0f, station::medium)); name.setColour(juce::Label::textColourId, ink);
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &category, &name, &prev, &next, &dice }) addAndMakeVisible(c);
        startTimerHz(10);
    }
    void resized() override {
        auto r = getLocalBounds(); category.setBounds(r.removeFromLeft(180).withSizeKeepingCentre(180, 30)); r.removeFromLeft(10);
        dice.setBounds(r.removeFromRight(52).withSizeKeepingCentre(52, 30)); r.removeFromRight(6);
        next.setBounds(r.removeFromRight(32).withSizeKeepingCentre(32, 30)); r.removeFromRight(4); prev.setBounds(r.removeFromRight(32).withSizeKeepingCentre(32, 30)); r.removeFromRight(8);
        name.setBounds(r);
    }
private:
    juce::String currentCategory() const {
        if (auto* s = proc.matchSound(proc.selected)) return s->category;
        const int ci = category.getSelectedItemIndex(); return ci >= 0 ? cats[ci] : cats[0];
    }
    void step(int d) {
        const auto cat = currentCategory(); std::vector<const StrikeSound*> in; for (auto& s : strikeSounds()) if (juce::String(s.category) == cat) in.push_back(&s);
        if (in.empty()) return;
        int at = -1; const auto* cur = proc.matchSound(proc.selected); for (int i = 0; i < (int)in.size(); i++) if (in[(size_t)i] == cur) at = i;
        at = at < 0 ? 0 : (at + d + (int)in.size()) % (int)in.size();
        load(in[(size_t)at]);
    }
    void load(const StrikeSound* s) { if (!s) return; proc.loadSound(proc.selected, *s); proc.audition.fetch_or(1 << proc.selected); timerCallback(); }
    void timerCallback() override {
        const auto* s = proc.matchSound(proc.selected);
        name.setText(s ? juce::String(s->name) : "Custom " + juce::String(drumModel(juce::roundToInt(proc.pv(proc.selected, "model"))).name), juce::dontSendNotification);
        if (s) { updating = true; category.setSelectedItemIndex(cats.indexOf(juce::String(s->category)), juce::dontSendNotification); updating = false; }
    }
    StrikeProcessor& proc; juce::StringArray cats; juce::ComboBox category; juce::Label name; juce::TextButton prev{ "<" }, next{ ">" }, dice{ "Dice" }; bool updating = false;
};

class StrikeEditor : public Editor {
public:
    explicit StrikeEditor(StrikeProcessor& p) : Editor(p, "Strike", "Drum synthesizer", kAccent, 1240, 800), sp(p), pads(p, accent), view(p, accent), browser(p),
        character(p.apvts, "character"), amount(p.apvts, "amount", "Amount"), glue(p.apvts, "glue", "Glue"), clip(p.apvts, "clip", "Clip"), vmix(p.apvts, "vmix", "Reverb"),
        vsize(p.apvts, "vsize", "Room"), human(p.apvts, "humanize", "Humanize"), width(p.apvts, "width", "Width"), volume(p.apvts, "volume", "Volume"), melodic(p.apvts, "melodic", "Melodic pad") {
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &pads, &view, &browser, &character, &amount, &glue, &clip, &vmix, &vsize, &human, &width, &volume, &melodic }) body.addAndMakeVisible(c);
        pads.onSelect = [this](int) { buildPad(); resized(); body.repaint(); };
        buildPad();
        setReady();
    }
protected:
    void buildPad() {
        const int i = sp.selected; auto& s = sp.apvts;
        padDials.clear(); model.reset(); choke.reset(); note.reset();
        static std::deque<std::string> keep;
        auto K = [&](const char* key) { keep.push_back(pid(i, key).toStdString()); return keep.back().c_str(); };
        model = std::make_unique<Choice>(s, K("model"), "Model");
        choke = std::make_unique<Segments>(s, K("choke"), juce::StringArray{ "-", "1", "2", "3" });
        for (auto& pk : padKeys()) padDials.push_back(std::make_unique<Dial>(s, K(pk.key), juce::String(pk.name).replace("Reverb Send", "Send").replace("Pitch Sweep", "Sweep").replace("Sweep Time", "Sweep time").replace("Snap Decay", "Snap decay"), true));
        note = std::make_unique<Dial>(s, K("note"), "Note", true);
        for (auto& d : padDials) body.addAndMakeVisible(*d);
        body.addAndMakeVisible(*model); body.addAndMakeVisible(*choke); body.addAndMakeVisible(*note);
        lastModel = -1;
    }
    void layout(juce::Rectangle<int> r) override {
        padArea = r.removeFromTop(124); pads.setBounds(padArea);
        r.removeFromTop(10);
        auto bottom = r.removeFromBottom(158); r.removeFromTop(0);
        machineArea = bottom.withTrimmedTop(10);
        editArea = r.removeFromLeft(r.getWidth() * 62 / 100); r.removeFromLeft(10); viewArea = r;
        {   auto a = editArea.reduced(14, 10).withTrimmedTop(station::kModuleTitle - 10);
            auto top = a.removeFromTop(48);
            if (model) model->setBounds(top.removeFromLeft(170)); top.removeFromLeft(12);
            browser.setBounds(top.withTrimmedTop(14));
            a.removeFromTop(4);
            const int h = juce::jmin(130, a.getHeight() / 2);
            a.removeFromTop((a.getHeight() - 2 * h) / 3);
            auto row1 = a.removeFromTop(h); a.removeFromTop((a.getHeight() - h) / 2); auto row2 = a.removeFromTop(h);
            const int w = row1.getWidth() / 8;
            for (int k = 0; k < 7 && k < (int)padDials.size(); k++) padDials[(size_t)k]->setBounds(row1.removeFromLeft(w));
            if (note) note->setBounds(row1.removeFromLeft(w));
            for (int k = 7; k < (int)padDials.size(); k++) padDials[(size_t)k]->setBounds(row2.removeFromLeft(w));
            chokeLabel = row2.removeFromTop(row2.getHeight() / 2).withTrimmedTop(14).removeFromTop(14);
            if (choke) choke->setBounds(row2.withSizeKeepingCentre(row2.getWidth() - 8, 30).translated(0, -18));
        }
        view.setBounds(viewArea.reduced(14, 10).withTrimmedTop(station::kModuleTitle - 10));
        {   auto a = machineArea.reduced(14, 10).withTrimmedTop(station::kModuleTitle - 10);
            auto left = a.removeFromLeft(560); charLabel = left.removeFromTop(14); character.setBounds(left.removeFromTop(30)); left.removeFromTop(10);
            melodic.setBounds(left.removeFromTop(48).removeFromLeft(170));
            a.removeFromLeft(16); const int w = a.getWidth() / 8;
            for (auto* d : { &amount, &glue, &clip, &vmix, &vsize, &human, &width, &volume }) d->setBounds(a.removeFromLeft(w));
        }
    }
    void paintBody(juce::Graphics& g) override {
        const auto& m = drumModel(juce::roundToInt(sp.pv(sp.selected, "model")));
        module(g, editArea, "Pad " + juce::String(sp.selected + 1) + "  " + m.name, "ST-01");
        module(g, viewArea, "Sound", "ST-02");
        module(g, machineArea, "Machine", "ST-03");
        station::drawCaps(g, "Machine character", charLabel.toFloat(), juce::Justification::centredLeft, 8.5f, dim);
        station::drawCaps(g, "Choke group", chokeLabel.toFloat(), juce::Justification::centredLeft, 8.5f, dim);
        station::drawCaps(g, "Sound", browser.getBounds().translated(0, -14).withHeight(12).toFloat(), juce::Justification::centredLeft, 8.5f, dim);
    }
    void tick() override {
        pads.tick(); view.tick();
        // the Colour dial's name follows the model (Saturation, Spread, Noise, Ratio ...)
        const int m = juce::roundToInt(sp.pv(sp.selected, "model"));
        if (m != lastModel) {
            lastModel = m;
            for (size_t k = 0; k < padKeys().size(); k++) if (juce::String(padKeys()[k].key) == "color") { padDials[k]->label = drumModel(m).colour; padDials[k]->repaint(); }
            body.repaint(editArea);
        }
        const int c = juce::roundToInt(sp.p("character")); amount.setAlpha(c == 0 ? 0.4f : 1.0f);
    }
private:
    StrikeProcessor& sp; PadStrip pads; PadView view; SoundBrowser browser;
    Segments character; Dial amount, glue, clip, vmix, vsize, human, width, volume; Choice melodic;
    std::unique_ptr<Choice> model; std::unique_ptr<Segments> choke; std::unique_ptr<Dial> note; std::vector<std::unique_ptr<Dial>> padDials;
    juce::Rectangle<int> padArea, editArea, viewArea, machineArea, charLabel, chokeLabel; int lastModel = -1;
};

juce::AudioProcessorEditor* StrikeProcessor::createEditor() { return new StrikeEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new StrikeProcessor(); }
