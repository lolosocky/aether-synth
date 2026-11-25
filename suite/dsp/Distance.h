// LGN Distance: places a sound at a distance and angle in a space, the way it would really sound there.
//
//   level      falls 6 dB per doubling of distance (Keep Level gives some of it back so the mix doesn't vanish)
//   air        high frequencies are absorbed more the further sound travels
//   proximity  very close sources gain bass, like a voice right on the mic
//   floor      a reflection off the ground arrives a moment after the direct sound
//   room       early reflections and a reverb tail from the chosen space; their level stays put while the direct
//              sound falls away, so distant sounds are mostly room
//   angle      a source to one side reaches that ear first and louder; the far ear is shadowed by the head
//   doppler    moving the distance changes the travel time, so movement bends the pitch like a passing car
// Plain C++ (no JUCE) so it can be tested on its own.
#pragma once
#include "Common.h"
#include "Reverb.h"

namespace lgn {

struct DistanceParams { double distance = 0.35, angleDeg = 0, keepLevel = 0.5, air = 1.0, doppler = 0.5, width = 1.0, outDb = 0; int space = 0; };

class DistanceDsp {
public:
    enum Space { studio, room, club, hall, street, stadium, field, numSpaces };
    struct SpaceInfo { const char* name; double decayS, size, reverb, reflections, damp, height; };
    static const SpaceInfo& space(int s) {
        static const SpaceInfo t[numSpaces] = {
            { "Studio",  0.35, 20, 0.10, 0.35, 9000, 1.4 },
            { "Room",    0.7,  35, 0.22, 0.55, 7000, 1.4 },
            { "Club",    1.3,  55, 0.30, 0.5,  5500, 1.6 },
            { "Hall",    2.4,  85, 0.38, 0.45, 6000, 1.6 },
            { "Street",  0.9,  60, 0.12, 0.3,  5000, 1.6 },
            { "Stadium", 3.5,  100, 0.40, 0.4, 4500, 1.8 },
            { "Field",   0.0,  0,  0.0,  0.0,  8000, 1.6 } };
        return t[clampv(s, 0, numSpaces - 1)];
    }
    // knob (0..1) to metres: 0.15 m (on the mic) to 150 m, evenly spread in perceived distance
    static double metres(double knob) { return 0.15 * std::pow(1000.0, clampv(knob, 0.0, 1.0)); }
    static double knobFor(double m) { return std::log(std::max(m, 0.15) / 0.15) / std::log(1000.0); }

    void prepare(double rate) {
        sr = rate;
        const int maxTravel = (int)(0.09 * sr) + 64;
        for (auto& d : travel) d.prepare(maxTravel);
        for (auto& d : ear) d.prepare((int)(0.002 * sr) + 8);
        for (auto& d : refl) d.prepare((int)(0.12 * sr) + 64);
        for (int c = 0; c < 2; c++) { airLp[c].reset(); airShelf[c].reset(); prox[c].reset(); shadow[c].reset(); floorLp[c].reset(); reflLp[c].reset(); }
        verb.prepare(sr);
        distS.setTime(sr / 16, 0.06); distS.reset(metres(p.distance));   // these update every 16 samples
        angS.setTime(sr / 16, 0.05); angS.reset(p.angleDeg);
        wetIn[0].assign(kChunk, 0.0f); wetIn[1].assign(kChunk, 0.0f);
        gainS.setTime(sr, 0.03); gainS.reset(1);
        counter = 0; update(true); tCur = travelSamples; tStep = 0;
    }
    void set(const DistanceParams& np) { p = np; }
    void reset() { for (auto& d : travel) d.reset(); for (auto& d : ear) d.reset(); for (auto& d : refl) d.reset(); verb.reset(); }

    void process(float* const* ch, int nch, int n) {
        for (int o = 0; o < n; o += kChunk) {   // fixed-size chunks: the reverb's scratch buffers never grow on the audio thread
            float* c2[2] = { ch[0] + o, nch > 1 ? ch[1] + o : nullptr };
            processChunk(c2, nch, std::min(kChunk, n - o));
        }
    }

