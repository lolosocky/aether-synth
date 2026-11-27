// LGN Haze: the flagship mono bass synth of the LGN suite.
// Two morphing oscillators with unison, sync and FM, a sub and noise, ten filter types, bass-keeping drive,
// three envelopes plus an 808 pitch envelope, two LFOs, an 8-slot mod matrix, glide, a note-repeat / arp /
// 16-step sequencer, and a full effects chain with a sub booster.
#include "../common/Suite.h"
#include "../dsp/Haze.h"
#include <deque>

namespace {
#include "Presets.inc"
using namespace lgn;
using station::ink; using station::dim; using station::faint; using station::hairline;

const juce::Colour kAccent{ 0xff72b31a };   // acid green

juce::StringArray fromC(const char* const* n, int count) { juce::StringArray a; for (int i = 0; i < count; i++) a.add(n[i]); return a; }
juce::StringArray divNames() { juce::StringArray a; for (auto& d : hzDivs()) a.add(d.first); return a; }
const juce::StringArray kLfoDivs{ "1/32", "1/16T", "1/16", "1/8T", "1/16D", "1/8", "1/4T", "1/8D", "1/4", "1/4D", "1/2", "1 Bar", "2 Bars", "4 Bars" };
const juce::StringArray kFilterTypes{ "Ladder 24", "Ladder 12", "SVF Low", "SVF Band", "SVF High", "Notch", "Comb +", "Comb -", "Vowel", "Off" };
const juce::StringArray kDirtTypes{ "Tube", "Tape", "Hard Clip", "Fold", "Rectify", "Crush" };
const juce::StringArray kLfoShapes{ "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Smooth" };
// rates show two decimals ("0.30 Hz"), which the suite's Hz formatter rounds away
void addRate(param::Layout& l, const char* id, const char* name, float lo, float hi, float def) {
    auto attrs = juce::AudioParameterFloatAttributes().withLabel("Hz").withStringFromValueFunction([](float v, int) { return juce::String(v, v < 10 ? 2 : 1) + " Hz"; });
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ id, 1 }, name, param::logRange(lo, hi), def, attrs));
}

