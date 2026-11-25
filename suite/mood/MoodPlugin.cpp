// LGN Mood: flips a sample between major and minor (or other modes) by moving just the notes that change.
#include "../common/Suite.h"
#include "../dsp/Gravity.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

const juce::Colour kAccent{ 0xff5a6ad6 };   // dusk blue

// Moves (in semitones) for each scale degree above the key, for each flip. Degrees: 0 root, 4 major 3rd, 3 minor
// 3rd, 9 major 6th, 8 minor 6th, 11 major 7th, 10 minor 7th.
struct Flip { const char* name; bool fromMajor; std::array<double, 12> shift; };
const std::vector<Flip>& flips() {
    static const std::vector<Flip> f = {
        { "Major to Minor",          true,  { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } },
        { "Major to Harmonic Minor", true,  { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } },
        { "Major to Dorian",         true,  { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } },
        { "Major: 3rd only",         true,  { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } },
        { "Minor to Major",          false, { 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0 } },
        { "Minor to Dorian",         false, { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
        { "Minor: 3rd only",         false, { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 } } };
    return f;
}
juce::StringArray flipNames() { juce::StringArray a; for (auto& f : flips()) a.add(f.name); return a; }
juce::StringArray keyNames() { juce::StringArray a; for (int i = 0; i < 12; i++) a.add(noteNames()[i]); return a; }

class MoodProcessor : public Processor {
public:
    MoodProcessor() : Processor("LGNMood", layout()) { setPresets(factoryPresets(), presetKeep()); }
    static param::Layout layout() {
        param::Layout l;
        param::addChoice(l, "key", "Key", keyNames(), 0);
        param::addChoice(l, "flip", "Flip", flipNames(), 0);
        param::addPct(l, "mood", "Mood", 100);
        param::addMs(l, "glide", "Glide", 0, 400, 30, 60);
        param::addHz(l, "floor", "Floor", 20, 400, 40);
        param::addPct(l, "mix", "Mix", 100);
        return l;
    }
    GravityParams params() const {
        GravityParams g; g.moodMode = true; g.fine = true; g.mass = 0;
        g.key = juce::roundToInt(p("key")); g.pull = p("mood") / 100.0; g.driftMs = p("glide"); g.floorHz = p("floor"); g.mix = p("mix") / 100.0;
        g.noteShift = flips()[(size_t)juce::jlimit(0, (int)flips().size() - 1, juce::roundToInt(p("flip")))].shift;
        return g;
    }
    void prepareToPlay(double sr, int) override { dsp.set(params()); dsp.prepare(sr); setLatencySamples(dsp.latency()); }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples(), ch = juce::jmin(2, getTotalNumInputChannels());
        for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); c++) buf.clear(c, 0, n);
        meterInput(buf, n);
        dsp.set(params());
        float* ptrs[2] = { buf.getWritePointer(0), ch > 1 ? buf.getWritePointer(1) : nullptr };
        dsp.process(ptrs, ch, n);
        const auto c = dsp.chromaNow(); for (int i = 0; i < 12; i++) chroma[(size_t)i] = (float)c[(size_t)i];
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 0.1; }
    juce::AudioProcessorEditor* createEditor() override;

    // Krumhansl-Schmuckler: correlate what has been playing with the major and minor key profiles.
    // Returns key (0-11) and whether it sounds major; false if there wasn't enough to go on.
    bool detectKey(int& key, bool& major) const {
        static const double maj[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
        static const double mnr[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
        double c[12], tot = 0; for (int i = 0; i < 12; i++) { c[i] = std::sqrt(chroma[(size_t)i].load()); tot += c[i]; }
        if (tot < 1e-6) return false;
        auto corr = [&](const double* prof, int k) {
            double mx = 0, my = 0; for (int i = 0; i < 12; i++) { mx += c[(i + k) % 12]; my += prof[i]; } mx /= 12; my /= 12;
            double sxy = 0, sxx = 0, syy = 0; for (int i = 0; i < 12; i++) { const double a = c[(i + k) % 12] - mx, b = prof[i] - my; sxy += a * b; sxx += a * a; syy += b * b; }
            return sxy / std::sqrt(sxx * syy + 1e-12);
        };
        double best = -2;
        for (int k = 0; k < 12; k++) {
            const double a = corr(maj, k), b = corr(mnr, k);
            if (a > best) { best = a; key = k; major = true; }
            if (b > best) { best = b; key = k; major = false; }
        }
        return true;
    }
    std::array<std::atomic<float>, 12> chroma{};
private:
    GravityDsp dsp;
};

// The twelve notes from the key up: what is playing (bars), which notes move (arrows) and where they go.
class NoteStrip : public juce::Component {
public:
    NoteStrip(MoodProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    void tick() { for (int i = 0; i < 12; i++) shown[(size_t)i] = shown[(size_t)i] * 0.8f + 0.2f * proc.chroma[(size_t)i].load(); repaint(); }
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        auto a = r.reduced(16, 14);
        const int key = juce::roundToInt(proc.p("key"));
        const auto& fl = flips()[(size_t)juce::jlimit(0, (int)flips().size() - 1, juce::roundToInt(proc.p("flip")))];
        const float mood = proc.p("mood") / 100.0f;
        float mx = 1e-9f; for (float v : shown) mx = juce::jmax(mx, v);
        const float w = a.getWidth() / 12.0f;
        const auto names = a.removeFromBottom(34);
        const auto arrows = a.removeFromTop(30);
        for (int i = 0; i < 12; i++) {
            const int pc = (key + i) % 12;
            const bool black = juce::String("010100101010").substring(pc, pc + 1) == "1";
            const auto col = juce::Rectangle<float>(a.getX() + i * w, a.getY(), w, a.getHeight()).reduced(4, 0);
            // activity
            const float lv = std::sqrt(shown[(size_t)pc] / mx);
            g.setColour(black ? juce::Colour(0xffe3e6ea) : juce::Colours::white); g.fillRoundedRectangle(col, 5);
            g.setColour(hairline); g.drawRoundedRectangle(col, 5, 1);
            const bool moves = fl.shift[(size_t)i] != 0;
            g.setColour((moves ? accent : station::graphite).withAlpha(0.25f + 0.55f * lv));
            g.fillRoundedRectangle(col.withTrimmedTop(col.getHeight() * (1 - lv)).reduced(3, 3), 3);
            // name and degree
            g.setFont(station::mono(11.0f, true)); g.setColour(i == 0 ? accent : ink);
            g.drawText(noteNames()[pc], juce::Rectangle<float>(col.getX(), names.getY() + 2, col.getWidth(), 14), juce::Justification::centred);
            static const char* deg[12] = { "1", "b2", "2", "b3", "3", "4", "b5", "5", "b6", "6", "b7", "7" };
            g.setFont(station::mono(9.0f)); g.setColour(faint);
            g.drawText(deg[i], juce::Rectangle<float>(col.getX(), names.getY() + 17, col.getWidth(), 12), juce::Justification::centred);
            // an arrow from a moving note to where it goes
            if (moves && mood > 0.01f) {
                const int dir = fl.shift[(size_t)i] > 0 ? 1 : -1;
                const float x0 = col.getCentreX(), x1 = x0 + dir * w * mood, y = arrows.getCentreY();
                juce::Path p; p.startNewSubPath(x0, y + 6); p.quadraticTo((x0 + x1) / 2, y - 10, x1, y + 6);
                g.setColour(accent); g.strokePath(p, juce::PathStrokeType(1.8f));
                juce::Path head; head.addTriangle(x1, y + 9, x1 - 4 * dir, y + 1, x1 + 3 * dir, y + 1);
                g.fillPath(head);
            }
        }
    }
private:
    MoodProcessor& proc; juce::Colour accent; std::array<float, 12> shown{};
};

class MoodEditor : public Editor {
public:
    explicit MoodEditor(MoodProcessor& p)
        : Editor(p, "Mood", "Major / minor flip", kAccent, 920, 560), mp(p), strip(p, accent),
          key(p.apvts, "key"), flip(p.apvts, "flip", "Flip"), mood(p.apvts, "mood", "Mood"), glide(p.apvts, "glide", "Glide"), floor(p.apvts, "floor", "Floor"), mix(p.apvts, "mix", "Mix") {
        mood.maxKnob = 108;
        detect.setButtonText("Detect key"); detect.getProperties().set("tone", "accent");
        detect.onClick = [this] {
            int k; bool major;
            if (mp.detectKey(k, major)) {
                setParam("key", (float)k);
                // pick the flip that starts from what was detected, keeping the same idea if it already does
                const int cur = juce::roundToInt(mp.p("flip"));
                if (flips()[(size_t)cur].fromMajor != major) setParam("flip", major ? 0.0f : 4.0f);
                status = juce::String("Sounds like ") + noteNames()[k] + (major ? " major" : " minor");
            } else status = "Play a few bars first, then press Detect";
            body.repaint();
        };
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &strip, &key, &flip, &mood, &glide, &floor, &mix, &detect }) body.addAndMakeVisible(c);
        setReady();
    }
