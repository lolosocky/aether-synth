// LGN Haze: a monophonic bass synthesizer.
//
// Signal path (the core runs at twice the host rate, then a halfband filter brings it down, so the oscillators,
// filter and drive do not alias):
//   Osc 1 + Osc 2 (wave morph sine > triangle > saw > pulse, up to 7 unison voices, sync, FM, drift)
//   + Sub (sine / triangle / square / pulse, 1 or 2 octaves down, before or after the filter) + Noise
//   > Filter (ladder 24 / 12, state-variable LP / BP / HP / notch, comb, vowel), with a low-end keeper
//   > Dirt (tube, tape, hard clip, fold, rectify, crush) with a bass-keep crossover so the sub stays clean
//   > Amp
//   then at the host rate: EQ > Bass Boost (sub shelf + harmonics, so 808s read on phone speakers)
//   > Compressor > Chorus > Delay > Reverb > Width (mono below a frequency) > Limiter > Volume.
// Control: amp / filter / mod envelopes, an 808 pitch envelope, two LFOs, an 8-slot modulation matrix,
// glide (exponential or linear, always or legato), note priority, and a pattern section:
// note repeat, arpeggiator, or a 16-step sequencer with accent and slide.
#pragma once
#include "Common.h"
#include "Comp.h"
#include "Delay.h"
#include "Reverb.h"

namespace lgn {

// ---------------------------------------------------------------- parameters
enum class HzSrc { off, lfo1, lfo2, filterEnv, modEnv, ampEnv, velocity, note, modWheel, aftertouch, pitchBend, random, count };
enum class HzDst { off, pitch, osc1Pitch, osc2Pitch, osc1Wave, osc2Wave, pulseWidth, fm, detune, osc1Level, osc2Level, subLevel, noiseLevel,
                   cutoff, resonance, filterDrive, dirt, amp, pan, lfo1Rate, lfo2Rate, count };
inline const char* const* hzSrcNames() { static const char* n[] = { "Off", "LFO 1", "LFO 2", "Filter Env", "Mod Env", "Amp Env", "Velocity", "Note", "Mod Wheel", "Aftertouch", "Pitch Bend", "Random" }; return n; }
inline const char* const* hzDstNames() { static const char* n[] = { "Off", "Pitch", "Osc 1 Pitch", "Osc 2 Pitch", "Osc 1 Wave", "Osc 2 Wave", "Pulse Width", "FM", "Detune",
    "Osc 1 Level", "Osc 2 Level", "Sub Level", "Noise Level", "Cutoff", "Resonance", "Filter Drive", "Dirt", "Amp", "Pan", "LFO 1 Rate", "LFO 2 Rate" }; return n; }

// Pattern step lengths, in beats.
inline const std::vector<std::pair<const char*, double>>& hzDivs() {
    static const std::vector<std::pair<const char*, double>> d = { { "1/64", 1.0 / 16 }, { "1/32", 0.125 }, { "1/16T", 1.0 / 6 }, { "1/16", 0.25 }, { "1/16D", 0.375 },
        { "1/8T", 1.0 / 3 }, { "1/8", 0.5 }, { "1/8D", 0.75 }, { "1/4T", 2.0 / 3 }, { "1/4", 1.0 }, { "1/4D", 1.5 }, { "1/2", 2.0 }, { "1 Bar", 4.0 }, { "2 Bars", 8.0 }, { "4 Bars", 16.0 } };
    return d;
}

struct HzOsc { double wave = 2, octave = 0, semi = 0, fine = 0, level = 1, pw = 0.5; int voices = 1; double detune = 0.2, spread = 0.5; };
struct HzEnv { double a = 0.002, d = 0.3, s = 0.7, r = 0.2; };
struct HzLfo { int shape = 0; bool sync = true; double hz = 2; int div = 6; bool retrig = false; };
struct HzSlot { int src = 0, dst = 0; double amt = 0; };
struct HzStep { bool on = true; int pitch = 0; bool accent = false, slide = false; };

struct HazeParams {
    HzOsc osc1, osc2{ 2, 0, 0, 7, 0, 0.5, 1, 0.2, 0.5 };
    bool sync = false; double fm = 0; bool phaseReset = true; double drift = 0.15;
    int subWave = 0, subOct = 1; double subLevel = 0; bool subPostFilter = false;
    int noiseType = 0; double noiseLevel = 0;
    // filter
    int fType = 0; double cutoff = 2000, res = 0.15, fDrive = 0, keyTrack = 0.5, envAmt = 24, fVel = 0, lowKeep = 0.5;
    // dirt
    int dirtType = 0; double dirt = 0, dirtTone = 0.5, dirtMix = 1, bassKeep = 0.6;
    // envelopes
    HzEnv ampEnv{ 0.002, 0.4, 0.8, 0.15 }, fEnv{ 0.002, 0.35, 0.2, 0.25 }, mEnv{ 0.01, 0.5, 0, 0.3 };
    double velAmp = 0.5;
    // pitch
    double pEnvAmt = 0, pEnvTime = 0.12, glide = 0.06; int glideMode = 0, glideShape = 0;   // glideMode 0 off, 1 always, 2 legato
    int trigger = 0, priority = 0;   // trigger 0 retrigger, 1 legato; priority 0 last, 1 low, 2 high
    double bend = 2;
    HzLfo lfo1, lfo2{ 0, false, 0.3, 6, false };
    std::array<HzSlot, 8> slots{};
    // pattern
    int patMode = 0, patDiv = 3, arpDir = 0, arpOct = 1, seqLen = 16; double gate = 0.6, swing = 0, accent = 0.5;
    std::array<HzStep, 16> steps{};
    // effects
    double eqLow = 0, eqLowHz = 100, eqMid = 0, eqMidHz = 800, eqHigh = 0, eqHighHz = 6000;
    double subBoost = 0, harmonics = 0, boostHz = 60;
    double comp = 0, chorus = 0, chorusRate = 0.6, chorusDepth = 0.5;
    double delayMix = 0, delayFb = 0.3; int delayDiv = 6; bool pingPong = true;
    double verbMix = 0, verbSize = 0.5;
    double width = 1, monoBelow = 120; bool limiter = true; double volumeDb = 0;
};

// ---------------------------------------------------------------- building blocks
namespace hz {
inline double blep(double t, double dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1; }
    if (t > 1 - dt) { t = (t - 1) / dt; return t * t + t + t + 1; }
    return 0;
}
inline double blamp(double t, double dt) {
    if (t < dt) { t = t / dt - 1; return -t * t * t / 3.0; }
    if (t > 1 - dt) { t = (t - 1) / dt + 1; return t * t * t / 3.0; }
    return 0;
}
inline double frac(double x) { return x - std::floor(x); }
// One band-limited shape: 0 sine, 1 triangle, 2 saw, 3 pulse (width pw, DC removed).
inline double shape(int k, double t, double dt, double pw) {
    switch (k) {
        case 0: return std::sin(2 * kPi * t);
        case 1: { const double u = frac(t + 0.25); return 1 - 4 * std::abs(u - 0.5) + 4 * dt * (blamp(u, dt) - blamp(frac(t + 0.75), dt)); }
        case 2: return 2 * t - 1 - blep(t, dt);
        default: { const double v = (t < pw ? 1.0 : -1.0) + blep(t, dt) - blep(frac(t + 1 - pw), dt); return v - (2 * pw - 1); }
    }
}
// Morph through the four shapes (0..3, continuous).
inline double morph(double m, double t, double dt, double pw) {
    m = clampv(m, 0.0, 3.0);
    const int i = std::min(2, (int)m); const double f = m - i;
    if (f < 1e-4) return shape(i, t, dt, pw);
    if (f > 1 - 1e-4) return shape(i + 1, t, dt, pw);
    return shape(i, t, dt, pw) * (1 - f) + shape(i + 1, t, dt, pw) * f;
}
inline double noteHz(double n) { return 440.0 * std::exp2((n - 69.0) / 12.0); }

struct Rng { uint32_t s = 0x9E3779B9u; double next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s / 4294967296.0) * 2 - 1; } };

