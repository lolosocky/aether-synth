// LGN Flipside: tempo-synced time and volume curves (the Gross Beat idea), with a library of well over a thousand
// presets, an editable curve display and MIDI-triggered curves.
#include "../common/Suite.h"
#include "../dsp/Flipside.h"
#include <set>

namespace {
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

const juce::Colour kAccent{ 0xffe0457b };    // flip pink-red
const juce::Colour kVolColour{ 0xff2a2e35 }; // graphite for the volume lane
const juce::StringArray kRates{ "Half", "Normal", "Double" };
constexpr double kRateMul[3] = { 0.5, 1.0, 2.0 };

juce::StringArray names(const std::vector<FlipEntry>& v) { juce::StringArray a; for (auto& e : v) a.add(e.name); return a; }
std::vector<Processor::Preset> comboPresets() {
    std::vector<Processor::Preset> out;
    for (auto& c : FlipBank::get().combos) {
        Processor::Preset pr{ c.category, c.name, { { "time", (float)c.time }, { "vol", (float)c.vol } } };
        // volume-only presets switch the time curve off so the audio stays exactly on the grid
        if (c.category.rfind("Gates Only", 0) == 0 || c.name == "Off (Clean)") pr.values.push_back({ "timeon", 0.0f });
        out.push_back(pr);
    }
    return out;
}

class FlipsideProcessor : public Processor {
public:
    FlipsideProcessor() : Processor("LGNFlipside", layout()) { setPresets(comboPresets(), { "trigger" }); }
    static param::Layout layout() {
        const auto& b = FlipBank::get();
        param::Layout l;
        param::addChoice(l, "time", "Time Curve", names(b.time), 0);
        param::addChoice(l, "vol", "Volume Curve", names(b.vol), b.findVol("Full"));
        param::addBool(l, "timeon", "Time On", true);
        param::addBool(l, "volon", "Volume On", true);
        param::addPct(l, "timemix", "Time Mix", 100);
        param::addPct(l, "volamt", "Volume Depth", 100);
        param::addMs(l, "smooth", "Smooth", 0.1f, 20, 2, 3);
        param::addChoice(l, "rate", "Rate", kRates, 1);
        param::addChoice(l, "trigger", "Trigger", { "Always", "MIDI Notes" }, 0);
        param::addPct(l, "mix", "Mix", 100);
        param::addDb(l, "out", "Output", -12, 12, 0);
        return l;
    }
    bool acceptsMidi() const override { return true; }
    void prepareToPlay(double sr, int) override { FlipParams fp; fp.outDb = p("out"); dsp.set(fp); dsp.prepare(sr); heldTime = heldVol = -1; fallbackPpq = 0; }
    void reset() override { dsp.reset(); }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples(), ch = juce::jmin(2, getTotalNumInputChannels());
        for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); c++) buf.clear(c, 0, n);
        const auto& bank = FlipBank::get();
        // MIDI: C2-B4 pick a time curve from the current time category, C5-B7 a volume curve from the volume category
        const bool midiMode = juce::roundToInt(p("trigger")) == 1;
        if (!midiMode) { heldNotes.clear(); heldTime = heldVol = -1; }
        else for (const auto m : midi) {
            const auto msg = m.getMessage();
            if (msg.isNoteOn()) {
                const int note = msg.getNoteNumber();
                if (note >= 36 && note < 72) heldTime = categoryStart(bank.time, (int)p("time")) + (note - 36);
                else if (note >= 72 && note < 108) heldVol = categoryStart(bank.vol, (int)p("vol")) + (note - 72);
                heldNotes.insert(note);
            } else if (msg.isNoteOff()) {
                heldNotes.erase(msg.getNoteNumber());
                const int note = msg.getNoteNumber();
                if (note >= 36 && note < 72 && !anyHeld(36, 72)) heldTime = -1;
                if (note >= 72 && note < 108 && !anyHeld(72, 108)) heldVol = -1;
            } else if (msg.isAllNotesOff()) { heldNotes.clear(); heldTime = heldVol = -1; }
        }
        double bpmNow = 120, ppq = fallbackPpq; bool playing = false;
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) {
            if (auto bb = pos->getBpm()) bpmNow = juce::jlimit(20.0, 999.0, *bb);
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }
        fallbackPpq = ppq + n * bpmNow / 60.0 / getSampleRate();
        meterInput(buf, n);
        int ti = juce::jlimit(0, (int)bank.time.size() - 1, juce::roundToInt(p("time")));
        int vi = juce::jlimit(0, (int)bank.vol.size() - 1, juce::roundToInt(p("vol")));
        if (heldTime >= 0) ti = juce::jlimit(0, (int)bank.time.size() - 1, heldTime);
        if (heldVol >= 0) vi = juce::jlimit(0, (int)bank.vol.size() - 1, heldVol);
        FlipParams fp; fp.timeMix = p("timemix") / 100.0; fp.volAmount = p("volamt") / 100.0; fp.mix = p("mix") / 100.0; fp.smoothMs = p("smooth");
        fp.rate = kRateMul[juce::jlimit(0, 2, juce::roundToInt(p("rate")))]; fp.outDb = p("out");
        fp.timeOn = p("timeon") > 0.5f && (!midiMode || heldTime >= 0);
        fp.volOn = p("volon") > 0.5f && (!midiMode || heldVol >= 0);
        {   // the curves: an edited one if the user has drawn on it, otherwise the library's
            const juce::SpinLock::ScopedTryLockType lock(editLock);
            if (lock.isLocked()) {
                curT = (editedTime && ti == editTimeIdx && heldTime < 0) ? editT : bank.time[(size_t)ti].curve;
                curV = (editedVol && vi == editVolIdx && heldVol < 0) ? editV : bank.vol[(size_t)vi].curve;
            }
        }
        dsp.set(fp); dsp.setCurves(curT, curV);
        float* ptrs[2] = { buf.getWritePointer(0), ch > 1 ? buf.getWritePointer(1) : nullptr };
        dsp.process(ptrs, ch, n, ppq, bpmNow, playing);
        uiPpq = ppq + n * bpmNow / 60.0 / getSampleRate(); uiPlaying = playing; uiTime = ti; uiVol = vi; uiGain = (float)dsp.lastGain; uiDelay = (float)dsp.lastDelayBeats;
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 0.0; }
    juce::AudioProcessorEditor* createEditor() override;

    // edits drawn in the display (kept with the project)
    void setEdit(bool timeCurve, const FlipCurve& c, int idx) {
        const juce::SpinLock::ScopedLockType lock(editLock);
        if (timeCurve) { editT = c; editedTime = true; editTimeIdx = idx; } else { editV = c; editedVol = true; editVolIdx = idx; }
    }
    void clearEdits() { const juce::SpinLock::ScopedLockType lock(editLock); editedTime = editedVol = false; }
    FlipCurve shownCurve(bool timeCurve) {
        const juce::SpinLock::ScopedLockType lock(editLock);
        const auto& bank = FlipBank::get();
        if (timeCurve) return (editedTime && uiTime.load() == editTimeIdx) ? editT : bank.time[(size_t)juce::jlimit(0, (int)bank.time.size() - 1, uiTime.load())].curve;
        return (editedVol && uiVol.load() == editVolIdx) ? editV : bank.vol[(size_t)juce::jlimit(0, (int)bank.vol.size() - 1, uiVol.load())].curve;
    }
    bool isEdited(bool timeCurve) const { return timeCurve ? editedTime && uiTime.load() == editTimeIdx : editedVol && uiVol.load() == editVolIdx; }
    static int categoryStart(const std::vector<FlipEntry>& v, int idx) {
        idx = juce::jlimit(0, (int)v.size() - 1, idx); const auto& cat = v[(size_t)idx].category;
        while (idx > 0 && v[(size_t)idx - 1].category == cat) idx--;
        return idx;
    }

    std::atomic<double> uiPpq{ 0 }; std::atomic<bool> uiPlaying{ false }; std::atomic<int> uiTime{ 0 }, uiVol{ 0 }; std::atomic<float> uiGain{ 1 }, uiDelay{ 0 };

    void getStateInformation(juce::MemoryBlock& dest) override {
        {   const juce::SpinLock::ScopedLockType lock(editLock);
            apvts.state.setProperty("editT", editedTime ? curveText(editT) + "@" + juce::String(editTimeIdx) : juce::String(), nullptr);
            apvts.state.setProperty("editV", editedVol ? curveText(editV) + "@" + juce::String(editVolIdx) : juce::String(), nullptr); }
        Processor::getStateInformation(dest);
    }
    void setStateInformation(const void* data, int size) override {
        Processor::setStateInformation(data, size);
        auto parse = [](const juce::String& s, FlipCurve& c, int& idx) {
            if (s.isEmpty()) return false;
            idx = s.fromLastOccurrenceOf("@", false, false).getIntValue();
            auto t = juce::StringArray::fromTokens(s.upToLastOccurrenceOf("@", false, false), " ", "");
            c.clear(); if (t.size() < 3) return false; c.lengthBeats = t[0].getFloatValue();
            for (int i = 1; i + 1 < t.size(); i += 2) c.add(t[i].getFloatValue(), t[i + 1].getFloatValue());
            return c.n >= 2;
        };
        const juce::SpinLock::ScopedLockType lock(editLock);
        editedTime = parse(apvts.state.getProperty("editT").toString(), editT, editTimeIdx);
        editedVol = parse(apvts.state.getProperty("editV").toString(), editV, editVolIdx);
    }