class HazeProcessor : public Processor {
public:
    HazeProcessor() : Processor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true), "LGNHaze", layout()) {
        setPresets(factoryPresets(), presetKeep());
    }
    static param::Layout layout() {
        using namespace param;
        Layout l;
        for (int o = 1; o <= 2; o++) {
            auto id = [&](const char* s) { static std::deque<std::string> keep; keep.push_back("o" + std::to_string(o) + s); return keep.back().c_str(); };
            auto nm = [&](const char* s) { static std::deque<std::string> keep; keep.push_back("Osc " + std::to_string(o) + " " + s); return keep.back().c_str(); };
            {   // the morph position as names: "Saw", "Saw > Pulse 40%"
                auto attrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) {
                    static const char* w[4] = { "Sine", "Tri", "Saw", "Pulse" };
                    const int i = juce::jlimit(0, 3, (int)std::floor(v + 0.02f)); const float f = v - (float)i;
                    if (i == 3 || f < 0.02f) return juce::String(w[i]);
                    return juce::String(w[i]) + ">" + w[i + 1] + " " + juce::String(juce::roundToInt(f * 100)) + "%";
                }).withValueFromStringFunction([](const juce::String& t) { return t.getFloatValue(); });
                l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ id("wave"), 1 }, nm("Wave"), juce::NormalisableRange<float>(0, 3, 0.001f), 2.0f, attrs));
            }
            addChoice(l, id("oct"), nm("Octave"), { "-2", "-1", "0", "+1", "+2" }, 2);
            add(l, id("semi"), nm("Semitone"), { -12, 12, 1 }, o == 2 ? 7.0f : 0.0f, "st", 0);
            add(l, id("fine"), nm("Fine"), { -100, 100, 0.1f }, 0, "ct", 0);
            addPct(l, id("level"), nm("Level"), o == 1 ? 100.0f : 0.0f);
            add(l, id("pw"), nm("Pulse Width"), { 5, 95, 0.1f }, 50, "%", 0);
            add(l, id("voices"), nm("Voices"), { 1, 7, 1 }, 1, "", 0);
            addPct(l, id("detune"), nm("Detune"), 20);
            addPct(l, id("spread"), nm("Spread"), 50);
        }
        addBool(l, "sync", "Osc 2 Sync", false);
        addPct(l, "fm", "FM", 0);
        addBool(l, "phase", "Phase Reset", true);
        addPct(l, "drift", "Drift", 15);
        addChoice(l, "subwave", "Sub Wave", { "Sine", "Triangle", "Square", "Pulse" }, 0);
        addChoice(l, "suboct", "Sub Octave", { "0", "-1", "-2" }, 1);
        addPct(l, "sublevel", "Sub Level", 0);
        addBool(l, "subpost", "Sub After Filter", false);
        addChoice(l, "noisetype", "Noise Type", { "White", "Pink", "Brown" }, 0);
        addPct(l, "noise", "Noise", 0);
        // filter
        addChoice(l, "ftype", "Filter Type", kFilterTypes, 0);
        addHz(l, "cutoff", "Cutoff", 20, 20000, 1200);
        addPct(l, "res", "Resonance", 15);
        addDb(l, "fdrive", "Filter Drive", 0, 36, 0);
        addPct(l, "keytrack", "Key Track", 50);
        add(l, "fenv", "Filter Env", { -96, 96, 0.1f }, 24, "st", 0);
        addPct(l, "fvel", "Filter Velocity", 0);
        addPct(l, "lowkeep", "Low Keep", 50);
        // dirt
        addChoice(l, "dirttype", "Dirt Type", kDirtTypes, 0);
        addPct(l, "dirt", "Dirt", 0);
        addPct(l, "dirttone", "Dirt Tone", 60);
        addPct(l, "dirtmix", "Dirt Mix", 100);
        addPct(l, "basskeep", "Bass Keep", 60);
        // envelopes
        auto env = [&](const char* pre, const char* name, float a, float d, float s, float r) {
            static std::deque<std::string> keep;
            auto k = [&](const char* x) { keep.push_back(std::string(pre) + x); return keep.back().c_str(); };
            auto n = [&](const char* x) { keep.push_back(std::string(name) + " " + x); return keep.back().c_str(); };
            add(l, k("a"), n("Attack"), skewRange(0, 10000, 300), a, "ms", 1);
            add(l, k("d"), n("Decay"), skewRange(1, 10000, 600), d, "ms", 0);
            addPct(l, k("s"), n("Sustain"), s);
            add(l, k("r"), n("Release"), skewRange(1, 10000, 600), r, "ms", 0);
        };
        env("a", "Amp", 2, 400, 80, 150);
        env("f", "Filter", 2, 350, 20, 250);
        env("m", "Mod", 10, 500, 0, 300);
        addPct(l, "velamp", "Velocity", 50);
        // pitch / play
        add(l, "penv", "Pitch Env", { -48, 48, 0.1f }, 0, "st", 1);
        add(l, "penvtime", "Pitch Env Time", skewRange(5, 2000, 150), 120, "ms", 0);
        add(l, "glide", "Glide", skewRange(0, 2000, 200), 60, "ms", 0);
        addChoice(l, "glidemode", "Glide Mode", { "Off", "Always", "Legato" }, 0);
        addChoice(l, "glideshape", "Glide Shape", { "Exponential", "Linear" }, 0);
        addChoice(l, "trigger", "Trigger", { "Retrigger", "Legato" }, 0);
        addChoice(l, "priority", "Note Priority", { "Last", "Low", "High" }, 0);
        add(l, "bend", "Bend Range", { 0, 24, 1 }, 2, "st", 0);
        for (int k = 1; k <= 2; k++) {
            static std::deque<std::string> keep;
            auto id = [&](const char* s) { keep.push_back("l" + std::to_string(k) + s); return keep.back().c_str(); };
            auto nm = [&](const char* s) { keep.push_back("LFO " + std::to_string(k) + " " + s); return keep.back().c_str(); };
            addChoice(l, id("shape"), nm("Shape"), kLfoShapes, 0);
            addBool(l, id("sync"), nm("Sync"), k == 1);
            addRate(l, id("rate"), nm("Rate"), 0.02f, 50.0f, k == 1 ? 2.0f : 0.3f);
            addChoice(l, id("div"), nm("Division"), kLfoDivs, 5);
            addBool(l, id("retrig"), nm("Retrigger"), false);
        }
        for (int i = 0; i < 8; i++) {
            static std::deque<std::string> keep;
            auto id = [&](const char* s) { keep.push_back("m" + std::to_string(i + 1) + s); return keep.back().c_str(); };
            auto nm = [&](const char* s) { keep.push_back("Mod " + std::to_string(i + 1) + " " + s); return keep.back().c_str(); };
            addChoice(l, id("src"), nm("Source"), fromC(hzSrcNames(), (int)HzSrc::count), 0);
            addChoice(l, id("dst"), nm("Destination"), fromC(hzDstNames(), (int)HzDst::count), 0);
            add(l, id("amt"), nm("Amount"), { -100, 100, 0.1f }, 0, "%", 0);
        }
        // pattern
        addChoice(l, "patmode", "Pattern", { "Off", "Repeat", "Arp", "Sequencer" }, 0);
        addChoice(l, "patdiv", "Pattern Rate", divNames(), 3);
        addChoice(l, "arpdir", "Arp Direction", { "Up", "Down", "Up-Down", "Down-Up", "Random", "As Played" }, 0);
        addChoice(l, "arpoct", "Arp Octaves", { "1", "2", "3", "4" }, 0);
        add(l, "seqlen", "Steps", { 1, 16, 1 }, 16, "", 0);
        addPct(l, "gate", "Gate", 60);
        addPct(l, "swing", "Swing", 0);
        addPct(l, "accent", "Accent", 50);
        for (int i = 0; i < 16; i++) {
            static std::deque<std::string> keep;
            auto id = [&](const char* s) { keep.push_back("s" + std::to_string(i + 1) + s); return keep.back().c_str(); };
            auto nm = [&](const char* s) { keep.push_back("Step " + std::to_string(i + 1) + " " + s); return keep.back().c_str(); };
            addBool(l, id("on"), nm("On"), true);
            add(l, id("pitch"), nm("Pitch"), { -24, 24, 1 }, 0, "st", 0);
            addBool(l, id("acc"), nm("Accent"), false);
            addBool(l, id("slide"), nm("Slide"), false);
        }
        // effects
        addDb(l, "eqlow", "EQ Low", -12, 12, 0); addHz(l, "eqlowf", "EQ Low Freq", 30, 300, 100);
        addDb(l, "eqmid", "EQ Mid", -12, 12, 0); addHz(l, "eqmidf", "EQ Mid Freq", 100, 8000, 800);
        addDb(l, "eqhigh", "EQ High", -12, 12, 0); addHz(l, "eqhighf", "EQ High Freq", 2000, 16000, 6000);
        addPct(l, "subboost", "Sub Boost", 0); addPct(l, "harm", "Harmonics", 0); addHz(l, "boostf", "Boost Freq", 30, 160, 60);
        addPct(l, "comp", "Compressor", 0);
        addPct(l, "chorus", "Chorus", 0); addRate(l, "chrate", "Chorus Rate", 0.05f, 5.0f, 0.6f); addPct(l, "chdepth", "Chorus Depth", 50);
        addPct(l, "dmix", "Delay", 0); addPct(l, "dfb", "Delay Feedback", 30); addChoice(l, "ddiv", "Delay Time", divNames(), 6); addBool(l, "pingpong", "Ping-Pong", true);
        addPct(l, "vmix", "Reverb", 0); addPct(l, "vsize", "Reverb Size", 50);
        add(l, "width", "Width", { 0, 200, 0.1f }, 100, "%", 0); addHz(l, "monobelow", "Mono Below", 20, 400, 120);
        addBool(l, "limiter", "Limiter", true);
        addDb(l, "volume", "Volume", -24, 12, 0);
        return l;
    }
    int ri(const char* id) const { return juce::roundToInt(p(id)); }
    bool slotTargets(HzDst d) const {
        for (int i = 1; i <= 8; i++) if (ri(("m" + std::to_string(i) + "src").c_str()) > 0 && ri(("m" + std::to_string(i) + "dst").c_str()) == (int)d) return true;
        return false;
    }
    float pp(const juce::String& id) const { return p(id.toRawUTF8()); }
    HazeParams params() const {
        HazeParams h;
        for (int o = 0; o < 2; o++) {
            auto& O = o == 0 ? h.osc1 : h.osc2; const juce::String pre = "o" + juce::String(o + 1);
            O.wave = pp(pre + "wave"); O.octave = juce::roundToInt(pp(pre + "oct")) - 2; O.semi = pp(pre + "semi"); O.fine = pp(pre + "fine");
            O.level = pp(pre + "level") / 100.0; O.pw = pp(pre + "pw") / 100.0; O.voices = juce::roundToInt(pp(pre + "voices"));
            O.detune = pp(pre + "detune") / 100.0; O.spread = pp(pre + "spread") / 100.0;
        }
        h.sync = p("sync") > 0.5f; h.fm = p("fm") / 100.0; h.phaseReset = p("phase") > 0.5f; h.drift = p("drift") / 100.0;
        h.subWave = ri("subwave"); h.subOct = ri("suboct"); h.subLevel = p("sublevel") / 100.0; h.subPostFilter = p("subpost") > 0.5f;
        h.noiseType = ri("noisetype"); h.noiseLevel = p("noise") / 100.0;
        h.fType = ri("ftype"); h.cutoff = p("cutoff"); h.res = p("res") / 100.0; h.fDrive = p("fdrive"); h.keyTrack = p("keytrack") / 100.0;
        h.envAmt = p("fenv"); h.fVel = p("fvel") / 100.0; h.lowKeep = p("lowkeep") / 100.0;
        h.dirtType = ri("dirttype"); h.dirt = p("dirt") / 100.0; h.dirtTone = p("dirttone") / 100.0; h.dirtMix = p("dirtmix") / 100.0; h.bassKeep = p("basskeep") / 100.0;
        auto env = [&](const char* pre) { const juce::String s(pre); return HzEnv{ pp(s + "a") / 1000.0, pp(s + "d") / 1000.0, pp(s + "s") / 100.0, pp(s + "r") / 1000.0 }; };
        h.ampEnv = env("a"); h.fEnv = env("f"); h.mEnv = env("m"); h.velAmp = p("velamp") / 100.0;
        h.pEnvAmt = p("penv"); h.pEnvTime = p("penvtime") / 1000.0; h.glide = p("glide") / 1000.0; h.glideMode = ri("glidemode"); h.glideShape = ri("glideshape");
        h.trigger = ri("trigger"); h.priority = ri("priority"); h.bend = p("bend");
        for (int k = 0; k < 2; k++) {
            auto& L = k == 0 ? h.lfo1 : h.lfo2; const juce::String pre = "l" + juce::String(k + 1);
            L.shape = juce::roundToInt(pp(pre + "shape")); L.sync = pp(pre + "sync") > 0.5f; L.hz = pp(pre + "rate"); L.div = juce::roundToInt(pp(pre + "div")); L.retrig = pp(pre + "retrig") > 0.5f;
        }
        for (int i = 0; i < 8; i++) { const juce::String pre = "m" + juce::String(i + 1); h.slots[(size_t)i] = { juce::roundToInt(pp(pre + "src")), juce::roundToInt(pp(pre + "dst")), pp(pre + "amt") / 100.0 }; }
        h.patMode = ri("patmode"); h.patDiv = ri("patdiv"); h.arpDir = ri("arpdir"); h.arpOct = ri("arpoct") + 1; h.seqLen = ri("seqlen");
        h.gate = p("gate") / 100.0; h.swing = p("swing") / 100.0; h.accent = p("accent") / 100.0;
        for (int i = 0; i < 16; i++) { const juce::String pre = "s" + juce::String(i + 1); h.steps[(size_t)i] = { pp(pre + "on") > 0.5f, juce::roundToInt(pp(pre + "pitch")), pp(pre + "acc") > 0.5f, pp(pre + "slide") > 0.5f }; }
        h.eqLow = p("eqlow"); h.eqLowHz = p("eqlowf"); h.eqMid = p("eqmid"); h.eqMidHz = p("eqmidf"); h.eqHigh = p("eqhigh"); h.eqHighHz = p("eqhighf");
        h.subBoost = p("subboost") / 100.0; h.harmonics = p("harm") / 100.0; h.boostHz = p("boostf"); h.comp = p("comp") / 100.0;
        h.chorus = p("chorus") / 100.0; h.chorusRate = p("chrate"); h.chorusDepth = p("chdepth") / 100.0;
        h.delayMix = p("dmix") / 100.0; h.delayFb = p("dfb") / 100.0; h.delayDiv = ri("ddiv"); h.pingPong = p("pingpong") > 0.5f;
        h.verbMix = p("vmix") / 100.0; h.verbSize = p("vsize") / 100.0; h.width = p("width") / 100.0; h.monoBelow = p("monobelow");
        h.limiter = p("limiter") > 0.5f; h.volumeDb = p("volume");
        return h;
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
        double bpmNow = 120, ppq = 0; bool playing = false;
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) {
            if (auto b = pos->getBpm()) bpmNow = *b;
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) ppq = *q; else playing = false;
        }
        dsp.set(params()); dsp.setTransport(ppq, bpmNow, playing);
        for (const auto m : midi) {
            const auto msg = m.getMessage(); const int off = juce::jlimit(0, n - 1, m.samplePosition);
            if (msg.isNoteOn()) dsp.noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), off);
            else if (msg.isNoteOff()) dsp.noteOff(msg.getNoteNumber(), off);
            else if (msg.isPitchWheel()) dsp.pitchBend((msg.getPitchWheelValue() - 8192) / 8192.0);
            else if (msg.isController() && msg.getControllerNumber() == 1) dsp.modWheel(msg.getControllerValue() / 127.0);
            else if (msg.isChannelPressure()) dsp.aftertouch(msg.getChannelPressureValue() / 127.0);
            else if (msg.isAftertouch()) dsp.aftertouch(msg.getAfterTouchValue() / 127.0);
            else if (msg.isAllNotesOff() || msg.isAllSoundOff()) dsp.allNotesOff();
        }
        float* L = buf.getWritePointer(0);
        if (buf.getNumChannels() > 1) dsp.process(L, buf.getWritePointer(1), n);
        else { scratchR.resize((size_t)n); dsp.process(L, scratchR.data(), n); for (int i = 0; i < n; i++) L[i] = 0.5f * (L[i] + scratchR[(size_t)i]); }
        for (int i = 0; i < n; i++) { scope[(size_t)scopePos].store(L[i], std::memory_order_relaxed); scopePos = (scopePos + 1) % kScope; }
        scopeHead.store(scopePos);
        uiGr = (float)dsp.compReductionDb(); uiOut = juce::Decibels::gainToDecibels(juce::jmax(buf.getMagnitude(0, 0, n), 1e-6f));
        const auto v = dsp.view(); uiNote = v.note; uiStep = v.step; uiHz = (float)v.hz; uiCut = (float)v.cutoffHz; uiEnv = (float)v.env;
        meterOutput(buf, n);
    }
    double getTailLengthSeconds() const override { return 6.0; }
    juce::AudioProcessorEditor* createEditor() override;

    static constexpr int kScope = 4096;
    std::array<std::atomic<float>, kScope> scope{}; std::atomic<int> scopeHead{ 0 }; int scopePos = 0;
    std::atomic<int> uiNote{ -1 }, uiStep{ -1 }; std::atomic<float> uiHz{ 0 }, uiCut{ 1000 }, uiEnv{ 0 }, uiGr{ 0 }, uiOut{ -90 };
    int page = 0;   // the editor's open page, kept while the plugin lives