protected:
    void setParam(const char* id, float v) { auto* prm = mp.apvts.getParameter(id); prm->beginChangeGesture(); prm->setValueNotifyingHost(prm->convertTo0to1(v)); prm->endChangeGesture(); }
    void layout(juce::Rectangle<int> r) override {
        stripArea = r.removeFromTop(250);
        strip.setBounds(stripArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        r.removeFromTop(12);
        keyArea = r.removeFromLeft(480); r.removeFromLeft(12); controlsArea = r;
        auto k = keyArea.reduced(16, 12).withTrimmedTop(station::kModuleTitle - 8);
        key.setBounds(k.removeFromTop(34)); k.removeFromTop(12);
        auto row = k.removeFromTop(50); flip.setBounds(row.removeFromLeft(250)); row.removeFromLeft(12); detect.setBounds(row.withTrimmedTop(16).withHeight(34));
        statusArea = k.reduced(0, 6);
        auto c = controlsArea.reduced(12, 12).withTrimmedTop(station::kModuleTitle - 8);
        mood.setBounds(c.removeFromLeft(140));
        const int w = c.getWidth() / 3;
        for (auto* d : { &glide, &floor, &mix }) d->setBounds(c.removeFromLeft(w));
    }
    void paintBody(juce::Graphics& g) override {
        module(g, stripArea, "Notes", "MD-01");
        module(g, keyArea, "Key", "MD-02");
        module(g, controlsArea, "Mood", "MD-03");
        g.setFont(station::sans(12.5f)); g.setColour(status.startsWith("Play") ? dim : ink);
        g.drawText(status.isEmpty() ? juce::String("Set the key, or play the sample and press Detect key") : status, statusArea.toFloat(), juce::Justification::centredLeft);
    }
    void tick() override { strip.tick(); }
private:
    MoodProcessor& mp; NoteStrip strip; Segments key; Choice flip; Dial mood, glide, floor, mix; juce::TextButton detect;
    juce::Rectangle<int> stripArea, keyArea, controlsArea, statusArea; juce::String status;
};

juce::AudioProcessorEditor* MoodProcessor::createEditor() { return new MoodEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MoodProcessor(); }