private:
    static juce::String curveText(const FlipCurve& c) { juce::String s(c.lengthBeats); for (int i = 0; i < c.n; i++) s << " " << c.x[(size_t)i] << " " << c.y[(size_t)i]; return s; }
    bool anyHeld(int lo, int hi) const { for (int n : heldNotes) if (n >= lo && n < hi) return true; return false; }
    FlipsideDsp dsp; FlipCurve curT, curV;
    juce::SpinLock editLock; FlipCurve editT, editV; bool editedTime = false, editedVol = false; int editTimeIdx = -1, editVolIdx = -1;
    std::set<int> heldNotes; int heldTime = -1, heldVol = -1; double fallbackPpq = 0;
};

// ---------------------------------------------------------------- curve display / editor
// Top lane: the time curve (how far back it plays from, in beats; up = further back). Bottom lane: the volume curve.
// Drag a point to move it, double-click empty space to add one, double-click a point to remove it.
class CurveView : public juce::Component {
public:
    CurveView(FlipsideProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    void tick() { repaint(); }
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        auto a = r.reduced(14, 10);
        lanes[0] = a.removeFromTop(a.getHeight() * 0.56f).withTrimmedTop(14).withTrimmedBottom(6);
        lanes[1] = a.withTrimmedTop(14).withTrimmedBottom(12);
        for (int k = 0; k < 2; k++) drawLane(g, k == 0);
    }
    void mouseDown(const juce::MouseEvent& e) override {
        drag = -1; dragLane = laneAt(e.position);
        if (dragLane < 0) return;
        work = proc.shownCurve(dragLane == 0);
        drag = nearest(work, dragLane, e.position);
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (drag < 0 || dragLane < 0) return;
        const auto lane = lanes[(size_t)dragLane];
        const float lo = drag > 0 ? work.x[(size_t)drag - 1] : 0.0f, hi = drag < work.n - 1 ? work.x[(size_t)drag + 1] : 1.0f;
        work.x[(size_t)drag] = juce::jlimit(lo, hi, (e.position.x - lane.getX()) / lane.getWidth());
        work.y[(size_t)drag] = (float)toValue(dragLane, lane, e.position.y);
        if (drag == 0) work.x[0] = 0; if (drag == work.n - 1) work.x[(size_t)work.n - 1] = 1;
        commit();
    }
    void mouseDoubleClick(const juce::MouseEvent& e) override {
        const int lane = laneAt(e.position); if (lane < 0) return;
        work = proc.shownCurve(lane == 0); dragLane = lane;
        const int hit = nearest(work, lane, e.position);
        if (hit > 0 && hit < work.n - 1) {   // remove
            for (int i = hit; i < work.n - 1; i++) { work.x[(size_t)i] = work.x[(size_t)i + 1]; work.y[(size_t)i] = work.y[(size_t)i + 1]; }
            work.n--;
        } else if (hit < 0 && work.n < FlipCurve::kMax) {   // add
            const auto L = lanes[(size_t)lane];
            const float x = juce::jlimit(0.0f, 1.0f, (e.position.x - L.getX()) / L.getWidth());
            int at = 0; while (at < work.n && work.x[(size_t)at] <= x) at++;
            for (int i = work.n; i > at; i--) { work.x[(size_t)i] = work.x[(size_t)i - 1]; work.y[(size_t)i] = work.y[(size_t)i - 1]; }
            work.x[(size_t)at] = x; work.y[(size_t)at] = (float)toValue(lane, L, e.position.y); work.n++;
        }
        commit();
    }
private:
    double timeTop() const { const auto c = const_cast<FlipsideProcessor&>(proc).shownCurve(true); float m = 0.5f; for (int i = 0; i < c.n; i++) m = juce::jmax(m, c.y[(size_t)i]); return std::ceil(m * 4.0) / 4.0; }
    double toValue(int lane, juce::Rectangle<float> L, float y) const {
        const double t = juce::jlimit(0.0, 1.0, (double)(L.getBottom() - y) / L.getHeight());
        return lane == 0 ? t * timeTopCache : t;
    }
    juce::Point<float> toPoint(int lane, juce::Rectangle<float> L, float x, float y) const {
        const double top = lane == 0 ? timeTopCache : 1.0;
        return { L.getX() + L.getWidth() * x, L.getBottom() - L.getHeight() * (float)(y / top) };
    }
    int laneAt(juce::Point<float> p) const { for (int k = 0; k < 2; k++) if (lanes[(size_t)k].expanded(0, 8).contains(p)) return k; return -1; }
    int nearest(const FlipCurve& c, int lane, juce::Point<float> p) const {
        int best = -1; float bd = 10;
        for (int i = 0; i < c.n; i++) { const float d = toPoint(lane, lanes[(size_t)lane], c.x[(size_t)i], c.y[(size_t)i]).getDistanceFrom(p); if (d < bd) { bd = d; best = i; } }
        return best;
    }
    void commit() { proc.setEdit(dragLane == 0, work, dragLane == 0 ? proc.uiTime.load() : proc.uiVol.load()); repaint(); }
    void drawLane(juce::Graphics& g, bool time) {
        const int lane = time ? 0 : 1; const auto L = lanes[(size_t)lane];
        const auto c = proc.shownCurve(time);
        if (time) timeTopCache = timeTop();
        const double beats = c.lengthBeats;
        // grid: beats, and bars stronger
        for (int k = 0; k <= (int)std::round(beats * 4); k++) {
            const float x = L.getX() + L.getWidth() * (float)(k / (beats * 4));
            g.setColour(k % 16 == 0 ? station::hairlineHi : k % 4 == 0 ? hairline : hairline.withAlpha(0.4f));
            g.drawVerticalLine((int)x, L.getY(), L.getBottom());
        }
        g.setColour(hairline); g.drawHorizontalLine((int)L.getBottom(), L.getX(), L.getRight());
        // the curve, filled
        juce::Path line, fill; fill.startNewSubPath(L.getX(), L.getBottom());
        for (int i = 0; i < c.n; i++) {
            const auto pt = toPoint(lane, L, c.x[(size_t)i], c.y[(size_t)i]);
            if (i == 0) line.startNewSubPath(pt); else line.lineTo(pt);
            fill.lineTo(pt);
        }
        fill.lineTo(L.getRight(), L.getBottom()); fill.closeSubPath();
        const auto col = time ? accent : kVolColour;
        g.setColour(col.withAlpha(0.12f)); g.fillPath(fill);
        g.setColour(col); g.strokePath(line, juce::PathStrokeType(1.8f));
        if (c.n <= 48) for (int i = 0; i < c.n; i++) { const auto pt = toPoint(lane, L, c.x[(size_t)i], c.y[(size_t)i]); g.setColour(juce::Colours::white); g.fillEllipse(pt.x - 3.5f, pt.y - 3.5f, 7, 7); g.setColour(col); g.drawEllipse(pt.x - 3.5f, pt.y - 3.5f, 7, 7, 1.4f); }
        // labels
        const auto& bank = FlipBank::get();
        const auto& e = time ? bank.time[(size_t)juce::jlimit(0, (int)bank.time.size() - 1, proc.uiTime.load())] : bank.vol[(size_t)juce::jlimit(0, (int)bank.vol.size() - 1, proc.uiVol.load())];
        station::drawCaps(g, time ? "Time" : "Volume", juce::Rectangle<float>(L.getX(), L.getY() - 14, 60, 12), juce::Justification::centredLeft, 9.0f, dim);
        g.setFont(station::mono(10.0f, true)); g.setColour(ink);
        g.drawText(juce::String(e.name) + (proc.isEdited(time) ? "  (edited)" : "") + "   " + juce::String(beats / 4.0, beats < 4 ? 2 : 0) + (beats == 4 ? " bar" : " bars"),
                   juce::Rectangle<float>(L.getX() + 64, L.getY() - 15, L.getWidth() - 64, 13), juce::Justification::centredLeft);
        if (time) { g.setFont(station::mono(9.0f)); g.setColour(faint); g.drawText(juce::String(timeTopCache, 2) + " beats back", juce::Rectangle<float>(L.getRight() - 120, L.getY() - 15, 120, 13), juce::Justification::right); }
        // playhead
        if (proc.uiPlaying.load()) {
            const double rate = kRateMul[juce::jlimit(0, 2, juce::roundToInt(proc.p("rate")))];
            double f = std::fmod(proc.uiPpq.load() * rate / beats, 1.0); if (f < 0) f += 1;
            const float x = L.getX() + L.getWidth() * (float)f;
            g.setColour(ink); g.fillRect(x - 0.75f, L.getY(), 1.5f, L.getHeight());
            int h = 0; const auto pt = toPoint(lane, L, (float)f, (float)c.eval(f, h));
            g.setColour(col); g.fillEllipse(pt.x - 5, pt.y - 5, 10, 10);
        }
    }
    FlipsideProcessor& proc; juce::Colour accent;
    std::array<juce::Rectangle<float>, 2> lanes; FlipCurve work; int drag = -1, dragLane = -1; double timeTopCache = 1;
};

