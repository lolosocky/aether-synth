#!/usr/bin/env python3
"""Factory presets for the LGN suite.

Each plugin gets <plugin>/Presets.inc, a C++ function returning its preset list.
Values are in real units (Hz, dB, ms, %, choice index). Any parameter a preset leaves out
goes to its default when the preset loads, so each preset is complete on its own.

Run from suite/:  python3 tools/presets.py
"""
import os

# ---------------------------------------------------------------- EQ
# band shorthand: lc/hc = (freq, slope 0=12 1=24 2=48 dB/oct); others = (freq, gain, q)
def eq(lc=None, hc=None, ls=None, b1=None, b2=None, b3=None, hs=None, out=0):
    v = {}
    if lc: v.update(lc_on=1, lc_freq=lc[0], lc_slope=lc[1])
    if hc: v.update(hc_on=1, hc_freq=hc[0], hc_slope=hc[1])
    for name, band in (("ls", ls), ("b1", b1), ("b2", b2), ("b3", b3), ("hs", hs)):
        if band: v.update({f"{name}_freq": band[0], f"{name}_gain": band[1], f"{name}_q": band[2]})
    if out: v["out"] = out
    return v

EQ = [
    ("Vocals", "Lead Vocal Clarity",  eq(lc=(90, 1), b1=(250, -2.5, 1.2), b2=(3000, 2, 1.0), hs=(10000, 3, 0.707))),
    ("Vocals", "Rap Vocal Upfront",   eq(lc=(110, 1), b1=(300, -3, 1.4), b2=(4500, 2.5, 1.2), b3=(7500, -1.5, 4), hs=(12000, 2.5, 0.707))),
    ("Vocals", "Vocal De-Box",        eq(lc=(100, 1), b1=(420, -3.5, 1.8), b2=(1000, -1.5, 2), hs=(12000, 2, 0.707))),
    ("Vocals", "Backing Vocals",      eq(lc=(200, 1), b1=(350, -3, 1.0), b2=(2500, -1.5, 0.8), hs=(10000, 2, 0.707))),
    ("Vocals", "Air Lift",            eq(hs=(14000, 3.5, 0.6))),
    ("Drums", "Kick Punch",           eq(lc=(30, 2), b1=(62, 3, 1.4), b2=(350, -4, 1.6), b3=(3500, 3, 1.2))),
    ("Drums", "Snare Crack",          eq(lc=(80, 1), b1=(200, 2, 1.4), b2=(900, -2.5, 2), b3=(5000, 3, 1.0), hs=(10000, 1.5, 0.707))),
    ("Drums", "Hi-Hat Smooth",        eq(lc=(300, 2), b3=(6500, -2, 3), hs=(12000, 1.5, 0.707))),
    ("Drums", "808 Clean Low End",    eq(lc=(28, 2), b1=(55, 2, 1.0), b2=(250, -3, 1.2), b3=(900, 2, 1.2), hc=(9000, 0))),
    ("Drums", "Drum Bus",             eq(lc=(25, 1), ls=(80, 1.5, 0.707), b1=(400, -1.5, 1.0), hs=(10000, 2, 0.707))),
    ("Instruments", "Bass Definition", eq(lc=(35, 2), ls=(80, 1.5, 0.707), b1=(250, -2.5, 1.2), b2=(800, 2, 1.2), hc=(5000, 0))),
    ("Instruments", "Acoustic Guitar", eq(lc=(90, 1), b1=(200, -2.5, 1.2), b2=(2500, 1.5, 1.0), hs=(10000, 2, 0.707))),
    ("Instruments", "Piano Brighten",  eq(lc=(50, 1), b1=(300, -2, 1.0), hs=(8000, 2.5, 0.707))),
    ("Instruments", "Synth Lead Bite", eq(lc=(150, 1), b1=(500, -2, 1.2), b2=(2800, 2.5, 1.0), hc=(16000, 1))),
    ("Instruments", "Pad Make Room",   eq(lc=(200, 1), b1=(400, -2, 0.9), b2=(3000, -2, 0.8), hc=(12000, 0))),
    ("Mix & Master", "Master Polish",  eq(lc=(20, 2), ls=(60, 0.8, 0.7), b1=(300, -0.8, 0.8), hs=(12000, 1.2, 0.7))),
    ("Mix & Master", "Mud Cleanup",    eq(lc=(30, 1), b1=(280, -2, 0.9))),
    ("Mix & Master", "Subsonic Filter", eq(lc=(30, 2))),
    ("Creative", "Telephone",          eq(lc=(400, 2), hc=(3400, 2), b2=(1500, 4, 1.0))),
    ("Creative", "Lo-Fi Radio",        eq(lc=(250, 1), hc=(5000, 1), b1=(1000, 3, 0.8))),
]

