// LGN Fret: a physically modelled guitar.
//
//   pluck ──► string (waveguide) ──► pickup comb ──► body / pickup ──► amp + cab ──► chorus ─ delay ─ reverb ─ age
//
// String: a delay line one period long with a loss filter (brightness, decay) and allpass stiffness (the slight
// sharpening of upper partials real strings have), tuned to within a cent by compensating every filter's delay.
// Pluck: pick, finger or thumb excitation, filtered by how hard it is played and comb-filtered by where along the
// string it is played. Re-plucking a ringing string keeps it ringing (and briefly damps it, as a pick does).
// Strings: notes are given to six strings the way a guitarist would, so a new note on a busy string chokes it,
// chords spread across the neck, and chords are strummed down / up across the strings.
// Plain C++ (no JUCE) so it can be tested on its own.
#pragma once
#include "Common.h"
#include "Delay.h"
#include "Reverb.h"
#include <random>

namespace lgn {

struct GuitarParams {
    // play
    int mode = 0;            // 0 Guitar (6 strings), 1 Free poly (12 voices), 2 Legato (mono, slides)
    double strumMs = 35;     // time from the first to the last of six strings
    int strumDir = 2;        // 0 down, 1 up, 2 alternate
    double humanize = 0.3, glideMs = 60, bendSemis = 2, vibCents = 0, vibHz = 5.5;
    int tuning = 0;          // see tunings()
    // string
    int pick = 0;            // 0 pick, 1 finger, 2 thumb
    double pickHard = 0.6, pickPos = 0.18, pickNoise = 0.3, bright = 0.6, decayS = 4.0, stiffness = 0.2, mute = 0, releaseS = 0.12, velSense = 0.7;
    int course = 0;          // 0 single, 1 unison double, 2 twelve-string (octave courses on the four low strings)
    double detuneCents = 6;
    // body / pickup
    int body = 1;            // 0 none, 1 dreadnought, 2 parlor, 3 classical, 4 jumbo, 5 archtop
    double bodyAmt = 0.7, bodySize = 1.0;
    int pickup = 0;          // 0 off (acoustic), 1 single coil, 2 humbucker, 3 piezo
    double pickupPos = 0.5;  // 0 neck .. 1 bridge
    // amp
    int amp = 0;             // 0 off, 1 clean, 2 crunch, 3 lead, 4 fuzz
    double driveDb = 12, bassDb = 0, midDb = 0, trebleDb = 0, ampVolDb = 0;
    int cab = 0;             // 0 off, 1 1x12 open, 2 2x12, 3 4x12
    // fx
    double chorus = 0, chorusHz = 0.8, delayMix = 0, delayMs = 375, delayFb = 0.3, verbMix = 0.15, verbSize = 0.5, age = 0, width = 0.6, volumeDb = 0;
};

struct Tuning { const char* name; int open[6]; };
inline const std::vector<Tuning>& tunings() {
    static const std::vector<Tuning> t = {
        { "Standard",  { 40, 45, 50, 55, 59, 64 } },
        { "Drop D",    { 38, 45, 50, 55, 59, 64 } },
        { "Half Down", { 39, 44, 49, 54, 58, 63 } },
        { "Open D",    { 38, 45, 50, 54, 57, 62 } },
        { "DADGAD",    { 38, 45, 50, 55, 57, 62 } },
        { "Open G",    { 38, 43, 50, 55, 59, 62 } } };
    return t;
}

// 2x oversampler (31-tap windowed-sinc halfband, polyphase) for the amp's distortion, so it does not alias.
class Oversampler2x {
public:
    static constexpr int kTaps = 31, kHalf = 16;
    Oversampler2x() {
        const int M = kTaps / 2;
        std::array<double, kTaps> h{};
        for (int i = 0; i < kTaps; i++) {
            const int k = i - M;
            const double sinc = k == 0 ? 0.5 : std::sin(kPi * k / 2.0) / (kPi * k);
            const double w = 0.42 - 0.5 * std::cos(2 * kPi * i / (kTaps - 1)) + 0.08 * std::cos(4 * kPi * i / (kTaps - 1));
            h[(size_t)i] = sinc * w;
        }
        double s = 0; for (double v : h) s += v;
        for (int i = 0; i < kTaps; i++) { hh[(size_t)i] = h[(size_t)i] / s; }
        for (int k = 0; k < kHalf; k++) { even[(size_t)k] = 2 * (2 * k < kTaps ? hh[(size_t)(2 * k)] : 0); odd[(size_t)k] = 2 * (2 * k + 1 < kTaps ? hh[(size_t)(2 * k + 1)] : 0); }
    }
    void reset() { inHist.fill(0); outHist.fill(0); ip = op = 0; }
    // one input sample in, two output samples at twice the rate
    void upsample(double x, double& a, double& b) {
        ip = (ip + kHalf - 1) % kHalf; inHist[(size_t)ip] = x;
        double sa = 0, sb = 0;
        for (int k = 0; k < kHalf; k++) { const double v = inHist[(size_t)((ip + k) % kHalf)]; sa += even[(size_t)k] * v; sb += odd[(size_t)k] * v; }
        a = sa; b = sb;
    }
    // two samples at twice the rate in, one out (filtered, then every other sample kept)
    double downsample(double a, double b) {
        push(a); push(b);
        double s = 0; for (int i = 0; i < kTaps; i++) s += hh[(size_t)i] * outHist[(size_t)((op + i) % kTaps)];
        return s;
    }
private:
    void push(double x) { op = (op + kTaps - 1) % kTaps; outHist[(size_t)op] = x; }
    std::array<double, kTaps> hh{}, outHist{}; std::array<double, kHalf> even{}, odd{}, inHist{};
    int ip = 0, op = 0;
};

class GuitarDsp {
public:
    static constexpr int kStrings = 6, kVoices = 12;

