// LGN Strike: a synthesized drum machine. Every sound is generated, nothing is sampled.
//
// Eight pads. Each pad runs one of 25 drum models (808 / 909 / modern / acoustic kicks, an 808 boom, four snare
// families, clap, snap, rim, clave, closed and open hats, cymbal, cowbell, analog and Simmons toms, conga,
// shaker, tambourine, FM percussion, zap and noise FX), shaped by the same set of controls: tune, decay,
// pitch sweep, tone, snap (the noise layer), colour (a model-specific control), drive and click.
// Pads can choke each other (open / closed hats), one pad can be played melodically across the keyboard,
// and Humanize varies every hit slightly. The mix then runs through a machine character
// (Analog, MPC60, SP-1200, LinnDrum, Cassette, Vinyl or a modern clipper), glue compression, a room reverb
// send and an output clipper.
#pragma once
#include "Common.h"
#include "Comp.h"
#include "Reverb.h"

namespace lgn {

enum class DrumModel { kick808, kick909, kickModern, kickAcoustic, boom808, snare808, snare909, snareModern, snareAcoustic, clap, snap, rim, clave,
                       hatClosed, hatOpen, cymbal, cowbell, tomAnalog, tomSimmons, conga, shaker, tambourine, fmPerc, zap, noiseFx, count };

struct DrumModelInfo { const char* name; const char* family; const char* colour; double f0; };
inline const DrumModelInfo& drumModel(int m) {
    static const DrumModelInfo info[] = {
        { "Kick 808", "Kick", "Saturation", 50 },    { "Kick 909", "Kick", "Triangle", 55 },        { "Kick Modern", "Kick", "Punch", 48 },
        { "Kick Acoustic", "Kick", "Skin", 62 },     { "808 Boom", "Kick", "Harmonics", 44 },       { "Snare 808", "Snare", "Upper tone", 180 },
        { "Snare 909", "Snare", "Upper tone", 185 }, { "Snare Modern", "Snare", "Crack", 200 },      { "Snare Acoustic", "Snare", "Wires", 190 },
        { "Clap", "Clap", "Spread", 1200 },          { "Snap", "Clap", "Body", 2000 },              { "Rim", "Perc", "Wood", 1700 },
        { "Clave", "Perc", "Ring", 2500 },           { "Hat Closed", "Hat", "Noise", 205.3 },        { "Hat Open", "Hat", "Noise", 205.3 },
        { "Cymbal", "Cymbal", "Wash", 260 },         { "Cowbell", "Perc", "Detune", 540 },           { "Tom Analog", "Tom", "Noise", 110 },
        { "Tom Simmons", "Tom", "Noise", 150 },      { "Conga", "Perc", "Slap", 220 },              { "Shaker", "Perc", "Attack", 6000 },
        { "Tambourine", "Perc", "Jingle", 380 },     { "FM Perc", "Perc", "Ratio", 400 },           { "Zap", "FX", "Wave", 120 },
        { "Noise FX", "FX", "Sweep", 2000 } };
    return info[clampv(m, 0, (int)DrumModel::count - 1)];
}

// One pad's sound. Units: tune / pitch in semitones, times in ms, levels in dB, the rest 0..1 (pan -1..1).
struct PadParams {
    int model = 0;
    double tune = 0, decay = 400, pitch = 12, pdecay = 30, tone = 0.5, snap = 0.3, ndecay = 120, color = 0.3, drive = 0.1, click = 0.3;
    double level = 0, pan = 0, vel = 0.7, send = 0; int choke = 0, note = -1;   // note -1: not mapped
};

// The per-pad controls: one table drives the plugin's parameters, the sound library and the kits.
struct PadKey { const char* key; const char* name; float lo, hi, def, skewMid; const char* unit; };
inline const std::vector<PadKey>& padKeys() {
    static const std::vector<PadKey> k = {
        { "tune", "Tune", -24, 24, 0, 0, "st" },           { "decay", "Decay", 5, 5000, 400, 400, "ms" },     { "pitch", "Pitch Sweep", 0, 48, 12, 0, "st" },
        { "pdecay", "Sweep Time", 1, 1000, 30, 60, "ms" }, { "tone", "Tone", 0, 100, 50, 0, "%" },            { "snap", "Snap", 0, 100, 30, 0, "%" },
        { "ndecay", "Snap Decay", 5, 2000, 120, 200, "ms" }, { "color", "Colour", 0, 100, 30, 0, "%" },       { "drive", "Drive", 0, 100, 10, 0, "%" },
        { "click", "Click", 0, 100, 30, 0, "%" },          { "level", "Level", -36, 6, 0, 0, "dB" },          { "pan", "Pan", -100, 100, 0, 0, "%" },
        { "vel", "Velocity", 0, 100, 70, 0, "%" },         { "send", "Reverb Send", 0, 100, 0, 0, "%" } };
    return k;
}
// convert table units (percent where the unit is %) to PadParams
inline void padSet(PadParams& p, const std::string& key, double v) {
    if (key == "model") p.model = (int)std::lround(v);
    else if (key == "tune") p.tune = v; else if (key == "decay") p.decay = v; else if (key == "pitch") p.pitch = v; else if (key == "pdecay") p.pdecay = v;
    else if (key == "tone") p.tone = v / 100; else if (key == "snap") p.snap = v / 100; else if (key == "ndecay") p.ndecay = v; else if (key == "color") p.color = v / 100;
    else if (key == "drive") p.drive = v / 100; else if (key == "click") p.click = v / 100; else if (key == "level") p.level = v; else if (key == "pan") p.pan = v / 100;
    else if (key == "vel") p.vel = v / 100; else if (key == "send") p.send = v / 100; else if (key == "choke") p.choke = (int)std::lround(v); else if (key == "note") p.note = (int)std::lround(v);
}

namespace drum {
struct Rng { uint32_t s = 0x1234567u; double next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s / 4294967296.0) * 2 - 1; } };
// band-limited square (polyBLEP) for the metallic banks
struct Sq {
    double ph = 0, inc = 0;
    static double blep(double t, double dt) { if (t < dt) { t /= dt; return t + t - t * t - 1; } if (t > 1 - dt) { t = (t - 1) / dt; return t * t + t + t + 1; } return 0; }
    double next() {
        ph += inc; if (ph >= 1) ph -= 1;
        double v = ph < 0.5 ? 1.0 : -1.0;
        v += blep(ph, inc); double t2 = ph + 0.5; if (t2 >= 1) t2 -= 1; v -= blep(t2, inc);
        return v;
    }
};
// exponential decay: reaches -60 dB after `ms`
inline double decayCoef(double ms, double sr) { return std::exp(-6.907755 / std::max(1.0, ms * 0.001 * sr)); }
} // namespace drum

