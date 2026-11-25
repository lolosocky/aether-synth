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

# ---------------------------------------------------------------- Duck  (rate 0=1/1 1=1/2 2=1/4 3=1/8 4=1/16; mode 0 Tempo, 1 Kick)
def dk(mode, amount, release, rate=2): return dict(mode=mode, amount=amount, release=release, rate=rate)
DUCK = [
    ("Tempo", "Classic 1/4 Pump",   dk(0, 75, 50)),
    ("Tempo", "Subtle Breathing",   dk(0, 35, 45)),
    ("Tempo", "Hard EDM Pump",      dk(0, 100, 60)),
    ("Tempo", "1/8 Groove",         dk(0, 60, 35, rate=3)),
    ("Tempo", "1/16 Stutter",       dk(0, 85, 20, rate=4)),
    ("Tempo", "Half-Time Swell",    dk(0, 70, 70, rate=1)),
    ("Tempo", "Bar Swell",          dk(0, 85, 85, rate=0)),
    ("Kick", "808 Under Kick",      dk(1, 85, 15)),
    ("Kick", "Bass Make Room",      dk(1, 70, 30)),
    ("Kick", "Pad Pump",            dk(1, 85, 60)),
    ("Kick", "Vocal Nudge",         dk(1, 25, 25)),
    ("Kick", "Mix Bus Bounce",      dk(1, 30, 35)),
]