// Analog-style ADSR (exponential segments; attack aims past 1 so it curves like an RC charge).
struct Adsr {
    enum Stage { idle, attack, decay, sustain, release } stage = idle;
    double v = 0, aC = 0, aB = 0, dC = 0, dB = 0, rC = 0, rB = 0, s = 0;
    static void coef(double secs, double sr, double ratio, double target, double& c, double& b, bool up) {
        const double n = std::max(1.0, secs * sr);
        c = std::exp(-std::log((1 + ratio) / ratio) / n);
        b = up ? (target + ratio) * (1 - c) : (target - ratio) * (1 - c);
    }
    void set(const HzEnv& e, double sr) {
        s = clampv(e.s, 0.0, 1.0);
        coef(std::max(e.a, 0.0003), sr, 0.3, 1.0, aC, aB, true);
        coef(std::max(e.d, 0.001), sr, 0.0001, s, dC, dB, false);
        coef(std::max(e.r, 0.001), sr, 0.0001, 0.0, rC, rB, false);
    }
    void gateOn() { stage = attack; }
    void gateOff() { if (stage != idle) stage = release; }
    double next() {
        switch (stage) {
            case attack: v = aB + v * aC; if (v >= 1) { v = 1; stage = decay; } break;
            case decay: v = dB + v * dC; if (v <= s) { v = s; stage = sustain; } break;
            case sustain: v = s; break;
            case release: v = rB + v * rC; if (v <= 1e-5) { v = 0; stage = idle; } break;
            default: break;
        }
        return v;
    }
    void advance(int n) { for (int i = 0; i < n; i++) next(); }
};

// Zero-delay-feedback ladder (four TPT one-poles) with a saturating input and bass compensation.
struct Ladder {
    double s[4] = {};
    void reset() { for (auto& x : s) x = 0; }
    double process(double x, double G, double k, double keep, bool two) {
        const double G2 = G * G, G4 = G2 * G2;
        const double b0 = (1 - G) * s[0], b1 = (1 - G) * s[1], b2 = (1 - G) * s[2], b3 = (1 - G) * s[3];
        const double S = G * G2 * b0 + G2 * b1 + G * b2 + b3;
        double u = (x * (1 + keep * k) - k * S) / (1 + k * G4);
        u = std::tanh(u);
        double y = u;
        double out2 = 0;
        for (int i = 0; i < 4; i++) {
            const double v = (y - s[i]) * G; const double o = v + s[i]; s[i] = o + v; y = o;
            if (i == 1) out2 = o;
        }
        return two ? out2 : y;
    }
};

// Simper SVF core with per-sample coefficient (g, k), giving LP / BP / HP.
struct Svf2 {
    double ic1 = 0, ic2 = 0;
    void reset() { ic1 = ic2 = 0; }
    void tick(double v0, double g, double k, double& lp, double& bp, double& hp) {
        const double a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const double v3 = v0 - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }
};

struct OnePole { double z = 0, a = 0; void set(double sr, double hz) { a = std::exp(-2 * kPi * clampv(hz, 1.0, sr * 0.49) / sr); } double lp(double x) { z = x + (z - x) * a; return z; } };

inline double dirtShape(int type, double x) {
    switch (type) {
        case 0: return std::tanh(x + 0.25 * x * x);                                                              // tube: asymmetric, even harmonics
        case 1: return std::atan(x * 1.4) * (2 / kPi) * 1.15;                                                   // tape: soft
        case 2: return clampv(x, -1.0, 1.0);                                                                     // hard clip
        case 3: return std::sin(x * 1.5);                                                                        // fold
        case 4: return std::tanh(std::abs(x) * 1.2 + 0.35 * x) - 0.3;                                           // rectify: octave-up grit
        default: return x;                                                                                       // crush handled by caller
    }
}
} // namespace hz

// ---------------------------------------------------------------- the synth
class HazeDsp {
public:
    static constexpr int kMaxVoices = 7, kCtl = 16;   // control rate: every 16 samples of the 2x core
    struct View { int note = -1, step = -1; double env = 0, hz = 0, cutoffHz = 1000; };

    void prepare(double rate) {
        sr = rate; fs = 2 * sr;
        for (auto& o : os) o.reset();
        allNotesOff(); hardReset();
        comp.prepare(sr); delay.prepare(sr); verb.prepare(sr);
        for (auto& c : chorusLine) c.prepare((int)(0.04 * sr) + 8);
        volS.setTime(sr, 0.03); volS.reset(dbToGain(p.volumeDb));
        limGain = 1; limAtt = std::exp(-1.0 / (0.001 * sr)); ctl = 0; fxCounter = 0;
        updateFx(true);
    }
    void set(const HazeParams& np) { p = np; }
    void setTransport(double ppq, double bpmNow, bool playing) { hostPpq = ppq; bpm = clampv(bpmNow, 20.0, 999.0); hostPlaying = playing; }