    struct VoiceView { int note = -1, string = -1; float level = 0; };   // for the fretboard display

    void prepare(double rate) {
        sr = rate;
        for (auto& v : voices) v.prepare(sr);
        for (int c = 0; c < 2; c++) { for (auto& m : bodyModes[c]) m.reset(); pickupF[c].reset(); lowCut[c].reset(); ampHp[c].reset(); ampDc[c].reset(); for (auto& t : tone[c]) t.reset(); for (auto& f : cabF[c]) f.reset(); ageLp[c].reset(); os[c].reset(); chorusLine[c].prepare((int)(0.05 * sr) + 8); }
        excScratch.assign((size_t)(sr / 20) + 8, 0.0);
        events.reserve(4096); incoming.reserve(1024);
        delay.prepare(sr); verb.prepare(sr);
        events.clear(); now = 0; alternate = false; handFret = 3;
        bendS.setTime(sr, 0.01); bendS.reset(0);
        volS.setTime(sr, 0.03); volS.reset(dbToGain(p.volumeDb));
        chorusPh = 0; agePh = 0; filtersDirty = true; updateFilters();
    }
    void set(const GuitarParams& np) {
        if (np.body != p.body || np.bodySize != p.bodySize || np.pickup != p.pickup || np.amp != p.amp || np.bassDb != p.bassDb || np.midDb != p.midDb ||
            np.trebleDb != p.trebleDb || np.cab != p.cab || np.age != p.age) filtersDirty = true;
        p = np;
    }
    const GuitarParams& params() const { return p; }

    // MIDI in. offset = sample position within the next process() call.
    void noteOn(int note, double velocity, int offset) { incoming.push_back({ note, velocity, offset }); }
    void noteOff(int note, int offset) { events.push_back({ now + offset, Event::off, note, 0, -1 }); }
    void allNotesOff() { for (auto& v : voices) v.release(); events.clear(); }
    void pitchBend(double minus1to1) { bendTarget = minus1to1; }
    void modWheel(double v01) { mod = v01; }
    void sustainPedal(bool down) { sustain = down; if (!down) for (auto& v : voices) if (v.held && !v.keyDown) { v.held = false; v.release(); } }

