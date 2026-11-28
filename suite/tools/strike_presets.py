"""LGN Strike: the per-pad sound library and the factory kits.

A sound is a model plus values for the pad controls (tune st, decay ms, pitch st, pdecay ms, tone %, snap %, ndecay ms,
color %, drive %, click %, level dB, pan %, vel %, send %). A kit is eight sounds (pads 1-8) plus the machine settings.
Models: 0 Kick 808, 1 Kick 909, 2 Kick Modern, 3 Kick Acoustic, 4 808 Boom, 5 Snare 808, 6 Snare 909, 7 Snare Modern,
8 Snare Acoustic, 9 Clap, 10 Snap, 11 Rim, 12 Clave, 13 Hat Closed, 14 Hat Open, 15 Cymbal, 16 Cowbell, 17 Tom Analog,
18 Tom Simmons, 19 Conga, 20 Shaker, 21 Tambourine, 22 FM Perc, 23 Zap, 24 Noise FX.
Characters: 0 Clean 1 Analog 2 MPC60 3 SP-1200 4 LinnDrum 5 Cassette 6 Vinyl 7 Modern.
"""
import os

SOUNDS = []   # (category, name, dict)
def S(cat, name, model, **kw):
    d = dict(model=model)
    if model <= 4: d.update(snap=0, ndecay=15)   # kicks: no noise layer unless asked for
    d.update(kw); SOUNDS.append((cat, name, d))

