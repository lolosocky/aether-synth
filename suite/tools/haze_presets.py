"""LGN Haze factory presets. Values are in parameter units (Hz, ms, %, dB, semitones; choices by index).

Choice indexes:
  oNwave 0 sine .. 1 triangle .. 2 saw .. 3 pulse (continuous morph)      oNoct 0=-2 1=-1 2=0 3=+1 4=+2
  subwave 0 sine 1 tri 2 square 3 pulse   suboct 0 same 1 -1 2 -2   noisetype 0 white 1 pink 2 brown
  ftype 0 Ladder24 1 Ladder12 2 SVF LP 3 SVF BP 4 SVF HP 5 Notch 6 Comb+ 7 Comb- 8 Vowel 9 Off
  dirttype 0 Tube 1 Tape 2 Hard 3 Fold 4 Rectify 5 Crush
  glidemode 0 off 1 always 2 legato | glideshape 0 exp 1 linear | trigger 0 retrigger 1 legato | priority 0 last 1 low 2 high
  lNshape 0 sine 1 tri 2 sawup 3 sawdown 4 square 5 S&H 6 smooth
  lNdiv 0 1/32 1 1/16T 2 1/16 3 1/8T 4 1/16D 5 1/8 6 1/4T 7 1/8D 8 1/4 9 1/4D 10 1/2 11 1bar 12 2bars 13 4bars
  mNsrc 0 off 1 LFO1 2 LFO2 3 FiltEnv 4 ModEnv 5 AmpEnv 6 Vel 7 Note 8 Wheel 9 Aftertouch 10 Bend 11 Random
  mNdst 0 off 1 Pitch(24st) 2 O1Pitch 3 O2Pitch 4 O1Wave(3) 5 O2Wave 6 PW 7 FM 8 Detune 9 O1Lvl 10 O2Lvl 11 Sub 12 Noise
        13 Cutoff(96st) 14 Res 15 FDrive(24dB) 16 Dirt 17 Amp 18 Pan 19 LFO1Rate 20 LFO2Rate
  patmode 0 off 1 repeat 2 arp 3 seq | patdiv/ddiv 0 1/64 1 1/32 2 1/16T 3 1/16 4 1/16D 5 1/8T 6 1/8 7 1/8D 8 1/4T 9 1/4 10 1/4D 11 1/2 12 1bar
  arpdir 0 up 1 down 2 updown 3 downup 4 random 5 played | arpoct 0..3 = 1..4 octaves
"""

def H(*bases, **kw):
    d = {}
    for b in bases: d.update(b)
    d.update(kw)
    return d

def mods(*slots):
    """mods((src, dst, amt), ...) -> m1src.. entries"""
    d = {}
    for i, (s, t, a) in enumerate(slots):
        d[f"m{i+1}src"] = s; d[f"m{i+1}dst"] = t; d[f"m{i+1}amt"] = a
    return d

def seq(pattern, length=None):
    """'0a . 12s 7 -' : number = semitones, a = accent, s = slide, '.' = rest"""
    toks = pattern.split()
    d = {"patmode": 3, "seqlen": length or len(toks)}
    for i, t in enumerate(toks[:16]):
        k = f"s{i+1}"
        if t == ".":
            d[k + "on"] = 0; continue
        acc, sl = "a" in t, "s" in t
        num = t.replace("a", "").replace("s", "")
        d[k + "pitch"] = int(num) if num else 0
        if acc: d[k + "acc"] = 1
        if sl: d[k + "slide"] = 1
    return d

VIB = mods((2, 1, 1.0), (8, 1, 0.0))   # LFO2 -> pitch (subtle vibrato)
WHEEL_CUT = (8, 13, 20)                # mod wheel opens the filter
WHEEL_VIB = (8, 1, 1.2)

# ---------------------------------------------------------------- archetypes
SINE808 = dict(o1wave=0, ftype=9, aa=0.5, ad=1600, **{"as": 0}, ar=250, penv=12, penvtime=60, glidemode=2, glide=90, trigger=1,
               velamp=25, harm=25, subboost=15, boostf=55, drift=0, keytrack=0, fenv=0, monobelow=150)
SUB = dict(o1wave=0, ftype=9, aa=3, ad=400, **{"as": 100}, ar=120, velamp=20, drift=5, fenv=0, keytrack=0, monobelow=150)
SAW = dict(o1wave=2)
MOOG = dict(o1wave=2, o2wave=3, o2level=70, o2semi=0, o2oct=1, o2fine=6, ftype=0, cutoff=380, res=25, fenv=40, fa=1, fd=320, fs=15, fr=200,
            aa=1, ad=600, **{"as": 85}, ar=120, keytrack=40, lowkeep=60, drift=20, velamp=40, fvel=30)
REESE = dict(o1wave=2, o1voices=3, o1detune=14, o1spread=40, o2wave=2, o2level=85, o2voices=3, o2detune=22, o2fine=-9, o2spread=70,
             ftype=0, cutoff=900, res=18, fenv=6, keytrack=30, lowkeep=70, aa=8, ad=800, **{"as": 95}, ar=200, sublevel=55, monobelow=180, drift=25, chorus=20)
LEAD = dict(o1wave=2, o2wave=2, o2level=60, o2fine=8, ftype=0, cutoff=2600, res=20, fenv=18, fa=2, fd=400, fs=40, keytrack=70,
            aa=4, ad=600, **{"as": 90}, ar=300, glidemode=2, glide=70, trigger=1, dmix=18, ddiv=7, dfb=35, vmix=16, vsize=55, drift=18,
            l2rate=5.5, l2shape=0)
PLUCK = dict(o1wave=2, ftype=0, cutoff=300, res=25, fenv=48, fa=0.5, fd=180, fs=0, fr=180, aa=0.5, ad=350, **{"as": 0}, ar=220, keytrack=60, velamp=50, fvel=40)

def p808(decay, punch, **kw): return H(SINE808, ad=decay, penv=punch, **kw)

HAZE = []
def add(cat, name, d): HAZE.append((cat, name, d))