    // MIDI (offset = sample position in the next block)
    void noteOn(int note, double vel, int offset) { queue.push_back({ offset, 0, note, vel }); }
    void noteOff(int note, int offset) { queue.push_back({ offset, 1, note, 0 }); }
    void pitchBend(double v) { bendV = clampv(v, -1.0, 1.0); }
    void modWheel(double v) { wheel = clampv(v, 0.0, 1.0); }
    void aftertouch(double v) { touch = clampv(v, 0.0, 1.0); }
    void allNotesOff() { keys.clear(); queue.clear(); voiceRelease(); patternNoteOff(); }

    View view() const { return ui; }
    double compReductionDb() const { return p.comp > 0.001 ? comp.reductionDb() : 0.0; }
    // a ring of the latest output (left), for the oscilloscope
    static constexpr int kScope = 4096;
    std::array<float, kScope> scope{}; int scopePos = 0;

    void process(float* L, float* R, int n) {
        std::stable_sort(queue.begin(), queue.end(), [](const Ev& a, const Ev& b) { return a.off < b.off; });
        size_t qi = 0;
        const double beatsPerSample = bpm / 60.0 / sr;
        for (int i = 0; i < n; i++) {
            clock = hostPlaying ? hostPpq + i * beatsPerSample : clock + beatsPerSample;
            while (qi < queue.size() && queue[qi].off <= i) { handle(queue[qi]); qi++; ctl = 0; }
            if (p.patMode > 0) pattern();
            if (ctl == 0) control();
            ctl = (ctl + 2) % kCtl;
            double l0, r0, l1, r1;
            core(l0, r0); core(l1, r1);
            L[i] = (float)os[0].downsample(l0, l1); R[i] = (float)os[1].downsample(r0, r1);
        }
        queue.clear();
        effects(L, R, n);
        for (int i = 0; i < n; i++) { scope[(size_t)scopePos] = L[i]; scopePos = (scopePos + 1) % kScope; }
        ui.note = sounding ? curNote : -1; ui.env = amp.v; ui.hz = hz::noteHz(pitchNow); ui.step = p.patMode == 3 && !keys.empty() ? seqStepShown : -1;
    }

private:
    struct Ev { int off, kind, note; double vel; };
    struct Key { int note; double vel; uint64_t order; };

    // ------------------------------------------------ notes and pattern
    void handle(const Ev& e) {
        if (e.kind == 0) {
            const bool first = keys.empty();
            keys.erase(std::remove_if(keys.begin(), keys.end(), [&](const Key& k) { return k.note == e.note; }), keys.end());
            keys.push_back({ e.note, e.vel, orderCounter++ });
            if (p.patMode == 0) { const auto& k = pickKey(); voiceOn(k.note, k.vel, !first); }
            else if (first) { patternStart(); }
            lastVel = e.vel;
        } else {
            keys.erase(std::remove_if(keys.begin(), keys.end(), [&](const Key& k) { return k.note == e.note; }), keys.end());
            if (p.patMode == 0) {
                if (keys.empty()) voiceRelease();
                else { const auto& k = pickKey(); if (k.note != curNote) voiceOn(k.note, k.vel, true); }
            } else if (keys.empty()) patternNoteOff();
        }
    }
    const Key& pickKey() const {
        const Key* best = &keys.back();
        for (auto& k : keys) {
            if (p.priority == 1 && k.note < best->note) best = &k;
            if (p.priority == 2 && k.note > best->note) best = &k;
        }
        return *best;
    }
    // overlapped: another key was already down (a legato move)
    void voiceOn(int note, double vel, bool overlapped, bool forceSlide = false, bool accentOn = false) {
        const bool legatoMove = overlapped && sounding;
        const bool glideNow = forceSlide || (p.glideMode == 1 && hasPlayed) || (p.glideMode == 2 && legatoMove);
        const bool retrig = !(forceSlide || (legatoMove && p.trigger == 1));
        slideGlide = forceSlide;
        curNote = note; targetPitch = note;
        if (!glideNow) { pitchNow = note; glideFrom = note; }
        else { glideFrom = pitchNow; glideT = 0; }
        accentLevel = accentOn ? 1.0 : 0.0;
        if (retrig) {
            velocity = vel; noteRandom = rng.next();
            amp.gateOn(); fEnv.gateOn(); mEnv.gateOn();
            penvT = 0;
            if (!sounding || amp.v < 0.02) resetPhases();
            if (p.lfo1.retrig) lfoPh[0] = 0;
            if (p.lfo2.retrig) lfoPh[1] = 0;
        }
        sounding = true; hasPlayed = true;
    }
    void voiceRelease() { amp.gateOff(); fEnv.gateOff(); mEnv.gateOff(); sounding = false; }
    void resetPhases() {
        if (!p.phaseReset) return;
        for (int o = 0; o < 2; o++) for (int v = 0; v < kMaxVoices; v++) ph[o][(size_t)v] = v == 0 ? 0.0 : hz::frac(v * 0.6180339887 + o * 0.25);
        subPh = 0;
    }