# ---------------------------------------------------------------- Compressor  (style 0 Clean, 1 Glue, 2 Punch)
# Makeup is set by measurement (tests/host_test prints each preset's loudness change on a mix-level
# test signal peaking near -6 dBFS) so every preset lands within about 1 dB of unity: switching presets
# compares the sound, not the volume.
def comp(th, ra, at, rel, knee=6, makeup=0, mix=100, hpf=20, style=0):
    return dict(thresh=th, ratio=ra, attack=at, release=rel, knee=knee, makeup=makeup, mix=mix, schpf=hpf, style=style)

COMP = [
    ("Vocals", "Vocal Leveler",        comp(-24, 3, 5, 60, knee=6, makeup=3.5)),
    ("Vocals", "Rap Vocal Upfront",    comp(-28, 6, 2, 50, knee=3, style=2, makeup=10)),
    ("Vocals", "Vocal Gentle Glue",    comp(-20, 2, 15, 120, knee=10, style=1, makeup=1.5)),
    ("Vocals", "Vocal Parallel Body",  comp(-36, 8, 3, 80, knee=4, mix=35, style=2, makeup=15.5)),
    ("Drums", "Kick Punch",            comp(-18, 4, 25, 80, knee=3, style=2, makeup=2)),
    ("Drums", "Snare Snap",            comp(-20, 5, 15, 70, knee=3, style=2, makeup=3.5)),
    ("Drums", "Drum Bus Glue",         comp(-16, 2, 30, 150, knee=6, hpf=60, style=1, makeup=0.5)),
    ("Drums", "Drum Smash (Parallel)", comp(-40, 20, 1, 60, knee=0, mix=40, style=2, makeup=19.5)),
    ("Bass", "Bass Control",           comp(-22, 4, 15, 100, knee=6, makeup=2.5)),
    ("Bass", "808 Sustain",            comp(-26, 4, 30, 200, knee=6, makeup=4.5)),
    ("Instruments", "Acoustic Gentle", comp(-20, 2.5, 15, 120, knee=8, makeup=1.5)),
    ("Instruments", "Synth Even Out",  comp(-22, 3, 10, 100, knee=8, style=1, makeup=3)),
    ("Mix Bus", "Mix Bus Glue",        comp(-12, 2, 30, 300, knee=6, hpf=100, style=1)),
    ("Mix Bus", "Master Transparent",  comp(-8, 1.5, 30, 200, knee=12, hpf=80, style=1)),
    ("Mix Bus", "NY Parallel",         comp(-30, 8, 3, 80, knee=2, mix=30, style=2, makeup=12.5)),
    ("Utility", "Peak Catcher",        comp(-6, 10, 0.3, 50, knee=2)),
]

# ---------------------------------------------------------------- De-esser  (mode 0 Split, 1 Wide)
def ds(f, th, rng, sm=60, mode=0): return dict(freq=f, thresh=th, range=rng, smooth=sm, mode=mode)
DEESS = [
    ("Vocals", "Male Vocal",          ds(5000, -30, 6, 50)),
    ("Vocals", "Female Vocal",        ds(7000, -30, 6, 50)),
    ("Vocals", "Rap Vocal",           ds(6000, -32, 8, 40)),
    ("Vocals", "Transparent",         ds(6500, -26, 4, 80)),
    ("Vocals", "Harsh Sibilance",     ds(7500, -36, 12, 40)),
    ("Vocals", "Backing Vocals",      ds(6500, -34, 10, 50, mode=1)),
    ("Instruments", "Hi-Hat Tamer",   ds(9000, -30, 6, 30, mode=1)),
    ("Instruments", "Overheads",      ds(8000, -28, 5, 50, mode=1)),
    ("Instruments", "Guitar Squeaks", ds(3000, -32, 6, 40)),
    ("Mix Bus", "Master De-Harsh",    ds(8000, -24, 3, 100, mode=1)),
]

# ---------------------------------------------------------------- Saturator  (mode 0 Tape, 1 Tube, 2 Transistor, 3 Fuzz)
def sat(mode, drive, tone=0, mix=100, out=0, auto=1):
    return dict(mode=mode, drive=drive, tone=tone, mix=mix, out=out, autogain=auto)