// ---------------------------------------------------------------- browser: category, name, step, dice
class Browser : public juce::Component, private juce::Timer {
public:
    Browser(FlipsideProcessor& p, bool time, juce::Colour a) : proc(p), isTime(time), accent(a) {
        const auto& v = list();
        for (auto& e : v) if (!cats.contains(e.category)) cats.add(e.category);
        int id = 1; for (auto& c : cats) { int cnt = 0; for (auto& e : v) cnt += e.category == c; category.addItem(c + "  (" + juce::String(cnt) + ")", id++); }
        category.onChange = [this] {
            const int ci = category.getSelectedItemIndex(); if (ci < 0 || updating) return;
            const auto& v2 = list(); for (int i = 0; i < (int)v2.size(); i++) if (v2[(size_t)i].category == cats[ci]) { set(i); break; }
        };
        for (auto* b : { &prev, &next, &dice }) addAndMakeVisible(b);
        prev.onClick = [this] { set(idx() - 1); }; next.onClick = [this] { set(idx() + 1); };
        dice.onClick = [this] {   // random within the category
            const auto& v2 = list(); const auto& c = v2[(size_t)idx()].category; std::vector<int> in;
            for (int i = 0; i < (int)v2.size(); i++) if (v2[(size_t)i].category == c) in.push_back(i);
            set(in[(size_t)juce::Random::getSystemRandom().nextInt((int)in.size())]);
        };
        dice.setTooltip("Random from this category"); prev.setTooltip("Previous"); next.setTooltip("Next");
        name.setFont(station::sans(15.0f, station::medium)); name.setColour(juce::Label::textColourId, ink);
        addAndMakeVisible(category); addAndMakeVisible(name);
        startTimerHz(15); timerCallback();
    }
    void resized() override {
        auto r = getLocalBounds();
        category.setBounds(r.removeFromTop(32)); r.removeFromTop(8);
        auto row = r.removeFromTop(34);
        dice.setBounds(row.removeFromRight(44)); row.removeFromRight(6);
        next.setBounds(row.removeFromRight(34)); row.removeFromRight(4); prev.setBounds(row.removeFromRight(34)); row.removeFromRight(8);
        name.setBounds(row);
    }
private:
    const std::vector<FlipEntry>& list() const { return isTime ? FlipBank::get().time : FlipBank::get().vol; }
    const char* id() const { return isTime ? "time" : "vol"; }
    int idx() const { return juce::jlimit(0, (int)list().size() - 1, juce::roundToInt(proc.p(id()))); }
    void set(int i) {
        const int n = (int)list().size(); i = (i % n + n) % n;
        auto* prm = proc.apvts.getParameter(id()); prm->beginChangeGesture(); prm->setValueNotifyingHost(prm->convertTo0to1((float)i)); prm->endChangeGesture();
    }
    void timerCallback() override {
        const int i = idx(); if (i == shown) return; shown = i;
        const auto& e = list()[(size_t)i];
        updating = true; category.setSelectedItemIndex(cats.indexOf(juce::String(e.category)), juce::dontSendNotification); updating = false;
        int pos = 0, tot = 0; for (int k = 0; k < (int)list().size(); k++) if (list()[(size_t)k].category == e.category) { tot++; if (k <= i) pos++; }
        name.setText(juce::String(e.name), juce::dontSendNotification);
        name.setTooltip(juce::String(pos) + " of " + juce::String(tot) + " in " + e.category);
    }
    FlipsideProcessor& proc; bool isTime; juce::Colour accent; juce::StringArray cats;
    juce::ComboBox category; juce::Label name; juce::TextButton prev{ "<" }, next{ ">" }, dice{ "Dice" };
    int shown = -1; bool updating = false;
};