# ================================================================ 808
add("808", "Trap Classic 808", p808(2200, 12, penvtime=55))
add("808", "Clean Long 808", p808(4200, 7, penvtime=45, harm=30))
add("808", "Short Knock 808", p808(650, 14, penvtime=40, dirttype=2, dirt=30, basskeep=75, harm=35))
add("808", "Detroit Knock 808", p808(900, 18, penvtime=35, dirttype=2, dirt=55, basskeep=55, dirttone=75, harm=30, comp=25))
add("808", "Rage Distorted 808", p808(2600, 12, dirttype=0, dirt=72, basskeep=40, dirttone=80, harm=20, comp=30, eqhigh=2))
add("808", "Drill Slide 808", p808(2400, 10, glidemode=1, glide=110, glideshape=1, trigger=1, dirttype=1, dirt=30, basskeep=70))
add("808", "UK Drill 808", p808(2000, 9, glidemode=1, glide=85, glideshape=0, dirttype=0, dirt=35, basskeep=65, harm=35))
add("808", "NY Drill Slide", p808(2600, 11, glidemode=1, glide=140, glideshape=1, dirttype=2, dirt=45, basskeep=60, harm=30))
add("808", "Memphis Boom", p808(3200, 8, ftype=0, cutoff=500, res=10, keytrack=0, dirttype=1, dirt=25, basskeep=80, subboost=25))
add("808", "Pluggnb Soft 808", p808(1800, 5, o1wave=0.6, penvtime=70, harm=15, eqhigh=-3))
add("808", "Tube Warm 808", p808(2400, 9, dirttype=0, dirt=40, basskeep=70, dirttone=55))
add("808", "Tape Saturated 808", p808(2200, 10, dirttype=1, dirt=55, basskeep=60, dirttone=60))
add("808", "Fuzz Fold 808", p808(2000, 12, dirttype=3, dirt=45, basskeep=55, dirttone=70, comp=20))
add("808", "Crushed 808", p808(1900, 12, dirttype=5, dirt=35, basskeep=75, dirttone=70))
add("808", "Rectified Growl 808", p808(2100, 10, dirttype=4, dirt=50, basskeep=60, dirttone=65))
add("808", "Punch In Face 808", p808(1300, 24, penvtime=30, dirttype=2, dirt=60, basskeep=50, harm=40, comp=40))
add("808", "Deep Sub 808", p808(3600, 4, harm=10, subboost=35, boostf=45, eqhigh=-4))
add("808", "Phone Speaker 808", p808(2200, 12, harm=80, subboost=10, boostf=60, dirttype=0, dirt=20, basskeep=80))
add("808", "Reese 808", p808(2600, 9, o2wave=2, o2level=35, o2voices=3, o2detune=25, o2spread=40, ftype=0, cutoff=500, res=10, fenv=10, keytrack=20, lowkeep=80, monobelow=180))
add("808", "Square 808", p808(2000, 10, o1wave=2.6, ftype=0, cutoff=420, res=15, fenv=12, keytrack=20))
add("808", "Triangle 808", p808(2200, 8, o1wave=1.0, harm=15))
add("808", "Hollow 808", p808(2000, 10, ftype=3, cutoff=520, res=10, keytrack=100, sublevel=60, subwave=0, suboct=0, subpost=1))
add("808", "Tuned Kick 808", p808(800, 30, penvtime=25, dirttype=2, dirt=25, basskeep=80, harm=30))
add("808", "Boom Bap 808", p808(1400, 6, dirttype=1, dirt=30, basskeep=70, eqhigh=-5, eqmid=-2))
add("808", "Jersey Bounce 808", p808(700, 16, penvtime=40, dirttype=2, dirt=35, basskeep=70))
add("808", "Phonk 808", p808(2500, 14, dirttype=0, dirt=80, basskeep=35, dirttone=85, harm=25, comp=35, eqmid=3, eqmidf=900))
add("808", "Wavy 808", p808(3000, 7, **mods((1, 1, 0.6)), l1sync=1, l1div=6, harm=25))
add("808", "Vintage 808", p808(1700, 6, dirttype=1, dirt=18, drift=10, eqhigh=-3))
add("808", "Overdrive Slide 808", p808(2800, 10, glidemode=1, glide=160, glideshape=1, dirttype=0, dirt=60, basskeep=55, comp=20))
add("808", "Sustain 808", p808(400, 9, **{"as": 70}, ar=500, harm=25))
add("808", "Distant 808", p808(2600, 8, dirttype=1, dirt=20, vmix=12, vsize=40, eqhigh=-6))
add("808", "Crisp Top 808", p808(2200, 11, o2wave=2, o2level=10, o2oct=3, ftype=0, cutoff=2200, keytrack=0, fenv=0, harm=20))
add("808", "Gliding Low 808", p808(3000, 5, glidemode=2, glide=200, glideshape=0))
add("808", "Bounce Short 808", p808(450, 12, harm=30))
add("808", "Velocity Punch 808", p808(2000, 0, **mods((6, 1, 20)), velamp=60))
add("808", "Click 808", p808(2100, 9, noise=30, noisetype=0, ftype=0, cutoff=3000, fenv=0, keytrack=0, fa=0.3, fd=8, **mods((3, 12, 40))))
DECAYS = [("Short", 600), ("Punch", 1000), ("Mid", 1800), ("Long", 3200), ("Endless", 6000)]
CHARS = [("Clean", {}), ("Tube", dict(dirttype=0, dirt=45, basskeep=65)), ("Tape", dict(dirttype=1, dirt=45, basskeep=65)),
         ("Clip", dict(dirttype=2, dirt=55, basskeep=55, dirttone=75)), ("Fold", dict(dirttype=3, dirt=40, basskeep=60)),
         ("Grit", dict(dirttype=4, dirt=40, basskeep=65)), ("Crush", dict(dirttype=5, dirt=30, basskeep=75))]
for dn, dv in DECAYS:
    for cn, cv in CHARS:
        add("808 Kit", f"808 {dn} {cn}", p808(dv, 10 if dv > 900 else 13, **cv))

# ================================================================ Sub
add("Sub", "Pure Sine Sub", H(SUB))
add("Sub", "Warm Triangle Sub", H(SUB, o1wave=1, ftype=0, cutoff=260, res=5, fenv=0, keytrack=60))
add("Sub", "Round Square Sub", H(SUB, o1wave=3, ftype=0, cutoff=480, res=10, keytrack=100))
add("Sub", "House Sub", H(SUB, ad=250, **{"as": 40}, ar=90, penv=3, penvtime=30, harm=20))
add("Sub", "Garage Sub", H(SUB, o1wave=0.5, ad=300, **{"as": 30}, ar=80, penv=5, penvtime=25, glidemode=2, glide=40))
add("Sub", "Dub Sub", H(SUB, ar=400, subboost=25, boostf=50, dirttype=1, dirt=20, basskeep=80, vmix=0))
add("Sub", "Saturated Sub", H(SUB, dirttype=0, dirt=50, basskeep=50, dirttone=55, harm=20))
add("Sub", "Phone Sub", H(SUB, harm=75, subboost=10))
add("Sub", "Glide Sub", H(SUB, glidemode=2, glide=120, trigger=1))
add("Sub", "Pulse Sub", H(SUB, o1wave=3, o1pw=25, ftype=0, cutoff=520, keytrack=100))
add("Sub", "Two Octave Sub", H(SUB, sublevel=70, suboct=1, subwave=0))
add("Sub", "Breathing Sub", H(SUB, **mods((1, 17, -15)), l1sync=1, l1div=10, l1shape=0))
add("Sub", "Sub + Click", H(SUB, noise=25, ftype=0, cutoff=4000, keytrack=0, fenv=0, **mods((3, 12, 35)), fa=0.3, fd=10, fs=0))
add("Sub", "Analog Drift Sub", H(SUB, o1wave=0.3, drift=60))
add("Sub", "Fifth Sub", H(SUB, o2wave=0, o2level=35, o2semi=7, o2oct=2))
add("Sub", "Octave Sub", H(SUB, o2wave=0, o2level=30, o2oct=3, o2semi=0))
add("Sub", "Pluck Sub", H(SUB, ad=280, **{"as": 0}, ar=150, penv=4, penvtime=20))
add("Sub", "Lo-Fi Sub", H(SUB, dirttype=5, dirt=22, basskeep=70, eqhigh=-6))
add("Sub", "Reggaeton Sub", H(SUB, o1wave=0.4, ad=220, **{"as": 50}, ar=100, penv=6, penvtime=20, harm=30))
add("Sub", "Afro Sub", H(SUB, o1wave=0.2, ad=350, **{"as": 30}, ar=120, penv=2, glidemode=2, glide=60, harm=20))