    // Renders n samples, stereo, adding into out (cleared by the caller).
    void process(float* outL, float* outR, int n) {
        scheduleIncoming();
        if (filtersDirty) updateFilters();
        const double volT = dbToGain(p.volumeDb);
        const double ampIn = dbToGain(p.amp == 0 ? 0 : p.driveDb), ampOut = dbToGain(p.ampVolDb) * ampMakeup();
        for (int i = 0; i < n; i++, now++) {
            dispatchEvents();
            // control-rate updates every 16 samples
            if ((now & 15) == 0) controlUpdate();
            double l = 0, r = 0;
            for (auto& v : voices) if (v.active) v.render(l, r);
            double ch[2] = { l, r };
            for (int c = 0; c < 2; c++) {
                double x = ch[c];
                // body resonances (acoustic) and pickup response (electric)
                if (p.body > 0 && p.bodyAmt > 0) {
                    double res = 0; for (int m = 0; m < kModes; m++) res += bodyModes[c][(size_t)m].process(x) * modeGain[(size_t)m];
                    x = x * (1 - 0.45 * p.bodyAmt) + res * p.bodyAmt * bodyNorm;
                }
                if (p.pickup > 0) x = pickupF[c].process(x) * pickupGain;
                x = lowCut[c].process(x);
                // amp: drive (2x oversampled), tone stack, cab
                if (p.amp > 0) {
                    x = ampHp[c].process(x) * ampIn;
                    double a, b; os[c].upsample(x, a, b);
                    x = os[c].downsample(shapeAmp(a, c), shapeAmp(b, c));
                    x = ampDc[c].process(x);   // the drive is asymmetric on purpose (even harmonics); remove the DC it leaves
                    for (auto& t : tone[c]) x = t.process(x);
                    x *= ampOut;
                }
                if (p.cab > 0) for (int k = 0; k < 4; k++) x = cabF[c][(size_t)k].process(x);
                ch[c] = x;
            }
            // chorus: two modulated taps, opposite phase on each side
            if (p.chorus > 0.001) {
                chorusPh += p.chorusHz / sr; if (chorusPh >= 1) chorusPh -= 1;
                for (int c = 0; c < 2; c++) {
                    chorusLine[c].push((float)ch[c]);
                    const double d = (0.007 + 0.0035 * std::sin(2 * kPi * (chorusPh + c * 0.5))) * sr;
                    ch[c] = ch[c] * (1 - 0.35 * p.chorus) + chorusLine[c].read(d) * 0.7 * p.chorus;
                }
            }
            // age: tape wobble, dull top, a little hiss
            if (p.age > 0.001) {
                agePh += 0.9 / sr; if (agePh >= 1) agePh -= 1;
                for (int c = 0; c < 2; c++) {
                    ch[c] = ageLp[c].process(ch[c]);
                    ch[c] = std::tanh(ch[c] * (1 + p.age)) / (1 + p.age * 0.6) + p.age * 0.0025 * (rand01() * 2 - 1);
                }
            }
            const double g = volS.next(volT) * 2.0;   // +6 dB: puts a strummed chord around -6 dBFS peak at the default volume
            outL[i] += (float)(ch[0] * g); outR[i] += (float)(ch[1] * g);
        }
        // delay and reverb on the block (stereo)
        float* io[2] = { outL, outR };
        if (p.delayMix > 0.001) {
            DelayParams d; d.timeMs = p.delayMs; d.feedback = p.delayFb; d.mix = p.delayMix; d.lowCutHz = 250; d.highCutHz = 5500; d.warmth = 0.3; d.wow = 0.08 + p.age * 0.4; d.duck = 0.2; d.mode = DelayDsp::pingPong;
            delay.set(d); delay.process(io, 2, n);
        }
        if (p.verbMix > 0.001) {
            VerbParams v; v.sizePct = 30 + 60 * p.verbSize; v.decayS = 0.6 + 4.0 * p.verbSize * p.verbSize; v.preMs = 12 + 20 * p.verbSize; v.dampHz = 6000;
            v.lowCutHz = 180; v.widthPct = 100; v.modPct = 25; v.mix = p.verbMix; v.character = p.verbSize < 0.35 ? VerbDsp::room : p.verbSize < 0.7 ? VerbDsp::plate : VerbDsp::hall;
            verb.set(v); verb.process(io, 2, n);
        }
    }

    // For the display: what each voice is playing and how loud it is.
    int voiceViews(VoiceView* out) const {
        int k = 0;
        for (auto& v : voices) if (v.active && v.note >= 0) out[k++] = { v.note, v.string, (float)std::min(1.0, v.level * 3) };
        return k;
    }
    // Which string and fret a note would go to right now (display/tests).
    const int* openStrings() const { return tunings()[(size_t)clampv(p.tuning, 0, (int)tunings().size() - 1)].open; }
    static double noteHz(double midi) { return 440.0 * std::pow(2.0, (midi - 69) / 12.0); }

    // ------------------------------------------------------------------ one string (with an optional second course)
    struct Voice {
        struct Course {
            std::vector<double> buf; int mask = 0, w = 0;
            double lp = 0, ap1x = 0, ap1y = 0, ap2x = 0, ap2y = 0;
            double loopDelay = 100, g = 0.99, b = 0.3, apA = 0;
            void prepare(double sr) { int n = 1; while (n < (int)(sr / 20.0) + 8) n <<= 1; buf.assign((size_t)n, 0.0); mask = n - 1; w = 0; clear(); }
            void clear() { std::fill(buf.begin(), buf.end(), 0.0); lp = ap1x = ap1y = ap2x = ap2y = 0; }
            // value d samples ago (d >= 2), 4-point Lagrange interpolation
            double read(double d) const {
                const int i = (int)d; const double t = d - i;
                const double y0 = buf[(size_t)((w - i + 1) & mask)], y1 = buf[(size_t)((w - i) & mask)], y2 = buf[(size_t)((w - i - 1) & mask)], y3 = buf[(size_t)((w - i - 2) & mask)];
                const double c0 = -t * (t - 1) * (t - 2) / 6, c1 = (t + 1) * (t - 1) * (t - 2) / 2, c2 = -(t + 1) * t * (t - 2) / 2, c3 = (t + 1) * t * (t - 1) / 6;
                return c0 * y0 + c1 * y1 + c2 * y2 + c3 * y3;
            }
            double tick(double exc, double extraDamp) {
                const double y = read(loopDelay);
                // stiffness: two first-order allpasses
                const double a1 = apA * y + ap1x - apA * ap1y; ap1x = y; ap1y = a1;
                const double a2 = apA * a1 + ap2x - apA * ap2y; ap2x = a1; ap2y = a2;
                lp = (1 - b) * a2 + b * lp;
                const double v = lp * g * extraDamp + exc;
                buf[(size_t)w] = v; w = (w + 1) & mask;
                return y;
            }
            double tap(double d) const { return read(std::max(2.0, d)); }
        };