// ---------------------------------------------------------------- one drum hit
class DrumVoice {
public:
    bool active() const { return on; }
    void start(const PadParams& pp, double sampleRate, double velocity, double tuneOffset, drum::Rng& r) {
        p = pp; sr = sampleRate; on = true; t = 0; n = 0; fade = 1; fadeStep = 0; rng.s = (uint32_t)(r.next() * 2147483647.0) * 2654435761u + 0x9E3779B9u; if (rng.s == 0) rng.s = 1;
        const double vs = clampv(p.vel, 0.0, 1.0);
        gain = dbToGain(p.level) * (1 - vs * (1 - clampv(velocity, 0.0, 1.0)));
        bright = 1 + vs * (velocity - 0.8) * 0.3;   // harder hits are a little brighter
        const auto& m = drumModel(p.model);
        f = m.f0 * std::exp2((p.tune + tuneOffset) / 12.0);
        ph1 = ph2 = 0; ampE = 1; noiseE = 1; pitchE = 1; clickE = 1; tail = 0; burst = 0;
        aC = drum::decayCoef(p.decay, sr); nC = drum::decayCoef(p.ndecay, sr); pC = std::exp(-1.0 / std::max(1.0, p.pdecay * 0.001 * sr));
        cC = drum::decayCoef(1.5 + 6 * (1 - p.click), sr);
        pitchMul = std::exp2(p.pitch / 12.0) - 1;
        driveG = 1 + p.drive * 9; driveN = 1.0 / std::tanh(driveG);
        for (auto& s : flt) s.reset();
        flt[2].set(Svf::highpass, sr, std::min(sr * 0.45, 4000.0), 0.7);   // transients and extra noise
        setupModel();
    }
    void release(double ms) { if (on) fadeStep = 1.0 / std::max(1.0, ms * 0.001 * sr); }   // a choke or a retrigger: fade out fast
    double next() {
        if (!on) return 0;
        double x = render();
        // drive (every model)
        if (p.drive > 0.001) x = std::tanh(x * driveG) * driveN;
        // 0.15 ms declick at the start
        const double atk = n < atkN ? (double)n / atkN : 1.0;
        x *= gain * atk * fade;
        if (fadeStep > 0) { fade -= fadeStep; if (fade <= 0) { on = false; return 0; } }
        t += 1.0 / sr; n++;
        if (n > minLen && ampE < 1e-4 && noiseE < 1e-4 && tail < 1e-4) on = false;
        return x;
    }
    double pan() const { return p.pan; }
    double send() const { return p.send; }

private:
    double sine(double& ph, double fr) { ph += fr / sr; if (ph >= 1) ph -= 1; return std::sin(2 * kPi * ph); }
    double tri(double& ph, double fr) { ph += fr / sr; if (ph >= 1) ph -= 1; return 1 - 4 * std::abs(ph - 0.5); }
    double white() { return rng.next(); }
    void setupModel() {
        atkN = std::max(1, (int)(0.00015 * sr)); minLen = (int)(0.005 * sr);
        const auto M = (DrumModel)p.model;
        const double toneHz = 200 * std::pow(80.0, clampv(p.tone * bright, 0.0, 1.0));   // 200 Hz .. 16 kHz
        switch (M) {
            case DrumModel::kick808: case DrumModel::kick909: case DrumModel::kickModern: case DrumModel::boom808:
                flt[0].set(Svf::lowpass, sr, std::min(sr * 0.45, toneHz * 0.6 + 400), 0.7);    // click / noise colour
                flt[1].set(Svf::highpass, sr, 900, 0.7);
                break;
            case DrumModel::kickAcoustic:
                flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, 1500 + toneHz * 0.3), 1.2);   // beater
                flt[1].set(Svf::lowpass, sr, std::min(sr * 0.45, toneHz + 200), 0.7);
                break;
            case DrumModel::snare808: flt[0].set(Svf::highpass, sr, std::min(sr * 0.45, 600 + toneHz * 0.4), 0.7); break;
            case DrumModel::snare909: flt[0].set(Svf::lowpass, sr, std::min(sr * 0.45, 3000 + toneHz * 0.7), 0.7); flt[1].set(Svf::highpass, sr, 450, 0.7); break;
            case DrumModel::snareModern: flt[0].set(Svf::highpass, sr, std::min(sr * 0.45, 1200 + toneHz * 0.3), 0.7); flt[1].set(Svf::bell, sr, 5000, 0.8, 4); break;
            case DrumModel::snareAcoustic: flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, 2500 + toneHz * 0.3), 0.8); flt[1].set(Svf::highpass, sr, 300, 0.7); break;
            case DrumModel::clap: flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, 600 + toneHz * 0.25), 1.6); flt[1].set(Svf::highpass, sr, 350, 0.7); break;
            case DrumModel::snap: flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, 1200 + toneHz * 0.3), 2.5); break;
            case DrumModel::rim: flt[0].set(Svf::highpass, sr, 300, 0.7); break;
            case DrumModel::clave: flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, f), 18); break;
            case DrumModel::hatClosed: case DrumModel::hatOpen:
                flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, 3500 + toneHz * 0.5), 0.9);
                flt[1].set(Svf::highpass, sr, std::min(sr * 0.45, 5000 + toneHz * 0.15), 0.7);
                break;
            case DrumModel::cymbal: flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, 2500 + toneHz * 0.4), 0.7); flt[1].set(Svf::highpass, sr, 1800, 0.7); break;
            case DrumModel::cowbell: flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, 900 + toneHz * 0.15), 2.5); break;
            case DrumModel::tomAnalog: case DrumModel::tomSimmons: flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, 400 + toneHz * 0.25), 1.2); break;
            case DrumModel::conga: flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, f), 12 + 20 * (1 - p.color)); flt[1].set(Svf::bandpass, sr, std::min(sr * 0.45, 2500.0), 1.5); break;
            case DrumModel::shaker: flt[0].set(Svf::highpass, sr, std::min(sr * 0.45, 2500 + toneHz * 0.4), 0.8); break;
            case DrumModel::tambourine: flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, 6000 + toneHz * 0.2), 1.5); flt[1].set(Svf::highpass, sr, 3000, 0.7); break;
            case DrumModel::noiseFx: flt[0].set(Svf::bandpass, sr, std::min(sr * 0.45, toneHz), 2.0 + 6 * p.color); break;
            default: break;
        }
        // metallic banks (808 hat / cymbal ratios), band-limited squares
        static const double ratios[6] = { 1.0, 1.4827, 1.8003, 2.5460, 2.6300, 3.8967 };
        for (int i = 0; i < 6; i++) { metal[(size_t)i].ph = 0.13 * i; metal[(size_t)i].inc = std::min(0.45, f * ratios[i] * (M == DrumModel::cymbal ? 1.37 : 1.0) / sr); }
        for (int i = 0; i < 6; i++) { metal2[(size_t)i].ph = 0.07 * i; metal2[(size_t)i].inc = std::min(0.45, f * ratios[i] * 1.91 / sr); }
        if (M == DrumModel::cowbell) { metal[0].inc = std::min(0.45, f / sr); metal[1].inc = std::min(0.45, f * (1.4815 + 0.05 * p.color) / sr); }
        if (M == DrumModel::tambourine) for (int i = 0; i < 6; i++) metal[(size_t)i].inc = std::min(0.45, f * 16 * ratios[i] / sr);
        dc = 0;
    }
    double clickBurst() {   // the shared transient: a few samples of filtered noise plus a 1-cycle pulse
        const double c = clickE; clickE *= cC;
        return (white() * 0.5 + (n < (int)(0.0003 * sr) ? 0.45 : 0.0)) * c;
    }
    double render() {
        const auto M = (DrumModel)p.model;
        const double pe = pitchE; pitchE *= pC;
        const double fr = f * (1 + pitchMul * pe);
        double y = 0;
        switch (M) {
            case DrumModel::kick808: {   // bridged-T style sine with a gentle sweep; colour = second-harmonic saturation
                double b = sine(ph1, fr);
                if (p.color > 0.001) b = std::tanh(b * (1 + 3 * p.color)) / std::tanh(1 + 3 * p.color) + 0.15 * p.color * b * b;
                y = b * ampE + flt[0].process(clickBurst() * p.click * 0.6) + flt[1].process(white() * p.snap * 0.25 * noiseE);
                break;
            }
            case DrumModel::kick909: {   // sine / triangle body, fast deep sweep, noise click
                const double b = sine(ph1, fr) * (1 - p.color) + tri(ph2, fr) * p.color;
                y = b * ampE + flt[0].process(white()) * p.snap * 0.6 * noiseE + flt[1].process(clickBurst()) * p.click * 0.8;
                break;
            }
            case DrumModel::kickModern: {   // clean sine with a punch bump and a bright transient, clipped
                const double b = sine(ph1, fr);
                const double punch = 1 + p.color * 1.5 * std::exp(-t / 0.012);
                y = std::tanh(b * ampE * punch * (1 + p.color)) * (0.9 + 0.1 * (1 - p.color)) + flt[1].process(clickBurst() * p.click + white() * p.snap * 0.4 * noiseE);
                break;
            }
            case DrumModel::kickAcoustic: {   // body + an inharmonic skin mode + beater
                const double b = sine(ph1, fr) + 0.35 * p.color * sine(ph2, fr * 1.59) * std::exp(-t / 0.05);
                y = flt[1].process(b * ampE) + flt[0].process(white()) * (p.snap * 0.8 + 0.1) * noiseE + clickBurst() * p.click * 0.4;
                break;
            }
            case DrumModel::boom808: {   // long sub with harmonics on top (the colour control)
                const double b = sine(ph1, fr);
                const double h = p.color > 0.001 ? std::tanh(b * (1 + 6 * p.color)) / std::tanh(1 + 6 * p.color) : b;
                y = (b * (1 - p.color * 0.5) + h * p.color * 0.5) * ampE + flt[0].process(clickBurst()) * p.click * 0.4;
                break;
            }
            case DrumModel::snare808: case DrumModel::snare909: {   // two tuned bodies + filtered noise
                const double b1 = M == DrumModel::snare808 ? sine(ph1, fr) : tri(ph1, fr);
                const double b2 = M == DrumModel::snare808 ? sine(ph2, fr * 1.833) : tri(ph2, fr * 1.784);
                double nz = flt[0].process(white()); if (M == DrumModel::snare909) nz = flt[1].process(nz);
                y = (b1 * (1 - 0.6 * p.color) + b2 * (0.3 + 0.7 * p.color)) * 0.7 * ampE + nz * p.snap * 1.3 * noiseE + clickBurst() * p.click * 0.3;
                break;
            }
            case DrumModel::snareModern: {   // pitched body, bright noise, a clap-like double hit (colour)
                const double b = sine(ph1, fr) * 0.8;
                double env = noiseE; if (p.color > 0.01 && t < 0.012) env *= (std::fmod(t, 0.006) < 0.0015 ? 1.0 : 0.35);
                y = b * ampE + flt[1].process(flt[0].process(white())) * p.snap * 1.4 * env + clickBurst() * p.click * 0.6;
                break;
            }
            case DrumModel::snareAcoustic: {   // shell modes + wires (colour = wire amount) + rim crack
                const double b = sine(ph1, fr) + 0.5 * sine(ph2, fr * 1.6) * std::exp(-t / 0.04) + 0.3 * std::sin(2 * kPi * fr * 2.3 * t) * std::exp(-t / 0.025);
                const double wires = flt[1].process(flt[0].process(white()));
                y = b * 0.55 * ampE + wires * (0.4 + 1.2 * p.color) * p.snap * 1.2 * noiseE + clickBurst() * p.click * 0.7;
                break;
            }
            case DrumModel::clap: {   // several noise bursts a few ms apart, then a tail
                const double spread = 0.004 + 0.012 * p.color; const int bursts = 4;
                double env;
                if (t < spread * (bursts - 1)) { const double ph = std::fmod(t, spread); env = std::exp(-ph / 0.0022); }
                else { env = std::exp(-(t - spread * (bursts - 1)) / std::max(0.001, p.decay * 0.001 / 6.9)); }
                y = flt[1].process(flt[0].process(white())) * env * 1.6 + clickBurst() * p.click * 0.3;
                tail = env;
                break;
            }
            case DrumModel::snap: {
                const double env = std::exp(-t / std::max(0.001, p.decay * 0.001 / 6.9));
                y = flt[0].process(white()) * env * 2.2 + sine(ph1, fr * 0.25) * p.color * 0.4 * env + clickBurst() * p.click * 0.4;
                tail = env;
                break;
            }
            case DrumModel::rim: {   // 808-style: a high and a low ping through a little distortion
                const double b = sine(ph1, fr) * 0.8 + tri(ph2, fr * 0.28) * (0.3 + 0.5 * p.color);
                y = flt[0].process(std::tanh(b * 2.5)) * ampE * 0.6 + clickBurst() * p.click * 0.4;
                break;
            }
            case DrumModel::clave: {   // a ringing resonator struck by a click
                const double exc = n < 3 ? (n == 0 ? 1.0 : -0.5) : 0.0;
                y = flt[0].process(exc * 30) * (0.6 + 0.4 * p.color) + sine(ph1, fr) * ampE * 0.5;
                tail = ampE;
                break;
            }
            case DrumModel::hatClosed: case DrumModel::hatOpen: case DrumModel::cymbal: {   // six squares (808 ratios) and/or noise
                double mtl = 0; for (auto& s : metal) mtl += s.next();
                if (M == DrumModel::cymbal) for (auto& s : metal2) mtl += 0.6 * s.next();
                mtl *= M == DrumModel::cymbal ? 0.12 : 0.17;
                const double mix = clampv(p.color, 0.0, 1.0);
                double src = mtl * (1 - mix) + white() * mix * 0.6;
                double out = flt[1].process(flt[0].process(src));
                const double extra = (M == DrumModel::cymbal ? white() * p.snap * 0.3 * noiseE : 0.0) + clickBurst() * p.click * 0.25;
                y = out * 2.2 * ampE + flt[2].process(extra);
                break;
            }
            case DrumModel::cowbell: {   // two squares through a band-pass, a fast drop then a ringing tail
                const double src = metal[0].next() + metal[1].next();
                const double env = 0.6 * std::exp(-t / 0.012) + 0.4 * ampE;
                y = flt[0].process(src) * env * 1.3;
                tail = env;
                break;
            }
            case DrumModel::tomAnalog: case DrumModel::tomSimmons: {
                const double b = M == DrumModel::tomAnalog ? sine(ph1, fr) : sine(ph1, fr) * 0.7 + tri(ph2, fr) * 0.3;
                y = b * ampE + flt[0].process(white()) * p.snap * (0.3 + 0.7 * p.color) * noiseE + clickBurst() * p.click * 0.3;
                break;
            }
            case DrumModel::conga: {   // a high-Q resonator plus the skin's pitch drop; colour = slap
                const double exc = n < 2 ? 1.0 : 0.0;
                y = flt[0].process(exc * 18) * 0.8 + sine(ph1, fr) * ampE * 0.6 + flt[1].process(white()) * p.color * 0.8 * noiseE + clickBurst() * p.click * 0.2;
                tail = ampE;
                break;
            }
            case DrumModel::shaker: {   // noise with a soft attack (colour) and a short decay
                const double att = 0.002 + 0.04 * p.color;
                const double env = (t < att ? t / att : 1.0) * ampE;
                y = flt[0].process(white()) * env * 1.6;
                break;
            }
            case DrumModel::tambourine: {   // jingles: high metal clusters retriggered twice, plus noise
                double mtl = 0; for (auto& s : metal) mtl += s.next(); mtl *= 0.15;
                const double jing = 0.002 + 0.02 * p.color;
                const double env = (t < jing * 2 ? 0.6 + 0.4 * std::exp(-std::fmod(t, jing) / 0.003) : 1.0) * ampE;
                y = flt[1].process(flt[0].process(mtl + white() * 0.5)) * env * 2.0;
                break;
            }
            case DrumModel::fmPerc: {   // two-operator FM: the pitch sweep is the FM index, colour picks the ratio
                static const double rat[8] = { 0.5, 1.0, 1.41, 2.0, 2.76, 3.5, 5.19, 7.0 };
                const double ratio = rat[clampv((int)(p.color * 7.99), 0, 7)];
                ph2 += f * ratio / sr; if (ph2 >= 1) ph2 -= 1;
                const double mod = std::sin(2 * kPi * ph2) * (p.pitch / 6.0) * pe;
                ph1 += f / sr; if (ph1 >= 1) ph1 -= 1;
                y = std::sin(2 * kPi * ph1 + mod) * ampE + clickBurst() * p.click * 0.3;
                break;
            }
            case DrumModel::zap: {   // a deep, fast laser sweep; colour = sine to triangle
                y = (sine(ph1, fr) * (1 - p.color) + tri(ph2, fr) * p.color) * ampE;
                break;
            }
            case DrumModel::noiseFx: {   // filtered noise whose band sweeps with the pitch control
                if ((n & 15) == 0) flt[0].set(Svf::bandpass, sr, clampv(200 * std::pow(80.0, p.tone) * (1 + pitchMul * pe), 40.0, sr * 0.45), 2.0 + 6 * p.color);
                y = flt[0].process(white()) * ampE * 2.0;
                break;
            }
            default: break;
        }
        ampE *= aC; noiseE *= nC;
        if (M == DrumModel::clap || M == DrumModel::snap || M == DrumModel::clave || M == DrumModel::cowbell || M == DrumModel::conga) { /* tail set above */ }
        else tail = 0;
        // remove any DC the shaping added
        dc += (y - dc) * 0.0008;
        return y - dc;
    }

    PadParams p; double sr = 48000; bool on = false; double t = 0; int n = 0, atkN = 8, minLen = 0;
    double gain = 1, bright = 1, f = 100, ph1 = 0, ph2 = 0, ampE = 0, noiseE = 0, pitchE = 0, clickE = 0, tail = 0, burst = 0;
    double aC = 0, nC = 0, pC = 0, cC = 0, pitchMul = 0, driveG = 1, driveN = 1, fade = 1, fadeStep = 0, dc = 0;
    std::array<Svf, 3> flt; std::array<drum::Sq, 6> metal, metal2; drum::Rng rng;
};