# ---------------------------------------------------------------- kicks
S("Kick", "808 Kick Classic", 0, decay=650, pitch=3, pdecay=20, click=20, color=0, tone=45)
S("Kick", "808 Kick Long", 0, decay=1500, pitch=3, pdecay=22, click=18, tone=40)
S("Kick", "808 Kick Short", 0, decay=260, pitch=4, pdecay=18, click=25, tone=50)
S("Kick", "808 Kick Tight", 0, decay=320, pitch=7, pdecay=14, click=40, tone=55)
S("Kick", "808 Kick Saturated", 0, decay=600, pitch=4, pdecay=20, click=25, color=60, drive=30)
S("Kick", "808 Kick Deep", 0, tune=-3, decay=950, pitch=3, pdecay=24, click=15, tone=35)
S("Kick", "808 Kick High", 0, tune=4, decay=420, pitch=4, pdecay=16, click=30)
S("Kick", "909 Kick Classic", 1, decay=450, pitch=24, pdecay=22, snap=40, ndecay=15, click=50, drive=25, color=30, tone=60)
S("Kick", "909 Kick Hard", 1, decay=430, pitch=26, pdecay=20, snap=45, ndecay=14, click=60, drive=60, color=40, tone=65)
S("Kick", "909 Kick Long", 1, decay=850, pitch=22, pdecay=26, snap=35, ndecay=16, click=45, drive=30, color=30)
S("Kick", "909 Kick Punchy", 1, decay=380, pitch=32, pdecay=14, snap=40, ndecay=12, click=65, drive=35, color=25)
S("Kick", "909 Kick Techno", 1, tune=-2, decay=650, pitch=24, pdecay=24, snap=30, ndecay=14, click=45, drive=50, color=45)
S("Kick", "909 Kick Soft", 1, decay=420, pitch=20, pdecay=24, snap=20, ndecay=12, click=20, drive=10, color=20, tone=45)
S("Kick", "Trap Kick", 2, decay=360, pitch=30, pdecay=30, click=55, color=50, drive=20, snap=10, ndecay=10, tone=65)
S("Kick", "Rage Kick", 2, decay=300, pitch=34, pdecay=26, click=70, color=70, drive=60, snap=15, ndecay=10, tone=75)
S("Kick", "Drill Kick", 2, tune=1, decay=270, pitch=34, pdecay=24, click=45, color=45, drive=25, tone=60)
S("Kick", "Pop Kick", 2, decay=300, pitch=26, pdecay=28, click=40, color=40, drive=15)
S("Kick", "EDM Kick", 2, decay=520, pitch=36, pdecay=40, click=60, color=55, drive=30, tone=70)
S("Kick", "Hardstyle Kick", 2, tune=1, decay=720, pitch=40, pdecay=60, click=60, color=90, drive=90, tone=80)
S("Kick", "Techno Rumble Kick", 1, tune=-3, decay=950, pitch=22, pdecay=30, snap=25, ndecay=14, click=35, drive=55, color=50, send=35)
S("Kick", "Phonk Kick", 2, decay=420, pitch=30, pdecay=32, click=50, color=65, drive=75, tone=55)
S("Kick", "Afro Kick", 2, tune=2, decay=230, pitch=18, pdecay=22, click=30, color=30, drive=10)
S("Kick", "House Kick", 1, decay=400, pitch=26, pdecay=30, snap=25, ndecay=12, click=40, drive=20, color=20, tone=55)
S("Kick", "Deep House Kick", 1, tune=-2, decay=480, pitch=20, pdecay=34, snap=15, ndecay=12, click=25, drive=10, color=10, tone=40)
S("Kick", "Hyperpop Kick", 2, tune=2, decay=220, pitch=38, pdecay=20, click=80, color=80, drive=85, tone=85)
S("Kick", "Jersey Kick", 2, decay=200, pitch=28, pdecay=18, click=55, color=50, drive=30)
S("Kick", "Lo-Fi Kick", 0, decay=300, pitch=6, pdecay=20, click=12, tone=25, drive=15)
S("Kick", "Boom Bap Kick", 3, decay=320, pitch=6, pdecay=28, tone=35, snap=35, ndecay=25, color=45, drive=25, click=25)
S("Kick", "Dusty Kick", 3, tune=-1, decay=280, pitch=5, pdecay=26, tone=28, snap=25, ndecay=20, color=30, drive=35, click=15)
S("Kick", "Acoustic Kick Dry", 3, decay=260, pitch=6, pdecay=30, tone=45, snap=45, ndecay=22, color=40, click=35)
S("Kick", "Acoustic Kick Open", 3, decay=650, pitch=5, pdecay=36, tone=50, snap=40, ndecay=28, color=70, click=30)
S("Kick", "Funk Kick", 3, tune=1, decay=240, pitch=7, pdecay=24, tone=60, snap=55, ndecay=20, color=40, click=40)
S("Kick", "Jazz Kick", 3, tune=5, decay=520, pitch=4, pdecay=30, tone=45, snap=20, ndecay=24, color=75, click=15)
S("Kick", "Rock Kick", 3, decay=350, pitch=8, pdecay=26, tone=60, snap=60, ndecay=18, color=50, drive=30, click=55)
S("Kick", "Electro Kick", 0, tune=2, decay=380, pitch=12, pdecay=18, click=45, color=30, drive=20, tone=60)
S("Kick", "Synthwave Kick", 1, decay=500, pitch=18, pdecay=30, snap=20, ndecay=14, click=30, drive=15, color=10, send=20)
S("Kick", "Minimal Kick", 0, tune=3, decay=200, pitch=8, pdecay=14, click=35, tone=60)
S("Kick", "Reggaeton Kick", 2, decay=330, pitch=24, pdecay=30, click=45, color=40, drive=20)
S("Kick", "Industrial Kick", 1, tune=-1, decay=500, pitch=28, pdecay=22, snap=50, ndecay=20, click=70, drive=95, color=60)
S("808", "808 Boom Long", 4, decay=2400, pitch=7, pdecay=40, color=15, click=15)
S("808", "808 Boom Distorted", 4, decay=2000, pitch=9, pdecay=36, color=65, drive=50, click=20)
S("808", "808 Boom Short", 4, decay=850, pitch=8, pdecay=32, color=20, click=20)
S("808", "Sub Boom", 4, tune=-2, decay=3200, pitch=4, pdecay=40, color=0, click=10)
S("808", "808 Boom Punch", 4, decay=1600, pitch=14, pdecay=26, color=30, click=35, drive=20)
S("808", "Rage 808 Boom", 4, decay=2200, pitch=10, pdecay=30, color=80, drive=70, click=25)
# ---------------------------------------------------------------- snares
S("Snare", "808 Snare Classic", 5, decay=120, pitch=2, pdecay=15, tone=55, snap=55, ndecay=160, color=40, click=20)
S("Snare", "808 Snare Snappy", 5, decay=110, pitch=2, pdecay=15, tone=65, snap=85, ndecay=230, color=40, click=25)
S("Snare", "808 Snare Tight", 5, tune=3, decay=80, pitch=2, pdecay=12, tone=60, snap=60, ndecay=100, color=35, click=30)
S("Snare", "808 Snare Low", 5, tune=-4, decay=160, pitch=3, pdecay=18, tone=50, snap=50, ndecay=180, color=30, click=15)
S("Snare", "909 Snare Classic", 6, decay=140, pitch=5, pdecay=18, tone=70, snap=65, ndecay=210, color=40, click=25)
S("Snare", "909 Snare Bright", 6, tune=2, decay=120, pitch=5, pdecay=16, tone=85, snap=75, ndecay=200, color=45, click=35)
S("Snare", "909 Snare Fat", 6, tune=-2, decay=220, pitch=6, pdecay=22, tone=65, snap=60, ndecay=260, color=35, drive=30, click=25)
S("Snare", "909 Snare Short", 6, tune=1, decay=90, pitch=4, pdecay=14, tone=75, snap=60, ndecay=120, color=40, click=30)
S("Snare", "Trap Snare", 7, tune=2, decay=150, pitch=10, pdecay=12, tone=70, snap=75, ndecay=180, color=0, click=45, drive=20)
S("Snare", "Drill Snare", 7, tune=6, decay=110, pitch=12, pdecay=10, tone=80, snap=80, ndecay=150, color=0, click=60, drive=15)
S("Snare", "Rage Snare", 7, tune=3, decay=140, pitch=12, pdecay=12, tone=80, snap=80, ndecay=170, color=30, click=60, drive=60)
S("Snare", "Pop Snare", 7, decay=170, pitch=8, pdecay=14, tone=65, snap=70, ndecay=220, color=50, click=40, send=30)
S("Snare", "Clap Snare", 7, tune=1, decay=130, pitch=8, pdecay=12, tone=70, snap=80, ndecay=200, color=100, click=35)
S("Snare", "Hyperpop Snare", 7, tune=7, decay=100, pitch=14, pdecay=10, tone=90, snap=85, ndecay=140, color=60, click=70, drive=70)
S("Snare", "Phonk Snare", 7, tune=-1, decay=160, pitch=8, pdecay=14, tone=55, snap=70, ndecay=200, color=40, click=40, drive=55)
S("Snare", "Jersey Snare", 7, tune=4, decay=100, pitch=10, pdecay=10, tone=75, snap=75, ndecay=130, color=70, click=45)
S("Snare", "Boom Bap Snare", 8, decay=180, pitch=3, pdecay=18, tone=45, snap=70, ndecay=230, color=60, drive=25, click=40)
S("Snare", "Dusty Snare", 8, tune=-1, decay=170, pitch=3, pdecay=18, tone=35, snap=65, ndecay=210, color=50, drive=35, click=30)
S("Snare", "Lo-Fi Snare", 8, tune=1, decay=150, pitch=2, pdecay=16, tone=30, snap=60, ndecay=200, color=55, drive=20, click=20)
S("Snare", "Rimshot Snare", 8, tune=2, decay=160, pitch=4, pdecay=14, tone=60, snap=55, ndecay=200, color=40, click=90)
S("Snare", "Acoustic Snare Tight", 8, tune=2, decay=150, pitch=3, pdecay=15, tone=60, snap=65, ndecay=180, color=60, click=45)
S("Snare", "Acoustic Snare Fat", 8, tune=-3, decay=250, pitch=3, pdecay=20, tone=50, snap=70, ndecay=340, color=70, click=35)
S("Snare", "Funk Snare", 8, tune=4, decay=140, pitch=3, pdecay=14, tone=65, snap=65, ndecay=190, color=55, click=50)
S("Snare", "Piccolo Snare", 8, tune=9, decay=100, pitch=3, pdecay=12, tone=70, snap=70, ndecay=150, color=60, click=50)
S("Snare", "Brush Snare", 8, decay=30, pitch=0, pdecay=10, tone=30, snap=100, ndecay=420, color=100, click=0, vel=90)
S("Snare", "Rock Snare", 8, decay=200, pitch=4, pdecay=18, tone=60, snap=75, ndecay=260, color=70, drive=35, click=60, send=25)
S("Snare", "Electro Snare", 5, tune=5, decay=90, pitch=6, pdecay=12, tone=75, snap=70, ndecay=150, color=50, drive=25, click=30)
S("Snare", "Synthwave Snare", 6, tune=-1, decay=180, pitch=6, pdecay=20, tone=65, snap=70, ndecay=300, color=40, send=60)
S("Snare", "Gated Snare", 6, decay=160, pitch=5, pdecay=16, tone=70, snap=85, ndecay=240, color=45, drive=40, send=70)
S("Snare", "Reggaeton Snare", 7, tune=3, decay=120, pitch=10, pdecay=12, tone=70, snap=70, ndecay=150, color=20, click=40)
S("Snare", "Garage Snare", 7, tune=5, decay=110, pitch=8, pdecay=10, tone=75, snap=70, ndecay=140, color=30, click=45)
S("Snare", "Industrial Snare", 6, tune=-2, decay=200, pitch=8, pdecay=16, tone=80, snap=90, ndecay=280, color=60, drive=90, click=60)
# ---------------------------------------------------------------- claps and snaps
S("Clap", "808 Clap", 9, decay=260, tone=45, color=40, click=10)
S("Clap", "909 Clap", 9, decay=320, tone=55, color=25, click=15)
S("Clap", "Trap Clap", 9, decay=230, tone=62, color=30, drive=20, click=20)
S("Clap", "Big Room Clap", 9, decay=520, tone=55, color=35, send=45)
S("Clap", "Tight Clap", 9, decay=120, tone=60, color=15, click=20)
S("Clap", "Lo-Fi Clap", 9, decay=220, tone=30, color=40, drive=20)
S("Clap", "Jersey Clap", 9, decay=150, tone=65, color=20, click=30)
S("Clap", "Loose Clap", 9, decay=300, tone=50, color=90)
S("Clap", "Distorted Clap", 9, decay=240, tone=55, color=35, drive=75, click=20)
S("Clap", "Hall Clap", 9, decay=350, tone=50, color=45, send=80)
S("Clap", "Finger Snap", 10, decay=90, tone=55, color=30, click=30)
S("Clap", "Snap Tight", 10, decay=60, tone=65, color=15, click=40)
S("Clap", "Snap Big", 10, decay=140, tone=50, color=50, click=30, send=45)
S("Clap", "Snap Low", 10, tune=-5, decay=110, tone=40, color=60, click=25)
# ---------------------------------------------------------------- hats and cymbals
S("Hat", "808 Hat Closed", 13, decay=45, tone=65, color=0, click=10)
S("Hat", "909 Hat Closed", 13, decay=55, tone=80, color=70, click=15)
S("Hat", "Trap Hat", 13, decay=38, tone=85, color=40, click=20)
S("Hat", "Drill Hat", 13, tune=2, decay=30, tone=90, color=45, click=25)
S("Hat", "Tight Hat", 13, decay=20, tone=80, color=30, click=20)
S("Hat", "Lo-Fi Hat", 13, decay=50, tone=40, color=50, click=10)
S("Hat", "Dusty Hat", 13, decay=60, tone=35, color=65, drive=20)
S("Hat", "Metallic Hat", 13, tune=3, decay=50, tone=55, color=0, click=15)
S("Hat", "Noise Hat", 13, decay=40, tone=85, color=100, click=10)
S("Hat", "Rage Hat", 13, decay=35, tone=90, color=50, drive=50, click=30)
S("Hat", "House Hat", 13, decay=70, tone=75, color=60, click=10)
S("Hat", "Shuffle Hat", 13, decay=90, tone=70, color=55, click=10)
S("Open Hat", "808 Open Hat", 14, decay=480, tone=65, color=0)
S("Open Hat", "909 Open Hat", 14, decay=420, tone=80, color=70)
S("Open Hat", "Trap Open Hat", 14, decay=320, tone=85, color=40)
S("Open Hat", "Long Open Hat", 14, decay=950, tone=70, color=50)
S("Open Hat", "Pedal Hat", 14, decay=130, tone=55, color=30)
S("Open Hat", "Lo-Fi Open Hat", 14, decay=350, tone=40, color=55)
S("Open Hat", "House Open Hat", 14, decay=260, tone=78, color=65)
S("Open Hat", "Sizzle Hat", 14, decay=700, tone=88, color=80)
S("Cymbal", "808 Cymbal", 15, decay=1800, tone=60, color=30, snap=30, ndecay=1200)
S("Cymbal", "Crash", 15, decay=2600, tone=65, color=70, snap=60, ndecay=2000)
S("Cymbal", "Ride", 15, decay=2100, tone=72, color=12, snap=15, ndecay=600, click=55)
S("Cymbal", "Ride Bell", 15, tune=5, decay=1300, tone=60, color=0, click=40)
S("Cymbal", "China", 15, tune=-2, decay=1700, tone=55, color=60, snap=50, ndecay=1200, drive=50)
S("Cymbal", "Splash", 15, tune=3, decay=750, tone=75, color=60, snap=50, ndecay=600)
S("Cymbal", "Trash Crash", 15, decay=1400, tone=70, color=75, snap=70, ndecay=1000, drive=70)
# ---------------------------------------------------------------- percussion
S("Perc", "808 Cowbell", 16, decay=260, tone=40, color=0)
S("Perc", "Phonk Cowbell", 16, decay=480, tone=55, color=10, drive=40)
S("Perc", "Agogo High", 16, tune=7, decay=190, tone=55, color=20)
S("Perc", "Agogo Low", 16, tune=2, decay=220, tone=45, color=20)
S("Perc", "Distorted Cowbell", 16, decay=400, tone=50, color=15, drive=75)
S("Perc", "808 Rim", 11, decay=30, color=40, drive=20, click=30)
S("Perc", "Rim Wood", 11, decay=40, color=80, click=20)
S("Perc", "Rim High", 11, tune=5, decay=25, color=30, click=40)
S("Perc", "808 Clave", 12, decay=60, color=60)
S("Perc", "Woodblock", 12, tune=-5, decay=80, color=40)
S("Perc", "Conga Low", 19, tune=-2, decay=220, pitch=2, pdecay=20, color=30, click=20)
S("Perc", "Conga High", 19, tune=4, decay=180, pitch=2, pdecay=18, color=30, click=20)
S("Perc", "Bongo", 19, tune=12, decay=110, pitch=2, pdecay=12, color=40, click=25)
S("Perc", "Slap Conga", 19, tune=3, decay=140, pitch=3, pdecay=14, color=85, click=30)
S("Perc", "Timbale", 19, tune=9, decay=260, pitch=1, pdecay=10, color=30, drive=20, click=40, send=20)
S("Perc", "Shaker", 20, decay=90, tone=70, color=30)
S("Perc", "Shaker Soft", 20, decay=120, tone=60, color=75)
S("Perc", "Maracas", 20, decay=50, tone=85, color=15)
S("Perc", "Cabasa", 20, decay=70, tone=78, color=45)
S("Perc", "Tambourine", 21, decay=210, tone=60, color=40)
S("Perc", "Tambourine Shake", 21, decay=320, tone=65, color=90)
S("Perc", "FM Woodblock", 22, tune=7, decay=80, pitch=12, pdecay=10, color=15)
S("Perc", "FM Bell", 22, tune=12, decay=900, pitch=18, pdecay=300, color=80, send=30)
S("Perc", "FM Marimba", 22, decay=420, pitch=8, pdecay=50, color=45)
S("Perc", "FM Metal", 22, tune=5, decay=300, pitch=30, pdecay=80, color=95)
S("Perc", "FM Pluck", 22, tune=12, decay=180, pitch=20, pdecay=25, color=30)
S("Perc", "FM Log Drum", 22, tune=-12, decay=350, pitch=10, pdecay=30, color=15, drive=20)
S("Tom", "808 Tom Low", 17, tune=-5, decay=450, pitch=4, pdecay=60, snap=10, color=20)
S("Tom", "808 Tom Mid", 17, tune=0, decay=380, pitch=4, pdecay=55, snap=10, color=20)
S("Tom", "808 Tom High", 17, tune=5, decay=320, pitch=4, pdecay=50, snap=10, color=20)
S("Tom", "909 Tom Low", 17, tune=-4, decay=400, pitch=8, pdecay=40, snap=40, ndecay=60, color=60, click=30)
S("Tom", "909 Tom High", 17, tune=5, decay=300, pitch=8, pdecay=35, snap=40, ndecay=50, color=60, click=30)
S("Tom", "Simmons Tom Low", 18, tune=-4, decay=550, pitch=12, pdecay=200, snap=40, ndecay=100, color=50, send=30)
S("Tom", "Simmons Tom Mid", 18, tune=1, decay=480, pitch=12, pdecay=180, snap=40, ndecay=90, color=50, send=30)
S("Tom", "Simmons Tom High", 18, tune=6, decay=420, pitch=12, pdecay=160, snap=40, ndecay=80, color=50, send=30)
S("Tom", "Acoustic Tom Floor", 17, tune=-7, decay=650, pitch=3, pdecay=60, snap=25, ndecay=70, color=60, click=30, send=20)
S("Tom", "Acoustic Tom Rack", 17, tune=2, decay=480, pitch=3, pdecay=50, snap=25, ndecay=60, color=60, click=30, send=20)
S("Tom", "Electro Tom", 18, tune=0, decay=300, pitch=20, pdecay=80, snap=10, color=20)
# ---------------------------------------------------------------- FX
S("FX", "Laser Zap", 23, tune=12, decay=160, pitch=48, pdecay=40)
S("FX", "Pew", 23, tune=24, decay=90, pitch=36, pdecay=20, color=60)
S("FX", "Electro Zap", 23, tune=6, decay=220, pitch=40, pdecay=30, color=100, drive=40)
S("FX", "Drop Zap", 23, tune=-12, decay=900, pitch=48, pdecay=400)
S("FX", "Noise Sweep", 24, decay=900, pitch=36, pdecay=500, tone=30, color=50, send=40)
S("FX", "Noise Burst", 24, decay=180, pitch=12, pdecay=60, tone=60, color=20)
S("FX", "White Crash", 24, decay=1500, pitch=0, pdecay=100, tone=75, color=0, send=50)
S("FX", "Vinyl Scratch", 24, decay=120, pitch=24, pdecay=40, tone=50, color=90, drive=30)
S("FX", "Reverse Tail", 9, decay=900, tone=40, color=95, send=90)