# ================================================================ Reese
add("Reese", "Classic Reese", H(REESE))
add("Reese", "Dark Reese", H(REESE, cutoff=480, res=12, sublevel=70))
add("Reese", "Wide Reese", H(REESE, o1spread=90, o2spread=100, width=150, chorus=35))
add("Reese", "DnB Reese", H(REESE, cutoff=1400, res=25, dirttype=0, dirt=40, basskeep=70, comp=30, **mods((1, 13, 8)), l1sync=1, l1div=12))
add("Reese", "Moving Reese", H(REESE, **mods((1, 8, 25), (2, 13, 10)), l1sync=0, l1rate=0.25, l2sync=0, l2rate=0.13))
add("Reese", "Neuro Reese", H(REESE, ftype=7, cutoff=261, res=60, keytrack=100, dirttype=2, dirt=45, basskeep=75, **mods((1, 13, 12)), l1sync=1, l1div=8))
add("Reese", "Rage Reese", H(REESE, cutoff=2200, dirttype=0, dirt=65, basskeep=55, dirttone=80, comp=35, sublevel=45))
add("Reese", "Hoover Reese", H(REESE, o1wave=2.6, o1voices=5, o1detune=30, o2voices=5, o2detune=40, cutoff=1800, glidemode=1, glide=180, penv=-5, penvtime=200))
add("Reese", "Phaser Reese", H(REESE, ftype=5, cutoff=700, res=40, keytrack=0, **mods((1, 13, 18)), l1sync=1, l1div=12, l1shape=1))
add("Reese", "Tight Reese", H(REESE, o1detune=8, o2detune=12, o2fine=-4, cutoff=650, sublevel=65))
add("Reese", "Gritty Reese", H(REESE, dirttype=4, dirt=45, basskeep=70, cutoff=1100))
add("Reese", "Detuned Squares", H(REESE, o1wave=3, o2wave=3, o1pw=40, o2pw=60, cutoff=800))
add("Reese", "Reese Swell", H(REESE, aa=350, fa=600, fd=1500, fs=60, fenv=24))
add("Reese", "Talking Reese", H(REESE, ftype=8, cutoff=400, res=50, keytrack=0, **mods((1, 13, 20)), l1sync=1, l1div=8, l1shape=1))
add("Reese", "Sub Heavy Reese", H(REESE, sublevel=100, cutoff=600, monobelow=220))
add("Reese", "Cinematic Reese", H(REESE, cutoff=520, vmix=25, vsize=80, aa=60, ar=900))
add("Reese", "Halftime Reese", H(REESE, **mods((1, 13, 25)), l1sync=1, l1div=11, l1shape=0, cutoff=500))
add("Reese", "Steel Reese", H(REESE, ftype=6, cutoff=220, res=55, keytrack=100))

# ================================================================ Synth Bass (Moog-style)
add("Synth Bass", "Fat Moog Bass", H(MOOG))
add("Synth Bass", "Space Bass", H(MOOG, cutoff=520, res=35, glidemode=2, glide=70, trigger=1, dmix=10, ddiv=7))
add("Synth Bass", "Funk Bass", H(MOOG, cutoff=300, res=45, fenv=52, fd=180, fs=5, ad=280, **{"as": 50}))
add("Synth Bass", "Rubber Bass", H(MOOG, o1wave=3, o2level=0, cutoff=220, res=55, fenv=36, fd=220, glidemode=2, glide=50))
add("Synth Bass", "Squelch Bass", H(MOOG, cutoff=250, res=72, fenv=60, fd=200, fs=0))
add("Synth Bass", "Deep Moog", H(MOOG, cutoff=200, res=15, sublevel=50, o2oct=1))
add("Synth Bass", "Bright Saw Bass", H(MOOG, cutoff=900, res=20, fenv=30, o2level=40))
add("Synth Bass", "Square Bass", H(MOOG, o1wave=3, o2wave=3, o2pw=35, cutoff=600, fenv=24))
add("Synth Bass", "Synthwave Bass", H(MOOG, o2level=0, o1voices=2, o1detune=10, cutoff=700, res=25, fenv=30, fd=200, fs=20, chorus=25))
add("Synth Bass", "80s Pluck Bass", H(MOOG, ad=300, **{"as": 0}, cutoff=350, fenv=48, fd=160, chorus=30))
add("Synth Bass", "G-Funk Bass", H(MOOG, o1wave=1.6, o2wave=0, o2oct=1, cutoff=600, res=20, glidemode=2, glide=90, trigger=1, sublevel=30))
add("Synth Bass", "Wobbly Analog", H(MOOG, drift=70, cutoff=420))
add("Synth Bass", "Velocity Bass", H(MOOG, fvel=80, velamp=70, cutoff=260, fenv=55))
add("Synth Bass", "Sync Bass", H(MOOG, sync=1, o2oct=2, o2semi=7, o2level=80, o1level=40, **mods((3, 3, 22)), cutoff=1500, fenv=12))
add("Synth Bass", "FM Bass", H(MOOG, o1wave=0, o2wave=0, o2oct=3, o2level=0, fm=55, **mods((3, 7, 40)), ftype=9, aa=0.5, ad=400, **{"as": 40}))
add("Synth Bass", "Wooden Bass", H(MOOG, o1wave=1, o2wave=0, o2level=40, ftype=0, cutoff=350, fenv=30, fd=120, ad=350, **{"as": 20}))
add("Synth Bass", "Electro Bass", H(MOOG, o1wave=3, o1pw=30, o2level=0, cutoff=450, res=50, fenv=40, fd=110, fs=0, dirttype=2, dirt=25))
add("Synth Bass", "Pop Bass", H(MOOG, cutoff=500, res=10, fenv=25, sublevel=40, comp=30))
add("Synth Bass", "Mellow Bass", H(MOOG, o1wave=1.5, o2level=20, cutoff=280, res=5, fenv=15))
add("Synth Bass", "Detuned Saw Bass", H(MOOG, o1voices=3, o1detune=18, o1spread=50, o2level=0, cutoff=800, monobelow=150))
add("Synth Bass", "Bitter Bass", H(MOOG, dirttype=0, dirt=55, basskeep=60, cutoff=700))
add("Synth Bass", "Taurus Bass", H(MOOG, o1wave=2, o2wave=2, o2oct=1, o2fine=3, cutoff=160, res=30, fenv=34, fd=600, fs=40, sublevel=40, ad=2000, **{"as": 70}))
add("Synth Bass", "Minimal Bass", H(MOOG, o1wave=0.8, o2level=0, cutoff=180, fenv=20, fd=100, fs=0, ad=200, **{"as": 0}, ar=80))
add("Synth Bass", "Glide Moog", H(MOOG, glidemode=1, glide=160, glideshape=1))
add("Synth Bass", "Low Pass Pulse", H(MOOG, o1wave=3, o1pw=20, o2level=0, **mods((1, 6, 20)), l1sync=0, l1rate=0.4, cutoff=600))
add("Synth Bass", "Nasal Bass", H(MOOG, ftype=3, cutoff=500, res=45, keytrack=80, fenv=24, sublevel=50, subpost=1))
add("Synth Bass", "Highpass Bass", H(MOOG, ftype=4, cutoff=90, res=40, keytrack=100, fenv=24))
add("Synth Bass", "Disco Bass", H(MOOG, o2level=50, o2oct=3, cutoff=700, fenv=30, fd=150, fs=10, ad=250, **{"as": 30}))

# ================================================================ Acid (sequenced, 303-style)
ACID = dict(o1wave=2, ftype=0, cutoff=260, res=78, fenv=48, fa=0.5, fd=180, fs=0, fr=100, keytrack=30, lowkeep=40, aa=0.5, ad=300, **{"as": 70}, ar=60,
            glide=60, accent=70, gate=55, patdiv=3, dirttype=0, dirt=25, basskeep=50, velamp=0, drift=10)