# ---------------------------------------------------------------- Fret (guitar)
# choices: mode 0 Guitar 1 Free 2 Legato | strumdir 0 Down 1 Up 2 Alt | pick 0 Pick 1 Finger 2 Thumb | course 0 Single 1 Double 2 12-String
#          body 0 None 1 Dreadnought 2 Parlor 3 Classical 4 Jumbo 5 Archtop | pickup 0 Off 1 Single 2 Humbucker 3 Piezo
#          amp 0 Off 1 Clean 2 Crunch 3 Lead 4 Fuzz | cab 0 Off 1 1x12 2 2x12 3 4x12 | ddiv 0 1/16 1 1/8T 2 1/8 3 1/8D 4 1/4T 5 1/4 6 1/4D 7 1/2
#          tuning 0 Standard 1 Drop D 2 Half Down 3 Open D 4 DADGAD 5 Open G
ACOUSTIC = dict(body=1, pickup=0, amp=0, cab=0, vmix=15)
NYLON = dict(body=3, pick=1, bright=42, stiff=5, noise=12, hard=45, pickup=0, amp=0, cab=0)
ELECTRIC = dict(body=0, pickup=1, pupos=50, amp=1, drive=6, cab=1, vmix=15, noise=20)
def fr(base, **kw): d = dict(base); d.update(kw); return d
FRET = [
    ("Acoustic", "Steel Strummer",        fr(ACOUSTIC, strum=35, release=0.6, bright=60, decay=5)),
    ("Acoustic", "Campfire Dreadnought",  fr(ACOUSTIC, bodyamt=85, bright=55, strum=45, humanize=45, release=0.8, vmix=10)),
    ("Acoustic", "Bright Jumbo",          fr(ACOUSTIC, body=4, bright=72, decay=6, noise=40, release=0.7)),
    ("Acoustic", "Parlor Fingerpicking",  fr(ACOUSTIC, body=2, pick=1, hard=45, pickpos=22, strum=0, release=1.2, bright=50, vmix=18)),
    ("Acoustic", "Pop Acoustic",          fr(ACOUSTIC, bright=68, hard=70, decay=4, release=0.5, chorus=10, vmix=16, width=75)),
    ("Acoustic", "Muted Acoustic Rhythm", fr(ACOUSTIC, mute=45, release=0.15, strum=25, bright=55, vmix=10)),
    ("Acoustic", "Soft Fingerstyle",      fr(ACOUSTIC, pick=1, hard=35, bright=45, decay=6, release=1.5, vmix=22, strum=0)),
    ("Acoustic", "DADGAD Drone",          fr(ACOUSTIC, tuning=4, release=2, decay=8, vmix=25, vsize=70)),
    ("Acoustic", "Open G Slide",          fr(ACOUSTIC, tuning=5, mode=2, glide=120, vib=12, decay=6, release=1, bright=58)),
    ("Acoustic", "Bedroom Demo",          fr(ACOUSTIC, body=2, bright=50, hard=50, release=0.5, vmix=8, age=15)),
    ("Nylon", "Classical Nylon",          fr(NYLON, decay=4, release=1, vmix=20)),
    ("Nylon", "Bossa Thumb",              fr(NYLON, pick=2, bright=38, hard=40, release=0.4, strum=25, vmix=14)),
    ("Nylon", "Flamenco Bright",          fr(NYLON, bright=62, hard=80, strum=18, humanize=50, noise=25, release=0.5, vmix=16)),
    ("Nylon", "Lo-Fi Nylon Loop",         fr(NYLON, bright=35, age=55, vmix=18, release=0.8)),
    ("Nylon", "Spanish Lead",             fr(NYLON, mode=2, bright=50, vib=8, vibrate=5, glide=40, release=0.8, vmix=20)),
    ("Nylon", "Nylon Wash",               fr(NYLON, bright=40, decay=10, release=3, vmix=45, vsize=85, chorus=20, dmix=18, ddiv=3)),
    ("Clean Electric", "Glassy Single Coil", fr(ELECTRIC, bright=62, treble=2)),
    ("Clean Electric", "Neo-Soul Chorus",    fr(ELECTRIC, pupos=30, drive=5, chorus=45, chrate=0.6, vmix=22, pick=1, hard=50, bright=55, strum=20)),
    ("Clean Electric", "Funk Scratch",       fr(ELECTRIC, pupos=85, mute=25, release=0.06, bright=75, hard=80, drive=8, treble=3, mid=2, strum=8, vmix=8)),
    ("Clean Electric", "Clean Twin Reverb",  fr(ELECTRIC, pupos=60, drive=4, cab=2, vmix=30, vsize=55, treble=2, bright=60)),
    ("Clean Electric", "Bridge Twang",       fr(ELECTRIC, pupos=100, drive=10, bright=72, treble=3, mid=2, vmix=12, dmix=12, ddiv=2, dfb=10)),
    ("Clean Electric", "Jazz Box",           fr(ELECTRIC, body=5, bodyamt=35, pickup=2, pupos=0, drive=4, bright=35, treble=-4, bass=2, pick=2, vmix=14)),
    ("Clean Electric", "Clean Arpeggio Delay", fr(ELECTRIC, pupos=40, dmix=28, ddiv=3, dfb=38, vmix=25, chorus=15)),
    ("Clean Electric", "Warm Neck Pickup",   fr(ELECTRIC, pickup=2, pupos=0, bright=48, bass=2, treble=-2, vmix=18)),
    ("Crunch", "Blues Crunch",            fr(ELECTRIC, pickup=2, pupos=70, amp=2, drive=18, cab=2, mid=2, vmix=12)),
    ("Crunch", "Classic Rock Rhythm",     fr(ELECTRIC, pickup=2, pupos=100, amp=2, drive=24, cab=3, course=1, detune=6, width=90, mid=1, vmix=10)),
    ("Crunch", "Indie Jangle Crunch",     fr(ELECTRIC, pupos=80, amp=2, drive=12, chorus=15, vmix=18, bright=66)),
    ("Crunch", "Palm Mute Chug",          fr(ELECTRIC, pickup=2, pupos=100, amp=2, drive=26, cab=3, mute=55, release=0.05, strum=6, bass=3, vmix=6)),
    ("Crunch", "Punk Power",              fr(ELECTRIC, pickup=2, pupos=100, amp=2, drive=28, cab=3, course=1, width=100, strum=10, vmix=8)),
    ("Crunch", "Edge of Breakup",         fr(ELECTRIC, pupos=60, amp=2, drive=10, vmix=16)),
    ("Lead", "Singing Lead",              fr(ELECTRIC, mode=2, glide=50, vib=10, vibrate=5.5, pickup=2, pupos=0, amp=3, drive=30, cab=3, dmix=22, ddiv=3, dfb=35, vmix=18, decay=10, release=0.5)),
    ("Lead", "Shred Lead",                fr(ELECTRIC, mode=2, glide=25, pickup=2, pupos=100, amp=3, drive=36, cab=3, dmix=12, vmix=12, mid=3)),
    ("Lead", "Blues Lead",                fr(ELECTRIC, mode=2, glide=70, vib=14, vibrate=5, pickup=2, pupos=40, amp=2, drive=22, cab=2, vmix=16)),
    ("Lead", "Slide Lead",                fr(ELECTRIC, mode=2, glide=160, vib=8, pupos=80, amp=2, drive=18, vmix=22, bright=58)),
    ("Lead", "Fuzz Lead",                 fr(ELECTRIC, mode=2, glide=40, pickup=2, pupos=0, amp=4, drive=30, cab=2, dmix=18, ddiv=3)),
    ("Lead", "80s Chorus Lead",           fr(ELECTRIC, mode=2, pickup=2, pupos=100, amp=3, drive=28, cab=3, chorus=40, dmix=25, ddiv=3, dfb=40, vmix=30)),
    ("Lead", "Drop D Metal Rhythm",       fr(ELECTRIC, tuning=1, pickup=2, pupos=100, amp=3, drive=34, cab=3, mute=35, release=0.05, bass=3, mid=-2, treble=2, course=1, width=100, strum=4, vmix=6)),
    ("Lo-Fi & Soul", "Dusty Soul Sample",  fr(ELECTRIC, pupos=20, drive=8, age=60, vmix=20, chorus=10, bright=45, release=0.4)),
    ("Lo-Fi & Soul", "Rainy Window Nylon", fr(NYLON, age=45, vmix=25, bright=38, release=1, strum=50)),
    ("Lo-Fi & Soul", "70s Soul Rhythm",    fr(ELECTRIC, pupos=90, drive=10, mute=15, release=0.1, treble=3, age=25, vmix=12)),
    ("Lo-Fi & Soul", "Vinyl Jazz Chords",  fr(ELECTRIC, body=5, bodyamt=40, pickup=2, pupos=0, pick=2, age=55, vmix=18, bright=35)),
    ("Lo-Fi & Soul", "Tape-Warped Acoustic", fr(ACOUSTIC, age=75, vmix=18, chorus=15)),
    ("Lo-Fi & Soul", "Bedroom Lo-Fi Clean", fr(ELECTRIC, pupos=30, age=40, chorus=25, vmix=25)),
    ("Lo-Fi & Soul", "Boom Bap Loop",      fr(ELECTRIC, pickup=2, pupos=0, drive=12, age=50, mute=10, release=0.3, vmix=10, bright=40)),
    ("Ambient", "Shimmer Swell",          fr(ELECTRIC, pupos=40, decay=14, release=4, vmix=55, vsize=95, dmix=30, ddiv=6, dfb=55, chorus=25, hard=30, pick=1, strum=80)),
    ("Ambient", "Endless Pad",            fr(ACOUSTIC, pick=1, hard=25, decay=18, release=5, vmix=65, vsize=100, dmix=35, ddiv=7, dfb=60, chorus=30)),
    ("Ambient", "Dreamy Arps",            fr(ELECTRIC, dmix=35, ddiv=3, dfb=50, vmix=40, vsize=80, chorus=35)),
    ("Ambient", "Post-Rock Wall",         fr(ELECTRIC, pickup=2, amp=2, drive=20, cab=3, dmix=30, ddiv=3, dfb=55, vmix=55, vsize=95, course=1, width=100)),
    ("Ambient", "Glass Harmonics",        fr(ELECTRIC, pickup=3, amp=0, cab=0, bright=85, stiff=40, pickpos=6, hard=30, pick=1, decay=12, release=3, vmix=50, chorus=20)),
    ("Ambient", "Night Drive",            fr(ELECTRIC, pupos=30, cab=2, chorus=40, dmix=25, ddiv=3, vmix=40, age=15)),
    ("Trap & Rage", "Trap Guitar Loop",   fr(ELECTRIC, pupos=40, chorus=20, vmix=30, vsize=60, age=20, dmix=15, ddiv=3, bright=55, strum=25)),
    ("Trap & Rage", "Sad Trap Acoustic",  fr(ACOUSTIC, pick=1, hard=40, vmix=30, age=25, bright=50, release=0.8, strum=40, chorus=10)),
    ("Trap & Rage", "Melodic Drill Guitar", fr(ELECTRIC, dmix=22, ddiv=1, dfb=40, vmix=28, age=10, pick=1, hard=55)),
    ("Trap & Rage", "Rage Fuzz Guitar",   fr(ELECTRIC, amp=4, drive=32, cab=3, pickup=2, pupos=100, course=1, width=100, vmix=20, mid=-2)),
    ("Trap & Rage", "Rage Stab",          fr(ELECTRIC, pickup=2, pupos=100, amp=3, drive=30, cab=3, mute=30, release=0.1, dmix=20, ddiv=2, vmix=15)),
    ("Trap & Rage", "Astro Wash",         fr(ELECTRIC, chorus=50, chrate=0.4, dmix=30, ddiv=3, dfb=45, vmix=50, vsize=85, age=20, width=95)),
    ("Trap & Rage", "Pluggnb Clean",      fr(ELECTRIC, pupos=30, chorus=30, vmix=35, bright=60, release=0.6, dmix=12)),
    ("Trap & Rage", "Dark Trap Nylon",    fr(NYLON, bright=35, age=30, vmix=35, vsize=70, dmix=15, ddiv=3, release=1)),
    ("12-String & Doubles", "Jangle 12-String",   fr(ACOUSTIC, course=2, detune=5, bright=66, vmix=18, strum=40)),
    ("12-String & Doubles", "Electric 12-String", fr(ELECTRIC, course=2, pupos=70, chorus=10, vmix=18, bright=64)),
    ("12-String & Doubles", "Doubled Wall Acoustic", fr(ACOUSTIC, course=1, detune=9, width=100, body=4, vmix=16)),
    ("12-String & Doubles", "Folk 12-String",     fr(ACOUSTIC, course=2, detune=4, body=4, bright=60, release=1.0, strum=55, humanize=50)),
    ("12-String & Doubles", "Chorus Doubles Clean", fr(ELECTRIC, course=1, chorus=30, width=100, vmix=22)),
]