SOUND_BY_NAME = {n: (c, d) for c, n, d in SOUNDS}

# ---------------------------------------------------------------- kits
# pads: kick, snare, clap, closed hat, open hat, perc/tom, rim/perc, cymbal/fx. Notes C1 D1 D#1 F#1 A#1 A1 C#1 C#2.
NOTES = [36, 38, 39, 42, 46, 45, 37, 49]
STRIKE = []
def kit(cat, name, pads, char=0, amount=60, glue=20, clip=20, vmix=100, vsize=35, human=15, width=100, vol=0, melodic=0, levels=None, tunes=None):
    vals = dict(character=char, amount=amount, glue=glue, clip=clip, vmix=vmix, vsize=vsize, humanize=human, width=width, volume=vol, melodic=melodic)
    assert len(pads) == 8, name
    for i, sname in enumerate(pads):
        c, d = SOUND_BY_NAME[sname]
        for k, v in d.items(): vals[f"p{i+1}{k}"] = v
        vals[f"p{i+1}note"] = NOTES[i]
        vals[f"p{i+1}choke"] = 1 if i in (3, 4) else 0
        vals[f"p{i+1}sound"] = sname
        if levels and levels[i]: vals[f"p{i+1}level"] = vals.get(f"p{i+1}level", 0) + levels[i]
        if tunes and tunes[i]: vals[f"p{i+1}tune"] = vals.get(f"p{i+1}tune", 0) + tunes[i]
    STRIKE.append((cat, name, vals))