private:
    HazeDsp dsp; std::vector<float> scratchR;
};

// ---------------------------------------------------------------- oscilloscope
// Two cycles of the output, locked to the note's pitch so the wave stands still and you can watch it morph.
class Scope : public juce::Component {
public:
    Scope(HazeProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        auto a = r.reduced(16, 12);
        auto info = a.removeFromRight(170);
        // grid
        g.setColour(hairline); g.drawHorizontalLine((int)a.getCentreY(), a.getX(), a.getRight());
        for (int k = 1; k < 4; k++) g.drawVerticalLine((int)(a.getX() + a.getWidth() * k / 4), a.getY(), a.getBottom());
        const int head = proc.scopeHead.load(); const int N = HazeProcessor::kScope;
        const double sr = juce::jmax(8000.0, proc.getSampleRate());
        const float hzv = proc.uiHz.load(); const bool on = proc.uiNote.load() >= 0 || proc.uiEnv.load() > 0.001f;
        int len = hzv > 15 ? juce::jlimit(64, N / 2, (int)std::round(2 * sr / hzv)) : 1024;
        // find a rising zero crossing so the trace is stable
        int start = (head - len - 1 + N) % N;
        for (int k = 0; k < juce::jmin(N - len - 2, (int)(sr / juce::jmax(20.0f, hzv)) + 2); k++) {
            const int i0 = (head - len - 2 - k + 2 * N) % N, i1 = (i0 + 1) % N;
            if (proc.scope[(size_t)i0].load() < 0 && proc.scope[(size_t)i1].load() >= 0) { start = i1; break; }
        }
        float pk = 0.05f; for (int k = 0; k < len; k++) pk = juce::jmax(pk, std::abs(proc.scope[(size_t)((start + k) % N)].load()));
        shownPk = juce::jmax(pk, shownPk * 0.92f);
        juce::Path path;
        for (int k = 0; k < len; k++) {
            const float v = proc.scope[(size_t)((start + k) % N)].load() / shownPk;
            const float x = a.getX() + a.getWidth() * k / (float)(len - 1), y = a.getCentreY() - v * a.getHeight() * 0.44f;
            if (k == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
        }
        g.setColour(accent.withAlpha(on ? 0.16f : 0.05f)); g.strokePath(path, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved));
        g.setColour(on ? accent.darker(0.15f) : faint); g.strokePath(path, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));
        // readout
        auto row = [&](const juce::String& k, const juce::String& v, int i) {
            auto rr = info.withTrimmedLeft(14).withY(info.getY() + 6 + i * 28.0f).withHeight(26);
            station::drawCaps(g, k, rr.removeFromTop(11), juce::Justification::centredLeft, 8.5f, dim);
            g.setFont(station::mono(13.0f, true)); g.setColour(ink); g.drawText(v, rr, juce::Justification::centredLeft);
        };
        const int note = proc.uiNote.load();
        row("Note", note >= 0 ? juce::MidiMessage::getMidiNoteName(note, true, true, 3) : juce::String("-"), 0);
        row("Pitch", hzv > 1 && on ? juce::String(hzv, 1) + " Hz" : juce::String("-"), 1);
        row("Cutoff", formatHz(proc.uiCut.load()), 2);
        row("Pattern", juce::StringArray{ "Off", "Repeat", "Arp", "Sequencer" }[juce::jlimit(0, 3, proc.ri("patmode"))], 3);
        g.setColour(hairline); g.drawVerticalLine((int)info.getX(), info.getY(), info.getBottom());
    }
