// LGN Fret: a physically modelled guitar instrument. Strings, pluck, body, pickups, amp, cab and effects.
#include "../common/Suite.h"
#include "../dsp/Guitar.h"

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

const juce::Colour kAccent{ 0xff9b5b34 };   // rosewood
const juce::StringArray kDivNames{ "1/16", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2" };
constexpr double kDivBeats[8] = { 0.25, 1.0 / 3, 0.5, 0.75, 2.0 / 3, 1.0, 1.5, 2.0 };

juce::StringArray tuningNames() { juce::StringArray a; for (auto& t : tunings()) a.add(t.name); return a; }

class FretProcessor : public Processor {
public:
    FretProcessor()
        : Processor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true), "LGNFret", layout()) {
        setPresets(factoryPresets(), presetKeep());
    }
    static param::Layout layout() {
        using namespace param;
        Layout l;
        // play
        addChoice(l, "mode", "Play Mode", { "Guitar", "Free", "Legato" }, 0);
        addMs(l, "strum", "Strum", 0, 150, 35, 40);
        addChoice(l, "strumdir", "Strum Direction", { "Down", "Up", "Alternate" }, 2);
        addPct(l, "humanize", "Humanize", 30);
        addMs(l, "glide", "Glide", 5, 500, 60, 80);
        add(l, "bend", "Bend Range", { 1, 12, 1 }, 2, "st", 0);
        add(l, "vib", "Vibrato", { 0, 50, 0.1f }, 0, "ct", 0);
        add(l, "vibrate", "Vibrato Rate", { 2, 9, 0.01f }, 5.5f, "Hz", 1);
        addChoice(l, "tuning", "Tuning", tuningNames(), 0);
        // string
        addChoice(l, "pick", "Pluck", { "Pick", "Finger", "Thumb" }, 0);
        addPct(l, "hard", "Attack", 60);
        add(l, "pickpos", "Pick Position", { 3, 50, 0.1f }, 18, "%", 0);
        addPct(l, "noise", "Pick Noise", 30);
        addPct(l, "bright", "Brightness", 60);
        add(l, "decay", "Sustain", skewRange(0.3f, 20.0f, 4.0f), 4.0f, "s", 1);
        addPct(l, "stiff", "Stiffness", 20);
        addPct(l, "mute", "Palm Mute", 0);
        add(l, "release", "Release", skewRange(0.02f, 5.0f, 0.4f), 0.25f, "s", 2);
        addChoice(l, "course", "Strings", { "Single", "Double", "12-String" }, 0);
        add(l, "detune", "Detune", { 0, 30, 0.1f }, 6, "ct", 0);
        addPct(l, "vel", "Velocity", 70);
        // body / pickup
        addChoice(l, "body", "Body", { "None", "Dreadnought", "Parlor", "Classical", "Jumbo", "Archtop" }, 1);
        addPct(l, "bodyamt", "Body Amount", 70);
        add(l, "bodysize", "Body Size", { 70, 140, 0.1f }, 100, "%", 0);
        addChoice(l, "pickup", "Pickup", { "Off", "Single Coil", "Humbucker", "Piezo" }, 0);
        addPct(l, "pupos", "Pickup Position", 50);
        // amp
        addChoice(l, "amp", "Amp", { "Off", "Clean", "Crunch", "Lead", "Fuzz" }, 0);
        addDb(l, "drive", "Drive", 0, 40, 12);
        addDb(l, "bass", "Bass", -12, 12, 0);
        addDb(l, "mid", "Mid", -12, 12, 0);
        addDb(l, "treble", "Treble", -12, 12, 0);
        addChoice(l, "cab", "Cabinet", { "Off", "1x12", "2x12", "4x12" }, 0);
        addDb(l, "ampvol", "Amp Volume", -12, 12, 0);
        // fx
        addPct(l, "chorus", "Chorus", 0);
        add(l, "chrate", "Chorus Rate", logRange(0.1f, 5.0f), 0.8f, "Hz", 2);
        addPct(l, "dmix", "Delay", 0);
        addChoice(l, "ddiv", "Delay Time", kDivNames, 3);
        addPct(l, "dfb", "Delay Feedback", 30);
        addPct(l, "vmix", "Reverb", 15);
        addPct(l, "vsize", "Room Size", 50);
        addPct(l, "age", "Age", 0);
        addPct(l, "width", "Width", 60);
        addDb(l, "volume", "Volume", -24, 12, 0);
        return l;
    }
    GuitarParams params() const {
        GuitarParams g;
        g.mode = juce::roundToInt(p("mode")); g.strumMs = p("strum"); g.strumDir = juce::roundToInt(p("strumdir")); g.humanize = p("humanize") / 100.0;
        g.glideMs = p("glide"); g.bendSemis = p("bend"); g.vibCents = p("vib"); g.vibHz = p("vibrate"); g.tuning = juce::roundToInt(p("tuning"));
        g.pick = juce::roundToInt(p("pick")); g.pickHard = p("hard") / 100.0; g.pickPos = p("pickpos") / 100.0; g.pickNoise = p("noise") / 100.0;
        g.bright = p("bright") / 100.0; g.decayS = p("decay"); g.stiffness = p("stiff") / 100.0; g.mute = p("mute") / 100.0; g.releaseS = p("release");
        g.course = juce::roundToInt(p("course")); g.detuneCents = p("detune"); g.velSense = p("vel") / 100.0;
        g.body = juce::roundToInt(p("body")); g.bodyAmt = p("bodyamt") / 100.0; g.bodySize = p("bodysize") / 100.0;
        g.pickup = juce::roundToInt(p("pickup")); g.pickupPos = p("pupos") / 100.0;
        g.amp = juce::roundToInt(p("amp")); g.driveDb = p("drive"); g.bassDb = p("bass"); g.midDb = p("mid"); g.trebleDb = p("treble");
        g.cab = juce::roundToInt(p("cab")); g.ampVolDb = p("ampvol");
        g.chorus = p("chorus") / 100.0; g.chorusHz = p("chrate"); g.delayMix = p("dmix") / 100.0;
        g.delayMs = juce::jlimit(20.0, 2000.0, kDivBeats[juce::jlimit(0, 7, juce::roundToInt(p("ddiv")))] * 60000.0 / bpm.load());
        g.delayFb = p("dfb") / 100.0; g.verbMix = p("vmix") / 100.0; g.verbSize = p("vsize") / 100.0; g.age = p("age") / 100.0;
        g.width = p("width") / 100.0; g.volumeDb = p("volume");
        return g;
    }
    bool isBusesLayoutSupported(const BusesLayout& b) const override {
        const auto out = b.getMainOutputChannelSet();
        return b.inputBuses.isEmpty() && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
    }
    bool acceptsMidi() const override { return true; }
    void prepareToPlay(double sr, int) override { dsp.set(params()); dsp.prepare(sr); }
    void reset() override { dsp.allNotesOff(); }
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) override {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples();
        buf.clear();
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) bpm = juce::jlimit(20.0, 400.0, *b);
        dsp.set(params());
        for (const auto m : midi) {
            const auto msg = m.getMessage(); const int off = juce::jlimit(0, n - 1, m.samplePosition);
            if (msg.isNoteOn()) dsp.noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), off);
            else if (msg.isNoteOff()) dsp.noteOff(msg.getNoteNumber(), off);
            else if (msg.isPitchWheel()) dsp.pitchBend((msg.getPitchWheelValue() - 8192) / 8192.0);
            else if (msg.isController() && msg.getControllerNumber() == 1) dsp.modWheel(msg.getControllerValue() / 127.0);
            else if (msg.isSustainPedalOn()) dsp.sustainPedal(true);
            else if (msg.isSustainPedalOff()) dsp.sustainPedal(false);
            else if (msg.isAllNotesOff() || msg.isAllSoundOff()) dsp.allNotesOff();
        }
        float* L = buf.getWritePointer(0);
        if (buf.getNumChannels() > 1) dsp.process(L, buf.getWritePointer(1), n);
        else { scratchR.resize((size_t)n); std::fill(scratchR.begin(), scratchR.end(), 0.0f); dsp.process(L, scratchR.data(), n); for (int i = 0; i < n; i++) L[i] = 0.5f * (L[i] + scratchR[(size_t)i]); }
        // what the fretboard shows
        GuitarDsp::VoiceView vv[GuitarDsp::kVoices]; const int nv = dsp.voiceViews(vv);
        for (int i = 0; i < GuitarDsp::kVoices; i++) {
            uiNote[(size_t)i] = i < nv ? vv[i].note : -1; uiString[(size_t)i] = i < nv ? vv[i].string : -1; uiLevel[(size_t)i] = i < nv ? vv[i].level : 0.0f;
        }
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 4.0; }
    juce::AudioProcessorEditor* createEditor() override;

    std::atomic<double> bpm{ 120.0 };
    std::array<std::atomic<int>, GuitarDsp::kVoices> uiNote{}, uiString{};
    std::array<std::atomic<float>, GuitarDsp::kVoices> uiLevel{};