add("Acid", "Acid Line 01", H(ACID, **seq("0 0a 12s 0 . 0 10a 0 0s 12 0a . 3 0 12a 0")))
add("Acid", "Acid Line 02", H(ACID, **seq("0a . 0 12 0s 15 0 . 0a 7 0 12s 10 . 0 12")))
add("Acid", "Acid Line 03", H(ACID, o1wave=3, **seq("0 12a 0 12s 0 0 7a . 0 12 0a 5s 7 . 0 0")))
add("Acid", "Acid Squelch", H(ACID, res=88, fenv=60, dirt=50, **seq("0a 0s 12 . 0 0a 3s 5 0 . 12a 0 0s 10 0 .")))
add("Acid", "Acid Roller", H(ACID, cutoff=320, **seq("0 0 12a 0 0 12 0s 12 0 0 12a 0 0 12 10s 12")))
add("Acid", "Acid Minor", H(ACID, **seq("0 3 7a 3 0s 12 10 7 0a 3 . 7 12s 15 12 7")))
add("Acid", "Acid Phrygian", H(ACID, **seq("0a 1 0 . 0 13s 12 0 1a . 0 0s 7 8 0 .")))
add("Acid", "Acid Rave", H(ACID, dirt=60, dirttype=2, comp=30, dmix=15, ddiv=7, **seq("0a 0 0 12 0a 0 12s 0 0a 0 0 15 0a 12 0 10")))
add("Acid", "Acid Dub", H(ACID, res=70, dmix=30, dfb=55, ddiv=7, vmix=15, **seq("0a . . 12s 0 . 0 . 0a . 7 . 0s 12 . .")))
add("Acid", "Acid Slow Burn", H(ACID, patdiv=6, **seq("0 12s 10 0a 7 . 3s 5", 8)))
add("Acid", "Acid Triplet", H(ACID, patdiv=2, **seq("0a 12 0 0 12a 7 0s 3 12 0 0a 10", 12)))
add("Acid", "Acid Wheel", H(ACID, **mods(WHEEL_CUT, (8, 14, 10)), **seq("0 0a 0 12 0 0s 12 0 0 0a 10 0 7s 0 12 0")))
add("Acid", "Acid Square", H(ACID, o1wave=3, cutoff=300, res=70, **seq("0a 0 . 0 12s 0 0 . 0a 0 7 0 . 0 12s 0")))
add("Acid", "Acid Swing", H(ACID, swing=55, **seq("0 0a 12 0 0s 7 0 12a 0 3 0 0s 12 0a 10 0")))
add("Acid", "Acid Tense", H(ACID, res=85, cutoff=200, **seq("0 0 0a 0 0 1s 0 0 0a 0 0 0s 6 0 0a 0")))

# ================================================================ Growl / Wobble
WOB = dict(o1wave=2, o1voices=3, o1detune=15, o1spread=30, o2wave=3, o2level=60, o2oct=1, sublevel=70, ftype=0, cutoff=250, res=45, fenv=0, keytrack=30,
           l1sync=1, l1div=5, l1shape=0, l1retrig=1, lowkeep=60, dirttype=0, dirt=45, basskeep=75, aa=2, ad=800, **{"as": 100}, ar=150, monobelow=160, comp=30)
for name, div, shape, depth in [("Wobble 1/4", 8, 0, 40), ("Wobble 1/8", 5, 0, 40), ("Wobble 1/8T", 3, 0, 40), ("Wobble 1/16", 2, 0, 35),
                                ("Wobble Saw 1/8", 5, 3, 42), ("Wobble Square 1/8", 5, 4, 38), ("Wobble Half Bar", 10, 0, 45), ("Wobble Dotted", 7, 1, 40)]:
    add("Growl", name, H(WOB, l1div=div, l1shape=shape, **mods((1, 13, depth), WHEEL_CUT)))
add("Growl", "Talking Growl", H(WOB, ftype=8, cutoff=500, res=60, **mods((1, 13, 30), (2, 14, 10)), l1div=5, l2sync=1, l2div=8))
add("Growl", "Yoi Growl", H(WOB, ftype=8, cutoff=300, res=70, **mods((4, 13, 35)), ma=5, md=250, ms=0, l1retrig=0))
add("Growl", "FM Growl", H(WOB, o1wave=0, o2wave=0, o2level=0, o2oct=2, fm=40, **mods((1, 7, 40), (1, 13, 20)), ftype=0, cutoff=900))
add("Growl", "Comb Growl", H(WOB, ftype=6, cutoff=261, res=70, keytrack=100, **mods((1, 13, 25))))
add("Growl", "Rectified Growl", H(WOB, dirttype=4, dirt=70, **mods((1, 13, 35))))
add("Growl", "Folded Growl", H(WOB, dirttype=3, dirt=60, **mods((1, 16, 30), (1, 13, 25))))
add("Growl", "Riddim Growl", H(WOB, l1div=6, l1shape=4, ftype=1, res=60, **mods((1, 13, 40)), dirttype=2, dirt=60))
add("Growl", "Pitch Growl", H(WOB, **mods((1, 1, 2.5), (1, 13, 30))))
add("Growl", "Wheel Growl", H(WOB, **mods((8, 13, 45), (8, 16, 40))))
add("Growl", "Aftertouch Growl", H(WOB, **mods((9, 13, 45), (9, 19, 30)), l1sync=0, l1rate=3))
add("Growl", "Vowel Morph", H(WOB, ftype=8, cutoff=600, res=55, **mods((1, 13, 35)), l1div=11, l1shape=1))
add("Growl", "S&H Growl", H(WOB, l1shape=5, l1div=2, **mods((1, 13, 30))))
add("Growl", "Smooth Random Growl", H(WOB, l1shape=6, l1sync=0, l1rate=6, **mods((1, 13, 30))))

# ================================================================ Distorted / Rage
RAGE = dict(o1wave=2, o1voices=5, o1detune=22, o1spread=60, o2wave=2, o2level=50, o2voices=3, o2detune=30, o2oct=3, sublevel=80,
            ftype=0, cutoff=2400, res=15, fenv=12, keytrack=20, lowkeep=60, dirttype=0, dirt=70, basskeep=55, dirttone=80, comp=40,
            aa=1, ad=900, **{"as": 85}, ar=200, monobelow=180, eqhigh=2, glidemode=2, glide=50)
add("Rage", "Rage Bass", H(RAGE))
add("Rage", "Opium Bass", H(RAGE, cutoff=1400, dirttype=2, dirt=80, eqmid=3, eqmidf=1200))
add("Rage", "Rage Lead Bass", H(RAGE, o1oct=3, sublevel=50, glidemode=1, glide=70, dmix=15, ddiv=6, vmix=10))
add("Rage", "Distorted Saw Wall", H(RAGE, o1voices=7, o1detune=30, width=160, chorus=20))
add("Rage", "Clipped Square", H(RAGE, o1wave=3, o2wave=3, dirttype=2, dirt=75))
add("Rage", "Fuzz Bass", H(RAGE, dirttype=3, dirt=65, cutoff=1800))
add("Rage", "Bitcrush Bass", H(RAGE, dirttype=5, dirt=45, basskeep=75, cutoff=3000))
add("Rage", "Industrial Bass", H(RAGE, dirttype=4, dirt=75, noise=10, noisetype=1, cutoff=2000, res=30))
add("Rage", "Rage Stab", H(RAGE, ad=300, **{"as": 0}, ar=150, fd=200, fenv=24))
add("Rage", "Overdriven 808 Saw", H(RAGE, o1wave=1.2, o1voices=1, o2level=0, penv=12, penvtime=50, ad=2200, **{"as": 0}, sublevel=60, dirt=80))
add("Rage", "Screamer", H(RAGE, o1oct=3, res=55, cutoff=1500, fenv=30, **mods((8, 13, 30), WHEEL_VIB)))
add("Rage", "Dirty Pulse", H(RAGE, o1wave=3, o1pw=20, **mods((1, 6, 25)), l1sync=1, l1div=8))
add("Rage", "Hyperpop Bass", H(RAGE, dirttype=2, dirt=85, eqhigh=4, eqhighf=5000, cutoff=4000, glidemode=1, glide=40))
add("Rage", "Mosh Bass", H(RAGE, dirttype=0, dirt=90, basskeep=40, comp=55))
add("Rage", "Phonk Bass", H(RAGE, o1wave=1.5, o1voices=1, dirttype=0, dirt=85, cutoff=1200, eqmid=4, eqmidf=800))