    double stepBeats() const { return hzDivs()[(size_t)clampv(p.patDiv, 0, (int)hzDivs().size() - 1)].second; }
    double stepStart(int64_t k) const { return k * stepBeats() + ((k & 1) ? p.swing * 0.33 * stepBeats() : 0.0); }
    void patternStart() {
        // the first key: play the current step now, then stay on the grid
        if (!hostPlaying) clock = 0;
        stepK = (int64_t)std::floor(clock / stepBeats() + 1e-9);
        arpIdx = 0;
        fireStep(clock, true);
        nextStep = stepStart(stepK + 1);
        lastBeat = clock;
    }
    void patternNoteOff() { if (sounding) voiceRelease(); gateOffAt = -1; prevSlid = false; }
    void pattern() {
        if (keys.empty()) return;
        const double beat = clock;
        if (beat < lastBeat - 1e-6 || beat > nextStep + 4 * stepBeats()) {   // transport looped or jumped: resync
            stepK = (int64_t)std::floor(beat / stepBeats()) - 1; nextStep = stepStart(stepK + 1); gateOffAt = -1;
        }
        lastBeat = beat;
        if (gateOffAt >= 0 && beat >= gateOffAt) { voiceRelease(); gateOffAt = -1; }
        if (beat >= nextStep) { stepK++; fireStep(beat, false); nextStep = stepStart(stepK + 1); }
    }
    void fireStep(double beat, bool first) {
        const double L = stepBeats();
        const double start = first ? beat : stepStart(stepK);
        if (p.patMode == 1 || p.patMode == 2) {   // note repeat / arp
            int note; double vel;
            if (p.patMode == 1) { const auto& k = pickKey(); note = k.note; vel = k.vel; }
            else {
                std::vector<Key> ks = keys;
                if (p.arpDir != 5) std::sort(ks.begin(), ks.end(), [](const Key& a, const Key& b) { return a.note < b.note; });
                std::vector<int> seq;
                const int oct = clampv(p.arpOct, 1, 4);
                for (int o = 0; o < oct; o++) for (auto& k : ks) seq.push_back(k.note + 12 * o);
                std::vector<int> order;
                const int m = (int)seq.size();
                switch (p.arpDir) {
                    case 1: for (int i = m - 1; i >= 0; i--) order.push_back(seq[(size_t)i]); break;
                    case 2: for (int i = 0; i < m; i++) order.push_back(seq[(size_t)i]); for (int i = m - 2; i > 0; i--) order.push_back(seq[(size_t)i]); break;
                    case 3: for (int i = m - 1; i >= 0; i--) order.push_back(seq[(size_t)i]); for (int i = 1; i < m - 1; i++) order.push_back(seq[(size_t)i]); break;
                    default: order = seq; break;
                }
                if (order.empty()) order = seq;
                if (p.arpDir == 4) note = seq[(size_t)((rng.next() * 0.5 + 0.5) * m) % (size_t)m];
                else note = order[(size_t)(arpIdx % (int)order.size())];
                arpIdx++;
                vel = lastVel;
            }
            voiceOn(note, vel, false);
            gateOffAt = start + L * clampv(p.gate, 0.05, 1.0);
            if (p.gate >= 0.999) gateOffAt = -1;
        } else if (p.patMode == 3) {   // sequencer
            const int len = clampv(p.seqLen, 1, 16);
            const int idx = (int)(((stepK % len) + len) % len);
            seqStepShown = idx;
            const auto& st = p.steps[(size_t)idx];
            const auto& prev = p.steps[(size_t)((idx + len - 1) % len)];
            const auto& root = pickKey();
            if (!st.on) { if (sounding) voiceRelease(); prevSlid = false; gateOffAt = -1; return; }
            const bool slideIn = !first && prevSlid && prev.on && sounding;
            voiceOn(root.note + st.pitch, st.accent ? 1.0 : root.vel * 0.85, slideIn, slideIn, st.accent);
            prevSlid = st.slide;
            gateOffAt = st.slide ? -1 : start + L * clampv(p.gate, 0.05, 0.98);
        }
    }