        std::array<Course, 2> course;
        std::vector<float> exc; int excLen = 0, excPos = 0; std::vector<float> noise; int noiseLen = 0, noisePos = 0;
        bool active = false, keyDown = false, held = false, releasing = false;
        int note = -1, string = 0; double freq = 110, freqTarget = 110, level = 0, velocity = 0.8, pan = 0;
        double dampDip = 1, releaseMul = 1, age = 0;
        double sr = 48000; uint64_t startTime = 0;
        // per-note settings copied from the engine
        double decayS = 4, brightHz = 5000, stiffA = 0, pickupPos = 0, pickupAmt = 0, detuneRatio = 1, course2Gain = 0, course2Pan = 0, glideCoef = 1;
        GuitarDsp* eng = nullptr;

        void prepare(double rate) { sr = rate; for (auto& c : course) c.prepare(sr); exc.assign((size_t)(sr / 20) + 8, 0.0f); noise.assign((size_t)(sr * 0.012) + 8, 0.0f); }
        void release() { keyDown = false; releasing = true; }

        // loop tuning: delay line length such that line + filters = one period
        static void tune(Course& c, double f, double sr, double brightHz, double decayS, double stiffA, double extraDecay) {
            const double w0 = 2 * kPi * f / sr;
            c.b = std::exp(-2 * kPi * clampv(brightHz, 200.0, sr * 0.45) / sr);
            c.apA = stiffA;
            // one-pole lowpass: magnitude and phase delay at f
            const double re = 1 - c.b * std::cos(w0), im = c.b * std::sin(w0);
            const double lpMag = (1 - c.b) / std::sqrt(re * re + im * im), lpDelay = std::atan2(im, re) / w0;
            // allpass (a + z^-1) / (1 + a z^-1): phase delay at f, two of them
            const std::complex<double> z(std::cos(-w0), std::sin(-w0));
            const double apPh = std::arg((c.apA + z) / (1.0 + c.apA * z));
            const double apDelay = 2 * (-apPh / w0);
            c.loopDelay = std::max(2.0, sr / f - lpDelay - apDelay);
            const double t60 = std::max(0.01, decayS * extraDecay);
            c.g = std::min(0.99999, std::pow(10.0, -3.0 / (t60 * f)) / lpMag);
        }
        void retune() {
            const double f1 = freq, f2 = freq * detuneRatio;
            const double decay = decayS * releaseMul;
            tune(course[0], f1, sr, brightHz, decay, stiffA, 1.0);
            if (course2Gain > 0) tune(course[1], f2, sr, brightHz * (detuneRatio > 1.5 ? 1.3 : 1.0), decay * (detuneRatio > 1.5 ? 0.7 : 1.0), stiffA, 1.0);
        }
        void render(double& l, double& r) {
            if (std::abs(freq - freqTarget) > 1e-6) freq = freqTarget + (freq - freqTarget) * glideCoef;
            const double e = excPos < excLen ? exc[(size_t)excPos++] : 0.0;
            const double dip = dampDip; dampDip = 1 - (1 - dampDip) * 0.995;
            double y1 = course[0].tick(e, dip);
            // pickup position: a comb from the reflection off the bridge
            if (pickupAmt > 0) y1 = y1 - pickupAmt * course[0].tap(pickupPos * course[0].loopDelay);
            double y2 = 0;
            if (course2Gain > 0) {
                y2 = course[1].tick(e * 0.85, dip);
                if (pickupAmt > 0) y2 = y2 - pickupAmt * course[1].tap(pickupPos * course[1].loopDelay);
                y2 *= course2Gain;
            }
            double pn = 0; if (noisePos < noiseLen) pn = noise[(size_t)noisePos++];
            const double s1 = y1 + pn, s2 = y2;
            // equal-power pans
            const double p1 = (pan + 1) * 0.25 * kPi, p2 = (course2Pan + 1) * 0.25 * kPi;
            l += s1 * std::cos(p1) + s2 * std::cos(p2); r += s1 * std::sin(p1) + s2 * std::sin(p2);
            level = std::max(std::abs(y1), level * 0.9997);
            age += 1;
            if (releasing && level < 1e-5 && excPos >= excLen) { active = false; note = -1; }
            else if (!releasing && level < 2e-6 && excPos >= excLen && age > sr * 0.2) { active = false; note = -1; }
        }
    };

private:
    struct Event { uint64_t time; enum Kind { on, off } kind; int note; double vel; int stringHint; };
    struct Incoming { int note; double vel; int offset; };