# classic machines
kit("Classic", "808 Classic", ["808 Kick Classic", "808 Snare Classic", "808 Clap", "808 Hat Closed", "808 Open Hat", "808 Cowbell", "808 Rim", "808 Cymbal"], char=1, amount=30)
kit("Classic", "808 Long Boom", ["808 Kick Long", "808 Snare Snappy", "808 Clap", "808 Hat Closed", "808 Open Hat", "808 Tom Low", "808 Clave", "808 Cymbal"], char=1, amount=30)
kit("Classic", "909 Classic", ["909 Kick Classic", "909 Snare Classic", "909 Clap", "909 Hat Closed", "909 Open Hat", "909 Tom Low", "808 Rim", "Crash"], char=1, amount=25)
kit("Classic", "909 Hard", ["909 Kick Hard", "909 Snare Bright", "909 Clap", "909 Hat Closed", "909 Open Hat", "909 Tom High", "Rim High", "Ride"], char=7, amount=40, glue=35)
kit("Classic", "606 Style", ["808 Kick Tight", "808 Snare Tight", "Tight Clap", "Metallic Hat", "Pedal Hat", "808 Tom High", "808 Rim", "808 Cymbal"], char=1, amount=40, levels=[0, 0, -3, 0, 0, 0, 0, -3])
kit("Classic", "707 Style", ["909 Kick Soft", "909 Snare Short", "909 Clap", "Noise Hat", "909 Open Hat", "909 Tom High", "Rim High", "Crash"], char=4, amount=35)
kit("Classic", "LinnDrum Style", ["Acoustic Kick Dry", "Acoustic Snare Tight", "Snap Tight", "House Hat", "Pedal Hat", "Acoustic Tom Rack", "808 Rim", "Tambourine"], char=4, amount=70, vsize=30)
kit("Classic", "CR-78 Style", ["Minimal Kick", "808 Snare Tight", "Snap Tight", "Metallic Hat", "Pedal Hat", "Bongo", "Woodblock", "Maracas"], char=1, amount=50, levels=[0, -2, -4, 0, 0, 0, 0, 0])
kit("Classic", "Simmons 80s", ["Synthwave Kick", "Gated Snare", "Big Room Clap", "House Hat", "House Open Hat", "Simmons Tom Mid", "Electro Tom", "Crash"], char=1, amount=30, vsize=55)
kit("Classic", "Electro 82", ["Electro Kick", "Electro Snare", "808 Clap", "808 Hat Closed", "808 Open Hat", "808 Cowbell", "808 Clave", "Laser Zap"], char=1, amount=40)
# vintage samplers
kit("Vintage", "SP-1200 Boom Bap", ["Boom Bap Kick", "Boom Bap Snare", "Lo-Fi Clap", "Dusty Hat", "Lo-Fi Open Hat", "Conga Low", "Rim Wood", "Tambourine"], char=3, amount=80, glue=35, human=30)
kit("Vintage", "MPC60 Dusty", ["Dusty Kick", "Dusty Snare", "Lo-Fi Clap", "Lo-Fi Hat", "Lo-Fi Open Hat", "Shaker Soft", "Rim Wood", "Ride"], char=2, amount=80, glue=30, human=35)
kit("Vintage", "MPC Soul Kit", ["Funk Kick", "Funk Snare", "Snap Big", "Shuffle Hat", "Pedal Hat", "Conga High", "Rim Wood", "Tambourine"], char=2, amount=70, human=35)
kit("Vintage", "SP Crunch", ["Rock Kick", "Rimshot Snare", "Distorted Clap", "Dusty Hat", "Lo-Fi Open Hat", "Timbale", "808 Rim", "China"], char=3, amount=100, glue=50, clip=40, human=25)
kit("Vintage", "Linn Funk", ["Funk Kick", "Acoustic Snare Tight", "Snap Tight", "House Hat", "Pedal Hat", "Conga High", "Agogo High", "Tambourine Shake"], char=4, amount=80, human=20)
kit("Vintage", "Cassette Beat", ["Lo-Fi Kick", "Lo-Fi Snare", "Lo-Fi Clap", "Lo-Fi Hat", "Lo-Fi Open Hat", "FM Marimba", "Rim Wood", "Shaker Soft"], char=5, amount=80, human=30)
kit("Vintage", "Vinyl Breaks", ["Acoustic Kick Open", "Acoustic Snare Fat", "Snap Big", "Shuffle Hat", "Lo-Fi Open Hat", "Acoustic Tom Floor", "Rimshot Snare", "Ride"], char=6, amount=85, human=35, vsize=40)
kit("Vintage", "Dilla Kit", ["Dusty Kick", "Lo-Fi Snare", "Snap Low", "Lo-Fi Hat", "Pedal Hat", "Shaker Soft", "Rim Wood", "Ride Bell"], char=2, amount=90, human=45, glue=30)
kit("Vintage", "Madlib Crate", ["Boom Bap Kick", "Dusty Snare", "Loose Clap", "Dusty Hat", "Lo-Fi Open Hat", "Bongo", "Woodblock", "Tambourine"], char=6, amount=90, human=40)
kit("Vintage", "Jazz Brushes", ["Jazz Kick", "Brush Snare", "Finger Snap", "Shuffle Hat", "Pedal Hat", "Acoustic Tom Rack", "Rim Wood", "Ride"], char=6, amount=50, human=50, vsize=45)
# hip-hop now
kit("Trap", "Trap Modern", ["Trap Kick", "Trap Snare", "Trap Clap", "Trap Hat", "Trap Open Hat", "808 Boom Long", "Rim High", "Crash"], char=7, amount=40, glue=30)
kit("Trap", "Trap Dark", ["808 Kick Deep", "Clap Snare", "Trap Clap", "Trap Hat", "Trap Open Hat", "808 Boom Long", "Agogo Low", "Noise Sweep"], char=7, amount=35, glue=30)
kit("Trap", "Atlanta Bounce", ["Trap Kick", "808 Snare Snappy", "808 Clap", "Trap Hat", "808 Open Hat", "808 Boom Punch", "808 Rim", "808 Cymbal"], char=1, amount=40)
kit("Trap", "Drill UK", ["Drill Kick", "Drill Snare", "Trap Clap", "Drill Hat", "Trap Open Hat", "808 Boom Long", "Rim High", "Crash"], char=7, amount=35, glue=25)
kit("Trap", "NY Drill", ["Drill Kick", "Drill Snare", "Tight Clap", "Drill Hat", "Pedal Hat", "808 Boom Distorted", "808 Rim", "Laser Zap"], char=7, amount=45, glue=30)
kit("Trap", "Rage Kit", ["Rage Kick", "Rage Snare", "Distorted Clap", "Rage Hat", "Trap Open Hat", "Rage 808 Boom", "Electro Zap", "Trash Crash"], char=7, amount=60, glue=40, clip=45)
kit("Trap", "Phonk Kit", ["Phonk Kick", "Phonk Snare", "Lo-Fi Clap", "Dusty Hat", "Lo-Fi Open Hat", "808 Boom Distorted", "Phonk Cowbell", "Vinyl Scratch"], char=5, amount=55, glue=40, clip=40)
kit("Trap", "Pluggnb Kit", ["808 Kick Short", "Clap Snare", "Trap Clap", "Trap Hat", "Trap Open Hat", "808 Boom Long", "FM Bell", "Ride Bell"], char=1, amount=30, vsize=45)
kit("Trap", "Memphis Kit", ["808 Kick Saturated", "808 Snare Snappy", "Lo-Fi Clap", "808 Hat Closed", "808 Open Hat", "Phonk Cowbell", "808 Rim", "808 Cymbal"], char=5, amount=70, glue=30)
kit("Trap", "Detroit Kit", ["Trap Kick", "Tight Clap", "Jersey Clap", "Tight Hat", "Pedal Hat", "808 Boom Punch", "Rim High", "Laser Zap"], char=7, amount=40, glue=35)
kit("Trap", "Hyperpop Kit", ["Hyperpop Kick", "Hyperpop Snare", "Distorted Clap", "Rage Hat", "Sizzle Hat", "Rage 808 Boom", "Pew", "Trash Crash"], char=7, amount=80, glue=45, clip=60)
kit("Hip-Hop", "Boom Bap Classic", ["Boom Bap Kick", "Boom Bap Snare", "Snap Big", "Dusty Hat", "Lo-Fi Open Hat", "Conga Low", "Rim Wood", "Ride"], char=3, amount=60, human=30)
kit("Hip-Hop", "West Coast", ["808 Kick Classic", "Clap Snare", "808 Clap", "808 Hat Closed", "808 Open Hat", "Tambourine", "808 Rim", "Laser Zap"], char=1, amount=40, human=20)
kit("Hip-Hop", "Jersey Club", ["Jersey Kick", "Jersey Snare", "Jersey Clap", "Tight Hat", "Pedal Hat", "808 Boom Short", "Rim High", "Noise Burst"], char=7, amount=35)
kit("Hip-Hop", "Lo-Fi Hip-Hop", ["Lo-Fi Kick", "Lo-Fi Snare", "Snap Low", "Lo-Fi Hat", "Lo-Fi Open Hat", "Shaker Soft", "Rim Wood", "Ride Bell"], char=5, amount=90, human=40)
kit("Hip-Hop", "Southern Soul", ["Acoustic Kick Dry", "Funk Snare", "Snap Big", "Shuffle Hat", "Pedal Hat", "Tambourine", "Rim Wood", "Ride"], char=6, amount=60, human=35)
# electronic
kit("House", "House Classic", ["House Kick", "909 Snare Short", "909 Clap", "House Hat", "House Open Hat", "Shaker", "Rim High", "Ride"], char=1, amount=30)
kit("House", "Deep House", ["Deep House Kick", "808 Snare Tight", "808 Clap", "House Hat", "House Open Hat", "Conga High", "Rim Wood", "Ride"], char=1, amount=30, vsize=45)
kit("House", "Garage 2-Step", ["House Kick", "Garage Snare", "Tight Clap", "Shuffle Hat", "House Open Hat", "808 Boom Short", "Rim High", "Shaker"], char=7, amount=25, human=25)
kit("House", "Disco", ["Acoustic Kick Dry", "Funk Snare", "Big Room Clap", "House Hat", "Sizzle Hat", "Conga High", "Tambourine", "Crash"], char=6, amount=40, human=30)
kit("House", "Afro House", ["Afro Kick", "808 Snare Tight", "Tight Clap", "Shaker", "House Open Hat", "Conga Low", "Agogo High", "Bongo"], char=1, amount=30, human=30)
kit("Techno", "Techno Warehouse", ["909 Kick Techno", "909 Snare Fat", "909 Clap", "909 Hat Closed", "909 Open Hat", "Techno Rumble Kick", "Rim High", "Ride"], char=7, amount=40, glue=35, vsize=55)
kit("Techno", "Berlin Minimal", ["Minimal Kick", "808 Snare Tight", "Tight Clap", "Metallic Hat", "Pedal Hat", "FM Woodblock", "808 Clave", "FM Metal"], char=1, amount=30)
kit("Techno", "Industrial", ["Industrial Kick", "Industrial Snare", "Distorted Clap", "Rage Hat", "Sizzle Hat", "Distorted Cowbell", "FM Metal", "Trash Crash"], char=7, amount=70, glue=50, clip=55, vsize=60)
kit("Techno", "Acid Techno", ["909 Kick Punchy", "909 Snare Bright", "909 Clap", "909 Hat Closed", "909 Open Hat", "909 Tom High", "Rim High", "Electro Zap"], char=7, amount=40, glue=35)
kit("Electronic", "EDM Festival", ["EDM Kick", "Pop Snare", "Big Room Clap", "Trap Hat", "Trap Open Hat", "Noise Sweep", "Rim High", "Crash"], char=7, amount=60, glue=40, clip=35)
kit("Electronic", "Hardstyle", ["Hardstyle Kick", "Industrial Snare", "Big Room Clap", "Noise Hat", "Sizzle Hat", "White Crash", "Pew", "Crash"], char=7, amount=80, glue=45, clip=60)
kit("Electronic", "Synthwave", ["Synthwave Kick", "Synthwave Snare", "Big Room Clap", "House Hat", "House Open Hat", "Simmons Tom Low", "Simmons Tom High", "Crash"], char=1, amount=30, vsize=60)
kit("Electronic", "80s Pop", ["Acoustic Kick Dry", "Gated Snare", "Hall Clap", "House Hat", "Pedal Hat", "Simmons Tom Mid", "808 Rim", "Tambourine"], char=4, amount=60, vsize=55)
kit("Electronic", "Drum and Bass", ["909 Kick Punchy", "Funk Snare", "Tight Clap", "Tight Hat", "House Open Hat", "Acoustic Tom Rack", "Rim High", "Ride"], char=7, amount=30, glue=35, human=25)
kit("Electronic", "Jungle Breaks", ["Acoustic Kick Dry", "Rimshot Snare", "Snap Tight", "Shuffle Hat", "Lo-Fi Open Hat", "Conga High", "Rim Wood", "Ride"], char=3, amount=70, human=35, glue=35)
kit("Electronic", "Glitch", ["Minimal Kick", "Electro Snare", "Tight Clap", "Noise Hat", "Pedal Hat", "FM Pluck", "Pew", "Vinyl Scratch"], char=3, amount=60)
kit("Electronic", "Ambient", ["Deep House Kick", "Brush Snare", "Snap Big", "Shaker Soft", "Long Open Hat", "FM Bell", "FM Marimba", "Noise Sweep"], char=5, amount=40, vsize=85, human=30)
# world and pop
kit("World", "Afrobeats", ["Afro Kick", "Reggaeton Snare", "Tight Clap", "Shaker", "Pedal Hat", "Conga Low", "Agogo High", "Tambourine"], char=1, amount=30, human=30)
kit("World", "Amapiano", ["Afro Kick", "808 Snare Tight", "Tight Clap", "Shaker", "House Open Hat", "FM Log Drum", "Woodblock", "Cabasa"], char=1, amount=30, human=25, vsize=45)
kit("World", "Reggaeton", ["Reggaeton Kick", "Reggaeton Snare", "Tight Clap", "Trap Hat", "Pedal Hat", "Timbale", "Rim High", "Shaker"], char=7, amount=30)
kit("World", "Dancehall", ["808 Kick Tight", "Reggaeton Snare", "Jersey Clap", "Tight Hat", "Pedal Hat", "Timbale", "808 Rim", "Laser Zap"], char=1, amount=35, human=20)
kit("World", "Latin Percussion", ["Acoustic Kick Dry", "Rimshot Snare", "Finger Snap", "Shaker", "Pedal Hat", "Conga High", "Agogo High", "Bongo"], char=6, amount=40, human=40)
kit("World", "Baile Funk", ["808 Kick Short", "Clap Snare", "Jersey Clap", "Trap Hat", "Pedal Hat", "Conga Low", "Agogo Low", "Pew"], char=7, amount=40)
kit("Pop", "Pop Radio", ["Pop Kick", "Pop Snare", "Big Room Clap", "Trap Hat", "Trap Open Hat", "808 Boom Short", "Finger Snap", "Crash"], char=7, amount=30, glue=30)
kit("Pop", "Pop Ballad", ["Acoustic Kick Open", "Acoustic Snare Fat", "Hall Clap", "House Hat", "Pedal Hat", "Acoustic Tom Floor", "Rim Wood", "Crash"], char=1, amount=30, vsize=60, human=30)
kit("Pop", "R&B Smooth", ["808 Kick Short", "Clap Snare", "Finger Snap", "Trap Hat", "Pedal Hat", "808 Boom Long", "Rim Wood", "FM Bell"], char=1, amount=30, vsize=45, human=20)
kit("Pop", "Rock Room", ["Rock Kick", "Rock Snare", "Big Room Clap", "Shuffle Hat", "Sizzle Hat", "Acoustic Tom Floor", "Acoustic Tom Rack", "Crash"], char=1, amount=40, vsize=45, human=35, glue=40)
kit("Pop", "Funk Live", ["Funk Kick", "Funk Snare", "Snap Big", "Shuffle Hat", "Pedal Hat", "Acoustic Tom Rack", "Rimshot Snare", "Ride"], char=6, amount=40, human=40)
# kick-led melodic kits: the 808 boom plays chromatically from C3
kit("Melodic", "Trap 808 Keys", ["Trap Kick", "Trap Snare", "Trap Clap", "Trap Hat", "Trap Open Hat", "808 Boom Long", "Rim High", "Crash"], char=7, amount=40, glue=30, melodic=6)
kit("Melodic", "Rage 808 Keys", ["Rage Kick", "Rage Snare", "Distorted Clap", "Rage Hat", "Trap Open Hat", "Rage 808 Boom", "Electro Zap", "Trash Crash"], char=7, amount=60, glue=40, clip=45, melodic=6)
kit("Melodic", "Simmons Tom Keys", ["Synthwave Kick", "Gated Snare", "Big Room Clap", "House Hat", "House Open Hat", "Simmons Tom Mid", "Electro Tom", "Crash"], char=1, amount=30, vsize=55, melodic=6)
kit("Melodic", "Log Drum Keys", ["Afro Kick", "808 Snare Tight", "Tight Clap", "Shaker", "House Open Hat", "FM Log Drum", "Woodblock", "Cabasa"], char=1, amount=30, melodic=6)
kit("Melodic", "FM Bell Keys", ["Deep House Kick", "Brush Snare", "Snap Big", "Shaker Soft", "Long Open Hat", "FM Bell", "FM Marimba", "Noise Sweep"], char=5, amount=40, vsize=85, melodic=6)
# every kit again through the other machines, so any genre can sound vintage or modern
MACHINES = [("SP-1200", 3, 80), ("MPC60", 2, 80), ("Cassette", 5, 80), ("Vinyl", 6, 80), ("Modern", 7, 60), ("LinnDrum", 4, 70)]
for cat, name, vals in list(STRIKE):
    if cat in ("Melodic",): continue
    for mname, mchar, mamt in MACHINES:
        if vals["character"] == mchar: continue
        if cat in ("Vintage",) and mname in ("Modern",): continue
        if cat in ("Trap", "Electronic", "Techno") and mname in ("LinnDrum",): continue
        v = dict(vals); v["character"] = mchar; v["amount"] = mamt
        STRIKE.append((f"{cat} x {mname}", f"{name} ({mname})", v))