// ---------------------------------------------------------------- the machine
struct StrikeGlobal {
    int character = 0; double amount = 0.6, glue = 0.2, clip = 0.2, verbMix = 1.0, verbSize = 0.35, humanize = 0.15, width = 1.0, volumeDb = 0;
    int melodicPad = -1;   // -1 off, else notes 48..84 play this pad chromatically (C3 = original pitch)
};
inline const char* const* strikeCharacters() { static const char* n[] = { "Clean", "Analog", "MPC60", "SP-1200", "LinnDrum", "Cassette", "Vinyl", "Modern" }; return n; }

class StrikeDsp {
public:
    static constexpr int kPads = 8;
    void prepare(double rate) {
        sr = rate; for (auto& v : voices) for (auto& x : v) x = DrumVoice();
        CompParams cp; cp.style = CompDsp::glue; comp.prepare(sr); comp.set(cp);
        verb.prepare(sr); setVerb();
        hold[0] = hold[1] = 0; crushPh = 0; wowPh = 0; for (auto& d : wowLine) d.prepare((int)(0.02 * sr) + 8);
        for (int c = 0; c < 2; c++) { aa[c].set(Svf::lowpass, sr, 18000, 0.7); post[c].set(Svf::lowpass, sr, 18000, 0.7); hp[c].set(Svf::highpass, sr, 25, 0.7); }
        hiss.s = 777; lastChar = -1;
    }
    void setPad(int i, const PadParams& p) { pads[(size_t)clampv(i, 0, kPads - 1)] = p; }
    const PadParams& pad(int i) const { return pads[(size_t)clampv(i, 0, kPads - 1)]; }
    void setGlobal(const StrikeGlobal& g) { glob = g; }