# ---------------------------------------------------------------- Distance  (space 0 Studio 1 Room 2 Club 3 Hall 4 Street 5 Stadium 6 Field)
import math
def ds_(m, space, angle=0, keep=50, air=100, doppler=50, width=100):
    return dict(distance=round(math.log(m / 0.15) / math.log(1000), 4), space=space, angle=angle, keep=keep, air=air, doppler=doppler, width=width)
DISTANCE = [
    ("Close", "In Your Ear",          ds_(0.2, 0)),
    ("Close", "Whisper Right",        ds_(0.3, 0, angle=40)),
    ("Close", "Vocal Booth",          ds_(0.6, 0)),
    ("Close", "Ad-Lib Left",          ds_(2, 1, angle=-60)),
    ("Close", "Ad-Lib Right",         ds_(2, 1, angle=60)),
    ("Room", "Across the Room",       ds_(4, 1)),
    ("Room", "Corner of the Room",    ds_(6, 1, angle=-35)),
    ("Room", "Behind the Band",       ds_(8, 2, keep=60)),
    ("Venue", "Back of the Club",     ds_(18, 2, keep=65)),
    ("Venue", "Hall Seat",            ds_(12, 3, keep=60)),
    ("Venue", "Far Choir",            ds_(25, 3, keep=70, width=100)),
    ("Venue", "Stadium PA",           ds_(80, 5, keep=75)),
    ("Outside", "Down the Street",    ds_(40, 4, angle=30, keep=60)),
    ("Outside", "Next Block Over",    ds_(90, 4, angle=-20, keep=70)),
    ("Outside", "Across the Field",   ds_(120, 6, keep=75)),
    ("Outside", "Passing Car (automate distance)", ds_(15, 6, angle=50, doppler=100, keep=60)),
]