# kit levels are measured by tools/strike_level.py; sounds are already level-matched in the library
STRIKE_SOUND_LEVEL = {}
try:
    from strike_levels import STRIKE_SOUND_LEVEL
except ImportError:
    pass
# apply the measured sound levels inside the kits too, and keep only parameter values (drop the sound name tag)
for _c, _n, _v in STRIKE:
    for i in range(1, 9):
        sname = _v.pop(f"p{i}sound")
        _v[f"p{i}level"] = round(_v.get(f"p{i}level", 0.0) + STRIKE_SOUND_LEVEL.get(sname, 0.0), 1)

def write_sounds(path):
    """strike/Sounds.inc: the per-pad library the editor's browser loads from."""
    keys = ["model", "tune", "decay", "pitch", "pdecay", "tone", "snap", "ndecay", "color", "drive", "click", "level", "pan", "vel", "send"]
    defaults = dict(model=0, tune=0, decay=400, pitch=12, pdecay=30, tone=50, snap=30, ndecay=120, color=30, drive=10, click=30, level=0, pan=0, vel=70, send=0)
    with open(path, "w") as f:
        f.write("// Generated by tools/strike_presets.py: the LGN Strike sound library.\n")
        f.write("struct StrikeSound { const char* category; const char* name; float v[15]; };\n")
        f.write("inline const std::vector<StrikeSound>& strikeSounds() {\n    static const std::vector<StrikeSound> s = {\n")
        for c, n, d in SOUNDS:
            vals = dict(defaults); vals.update(d); vals["level"] = round(vals["level"] + STRIKE_SOUND_LEVEL.get(n, 0.0), 1)
            f.write(f'        {{ "{c}", "{n}", {{ ' + ", ".join(f"{float(vals[k])}f" for k in keys) + " } },\n")
        f.write("    };\n    return s;\n}\n")
        f.write('inline const char* const* strikeSoundKeys() { static const char* k[15] = { ' + ", ".join(f'"{k}"' for k in keys) + " }; return k; }\n")