    // current smoothed distance in metres (for the display)
    double currentMetres() const { return distNow; }

private:
    static constexpr int kChunk = 1024;
    void processChunk(float* const* ch, int nch, int n) {
        nch = std::min(nch, 2);
        for (int i = 0; i < n; i++) {
            if ((counter++ & 15) == 0) { update(false); tStep = (travelSamples - tCur) / 16.0; }
            tCur += tStep;
            const double inL = ch[0][i], inR = nch > 1 ? ch[1][i] : inL;
            // the source: mid plus a little of its own width when close
            const double mid = 0.5 * (inL + inR), side = 0.5 * (inL - inR) * srcWidth;
            // travel time (doppler): the delay follows the distance smoothly, so changes bend the pitch
            const double tDelay = std::max(2.0, tCur);
            travel[0].push((float)mid); travel[1].push((float)side);
            const double m = travel[0].read(tDelay), s = travel[1].read(tDelay);
            // air and proximity on the direct sound
            double d[2];
            for (int c = 0; c < 2; c++) {
                double x = c == 0 ? m : s;
                x = airShelf[c].process(airLp[c].process(x));
                if (proxOn) x = prox[c].process(x);
                d[c] = x;
            }
            // place it: equal-power pan from the angle, the near ear first, the far ear delayed and shadowed
            double L = (d[0] + d[1]) * panL, R = (d[0] - d[1]) * panR;
            ear[0].push((float)L); ear[1].push((float)R);
            const double eL = itd > 0 ? ear[0].read(2.0 + itd) : ear[0].read(2.0), eR = itd < 0 ? ear[1].read(2.0 - itd) : ear[1].read(2.0);
            double dl = farLeft ? shadow[0].process(eL) : eL, dr = farLeft ? eR : shadow[1].process(eR);
            // floor bounce and room reflections (they don't fall away with distance like the direct sound does)
            refl[0].push((float)m); refl[1].push((float)(m * 0.97 + s * 0.2));
            double fl = 0, fr = 0;
            if (floorGain > 0) {
                const double f = floorLp[0].process(refl[0].read(std::max(2.0, floorDelay)));
                fl += f * floorGain * panL; fr += f * floorGain * panR;
            }
            for (int k = 0; k < kTaps; k++) {
                if (tapGain[k] <= 0) continue;
                const double v = refl[(size_t)(k & 1)].read(tapDelay[k]) * tapGain[k];
                if (tapPan[k] < 0) fl += v; else fr += v;
            }
            fl = reflLp[0].process(fl); fr = reflLp[1].process(fr);
            const double g = gainS.next(directGain);
            const double outL = dl * g + fl * reflGain, outR = dr * g + fr * reflGain;
            wetIn[0][(size_t)i] = (float)(m * reverbSend); wetIn[1][(size_t)i] = (float)(m * reverbSend);
            ch[0][i] = (float)(outL * outGain);
            if (nch > 1) ch[1][i] = (float)(outR * outGain);
            else ch[0][i] = (float)(0.5 * (outL + outR) * outGain);
        }
        // the space's tail: 100% wet reverb fed with the source, added at a level that doesn't depend on distance
        if (reverbSend > 0) {
            float* w[2] = { wetIn[0].data(), wetIn[1].data() };
            verb.process(w, 2, n);
            for (int i = 0; i < n; i++) {
                ch[0][i] += (float)(w[0][i] * outGain);
                if (nch > 1) ch[1][i] += (float)(w[1][i] * outGain);
            }
        }
    }