    double rand01() { return std::uniform_real_distribution<double>(0, 1)(rng); }

    // Groups note-ons that arrive together (a chord) and spreads them across the strings as a strum.
    void scheduleIncoming() {
        if (incoming.empty()) return;
        std::sort(incoming.begin(), incoming.end(), [](const Incoming& a, const Incoming& b) { return a.offset < b.offset; });
        size_t i = 0;
        while (i < incoming.size()) {
            size_t j = i + 1;
            const int chordWindow = (int)(0.006 * sr);
            while (j < incoming.size() && incoming[j].offset - incoming[i].offset <= chordWindow) j++;
            const int cn = (int)std::min<size_t>(j - i, kMaxChord);
            std::array<Incoming, kMaxChord> chord; std::copy(incoming.begin() + (long)i, incoming.begin() + (long)i + cn, chord.begin());
            std::sort(chord.begin(), chord.begin() + cn, [](const Incoming& a, const Incoming& b) { return a.note < b.note; });
            const bool chordy = cn >= 2 && p.mode != 2;
            bool up = false;
            if (chordy) { up = p.strumDir == 1 || (p.strumDir == 2 && alternate); if (p.strumDir == 2) alternate = !alternate; }
            // string choice for the whole chord first (low notes to low strings), then timing
            std::array<int, kMaxChord> strings; assignStrings(chord.data(), cn, strings.data());
            const double spacing = chordy ? p.strumMs * 0.001 * sr / 5.0 : 0.0;
            for (int k = 0; k < cn; k++) {
                const int idx = up ? cn - 1 - k : k;
                double t = chord[0].offset + spacing * (double)k;
                if (chordy && p.humanize > 0) t += spacing * 0.35 * p.humanize * (rand01() * 2 - 1);
                double vel = chord[(size_t)idx].vel;
                if (p.humanize > 0) vel *= 1 - 0.15 * p.humanize * rand01();
                if (chordy && up) vel *= 0.85;   // up-strums are lighter
                events.push_back({ now + (uint64_t)std::max(0.0, t), Event::on, chord[(size_t)idx].note, vel, strings[(size_t)idx] });
            }
            i = j;
        }
        incoming.clear();
        std::stable_sort(events.begin(), events.end(), [](const Event& a, const Event& b) { return a.time < b.time; });
    }

    void assignStrings(const Incoming* chord, int cn, int* out) {
        const int* open = openStrings();
        bool used[kStrings] = {};
        for (int k = 0; k < cn; k++) {
            const int n = chord[k].note; int best = -1; double bestCost = 1e9;
            for (int s = 0; s < kStrings; s++) {
                const int fret = n - open[s];
                if (fret < 0 || fret > 22 || used[s]) continue;
                const double cost = cn > 1 ? fret + s * 0.01 : std::abs(fret - handFret) + 0.5 * (fret > 12);
                if (cost < bestCost) { bestCost = cost; best = s; }
            }
            if (best < 0) best = n < open[0] ? 0 : kStrings - 1;   // out of range, or more notes than strings
            used[best] = true; out[k] = best;
            if (cn == 1) handFret = handFret * 0.6 + 0.4 * std::max(0, n - open[best]);
        }
    }

    void dispatchEvents() {
        while (!events.empty() && events.front().time <= now) {
            const Event e = events.front(); events.erase(events.begin());
            if (e.kind == Event::on) startNote(e.note, e.vel, e.stringHint);
            else stopNote(e.note);
        }
    }

    Voice* findVoice(int note, int stringIdx) {
        if (p.mode == 2) {   // legato: one voice
            return &voices[0];
        }
        // the same note still ringing: re-pluck it
        for (auto& v : voices) if (v.active && v.note == note) return &v;
        if (p.mode == 0) {   // guitar: one voice per string, a new note chokes the old one
            for (auto& v : voices) if (v.active && v.string == stringIdx) return &v;
        }
        for (auto& v : voices) if (!v.active) return &v;
        // steal the quietest
        Voice* q = &voices[0]; for (auto& v : voices) if (v.level < q->level) q = &v;
        return q;
    }