private:
    static juce::String formatHz(float f) { return f >= 1000 ? juce::String(f / 1000.0f, 2) + " kHz" : juce::String(juce::roundToInt(f)) + " Hz"; }
    HazeProcessor& proc; juce::Colour accent; float shownPk = 0.1f;
};

// ---------------------------------------------------------------- tone curve: EQ bands plus the sub shelf
class ToneView : public juce::Component {
public:
    ToneView(HazeProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 8);
        const auto a = r.reduced(10, 8);
        FreqAxis ax; ax.lo = 20; ax.hi = 20000; ax.drawGrid(g, a, true);
        const double sr = 48000;
        Svf b[4];
        b[0].set(Svf::lowShelf, sr, proc.p("eqlowf"), 0.7, proc.p("eqlow"));
        b[1].set(Svf::bell, sr, proc.p("eqmidf"), 0.9, proc.p("eqmid"));
        b[2].set(Svf::highShelf, sr, proc.p("eqhighf"), 0.7, proc.p("eqhigh"));
        b[3].set(Svf::lowShelf, sr, proc.p("boostf"), 0.8, 9.0 * proc.p("subboost") / 100.0);
        auto y = [&](double db) { return a.getCentreY() - (float)(db / 15.0) * a.getHeight() * 0.5f; };
        g.setColour(hairline); g.drawHorizontalLine((int)y(0), a.getX(), a.getRight());
        juce::Path path;
        for (int k = 0; k <= 200; k++) {
            const float x = a.getX() + a.getWidth() * k / 200.0f; const double f = ax.f(x, a);
            double m = 1; for (auto& s : b) m *= s.magnitude(f, sr);
            const float yy = juce::jlimit(a.getY(), a.getBottom(), y(gainToDb(m)));
            if (k == 0) path.startNewSubPath(x, yy); else path.lineTo(x, yy);
        }
        juce::Path fill(path); fill.lineTo(a.getRight(), y(0)); fill.lineTo(a.getX(), y(0)); fill.closeSubPath();
        g.setColour(accent.withAlpha(0.12f)); g.fillPath(fill);
        g.setColour(accent); g.strokePath(path, juce::PathStrokeType(1.8f));
    }
private:
    HazeProcessor& proc; juce::Colour accent;
};

// ---------------------------------------------------------------- page tabs
class Tabs : public juce::Component {
public:
    Tabs(juce::Colour a) : accent(a) {}
    std::function<void(int)> onChange; int current = 0;
    juce::StringArray names{ "Synth", "Mod", "Pattern", "FX" };
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(station::well); g.fillRoundedRectangle(r, 8); g.setColour(hairline); g.drawRoundedRectangle(r.reduced(0.5f), 8, 1);
        const float w = r.getWidth() / names.size();
        for (int i = 0; i < names.size(); i++) {
            auto t = juce::Rectangle<float>(r.getX() + i * w, r.getY(), w, r.getHeight()).reduced(3);
            if (i == current) { station::drawShadow(g, t, 6, 0.6f); g.setColour(juce::Colours::white); g.fillRoundedRectangle(t, 6); g.setColour(accent); g.fillRoundedRectangle(t.removeFromBottom(2.5f).reduced(t.getWidth() * 0.3f, 0), 1.2f); }
            station::drawCaps(g, names[i], juce::Rectangle<float>(r.getX() + i * w, r.getY(), w, r.getHeight()), juce::Justification::centred, 10.0f, i == current ? ink : dim);
        }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        const int i = juce::jlimit(0, names.size() - 1, (int)(e.position.x / (getWidth() / (float)names.size())));
        if (i != current) { current = i; repaint(); if (onChange) onChange(i); }
    }
private:
    juce::Colour accent;
};