private:
    GuitarDsp dsp; std::vector<float> scratchR;
};

// ---------------------------------------------------------------- the fretboard
// Six strings across the neck (frets spaced like a real guitar). Each sounding note shows as a dot at its fret,
// and its string vibrates with how loud it is.
class Fretboard : public juce::Component {
public:
    Fretboard(FretProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    void tick() { phase += 0.9f; for (int i = 0; i < GuitarDsp::kVoices; i++) { const float l = proc.uiLevel[(size_t)i].load(); shown[(size_t)i] = juce::jmax(l, shown[(size_t)i] * 0.86f); } repaint(); }
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        auto neck = r.reduced(16, 14);
        auto labels = neck.removeFromLeft(48);
        neck.removeFromBottom(14);
        const int kFrets = 17;
        // fret x positions: the real 12th-root-of-two spacing, scaled to fill the width
        auto fretX = [&](double k) { const double full = 1 - std::pow(2.0, -kFrets / 12.0); return neck.getX() + neck.getWidth() * (float)((1 - std::pow(2.0, -k / 12.0)) / full); };
        // inlays
        for (int k : { 3, 5, 7, 9, 12, 15, 17 }) {
            const float x = 0.5f * (fretX(k - 1) + fretX(k));
            g.setColour(juce::Colour(0xffe6e9ed));
            if (k == 12) { g.fillEllipse(x - 5, neck.getY() + neck.getHeight() * 0.3f - 5, 10, 10); g.fillEllipse(x - 5, neck.getY() + neck.getHeight() * 0.7f - 5, 10, 10); }
            else g.fillEllipse(x - 5, neck.getCentreY() - 5, 10, 10);
            g.setFont(station::mono(9.0f)); g.setColour(faint);
            g.drawText(juce::String(k), juce::Rectangle<float>(x - 12, neck.getBottom() + 1, 24, 12), juce::Justification::centred);
        }
        // frets and nut
        for (int k = 1; k <= kFrets; k++) { g.setColour(station::hairlineHi); g.drawVerticalLine((int)fretX(k), neck.getY(), neck.getBottom()); }
        g.setColour(station::graphite); g.fillRoundedRectangle(neck.getX() - 3, neck.getY(), 4, neck.getHeight(), 2);
        // strings (low E at the bottom, like looking down at the neck)
        const int* open = tunings()[(size_t)juce::jlimit(0, (int)tunings().size() - 1, juce::roundToInt(proc.p("tuning")))].open;
        auto stringY = [&](int s) { return neck.getBottom() - neck.getHeight() * (0.08f + 0.84f * s / 5.0f); };
        float amp[GuitarDsp::kStrings] = {}; int fretOn[GuitarDsp::kStrings]; int noteOn[GuitarDsp::kStrings];
        for (int s = 0; s < 6; s++) { fretOn[s] = -1; noteOn[s] = -1; }
        for (int i = 0; i < GuitarDsp::kVoices; i++) {
            const int s = proc.uiString[(size_t)i].load(), nt = proc.uiNote[(size_t)i].load();
            if (s < 0 || s > 5 || nt < 0) continue;
            if (shown[(size_t)i] > amp[s]) { amp[s] = shown[(size_t)i]; fretOn[s] = nt - open[s]; noteOn[s] = nt; }
        }
        for (int s = 0; s < 6; s++) {
            const float y = stringY(s), thick = 2.6f - 0.35f * s;
            station::drawCaps(g, juce::MidiMessage::getMidiNoteName(open[s], true, false, 4), juce::Rectangle<float>(labels.getX(), y - 7, 20, 14), juce::Justification::centredLeft, 9.0f, dim);
            const float a = juce::jmin(1.0f, amp[s]);
            juce::Path str;
            const float x0 = neck.getX(), x1 = neck.getRight();
            // a vibrating string: the fundamental's shape between the fretted note and the bridge, scaled by level
            const float from = fretOn[s] > 0 ? fretX(juce::jmin(fretOn[s], kFrets)) : x0;
            str.startNewSubPath(x0, y);
            if (a > 0.01f) {
                str.lineTo(from, y);
                for (int k = 0; k <= 60; k++) {
                    const float t = k / 60.0f, x = from + (x1 - from) * t;
                    str.lineTo(x, y + std::sin(t * juce::MathConstants<float>::pi) * a * 7.0f * std::sin(phase + s * 1.3f));
                }
            } else str.lineTo(x1, y);
            g.setColour(a > 0.01f ? accent.interpolatedWith(ink, 0.15f).withAlpha(0.55f + 0.45f * a) : (s < 3 ? station::graphite.withAlpha(0.55f) : station::graphite.withAlpha(0.4f)));
            g.strokePath(str, juce::PathStrokeType(thick));
            // the fretted note
            if (noteOn[s] >= 0 && a > 0.01f) {
                const int f = fretOn[s];
                const float x = f <= 0 ? neck.getX() - 15 : 0.5f * (fretX(juce::jmin(f, kFrets) - 1) + fretX(juce::jmin(f, kFrets)));
                const float d = 15;
                g.setColour(accent.withAlpha(0.18f * a)); g.fillEllipse(x - d, y - d, d * 2, d * 2);
                g.setColour(f <= 0 ? juce::Colours::white : accent); g.fillEllipse(x - 9, y - 9, 18, 18);
                if (f <= 0) { g.setColour(accent); g.drawEllipse(x - 8.5f, y - 8.5f, 17, 17, 1.5f); }
                g.setFont(station::mono(8.5f, true)); g.setColour(f <= 0 ? accent : juce::Colours::white);
                g.drawText(juce::MidiMessage::getMidiNoteName(noteOn[s], true, false, 4), juce::Rectangle<float>(x - 9, y - 9, 18, 18), juce::Justification::centred);
            }
        }
    }
private:
    FretProcessor& proc; juce::Colour accent; float phase = 0;
    std::array<float, GuitarDsp::kVoices> shown{};
};