# ---------------------------------------------------------------- Era  (era: 0 Shellac 78, 1 AM Radio, 2 Soul Vinyl, 3 Cassette, 4 SP-1200, 5 MPC60, 6 S950,
#                                         7 VHS, 8 Boombox, 9 MP3 128k, 10 Phone Speaker, 11 Answering Machine)
def er(era, age=75, grit=100, wobble=100, noise=100, tone=0, width=100, mix=100, out=0):
    return dict(era=era, age=age, grit=grit, wobble=wobble, noise=noise, tone=tone, width=width, mix=mix, out=out)
ERA = [
    ("Vinyl & Shellac", "Dusty Soul Record",   er(2, 85)),
    ("Vinyl & Shellac", "Clean Vinyl Warmth",  er(2, 45, noise=25)),
    ("Vinyl & Shellac", "Shellac 78",          er(0, 100)),
    ("Samplers", "SP-1200 Drums",              er(4, 100)),
    ("Samplers", "SP-1200 Pitched Down",       er(4, 100, grit=135, tone=-0.2)),
    ("Samplers", "MPC60 Warmth",               er(5, 90)),
    ("Samplers", "S950 Filtered",              er(6, 100)),
    ("Samplers", "Parallel SP Grit",           er(4, 100, grit=150, mix=45)),
    ("Tape", "Cassette Demo",                  er(3, 80)),
    ("Tape", "Worn Tape",                      er(3, 100, wobble=160, noise=130)),
    ("Tape", "VHS Memory",                     er(7, 90)),
    ("Tape", "Boombox Bump",                   er(8, 85)),
    ("Devices & Broadcast", "AM Radio Intro",  er(1, 100)),
    ("Devices & Broadcast", "MP3 2003",        er(9, 80)),
    ("Devices & Broadcast", "Phone Speaker",   er(10, 100)),
    ("Devices & Broadcast", "Voicemail",       er(11, 100)),
    ("Mix Bus", "Lo-Fi Bus Glue",              er(2, 35, noise=20, wobble=50)),
]