SAT = [
    ("Vocals", "Tube Vocal Warmth",    sat(1, 8, 0.1)),
    ("Vocals", "Vocal Edge",           sat(2, 10, 0.25, mix=50)),
    ("Drums", "Snare Bite",            sat(2, 10, 0.3)),
    ("Drums", "Drum Crush (Parallel)", sat(2, 22, 0.1, mix=35)),
    ("Drums", "Kick Thump",            sat(0, 9, -0.25)),
    ("Bass", "808 Distortion",         sat(2, 16, 0.15)),
    ("Bass", "808 Heavy Clip",         sat(3, 20, 0.0, mix=70)),
    ("Bass", "Tube Bass Grit",         sat(1, 14, -0.2)),
    ("Instruments", "Rage Lead Fuzz",  sat(3, 24, 0.3)),
    ("Instruments", "Keys Warmth",     sat(1, 6, -0.1)),
    ("Mix Bus", "Tape Warmth",         sat(0, 6, -0.1)),
    ("Mix Bus", "Tape Bus Glue",       sat(0, 4, 0.0)),
    ("Mix Bus", "Master Analog Colour", sat(0, 2.5, 0.0)),
    ("Creative", "Lo-Fi Crunch",       sat(3, 12, -0.5, mix=50)),
]

# ---------------------------------------------------------------- Reverb  (character 0 Room, 1 Plate, 2 Hall, 3 Space)
def rv(ch, size, decay, pre, damp, lowcut, mix, width=100, mod=30):
    return dict(character=ch, size=size, decay=decay, predelay=pre, damp=damp, lowcut=lowcut, mix=mix, width=width, mod=mod)
VERB = [
    ("Vocals", "Vocal Plate",         rv(1, 50, 1.8, 40, 8000, 200, 20, mod=20)),
    ("Vocals", "Rap Vocal Room",      rv(0, 30, 0.8, 15, 6500, 250, 12, mod=15)),
    ("Vocals", "Ballad Hall",         rv(2, 75, 3.2, 60, 6000, 180, 22)),
    ("Drums", "Drum Room",            rv(0, 35, 0.6, 5, 9000, 120, 15, mod=10)),
    ("Drums", "Snare Plate",          rv(1, 45, 1.4, 10, 9000, 250, 20, mod=15)),
    ("Drums", "Small Room",           rv(0, 20, 0.4, 0, 8000, 150, 15, mod=10)),
    ("Spaces", "Concert Hall",        rv(2, 85, 2.8, 25, 7000, 100, 25)),
    ("Spaces", "Ambient Space",       rv(3, 100, 10, 80, 5000, 300, 35, mod=60)),
    ("Spaces", "Endless Pad",         rv(3, 100, 18, 40, 4500, 250, 50, mod=70)),
    ("Sends (100% wet)", "Send Plate", rv(1, 55, 2.0, 30, 7500, 250, 100, mod=20)),
    ("Sends (100% wet)", "Send Hall",  rv(2, 70, 2.6, 40, 6500, 200, 100)),
    ("Sends (100% wet)", "Send Room",  rv(0, 30, 0.7, 10, 7000, 200, 100, mod=10)),
]

# ---------------------------------------------------------------- Delay  (division index into delayDivisions(); mode 0 Stereo, 1 Ping-Pong, 2 Mono)
DIV = {"1/32": 0, "1/16T": 1, "1/16": 2, "1/16D": 3, "1/8T": 4, "1/8": 5, "1/8D": 6, "1/4T": 7, "1/4": 8, "1/4D": 9, "1/2T": 10, "1/2": 11, "1/2D": 12, "1/1": 13}
def dl(div=None, ms=None, fb=35, mix=25, lc=150, hc=8000, warmth=25, wow=10, duck=0, spread=0, mode=1):
    v = dict(feedback=fb, mix=mix, lowcut=lc, highcut=hc, warmth=warmth, wow=wow, duck=duck, spread=spread, mode=mode)
    if div: v.update(sync=1, division=DIV[div])
    else: v.update(sync=0, time=ms)
    return v