    // a MIDI note: which pad(s) it plays
    void noteOn(int note, double vel, int offset) { queue.push_back({ offset, note, vel }); }
    void trigger(int padIndex, double vel, double tuneOffset = 0) {
        const int i = clampv(padIndex, 0, kPads - 1);
        auto pp = pads[(size_t)i];
        if (glob.humanize > 0) {   // a living drummer: every hit a little different
            const double h = glob.humanize;
            vel = clampv(vel * (1 + 0.25 * h * rng.next()), 0.05, 1.0);
            tuneOffset += 0.18 * h * rng.next();
            pp.decay *= 1 + 0.12 * h * rng.next();
            pp.tone = clampv(pp.tone + 0.05 * h * rng.next(), 0.0, 1.0);
        }
        // choke: every other pad in the same group is cut
        if (pp.choke > 0) for (int k = 0; k < kPads; k++) if (k != i && pads[(size_t)k].choke == pp.choke) for (auto& v : voices[(size_t)k]) v.release(6);
        auto& slot = voices[(size_t)i];
        slot[(size_t)cur[(size_t)i]].release(4);               // the previous hit fades in 4 ms
        cur[(size_t)i] = (cur[(size_t)i] + 1) % 2;
        slot[(size_t)cur[(size_t)i]].start(pp, sr, vel, tuneOffset, rng);
        flash[(size_t)i] = 1.0f;
    }
    void allOff() { for (auto& v : voices) for (auto& x : v) x.release(10); queue.clear(); }
    std::array<float, kPads> flash{};   // for the pad lights (the editor fades them)