# ---------------------------------------------------------------- Swing  (grid 0 = 1/8, 1 = 1/16; steps s1..s16 in % of a step, + late / - early)
def sw(swing, grid=1, humanize=0, tail=0, **steps):
    d = dict(grid=grid, swing=swing, humanize=humanize, tail=tail)
    for k, v in steps.items(): d[k] = v
    return d
SWING = [
    ("Classic", "Straight",            sw(50)),
    ("Classic", "MPC 54%",             sw(54)),
    ("Classic", "MPC 58%",             sw(58)),
    ("Classic", "MPC 62%",             sw(62)),
    ("Classic", "Triplet 16ths",       sw(66.7)),
    ("Classic", "1/8 Shuffle",         sw(66.7, grid=0)),
    ("Classic", "Hard Swing 75%",      sw(75)),
    ("Hip-Hop", "Dilla Drunk",         sw(57, humanize=5, s2=4, s3=-4, s5=10, s6=6, s10=2, s11=-4, s13=10, s14=8, s16=-3)),
    ("Hip-Hop", "Lazy Snare",          sw(54, s5=14, s13=14)),
    ("Hip-Hop", "Neo-Soul Pocket",     sw(60, humanize=4, s5=8, s13=8)),
    ("Hip-Hop", "90s Boom Bap",        sw(60, s5=5, s13=5)),
    ("Hip-Hop", "Trap Bounce",         sw(54, s14=-5, s16=-5)),
    ("Hip-Hop", "Rushed Hats",         sw(50, s2=-8, s4=-8, s6=-8, s8=-8, s10=-8, s12=-8, s14=-8, s16=-8)),
    ("Hip-Hop", "Drunk Hats",          sw(55, humanize=12)),
    ("Hip-Hop", "Half-Time Shuffle",   sw(62, grid=0)),
]

# ---------------------------------------------------------------- Mood  (flip: 0 Maj->Min, 1 Maj->Harm Min, 2 Maj->Dorian, 3 Maj 3rd only, 4 Min->Maj, 5 Min->Dorian, 6 Min 3rd only)
def md(flip, mood=100, glide=30, floor=40, mix=100): return dict(flip=flip, mood=mood, glide=glide, floor=floor, mix=mix)
MOOD = [
    ("Major to Minor", "Sad Flip",           md(0)),
    ("Major to Minor", "Dark Harmonic",      md(1)),
    ("Major to Minor", "Dorian Soul",        md(2)),
    ("Major to Minor", "Just the Third",     md(3)),
    ("Major to Minor", "Blue Third",         md(3, mood=50)),
    ("Minor to Major", "Happy Flip",         md(4)),
    ("Minor to Major", "Minor to Dorian",    md(5)),
    ("Minor to Major", "Brighten the Third", md(6)),
    ("Motion", "Slow Morph (automate Mood)", md(0, glide=400)),
    ("Motion", "Bass Safe",                  md(0, floor=150)),
]