# ================================================================ Leads (mono, glide)
add("Lead", "Space Saw Lead", H(LEAD, **mods(WHEEL_VIB, (2, 1, 0.5)), l2sync=0))
add("Lead", "Portamento Lead", H(LEAD, glidemode=1, glide=140, glideshape=0))
add("Lead", "Sine Whistle", H(LEAD, o1wave=0, o2level=0, ftype=9, **mods((2, 1, 1.2), WHEEL_VIB), dmix=22, vmix=20, glidemode=1, glide=110))
add("Lead", "G-Funk Whistle", H(LEAD, o1wave=0.8, o2level=0, ftype=0, cutoff=4000, fenv=0, glidemode=1, glide=150, **mods((2, 1, 1.8)), l2rate=5.8, dmix=20, ddiv=7))
add("Lead", "Square Lead", H(LEAD, o1wave=3, o2wave=3, o2pw=30, cutoff=3000))
add("Lead", "Sync Lead", H(LEAD, sync=1, o2oct=3, o2level=90, o1level=30, **mods((4, 3, 20), WHEEL_VIB), md=600, ms=20, cutoff=5000))
add("Lead", "Detuned Saw Lead", H(LEAD, o1voices=5, o1detune=25, o1spread=70, o2voices=3, o2detune=20, o2spread=80, cutoff=4000, chorus=20))
add("Lead", "Mono Synth Lead", H(LEAD, cutoff=1600, res=35, fenv=30))
add("Lead", "Screaming Lead", H(LEAD, res=60, cutoff=2000, dirttype=0, dirt=50, basskeep=0, **mods(WHEEL_VIB, (8, 13, 15))))
add("Lead", "Soft Triangle Lead", H(LEAD, o1wave=1, o2wave=1, cutoff=2000, res=5))
add("Lead", "Chiptune Lead", H(LEAD, o1wave=3, o1pw=25, o2level=0, ftype=9, drift=0, glidemode=0, dmix=15, ddiv=6, **mods((1, 6, 20)), l1sync=1, l1div=2))
add("Lead", "Pulse Width Lead", H(LEAD, o1wave=3, o2wave=3, **mods((1, 6, 35)), l1sync=0, l1rate=0.8))
add("Lead", "Vocal Lead", H(LEAD, ftype=8, cutoff=1200, res=50, **mods((8, 13, 25), (2, 1, 0.8))))
add("Lead", "Cosmic Lead", H(LEAD, dmix=35, dfb=55, vmix=35, vsize=85, glidemode=1, glide=200))
add("Lead", "Fifth Lead", H(LEAD, o2semi=7, o2level=60, o2fine=0))
add("Lead", "Octave Lead", H(LEAD, o2oct=3, o2level=55, o2fine=4))
add("Lead", "FM Bell Lead", H(LEAD, o1wave=0, o2wave=0, o2oct=4, o2level=0, fm=45, **mods((4, 7, -30)), md=500, ms=0, ftype=9))
add("Lead", "Theremin", H(LEAD, o1wave=0.4, o2level=0, ftype=9, glidemode=1, glide=250, **mods((2, 1, 1.5)), l2rate=6.2, vmix=25))
add("Lead", "Talkbox Lead", H(LEAD, ftype=8, cutoff=800, res=60, **mods((4, 13, 25)), ma=10, md=300, ms=30))
add("Lead", "Funk Lead", H(LEAD, o1wave=2.5, cutoff=1200, res=40, fenv=36, fd=200, fs=20))
add("Lead", "Acid Lead", H(LEAD, ftype=0, cutoff=700, res=75, fenv=40, fd=250, fs=10, dirt=30))
add("Lead", "Rave Lead", H(LEAD, o1voices=7, o1detune=35, o2level=0, cutoff=6000, res=10, chorus=30, vmix=20))
add("Lead", "Siren Lead", H(LEAD, **mods((1, 1, 15)), l1sync=1, l1div=11, l1shape=1))
add("Lead", "Lazy Glide Lead", H(LEAD, glidemode=1, glide=300, glideshape=1))
add("Lead", "Muted Lead", H(LEAD, cutoff=900, res=15, fenv=10, eqhigh=-4))

# ================================================================ Pluck / Keys
add("Pluck", "Saw Pluck", H(PLUCK))
add("Pluck", "Square Pluck", H(PLUCK, o1wave=3, cutoff=400))
add("Pluck", "Rubber Pluck", H(PLUCK, res=55, fenv=40, fd=120))
add("Pluck", "Bell Pluck", H(PLUCK, o1wave=0, o2wave=0, o2oct=4, o2level=0, fm=40, **mods((4, 7, -40)), md=300, ms=0, ftype=9, ad=700))
add("Pluck", "Wide Pluck", H(PLUCK, o1voices=5, o1detune=20, o1spread=90, dmix=20, ddiv=7, vmix=20))
add("Pluck", "Marimba Pluck", H(PLUCK, o1wave=0.6, ftype=3, cutoff=800, res=30, fenv=12, keytrack=100, ad=200, ar=150))
add("Pluck", "Kalimba", H(PLUCK, o1wave=0, o2wave=0, o2oct=3, o2semi=7, o2level=25, ftype=9, ad=600, ar=400, vmix=20))
add("Pluck", "Synth Stab", H(PLUCK, o2wave=2, o2level=70, o2semi=7, cutoff=600, fenv=40, ad=250))
add("Pluck", "Clav Pluck", H(PLUCK, o1wave=3, o1pw=20, ftype=4, cutoff=400, res=25, keytrack=100, fenv=24, fd=100))
add("Pluck", "Chirp", H(PLUCK, penv=12, penvtime=25, cutoff=800, ad=180))
add("Pluck", "Lo-Fi Pluck", H(PLUCK, dirttype=5, dirt=30, basskeep=0, cutoff=500, vmix=15))
add("Pluck", "Dub Pluck", H(PLUCK, dmix=35, dfb=60, ddiv=7, vmix=20))
add("Pluck", "Glass Pluck", H(PLUCK, o1wave=1, ftype=6, cutoff=600, res=60, keytrack=100, fenv=0, ad=700))
add("Pluck", "Muted Pluck", H(PLUCK, cutoff=200, fenv=30, fd=90, ad=180))