    void startNote(int note, double vel, int stringIdx) {
        Voice* v = findVoice(note, stringIdx);
        const bool legatoSlide = p.mode == 2 && v->active && v->keyDown;
        const bool ringing = v->active && v->note == note;
        const bool choke = v->active && !ringing && !legatoSlide;
        v->eng = this; v->string = std::max(0, stringIdx); v->note = note; v->keyDown = true; v->held = false; v->releasing = false;
        v->velocity = vel; v->releaseMul = 1; v->age = 0;
        // string: wound strings (the low three) are darker; higher notes on the same string decay faster
        const double f = noteHz(note + bendSemis());
        const double velB = 1 - p.velSense * (1 - vel);
        double bright = 700 * std::pow(22.0, p.bright) * std::pow(f / 196.0, 0.35) * (v->string < 3 ? 0.8 : 1.0) * (0.6 + 0.4 * velB);
        double decay = p.decayS * std::pow(110.0 / f, 0.5);
        const double mute = clampv(p.mute, 0.0, 1.0);
        decay *= 1 - 0.93 * mute; bright *= 1 - 0.75 * mute;
        bright = std::max(bright, f * (4.0 - 1.5 * mute));   // never so dark that the fundamental itself is filtered away
        v->decayS = decay; v->brightHz = bright; v->stiffA = -0.55 * p.stiffness;
        v->pickupAmt = p.pickup > 0 ? 0.85 : 0.0;
        v->pickupPos = 0.06 + 0.24 * (1 - p.pickupPos);   // bridge pickup sits close to the bridge
        // second course
        if (p.course == 1) { v->course2Gain = 0.85; v->detuneRatio = std::pow(2.0, p.detuneCents / 1200.0); }
        else if (p.course == 2 && v->string < 4) { v->course2Gain = 0.55; v->detuneRatio = 2.0 * std::pow(2.0, p.detuneCents * 0.2 / 1200.0); }
        else if (p.course == 2) { v->course2Gain = 0.8; v->detuneRatio = std::pow(2.0, p.detuneCents * 0.3 / 1200.0); }
        else v->course2Gain = 0;
        const double spread = p.width * 0.55;
        v->pan = clampv((v->string - 2.5) / 2.5 * spread * 0.5 + (p.course == 1 ? -spread : 0.0), -1.0, 1.0);
        v->course2Pan = clampv((v->string - 2.5) / 2.5 * spread * 0.5 + (p.course == 1 ? spread : 0.0), -1.0, 1.0);
        v->glideCoef = legatoSlide ? std::exp(-1.0 / (std::max(1.0, p.glideMs) * 0.001 * sr)) : 0.0;
        v->freqTarget = f; if (!legatoSlide) v->freq = f;
        if (!v->active || choke) { for (auto& c : v->course) c.clear(); }
        v->retune();
        v->active = true;
        if (legatoSlide) { v->excLen = v->excPos = 0; v->dampDip = 0.97; return; }   // slide/hammer: no new pluck
        if (ringing) v->dampDip = 0.6;   // the pick touches the ringing string first
        makeExcitation(*v, vel);
    }

    void stopNote(int note) {
        for (auto& v : voices)
            if (v.active && v.note == note && v.keyDown) {
                if (p.mode == 2) {   // legato: fall back to the previous held note? keep simple: release
                }
                v.keyDown = false;
                if (sustain) { v.held = true; continue; }
                v.releasing = true;
                v.releaseMul = std::max(0.004, p.releaseS / std::max(0.01, v.decayS));   // fret hand lifts: damped
                v.retune();
            }
    }