DELAY = [
    ("Vocals", "Vocal Slapback",      dl(ms=110, fb=8, mix=15, lc=300, hc=5000, warmth=30, wow=5, mode=2)),
    ("Vocals", "Vocal 1/4 Throw",     dl("1/4", fb=30, mix=18, lc=250, hc=6000, duck=50)),
    ("Vocals", "Ad-Lib Echo",         dl("1/8", fb=45, mix=25, lc=400, hc=4500, duck=40)),
    ("Vocals", "Doubler",             dl(ms=18, fb=0, mix=30, lc=150, hc=12000, warmth=10, wow=20, spread=12, mode=0)),
    ("Rhythmic", "1/8 Dotted Ping-Pong", dl("1/8D", fb=35, mix=20, duck=30)),
    ("Rhythmic", "1/16 Synth Rhythm", dl("1/16", fb=40, mix=22, hc=8000)),
    ("Rhythmic", "Triplet Bounce",    dl("1/8T", fb=35, mix=20, hc=7000)),
    ("Character", "Tape Echo",        dl(ms=350, fb=45, mix=25, lc=150, hc=4000, warmth=60, wow=35, mode=2)),
    ("Character", "Dub Delay",        dl("1/4D", fb=70, mix=30, lc=300, hc=3000, warmth=50, wow=20)),
    ("Character", "Ambient Wash",     dl("1/2", fb=65, mix=35, hc=5000, warmth=30, wow=25, duck=20)),
    ("Character", "Guitar Slapback",  dl(ms=120, fb=10, mix=22, warmth=40, wow=5, mode=2)),
    ("Sends (100% wet)", "Send 1/4",  dl("1/4", fb=35, mix=100, lc=250, hc=6000)),
    ("Sends (100% wet)", "Send 1/8 Dotted", dl("1/8D", fb=35, mix=100, lc=250, hc=6000)),
]

# ---------------------------------------------------------------- Limiter
def lm(inp, ceil, rel, tp=1, auto=1): return dict(input=inp, ceiling=ceil, release=rel, truepeak=tp, autorel=auto)
LIMIT = [
    ("Mastering", "Streaming (-1 dBTP)",  lm(4, -1.0, 80)),
    ("Mastering", "Loud Master",          lm(8, -0.3, 50)),
    ("Mastering", "Hip-Hop Loud",         lm(10, -0.8, 40)),
    ("Mastering", "Transparent Safety",   lm(0, -0.3, 100)),
    ("Mastering", "Club Master",          lm(7, -0.5, 60)),
    ("Mixing", "Mix Bus Peak Catch",      lm(2, -1.0, 60)),
    ("Mixing", "Drum Bus Smash",          lm(12, -1.0, 20, tp=0, auto=0)),
    ("Mixing", "808 Limit",               lm(6, -1.0, 30, tp=0)),
    ("Mixing", "Vocal Peak Control",      lm(3, -3.0, 50)),
]

# ---------------------------------------------------------------- Gravity  (key and scale are left alone: they belong to the song)
def gv(pull, drift, mass, floor, mix=100): return dict(pull=pull, drift=drift, mass=mass, floor=floor, mix=mix)
GRAVITY = [
    ("Subtle", "Gentle Tune",       gv(35, 150, 10, 150)),
    ("Subtle", "Gentle Blend",      gv(100, 80, 20, 150, mix=30)),
    ("Subtle", "Harmonic Glow",     gv(80, 80, 50, 150, mix=40)),
    ("Lock", "Lock to Key",         gv(100, 20, 30, 120)),
    ("Lock", "808 Tuner",           gv(90, 40, 20, 40)),
    ("Lock", "Drums Into Key",      gv(70, 30, 40, 200)),
    ("Creative", "Vocal Hum",       gv(60, 200, 60, 250, mix=60)),
    ("Creative", "Slow Morph",      gv(100, 1000, 30, 150)),
    ("Creative", "Noise to Tone",   gv(100, 10, 100, 80)),
]

PLUGINS = {
    "eq": (EQ, ["spectrum"]),
    "comp": (COMP, []),
    "deess": (DEESS, ["listen"]),
    "saturate": (SAT, []),
    "verb": (VERB, ["freeze"]),
    "delay": (DELAY, []),
    "limit": (LIMIT, []),
    "gravity": (GRAVITY, ["key", "scale"]),
}

def num(x):
    s = repr(float(x))
    return s + "f"

here = os.path.dirname(os.path.abspath(__file__))
for folder, (presets, keep) in PLUGINS.items():
    lines = ["// Generated by tools/presets.py. Edit the presets there and re-run it.",
             "inline std::vector<lgn::Processor::Preset> factoryPresets() {", "    return {"]
    for cat, name, vals in presets:
        body = ", ".join(f'{{ "{k}", {num(v)} }}' for k, v in vals.items())
        lines.append(f'        {{ "{cat}", "{name}", {{ {body} }} }},')
    lines += ["    };", "}",
              "inline juce::StringArray presetKeep() { return { " + ", ".join(f'"{k}"' for k in keep) + " }; }", ""]
    with open(os.path.join(here, "..", folder, "Presets.inc"), "w") as f: f.write("\n".join(lines))
    print(f"{folder}: {len(presets)} presets")