# ================================================================ Sequences
SEQB = dict(MOOG, patdiv=3, gate=50, ad=300, **{"as": 60}, ar=80, cutoff=420, res=30, fenv=30, fd=150, fs=10, accent=50)
SEQS = [
    ("House Offbeat", dict(**seq(". . 0a . . . 0a . . . 0a . . . 0a .")), dict(gate=80)),
    ("Trance Roller", dict(**seq(". 0 0 0 . 0 0 0 . 0 0 0 . 0 0 0")), dict(cutoff=600, fd=110)),
    ("Synthwave Gallop", dict(**seq("0a 0 0 0a 0 0 0a 0 0 0a 0 0 0a 0 12 0")), dict(chorus=30, o2level=0)),
    ("Techno Rolling", dict(**seq("0 0 0a 0 0 0 0a 0 0 0 0a 0 0 0 0a 0")), dict(cutoff=300, res=45, dirt=30)),
    ("Berlin School", dict(**seq("0 12 7 3 0 12 7 5 0 12 7 3 0 15 12 10")), dict(dmix=25, ddiv=7, vmix=15)),
    ("Funk Sequence", dict(**seq("0a . 0 12s 0 . 10 0 0a . 7 . 0 12 . 10")), dict(swing=40)),
    ("Electro Line", dict(**seq("0a . 0 . . 0 . 0a . 0 . . 12 . 10 .")), dict(o1wave=3, dirttype=2, dirt=30)),
    ("Disco Octaves", dict(**seq("0 12 0 12 0 12 0 12 0 12 0 12 0 12 0 12")), dict(patdiv=6, gate=40)),
    ("Minimal Pulse", dict(**seq("0a . . 0 . . 0 . 0a . . 0 . 0 . .")), dict(cutoff=250)),
    ("Dark Techno", dict(**seq("0 0a 0 1 0 0a 0 0 0 0a 0 1 0 0 3s 0")), dict(ftype=1, res=55, dirt=40)),
    ("Italo Bass", dict(**seq("0 12 0 12 3 15 3 15 5 17 5 17 3 15 3 15")), dict(chorus=25, patdiv=3)),
    ("UK Garage Skip", dict(**seq("0a . . 0 . 0 0a . . 0 . . 0a . 12 .")), dict(swing=55, glide=40)),
    ("Bassline House", dict(**seq("0a . 12s 0 . 0 12a . 0 . 12s 10 . 0 7 .")), dict(dirttype=0, dirt=35, cutoff=500, res=40)),
    ("Arp Minor 16", dict(**seq("0 3 7 12 7 3 0 3 7 12 15 12 7 3 0 -5")), dict(cutoff=700, dmix=20, ddiv=7)),
    ("Hypnotic", dict(**seq("0 0 12 0 0 7 0 0 12 0 0 7 0 3 0 0")), dict(**mods((1, 13, 10)), l1sync=1, l1div=12)),
    ("Industrial Seq", dict(**seq("0a 0 0 0a 0 0 0a 0 0a 0 0 0a 0 0 0a 0")), dict(dirttype=4, dirt=60, noise=15)),
    ("Drum and Bass Roll", dict(**seq("0a . . 0 . . 0 0 0a . . 0 . 0 0 .")), dict(REESE, patmode=3, patdiv=3, gate=90)),
    ("Afro Bounce", dict(**seq("0a . . 0 . 0 . . 0a . 7 . . 0 . .")), dict(SUB, patmode=3, patdiv=3, gate=70)),
    ("Reggaeton Dembow", dict(**seq("0a . . 0 . . 0a . 0a . . 0 . . 0a .")), dict(SUB, patmode=3, patdiv=3, gate=60, penv=5, penvtime=20)),
    ("Jersey Club Bass", dict(**seq("0a . 0 . 0a . . 0 . 0 0a . 0 . . .")), dict(p808(500, 12), patmode=3, patdiv=3, gate=90)),
    ("Trap Roll 808", dict(**seq("0a . . . . . 0 0 0a . . . 0 . 12s 0")), dict(p808(1600, 10), patmode=3, patdiv=3, gate=95)),
    ("Drill 808 Pattern", dict(**seq("0a . . 0 . . 3s 0 . . 0a . . -2s 0 .")), dict(p808(2200, 9), patmode=3, patdiv=3, gate=98, glide=90)),
    ("Rage 808 Bounce", dict(**seq("0a . . 0 . 0 0a . . . 0 . 0a . 12 .")), dict(p808(1500, 12, dirttype=0, dirt=65, basskeep=50), patmode=3, patdiv=3, gate=95)),
    ("Pluck Sequence", dict(**seq("0 7 12 7 0 7 12 15 0 7 12 7 0 5 7 10")), dict(PLUCK, patmode=3, patdiv=3, gate=40, dmix=25, ddiv=7)),
    ("Acid House", dict(**seq("0 0a 12s 0 0 0a 10 0 0 0a 12s 15 0 0a 7 0")), dict(ACID, patmode=3)),
    ("Slow Groove", dict(**seq("0a . 7 10 12s 10 7 .", 8)), dict(patdiv=6, gate=70, glide=80)),
    ("Triplet Pulse", dict(**seq("0a 0 0 7a 7 7 10a 10 10 12a 12 12", 12)), dict(patdiv=2)),
    ("Ghost Notes", dict(**seq("0a 0 . 0 0a . 0 0 0a . 0 0 . 0 0a 0")), dict(accent=90, velamp=0, gate=35)),
]
for name, steps, extra in SEQS:
    # an entry that carries a whole patch (it sets o1wave) starts from that patch instead of the sequence bass
    add("Sequence", name, H(steps, extra) if "o1wave" in extra else H(SEQB, steps, extra))

# ================================================================ Arps
ARPB = dict(LEAD, patmode=2, patdiv=3, gate=55, arpdir=0, arpoct=1, glidemode=0, aa=1, ad=250, **{"as": 30}, ar=120, cutoff=1800, fenv=24, fd=150, fs=10, dmix=20, ddiv=7)
for name, kw in [
    ("Up 1/16", dict()), ("Down 1/16", dict(arpdir=1)), ("Up-Down 1/16", dict(arpdir=2)), ("Random 1/16", dict(arpdir=4)),
    ("Up 1/8", dict(patdiv=6, gate=70)), ("Up Triplets", dict(patdiv=2)), ("Three Octave Run", dict(arpoct=2, patdiv=3)),
    ("Four Octave Cascade", dict(arpoct=3, arpdir=1, patdiv=1)), ("As Played", dict(arpdir=5)), ("Swing Arp", dict(swing=55)),
    ("Pluck Arp", dict(PLUCK, patmode=2, patdiv=3, gate=40, arpoct=1, dmix=25, ddiv=7)), ("Bell Arp", dict(o1wave=0, o2wave=0, o2oct=4, o2level=0, fm=40, ftype=9, **mods((4, 7, -40)), md=300, ms=0)),
    ("Square Arp", dict(o1wave=3, o2wave=3, cutoff=2200)), ("Detuned Arp", dict(o1voices=5, o1detune=25, o1spread=80, chorus=20)),
    ("Bass Arp", dict(MOOG, patmode=2, patdiv=3, gate=60, arpoct=1)), ("Acid Arp", dict(ACID, patmode=2, arpoct=1, gate=60)),
    ("Chip Arp", dict(o1wave=3, o1pw=25, o2level=0, ftype=9, patdiv=1, gate=80, arpoct=1)), ("Dreamy Arp", dict(o1wave=1, cutoff=1500, vmix=35, vsize=80, dmix=35, dfb=50)),
    ("Trance Gate Arp", dict(o1voices=7, o1detune=30, o2level=0, patdiv=3, gate=35, cutoff=3500)), ("Repeat 1/16", dict(patmode=1, patdiv=3, gate=50)),
    ("Repeat 1/32 Roll", dict(patmode=1, patdiv=1, gate=60)), ("Repeat Triplet", dict(patmode=1, patdiv=2, gate=55)),
    ("808 Repeat Roll", dict(p808(400, 10), patmode=1, patdiv=3, gate=80)), ("Hat Style Repeat", dict(PLUCK, patmode=1, patdiv=1, gate=30)),
]:
    add("Arp", name, H(dict(patmode=2, patdiv=3, gate=55), kw) if "ad" in kw and "o1wave" in kw else H(ARPB, kw))

# ================================================================ FX
add("FX", "Sub Drop", H(SUB, penv=36, penvtime=1800, ad=2500, **{"as": 0}, ar=500, harm=30))
add("FX", "Riser", H(SAW, o1voices=7, o1detune=40, o1spread=90, ftype=0, cutoff=200, res=40, fenv=72, fa=4000, fd=100, fs=100, penv=-12, penvtime=4000,
                     aa=3000, **{"as": 100}, ar=1500, noise=30, noisetype=0, vmix=40, vsize=85, width=160))