    // Pluck excitation: a short burst shaped by pick / finger / thumb, how hard, and where on the string.
    void makeExcitation(Voice& v, double vel) {
        const double period = sr / v.freq;
        const int len = std::min((int)v.exc.size() - 1, std::max(8, (int)(period * (p.pick == 0 ? 1.0 : 1.4))));
        const double velB = 1 - p.velSense * (1 - vel);
        const double amp = 0.55 * (0.25 + 0.75 * velB);
        // hardness: lowpass on the burst
        double hardHz;
        switch (p.pick) {
            case 0: hardHz = 1200 * std::pow(10.0, p.pickHard * 1.0 + velB * 0.4); break;   // pick
            case 1: hardHz = 500 * std::pow(6.0, p.pickHard + velB * 0.3); break;           // finger
            default: hardHz = 300 * std::pow(4.0, p.pickHard + velB * 0.2); break;          // thumb
        }
        const double a = std::exp(-2 * kPi * std::min(hardHz, sr * 0.45) / sr);
        auto& e = excScratch;
        double lp = 0, lp2 = 0;
        for (int i = 0; i < len; i++) {
            const double x = (double)i / len;
            double src;
            if (p.pick == 0) src = (rand01() * 2 - 1) * (1 - x) + 0.6 * std::sin(kPi * x);   // pick: bright noise + displacement
            else src = std::sin(kPi * x) * (1 - x * 0.5) + 0.15 * (rand01() * 2 - 1);        // flesh: smooth pulse
            lp = (1 - a) * src + a * lp; lp2 = (1 - a) * lp + a * lp2;
            e[(size_t)i] = lp2;
        }
        // pick position comb: the string can't move at the point it is plucked's mirror
        const int P = std::max(1, (int)std::lround(p.pickPos * period));
        double mean = 0;
        for (int i = 0; i < len; i++) {
            const double y = e[(size_t)i] - (i >= P ? e[(size_t)(i - P)] : 0.0);
            v.exc[(size_t)i] = (float)y; mean += y;
        }
        mean /= len; for (int i = 0; i < len; i++) v.exc[(size_t)i] = (float)((v.exc[(size_t)i] - mean) * amp);
        // normalise the burst so every note starts at a similar level
        double pk = 1e-9; for (int i = 0; i < len; i++) pk = std::max(pk, (double)std::abs(v.exc[(size_t)i]));
        for (int i = 0; i < len; i++) v.exc[(size_t)i] = (float)(v.exc[(size_t)i] / pk * amp * 0.5);
        v.excLen = len; v.excPos = 0;
        // pick noise: a short scrape outside the string loop
        const int nl = std::min((int)v.noise.size(), (int)(sr * (p.pick == 0 ? 0.006 : 0.01)));
        double hp = 0, prev = 0;
        for (int i = 0; i < nl; i++) {
            const double nz = rand01() * 2 - 1; hp = 0.7 * (hp + nz - prev); prev = nz;
            v.noise[(size_t)i] = (float)(hp * std::exp(-6.0 * i / nl) * p.pickNoise * 0.12 * velB * (p.pick == 0 ? 1.0 : 0.5));
        }
        v.noiseLen = nl; v.noisePos = 0;
    }

    double bendSemis() const { return bendS.y * p.bendSemis; }

    void controlUpdate() {
        bendS.a = std::exp(-16.0 / (0.01 * sr));
        const double b = bendS.next(bendTarget);
        vibPh += 16.0 * p.vibHz / sr; if (vibPh >= 1) vibPh -= 1;
        const double vibC = (p.vibCents + mod * 35.0) * std::sin(2 * kPi * vibPh);
        const double ratio = std::pow(2.0, (b * p.bendSemis) / 12.0 + vibC / 1200.0);
        for (auto& v : voices) {
            if (!v.active) continue;
            if (v.glideCoef > 0) v.glideCoef = std::exp(-1.0 / (std::max(1.0, p.glideMs) * 0.001 * sr));
            const double base = noteHz(v.note);
            const double target = base * ratio;
            if (std::abs(target - v.freqTarget) > target * 1e-7 || std::abs(v.freq - v.freqTarget) > target * 1e-7) {
                v.freqTarget = target;
                if (v.glideCoef <= 0) v.freq = target;
                v.retune();
            }
        }
    }