# Preset volumes, measured: each preset was rendered playing a strummed progression (or a melody, for the legato
# presets; host_test --demos) and its volume set so every preset has the same average loudness, peaks <= -1 dBFS.
FRET_VOLUME = {'Steel Strummer': 2.0, 'Campfire Dreadnought': 3.0, 'Bright Jumbo': 1.5, 'Parlor Fingerpicking': -2.5, 'Pop Acoustic': 3.0, 'Muted Acoustic Rhythm': 4.0, 'Soft Fingerstyle': -3.5, 'DADGAD Drone': 0.5, 'Open G Slide': 6.5, 'Bedroom Demo': 1.5, 'Classical Nylon': -2.0, 'Bossa Thumb': -4.5, 'Flamenco Bright': -1.0, 'Lo-Fi Nylon Loop': -3.0, 'Spanish Lead': 3.0, 'Nylon Wash': -3.0, 'Glassy Single Coil': -1.5, 'Neo-Soul Chorus': -4.0, 'Funk Scratch': -3.0, 'Clean Twin Reverb': 0.0, 'Bridge Twang': -0.5, 'Jazz Box': -10.0, 'Clean Arpeggio Delay': 0.0, 'Warm Neck Pickup': -5.0, 'Blues Crunch': -2.0, 'Classic Rock Rhythm': -4.5, 'Indie Jangle Crunch': 3.0, 'Palm Mute Chug': -2.0, 'Punk Power': -5.5, 'Edge of Breakup': 1.5, 'Singing Lead': -5.0, 'Shred Lead': -5.5, 'Blues Lead': 0.0, 'Slide Lead': 6.0, 'Fuzz Lead': -1.5, '80s Chorus Lead': 1.0, 'Drop D Metal Rhythm': -4.5, 'Dusty Soul Sample': -3.0, 'Rainy Window Nylon': -2.5, '70s Soul Rhythm': 1.0, 'Vinyl Jazz Chords': -10.0, 'Tape-Warped Acoustic': 1.5, 'Bedroom Lo-Fi Clean': -1.5, 'Boom Bap Loop': -5.5, 'Shimmer Swell': -5.0, 'Endless Pad': -4.0, 'Dreamy Arps': 1.0, 'Post-Rock Wall': -2.5, 'Glass Harmonics': -5.5, 'Night Drive': 0.5, 'Trap Guitar Loop': -1.5, 'Sad Trap Acoustic': -2.0, 'Melodic Drill Guitar': -4.0, 'Rage Fuzz Guitar': -2.5, 'Rage Stab': -2.5, 'Astro Wash': 1.5, 'Pluggnb Clean': -0.5, 'Dark Trap Nylon': -2.0, 'Jangle 12-String': -0.5, 'Electric 12-String': -2.0, 'Doubled Wall Acoustic': 0.0, 'Folk 12-String': 0.5, 'Chorus Doubles Clean': -3.5}
for _c, _n, _v in FRET: _v.setdefault("volume", FRET_VOLUME.get(_n, 0.0))

PLUGINS = {
    "eq": (EQ, ["spectrum"]),
    "comp": (COMP, []),
    "deess": (DEESS, ["listen"]),
    "saturate": (SAT, []),
    "verb": (VERB, ["freeze"]),
    "delay": (DELAY, []),
    "limit": (LIMIT, []),
    "gravity": (GRAVITY, ["key", "scale"]),
    "duck": (DUCK, []),
    "fret": (FRET, []),
    "distance": (DISTANCE, []),
    "era": (ERA, []),
    "swing": (SWING, []),
    "mood": (MOOD, ["key"]),
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