// ---------------------------------------------------------------- step sequencer grid
// Top row: step on/off. Middle: pitch (drag up or down, double-click to reset). Then accent and slide rows.
class StepGrid : public juce::Component {
public:
    StepGrid(HazeProcessor& p, juce::Colour a) : proc(p), accent(a) {}
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        station::drawWell(g, r, 10);
        geometry();
        const int len = proc.ri("seqlen"), cur = proc.uiStep.load();
        station::drawCaps(g, "Step", labelRect(0), juce::Justification::centredLeft, 8.5f, dim);
        station::drawCaps(g, "Pitch", labelRect(1), juce::Justification::centredLeft, 8.5f, dim);
        station::drawCaps(g, "Accent", labelRect(2), juce::Justification::centredLeft, 8.5f, dim);
        station::drawCaps(g, "Slide", labelRect(3), juce::Justification::centredLeft, 8.5f, dim);
        for (int i = 0; i < 16; i++) {
            const bool inLen = i < len, on = val(i, "on") > 0.5f;
            const float alpha = inLen ? 1.0f : 0.3f;
            auto col = colRect(i);
            if (i == cur) { g.setColour(accent.withAlpha(0.10f)); g.fillRoundedRectangle(col.expanded(1, 4), 5); }
            // on
            auto c0 = cell(i, 0); g.setColour((on ? accent : juce::Colours::white).withMultipliedAlpha(alpha)); g.fillRoundedRectangle(c0, 4);
            g.setColour(on ? accent.darker(0.2f) : hairline); g.drawRoundedRectangle(c0, 4, 1);
            g.setFont(station::mono(9.0f, true)); g.setColour((on ? juce::Colours::white : dim).withMultipliedAlpha(alpha)); g.drawText(juce::String(i + 1), c0, juce::Justification::centred);
            // pitch bar from the centre
            auto c1 = cell(i, 1); g.setColour(juce::Colours::white.withMultipliedAlpha(alpha)); g.fillRoundedRectangle(c1, 4); g.setColour(hairline); g.drawRoundedRectangle(c1, 4, 1);
            const float pv = val(i, "pitch") / 24.0f, cy = c1.getCentreY();
            g.setColour(hairline); g.drawHorizontalLine((int)cy, c1.getX() + 2, c1.getRight() - 2);
            auto bar = pv >= 0 ? juce::Rectangle<float>(c1.getX() + 4, cy - pv * (c1.getHeight() / 2 - 3), c1.getWidth() - 8, pv * (c1.getHeight() / 2 - 3))
                               : juce::Rectangle<float>(c1.getX() + 4, cy, c1.getWidth() - 8, -pv * (c1.getHeight() / 2 - 3));
            g.setColour((on ? accent : faint).withMultipliedAlpha(alpha * 0.85f)); g.fillRoundedRectangle(bar, 2);
            g.setFont(station::mono(9.0f)); g.setColour(ink.withMultipliedAlpha(alpha));
            const int st = juce::roundToInt(val(i, "pitch"));
            g.drawText(st == 0 ? juce::String("0") : (st > 0 ? "+" : "") + juce::String(st), c1.withHeight(14).translated(0, pv >= 0 ? c1.getHeight() - 16 : 2), juce::Justification::centred);
            // accent / slide
            for (int k = 2; k <= 3; k++) {
                auto c = cell(i, k); const bool lit = val(i, k == 2 ? "acc" : "slide") > 0.5f;
                g.setColour((lit ? (k == 2 ? ink : accent) : juce::Colours::white).withMultipliedAlpha(alpha)); g.fillRoundedRectangle(c, 4);
                g.setColour(lit ? juce::Colours::transparentBlack : hairline); g.drawRoundedRectangle(c, 4, 1);
                if (lit) { g.setColour(juce::Colours::white); g.setFont(station::mono(9.0f, true)); g.drawText(k == 2 ? "ACC" : "SLD", c, juce::Justification::centred); }
            }
        }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        geometry(); dragStep = -1;
        for (int i = 0; i < 16; i++) for (int k = 0; k < 4; k++) if (cell(i, k).contains(e.position)) {
            if (k == 1) { dragStep = i; dragStart = e.position.y; startVal = val(i, "pitch"); begin(i, "pitch"); return; }
            toggle(i, k == 0 ? "on" : k == 2 ? "acc" : "slide"); paintSet = val(i, k == 0 ? "on" : k == 2 ? "acc" : "slide") > 0.5f; paintRow = k; return;
        }
        paintRow = -1;
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragStep >= 0) { setVal(dragStep, "pitch", juce::jlimit(-24.0f, 24.0f, std::round(startVal + (dragStart - e.position.y) / 5.0f))); return; }
        if (paintRow >= 0) for (int i = 0; i < 16; i++) if (cell(i, paintRow).expanded(2, 30).contains(e.position)) {   // drag across to paint
            const char* id = paintRow == 0 ? "on" : paintRow == 2 ? "acc" : "slide";
            if ((val(i, id) > 0.5f) != paintSet) { begin(i, id); setVal(i, id, paintSet ? 1.0f : 0.0f); end(i, id); }
        }
    }
    void mouseUp(const juce::MouseEvent&) override { if (dragStep >= 0) end(dragStep, "pitch"); dragStep = -1; }
    void mouseDoubleClick(const juce::MouseEvent& e) override { geometry(); for (int i = 0; i < 16; i++) if (cell(i, 1).contains(e.position)) { begin(i, "pitch"); setVal(i, "pitch", 0); end(i, "pitch"); } }
private:
    void geometry() { area = getLocalBounds().toFloat().reduced(14, 12); area.removeFromLeft(56); }
    juce::Rectangle<float> labelRect(int row) const { auto c = cell(0, row); return { 14.0f, c.getY(), 56.0f, c.getHeight() }; }
    juce::Rectangle<float> colRect(int i) const { const float w = area.getWidth() / 16; return { area.getX() + i * w + 2, area.getY(), w - 4, area.getHeight() }; }
    juce::Rectangle<float> cell(int i, int row) const {
        auto c = colRect(i); const float h = area.getHeight();
        const float y0[4] = { 0, 0.16f, 0.78f, 0.89f }, hh[4] = { 0.13f, 0.59f, 0.09f, 0.09f };
        return { c.getX(), area.getY() + h * y0[row], c.getWidth(), h * hh[row] };
    }
    juce::RangedAudioParameter* prm(int i, const char* id) const { return proc.apvts.getParameter("s" + juce::String(i + 1) + id); }
    float val(int i, const char* id) const { return proc.pp("s" + juce::String(i + 1) + id); }
    void begin(int i, const char* id) { prm(i, id)->beginChangeGesture(); }
    void end(int i, const char* id) { prm(i, id)->endChangeGesture(); }
    void setVal(int i, const char* id, float v) { auto* q = prm(i, id); q->setValueNotifyingHost(q->convertTo0to1(v)); repaint(); }
    void toggle(int i, const char* id) { begin(i, id); setVal(i, id, val(i, id) > 0.5f ? 0.0f : 1.0f); end(i, id); }
    HazeProcessor& proc; juce::Colour accent; juce::Rectangle<float> area;
    int dragStep = -1, paintRow = -1; bool paintSet = false; float dragStart = 0, startVal = 0;
};