    static constexpr int kTaps = 6;
    void update(bool snap) {
        const auto& sp = space(p.space);
        const double target = metres(p.distance);
        if (snap) distS.reset(target);
        distNow = distS.next(target);
        const double dm = distNow;
        // level: inverse distance from 1 m, partly given back by Keep Level; clamp very close so it doesn't explode
        const double lossDb = 20 * std::log10(std::max(dm, 0.63));   // closer than ~0.6 m gets at most +4 dB
        directGain = dbToGain(-lossDb * (1 - 0.85 * p.keepLevel));
        // the room keeps its level, so further = more room relative to the direct sound
        const double roomComp = dbToGain(lossDb * 0.85 * p.keepLevel * 0.5);   // keep level lifts the room a little too
        reverbSend = sp.reverb * roomComp;
        reflGain = sp.reflections * 0.5 * roomComp * std::min(1.0, dm / 1.5 + 0.15);
        // air absorption: a gentle lowpass plus a high shelf that both close in with distance
        const double airAmt = clampv(p.air, 0.0, 1.5);
        const double cutoff = clampv(20000.0 * std::exp(-dm * airAmt / 70.0), 1800.0, 20000.0);
        const double shelfDb = -std::min(18.0, dm * 0.12 * airAmt);
        for (int c = 0; c < 2; c++) { airLp[c].set(Svf::lowpass, sr, cutoff, 0.6); airShelf[c].set(Svf::highShelf, sr, 5000, 0.7, shelfDb); }
        // proximity: up to +6 dB of low end under 1 m
        const double proxDb = 6.0 * clampv((1.0 - dm) / 0.85, 0.0, 1.0);
        proxOn = proxDb > 0.05;
        for (int c = 0; c < 2; c++) prox[c].set(Svf::lowShelf, sr, 180, 0.7, proxDb);
        // width: close sources keep their own stereo width, distant ones become points
        srcWidth = p.width * clampv(1.2 - std::log10(std::max(dm, 0.3) + 1) * 0.7, 0.1, 1.0);
        // angle: equal-power pan, interaural delay (max ~0.65 ms) and the head's shadow on the far ear
        const double a = angS.next(p.angleDeg) * kPi / 180.0;   // -90 left .. +90 right
        const double pan = clampv(std::sin(a) * 0.6, -1.0, 1.0);   // a real head: the far ear still hears the source, ~10 dB down
        panL = std::cos((pan + 1) * 0.25 * kPi) * 1.41421356; panR = std::sin((pan + 1) * 0.25 * kPi) * 1.41421356;   // unity in the centre
        // distant sources are less directional (the room arrives from everywhere)
        itd = std::sin(a) * 0.00065 * sr * clampv(1.4 - dm / 30.0, 0.2, 1.0);
        farLeft = itd > 0;
        const double shadowHz = 20000.0 * std::pow(0.15, std::abs(std::sin(a)) * clampv(1.4 - dm / 30.0, 0.2, 1.0));
        for (int c = 0; c < 2; c++) shadow[c].set(Svf::lowpass, sr, std::max(1500.0, shadowHz), 0.6);
        // travel time for doppler: the real 343 m/s delay, scaled down so even far placements stay close to the beat
        travelSamples = 2.0 + p.doppler * (dm / 343.0) * sr * 0.25;
        travelSamples = std::min(travelSamples, 0.088 * sr);
        // floor reflection: path difference with source and listener at the space's ear height
        const double h = sp.height;
        const double path = std::sqrt(dm * dm + 4 * h * h);
        floorDelay = 2.0 + (path - dm) / 343.0 * sr;
        floorDelay = std::min(floorDelay, 0.115 * sr);
        floorGain = p.space == field || p.space == street ? 0.7 * dm / path : 0.45 * dm / path;
        for (int c = 0; c < 2; c++) floorLp[c].set(Svf::lowpass, sr, 6000, 0.6);
        // early reflections from walls: delays shrink as the source moves toward the far wall
        static const double base[kTaps] = { 0.011, 0.017, 0.023, 0.031, 0.043, 0.057 };
        static const double gains[kTaps] = { 0.55, 0.5, 0.42, 0.36, 0.3, 0.24 };
        const double scale = sp.size / 50.0;
        for (int k = 0; k < kTaps; k++) {
            if (sp.reflections <= 0) { tapGain[k] = 0; continue; }
            const double shrink = 1.0 / (1.0 + dm / (sp.size * 0.6 + 1e-9));
            tapDelay[k] = std::min(0.115 * sr, 2.0 + base[k] * scale * sr * (0.35 + 0.65 * shrink));
            tapGain[k] = gains[k] * (p.space == street && k > 1 ? 0.4 : 1.0);
            tapPan[k] = (k % 2 == 0) ? -1 : 1;
        }
        for (int c = 0; c < 2; c++) reflLp[c].set(Svf::lowpass, sr, sp.damp, 0.6);
        // reverb settings for the space
        if (sp.reverb > 0) {
            VerbParams v; v.sizePct = sp.size; v.decayS = sp.decayS; v.preMs = std::min(60.0, 8 + sp.size * 0.3); v.dampHz = sp.damp; v.lowCutHz = 150; v.widthPct = 100; v.modPct = 20; v.mix = 1.0;
            v.character = p.space == hall || p.space == stadium ? VerbDsp::hall : p.space == club ? VerbDsp::plate : VerbDsp::room;
            verb.set(v);
        }
        outGain = dbToGain(p.outDb);
    }

    DistanceParams p; double sr = 48000;
    std::array<DelayLine, 2> travel, ear, refl;
    std::array<Svf, 2> airLp, airShelf, prox, shadow, floorLp, reflLp;
    VerbDsp verb; std::vector<float> wetIn[2];
    Smooth distS, angS, gainS;
    double distNow = 1, directGain = 1, reverbSend = 0, reflGain = 0, srcWidth = 1, panL = 0.7071, panR = 0.7071, itd = 0, travelSamples = 2;
    double floorDelay = 10, floorGain = 0, outGain = 1, tCur = 2, tStep = 0;
    double tapDelay[kTaps] = {}, tapGain[kTaps] = {}; int tapPan[kTaps] = {};
    bool proxOn = false, farLeft = false; unsigned counter = 0;
};
} // namespace lgn