add("FX", "Laser Zap", H(SAW, o1wave=3, penv=36, penvtime=180, ftype=9, ad=250, **{"as": 0}, dmix=30, ddiv=6, dfb=40))
add("FX", "Noise Sweep", H(o1level=0, noise=80, noisetype=0, ftype=3, cutoff=300, res=60, fenv=60, fa=2000, fd=2000, fs=0, keytrack=0, aa=1500, ad=2000, **{"as": 0}, ar=1000, vmix=30))
add("FX", "Siren", H(SAW, o1wave=1.5, **mods((1, 1, 25)), l1sync=1, l1div=10, l1shape=1, ftype=0, cutoff=3000, dmix=20, vmix=20))
add("FX", "Wobble Drop", H(WOB, penv=-24, penvtime=1500, **mods((1, 13, 40)), l1div=5))
add("FX", "Impact Boom", H(SINE808, ad=3500, penv=48, penvtime=300, noise=40, noisetype=2, dirttype=1, dirt=40, vmix=35, vsize=90))
add("FX", "Glitch Bleeps", H(SAW, o1wave=3, ftype=9, patmode=1, patdiv=1, gate=30, **mods((1, 1, 12)), l1shape=5, l1sync=1, l1div=0, dmix=20))
add("FX", "Space Drone", H(REESE, aa=1500, ar=3000, cutoff=600, **mods((1, 13, 15), (2, 8, 20)), l1sync=0, l1rate=0.1, l2sync=0, l2rate=0.07, vmix=50, vsize=95, dmix=25, dfb=55))
add("FX", "Tape Stop Bass", H(MOOG, glidemode=1, glide=900, glideshape=0, cutoff=500))
add("FX", "Pitch Dive", H(LEAD, penv=-24, penvtime=600, ad=800, **{"as": 0}, glidemode=0))
add("FX", "Ring Wobble", H(SAW, o2wave=0, o2oct=4, o2level=0, fm=80, **mods((1, 7, 30)), l1sync=1, l1div=5, ftype=0, cutoff=2000))


# ================================================================ kits: systematic families
# Reese kit: filter flavour x motion
RF = [("Ladder", dict(ftype=0, cutoff=900, res=18)), ("Low 12", dict(ftype=1, cutoff=1100, res=30)), ("Notch", dict(ftype=5, cutoff=800, res=35, keytrack=0)), ("Comb", dict(ftype=6, cutoff=261, res=55, keytrack=100))]
RM = [("Still", {}), ("Drift", dict(**mods((1, 8, 25)), l1sync=0, l1rate=0.2)), ("Pulse 1/4", dict(**mods((1, 13, 18)), l1sync=1, l1div=8)), ("Sweep Bar", dict(**mods((1, 13, 25)), l1sync=1, l1div=11, l1shape=1))]
for fn, fv in RF:
    for mn, mv in RM:
        add("Reese Kit", f"Reese {fn} {mn}", H(REESE, fv, mv))
# Synth bass kit: wave x envelope
SW = [("Saw", dict(o1wave=2, o2wave=2)), ("Square", dict(o1wave=3, o2wave=3)), ("Pulse", dict(o1wave=3, o1pw=22, o2wave=3, o2pw=35)), ("Tri", dict(o1wave=1, o2wave=0))]
SE = [("Pluck", dict(ad=260, **{"as": 0}, ar=120, fd=140, fs=0, fenv=48)), ("Punch", dict(ad=500, **{"as": 55}, fd=180, fs=10, fenv=40)),
      ("Sustain", dict(ad=800, **{"as": 95}, fd=600, fs=40, fenv=18)), ("Swell", dict(aa=180, ad=800, **{"as": 90}, fa=300, fd=900, fs=50, fenv=24))]
for wn, wv in SW:
    for en, ev in SE:
        add("Synth Bass Kit", f"{wn} Bass {en}", H(MOOG, wv, ev))
# Sub kit
for wn, wv in [("Sine", dict(o1wave=0)), ("Soft", dict(o1wave=0.5)), ("Tri", dict(o1wave=1, ftype=0, cutoff=600, keytrack=100)), ("Round Square", dict(o1wave=3, ftype=0, cutoff=480, keytrack=100)), ("Round Saw", dict(o1wave=2, ftype=0, cutoff=420, keytrack=100))]:
    for sn, sv in [("Hold", {}), ("Pluck", dict(ad=300, **{"as": 0}, ar=140)), ("Slide", dict(glidemode=2, glide=110, trigger=1))]:
        add("Sub Kit", f"Sub {wn} {sn}", H(SUB, wv, sv))
# Lead kit
for wn, wv in [("Saw", dict(o1wave=2, o2wave=2)), ("Square", dict(o1wave=3, o2wave=3)), ("Sine", dict(o1wave=0, o2wave=0, o2level=0, ftype=9)), ("Sync", dict(sync=1, o2oct=3, o2level=90, o1level=30, **mods((4, 3, 18)), md=500, ms=25))]:
    for sn, sv in [("Dry", dict(dmix=0, vmix=0)), ("Glide", dict(glidemode=1, glide=150)), ("Space", dict(dmix=35, dfb=50, vmix=35, vsize=85)), ("Vibrato", dict(**mods((2, 1, 1.6)), l2rate=5.8))]:
        if wn == "Sync" and sn == "Vibrato": sv = dict(sv); sv.update(mods((4, 3, 18), (2, 1, 1.6)))
        add("Lead Kit", f"{wn} Lead {sn}", H(LEAD, wv, sv))
# 808 glide kit
for gn, gv in [("Legato Slide", dict(glidemode=2, glide=100, glideshape=0)), ("Always Slide", dict(glidemode=1, glide=90, glideshape=1))]:
    for dn, dv in [("Clean", {}), ("Tube", dict(dirttype=0, dirt=45, basskeep=65)), ("Clip", dict(dirttype=2, dirt=55, basskeep=55)), ("Tape", dict(dirttype=1, dirt=45, basskeep=65))]:
        add("808 Kit", f"808 {gn} {dn}", p808(2600, 10, **gv, **dv))
# more acid
for i, pat in enumerate([
    "0 12 0 0a 10s 12 0 7 0 12a 0 3s 5 0 12 .", "0a 0 3 0 5s 7 0 . 0a 12 10 7 5s 3 0 .", "0 0a . 0 12s 0 . 0 0 0a . 15 12s 10 . 0",
    "0a 12 0 12 0a 12 0s 13 0a 12 0 12 0a 12 10s 12", "0 . 0a 0 . 0 7s 5 0 . 0a 0 . 0 3s 0", "12a 0 0 12 0 0s 10 0 12a 0 0 7 0 0s 5 0",
    "0 0 0 0a 0 0 0 0a 0 0 0 0a 0 3s 5 7a", "0a . 7 . 5s 3 0 . 0a . 10 . 12s 15 12 .", "0 12a 0s 12 0 10a 0s 10 0 7a 0s 7 0 5a 0s 5",
    "0a 1 0 1 0a 1 0 13s 0a 1 0 1 0a 1 12s 0"]):
    add("Acid", f"Acid Line {i + 4:02d}", H(ACID, **seq(pat), cutoff=220 + 30 * (i % 4), res=70 + 2 * (i % 6), dirt=20 + 5 * (i % 5)))