class FlipsideEditor : public Editor {
public:
    explicit FlipsideEditor(FlipsideProcessor& p)
        : Editor(p, "Flipside", "Time + volume curves", kAccent, 1040, 765), fp(p), curves(p, accent), timeB(p, true, accent), volB(p, false, accent),
          timeOn(p.apvts, "timeon", "Time"), volOn(p.apvts, "volon", "Volume"), timeMix(p.apvts, "timemix", "Time mix"), volAmt(p.apvts, "volamt", "Depth"),
          rate(p.apvts, "rate"), trigger(p.apvts, "trigger"), smooth(p.apvts, "smooth", "Smooth"), mix(p.apvts, "mix", "Mix"), out(p.apvts, "out", "Output") {
        reset.setButtonText("Reset edits"); reset.onClick = [this] { fp.clearEdits(); };
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &curves, &timeB, &volB, &timeOn, &volOn, &timeMix, &volAmt, &rate, &trigger, &smooth, &mix, &out, &reset }) body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void layout(juce::Rectangle<int> r) override {
        curveArea = r.removeFromTop(310);
        curves.setBounds(curveArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        r.removeFromTop(12);
        auto mid = r.removeFromTop(150); r.removeFromTop(12);
        timeArea = mid.removeFromLeft(mid.getWidth() / 2 - 6); mid.removeFromLeft(12); volArea = mid;
        auto t = timeArea.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        timeMix.setBounds(t.removeFromRight(100)); t.removeFromRight(10);
        timeOn.setBounds(t.removeFromBottom(24).withWidth(120)); t.removeFromBottom(6); timeB.setBounds(t);
        auto v = volArea.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        volAmt.setBounds(v.removeFromRight(100)); v.removeFromRight(10);
        volOn.setBounds(v.removeFromBottom(24).withWidth(120)); v.removeFromBottom(6); volB.setBounds(v);
        bottomArea = r;
        auto b = r.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        auto left = b.removeFromLeft(380);
        rateLabel = left.removeFromTop(14); left.removeFromTop(4); rate.setBounds(left.removeFromTop(32)); left.removeFromTop(10);
        trigLabel = left.removeFromTop(14); left.removeFromTop(4);
        auto tr = left.removeFromTop(32); reset.setBounds(tr.removeFromRight(110)); tr.removeFromRight(8); trigger.setBounds(tr);
        b.removeFromLeft(16);
        const int w = b.getWidth() / 3;
        for (auto* d : { &smooth, &mix, &out }) d->setBounds(b.removeFromLeft(w));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, curveArea, "Curves", "FS-01");
        module(g, timeArea, "Time", "FS-02");
        module(g, volArea, "Volume", "FS-03");
        module(g, bottomArea, "Play", "FS-04");
        station::drawCaps(g, "Rate", rateLabel.toFloat(), juce::Justification::centredLeft, 9.5f, dim);
        const bool midi = juce::roundToInt(fp.p("trigger")) == 1;
        station::drawCaps(g, midi ? "Trigger: hold C2-B4 for time curves, C5-B7 for volume (from each category)" : "Trigger",
                          trigLabel.toFloat().withWidth(600), juce::Justification::centredLeft, 9.0f, midi ? ink : dim);
    }
    void tick() override { curves.tick(); body.repaint(bottomArea); }
private:
    FlipsideProcessor& fp; CurveView curves; Browser timeB, volB; Switch timeOn, volOn; Dial timeMix, volAmt; Segments rate, trigger; Dial smooth, mix, out;
    juce::TextButton reset; juce::Rectangle<int> curveArea, timeArea, volArea, bottomArea, rateLabel, trigLabel;
};

juce::AudioProcessorEditor* FlipsideProcessor::createEditor() { return new FlipsideEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FlipsideProcessor(); }