    // ------------------------------------------------------------------ body, pickups, amp, cab
    static constexpr int kModes = 9;
    void updateFilters() {
        filtersDirty = false;
        struct M { double f, q, g; };
        static const M dread[kModes] = { { 98, 7, 1.0 }, { 196, 11, 0.85 }, { 228, 9, 0.5 }, { 392, 8, 0.38 }, { 510, 8, 0.3 }, { 860, 6, 0.24 }, { 1240, 5, 0.2 }, { 2150, 4, 0.14 }, { 3400, 3, 0.1 } };
        static const M classical[kModes] = { { 96, 6, 0.9 }, { 185, 8, 0.9 }, { 250, 7, 0.45 }, { 410, 6, 0.3 }, { 580, 6, 0.25 }, { 950, 5, 0.16 }, { 1400, 4, 0.1 }, { 2300, 3, 0.06 }, { 3200, 3, 0.04 } };
        static const M archtop[kModes] = { { 110, 6, 0.6 }, { 230, 8, 0.7 }, { 330, 7, 0.6 }, { 480, 7, 0.45 }, { 700, 6, 0.4 }, { 1050, 5, 0.3 }, { 1600, 4, 0.22 }, { 2400, 4, 0.12 }, { 3600, 3, 0.06 } };
        const M* set = p.body == 3 ? classical : p.body == 5 ? archtop : dread;
        const double scale = (p.body == 2 ? 1.22 : p.body == 4 ? 0.9 : 1.0) / clampv(p.bodySize, 0.6, 1.6);
        double gsum = 0;
        for (int m = 0; m < kModes; m++) {
            for (int c = 0; c < 2; c++) bodyModes[c][(size_t)m].set(Svf::bandpass, sr, set[m].f * scale * (c ? 1.012 : 1.0), set[m].q);
            modeGain[(size_t)m] = set[m].g * (p.body == 2 && m == 0 ? 0.6 : 1.0);
            gsum += modeGain[(size_t)m];
        }
        bodyNorm = 2.2 / gsum;
        for (int c = 0; c < 2; c++) {
            switch (p.pickup) {
                case 1: pickupF[c].set(Svf::bell, sr, 4800, 1.6, 5); pickupGain = 0.9; break;          // single coil: glassy peak
                case 2: pickupF[c].set(Svf::bell, sr, 2600, 1.2, 4.5); pickupGain = 1.15; break;       // humbucker: thicker, lower peak
                case 3: pickupF[c].set(Svf::highShelf, sr, 3500, 0.7, 4); pickupGain = 0.9; break;      // piezo: bright, direct
                default: pickupF[c].set(Svf::bell, sr, 1000, 1, 0); pickupGain = 1; break;
            }
            lowCut[c].set(Svf::highpass, sr, p.pickup == 0 ? 65 : 45, 0.707);
            ampHp[c].set(Svf::highpass, sr, p.amp >= 3 ? 140 : 70, 0.707);
            ampDc[c].set(Svf::highpass, sr, 18, 0.707);   // tight low end into high gain
            tone[c][0].set(Svf::lowShelf, sr, 120, 0.7, p.bassDb + (p.amp >= 3 ? 2 : 0));
            tone[c][1].set(Svf::bell, sr, p.amp >= 3 ? 650 : 800, 0.8, p.midDb + (p.amp == 3 ? 1.5 : p.amp == 4 ? -4 : 0));
            tone[c][2].set(Svf::highShelf, sr, 2600, 0.7, p.trebleDb);
            // cab: low resonance, a mid scoop, presence, and the speaker's top-end roll-off
            const double cabRes = p.cab == 1 ? 110 : p.cab == 2 ? 100 : 85, cabLp = p.cab == 1 ? 5200 : p.cab == 2 ? 4600 : 4000;
            cabF[c][0].set(Svf::bell, sr, cabRes, 1.6, p.cab == 3 ? 4.5 : 3);
            cabF[c][1].set(Svf::bell, sr, 450, 1.0, p.cab == 1 ? -1.5 : -3);
            cabF[c][2].set(Svf::bell, sr, 2400, 1.4, 2.5);
            cabF[c][3].set(Svf::lowpass, sr, cabLp, 0.9);
            ageLp[c].set(Svf::lowpass, sr, 14000 * std::pow(0.25, p.age), 0.7);
        }
    }
    double shapeAmp(double x, int c) {
        switch (p.amp) {
            case 1: return std::tanh(x * 0.8) / 0.8;                                          // clean: gentle
            case 2: { const double y = std::tanh(x + 0.15) - std::tanh(0.15); return y; }      // crunch: asymmetric
            case 3: { const double y = std::tanh(1.8 * std::tanh(x + 0.1)) - std::tanh(1.8 * std::tanh(0.1)); return y * 0.9; }   // lead: two stages
            case 4: { const double r = x + 0.6 * std::abs(x); return std::tanh(3 * r) * 0.7; } // fuzz: rectified, square-ish
            default: (void)c; return x;
        }
    }
    // Keeps the level steady as drive rises: what a typical string level (0.2) comes out of the drive stage at,
    // turned back to that level. Clean stays a touch louder than its input, high gain sits a little under.
    double ampMakeup() {
        if (p.amp == 0) return 1;
        const double ref = 0.2, out = std::abs(shapeAmp(ref * dbToGain(p.driveDb), 0) - shapeAmp(-ref * dbToGain(p.driveDb), 0)) * 0.5;
        return ref / std::max(1e-3, out) * (p.amp >= 3 ? 0.5 : p.amp == 2 ? 0.75 : 1.0);
    }

    GuitarParams p; double sr = 48000;
    std::array<Voice, kVoices> voices;
    std::vector<Event> events; std::vector<Incoming> incoming;
    uint64_t now = 0; bool alternate = false, sustain = false; double handFret = 3, bendTarget = 0, mod = 0, vibPh = 0;
    Smooth bendS, volS;
    std::array<std::array<Svf, kModes>, 2> bodyModes; std::array<double, kModes> modeGain{}; double bodyNorm = 1;
    std::array<Svf, 2> pickupF, lowCut, ampHp, ageLp, ampDc; double pickupGain = 1;
    std::array<std::array<Svf, 3>, 2> tone; std::array<std::array<Svf, 4>, 2> cabF;
    std::array<Oversampler2x, 2> os;
    std::array<DelayLine, 2> chorusLine; double chorusPh = 0, agePh = 0;
    DelayDsp delay; VerbDsp verb;
    std::mt19937 rng{ 12345 };
    bool filtersDirty = true;
    std::vector<double> excScratch;
    static constexpr int kMaxChord = 32;
};
} // namespace lgn