# more sequences
MORE_SEQ = [
    ("Techno Offbeat 16th", ". 0 0a 0 . 0 0a 0 . 0 0a 0 . 0 0a 0", dict(cutoff=350, res=40), SEQB),
    ("Melodic Techno", "0 0 12 0 3 0 12 0 7 0 12 0 5 0 3 0", dict(cutoff=500, dmix=20, ddiv=7, vmix=15), SEQB),
    ("Progressive Roll", ". 0 0 0 . 0 0 0 . 0 0 0 . 0 0 12", dict(cutoff=650, fd=120, chorus=15), SEQB),
    ("Electro Funk", "0a . 0 12 . 0 . 10 0a . 7 . 0 12 0 .", dict(o1wave=3, o1pw=30, swing=30), SEQB),
    ("Miami Bass", "0a . . 0 . . 0 . 0a . . 0 . 0 . .", dict(penv=10, penvtime=40), SINE808),
    ("Phonk Cowbell Bass", "0a . 0 . 7 . 0 . 0a . 0 . 10 . 7 .", dict(dirttype=0, dirt=70, basskeep=50), SINE808),
    ("Detroit 808 Seq", "0a . 0 0 . 0 . 0a . 0 0 . 0 0a . .", dict(ad=500, dirttype=2, dirt=50, basskeep=60), SINE808),
    ("Trap Triplet 808", "0a . . 0 . . 0 . . 12s 0 .", dict(patdiv=2), SINE808),
    ("Garage Wobble", "0a . 0 . . 0 . 0a . . 0 . 0 . 10 .", dict(**mods((1, 13, 25)), l1sync=1, l1div=5, swing=50), WOB),
    ("Riddim Seq", "0a . . 0 . . 0a . . . 0 . 0a . . .", dict(l1div=6, l1shape=4, **mods((1, 13, 40))), WOB),
    ("Halftime Growl", "0a . . . . . . . 0a . . . 0 . . .", dict(**mods((1, 13, 30)), l1div=8), WOB),
    ("Reese Roller", "0a . . 0 . . 0 . 0a . . 0 . 0 . .", dict(gate=95), REESE),
    ("Neuro Stabs", "0a . 0 . . 0a . . 0 . 0a . . 0 . 0", dict(ftype=7, cutoff=180, res=60, keytrack=100, dirt=50, dirttype=2), REESE),
    ("Synthwave Octaves", "0 12 0 12 0 12 0 12 -2 10 -2 10 3 15 3 15", dict(chorus=30, cutoff=900), SEQB),
    ("Dark Synthwave", "0a 0 0 0 0a 0 0 0 1a 1 1 1 -2a -2 -2 -2", dict(cutoff=600, res=35, dirt=20), SEQB),
    ("80s Pop Pulse", "0 0 0 0 0 0 0 0 5 5 5 5 7 7 7 7", dict(patdiv=6, gate=70, cutoff=1200, chorus=30), SEQB),
    ("Moroder Pulse", "0 12 0 12 0 12 0 12 0 12 0 12 0 12 0 12", dict(patdiv=3, gate=45, cutoff=800, dmix=15, ddiv=7), SEQB),
    ("Afrobeats Log", "0a . . 0 . . 7 . 0a . . 0 . 5 . .", dict(penv=4, penvtime=30, ad=250, **{"as": 0}), SUB),
    ("Amapiano Log Drum", "0a . . . 0 . . 0 . . 0a . . . 7 .", dict(o1wave=0.3, penv=7, penvtime=25, ad=220, **{"as": 0}, harm=40, ftype=3, cutoff=520, res=20, keytrack=100, subpost=1, sublevel=40), SUB),
    ("Dancehall Bounce", "0a . . 0 . . 0a . . 0 . . 0a . 7 .", dict(o1wave=0.5, ad=200, **{"as": 20}), SUB),
    ("Funk Octave Slap", "0a . 12 . 0 0 12s 10 0a . 12 . 7 . 10 12", dict(cutoff=380, res=40, fd=120, swing=35), SEQB),
    ("Boom Bap Walk", "0a . . 0 3 . 5 . 7a . . 5 3 . 0 .", dict(patdiv=3, gate=80, o1wave=1.4, cutoff=300, eqhigh=-4, swing=45), SEQB),
    ("Psytrance Roll", ". 0a 0 0 . 0a 0 0 . 0a 0 0 . 0a 0 0", dict(cutoff=900, res=30, fd=90, gate=40), SEQB),
    ("Hard Techno Rumble", "0a 0 0 0 0a 0 0 0 0a 0 0 0 0a 0 0 0", dict(dirttype=0, dirt=60, cutoff=200, res=50, vmix=20), SEQB),
    ("Glitch Sequence", "0 . 12a . 0s 7 . 19 0 . 12 0a . 24 . 7", dict(o1wave=3, dirttype=5, dirt=35), SEQB),
    ("Lo-Fi Walking", "0 . 3 . 5 . 7 . 10 . 7 . 5 . 3 .", dict(o1wave=1.5, cutoff=350, eqhigh=-5, dirttype=1, dirt=25, swing=50), SEQB),
    ("Sync Arpeggio Seq", "0 7 12 19 12 7 0 7 12 19 24 19 12 7 0 -5", dict(sync=1, o2oct=3, o2level=80, o1level=30, **mods((4, 3, 15)), md=200, ms=0), SEQB),
    ("Rave Stab Seq", "0a . . 0 . . 0a . . 0 . . 0a . 5 .", dict(o1voices=7, o1detune=35, o1spread=80, cutoff=2500, fenv=20, chorus=25), SEQB),
    ("Jungle Sub Seq", "0a . . . . 0 . . 0a . 5s 0 . . . .", dict(penv=3), SUB),
    ("Pluggnb Bounce Seq", "0a . . 0 . 0 12 . 0a . . 7 . 0 . .", dict(o1wave=0.6, ad=900, **{"as": 0}), SINE808),
]
for name, pat, kw, base in MORE_SEQ:
    add("Sequence", name, H(base, dict(patmode=3, patdiv=3, gate=60), seq(pat), kw))
# more arps
for name, kw in [("Octave Bounce Arp", dict(arpdir=2, arpoct=1, patdiv=3)), ("Slow Bloom Arp", dict(patdiv=6, gate=90, aa=40, vmix=35, vsize=80)),
                 ("Glide Arp", dict(glidemode=1, glide=40)), ("Gated Supersaw Arp", dict(o1voices=7, o1detune=30, o1spread=90, gate=30, patdiv=3)),
                 ("Dotted Arp", dict(patdiv=4, gate=60, dmix=30, ddiv=7)), ("Rolling 1/32 Arp", dict(patdiv=1, gate=50, arpoct=2)),
                 ("Wobble Arp", dict(**mods((1, 13, 20)), l1sync=1, l1div=8)), ("Down Cascade", dict(arpdir=1, arpoct=3, patdiv=3)),
                 ("Random Bleeps", dict(arpdir=4, arpoct=2, o1wave=3, o1pw=20, ftype=9, patdiv=3, gate=30)), ("Trance Pluck Arp", dict(PLUCK, patmode=2, patdiv=3, gate=45, arpoct=2, dmix=30, ddiv=7, vmix=25))]:
    add("Arp", name, H(dict(patmode=2, patdiv=3, gate=55), kw) if "ad" in kw and "o1wave" in kw else H(ARPB, kw))
# more growl
for name, kw in [("Growl Riser", dict(**mods((1, 13, 35), (4, 19, 60)), ma=3000, md=100, ms=100)), ("Wet Growl", dict(**mods((1, 13, 35)), vmix=25, dmix=20)),
                 ("Sub Wobble", dict(o1level=40, sublevel=100, **mods((1, 13, 30)))), ("Formant Wobble 1/8T", dict(ftype=8, res=55, **mods((1, 13, 35)), l1div=3)),
                 ("Crushed Wobble", dict(dirttype=5, dirt=40, **mods((1, 13, 35)))), ("Off-Beat Wobble", dict(l1shape=3, l1div=5, **mods((1, 13, 40)))),
                 ("Wide Growl", dict(o1voices=7, o1spread=100, width=170, **mods((1, 13, 35)))), ("Envelope Growl", dict(**mods((4, 13, 40), (4, 16, 30)), ma=2, md=180, ms=0))]:
    add("Growl", name, H(WOB, kw))

# volumes are measured and filled in by tools/haze_level.py (same loudness across every preset)
HAZE_VOLUME = {}
try:
    from haze_volume import HAZE_VOLUME
except ImportError:
    pass
for _c, _n, _v in HAZE:
    _v["volume"] = round(_v.get("volume", 0.0) + HAZE_VOLUME.get(_c + "/" + _n, 0.0), 1)