    void process(float* L, float* R, int n) {
        std::stable_sort(queue.begin(), queue.end(), [](const Ev& a, const Ev& b) { return a.off < b.off; });
        size_t qi = 0;
        if (glob.character != lastChar) { setCharacter(); lastChar = glob.character; }
        setVerb();
        CompParams cp; cp.style = CompDsp::glue; cp.thresh = -glob.glue * 24; cp.ratio = 2 + 2 * glob.glue; cp.attackMs = 10; cp.releaseMs = 120; cp.autoMakeup = true; cp.kneeDb = 6; cp.scHpf = 80;
        comp.set(cp);
        sendL.assign((size_t)n, 0.0f); sendR.assign((size_t)n, 0.0f);
        for (int i = 0; i < n; i++) {
            while (qi < queue.size() && queue[qi].off <= i) { route(queue[qi]); qi++; }
            double l = 0, r = 0, sl = 0, sr2 = 0;
            for (int k = 0; k < kPads; k++) for (auto& v : voices[(size_t)k]) {
                if (!v.active()) continue;
                const double x = v.next(), pn = v.pan();
                const double gl = std::cos((pn + 1) * kPi / 4) * 1.41421356, gr = std::sin((pn + 1) * kPi / 4) * 1.41421356;
                l += x * gl; r += x * gr;
                if (v.send() > 0.001) { sl += x * gl * v.send(); sr2 += x * gr * v.send(); }
            }
            L[i] = (float)l; R[i] = (float)r; sendL[(size_t)i] = (float)sl; sendR[(size_t)i] = (float)sr2;
        }
        queue.clear();
        // reverb send (100% wet), returned into the mix
        if (glob.verbMix > 0.001) {
            float* s[2] = { sendL.data(), sendR.data() };
            verb.process(s, 2, n);
            for (int i = 0; i < n; i++) { L[i] += sendL[(size_t)i] * (float)glob.verbMix; R[i] += sendR[(size_t)i] * (float)glob.verbMix; }
        }
        character(L, R, n);
        float* ch[2] = { L, R };
        if (glob.glue > 0.001) comp.process(ch, 2, n);
        const double vol = dbToGain(glob.volumeDb), drive = 1 + glob.clip * 4;
        for (int i = 0; i < n; i++) {
            double l = L[i], r = R[i];
            const double m = 0.5 * (l + r), s = 0.5 * (l - r) * glob.width; l = m + s; r = m - s;
            l *= vol; r *= vol;
            // output clipper: soft above -6 dB, scaled by the Clip control, never past 0 dBFS
            const double knee = 0.92 - 0.42 * glob.clip;
            l = clipOut(l * drive, knee) / std::sqrt(drive); r = clipOut(r * drive, knee) / std::sqrt(drive);
            L[i] = (float)clampv(l, -0.995, 0.995); R[i] = (float)clampv(r, -0.995, 0.995);
        }
    }
    // render one pad alone, dry, for the editor's waveform and the level tool
    static void preview(const PadParams& p, double sr, float* out, int n) {
        DrumVoice v; drum::Rng r; v.start(p, sr, 1.0, 0, r);
        for (int i = 0; i < n; i++) out[i] = (float)v.next();
    }

private:
    struct Ev { int off, note; double vel; };
    void route(const Ev& e) {
        if (glob.melodicPad >= 0 && e.note >= 48 && e.note <= 84) { trigger(glob.melodicPad, e.vel, e.note - 60); return; }
        for (int k = 0; k < kPads; k++) if (pads[(size_t)k].note == e.note) trigger(k, e.vel);
    }
    static double clipOut(double x, double k) { const double a = std::abs(x); return a <= k ? x : std::copysign(k + (0.99 - k) * std::tanh((a - k) / (0.99 - k)), x); }
    void setVerb() { VerbParams vp; vp.mix = 1.0; vp.sizePct = 15 + 70 * glob.verbSize; vp.decayS = 0.3 + 2.7 * glob.verbSize; vp.character = glob.verbSize > 0.6 ? VerbDsp::plate : VerbDsp::room; vp.lowCutHz = 200; vp.dampHz = 7000; vp.preMs = 8; verb.set(vp); }
    void setCharacter() {
        // anti-alias / output filters per machine
        double aaHz = 20000, postHz = 20000;
        switch (glob.character) {
            case 1: postHz = 15000; break;            // analog
            case 2: aaHz = 15000; postHz = 14000; break;   // MPC60: 40 kHz, 12-bit, filtered
            case 3: aaHz = 20000; postHz = 11000; break;   // SP-1200: 26 kHz, unfiltered input (it aliases), soft output filter
            case 4: aaHz = 12000; postHz = 11000; break;   // LinnDrum: 28 kHz, 8-bit companded
            case 5: postHz = 9000; break;             // cassette
            case 6: postHz = 12000; break;            // vinyl
            default: break;
        }
        for (int c = 0; c < 2; c++) { aa[c].set(Svf::lowpass, sr, std::min(aaHz, sr * 0.45), 0.7); post[c].set(Svf::lowpass, sr, std::min(postHz, sr * 0.45), 0.7); }
    }
    void character(float* L, float* R, int n) {
        const int c = glob.character; const double a = clampv(glob.amount, 0.0, 1.0);
        if (c == 0 || a < 0.001) return;
        double rate = 0, bits = 0;
        if (c == 2) { rate = 40000; bits = 12; } else if (c == 3) { rate = 26040; bits = 12; } else if (c == 4) { rate = 28000; bits = 8; }
        for (int i = 0; i < n; i++) {
            double x[2] = { L[i], R[i] }, dry[2] = { L[i], R[i] };
            if (c == 1 || c == 5) for (auto& v : x) v = std::tanh(v * (1 + 1.5 * a)) / (1 + 0.5 * a);   // tape warmth
            if (c == 7) for (auto& v : x) v = std::tanh(v * (1 + 3 * a)) / std::tanh(1 + 3 * a) * 0.9;   // modern loudness
            if (rate > 0) {
                for (int k = 0; k < 2; k++) x[k] = aa[k].process(x[k]);
                crushPh += std::min(1.0, rate / sr);
                if (crushPh >= 1) {
                    crushPh -= 1;
                    for (int k = 0; k < 2; k++) {
                        double v = x[k];
                        if (c == 4) { const double mu = 255; v = std::copysign(std::log1p(mu * std::min(1.0, std::abs(v))) / std::log1p(mu), v); }   // companded 8-bit
                        const double q = std::exp2(bits - 1); v = std::round(v * q) / q;
                        if (c == 4) { const double mu = 255; v = std::copysign((std::pow(1 + mu, std::abs(v)) - 1) / mu, v); }
                        hold[k] = v;
                    }
                }
                x[0] = hold[0]; x[1] = hold[1];
                if (c == 2) for (auto& v : x) v = std::tanh(v * 1.3) / 1.2;   // the MPC's warm converter
            }
            if (c == 5 || c == 6) {   // wow / flutter through a short modulated delay
                wowPh += (c == 5 ? 0.9 : 0.55) / sr; if (wowPh >= 1) wowPh -= 1;
                const double d = (0.004 + 0.0012 * a * std::sin(2 * kPi * wowPh) + 0.0003 * a * std::sin(2 * kPi * wowPh * 11.3)) * sr;
                for (int k = 0; k < 2; k++) { wowLine[k].push((float)x[k]); x[k] = wowLine[k].read(d); }
            }
            for (int k = 0; k < 2; k++) x[k] = hp[k].process(post[k].process(x[k]));
            if (c == 5) for (auto& v : x) v += hiss.next() * 0.002 * a;
            if (c == 6) { const double cr = hiss.next(); if (std::abs(cr) > 0.9997) crackle = cr * 0.25 * a; crackle *= 0.9; for (auto& v : x) v += crackle + hiss.next() * 0.0006 * a; }
            L[i] = (float)(dry[0] + (x[0] - dry[0]) * a); R[i] = (float)(dry[1] + (x[1] - dry[1]) * a);
        }
    }

    double sr = 48000;
    std::array<PadParams, kPads> pads{}; StrikeGlobal glob;
    std::array<std::array<DrumVoice, 2>, kPads> voices; std::array<int, kPads> cur{};
    std::vector<Ev> queue; drum::Rng rng, hiss;
    CompDsp comp; VerbDsp verb; std::vector<float> sendL, sendR;
    Svf aa[2], post[2], hp[2]; double hold[2] = {}, crushPh = 0, wowPh = 0, crackle = 0; std::array<DelayLine, 2> wowLine; int lastChar = -1;
};
} // namespace lgn