    // ------------------------------------------------ control rate (every kCtl samples of the core)
    void control() {
        const double dtc = kCtl / fs;
        amp.set(p.ampEnv, fs); fEnv.set(p.fEnv, fs / kCtl); mEnv.set(p.mEnv, fs / kCtl); fEnvS = fEnv.next(); mEnvS = mEnv.next();
        // LFOs
        for (int k = 0; k < 2; k++) {
            const auto& L = k == 0 ? p.lfo1 : p.lfo2;
            double rateMul = std::exp2(4 * modSum[k == 0 ? (int)HzDst::lfo1Rate : (int)HzDst::lfo2Rate]);
            if (L.sync && !L.retrig && hostPlaying) {
                const double divB = lfoDivBeats(L.div);
                lfoPh[(size_t)k] = hz::frac(hostPpqNow() / divB * rateMul);
            } else {
                const double hzv = L.sync ? bpm / 60.0 / lfoDivBeats(L.div) : L.hz;
                const double before = lfoPh[(size_t)k];
                lfoPh[(size_t)k] = hz::frac(lfoPh[(size_t)k] + hzv * rateMul * dtc);
                if (lfoPh[(size_t)k] < before) { lfoHold[(size_t)k] = rng.next(); }
            }
            lfoV[(size_t)k] = lfoValue(k, L.shape, lfoPh[(size_t)k]);
        }
        // modulation matrix
        double src[(int)HzSrc::count] = {};
        src[(int)HzSrc::lfo1] = lfoV[0]; src[(int)HzSrc::lfo2] = lfoV[1]; src[(int)HzSrc::filterEnv] = fEnvS; src[(int)HzSrc::modEnv] = mEnvS; src[(int)HzSrc::ampEnv] = amp.v;
        src[(int)HzSrc::velocity] = velocity; src[(int)HzSrc::note] = (curNote - 60) / 48.0; src[(int)HzSrc::modWheel] = wheel; src[(int)HzSrc::aftertouch] = touch;
        src[(int)HzSrc::pitchBend] = bendV; src[(int)HzSrc::random] = noteRandom;
        for (auto& m : modSum) m = 0;
        for (auto& s : p.slots) if (s.src > 0 && s.dst > 0 && s.src < (int)HzSrc::count && s.dst < (int)HzDst::count) modSum[s.dst] += src[s.src] * s.amt;
        // glide and pitch envelope
        const double glideS = slideGlide ? std::max(0.04, p.glide) : p.glide;
        if ((p.glideMode > 0 || slideGlide) && glideS > 0.0005 && pitchNow != targetPitch) {
            if (p.glideShape == 1) {   // linear, constant time
                glideT = std::min(1.0, glideT + dtc / glideS);
                pitchNow = glideFrom + (targetPitch - glideFrom) * glideT;
            } else pitchNow = targetPitch + (pitchNow - targetPitch) * std::exp(-dtc / (glideS / 4.0));
            if (std::abs(pitchNow - targetPitch) < 1e-4) pitchNow = targetPitch;
        } else pitchNow = targetPitch;
        penvT += dtc;
        const double penv = p.pEnvAmt * std::exp(-penvT / std::max(0.003, p.pEnvTime / 4.6));
        const double base = pitchNow + penv + bendV * p.bend + 24 * modSum[(int)HzDst::pitch];
        // oscillators
        for (int o = 0; o < 2; o++) {
            const auto& O = o == 0 ? p.osc1 : p.osc2;
            const double pitch = base + O.octave * 12 + O.semi + O.fine / 100.0 + 24 * modSum[o == 0 ? (int)HzDst::osc1Pitch : (int)HzDst::osc2Pitch];
            const int nv = clampv(O.voices, 1, kMaxVoices);
            const double det = clampv(O.detune + modSum[(int)HzDst::detune], 0.0, 1.0) * 100.0;   // cents, edge voices
            for (int v = 0; v < nv; v++) {
                const double pos = nv == 1 ? 0.0 : (2.0 * v / (nv - 1) - 1.0);
                // drift: a slow random walk per voice
                driftV[o][(size_t)v] += (rng.next() * 0.12 - driftV[o][(size_t)v] * 0.02);
                const double cents = det * pos + p.drift * 14.0 * driftV[o][(size_t)v];
                const double f = hz::noteHz(pitch + cents / 100.0);
                incT[o][(size_t)v] = std::min(0.45, f / fs);
                const double pan = pos * O.spread;
                panL[o][(size_t)v] = std::cos((pan + 1) * kPi / 4) * 1.41421356 / std::sqrt((double)nv);
                panR[o][(size_t)v] = std::sin((pan + 1) * kPi / 4) * 1.41421356 / std::sqrt((double)nv);
            }
            nVoices[o] = nv;
            waveT[o] = clampv(O.wave + 3 * modSum[o == 0 ? (int)HzDst::osc1Wave : (int)HzDst::osc2Wave], 0.0, 3.0);
            levelT[o] = clampv(O.level + modSum[o == 0 ? (int)HzDst::osc1Level : (int)HzDst::osc2Level], 0.0, 1.5);
        }
        pwT = clampv(p.osc1.pw + 0.45 * modSum[(int)HzDst::pulseWidth], 0.05, 0.95);
        pw2T = clampv(p.osc2.pw + 0.45 * modSum[(int)HzDst::pulseWidth], 0.05, 0.95);
        fmT = clampv(p.fm + modSum[(int)HzDst::fm], 0.0, 1.0);
        subInc = std::min(0.45, hz::noteHz(base - 12 * clampv(p.subOct, 0, 2)) / fs);
        subLvlT = clampv(p.subLevel + modSum[(int)HzDst::subLevel], 0.0, 1.5);
        noiseLvlT = clampv(p.noiseLevel + modSum[(int)HzDst::noiseLevel], 0.0, 1.0);
        const bool wasStereo = stereo;
        stereo = (p.osc1.voices > 1 && p.osc1.spread > 0.001 && p.osc1.level > 0) || (p.osc2.voices > 1 && p.osc2.spread > 0.001 && p.osc2.level > 0) || std::abs(modSum[(int)HzDst::pan]) > 1e-4;
        if (stereo && !wasStereo) { lad[1] = lad[0]; svf[1] = svf[0]; vow[1] = vow[0]; combDamp[1] = combDamp[0]; lowState[1] = lowState[0]; toneLp[1] = toneLp[0]; dc[1] = dc[0]; }
        // filter
        const double env = fEnvS * (1 + p.fVel * (velocity - 1));
        const double cutSemis = 12 * std::log2(std::max(20.0, p.cutoff) / 440.0) + 69 + p.envAmt * env + p.keyTrack * (pitchNow - 60)
                              + 96 * modSum[(int)HzDst::cutoff] + p.accent * accentLevel * 18 + 0.8 * p.drift * driftV[0][0];
        cutHz = clampv(hz::noteHz(cutSemis), 15.0, fs * 0.45);
        gT = std::tan(kPi * std::min(cutHz, fs * 0.42) / fs);
        resT = clampv(p.res + modSum[(int)HzDst::resonance] + p.accent * accentLevel * 0.15, 0.0, 1.0);
        fDriveT = dbToGain(clampv(p.fDrive + 24 * modSum[(int)HzDst::filterDrive], 0.0, 36.0)); driveComp = std::pow(fDriveT, -0.3);
        dirtT = clampv(p.dirt + modSum[(int)HzDst::dirt], 0.0, 1.0);
        ampModT = std::max(0.0, 1 + modSum[(int)HzDst::amp]) * (1 - p.velAmp * (1 - velocity)) * (1 + 0.4 * p.accent * accentLevel);
        panT = clampv(modSum[(int)HzDst::pan], -1.0, 1.0);
        { const double toneHz = 400 * std::pow(40.0, p.dirtTone); toneLp[0].set(fs, toneHz); toneLp[1].set(fs, toneHz); }
        // ramps over this block
        const double inv = 1.0 / kCtl;
        dG = (gT - gNow) * inv; dAmpMod = (ampModT - ampModNow) * inv;
        for (int o = 0; o < 2; o++) for (int v = 0; v < nVoices[o]; v++) dInc[o][(size_t)v] = (incT[o][(size_t)v] - inc[o][(size_t)v]) * inv;
        dSubInc = (subInc - subIncNow) * inv;
        if (!primed) { gNow = gT; ampModNow = ampModT; for (int o = 0; o < 2; o++) for (int v = 0; v < kMaxVoices; v++) inc[o][(size_t)v] = incT[o][(size_t)v]; subIncNow = subInc; dG = dAmpMod = dSubInc = 0; for (auto& a : dInc) a.fill(0); primed = true; }
        ui.cutoffHz = cutHz; vowelDirty = true;
        dirtGain = dbToGain(dirtT * 36.0); dirtNorm = 1.0 / (0.4 + 0.6 * std::sqrt(dirtGain));
    }
    double hostPpqNow() const { return hostPpq; }
    static double lfoDivBeats(int d) {
        static const double b[] = { 0.125, 1.0 / 6, 0.25, 1.0 / 3, 0.375, 0.5, 2.0 / 3, 0.75, 1, 1.5, 2, 4, 8, 16 };
        return b[clampv(d, 0, 13)];
    }
    double lfoValue(int k, int shape, double t) {
        switch (shape) {
            case 0: return std::sin(2 * kPi * t);
            case 1: return 1 - 4 * std::abs(t - 0.5);
            case 2: return 2 * t - 1;
            case 3: return 1 - 2 * t;
            case 4: return t < 0.5 ? 1.0 : -1.0;
            case 5: return lfoHold[(size_t)k];
            default: { lfoSmooth[(size_t)k] += (lfoHold[(size_t)k] - lfoSmooth[(size_t)k]) * 0.05; return lfoSmooth[(size_t)k]; }
        }
    }