class FretEditor : public Editor {
public:
    explicit FretEditor(FretProcessor& p)
        : Editor(p, "Fret", "Modelled guitar", kAccent, 1120, 780), fp(p), board(p, accent),
          mode(p.apvts, "mode"), strumDir(p.apvts, "strumdir"), tuning(p.apvts, "tuning", "Tuning"),
          strum(p.apvts, "strum", "Strum", true), humanize(p.apvts, "humanize", "Humanize", true), glide(p.apvts, "glide", "Glide", true),
          bend(p.apvts, "bend", "Bend", true), vib(p.apvts, "vib", "Vibrato", true), vibRate(p.apvts, "vibrate", "Vib rate", true),
          pick(p.apvts, "pick"), course(p.apvts, "course"),
          hard(p.apvts, "hard", "Attack", true), pickPos(p.apvts, "pickpos", "Pick pos", true), noise(p.apvts, "noise", "Pick noise", true),
          bright(p.apvts, "bright", "Bright", true), decay(p.apvts, "decay", "Sustain", true), stiff(p.apvts, "stiff", "Stiffness", true),
          mute(p.apvts, "mute", "Palm mute", true), release(p.apvts, "release", "Release", true), detune(p.apvts, "detune", "Detune", true), vel(p.apvts, "vel", "Velocity", true),
          bodyType(p.apvts, "body", "Body"), pickup(p.apvts, "pickup", "Pickup"),
          bodyAmt(p.apvts, "bodyamt", "Body", true), bodySize(p.apvts, "bodysize", "Size", true), puPos(p.apvts, "pupos", "Neck / Bridge", true),
          amp(p.apvts, "amp"), cab(p.apvts, "cab"),
          drive(p.apvts, "drive", "Drive", true), bass(p.apvts, "bass", "Bass", true), mid(p.apvts, "mid", "Mid", true), treble(p.apvts, "treble", "Treble", true), ampVol(p.apvts, "ampvol", "Level", true),
          chorus(p.apvts, "chorus", "Chorus", true), chRate(p.apvts, "chrate", "Rate", true), dmix(p.apvts, "dmix", "Delay", true), ddiv(p.apvts, "ddiv", "Time"),
          dfb(p.apvts, "dfb", "Feedback", true), vmix(p.apvts, "vmix", "Reverb", true), vsize(p.apvts, "vsize", "Size", true), age(p.apvts, "age", "Age", true),
          width(p.apvts, "width", "Width", true), volume(p.apvts, "volume", "Volume", true) {
        for (juce::Component* c : std::initializer_list<juce::Component*>{ &board, &mode, &strumDir, &tuning, &strum, &humanize, &glide, &bend, &vib, &vibRate,
                 &pick, &course, &hard, &pickPos, &noise, &bright, &decay, &stiff, &mute, &release, &detune, &vel, &bodyType, &pickup, &bodyAmt, &bodySize, &puPos,
                 &amp, &cab, &drive, &bass, &mid, &treble, &ampVol, &chorus, &chRate, &dmix, &ddiv, &dfb, &vmix, &vsize, &age, &width, &volume })
            Editor::body.addAndMakeVisible(c);
        setReady();
    }
protected:
    static void row(juce::Rectangle<int> r, std::initializer_list<juce::Component*> cs) {
        const int w = r.getWidth() / (int)cs.size();
        for (auto* c : cs) c->setBounds(r.removeFromLeft(w));
    }
    void layout(juce::Rectangle<int> r) override {
        boardArea = r.removeFromTop(200);
        board.setBounds(boardArea.reduced(14).withTrimmedTop(station::kModuleTitle - 12));
        r.removeFromTop(12);
        auto mid2 = r.removeFromTop(258); r.removeFromTop(12);
        playArea = mid2.removeFromLeft(330); mid2.removeFromLeft(12);
        stringArea = mid2.removeFromLeft(440); mid2.removeFromLeft(12);
        bodyArea = mid2;
        ampArea = r.removeFromLeft(470); r.removeFromLeft(12);
        fxArea = r;
        auto inner = [](juce::Rectangle<int> a) { return a.reduced(14, 10).withTrimmedTop(station::kModuleTitle - 8); };
        {   auto a = inner(playArea);
            mode.setBounds(a.removeFromTop(32)); a.removeFromTop(2);
            auto s = a.removeFromTop(48); strumDir.setBounds(s.removeFromLeft(s.getWidth() * 3 / 5).removeFromBottom(32)); s.removeFromLeft(8); tuning.setBounds(s);
            a.removeFromTop(2);
            row(a.removeFromTop(a.getHeight() / 2), { &strum, &humanize, &glide }); row(a, { &bend, &vib, &vibRate }); }
        {   auto a = inner(stringArea);
            auto s = a.removeFromTop(32); pick.setBounds(s.removeFromLeft(s.getWidth() / 2 - 4)); s.removeFromLeft(8); course.setBounds(s);
            a.removeFromTop(8);
            row(a.removeFromTop(a.getHeight() / 2), { &hard, &pickPos, &noise, &bright, &decay }); row(a, { &stiff, &mute, &release, &detune, &vel }); }
        {   auto a = inner(bodyArea);
            auto s = a.removeFromTop(48); bodyType.setBounds(s.removeFromLeft(s.getWidth() / 2 - 4)); s.removeFromLeft(8); pickup.setBounds(s);
            a.removeFromTop(10);
            row(a.removeFromTop(juce::jmin(a.getHeight(), 104)), { &bodyAmt, &bodySize, &puPos }); }
        {   auto a = inner(ampArea);
            auto s = a.removeFromTop(32); amp.setBounds(s.removeFromLeft(s.getWidth() * 5 / 9)); s.removeFromLeft(8); cab.setBounds(s);
            a.removeFromTop(6);
            row(a, { &drive, &bass, &mid, &treble, &ampVol }); }
        {   auto a = inner(fxArea);
            row(a.removeFromTop(a.getHeight() / 2), { &chorus, &chRate, &dmix, &ddiv, &dfb }); row(a, { &vmix, &vsize, &age, &width, &volume });
            ddiv.setBounds(ddiv.getBounds().withSizeKeepingCentre(ddiv.getWidth() - 6, 50)); }
    }
    void paintBody(juce::Graphics& g) override {
        module(g, boardArea, "Fretboard", "FR-01");
        module(g, playArea, "Play", "FR-02");
        module(g, stringArea, "String", "FR-03");
        module(g, bodyArea, "Body / Pickup", "FR-04");
        module(g, ampArea, "Amp", "FR-05");
        module(g, fxArea, "Effects", "FR-06");
    }
    void tick() override {
        board.tick();
        const bool legato = juce::roundToInt(fp.p("mode")) == 2, ampOn = juce::roundToInt(fp.p("amp")) > 0, electric = juce::roundToInt(fp.p("pickup")) > 0;
        const bool hasBody = juce::roundToInt(fp.p("body")) > 0, doubled = juce::roundToInt(fp.p("course")) > 0;
        glide.setAlpha(legato ? 1.0f : 0.4f);
        for (auto* d : { &drive, &bass, &mid, &treble, &ampVol }) d->setAlpha(ampOn ? 1.0f : 0.4f);
        puPos.setAlpha(electric ? 1.0f : 0.4f);
        for (auto* d : { &bodyAmt, &bodySize }) d->setAlpha(hasBody ? 1.0f : 0.4f);
        detune.setAlpha(doubled ? 1.0f : 0.4f);
    }
private:
    FretProcessor& fp; Fretboard board;
    Segments mode, strumDir; Choice tuning; Dial strum, humanize, glide, bend, vib, vibRate;
    Segments pick, course; Dial hard, pickPos, noise, bright, decay, stiff, mute, release, detune, vel;
    Choice bodyType, pickup; Dial bodyAmt, bodySize, puPos;
    Segments amp, cab; Dial drive, bass, mid, treble, ampVol;
    Dial chorus, chRate, dmix; Choice ddiv; Dial dfb, vmix, vsize, age, width, volume;
    juce::Rectangle<int> boardArea, playArea, stringArea, bodyArea, ampArea, fxArea;
};

juce::AudioProcessorEditor* FretProcessor::createEditor() { return new FretEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FretProcessor(); }