// ---------------------------------------------------------------- the editor
class HazeEditor : public Editor {
public:
    explicit HazeEditor(HazeProcessor& p) : Editor(p, "Haze", "Mono bass synthesizer", kAccent, 1240, 860), hp(p), scope(p, accent), tabs(accent), grid(p, accent), tone(p, accent), compHist(accent, 18.0f) {
        auto& s = p.apvts;
        auto D = [&](const char* id, const char* label) { dials.push_back(std::make_unique<Dial>(s, id, label, true)); byId[id] = dials.back().get(); return dials.back().get(); };
        auto S = [&](const char* id, juce::StringArray labels = {}) { segs.push_back(std::make_unique<Segments>(s, id, labels)); byId[id] = segs.back().get(); return segs.back().get(); };
        auto C = [&](const char* id, const char* label) { choices.push_back(std::make_unique<Choice>(s, id, label)); byId[id] = choices.back().get(); return choices.back().get(); };
        auto W = [&](const char* id, const char* label) { switches.push_back(std::make_unique<Switch>(s, id, label)); byId[id] = switches.back().get(); return switches.back().get(); };
        // synth page
        for (int o = 1; o <= 2; o++) {
            const std::string pre = "o" + std::to_string(o);
            static std::deque<std::string> keep;
            auto k = [&](const char* x) { keep.push_back(pre + x); return keep.back().c_str(); };
            pageAdd(0, { D(k("wave"), "Wave"), S(k("oct")), D(k("semi"), "Semi"), D(k("fine"), "Fine"), D(k("level"), "Level"), D(k("pw"), "Width"), D(k("voices"), "Voices"), D(k("detune"), "Detune"), D(k("spread"), "Spread") });
        }
        pageAdd(0, { W("sync", "Sync to 1"), D("fm", "FM 2>1"), W("phase", "Phase reset"), D("drift", "Drift"),
                     S("subwave"), S("suboct"), D("sublevel", "Sub"), W("subpost", "After filter"), S("noisetype"), D("noise", "Noise"),
                     C("ftype", "Type"), D("cutoff", "Cutoff"), D("res", "Resonance"), D("fdrive", "Drive"), D("keytrack", "Key track"), D("fenv", "Env amt"), D("fvel", "Velocity"), D("lowkeep", "Low keep"),
                     D("fa", "Attack"), D("fd", "Decay"), D("fs", "Sustain"), D("fr", "Release"),
                     D("aa", "Attack"), D("ad", "Decay"), D("as", "Sustain"), D("ar", "Release"), D("velamp", "Velocity"),
                     C("dirttype", "Type"), D("dirt", "Amount"), D("dirttone", "Tone"), D("dirtmix", "Mix"), D("basskeep", "Bass keep") });
        // mod page
        for (int k = 1; k <= 2; k++) {
            const std::string pre = "l" + std::to_string(k);
            static std::deque<std::string> keep;
            auto q = [&](const char* x) { keep.push_back(pre + x); return keep.back().c_str(); };
            pageAdd(1, { C(q("shape"), "Shape"), W(q("sync"), "Tempo sync"), D(q("rate"), "Rate"), C(q("div"), "Division"), W(q("retrig"), "Retrigger") });
        }
        pageAdd(1, { D("ma", "Attack"), D("md", "Decay"), D("ms", "Sustain"), D("mr", "Release"),
                     D("penv", "808 pitch"), D("penvtime", "Pitch time"), D("glide", "Glide"), S("glidemode"), S("glideshape", { "Exp", "Linear" }), S("trigger"), S("priority"), D("bend", "Bend") });
        for (int i = 1; i <= 8; i++) {
            const std::string pre = "m" + std::to_string(i);
            static std::deque<std::string> keep;
            auto q = [&](const char* x) { keep.push_back(pre + x); return keep.back().c_str(); };
            pageAdd(1, { C(q("src"), "Source"), C(q("dst"), "Destination"), D(q("amt"), "Amount") });
        }
        // pattern page
        pageAdd(2, { S("patmode"), C("patdiv", "Rate"), D("gate", "Gate"), D("swing", "Swing"), D("accent", "Accent"), D("seqlen", "Steps"), S("arpdir"), S("arpoct") });
        pages[2].push_back(&grid); body.addAndMakeVisible(grid);
        pages[3].push_back(&tone); body.addAndMakeVisible(tone); pages[3].push_back(&compHist); body.addAndMakeVisible(compHist);
        // fx page
        pageAdd(3, { D("eqlow", "Low"), D("eqlowf", "Low freq"), D("eqmid", "Mid"), D("eqmidf", "Mid freq"), D("eqhigh", "High"), D("eqhighf", "High freq"),
                     D("subboost", "Sub boost"), D("harm", "Harmonics"), D("boostf", "Freq"), D("comp", "Amount"),
                     D("chorus", "Mix"), D("chrate", "Rate"), D("chdepth", "Depth"), D("dmix", "Mix"), D("dfb", "Feedback"), C("ddiv", "Time"), W("pingpong", "Ping-pong"),
                     D("vmix", "Mix"), D("vsize", "Size"), D("width", "Width"), D("monobelow", "Mono below"), W("limiter", "Limiter"), D("volume", "Volume") });
        body.addAndMakeVisible(scope); body.addAndMakeVisible(tabs);
        // the main controls stay on every page
        mainCut = std::make_unique<Dial>(s, "cutoff", "Cutoff"); mainRes = std::make_unique<Dial>(s, "res", "Resonance");
        mainDirt = std::make_unique<Dial>(s, "dirt", "Dirt"); mainGlide = std::make_unique<Dial>(s, "glide", "Glide"); mainVol = std::make_unique<Dial>(s, "volume", "Volume");
        for (auto* d : { mainCut.get(), mainRes.get(), mainDirt.get(), mainGlide.get(), mainVol.get() }) { d->maxKnob = 58; body.addAndMakeVisible(*d); }
        if (auto* e = std::getenv("LGN_HAZE_PAGE")) hp.page = juce::jlimit(0, 3, juce::String(e).getIntValue());   // for the test snapshots
        tabs.current = hp.page;
        tabs.onChange = [this](int i) { hp.page = i; showPage(); resized(); body.repaint(); };
        showPage();
        setReady();
    }
protected:
    void pageAdd(int page, std::initializer_list<juce::Component*> cs) { for (auto* c : cs) { pages[(size_t)page].push_back(c); body.addChildComponent(c); } }
    void showPage() { for (int k = 0; k < 4; k++) for (auto* c : pages[(size_t)k]) c->setVisible(k == tabs.current); }
    juce::Component* get(const char* id) { return byId.at(id); }
    void row(juce::Rectangle<int> r, std::initializer_list<const char*> ids) {
        const int w = r.getWidth() / (int)ids.size();
        for (auto* id : ids) get(id)->setBounds(r.removeFromLeft(w));
    }
    static juce::Rectangle<int> inner(juce::Rectangle<int> a) { return a.reduced(12, 8).withTrimmedTop(station::kModuleTitle - 10); }
    void layout(juce::Rectangle<int> r) override {
        mods.clear(); labelRects.clear();
        auto top = r.removeFromTop(176); r.removeFromTop(10);
        auto mainA = top.removeFromRight(360); top.removeFromRight(10);
        scope.setBounds(top); mainArea = mainA;
        {   auto a = mainA.reduced(10, 8).withTrimmedTop(station::kModuleTitle - 8); const int w = a.getWidth() / 5;
            for (auto* d : { mainCut.get(), mainRes.get(), mainDirt.get(), mainGlide.get(), mainVol.get() }) d->setBounds(a.removeFromLeft(w)); }
        tabs.setBounds(r.removeFromTop(34).withSizeKeepingCentre(520, 34)); r.removeFromTop(10);
        const int page = tabs.current;
        if (page == 0) {
            auto r1 = r.removeFromTop(r.getHeight() / 2 - 5); r.removeFromTop(10);
            auto o1 = r1.removeFromLeft(400); r1.removeFromLeft(10); auto o2 = r1.removeFromLeft(400); r1.removeFromLeft(10); auto sn = r1;
            for (int o = 0; o < 2; o++) {
                const auto box = o == 0 ? o1 : o2; mods.push_back({ box, o == 0 ? "Oscillator 1" : "Oscillator 2", o == 0 ? "HZ-01" : "HZ-02" });
                auto a = inner(box); const std::string pre = "o" + std::to_string(o + 1);
                auto octRow = a.removeFromTop(28); get((pre + "oct").c_str())->setBounds(octRow.removeFromLeft(200));
                if (o == 1) { octRow.removeFromLeft(10); get("sync")->setBounds(octRow.removeFromLeft(100)); }
                else { octRow.removeFromLeft(10); get("phase")->setBounds(octRow.removeFromLeft(120)); }
                a.removeFromTop(4);
                const int h = a.getHeight() / 2;
                rowS(a.removeFromTop(h), { pre + "wave", pre + "semi", pre + "fine", pre + "level", o == 1 ? std::string("fm") : std::string("drift") });
                rowS(a, { pre + "pw", pre + "voices", pre + "detune", pre + "spread" });
            }
            mods.push_back({ sn, "Sub + Noise", "HZ-03" });
            {   auto a = inner(sn);
                get("subwave")->setBounds(a.removeFromTop(28)); a.removeFromTop(6);
                auto rr = a.removeFromTop(28); get("suboct")->setBounds(rr.removeFromLeft(rr.getWidth() / 2 - 4)); rr.removeFromLeft(8); get("subpost")->setBounds(rr);
                a.removeFromTop(6); get("noisetype")->setBounds(a.removeFromTop(28)); a.removeFromTop(2);
                row(a, { "sublevel", "noise" }); }
            auto f = r.removeFromLeft(400); r.removeFromLeft(10); auto fe = r.removeFromLeft(210); r.removeFromLeft(10);
            auto ae = r.removeFromLeft(260); r.removeFromLeft(10); auto dt = r;
            mods.push_back({ f, "Filter", "HZ-04" }); mods.push_back({ fe, "Filter Env", "HZ-05" }); mods.push_back({ ae, "Amp", "HZ-06" }); mods.push_back({ dt, "Dirt", "HZ-07" });
            {   auto a = inner(f); get("ftype")->setBounds(a.removeFromTop(48).removeFromLeft(200)); const int h = a.getHeight() / 2;
                row(a.removeFromTop(h), { "cutoff", "res", "fdrive", "lowkeep" }); row(a, { "fenv", "keytrack", "fvel" }); }
            {   auto a = inner(fe); const int h = a.getHeight() / 2; row(a.removeFromTop(h), { "fa", "fd" }); row(a, { "fs", "fr" }); }
            {   auto a = inner(ae); const int h = a.getHeight() / 2; row(a.removeFromTop(h), { "aa", "ad", "velamp" }); row(a.withTrimmedRight(a.getWidth() / 3), { "as", "ar" }); }
            {   auto a = inner(dt); get("dirttype")->setBounds(a.removeFromTop(48)); const int h = a.getHeight() / 2;
                row(a.removeFromTop(h), { "dirt", "dirttone" }); row(a, { "dirtmix", "basskeep" }); }
        } else if (page == 1) {
            auto r1 = r.removeFromTop(250); r.removeFromTop(10);
            for (int k = 0; k < 2; k++) {
                auto box = r1.removeFromLeft(250); r1.removeFromLeft(10); const std::string pre = "l" + std::to_string(k + 1);
                mods.push_back({ box, k == 0 ? "LFO 1" : "LFO 2", k == 0 ? "HZ-08" : "HZ-09" });
                auto a = inner(box);
                auto rr = a.removeFromTop(48); get((pre + "shape").c_str())->setBounds(rr.removeFromLeft(rr.getWidth() / 2 - 4)); rr.removeFromLeft(8); get((pre + "div").c_str())->setBounds(rr);
                a.removeFromTop(6); auto sw = a.removeFromTop(26); get((pre + "sync").c_str())->setBounds(sw.removeFromLeft(sw.getWidth() / 2)); get((pre + "retrig").c_str())->setBounds(sw);
                get((pre + "rate").c_str())->setBounds(a);
            }
            auto me = r1.removeFromLeft(220); r1.removeFromLeft(10); auto pt = r1;
            mods.push_back({ me, "Mod Env", "HZ-10" }); mods.push_back({ pt, "Pitch + Play", "HZ-11" });
            {   auto a = inner(me); const int h = a.getHeight() / 2; row(a.removeFromTop(h), { "ma", "md" }); row(a, { "ms", "mr" }); }
            {   auto a = inner(pt); auto left = a.removeFromLeft(a.getWidth() / 2); a.removeFromLeft(10);
                const int h = left.getHeight() / 2; row(left.removeFromTop(h), { "penv", "penvtime" }); row(left, { "glide", "bend" });
                for (auto* id : { "glidemode", "glideshape", "trigger", "priority" }) { labelRects.push_back({ a.removeFromTop(14), juce::String(id) }); get(id)->setBounds(a.removeFromTop(28)); a.removeFromTop(8); } }
            mods.push_back({ r, "Modulation Matrix", "HZ-12" });
            {   auto a = inner(r); auto top2 = a.removeFromTop(a.getHeight() / 2 - 4); a.removeFromTop(8);
                for (int i = 0; i < 8; i++) {
                    auto& line = i < 4 ? top2 : a; auto cellR = line.removeFromLeft((i % 4 == 3) ? line.getWidth() : line.getWidth() / (4 - i % 4)).reduced(6, 0);
                    const std::string pre = "m" + std::to_string(i + 1);
                    slotRects[(size_t)i] = cellR;
                    auto cc = cellR.withTrimmedTop(16);
                    auto right = cc.removeFromRight(86);
                    get((pre + "src").c_str())->setBounds(cc.removeFromTop(cc.getHeight() / 2)); get((pre + "dst").c_str())->setBounds(cc);
                    get((pre + "amt").c_str())->setBounds(right);
                } }
        } else if (page == 2) {
            auto ctl = r.removeFromTop(150); r.removeFromTop(10);
            mods.push_back({ ctl, "Pattern", "HZ-13" }); mods.push_back({ r, "Steps", "HZ-14" });
            {   auto a = inner(ctl); auto left = a.removeFromLeft(640); a.removeFromLeft(16);
                labelRects.push_back({ left.removeFromTop(14), "patmode" }); get("patmode")->setBounds(left.removeFromTop(30).withWidth(440)); left.removeFromTop(10);
                auto rr = left; get("patdiv")->setBounds(rr.removeFromLeft(130).withHeight(48)); rr.removeFromLeft(14);
                auto dir = rr.removeFromLeft(330); labelRects.push_back({ dir.removeFromTop(14), "arpdir" }); get("arpdir")->setBounds(dir.removeFromTop(30)); rr.removeFromLeft(14);
                labelRects.push_back({ rr.removeFromTop(14), "arpoct" }); get("arpoct")->setBounds(rr.removeFromTop(30));
                row(a, { "gate", "swing", "accent", "seqlen" }); }
            grid.setBounds(inner(r));
        } else {
            auto r1 = r.removeFromTop(r.getHeight() / 2 - 5); r.removeFromTop(10);
            auto eqA = r1.removeFromLeft(560); r1.removeFromLeft(10); auto bb = r1.removeFromLeft(330); r1.removeFromLeft(10); auto cp = r1;
            mods.push_back({ eqA, "EQ", "HZ-15" }); mods.push_back({ bb, "Bass Boost", "HZ-16" }); mods.push_back({ cp, "Compressor", "HZ-17" });
            {   auto a = inner(eqA); tone.setBounds(a.removeFromTop(a.getHeight() - 112)); a.removeFromTop(6); row(a, { "eqlow", "eqlowf", "eqmid", "eqmidf", "eqhigh", "eqhighf" }); }
            {   auto a = inner(bb); a.removeFromTop((a.getHeight() - 112) / 2); row(a.removeFromTop(112), { "subboost", "harm", "boostf" }); }
            {   auto a = inner(cp); compHist.setBounds(a.removeFromTop(a.getHeight() - 112)); a.removeFromTop(6); row(a, { "comp" }); }
            auto ch = r.removeFromLeft(300); r.removeFromLeft(10); auto dl = r.removeFromLeft(370); r.removeFromLeft(10); auto rv = r.removeFromLeft(200); r.removeFromLeft(10); auto out = r;
            mods.push_back({ ch, "Chorus", "HZ-18" }); mods.push_back({ dl, "Delay", "HZ-19" }); mods.push_back({ rv, "Reverb", "HZ-20" }); mods.push_back({ out, "Output", "HZ-21" });
            row(inner(ch), { "chorus", "chrate", "chdepth" });
            {   auto a = inner(dl); auto sw = a.removeFromTop(24); get("pingpong")->setBounds(sw.removeFromLeft(120)); row(a, { "dmix", "dfb", "ddiv" });
                get("ddiv")->setBounds(get("ddiv")->getBounds().withSizeKeepingCentre(get("ddiv")->getWidth() - 8, 50)); }
            row(inner(rv), { "vmix", "vsize" });
            {   auto a = inner(out); auto sw = a.removeFromTop(24); get("limiter")->setBounds(sw.removeFromLeft(110)); row(a, { "width", "monobelow", "volume" }); }
        }
    }
    void rowS(juce::Rectangle<int> r, std::vector<std::string> ids) { const int w = r.getWidth() / (int)ids.size(); for (auto& id : ids) get(id.c_str())->setBounds(r.removeFromLeft(w)); }
    void paintBody(juce::Graphics& g) override {
        module(g, mainArea, "Main", "HZ-00");
        for (auto& m : mods) module(g, m.r, m.title, m.code);
        static const std::map<juce::String, juce::String> names{ { "glidemode", "Glide" }, { "glideshape", "Glide curve" }, { "trigger", "Envelopes" }, { "priority", "Note priority" },
            { "patmode", "Mode" }, { "arpdir", "Arp direction" }, { "arpoct", "Arp octaves" } };
        for (auto& l : labelRects) station::drawCaps(g, names.at(l.second), l.first.toFloat(), juce::Justification::centredLeft, 8.5f, dim);
        if (tabs.current == 1) for (int i = 0; i < 8; i++) station::drawCaps(g, "Slot " + juce::String(i + 1), slotRects[(size_t)i].withHeight(14).toFloat(), juce::Justification::centredLeft, 8.5f,
                                                                    juce::roundToInt(hp.p(("m" + std::to_string(i + 1) + "src").c_str())) > 0 && juce::roundToInt(hp.p(("m" + std::to_string(i + 1) + "dst").c_str())) > 0 ? accent : dim);
    }
    void tick() override {
        scope.repaint();
        if (tabs.current == 2) grid.repaint();
        if (tabs.current == 3) { tone.repaint(); compHist.push(hp.uiOut.load(), hp.uiGr.load()); }
        // dim what is not in use
        auto fade = [&](const char* id, bool on) { if (auto it = byId.find(id); it != byId.end()) it->second->setAlpha(on ? 1.0f : 0.38f); };
        for (int o = 1; o <= 2; o++) {
            const std::string pre = "o" + std::to_string(o);
            const bool uni = juce::roundToInt(hp.p((pre + "voices").c_str())) > 1;
            fade((pre + "detune").c_str(), uni); fade((pre + "spread").c_str(), uni);
            fade((pre + "pw").c_str(), hp.p((pre + "wave").c_str()) > 2.0f || hp.slotTargets(HzDst::pulseWidth));
        }
        for (int k = 1; k <= 2; k++) { const std::string pre = "l" + std::to_string(k); const bool sy = hp.p((pre + "sync").c_str()) > 0.5f; fade((pre + "rate").c_str(), !sy); fade((pre + "div").c_str(), sy); }
        const int pm = hp.ri("patmode");
        fade("patdiv", pm > 0); fade("gate", pm > 0); fade("swing", pm > 0); fade("accent", pm == 3); fade("seqlen", pm == 3); fade("arpdir", pm == 2); fade("arpoct", pm == 2);
        grid.setAlpha(pm == 3 ? 1.0f : 0.5f);
        fade("glideshape", hp.ri("glidemode") > 0);
        const bool dirtOn = hp.p("dirt") > 0.05f; for (auto* id : { "dirttone", "dirtmix", "basskeep" }) fade(id, dirtOn);
        fade("lowkeep", hp.ri("ftype") <= 1);
    }
private:
    struct Mod { juce::Rectangle<int> r; juce::String title, code; };
    HazeProcessor& hp; Scope scope; Tabs tabs; StepGrid grid; ToneView tone; History compHist;
    std::vector<std::unique_ptr<Dial>> dials; std::vector<std::unique_ptr<Segments>> segs; std::vector<std::unique_ptr<Choice>> choices; std::vector<std::unique_ptr<Switch>> switches;
    std::unique_ptr<Dial> mainCut, mainRes, mainDirt, mainGlide, mainVol;
    std::map<std::string, juce::Component*> byId;
    std::array<std::vector<juce::Component*>, 4> pages;
    std::vector<Mod> mods; juce::Rectangle<int> mainArea;
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> labelRects; 
    std::array<juce::Rectangle<int>, 8> slotRects;
};

juce::AudioProcessorEditor* HazeProcessor::createEditor() { return new HazeEditor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HazeProcessor(); }