    // ------------------------------------------------ the 2x core, one sample
    void core(double& outL, double& outR) {
        // ramps
        gNow += dG; ampModNow += dAmpMod; subIncNow += dSubInc;
        double sumL = 0, sumR = 0;
        // osc 2 first (it can modulate osc 1 and is synced to it)
        const double w1 = waveNow[0] += (waveT[0] - waveNow[0]) * 0.002, w2 = waveNow[1] += (waveT[1] - waveNow[1]) * 0.002;
        lvlNow[0] += (levelT[0] - lvlNow[0]) * 0.002; lvlNow[1] += (levelT[1] - lvlNow[1]) * 0.002;
        const bool need2 = lvlNow[1] > 1e-4 || fmT > 1e-4;
        bool wrapped[kMaxVoices] = {}; double wrapFrac[kMaxVoices] = {};
        for (int v = 0; v < nVoices[0]; v++) {
            inc[0][(size_t)v] += dInc[0][(size_t)v];
            double& t = ph[0][(size_t)v]; t += inc[0][(size_t)v];
            if (t >= 1) { t -= 1; wrapped[v] = true; wrapFrac[v] = t / inc[0][(size_t)v]; }
        }
        double o2[kMaxVoices] = {};
        if (need2) for (int v = 0; v < nVoices[1]; v++) {
            inc[1][(size_t)v] += dInc[1][(size_t)v];
            double& t = ph[1][(size_t)v];
            const int m = std::min(v, nVoices[0] - 1);
            if (p.sync && wrapped[m]) t = wrapFrac[m] * inc[1][(size_t)v];
            else { t += inc[1][(size_t)v]; if (t >= 1) t -= 1; }
            o2[v] = hz::morph(w2, t, inc[1][(size_t)v], pw2T);
            sumL += o2[v] * panL[1][(size_t)v] * lvlNow[1]; sumR += o2[v] * panR[1][(size_t)v] * lvlNow[1];
        }
        if (lvlNow[0] > 1e-4) for (int v = 0; v < nVoices[0]; v++) {
            double t = ph[0][(size_t)v];
            if (fmT > 1e-4) t = hz::frac(t + fmT * 0.5 * o2[std::min(v, nVoices[1] - 1)]);
            const double x = hz::morph(w1, t, inc[0][(size_t)v], pwT);
            sumL += x * panL[0][(size_t)v] * lvlNow[0]; sumR += x * panR[0][(size_t)v] * lvlNow[0];
        }
        // sub
        subLvl += (subLvlT - subLvl) * 0.002;
        double sub = 0;
        if (subLvl > 1e-4) {
            subPh += subIncNow; if (subPh >= 1) subPh -= 1;
            switch (p.subWave) {
                case 0: sub = std::sin(2 * kPi * subPh); break;
                case 1: sub = hz::shape(1, subPh, subIncNow, 0.5); break;
                case 2: sub = hz::shape(3, subPh, subIncNow, 0.5); break;
                default: sub = hz::shape(3, subPh, subIncNow, 0.25); break;
            }
            sub *= subLvl;
        }
        // noise
        noiseLvl += (noiseLvlT - noiseLvl) * 0.002;
        double nz = 0;
        if (noiseLvl > 1e-4) {
            const double w = rng.next();
            if (p.noiseType == 1) { pk0 = 0.99886 * pk0 + w * 0.0555179; pk1 = 0.99332 * pk1 + w * 0.0750759; pk2 = 0.969 * pk2 + w * 0.153852; pk3 = 0.8665 * pk3 + w * 0.3104856; nz = (pk0 + pk1 + pk2 + pk3 + w * 0.5362) * 0.18; }
            else if (p.noiseType == 2) { brown = (brown + 0.02 * w) / 1.02; nz = brown * 3.5; }
            else nz = w * 0.5;
            nz *= noiseLvl;
        }
        double inL = sumL + nz, inR = sumR + nz;
        if (!p.subPostFilter) { inL += sub; inR += sub; }
        // filter
        double yL, yR;
        if (stereo) { yL = filter(0, inL); yR = filter(1, inR); }
        else { yL = yR = filter(0, 0.5 * (inL + inR)); }
        if (p.subPostFilter) { yL += sub; yR += sub; }
        // dirt
        if (dirtT > 1e-4) {
            yL = dirtStage(0, yL); yR = stereo ? dirtStage(1, yR) : yL;
        }
        // amp
        const double a = amp.next() * ampModNow;
        double gl = a, gr = a;
        if (std::abs(panT) > 1e-4) { gl *= std::cos((panT + 1) * kPi / 4) * 1.41421356; gr *= std::sin((panT + 1) * kPi / 4) * 1.41421356; }
        outL = yL * gl * 0.85; outR = yR * gr * 0.85;
    }
    double filter(int c, double x) {
        if (fDriveT > 1.001) x = std::tanh(x * fDriveT * 0.7) / 0.7 * driveComp;   // filter drive: soft saturation, level kept roughly even
        const double g = gNow, res = resT;
        double y;
        switch (p.fType) {
            case 0: case 1: {
                const double G = g / (1 + g);
                y = lad[(size_t)c].process(x * 0.8, G, res * 4.1, p.lowKeep, p.fType == 1) * 1.25;
                break;
            }
            case 2: case 3: case 4: case 5: {
                const double k = 2.0 - 1.9 * std::pow(res, 0.6);
                double lp, bp, hp; svf[(size_t)c].tick(x, g, k, lp, bp, hp);
                const double resComp = 0.35 + 0.65 * std::min(1.0, k / 1.414);   // tame the resonant peak's level
                y = (p.fType == 2 ? lp : p.fType == 3 ? bp * (0.3 + 0.7 * k) * 1.4 : p.fType == 4 ? hp : lp + hp) * (p.fType == 5 ? 1.0 : resComp);
                break;
            }
            case 6: case 7: {   // comb, tuned by cutoff
                auto& d = combLine[(size_t)c];
                const double delayS = clampv(fs / cutHz, 2.0, (double)combSize - 4);
                const double fb = (p.fType == 6 ? 1 : -1) * res * 0.97;
                const double fbSig = d.read(delayS);
                combDamp[(size_t)c] = fbSig + (combDamp[(size_t)c] - fbSig) * 0.25;
                const double v = std::tanh(x + fb * combDamp[(size_t)c]);
                d.push((float)v);
                y = 0.5 * (x + v) * (1 - 0.45 * res);
                break;
            }
            case 8: {   // vowel: three formant band-passes, the cutoff sweeps A > E > I > O > U
                static const double F[5][3] = { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 }, { 570, 840, 2410 }, { 300, 870, 2240 } };
                const double pos = clampv((std::log2(cutHz / 20.0)) / 10.0, 0.0, 1.0) * 4.0;
                const int i = std::min(3, (int)pos); const double f = pos - i;
                const double q = 4 + 16 * res; double sum = 0;
                if (c == 0 && vowelDirty) {   // formant coefficients: once per control block
                    for (int k = 0; k < 3; k++) vowG[(size_t)k] = std::tan(kPi * (F[i][k] * (1 - f) + F[i + 1][k] * f) / fs);
                    vowelDirty = false;
                }
                for (int k = 0; k < 3; k++) {
                    double lp, bp, hp; vow[(size_t)c][(size_t)k].tick(x, vowG[(size_t)k], 1.0 / q, lp, bp, hp);
                    sum += bp / q * (k == 0 ? 2.2 : k == 1 ? 1.5 : 0.9) * std::sqrt(q);
                }
                y = std::tanh(sum * 0.9);
                break;
            }
            default: y = x; break;
        }
        return y;
    }
    double dirtStage(int c, double x) {
        const double amt = dirtT, g = dirtGain;
        const double low = lowState[(size_t)c].lp(x);
        const double keep = p.bassKeep;
        const double hi = x - keep * low;
        double y;
        if (p.dirtType == 5) {   // crush: bit depth and sample-rate reduction
            const double bits = 16 - 13 * amt, q = std::exp2(bits - 1);
            crushPh[(size_t)c] += 1.0 - 0.94 * amt; if (crushPh[(size_t)c] >= 1) { crushPh[(size_t)c] -= 1; crushHold[(size_t)c] = std::round(hi * q) / q; }
            y = crushHold[(size_t)c];
        } else y = hz::dirtShape(p.dirtType, hi * g) * dirtNorm;
        y = toneLp[(size_t)c].lp(y);
        y = y - dc[(size_t)c].lp(y);   // the shapers can add DC: remove it (5 Hz)
        const double wet = keep * low + y;
        return x + (wet - x) * p.dirtMix;
    }

    // ------------------------------------------------ effects at the host rate
    void updateFx(bool force) {
        if (!force && ++fxCounter < 8) return;
        fxCounter = 0;
        for (int c = 0; c < 2; c++) {
            eq[c][0].set(Svf::lowShelf, sr, p.eqLowHz, 0.7, p.eqLow);
            eq[c][1].set(Svf::bell, sr, p.eqMidHz, 0.9, p.eqMid);
            eq[c][2].set(Svf::highShelf, sr, p.eqHighHz, 0.7, p.eqHigh);
            boostShelf[c].set(Svf::lowShelf, sr, p.boostHz, 0.8, 9.0 * p.subBoost);
            boostLp[c].set(Svf::lowpass, sr, p.boostHz * 1.6, 0.7);
            harmHp[c].set(Svf::highpass, sr, p.boostHz * 1.7, 0.7);
            harmLp[c].set(Svf::lowpass, sr, p.boostHz * 7.0, 0.7);
            chorusSplit[c].set(Svf::highpass, sr, 220, 0.7);
        }
        monoSplit.set(Svf::lowpass, sr, p.monoBelow, 0.7);
        CompParams cp; cp.thresh = -p.comp * 30; cp.ratio = 2 + 4 * p.comp; cp.attackMs = 8; cp.releaseMs = 90; cp.kneeDb = 6; cp.autoMakeup = true; cp.style = CompDsp::glue; cp.scHpf = 60;
        comp.set(cp);
        DelayParams dp; dp.timeMs = clampv(hzDivs()[(size_t)clampv(p.delayDiv, 0, (int)hzDivs().size() - 1)].second * 60000.0 / bpm, 20.0, 2000.0);
        dp.feedback = p.delayFb; dp.mix = p.delayMix; dp.lowCutHz = 250; dp.highCutHz = 6500; dp.warmth = 0.3; dp.wow = 0.05; dp.mode = p.pingPong ? DelayDsp::pingPong : DelayDsp::stereo;
        delay.set(dp);
        VerbParams vp; vp.mix = p.verbMix; vp.sizePct = 20 + 80 * p.verbSize; vp.decayS = 0.6 + 5.4 * p.verbSize * p.verbSize; vp.lowCutHz = 250; vp.dampHz = 6000; vp.preMs = 12; vp.character = p.verbSize > 0.6 ? VerbDsp::hall : VerbDsp::room;
        verb.set(vp);
    }
    void effects(float* L, float* R, int n) {
        updateFx(false);
        const bool eqOn = std::abs(p.eqLow) > 0.01 || std::abs(p.eqMid) > 0.01 || std::abs(p.eqHigh) > 0.01;
        const bool boostOn = p.subBoost > 0.001 || p.harmonics > 0.001;
        for (int i = 0; i < n; i++) {
            double x[2] = { L[i], R[i] };
            for (int c = 0; c < 2; c++) {
                if (eqOn) for (auto& f : eq[c]) x[c] = f.process(x[c]);
                if (boostOn) {
                    const double low = boostLp[c].process(x[c]);
                    double h = std::abs(low) * 1.6 + std::tanh(low * 3.0) * 0.5;   // even + odd harmonics of the sub
                    h = harmLp[c].process(harmHp[c].process(h));
                    x[c] = boostShelf[c].process(x[c]) + h * p.harmonics * 1.4;
                }
            }
            L[i] = (float)x[0]; R[i] = (float)x[1];
        }
        float* ch[2] = { L, R };
        if (p.comp > 0.001) comp.process(ch, 2, n);
        if (p.chorus > 0.001) {
            for (int i = 0; i < n; i++) {
                chorusPh = hz::frac(chorusPh + p.chorusRate / sr);
                for (int c = 0; c < 2; c++) {
                    const double hi = chorusSplit[c].process(c == 0 ? L[i] : R[i]);
                    chorusLine[c].push((float)hi);
                    const double m = std::sin(2 * kPi * (chorusPh + c * 0.25));
                    const double d = (0.012 + 0.006 * p.chorusDepth * m) * sr;
                    const double wet = chorusLine[c].read(d);
                    (c == 0 ? L[i] : R[i]) += (float)(wet * p.chorus * 0.8);
                }
            }
        }
        if (p.delayMix > 0.001) delay.process(ch, 2, n); else if (!delayClear) { delay.reset(); delayClear = true; }
        if (p.delayMix > 0.001) delayClear = false;
        if (p.verbMix > 0.001) verb.process(ch, 2, n); else if (!verbClear) { verb.reset(); verbClear = true; }
        if (p.verbMix > 0.001) verbClear = false;
        // width, mono below, limiter, volume
        const double ceil = dbToGain(-0.5), rel = std::exp(-1.0 / (0.08 * sr));
        for (int i = 0; i < n; i++) {
            double m = 0.5 * (L[i] + R[i]), s = 0.5 * (L[i] - R[i]);
            s = (s - monoSplit.process(s)) * p.width;
            double l = m + s, r = m - s;
            const double v = volS.next(dbToGain(p.volumeDb));
            l *= v; r *= v;
            if (p.limiter) {
                // peak limiter with a 1 ms attack and 80 ms release, then a soft clipper that rounds off whatever
                // gets past it, so a fast 808 transient is shaped instead of stepped
                const double pk = std::max(std::abs(l), std::abs(r));
                const double want = pk > ceil ? ceil / pk : 1.0;
                limGain = want < limGain ? want + (limGain - want) * limAtt : 1 + (limGain - 1) * rel;
                limGain = std::min(limGain, 1.0);
                l = softClip(l * limGain); r = softClip(r * limGain);
            }
            L[i] = (float)l; R[i] = (float)r;
        }
    }

    // linear up to 0.75, then a smooth knee that never passes 0.99
    static double softClip(double x) {
        const double a = std::abs(x), k = 0.75, room = 0.99 - k;
        if (a <= k) return x;
        return std::copysign(k + room * std::tanh((a - k) / room), x);
    }
    void hardReset() {
        for (auto& o : ph) o.fill(0);
        for (auto& o : inc) o.fill(0);
        for (auto& o : dInc) o.fill(0);
        for (auto& o : driftV) for (auto& v : o) v = rng.next() * 0.5;
        for (auto& l : lad) l.reset();
        for (auto& s : svf) s.reset();
        for (auto& c : vow) for (auto& s : c) s.reset();
        combSize = 1; while (combSize < (int)(fs / 15.0) + 8) combSize <<= 1;
        for (auto& d : combLine) d.prepare(combSize);
        for (auto& c : eq) for (auto& f : c) f.reset();
        amp = {}; fEnv = {}; mEnv = {};
        lowState[0].set(fs, 120); lowState[1].set(fs, 120); dc[0].set(fs, 5); dc[1].set(fs, 5); toneLp[0].set(fs, 12000); toneLp[1].set(fs, 12000);
        primed = false; sounding = false; hasPlayed = false; pitchNow = targetPitch = 48;
        subPh = 0; brown = pk0 = pk1 = pk2 = pk3 = 0; lfoPh = { 0, 0 };
        waveNow = { p.osc1.wave, p.osc2.wave }; lvlNow = { p.osc1.level, p.osc2.level }; subLvl = p.subLevel; noiseLvl = p.noiseLevel;
    }

    HazeParams p;
    double sr = 48000, fs = 96000, bpm = 120, hostPpq = 0, clock = 0; bool hostPlaying = false;
    std::array<Oversampler2x, 2> os;
    std::vector<Ev> queue; std::vector<Key> keys; uint64_t orderCounter = 0;
    hz::Rng rng;
    // voice
    int curNote = 48; bool sounding = false, hasPlayed = false; double pitchNow = 48, targetPitch = 48, glideFrom = 48, glideT = 1, penvT = 10;
    double velocity = 1, lastVel = 0.8, noteRandom = 0, accentLevel = 0, bendV = 0, wheel = 0, touch = 0;
    hz::Adsr amp, fEnv, mEnv; double fEnvS = 0, mEnvS = 0;
    std::array<double, 2> lfoPh{}, lfoV{}, lfoHold{}, lfoSmooth{};
    double modSum[(int)HzDst::count] = {};
    // oscillators
    std::array<std::array<double, kMaxVoices>, 2> ph{}, inc{}, incT{}, dInc{}, panL{}, panR{}, driftV{};
    int nVoices[2] = { 1, 1 };
    double waveT[2] = { 2, 2 }, levelT[2] = { 1, 0 }; std::array<double, 2> waveNow{ 2, 2 }, lvlNow{ 1, 0 };
    double pwT = 0.5, pw2T = 0.5, fmT = 0, subPh = 0, subInc = 0, subIncNow = 0, dSubInc = 0, subLvlT = 0, subLvl = 0, noiseLvlT = 0, noiseLvl = 0;
    double pk0 = 0, pk1 = 0, pk2 = 0, pk3 = 0, brown = 0;
    bool stereo = false, primed = false;
    // filter / dirt
    double driveComp = 1, cutHz = 1000, gT = 0.1, gNow = 0.1, dG = 0, resT = 0, fDriveT = 1, dirtT = 0, ampModT = 1, ampModNow = 1, dAmpMod = 0, panT = 0;
    std::array<hz::Ladder, 2> lad; std::array<hz::Svf2, 2> svf; std::array<std::array<hz::Svf2, 3>, 2> vow;
    std::array<DelayLine, 2> combLine; std::array<double, 2> combDamp{}; int combSize = 1;
    std::array<hz::OnePole, 2> lowState, toneLp, dc; std::array<double, 2> crushPh{}, crushHold{};
    int ctl = 0; bool vowelDirty = true; std::array<double, 3> vowG{}; double dirtGain = 1, dirtNorm = 1;
    // pattern
    int64_t stepK = 0; double nextStep = 0, gateOffAt = -1, lastBeat = 0; int arpIdx = 0, seqStepShown = -1; bool prevSlid = false, slideGlide = false;
    // effects
    Svf eq[2][3], boostShelf[2], boostLp[2], harmHp[2], harmLp[2], chorusSplit[2], monoSplit;
    CompDsp comp; DelayDsp delay; VerbDsp verb; bool delayClear = true, verbClear = true;
    std::array<DelayLine, 2> chorusLine; double chorusPh = 0;
    Smooth volS; double limGain = 1, limAtt = 0; int fxCounter = 0;
    View ui;
};
} // namespace lgn
