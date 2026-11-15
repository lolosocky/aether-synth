function AETHER_PRESETS(D, TRIMS) {
'use strict';
TRIMS = TRIMS || {};
const CATS = ['Pads', 'Atmospheres', 'Keys', 'Lo-Fi & Dusty', 'Bells & Mallets', 'Plucks', 'Leads', 'Synth Bass', '808 & Sub', 'Strings',
  'Brass & Winds', 'Choir & Vox', 'Organs', 'Guitars & World', 'Arps & Sequences', 'Pulses & Gates', 'Drones', 'Textures & Noise',
  'Granular', 'Sweeps & FX', 'Trap & Drill', 'Soul & Boom Bap', 'Cinematic', 'Modern Hip Hop'];

/* ---------- building blocks ---------- */
const lay = (mode, o) => Object.assign({ on: 1, mode: mode }, o || {});
const saw = o => lay('analog', Object.assign({ shp: 0.667 }, o));
const sq = o => lay('analog', Object.assign({ shp: 1, pw: 0.5 }, o));
const tri = o => lay('analog', Object.assign({ shp: 0.333 }, o));
const sine = o => lay('analog', Object.assign({ shp: 0, f1t: 'off' }, o));
const wt = (name, pos, o) => lay('wt', Object.assign({ wt: name, pos: pos }, o));
const modal = (m, o) => lay('modal', Object.assign({ mdl: m, f1t: 'off' }, o));
const pluck = o => lay('pluck', Object.assign({ f1t: 'off' }, o));
const vox = o => lay('formant', Object.assign({ f1t: 'off' }, o));
const noise = (t, o) => lay('noise', Object.assign({ ntyp: t, f1t: 'off' }, o));
const gran = (src, o) => lay('gran', Object.assign({ src: src, f1t: 'off' }, o));
const AMP = (a, d, s, r) => ({ aa: a, ad: d, as: s, ar: r });
const F = (a, d, s, r, amt) => ({ fa: a, fd: d, fs: s, fr: r, f1e: amt });
const flt = t => (c, r, x) => Object.assign({ f1t: t, f1c: c, f1r: r == null ? 0.1 : r }, x || {});
const lp = flt('lp24'), lp12 = flt('lp12'), hp = flt('hp12'), bp = flt('bp'), ladderF = flt('ladder');
const ME = (a, d, s, r, loop) => ({ a: a, d: d, s: s, r: r, loop: loop ? 1 : 0 });
const LF = (shp, rate, x) => Object.assign({ shp: shp, rate: rate, sync: 0 }, x || {});
const LS = (shp, div, x) => Object.assign({ shp: shp, div: div, sync: 1 }, x || {});
const REV = (mix, decay, size, type, x) => ({ rev: Object.assign({ on: 1, mix: mix, decay: decay, size: size, type: type }, x || {}) });
const DLY = (mix, div, fb, x) => ({ dly: Object.assign({ on: 1, mix: mix, div: div, fb: fb, sync: 1 }, x || {}) });
const CHO = (mix, rate, depth, mode) => ({ cho: { on: 1, mix: mix, rate: rate, depth: depth, mode: mode } });
const PHA = (mix, rate, depth, fb) => ({ pha: { on: 1, mix: mix, rate: rate, depth: depth, fb: fb } });
const DRV = (amt, type, tone) => ({ drv: { on: 1, amt: amt, type: type, tone: tone, mix: 1 } });
const LOFI = (mix, o) => ({ lofi: Object.assign({ on: 1, mix: mix }, o) });
const GATE = (pat, div, depth, smooth) => ({ gate: { on: 1, pat: pat, div: div, depth: depth, smooth: smooth } });
const EQ = (low, mid, mf, high) => ({ eq: { on: 1, low: low, mid: mid, mf: mf, high: high } });
const COMP = (thr, ratio, gain) => ({ comp: { on: 1, thr: thr, ratio: ratio, gain: gain } });
const fx = function () { return Object.assign.apply(null, [{}].concat(Array.prototype.slice.call(arguments))); };
const ARP = (mode, div, oct, gate, x) => Object.assign({ on: 1, mode: mode, div: div, oct: oct, gate: gate, len: 16 }, x || {});
const DRY = REV(0.04, 1, 0.3, 1);

/* ---------- macro kits (macros + orb + mod wheel per category) ---------- */
const KITS = {
  drum: { n: ['Pitch', 'Decay', 'Tone', 'Grit'],
    m: [['m1', '*.pitch', 0.25], ['m2', '*.adec', 0.7], ['m2', '*.arel', 0.5], ['m3', '*.f1c', 0.25], ['m3', '*.ncol', 0.3], ['m3', '*.mbri', 0.3],
      ['m4', 'FX.drv.amt', 0.6], ['mw', '*.f1c', 0.2], ['ox', '*.pitch', 0.1], ['oy', 'FX.drv.amt', 0.3]],
    on: ['drv'] },
  pad: { n: ['Brightness', 'Motion', 'Space', 'Grit'], l3: { shp: 0, rate: 0.11 }, l4: { shp: 6, rate: 0.3 },
    m: [['m1', '*.f1c', 0.28], ['l3', '*.f1c', 0.12, 'm2'], ['l4', '*.pan', 0.4, 'm2'], ['m3', 'FX.rev.mix', 0.35], ['m3', 'FX.dly.mix', 0.18],
      ['m4', 'FX.drv.amt', 0.5], ['m4', 'FX.lofi.mix', 0.8], ['mw', '*.f1c', 0.2], ['ox', '*.f1c', 0.2], ['oy', '*.det', 0.3], ['oy', 'FX.cho.mix', 0.35]],
    on: ['drv', 'lofi', 'cho', 'dly'] },
  keys: { n: ['Tone', 'Tremolo', 'Space', 'Dust'], l3: { shp: 0, rate: 4.8 },
    m: [['m1', '*.f1c', 0.25], ['m1', '*.mbri', 0.25], ['l3', '*.lvl', 0.32, 'm2'], ['l3', '*.pan', 0.5, 'm2'], ['m3', 'FX.rev.mix', 0.32], ['m3', 'FX.dly.mix', 0.15],
      ['m4', 'FX.lofi.mix', 0.9], ['l3', '*.lvl', 0.3, 'mw'], ['ox', '*.f1c', 0.18], ['ox', '*.mbri', 0.2], ['oy', 'FX.cho.mix', 0.5]],
    on: ['lofi', 'cho', 'dly'] },
  bass: { n: ['Cutoff', 'Drive', 'Resonance', 'Wobble'], l3: { shp: 0, sync: 1, div: '1/8', rtg: 1 },
    m: [['m1', '*.f1c', 0.3], ['m2', 'FX.drv.amt', 0.65], ['m3', '*.f1r', 0.45], ['l3', '*.f1c', 0.28, 'm4'], ['mw', '*.f1c', 0.25], ['ox', '*.f1c', 0.22], ['oy', '*.sub', 0.4]],
    on: ['drv'] },
  lead: { n: ['Brightness', 'Vibrato', 'Space', 'Drive'], l3: { shp: 0, rate: 5.5, rtg: 1, fade: 0.25 },
    m: [['m1', '*.f1c', 0.3], ['l3', '*.pitch', 0.012, 'm2'], ['l3', '*.pitch', 0.012, 'mw'], ['m3', 'FX.rev.mix', 0.3], ['m3', 'FX.dly.mix', 0.3],
      ['m4', 'FX.drv.amt', 0.6], ['at', '*.f1c', 0.15], ['ox', '*.f1c', 0.2], ['oy', '*.det', 0.35]],
    on: ['drv', 'dly'] },
  pluck: { n: ['Tone', 'Length', 'Space', 'Dust'],
    m: [['m1', '*.f1c', 0.3], ['m1', '*.pdmp', -0.3], ['m2', '*.adec', 0.55], ['m2', '*.arel', 0.5], ['m2', '*.fdec', 0.5], ['m3', 'FX.rev.mix', 0.3], ['m3', 'FX.dly.mix', 0.25],
      ['m4', 'FX.lofi.mix', 0.9], ['mw', '*.f1c', 0.2], ['ox', '*.f1c', 0.2], ['oy', 'FX.dly.mix', 0.3]],
    on: ['lofi', 'dly'] },
  guitar: { n: ['Tone', 'Damping', 'Space', 'Chorus'],
    m: [['m1', '*.f1c', 0.25], ['m1', '*.pdmp', -0.25], ['m2', '*.pdmp', 0.35], ['m3', 'FX.rev.mix', 0.3], ['m3', 'FX.dly.mix', 0.2], ['m4', 'FX.cho.mix', 0.6],
      ['mw', '*.pdmp', -0.3], ['ox', '*.f1c', 0.2], ['oy', 'FX.dly.mix', 0.3]],
    on: ['cho', 'dly'] },
  bell: { n: ['Tone', 'Length', 'Space', 'Shimmer'],
    m: [['m1', '*.f1c', 0.25], ['m1', '*.mbri', 0.3], ['m2', '*.adec', 0.5], ['m2', '*.arel', 0.6], ['m3', 'FX.rev.mix', 0.35], ['m3', 'FX.dly.mix', 0.2],
      ['m4', 'FX.cho.mix', 0.6], ['m4', 'FX.rev.mix', 0.15], ['mw', '*.mbri', 0.25], ['ox', '*.mbri', 0.3], ['oy', 'FX.dly.mix', 0.3]],
    on: ['cho', 'dly'] },
  texture: { n: ['Density', 'Motion', 'Space', 'Grit'], l3: { shp: 6, rate: 0.2 },
    m: [['m1', '*.gden', 0.3], ['m1', '*.ncol', 0.3], ['l3', '*.gpos', 0.2, 'm2'], ['l3', '*.f1c', 0.15, 'm2'], ['m3', 'FX.rev.mix', 0.35], ['m3', 'FX.dly.mix', 0.2],
      ['m4', 'FX.drv.amt', 0.5], ['m4', 'FX.lofi.mix', 0.8], ['mw', '*.gspr', 0.4], ['ox', '*.gpos', 0.3], ['oy', '*.gpit', 0.3]],
    on: ['drv', 'lofi', 'dly'] },
  drone: { n: ['Brightness', 'Motion', 'Space', 'Dirt'], l3: { shp: 6, rate: 0.08 }, l4: { shp: 0, rate: 0.05 },
    m: [['m1', '*.f1c', 0.25], ['l3', '*.f1c', 0.15, 'm2'], ['l4', '*.pos', 0.3, 'm2'], ['m3', 'FX.rev.mix', 0.3], ['m3', 'FX.dly.mix', 0.2],
      ['m4', 'FX.drv.amt', 0.6], ['m4', '*.ring', 0.3], ['mw', '*.f1c', 0.2], ['ox', '*.f1c', 0.2], ['oy', '*.fold', 0.4]],
    on: ['drv', 'dly'] },
  arp: { n: ['Cutoff', 'Decay', 'Space', 'Dust'],
    m: [['m1', '*.f1c', 0.3], ['m1', '*.mbri', 0.25], ['m2', '*.adec', 0.6], ['m2', '*.fdec', 0.6], ['m3', 'FX.rev.mix', 0.3], ['m3', 'FX.dly.mix', 0.3],
      ['m4', 'FX.lofi.mix', 0.9], ['mw', '*.f1c', 0.25], ['ox', '*.f1c', 0.2], ['oy', 'FX.dly.mix', 0.3]],
    on: ['lofi', 'dly'] },
  fx: { n: ['Tone', 'Rate', 'Space', 'Grit'],
    m: [['m1', '*.f1c', 0.3], ['m2', 'G.l1r', 0.4], ['m3', 'FX.rev.mix', 0.35], ['m3', 'FX.dly.mix', 0.3], ['m4', 'FX.drv.amt', 0.6], ['m4', 'FX.lofi.mix', 0.7],
      ['mw', '*.f1c', 0.25], ['ox', '*.f1c', 0.25], ['oy', '*.pitch', 0.25]],
    on: ['drv', 'lofi', 'dly'] },
  s808: { n: ['Drive', 'Length', 'Punch', 'Tone'],
    m: [['m1', 'FX.drv.amt', 0.6], ['m2', '*.adec', 0.6], ['m2', '*.arel', 0.4], ['e1', '*.pitch', 0.25, 'm3'], ['m4', '*.fold', 0.35],
      ['mw', '*.fold', 0.3], ['ox', '*.fold', 0.25], ['oy', 'FX.drv.amt', 0.3]],
    on: ['drv'] },
  vox: { n: ['Vowel', 'Breath', 'Space', 'Motion'], l3: { shp: 6, rate: 0.25 },
    m: [['m1', '*.vow', 0.5], ['m2', '*.brth', 0.4], ['m3', 'FX.rev.mix', 0.35], ['m3', 'FX.dly.mix', 0.15], ['l3', '*.vow', 0.3, 'm4'], ['l3', '*.gpos', 0.15, 'm4'],
      ['mw', '*.vow', 0.4], ['ox', '*.vow', 0.4], ['oy', '*.fsh', 0.3]],
    on: ['dly'] },
  organ: { n: ['Drive', 'Leslie', 'Space', 'Dust'], l3: { shp: 0, rate: 6.3 },
    m: [['m1', 'FX.drv.amt', 0.6], ['m2', 'FX.cho.mix', 0.5], ['l3', '*.pan', 0.4, 'm2'], ['m3', 'FX.rev.mix', 0.3], ['m4', 'FX.lofi.mix', 0.9],
      ['mw', 'FX.cho.mix', 0.5], ['ox', '*.pos', 0.3], ['oy', 'FX.cho.mix', 0.4]],
    on: ['drv', 'cho', 'lofi'] },
  strings: { n: ['Brightness', 'Vibrato', 'Space', 'Bow'], l3: { shp: 0, rate: 5.2, rtg: 1, fade: 0.6 },
    m: [['m1', '*.f1c', 0.25], ['l3', '*.pitch', 0.008, 'm2'], ['m3', 'FX.rev.mix', 0.3], ['m4', '*.aatk', 0.5], ['m4', '*.brth', 0.3], ['l3', '*.pitch', 0.008, 'mw'],
      ['at', '*.f1c', 0.15], ['ox', '*.f1c', 0.2], ['oy', '*.det', 0.3]],
    on: [] },
  brass: { n: ['Brightness', 'Vibrato', 'Space', 'Growl'], l3: { shp: 0, rate: 5.4, rtg: 1, fade: 0.4 },
    m: [['m1', '*.f1c', 0.28], ['l3', '*.pitch', 0.01, 'm2'], ['m3', 'FX.rev.mix', 0.3], ['m4', '*.f1r', 0.35], ['m4', 'FX.drv.amt', 0.45], ['l3', '*.pitch', 0.01, 'mw'],
      ['at', '*.f1c', 0.2], ['ox', '*.f1c', 0.2], ['oy', '*.pos', 0.3]],
    on: ['drv'] },
  cine: { n: ['Brightness', 'Tension', 'Space', 'Grit'], l3: { shp: 1, rate: 7 },
    m: [['m1', '*.f1c', 0.28], ['l3', '*.lvl', 0.4, 'm2'], ['m3', 'FX.rev.mix', 0.3], ['m3', 'FX.dly.mix', 0.2], ['m4', 'FX.drv.amt', 0.6],
      ['mw', '*.f1c', 0.2], ['ox', '*.f1c', 0.2], ['oy', '*.det', 0.3]],
    on: ['drv', 'dly'] }
};
const CAT_KIT = {
  'Pads': 'pad', 'Atmospheres': 'pad', 'Keys': 'keys', 'Lo-Fi & Dusty': 'keys', 'Bells & Mallets': 'bell', 'Plucks': 'pluck', 'Leads': 'lead',
  'Synth Bass': 'bass', '808 & Sub': 's808', 'Strings': 'strings', 'Brass & Winds': 'brass', 'Choir & Vox': 'vox', 'Organs': 'organ',
  'Guitars & World': 'guitar', 'Arps & Sequences': 'arp', 'Pulses & Gates': 'pad', 'Drones': 'drone', 'Textures & Noise': 'texture',
  'Granular': 'texture', 'Sweeps & FX': 'fx', 'Trap & Drill': 'bell', 'Soul & Boom Bap': 'keys', 'Cinematic': 'cine', 'Modern Hip Hop': 'bell'
};

/* ---------- seeded variation ---------- */
let rs = 1;
function srnd() { rs ^= rs << 13; rs ^= rs >>> 17; rs ^= rs << 5; return (rs >>> 0) / 4294967296; }
function hashStr(s) { let h = 2166136261; for (let i = 0; i < s.length; i++) { h ^= s.charCodeAt(i); h = Math.imul(h, 16777619); } return (h >>> 0) || 1; }
function pick(a) { return a[Math.floor(srnd() * a.length) % a.length]; }
const cl = (x, a, b) => (x < a ? a : x > b ? b : x);
const onLayers = p => p.L.filter(L => L.on);

function applyVary(p, spec) {
  for (const s of spec) {
    let v;
    if (Array.isArray(s[1])) v = pick(s[1]);
    else v = s[1] + srnd() * (s[2] - s[1]);
    if (/\.(uni|crs|oct)$/.test(s[0])) v = Math.round(v);
    D.setPath(p, s[0], Array.isArray(v) ? v.slice() : v);
  }
}
function gentle(p) {
  for (const L of onLayers(p)) {
    if (L.f1t !== 'off') L.f1c = cl(L.f1c + (srnd() * 2 - 1) * 0.03, 0.05, 1);
    if (L.uni > 1) L.det = cl(L.det * (0.85 + srnd() * 0.3), 0, 100);
    if (L.mode === 'modal') L.mbri = cl(L.mbri + (srnd() * 2 - 1) * 0.04, 0, 1);
  }
  p.fx.rev.mix = cl(p.fx.rev.mix + (srnd() * 2 - 1) * 0.04, 0, 0.7);
  p.fx.rev.decay = cl(p.fx.rev.decay * (0.85 + srnd() * 0.3), 0.3, 12);
}
function shiftTone(p, d) {
  for (const L of onLayers(p)) {
    if (L.mode === 'modal') L.mbri = cl(L.mbri + d * 1.4, 0.05, 1);
    else if (L.mode === 'pluck') L.pdmp = cl(L.pdmp - d * 2, 0, 1);
    if (L.f1t !== 'off' && L.f1t !== 'vowel') L.f1c = cl(L.f1c + d, 0.12, 1);
  }
}
const GATE_PATS = [[1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1], [1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0], [1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0], [1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1]];
const OVERLAYS = [
  [/\b(Dusty|Lo-?Fi|Tape|Cassette|Worn|VHS|Faded|Attic|Thrift|Crate|Old|Grandma)\b/i, p => {
    const f = p.fx.lofi; if (!f.on || f.mix < 0.5) { Object.assign(f, { on: 1, mix: 0.85, bits: 11, rate: 1.8, wow: 0.35, flut: 0.2, noise: 0.22, tone: 0.55 }); } }],
  [/\bVinyl\b/i, p => { Object.assign(p.fx.lofi, { on: 1, mix: 1, noise: 0.5, tone: 0.55 }); }],
  [/\b(Dark|Shadow|Midnight|Night|Murk|Smoke|Black|Nocturne|Underground|Basement|Cold)\b/i, p => shiftTone(p, -0.07)],
  [/\b(Bright|Shine|Sparkle|Glitter)\b/i, p => shiftTone(p, 0.06)],
  [/\b(Wide|Vast)\b/i, p => { for (const L of onLayers(p)) { L.spr = cl(L.spr + 0.3, 0, 1); if ((L.mode === 'analog' || L.mode === 'wt') && L.uni < 3 && L.crs > -13) { L.uni = 3; L.det = Math.max(L.det, 10); } } p.fx.width = 1.35; }],
  [/\b(Air|Airy|Clouds?|Heaven|Halo|Shimmer|Cosmic|Nebula|Celestial|Weightless|Cathedral|Hall|Space)\b/i, p => { const r = p.fx.rev; r.on = 1; r.mix = Math.max(r.mix, 0.4); r.decay = Math.max(r.decay, 6); r.size = Math.max(r.size, 0.85); }],
  [/\b(Echo|Dub|Repeat)\b/i, p => { const d = p.fx.dly; d.on = 1; d.mix = Math.max(d.mix, 0.28); d.fb = Math.max(d.fb, 0.45); }],
  [/\b(Gated|Stutter|Chopped|Morse)\b/i, p => { if (!p.fx.gate.on) Object.assign(p.fx.gate, { on: 1, pat: pick(GATE_PATS).slice(), div: '1/16', depth: 1, smooth: 0.15 }); }],
  [/\b(Mono|Solo)\b/i, p => { p.g.vmode = 2; p.g.glide = Math.max(p.g.glide, 0.04); }],
  [/\b(Glide|Slide|Portamento)\b/i, p => { p.g.vmode = 2; p.g.glide = Math.max(p.g.glide, 0.09); }],
  [/\b(Grit|Gritty|Dirty|Crunch|Fuzz|Distorted|Raw|Gnarly)\b/i, p => { const d = p.fx.drv; d.on = 1; d.amt = Math.max(d.amt, 0.5); d.type = d.type || 1; }],
  [/\b(Chorus|Ensemble|Juno)\b/i, p => { const c = p.fx.cho; c.on = 1; c.mix = Math.max(c.mix, 0.5); c.mode = /Ensemble/i.test(p.name) ? 1 : c.mode; }],
  [/\b(Phase|Phaser|Swirl)\b/i, p => { Object.assign(p.fx.pha, { on: 1, mix: 0.55, rate: 0.18, depth: 0.75, fb: 0.5 }); }],
  [/\b(Short|Tight|Stabs?|Staccato)\b/i, p => { for (const L of onLayers(p)) { L.ad *= 0.5; L.ar = Math.min(L.ar, 0.2); L.as *= 0.5; L.pdec = Math.min(L.pdec, 0.8); L.mdec *= 0.6; } }],
  [/\b(Long|Endless|Infinite)\b/i, p => { for (const L of onLayers(p)) { L.ar = Math.max(L.ar * 1.8, 2.5); L.pdec *= 1.6; L.mdec *= 1.5; } }],
  [/\b(Soft|Gentle|Velvet|Silk|Satin|Cotton|Hush|Mellow)\b/i, p => { shiftTone(p, -0.04); for (const L of onLayers(p)) { if (L.mode === 'modal') L.mhard = cl(L.mhard - 0.2, 0, 1); if (L.aa > 0.2) L.aa *= 1.4; } }],
  [/\b(Evolving|Morph|Morphing|Shifting|Drift|Motion|Moving|Living|Breathing)\b/i, p => { p.orb.ch = Math.max(p.orb.ch, 0.35); p.orb.sp = 0.22; }]
];
function usesSrc(p, id) { return p.mods.some(m => m.s === id || m.v === id); }
function applyKit(p, kname) {
  const k = KITS[kname]; if (!k) return;
  k.n.forEach((nm, i) => { p.mac[i].n = nm; p.mac[i].v = 0; });
  const base = { shp: 0, rate: 0.2, sync: 0, div: '1/4', fade: 0, rtg: 0, ph: 0 };
  if (k.l3 && !usesSrc(p, 'l3')) Object.assign(p.lfo[2], base, k.l3);
  if (k.l4 && !usesSrc(p, 'l4')) Object.assign(p.lfo[3], base, k.l4);
  for (const r of k.m) { if (p.mods.length >= 20) break; p.mods.push({ s: r[0], d: r[1], a: r[2], v: r[3] || '' }); }
  for (const u of k.on) {
    const f = p.fx[u];
    if (f.on) continue;
    f.on = 1;
    if (u === 'drv') f.amt = 0; else f.mix = 0;
  }
}

/* ---------- archetype expansion ---------- */
const out = [], names = new Set();
function A(cat, list, base, opt) {
  opt = opt || {};
  list.forEach((nm, i) => {
    if (names.has(nm)) throw new Error('Duplicate preset name: ' + nm);
    names.add(nm);
    const src = JSON.parse(JSON.stringify(base));
    if (src.mods) src.mods = src.mods.map(m => (Array.isArray(m) ? { s: m[0], d: m[1], a: m[2], v: m[3] || '' } : m));
    const p = D.normalizePatch(src);
    p.name = nm; p.cat = cat; p.tags = (opt.tags || []).slice();
    rs = hashStr(nm);
    if (i > 0) { applyVary(p, opt.vary || []); gentle(p); }
    if (opt.over && opt.over[nm]) opt.over[nm](p);
    for (const o of OVERLAYS) if (o[0].test(nm)) o[1](p);
    applyKit(p, opt.kit || CAT_KIT[cat]);
    if (TRIMS[nm]) p.g.trim = TRIMS[nm];
    out.push(p);
  });
}

/* =========================================================== PADS */
A('Pads', ['Silk Horizon', 'Velvet Dawn', 'Slow Sunrise', 'Golden Hour', 'Warm Blanket', 'Soft Focus', 'Satin Sky', 'Dusty Silk'], {
  L: [saw(Object.assign({ uni: 5, det: 18, spr: 0.75, drift: 0.3, vs: 0.3 }, lp(0.5, 0.15), F(1.2, 2.5, 0.55, 2.5, 0.14), AMP(0.9, 1.5, 0.85, 2.8))),
    tri(Object.assign({ crs: -12, lvl: 0.35 }, lp12(0.6), AMP(1.1, 2, 0.9, 3)))],
  lfo: [LF(0, 0.09)], mods: [['l1', 'A.f1c', 0.06]],
  fx: fx(CHO(0.35, 0.4, 0.5, 0), REV(0.32, 4.5, 0.7, 0))
}, { vary: [['L.0.f1c', 0.42, 0.6], ['L.0.det', 10, 26], ['L.1.crs', [-12, 0, 12]], ['fx.rev.decay', 3.5, 6], ['L.0.uni', [3, 5, 7]]] });

A('Pads', ['Glass Cathedral', 'Crystal Bloom', 'Frozen Lake', 'Prism Halo', 'Ice Garden', 'Shimmer Veil', 'Cold Light'], {
  L: [wt('Glass', 0.3, Object.assign({ uni: 3, det: 10, spr: 0.7, vs: 0.3 }, lp12(0.72, 0.1), AMP(1.2, 2, 0.9, 3.2))),
    wt('Harmonic Rise', 0.2, Object.assign({ crs: 12, lvl: 0.3, uni: 2, det: 7, spr: 0.9 }, lp12(0.8), AMP(1.6, 2, 0.85, 3.5)))],
  lfo: [LF(1, 0.07), LF(6, 0.13)], mods: [['l1', 'A.pos', 0.3], ['l2', 'B.pos', 0.25]],
  fx: fx(REV(0.42, 6.5, 0.85, 0), DLY(0.14, '1/4.', 0.35, { hc: 4000 }), CHO(0.25, 0.3, 0.5, 0))
}, { vary: [['L.0.wt', ['Glass', 'Ghost', 'Octave Stack', 'FM Bell']], ['L.0.pos', 0.1, 0.6], ['L.1.wt', ['Harmonic Rise', 'Odd Rise', 'Glass']]] });

A('Pads', ['Juno Clouds', 'Eighties Wash', 'PWM Dreams', 'Polaroid Pad', 'Retro Ensemble', 'Neon Twilight'], {
  L: [sq(Object.assign({ pw: 0.5, sub: 0.35, drift: 0.25 }, lp(0.52, 0.22, { f1k: 0.6 }), F(0.4, 1.5, 0.5, 1.2, 0.12), AMP(0.25, 1, 0.85, 1.4))),
    saw(Object.assign({ lvl: 0.45, uni: 2, det: 6, spr: 0.5 }, lp(0.5, 0.15), AMP(0.3, 1, 0.85, 1.4)))],
  lfo: [LF(1, 0.55)], mods: [['l1', 'A.pw', 0.32]],
  fx: fx(CHO(0.55, 0.45, 0.65, 0), REV(0.25, 3, 0.6, 0))
}, { vary: [['L.0.f1c', 0.42, 0.62], ['lfo.0.rate', 0.3, 1.2], ['L.1.lvl', 0.2, 0.6], ['fx.cho.mode', [0, 1]]] });

A('Pads', ['Living Fog', 'Shifting Sands', 'Morphing Nebula', 'Breathing Walls', 'Tidal Drift', 'Aurora Loop'], {
  L: [wt('Random Spectra', 0.2, Object.assign({ uni: 3, det: 12, spr: 0.8 }, lp(0.62, 0.12), AMP(1.8, 2, 0.9, 4))),
    wt('Vowel', 0.3, Object.assign({ lvl: 0.5, uni: 2, det: 9, spr: 0.9 }, lp12(0.7), AMP(2.2, 2, 0.9, 4.5)))],
  lfo: [LF(0, 0.045), LF(6, 0.09)], mods: [['l1', 'A.pos', 0.45], ['l2', 'B.pos', 0.35], ['l2', 'A.f1c', 0.08]],
  orb: { ch: 0.3, sp: 0.2 },
  fx: fx(REV(0.45, 7, 0.85, 3), DLY(0.16, '1/2', 0.4), CHO(0.25, 0.25, 0.5, 0))
}, { vary: [['L.0.wt', ['Random Spectra', 'Comb', 'Metallic', 'Choir']], ['L.1.wt', ['Vowel', 'Choir', 'Glass', 'Ghost']], ['lfo.0.rate', 0.02, 0.08]] });

A('Pads', ['Midnight Haze', 'Underground', 'Basement Fog', 'Smoke Room', 'Black Velvet', 'Nocturne Pad', 'Shadow Bloom'], {
  L: [saw(Object.assign({ uni: 4, det: 13, spr: 0.7, drift: 0.3 }, lp(0.4, 0.25), F(1.5, 3, 0.6, 3, 0.1), AMP(1.4, 2, 0.9, 3))),
    wt('Growl', 0.2, Object.assign({ crs: -12, lvl: 0.45 }, lp(0.45, 0.2), AMP(1.6, 2, 0.9, 3)))],
  lfo: [LF(0, 0.08)], mods: [['l1', 'A.f1c', 0.07], ['l1', 'B.pos', 0.2]],
  fx: fx(REV(0.36, 5, 0.75, 0), CHO(0.25, 0.3, 0.5, 0))
}, { vary: [['L.0.f1c', 0.34, 0.46], ['L.1.wt', ['Growl', 'Hollow', 'Reed', 'Strings']], ['L.0.uni', [3, 4, 6]]] });

A('Pads', ['Stadium Lights', 'Supernova', 'Euphoria', 'Big Room Pad', 'Hands Up', 'Solar Wind'], {
  L: [saw(Object.assign({ uni: 7, det: 30, spr: 0.9 }, lp12(0.74, 0.06), AMP(0.35, 1, 0.9, 1.6))),
    saw(Object.assign({ crs: 12, lvl: 0.32, uni: 3, det: 15, spr: 0.9 }, lp12(0.78), AMP(0.4, 1, 0.9, 1.6)))],
  fx: fx(REV(0.3, 3.5, 0.7, 0), CHO(0.2, 0.4, 0.5, 0), DLY(0.12, '1/8.', 0.3))
}, { vary: [['L.0.det', 22, 40], ['L.1.crs', [12, 7, -12]], ['L.0.f1c', 0.66, 0.82]] });

A('Pads', ['Grain Cloud', 'Dust Choir', 'Particle Hymn', 'Ghost Chapel', 'Faded Voices'], {
  L: [gran('Choir Chord', Object.assign({ gpos: 0.45, gspr: 0.25, gsz: 0.62, gden: 0.6, gpit: 0.03, spr: 0.8 }, lp12(0.72), AMP(1.5, 2, 0.9, 3.5))),
    sine(Object.assign({ crs: -12, lvl: 0.25 }, AMP(1.5, 2, 0.9, 3)))],
  lfo: [LF(6, 0.06)], mods: [['l1', 'A.gpos', 0.15]],
  fx: fx(REV(0.45, 6, 0.85, 0))
}, { vary: [['L.0.src', ['Choir Chord', 'Strings', 'Glass Pad', 'Organ Chord']], ['L.0.gpos', 0.2, 0.7], ['L.0.gsz', 0.5, 0.75]] });

A('Pads', ['Bowed Glass', 'Singing Bowl', 'Crystal Resonance', 'Temple Air', 'Glass Harmonica'], {
  L: [modal('Glass', Object.assign({ mbow: 0.65, mdec: 5, mbri: 0.55, mhard: 0.2, spr: 0.6 }, AMP(0.9, 1, 1, 3))),
    sine(Object.assign({ crs: 12, lvl: 0.18 }, AMP(1.5, 2, 0.9, 3)))],
  fx: fx(REV(0.5, 6.5, 0.85, 0), CHO(0.3, 0.3, 0.5, 0))
}, { vary: [['L.0.mdl', ['Glass', 'Crystal', 'Bell', 'Plate']], ['L.0.mbri', 0.35, 0.7], ['L.0.mbow', 0.4, 0.8]] });

A('Pads', ['Talking Clouds', 'Vowel Drift', 'Wah Pad', 'Mouth Pad'], {
  L: [wt('Vocal Pulse', 0.2, Object.assign({ uni: 4, det: 14, spr: 0.8 }, lp12(0.7), AMP(0.8, 1.5, 0.9, 2.5)))],
  lfo: [LF(1, 0.12)], mods: [['l1', 'A.pos', 0.45]],
  fx: fx(REV(0.36, 4.5, 0.7, 0), CHO(0.3, 0.4, 0.5, 1))
}, { vary: [['lfo.0.rate', 0.06, 0.3], ['L.0.wt', ['Vocal Pulse', 'Vowel', 'Choir']]] });

/* =========================================================== ATMOSPHERES */
A('Atmospheres', ['Desert Wind', 'Arctic Gust', 'Hollow Canyon', 'Storm Front', 'Lonely Highway'], {
  L: [noise(4, Object.assign({ ncol: 0.5, kt: 0, spr: 0.7 }, bp(0.6, 0.45, { f1k: 0 }), AMP(1.5, 2, 1, 3))),
    wt('Glass', 0.5, Object.assign({ crs: 12, lvl: 0.22, uni: 3, det: 9, spr: 0.8 }, lp12(0.7), AMP(2.5, 2, 0.85, 4)))],
  lfo: [LF(6, 0.1)], mods: [['l1', 'A.f1c', 0.12]],
  fx: fx(REV(0.55, 8, 0.9, 3))
}, { vary: [['L.0.ncol', 0.3, 0.8], ['L.1.wt', ['Glass', 'Ghost', 'Choir']], ['L.1.crs', [0, 12, 19]]] });

A('Atmospheres', ['Heaven Gate', 'Ascension', 'Celestial', 'Halo Rising', 'Weightless', 'Stratosphere'], {
  L: [tri(Object.assign({ uni: 3, det: 10, spr: 0.8, f1t: 'off' }, AMP(1.5, 2, 0.9, 4))),
    wt('Harmonic Rise', 0.35, Object.assign({ crs: 12, lvl: 0.4, uni: 3, det: 12, spr: 0.9 }, lp12(0.75), AMP(2, 2, 0.85, 4.5))),
    modal('Crystal', Object.assign({ crs: 24, lvl: 0.18, mdec: 3, mbri: 0.6, mbow: 0.3, spr: 0.8 }, AMP(2.5, 1, 1, 4)))],
  lfo: [LF(6, 0.08)], mods: [['l1', 'B.pos', 0.25]],
  fx: fx(REV(0.6, 10, 0.95, 3), DLY(0.25, '1/4.', 0.45, { pp: 1, hc: 3500 }), CHO(0.3, 0.3, 0.5, 0))
}, { vary: [['L.1.wt', ['Harmonic Rise', 'Octave Stack', 'Ghost']], ['L.2.crs', [12, 19, 24]]] });

A('Atmospheres', ['Abandoned Station', 'Deep Space Probe', 'Lost Signal', 'Cold War', 'Derelict'], {
  L: [saw(Object.assign({ ring: 0.45, rngr: 1.51, uni: 2, det: 9, spr: 0.6 }, lp(0.42, 0.35), AMP(2, 2, 0.9, 4))),
    noise(0, Object.assign({ ncol: 0.35, kt: 0, lvl: 0.25 }, bp(0.45, 0.5, { f1k: 0 }), AMP(2, 2, 1, 4)))],
  lfo: [LF(0, 0.05), LF(6, 0.11)], mods: [['l1', 'A.f1c', 0.1], ['l2', 'B.f1c', 0.15], ['l2', 'A.ring', 0.15]],
  fx: fx(REV(0.55, 9, 0.9, 3), DLY(0.2, '1/2', 0.5))
}, { vary: [['L.0.rngr', [1.41, 1.51, 2.27, 0.73]], ['L.0.f1c', 0.35, 0.5]] });

A('Atmospheres', ['Metal Mist', 'Alien Garden', 'Bioluminescence', 'Spore Field', 'Glass Rain'], {
  L: [wt('FM 3:1', 0.25, Object.assign({ uni: 2, det: 8, spr: 0.8 }, lp12(0.72), AMP(1.8, 2, 0.85, 4))),
    sine(Object.assign({ fm: 0.35, fmr: 3.53, lvl: 0.4, uni: 2, det: 6, spr: 0.9 }, AMP(2.5, 3, 0.6, 4)))],
  lfo: [LF(0, 0.06), LF(6, 0.2)], mods: [['l1', 'A.pos', 0.4], ['l2', 'B.fm', 0.15]],
  fx: fx(REV(0.5, 7, 0.85, 0), DLY(0.2, '1/8.', 0.45), CHO(0.25, 0.3, 0.5, 0))
}, { vary: [['L.1.fmr', [3.53, 1.41, 2.76, 5.4]], ['L.0.wt', ['FM 3:1', 'FM Bell', 'FM 2:1', 'Metallic']]] });

A('Atmospheres', ['Whisper Hall', 'Phantom Choir', 'Ancestors', 'Spirit Room', 'Murmurs'], {
  L: [vox(Object.assign({ vow: 0.6, uni: 5, det: 16, spr: 0.9, brth: 0.35 }, lp12(0.7), AMP(2, 2, 0.9, 4))),
    noise(0, Object.assign({ ncol: 0.65, kt: 0, lvl: 0.15 }, bp(0.7, 0.3, { f1k: 0 }), AMP(1.5, 2, 1, 3)))],
  lfo: [LF(6, 0.12)], mods: [['l1', 'A.vow', 0.3]],
  fx: fx(REV(0.6, 8, 0.9, 3), DLY(0.18, '1/4', 0.45))
}, { vary: [['L.0.vow', 0.2, 0.9], ['L.0.fsh', 0.35, 0.65]], kit: 'vox' });

A('Atmospheres', ['Submerged', 'Deep Blue', 'Ocean Floor', 'Pressure Zone', 'Abyss Light'], {
  L: [wt('Hollow', 0.4, Object.assign({ uni: 3, det: 10, spr: 0.7 }, lp(0.38, 0.45), AMP(1.5, 2, 0.9, 3.5))),
    noise(4, Object.assign({ ncol: 0.3, kt: 0, lvl: 0.3 }, lp(0.4, 0.3, { f1k: 0 }), AMP(2, 2, 1, 4)))],
  lfo: [LF(0, 0.07)], mods: [['l1', 'A.f1c', 0.1]],
  fx: fx(CHO(0.5, 0.25, 0.8, 1), REV(0.5, 6, 0.8, 0), PHA(0.3, 0.08, 0.6, 0.4))
}, { vary: [['L.0.f1c', 0.32, 0.44], ['L.0.wt', ['Hollow', 'Choir', 'Odd Rise']]] });

/* =========================================================== KEYS */
A('Keys', ['Tine Classic', 'DX Ballad', 'Glass Rhodes', 'Eighties Love Keys', 'Crystal EP', 'Silver Tines'], {
  L: [sine(Object.assign({ fmr: 1, vs: 0.7 }, AMP(0.002, 3.2, 0, 0.45))),
    sine(Object.assign({ fmr: 14, lvl: 0.3, vs: 0.8 }, AMP(0.001, 0.9, 0, 0.3)))],
  me: [ME(0, 1.4, 0.12, 0.4), ME(0, 0.18, 0, 0.2)],
  mods: [['e1', 'A.fm', 0.42], ['e2', 'B.fm', 0.55], ['vel', 'A.fm', 0.18], ['vel', 'B.fm', 0.2]],
  fx: fx(CHO(0.35, 0.6, 0.4, 0), REV(0.22, 2.5, 0.5, 1), EQ(1, 0, 1000, 1.5))
}, { vary: [['me.0.d', 0.8, 2], ['L.1.lvl', 0.15, 0.4], ['L.1.fmr', [14, 13, 7, 19]]] });

A('Keys', ['Warm Suitcase', 'Mark One', 'Late Night Rhodes', 'Velvet Keys', 'Neo Soul Keys', 'Quiet Storm', 'Sunday Keys'], {
  L: [modal('Tine', Object.assign({ mdec: 5.5, mbri: 0.42, mhard: 0.45, vs: 0.75 }, lp12(0.66, 0.05, { f1v: 0.6, f1k: 0.4 }), AMP(0.001, 1, 1, 0.35))),
    sine(Object.assign({ lvl: 0.35, vs: 0.6 }, AMP(0.002, 4.5, 0, 0.3)))],
  fx: fx(DRV(0.18, 1, 0.6), CHO(0.25, 0.5, 0.4, 0), REV(0.2, 2.2, 0.5, 1))
}, { vary: [['L.0.mbri', 0.32, 0.55], ['L.0.mhard', 0.3, 0.6], ['L.0.f1c', 0.58, 0.72]] });

A('Keys', ['Wurli Bark', 'Classroom Keys', 'Reedy EP', 'Woody Wurli', 'Barbershop Keys'], {
  L: [lay('analog', Object.assign({ shp: 0.85, pw: 0.42, fold: 0.15, vs: 0.75 }, lp12(0.52, 0.15, { f1v: 0.5 }), F(0.001, 0.35, 0.2, 0.3, 0.22), AMP(0.002, 2.4, 0.08, 0.3))),
    modal('Tine', Object.assign({ lvl: 0.35, mdec: 2.6, mbri: 0.6, mhard: 0.7 }, AMP(0.001, 1, 1, 0.3)))],
  lfo: [LF(0, 5.2)], mods: [['l1', '*.lvl', 0.12]],
  fx: fx(DRV(0.32, 1, 0.55), REV(0.18, 1.8, 0.4, 1))
}, { vary: [['L.0.fold', 0.05, 0.3], ['L.0.f1c', 0.45, 0.6], ['fx.drv.amt', 0.2, 0.45]] });

A('Keys', ['Felt Piano', 'Muted Upright', 'Hush Piano', 'Bedroom Piano', 'Cotton Keys', 'Soft Hammers'], {
  L: [modal('Piano', Object.assign({ mdec: 3.5, mbri: 0.38, mhard: 0.25, minh: 0.45, spr: 0.3, vs: 0.8 }, lp12(0.6, 0.05, { f1v: 0.7, f1k: 0.6 }), AMP(0.002, 1, 1, 0.45))),
    noise(0, Object.assign({ ncol: 0.25, lvl: 0.12 }, lp12(0.35, 0.1, { f1k: 1 }), AMP(0.001, 0.06, 0, 0.05)))],
  fx: fx(REV(0.24, 2.2, 0.55, 1), EQ(1.5, -1, 600, -2))
}, { vary: [['L.0.mbri', 0.3, 0.45], ['L.0.mdec', 2.5, 5], ['L.0.f1c', 0.52, 0.66]] });

A('Keys', ['Pop Grand', 'Bright Upright', 'Honky Bar', 'Ballad Piano', 'Studio Piano'], {
  L: [modal('Piano', Object.assign({ mdec: 4.5, mbri: 0.55, mhard: 0.65, minh: 0.6, spr: 0.4, vs: 0.75 }, lp12(0.8, 0.05, { f1v: 0.5, f1k: 0.4 }), AMP(0.001, 1, 1, 0.5))),
    sine(Object.assign({ fmr: 2, lvl: 0.25 }, AMP(0.001, 1.8, 0, 0.4)))],
  me: [ME(0, 0.5, 0, 0.3)], mods: [['e1', 'B.fm', 0.3]],
  fx: fx(REV(0.22, 2.5, 0.6, 0), COMP(-16, 3, 3))
}, { vary: [['L.0.minh', 0.3, 0.9], ['L.0.mbri', 0.45, 0.65]], over: { 'Honky Bar': p => { p.L[1].fin = 14; p.L[1].lvl = 0.45; p.L[1].fm = 0.05; } } });

A('Keys', ['Funk Clav', 'Wah Clav', 'Rubber Clav', 'Stank Clav', 'Clavinet Pop'], {
  L: [sq(Object.assign({ pw: 0.22, vs: 0.8 }, lp(0.5, 0.35, { f1v: 0.6, f1k: 0.5 }), F(0.001, 0.22, 0.25, 0.1, 0.35), AMP(0.001, 0.8, 0.35, 0.08))),
    noise(0, Object.assign({ ncol: 0.8, lvl: 0.15 }, hp(0.7, 0.1), AMP(0.001, 0.012, 0, 0.01)))],
  fx: fx(DRV(0.25, 0, 0.7), COMP(-18, 4, 4), REV(0.12, 1.2, 0.3, 1))
}, { vary: [['L.0.pw', 0.12, 0.35], ['L.0.f1c', 0.42, 0.6], ['L.0.f1r', 0.2, 0.55]], over: { 'Wah Clav': p => { p.lfo[0] = Object.assign(p.lfo[0], { shp: 1, rate: 1.2, sync: 1, div: '1/8', rtg: 0 }); p.mods.push({ s: 'l1', d: 'A.f1c', a: 0.18, v: '' }); } } });

A('Keys', ['Poly Keys', 'Prophet Keys', 'Cozy Keys', 'House Keys', 'Fuzzy Chords'], {
  L: [saw(Object.assign({ uni: 2, det: 9, spr: 0.5 }, lp(0.45, 0.2, { f1v: 0.5 }), F(0.001, 0.6, 0.25, 0.4, 0.3), AMP(0.002, 1.2, 0.45, 0.45))),
    sq(Object.assign({ crs: 12, lvl: 0.25, pw: 0.4 }, lp(0.55, 0.1), AMP(0.002, 1, 0.4, 0.4)))],
  fx: fx(CHO(0.35, 0.5, 0.5, 0), REV(0.2, 2, 0.5, 1), DLY(0.1, '1/8.', 0.3))
}, { vary: [['L.0.f1c', 0.38, 0.55], ['L.0.fd', 0.3, 0.9], ['L.1.crs', [12, 0, 7]]] });

/* =========================================================== LO-FI */
A('Lo-Fi & Dusty', ['Dusty Rhodes', 'Basement Tape', 'Study Session', 'Rainy Window Keys', 'Thrift Store Keys', 'Cassette Rhodes', 'Old Soul'], {
  L: [modal('Tine', Object.assign({ mdec: 5, mbri: 0.38, mhard: 0.4, vs: 0.7 }, lp12(0.55, 0.08, { f1v: 0.5 }), AMP(0.001, 1, 1, 0.35))),
    sine(Object.assign({ lvl: 0.3 }, AMP(0.002, 4, 0, 0.3)))],
  fx: fx(LOFI(0.9, { bits: 11, rate: 2, wow: 0.4, flut: 0.25, noise: 0.25, tone: 0.5 }), CHO(0.25, 0.6, 0.4, 0), REV(0.22, 2, 0.5, 1))
}, { vary: [['fx.lofi.bits', 9, 13], ['fx.lofi.wow', 0.2, 0.55], ['fx.lofi.rate', 1, 3]] });

A('Lo-Fi & Dusty', ['Worn Piano', 'Attic Piano', 'Grandma Upright', 'Dusty Keys', 'Faded Photo', 'Vinyl Piano'], {
  L: [modal('Piano', Object.assign({ mdec: 3, mbri: 0.35, mhard: 0.3, minh: 0.55, vs: 0.75 }, lp12(0.55, 0.05, { f1v: 0.6 }), AMP(0.002, 1, 1, 0.4)))],
  fx: fx(LOFI(0.85, { bits: 10, rate: 2.5, wow: 0.45, flut: 0.3, noise: 0.3, tone: 0.48 }), REV(0.25, 2, 0.55, 1))
}, { vary: [['fx.lofi.bits', 8, 12], ['L.0.minh', 0.4, 1], ['fx.lofi.tone', 0.4, 0.6]] });

A('Lo-Fi & Dusty', ['Cassette Clouds', 'VHS Dreams', 'Warped Tape Pad', 'Sun Bleached', 'Memory Lane'], {
  L: [sq(Object.assign({ pw: 0.45, sub: 0.3, drift: 0.4 }, lp(0.48, 0.18), AMP(0.5, 1, 0.85, 1.8))),
    saw(Object.assign({ lvl: 0.4, uni: 3, det: 12, spr: 0.6, drift: 0.4 }, lp(0.48, 0.12), AMP(0.6, 1, 0.85, 1.8)))],
  lfo: [LF(1, 0.4)], mods: [['l1', 'A.pw', 0.25]],
  fx: fx(LOFI(0.9, { bits: 10, rate: 2, wow: 0.6, flut: 0.3, noise: 0.2, tone: 0.5 }), CHO(0.4, 0.4, 0.6, 0), REV(0.3, 3.5, 0.7, 0))
}, { vary: [['fx.lofi.wow', 0.4, 0.8], ['L.0.f1c', 0.4, 0.55]], kit: 'pad' });

A('Lo-Fi & Dusty', ['Dusty Nylon', 'Bedroom Guitar', 'Coffee Shop', 'Lazy Sunday', 'Window Seat'], {
  L: [pluck(Object.assign({ pdec: 2.2, pdmp: 0.5, ppos: 0.18, pexc: 0.45, vs: 0.7 }, lp12(0.62, 0.05), AMP(0.001, 1, 1, 0.35)))],
  fx: fx(LOFI(0.85, { bits: 11, rate: 1.5, wow: 0.35, flut: 0.25, noise: 0.2, tone: 0.5 }), CHO(0.2, 0.6, 0.4, 0), REV(0.22, 2, 0.5, 1))
}, { vary: [['L.0.pdmp', 0.35, 0.6], ['L.0.pexc', 0.3, 0.6]], kit: 'guitar' });

A('Lo-Fi & Dusty', ['Dusty Vibes', 'Tape Mallets', 'Crate Vibes', 'Faded Bells', 'Jazz Record'], {
  L: [modal('Vibes', Object.assign({ mdec: 2.5, mbri: 0.4, mhard: 0.35, vs: 0.7 }, lp12(0.62), AMP(0.001, 1, 1, 0.6)))],
  lfo: [LF(0, 4.5)], mods: [['l1', '*.lvl', 0.25]],
  fx: fx(LOFI(0.85, { bits: 10, rate: 2, wow: 0.35, flut: 0.2, noise: 0.25, tone: 0.5 }), REV(0.25, 2.5, 0.6, 0))
}, { vary: [['L.0.mbri', 0.3, 0.5], ['lfo.0.rate', 3.5, 6]], kit: 'bell' });

/* =========================================================== BELLS & MALLETS */
A('Bells & Mallets', ['Marimba', 'Wood Mallets', 'Island Marimba', 'Soft Marimba', 'Bright Marimba'], {
  L: [modal('Marimba', Object.assign({ mdec: 0.9, mbri: 0.45, mhard: 0.5, spr: 0.3, vs: 0.7 }, AMP(0.001, 1, 1, 0.3)))],
  fx: fx(REV(0.2, 1.8, 0.45, 1))
}, { vary: [['L.0.mhard', 0.35, 0.7], ['L.0.mdec', 0.6, 1.3]] });

A('Bells & Mallets', ['Vibraphone', 'Motor Vibes', 'Cool Jazz Vibes', 'Late Vibes', 'Midnight Vibes'], {
  L: [modal('Vibes', Object.assign({ mdec: 3.2, mbri: 0.45, mhard: 0.45, spr: 0.3, vs: 0.7 }, AMP(0.001, 1, 1, 0.9)))],
  lfo: [LF(0, 5.2)], mods: [['l1', 'A.lvl', 0.28]],
  fx: fx(REV(0.28, 2.5, 0.6, 0))
}, { vary: [['lfo.0.rate', 3, 6.5], ['mods.0.a', 0.1, 0.4]] });

A('Bells & Mallets', ['Glockenspiel', 'Celesta', 'Toy Piano', 'Sugarplum', 'Sparkle Glock'], {
  L: [modal('Glock', Object.assign({ crs: 12, mdec: 1.6, mbri: 0.6, mhard: 0.6 }, AMP(0.001, 1, 1, 0.6)))],
  fx: fx(REV(0.28, 2.5, 0.6, 0))
}, { vary: [['L.0.mdl', ['Glock', 'Music Box', 'Crystal']], ['L.0.mbri', 0.45, 0.7]] });

A('Bells & Mallets', ['Church Bell', 'Tubular Bells', 'Temple Bell', 'Midnight Chime', 'Distant Bell'], {
  L: [modal('Bell', Object.assign({ mdec: 6, mbri: 0.5, mhard: 0.6, spr: 0.4 }, AMP(0.001, 1, 1, 4)))],
  fx: fx(REV(0.4, 5, 0.85, 0))
}, { vary: [['L.0.mdec', 3, 9], ['L.0.minh', 0, 0.6]] });

A('Bells & Mallets', ['FM Bell', 'Digital Chime', 'Ice Bell', 'Crystal Ring', 'Satellite Bell', 'DX Bell'], {
  L: [sine(Object.assign({ fmr: 3.5, vs: 0.6 }, AMP(0.001, 4, 0, 1.5))),
    sine(Object.assign({ fmr: 1.41, lvl: 0.4, crs: 12 }, AMP(0.001, 2.5, 0, 1.2)))],
  me: [ME(0, 2.5, 0.1, 1), ME(0, 1.2, 0, 0.6)], mods: [['e1', 'A.fm', 0.45], ['e2', 'B.fm', 0.4], ['vel', 'A.fm', 0.15]],
  fx: fx(REV(0.32, 3.5, 0.7, 0), DLY(0.15, '1/8.', 0.35))
}, { vary: [['L.0.fmr', [3.5, 2.76, 4.2, 7]], ['L.1.fmr', [1.41, 2, 3.5]]] });

A('Bells & Mallets', ['Music Box', 'Lullaby Box', 'Haunted Music Box', 'Clockwork Dreams', 'Wind-Up Toy'], {
  L: [modal('Music Box', Object.assign({ crs: 12, mdec: 2, mbri: 0.6, mhard: 0.75 }, AMP(0.001, 1, 1, 1)))],
  fx: fx(REV(0.32, 3, 0.65, 0))
}, { vary: [['L.0.mbri', 0.45, 0.7]], over: { 'Haunted Music Box': p => { p.L[0].fin = -18; Object.assign(p.fx.lofi, { on: 1, mix: 0.7, wow: 0.7, flut: 0.3, bits: 12, rate: 1, noise: 0.15, tone: 0.5 }); } } });

A('Bells & Mallets', ['Steel Pan', 'Caribbean Pan', 'Tin Drum'], {
  L: [modal('Steel Pan', Object.assign({ mdec: 1.6, mbri: 0.5, mhard: 0.55 }, AMP(0.001, 1, 1, 0.5)))],
  fx: fx(REV(0.22, 2, 0.5, 1))
}, { vary: [['L.0.mbri', 0.4, 0.6]] });

/* =========================================================== PLUCKS */
A('Plucks', ['Pluck Classic', 'Future Pluck', 'Bounce Pluck', 'Candy Pluck', 'Neon Pluck', 'House Pluck'], {
  L: [saw(Object.assign({ uni: 3, det: 12, spr: 0.6, vs: 0.6 }, lp(0.32, 0.22, { f1v: 0.4 }), F(0.001, 0.25, 0, 0.2, 0.45), AMP(0.001, 0.5, 0, 0.3)))],
  fx: fx(DLY(0.22, '1/8.', 0.35), REV(0.2, 2, 0.5, 1))
}, { vary: [['L.0.f1c', 0.26, 0.38], ['L.0.fd', 0.15, 0.4], ['L.0.f1e', 0.35, 0.55], ['L.0.shp', [0.667, 1, 0.85]]] });

A('Plucks', ['Digital Pluck', 'Zap Pluck', 'Sync Pluck', 'Crystal Pick', 'Tech Pluck'], {
  L: [wt('Sync Saw', 0.1, Object.assign({ uni: 2, det: 8, spr: 0.5 }, lp(0.55, 0.15), F(0.001, 0.3, 0, 0.2, 0.25), AMP(0.001, 0.45, 0, 0.25)))],
  me: [ME(0, 0.25, 0, 0.2)], mods: [['e1', 'A.pos', 0.6]],
  fx: fx(DLY(0.2, '1/16', 0.3), REV(0.18, 1.8, 0.4, 1))
}, { vary: [['L.0.wt', ['Sync Saw', 'PWM', 'Phase Dist', 'Digital Saw']], ['me.0.d', 0.12, 0.4]] });

A('Plucks', ['String Pluck', 'Harp Pluck', 'Glass Pick', 'Pizz Synth', 'Wire Pluck'], {
  L: [pluck(Object.assign({ pdec: 1.4, pdmp: 0.45, ppos: 0.15, pexc: 0.6 }, lp12(0.78), AMP(0.001, 1, 1, 0.3)))],
  fx: fx(DLY(0.22, '1/8.', 0.35), REV(0.25, 2.5, 0.6, 0))
}, { vary: [['L.0.pdmp', 0.2, 0.6], ['L.0.pdec', 0.6, 2.5], ['L.0.uni', [1, 2]]] });

A('Plucks', ['Bell Pluck', 'Ice Pluck', 'Sparkle Pick', 'Chime Pluck', 'Sugar Pluck'], {
  L: [modal('Glock', Object.assign({ mdec: 0.8, mbri: 0.6, mhard: 0.6 }, AMP(0.001, 1, 1, 0.3))),
    saw(Object.assign({ lvl: 0.45, uni: 2, det: 10, spr: 0.6 }, lp(0.35, 0.15), F(0.001, 0.18, 0, 0.15, 0.4), AMP(0.001, 0.35, 0, 0.2)))],
  fx: fx(DLY(0.25, '1/8.', 0.4), REV(0.25, 2.5, 0.6, 0))
}, { vary: [['L.0.mdl', ['Glock', 'Crystal', 'Music Box', 'Bell']], ['L.1.lvl', 0.25, 0.55]] });

A('Plucks', ['Wood Pluck', 'Tropical Pluck', 'Dancehall Pluck', 'Afro Pluck', 'Log Drum Pluck'], {
  L: [modal('Marimba', Object.assign({ mdec: 0.7, mbri: 0.45, mhard: 0.6 }, AMP(0.001, 1, 1, 0.2))),
    sq(Object.assign({ lvl: 0.35, pw: 0.3 }, lp(0.38, 0.2), F(0.001, 0.15, 0, 0.1, 0.35), AMP(0.001, 0.25, 0, 0.15)))],
  fx: fx(DLY(0.15, '1/8', 0.3), REV(0.15, 1.5, 0.4, 1))
}, { vary: [['L.0.mdl', ['Marimba', 'Wood', 'Drum', 'Kalimba']], ['L.0.mhard', 0.4, 0.8]],
  over: { 'Log Drum Pluck': p => { p.L[0].mdl = 'Wood'; p.L[0].crs = -12; p.me[0] = { a: 0, d: 0.06, s: 0, r: 0.05, loop: 0 }; p.mods.push({ s: 'e1', d: '*.pitch', a: 0.12, v: '' }); } } });

A('Plucks', ['Chip Pluck', 'Eight Bit Pick', 'Game Over', 'Pixel Drop'], {
  L: [sq(Object.assign({ pw: 0.25, f1t: 'off' }, AMP(0.001, 0.3, 0, 0.08)))],
  me: [ME(0, 0.05, 0, 0.05)], mods: [['e1', 'A.pitch', 0.5]],
  fx: fx(LOFI(0.6, { bits: 6, rate: 4, wow: 0, flut: 0, noise: 0, tone: 0.8 }), DLY(0.15, '1/16', 0.25))
}, { vary: [['L.0.pw', [0.125, 0.25, 0.5]], ['me.0.d', 0.02, 0.08]] });

/* =========================================================== LEADS */
A('Leads', ['Saw Lead', 'Big Saw Lead', 'Festival Lead', 'Fuzz Screamer', 'Anthem Lead'], {
  L: [saw(Object.assign({ uni: 3, det: 12, spr: 0.5 }, lp(0.68, 0.2), F(0.001, 0.4, 0.6, 0.3, 0.12), AMP(0.003, 0.5, 1, 0.25))),
    saw(Object.assign({ crs: 12, lvl: 0.3 }, lp(0.7, 0.1), AMP(0.003, 0.5, 1, 0.25)))],
  g: { glide: 0.03 },
  fx: fx(DLY(0.22, '1/8.', 0.35), REV(0.18, 2, 0.5, 1))
}, { vary: [['L.0.det', 8, 20], ['L.1.crs', [12, -12, 7]], ['L.0.f1c', 0.6, 0.78]] });

A('Leads', ['Square Lead', 'Mono Square', 'Glide Square', 'Retro Hero', 'Arcade Lead'], {
  L: [sq(Object.assign({ pw: 0.5, sub: 0.25 }, lp(0.6, 0.25), F(0.001, 0.3, 0.5, 0.2, 0.15), AMP(0.002, 0.3, 1, 0.15)))],
  g: { vmode: 2, glide: 0.06 },
  lfo: [LF(1, 0.4)], mods: [['l1', 'A.pw', 0.12]],
  fx: fx(DLY(0.2, '1/8', 0.3), REV(0.15, 1.8, 0.4, 1))
}, { vary: [['L.0.pw', 0.3, 0.5], ['g.glide', 0.03, 0.12]] });

A('Leads', ['Whistle Lead', 'Ghost Whistle', 'Falsetto Sine', 'Space Whistle', 'Dream Lead', 'Moon Lead'], {
  L: [lay('analog', Object.assign({ shp: 0.08, vs: 0.4, f1t: 'off' }, AMP(0.04, 0.5, 1, 0.35))),
    sine(Object.assign({ crs: 12, lvl: 0.15 }, AMP(0.08, 0.5, 1, 0.35)))],
  g: { vmode: 2, glide: 0.07 },
  lfo: [LF(0, 5.4, { rtg: 1, fade: 0.35 })], mods: [['l1', '*.pitch', 0.01]],
  fx: fx(DLY(0.3, '1/4.', 0.45, { hc: 4000 }), REV(0.38, 4.5, 0.75, 0), CHO(0.2, 0.4, 0.4, 0))
}, { vary: [['L.0.shp', 0, 0.25], ['lfo.0.rate', 4.5, 6.2], ['L.1.crs', [12, 19, 24]]] });

A('Leads', ['Sync Lead', 'Sync Screamer', 'Tearing Lead', 'Sync Sweep Lead'], {
  L: [saw(Object.assign({ sync: 0.3, uni: 2, det: 8, spr: 0.4 }, lp(0.78, 0.15), AMP(0.002, 0.4, 1, 0.2)))],
  me: [ME(0, 0.6, 0.2, 0.3)], mods: [['e1', 'A.sync', 0.4], ['mw', 'A.sync', 0.4]],
  fx: fx(DRV(0.25, 0, 0.7), DLY(0.2, '1/8.', 0.3), REV(0.15, 1.6, 0.4, 1))
}, { vary: [['L.0.sync', 0.15, 0.45], ['me.0.d', 0.3, 1.2]] });

A('Leads', ['Talkbox Lead', 'Vowel Lead', 'Robo Voice', 'Yoi Lead'], {
  L: [wt('Vowel', 0.1, Object.assign({ uni: 2, det: 6, spr: 0.3 }, lp(0.75, 0.1), AMP(0.005, 0.4, 1, 0.2)))],
  g: { vmode: 2, glide: 0.05 },
  me: [ME(0.05, 0.4, 0.3, 0.2)], mods: [['e1', 'A.pos', 0.5], ['mw', 'A.pos', 0.5]],
  fx: fx(DRV(0.2, 1, 0.7), DLY(0.18, '1/8.', 0.3), REV(0.15, 1.5, 0.4, 1))
}, { vary: [['L.0.wt', ['Vowel', 'Vocal Pulse', 'Choir']], ['me.0.d', 0.2, 0.6]] });

A('Leads', ['Supersaw Lead', 'Trance Lead', 'Laser Lead', 'Hyper Lead'], {
  L: [saw(Object.assign({ uni: 7, det: 26, spr: 0.8 }, lp12(0.78, 0.08), AMP(0.002, 0.5, 1, 0.3)))],
  fx: fx(DLY(0.25, '1/8.', 0.4), REV(0.25, 3, 0.6, 0))
}, { vary: [['L.0.det', 18, 35]] });

A('Leads', ['Ladder Lead', 'Funk Lead', 'West Coast Whine', 'Slide Synth Lead', 'Talk Box Funk'], {
  L: [lay('analog', Object.assign({ shp: 0.2 }, ladderF(0.62, 0.35), F(0.001, 0.3, 0.5, 0.2, 0.15), AMP(0.005, 0.4, 1, 0.2))),
    saw(Object.assign({ lvl: 0.25 }, ladderF(0.55, 0.2), AMP(0.005, 0.4, 1, 0.2)))],
  g: { vmode: 2, glide: 0.1 },
  lfo: [LF(0, 5.8, { rtg: 1, fade: 0.2 })], mods: [['l1', '*.pitch', 0.012]],
  fx: fx(DLY(0.2, '1/8.', 0.35), REV(0.2, 2.2, 0.5, 1))
}, { vary: [['L.0.shp', 0.05, 0.4], ['g.glide', 0.06, 0.15]] });

/* =========================================================== SYNTH BASS */
A('Synth Bass', ['Analog Bass', 'Moog Low', 'Fat Bass', 'Rubber Bass', 'Round Bass'], {
  L: [saw(Object.assign({ sub: 0.45, vs: 0.5 }, ladderF(0.32, 0.3, { f1v: 0.4, f1k: 0.3 }), F(0.001, 0.3, 0.15, 0.2, 0.32), AMP(0.001, 0.6, 0.8, 0.12))),
    sq(Object.assign({ lvl: 0.35 }, ladderF(0.32, 0.2), AMP(0.001, 0.6, 0.8, 0.12)))],
  g: { vmode: 2, glide: 0.025 },
  fx: fx(DRV(0.2, 1, 0.6), DRY)
}, { vary: [['L.0.f1c', 0.26, 0.4], ['L.0.f1r', 0.15, 0.45], ['L.0.f1e', 0.2, 0.4]] });

A('Synth Bass', ['Reese', 'Dark Reese', 'Liquid Reese', 'Neuro Reese', 'Hollow Reese'], {
  L: [saw(Object.assign({ uni: 4, det: 24, spr: 0.4 }, lp(0.42, 0.15), AMP(0.005, 0.5, 1, 0.15))),
    sine(Object.assign({ crs: -12, lvl: 0.5 }, AMP(0.005, 0.5, 1, 0.15)))],
  g: { vmode: 2, glide: 0.04 },
  lfo: [LF(0, 0.15)], mods: [['l1', 'A.f1c', 0.08]],
  fx: fx(DRV(0.3, 0, 0.55), REV(0.06, 1.5, 0.3, 1))
}, { vary: [['L.0.det', 15, 35], ['L.0.f1c', 0.35, 0.5]] });

A('Synth Bass', ['Pluck Bass', 'Funk Bass', 'Slap Synth', 'Bounce Bass', 'Pop Bass'], {
  L: [saw(Object.assign({ sub: 0.3, vs: 0.7 }, lp(0.3, 0.25, { f1v: 0.5 }), F(0.001, 0.18, 0, 0.1, 0.42), AMP(0.001, 0.55, 0, 0.1)))],
  g: { vmode: 1 },
  fx: fx(COMP(-20, 4, 4), DRY)
}, { vary: [['L.0.fd', 0.1, 0.3], ['L.0.shp', [0.667, 1, 0.8]]] });

A('Synth Bass', ['Wobble', 'Wub Machine', 'Dubstep Growl', 'Half-Time Wobble'], {
  L: [wt('Growl', 0.4, Object.assign({ uni: 2, det: 10, spr: 0.3, sub: 0.4 }, lp(0.4, 0.35), AMP(0.003, 0.5, 1, 0.15)))],
  g: { vmode: 2, glide: 0.03 },
  lfo: [LS(0, '1/8', { rtg: 1 })], mods: [['l1', 'A.f1c', 0.25], ['l1', 'A.pos', 0.3]],
  fx: fx(DRV(0.4, 0, 0.6), DRY)
}, { vary: [['lfo.0.div', ['1/8', '1/4', '1/8T', '1/16']], ['L.0.wt', ['Growl', 'FM 2:1', 'Fold']]], over: { 'Half-Time Wobble': p => { p.lfo[0].div = '1/4'; } } });

A('Synth Bass', ['FM Bass', 'Growl Bass', 'Metallic Bass', 'Digital Bass'], {
  L: [sine(Object.assign({ fm: 0.35, fmr: 1, sub: 0.3 }, AMP(0.001, 0.6, 0.7, 0.12))),
    sine(Object.assign({ crs: -12, lvl: 0.4 }, AMP(0.001, 0.6, 0.8, 0.12)))],
  me: [ME(0, 0.3, 0.2, 0.2)], mods: [['e1', 'A.fm', 0.25], ['vel', 'A.fm', 0.15]],
  g: { vmode: 2, glide: 0.02 },
  fx: fx(DRV(0.25, 1, 0.6), DRY)
}, { vary: [['L.0.fmr', [1, 2, 0.5, 3]], ['L.0.fm', 0.2, 0.45]] });

A('Synth Bass', ['Upright Bass', 'Finger Bass', 'Jazz Bass', 'Soul Bass', 'Walking Bass'], {
  L: [pluck(Object.assign({ crs: -12, pdec: 1.6, pdmp: 0.62, ppos: 0.22, pexc: 0.35, vs: 0.7 }, lp12(0.5, 0.1, { f1k: 0.6 }), AMP(0.001, 1, 1, 0.12))),
    sine(Object.assign({ crs: -12, lvl: 0.35 }, AMP(0.002, 0.8, 0.3, 0.12)))],
  fx: fx(COMP(-18, 3, 3), REV(0.08, 1.2, 0.35, 1))
}, { vary: [['L.0.pdmp', 0.5, 0.75], ['L.0.pdec', 1, 2.5]], kit: 'bass' });

/* =========================================================== 808 & SUB */
function b808(o) {
  return {
    L: [sine(Object.assign({ vs: 0.35 }, AMP(0.001, o.dec || 2.2, o.sus || 0, 0.2), o.L0 || {}))],
    me: [ME(0, o.pd || 0.055, 0, 0.05)], mods: [['e1', 'A.pitch', o.pa == null ? 0.42 : o.pa]],
    g: { vmode: 2, glide: o.gl == null ? 0.06 : o.gl, gmode: 1 },
    fx: fx(DRV(o.drv == null ? 0.3 : o.drv, o.dt == null ? 1 : o.dt, 0.55), COMP(-14, 3, 2), DRY)
  };
}
const V808 = [
  ['808 Classic', {}], ['808 Long Slide', { dec: 4, gl: 0.09 }], ['808 Short Punch', { dec: 0.7, pa: 0.5 }], ['808 Distorted', { drv: 0.7, dt: 0 }],
  ['808 Boom', { dec: 3, pa: 0.3 }], ['808 Atlanta', { dec: 2.5, drv: 0.4, pa: 0.45 }], ['808 Drill Slide', { dec: 3.2, gl: 0.12, drv: 0.35 }],
  ['808 Clean Sub', { drv: 0, pa: 0.25 }], ['808 Gritty', { drv: 0.55, dt: 3 }], ['808 Hard Knock', { pa: 0.6, pd: 0.04, drv: 0.45 }],
  ['808 Tape', { drv: 0.25 }], ['808 Houston', { dec: 4, gl: 0.1, drv: 0.2 }], ['808 Round', { drv: 0.15, L0: { shp: 0.08 } }],
  ['808 Thump', { dec: 0.9, pa: 0.55, pd: 0.03 }], ['808 Deep Cut', { pa: 0.2, dec: 2.8 }], ['808 Bounce', { dec: 1.2, gl: 0.05 }],
  ['808 Fold', { L0: { fold: 0.35 } }], ['808 Memphis', { dec: 1.8, drv: 0.5, dt: 1 }], ['808 UK Drill', { dec: 3, gl: 0.14, pa: 0.35 }],
  ['808 Brooklyn', { dec: 2.4, drv: 0.6, dt: 0, gl: 0.1 }], ['808 Sustain', { sus: 0.85, dec: 1 }]
];
for (const v of V808) A('808 & Sub', [v[0]], b808(v[1]), { kit: 's808' });

A('808 & Sub', ['Sine Sub', 'Pure Sub', 'Reggae Sub', 'Deep Sub', 'Sub Pulse', 'Garage Sub', 'Roots Sub', 'Sub Triangle'], {
  L: [sine(Object.assign({ vs: 0.3 }, AMP(0.004, 0.5, 1, 0.12)))],
  g: { vmode: 2, glide: 0.02 },
  fx: fx(DRY)
}, { vary: [['L.0.shp', [0, 0, 0.15, 0.33]], ['L.0.sub', [0, 0, 0.2]]], kit: 'bass', over: { 'Sub Triangle': p => { p.L[0].shp = 0.333; } } });

/* =========================================================== STRINGS */
A('Strings', ['String Machine', 'Solina Strings', 'Velvet Strings', 'Seventies Strings', 'Disco Strings', 'Lush Strings'], {
  L: [saw(Object.assign({ uni: 5, det: 14, spr: 0.8, drift: 0.3 }, lp(0.58, 0.05, { f1k: 0.3 }), AMP(0.35, 1, 0.9, 0.9))),
    saw(Object.assign({ crs: 12, lvl: 0.3, uni: 3, det: 10, spr: 0.8 }, lp(0.62, 0.05), AMP(0.4, 1, 0.9, 0.9)))],
  lfo: [LF(0, 5.2, { rtg: 1, fade: 0.8 })], mods: [['l1', '*.pitch', 0.004]],
  fx: fx(CHO(0.6, 0.35, 0.7, 1), REV(0.3, 3.5, 0.7, 0))
}, { vary: [['L.0.f1c', 0.5, 0.65], ['L.1.lvl', 0.15, 0.45]] });

A('Strings', ['Film Strings', 'Legato Strings', 'Slow Bow', 'Epic Strings', 'Sad Violins', 'Dark Cellos'], {
  L: [wt('Strings', 0.4, Object.assign({ uni: 4, det: 10, spr: 0.8, drift: 0.4 }, lp(0.6, 0.08, { f1k: 0.4 }), AMP(0.7, 1, 0.95, 1.3))),
    vox(Object.assign({ lvl: 0.18, uni: 3, det: 8, vow: 0.05, brth: 0.25 }, AMP(0.9, 1, 0.9, 1.3)))],
  lfo: [LF(0, 5, { rtg: 1, fade: 1 })], mods: [['l1', '*.pitch', 0.006], ['vel', 'A.f1c', 0.12]],
  fx: fx(REV(0.38, 4.5, 0.8, 0), EQ(1, 0, 1000, 1))
}, { vary: [['L.0.pos', 0.2, 0.7], ['L.0.aa', 0.4, 1.2]], over: { 'Dark Cellos': p => { p.L[0].crs = -12; p.L[1].crs = -12; }, 'Legato Strings': p => { p.g.vmode = 2; p.g.glide = 0.06; } } });

A('Strings', ['Pizzicato', 'Pizz Ensemble', 'Plucked Cellos', 'Spy Pizz'], {
  L: [pluck(Object.assign({ pdec: 0.45, pdmp: 0.55, ppos: 0.3, pexc: 0.45, uni: 2, det: 10, spr: 0.6, f2t: 'peak', f2c: 0.48, f2r: 0.35, f2k: 0.3 }, lp12(0.62, 0.1), AMP(0.001, 1, 1, 0.15)))],
  fx: fx(REV(0.3, 2.5, 0.6, 0))
}, { vary: [['L.0.pdec', 0.3, 0.7]], kit: 'pluck', over: { 'Plucked Cellos': p => { p.L[0].crs = -12; } } });

A('Strings', ['Staccato Strings', 'Hype Strings', 'Orchestral Stabs', 'Spiccato Run', 'Trap Strings'], {
  L: [wt('Strings', 0.5, Object.assign({ uni: 4, det: 12, spr: 0.8 }, lp(0.62, 0.1), AMP(0.008, 0.22, 0, 0.12))),
    saw(Object.assign({ crs: -12, lvl: 0.3, uni: 2, det: 8 }, lp(0.5, 0.1), AMP(0.008, 0.22, 0, 0.12)))],
  fx: fx(REV(0.3, 2.5, 0.65, 0), COMP(-18, 3, 3))
}, { vary: [['L.0.ad', 0.15, 0.35]] });

/* =========================================================== BRASS & WINDS */
A('Brass & Winds', ['Synth Brass', 'Jump Brass', 'Power Brass', 'Eighties Horns', 'Brass Stab'], {
  L: [saw(Object.assign({ uni: 3, det: 10, spr: 0.6, vs: 0.6 }, lp(0.42, 0.12, { f1v: 0.5 }), F(0.06, 0.6, 0.45, 0.3, 0.32), AMP(0.03, 0.5, 0.9, 0.25)))],
  fx: fx(CHO(0.25, 0.5, 0.5, 0), REV(0.22, 2.2, 0.55, 0))
}, { vary: [['L.0.f1e', 0.25, 0.4], ['L.0.fa', 0.03, 0.12]] });

A('Brass & Winds', ['Soul Horns', 'Horn Section', 'Big Band Hit', 'Funk Section', 'Gospel Horns'], {
  L: [wt('Brass', 0.5, Object.assign({ uni: 3, det: 8, spr: 0.7, vs: 0.6 }, lp(0.55, 0.1, { f1v: 0.6 }), F(0.03, 0.4, 0.6, 0.2, 0.15), AMP(0.02, 0.4, 0.85, 0.2))),
    vox(Object.assign({ lvl: 0.2, vow: 0, brth: 0.2, uni: 2 }, AMP(0.03, 0.4, 0.8, 0.2)))],
  me: [ME(0, 0.12, 0, 0.1)], mods: [['e1', 'A.pitch', -0.02]],
  fx: fx(REV(0.22, 2, 0.5, 1), COMP(-18, 3, 3))
}, { vary: [['L.0.pos', 0.3, 0.8]] });

A('Brass & Winds', ['Breath Flute', 'Pan Flute', 'Wooden Flute', 'Shakuhachi', 'Night Flute', 'Bamboo Flute'], {
  L: [lay('analog', Object.assign({ shp: 0.12, nz: 0.08, vs: 0.5 }, lp12(0.7, 0.1), AMP(0.06, 0.4, 0.95, 0.25))),
    noise(0, Object.assign({ ncol: 0.7, lvl: 0.22 }, bp(0.68, 0.35, { f1k: 1 }), AMP(0.05, 0.25, 0.5, 0.2)))],
  lfo: [LF(0, 5.2, { rtg: 1, fade: 0.4 })], mods: [['l1', '*.pitch', 0.007], ['l1', 'A.lvl', 0.08]],
  g: { vmode: 2, glide: 0.04 },
  fx: fx(REV(0.3, 3, 0.6, 0), DLY(0.12, '1/8.', 0.3))
}, { vary: [['L.1.lvl', 0.12, 0.35], ['L.0.shp', 0.05, 0.25]] });

A('Brass & Winds', ['Clarinet', 'Reed Lead', 'Oboe Air', 'Smoky Sax'], {
  L: [wt('Reed', 0.4, Object.assign({ vs: 0.6 }, lp(0.65, 0.1, { f1v: 0.5 }), AMP(0.03, 0.4, 0.95, 0.2)))],
  lfo: [LF(0, 5, { rtg: 1, fade: 0.5 })], mods: [['l1', 'A.pitch', 0.006]],
  g: { vmode: 2, glide: 0.03 },
  fx: fx(REV(0.25, 2.5, 0.6, 1))
}, { vary: [['L.0.pos', 0.2, 0.7]], over: { 'Smoky Sax': p => { p.L[0].wt = 'Brass'; p.L[0].fold = 0.12; } } });

/* =========================================================== CHOIR & VOX */
A('Choir & Vox', ['Choir Aah', 'Cathedral Choir', 'Gospel Aahs', 'Angel Voices', 'Heaven Choir', 'Dark Choir'], {
  L: [vox(Object.assign({ vow: 0.03, uni: 5, det: 14, spr: 0.85, brth: 0.12 }, lp12(0.78), AMP(0.45, 1, 1, 1.4))),
    vox(Object.assign({ crs: 12, lvl: 0.35, vow: 0.1, fsh: 0.62, uni: 3, det: 12, spr: 0.9, brth: 0.15 }, lp12(0.8), AMP(0.6, 1, 1, 1.4)))],
  lfo: [LF(0, 5, { rtg: 1, fade: 0.7 })], mods: [['l1', '*.pitch', 0.006]],
  fx: fx(REV(0.45, 5, 0.85, 0), CHO(0.2, 0.3, 0.5, 0))
}, { vary: [['L.0.vow', 0, 0.2], ['L.1.crs', [12, 0, 7]]] });

A('Choir & Vox', ['Choir Ooh', 'Soft Oohs', 'Velvet Oohs', 'Boys Choir', 'Chamber Oohs'], {
  L: [vox(Object.assign({ vow: 0.92, uni: 5, det: 12, spr: 0.85, brth: 0.1 }, lp12(0.72), AMP(0.5, 1, 1, 1.4)))],
  lfo: [LF(0, 4.8, { rtg: 1, fade: 0.8 })], mods: [['l1', '*.pitch', 0.006]],
  fx: fx(REV(0.42, 4.5, 0.8, 0))
}, { vary: [['L.0.vow', 0.78, 1], ['L.0.fsh', 0.4, 0.7]], over: { 'Boys Choir': p => { p.L[0].fsh = 0.72; } } });

A('Choir & Vox', ['Vox Chop', 'Vocal Stab', 'Yeah Chop', 'Hey Chop', 'Chopped Soul Vox'], {
  L: [vox(Object.assign({ vow: 0.1, uni: 2, det: 8, spr: 0.4, brth: 0.15 }, AMP(0.005, 0.25, 0, 0.12)))],
  me: [ME(0, 0.25, 0, 0.1)], mods: [['e1', 'A.vow', 0.5]],
  fx: fx(DLY(0.18, '1/8', 0.3), REV(0.25, 2, 0.5, 1))
}, { vary: [['L.0.vow', 0, 0.5], ['mods.0.a', -0.5, 0.6]] });

A('Choir & Vox', ['Robot Choir', 'Vocoder Pad', 'Android Hymn', 'Cyber Voice'], {
  L: [wt('Vocal Pulse', 0.3, Object.assign({ uni: 3, det: 9, spr: 0.7, ring: 0.15, rngr: 2 }, lp(0.7, 0.1), AMP(0.2, 1, 0.9, 1))),
    vox(Object.assign({ lvl: 0.4, vow: 0.3, uni: 3 }, AMP(0.25, 1, 0.9, 1)))],
  lfo: [LF(6, 0.3)], mods: [['l1', 'A.pos', 0.35], ['l1', 'B.vow', 0.3]],
  fx: fx(CHO(0.35, 0.4, 0.5, 0), REV(0.3, 3, 0.6, 0))
}, { vary: [['L.0.ring', 0, 0.35]] });

A('Choir & Vox', ['Grain Choir', 'Spectral Voices', 'Choir Dust', 'Frozen Choir'], {
  L: [gran('Choir Aah', Object.assign({ gpos: 0.5, gspr: 0.3, gsz: 0.6, gden: 0.65, spr: 0.85 }, lp12(0.75), AMP(0.8, 1, 1, 2.5)))],
  lfo: [LF(6, 0.1)], mods: [['l1', 'A.gpos', 0.15]],
  fx: fx(REV(0.5, 6, 0.85, 0))
}, { vary: [['L.0.src', ['Choir Aah', 'Choir Chord', 'Vox']], ['L.0.gsz', 0.45, 0.7]], kit: 'texture', over: { 'Frozen Choir': p => { p.L[0].gscan = 0; p.L[0].gspr = 0.05; } } });

/* =========================================================== ORGANS */
A('Organs', ['Drawbar Organ', 'Gospel Organ', 'Church Tonewheel', 'Jazz Organ', 'Rock Organ', 'Soul Organ', 'Rotary Fast', 'Rotary Slow'], {
  L: [wt('Drawbars', 0.17, Object.assign({ crs: 12, vs: 0.2, f1t: 'off' }, AMP(0.004, 0.2, 1, 0.06))),
    noise(0, Object.assign({ ncol: 0.8, lvl: 0.06 }, hp(0.75, 0.1), AMP(0.001, 0.008, 0, 0.01)))],
  lfo: [LF(0, 6.2)], mods: [['l1', 'A.pan', 0.35]],
  fx: fx(CHO(0.4, 5.5, 0.3, 0), DRV(0.22, 1, 0.6), REV(0.18, 1.8, 0.5, 1))
}, { vary: [['L.0.pos', 0, 0.85], ['fx.drv.amt', 0.1, 0.4]],
  over: { 'Rotary Slow': p => { p.fx.cho.rate = 0.8; p.lfo[0].rate = 0.8; }, 'Rock Organ': p => { p.L[0].pos = 0.66; p.fx.drv.amt = 0.5; }, 'Jazz Organ': p => { p.L[0].pos = 0.17; } } });

A('Organs', ['Pipe Organ', 'Cathedral Pipes', 'Funeral Organ', 'Gothic Organ'], {
  L: [wt('Drawbars', 1, Object.assign({ crs: 12, f1t: 'off' }, AMP(0.08, 0.5, 1, 0.6))),
    wt('Reed', 0.6, Object.assign({ lvl: 0.3 }, lp(0.6, 0.1), AMP(0.1, 0.5, 1, 0.6)))],
  fx: fx(REV(0.45, 6, 0.9, 0))
}, { vary: [['L.0.pos', 0.7, 1]] });

A('Organs', ['Combo Organ', 'Garage Organ', 'Surf Organ', 'Reggae Bubble'], {
  L: [sq(Object.assign({ pw: 0.4 }, lp(0.7, 0.15), AMP(0.003, 0.2, 1, 0.05))),
    saw(Object.assign({ crs: 12, lvl: 0.3 }, lp(0.75, 0.1), AMP(0.003, 0.2, 1, 0.05)))],
  lfo: [LF(0, 6)], mods: [['l1', '*.pitch', 0.006]],
  fx: fx(REV(0.2, 1.6, 0.45, 1), DRV(0.15, 1, 0.7))
}, { vary: [['L.0.pw', 0.25, 0.5]], over: { 'Reggae Bubble': p => { for (const L of p.L) { L.ad = 0.12; L.as = 0; L.ar = 0.05; } } } });

A('Organs', ['House Organ', 'Deep House Organ', 'Rave Organ', 'Bass Organ'], {
  L: [sine(Object.assign({ fm: 0.3, fmr: 2 }, AMP(0.002, 0.5, 0.6, 0.15))),
    sq(Object.assign({ crs: 12, lvl: 0.25, pw: 0.5 }, lp(0.6, 0.1), AMP(0.002, 0.4, 0.5, 0.15)))],
  me: [ME(0, 0.25, 0.3, 0.1)], mods: [['e1', 'A.fm', 0.2]],
  fx: fx(REV(0.18, 1.5, 0.4, 1), DLY(0.12, '1/8.', 0.3))
}, { vary: [['L.0.fmr', [2, 3, 1]]], over: { 'Bass Organ': p => { p.L[0].crs = -12; p.L[1].crs = 0; } } });

/* =========================================================== GUITARS & WORLD */
A('Guitars & World', ['Nylon Guitar', 'Spanish Guitar', 'Bossa Nylon', 'Classical Pick', 'Fingerstyle'], {
  L: [pluck(Object.assign({ pdec: 2.4, pdmp: 0.55, ppos: 0.16, pexc: 0.45, vs: 0.7, f2t: 'peak', f2c: 0.42, f2r: 0.3, f2k: 0 }, lp12(0.7, 0.05), AMP(0.001, 1, 1, 0.3)))],
  fx: fx(REV(0.22, 2.2, 0.55, 1), CHO(0.12, 0.5, 0.3, 0))
}, { vary: [['L.0.pdmp', 0.45, 0.65], ['L.0.ppos', 0.1, 0.25]] });

A('Guitars & World', ['Steel Strings', 'Twelve String', 'Campfire', 'Folk Pick'], {
  L: [pluck(Object.assign({ pdec: 3, pdmp: 0.3, ppos: 0.12, pexc: 0.7, uni: 2, det: 6, spr: 0.6, vs: 0.7 }, lp12(0.82), AMP(0.001, 1, 1, 0.35)))],
  fx: fx(REV(0.22, 2.2, 0.55, 1))
}, { vary: [['L.0.pdmp', 0.2, 0.4], ['L.0.uni', [2, 3, 1]]], over: { 'Twelve String': p => { p.L[0].uni = 3; p.L[0].det = 14; } } });

A('Guitars & World', ['Clean Electric', 'Neo Soul Guitar', 'Chorus Guitar', 'R&B Guitar', 'Jazz Box'], {
  L: [pluck(Object.assign({ pdec: 2.5, pdmp: 0.45, ppos: 0.1, pexc: 0.55, vs: 0.7 }, lp12(0.66, 0.1), AMP(0.001, 1, 1, 0.25)))],
  fx: fx(CHO(0.35, 0.6, 0.5, 0), DRV(0.12, 1, 0.6), REV(0.2, 2, 0.5, 1), DLY(0.1, '1/8.', 0.3))
}, { vary: [['L.0.f1c', 0.58, 0.72]], over: { 'Jazz Box': p => { p.L[0].pdmp = 0.65; p.L[0].f1c = 0.52; } } });

A('Guitars & World', ['Koto', 'Harp', 'Celtic Harp', 'Zither', 'Guzheng'], {
  L: [pluck(Object.assign({ pdec: 3, pdmp: 0.25, ppos: 0.08, pexc: 0.75, vs: 0.6 }, lp12(0.85), AMP(0.001, 1, 1, 0.6)))],
  fx: fx(REV(0.3, 3, 0.65, 0), DLY(0.1, '1/8', 0.25))
}, { vary: [['L.0.ppos', 0.05, 0.15], ['L.0.pdmp', 0.15, 0.35]], over: { 'Harp': p => { p.L[0].pexc = 0.4; p.L[0].ppos = 0.25; }, 'Celtic Harp': p => { p.L[0].pexc = 0.5; p.L[0].ppos = 0.2; } } });

A('Guitars & World', ['Kalimba', 'Thumb Piano', 'Mbira', 'Kalimba Dreams', 'Afro Kalimba'], {
  L: [modal('Kalimba', Object.assign({ mdec: 1.6, mbri: 0.5, mhard: 0.6 }, AMP(0.001, 1, 1, 0.5)))],
  fx: fx(REV(0.25, 2.5, 0.55, 1), DLY(0.12, '1/8.', 0.3))
}, { vary: [['L.0.mbri', 0.4, 0.6], ['L.0.mdec', 1, 2.5]], kit: 'bell' });

/* =========================================================== ARPS & SEQUENCES */
A('Arps & Sequences', ['Arp Classic', 'Rising Arp', 'Octave Runner', 'Neon Arp', 'Synthwave Arp', 'Night Drive'], {
  L: [saw(Object.assign({ uni: 2, det: 9, spr: 0.5 }, lp(0.38, 0.25), F(0.001, 0.18, 0, 0.1, 0.38), AMP(0.001, 0.3, 0, 0.15)))],
  arp: ARP('up', '1/16', 2, 0.5),
  fx: fx(DLY(0.25, '1/8.', 0.4), REV(0.22, 2.5, 0.6, 0))
}, { vary: [['arp.mode', ['up', 'updown', 'down', 'order']], ['arp.oct', [1, 2, 3]], ['L.0.shp', [0.667, 1]]] });

A('Arps & Sequences', ['Bell Sequence', 'Glass Steps', 'Crystal Ladder', 'Chime Arp', 'Twinkle Arp'], {
  L: [modal('Glock', Object.assign({ mdec: 1, mbri: 0.55, mhard: 0.55 }, AMP(0.001, 1, 1, 0.3)))],
  arp: ARP('updown', '1/8', 2, 0.8),
  fx: fx(DLY(0.3, '1/8.', 0.45, { pp: 1 }), REV(0.35, 4, 0.75, 0))
}, { vary: [['L.0.mdl', ['Glock', 'Bell', 'Crystal', 'Music Box']], ['arp.div', ['1/8', '1/16', '1/8T']]], kit: 'bell' });

A('Arps & Sequences', ['Random Robot', 'Computer Talk', 'Data Stream', 'Glitch Arp', 'Bleep Machine'], {
  L: [sq(Object.assign({ pw: 0.3 }, lp(0.5, 0.45), AMP(0.001, 0.15, 0, 0.08)))],
  lfo: [LS(5, '1/16')], mods: [['l1', 'A.f1c', 0.25]],
  arp: ARP('random', '1/16', 3, 0.5),
  fx: fx(DLY(0.2, '1/16', 0.35), REV(0.15, 1.5, 0.4, 1))
}, { vary: [['L.0.f1r', 0.3, 0.65], ['arp.div', ['1/16', '1/32', '1/16T']]] });

A('Arps & Sequences', ['Seq Bass', 'Acid Line', 'Berlin Seq', 'Rolling Bass', 'Pulse Bass Seq'], {
  L: [saw(Object.assign({}, ladderF(0.3, 0.55, { f1v: 0.5 }), F(0.001, 0.15, 0, 0.1, 0.4), AMP(0.001, 0.25, 0, 0.08)))],
  g: { vmode: 1 },
  arp: ARP('order', '1/16', 1, 0.45, { vel: [1, 0.6, 0.8, 0.6, 1, 0.6, 0.8, 0.6, 1, 0.6, 0.8, 0.6, 1, 0.6, 0.8, 0.6], tr: [0, 0, 12, 0, 0, 12, 0, 0, 0, 0, 12, 0, 0, 7, 12, 0] }),
  fx: fx(DRV(0.3, 1, 0.6), DLY(0.15, '1/8.', 0.3), REV(0.08, 1.2, 0.3, 1))
}, { vary: [['L.0.f1r', 0.4, 0.75], ['L.0.f1c', 0.22, 0.36]], kit: 'bass', over: { 'Acid Line': p => { p.L[0].f1r = 0.85; p.L[0].f1e = 0.55; p.fx.drv.amt = 0.5; } } });

A('Arps & Sequences', ['Trap Bell Arp', 'Triplet Chime Arp', 'Hi-Hat Bells', 'Drill Arp'], {
  L: [modal('Bell', Object.assign({ crs: 12, mdec: 1.2, mbri: 0.6, mhard: 0.6 }, AMP(0.001, 1, 1, 0.3)))],
  arp: ARP('updown', '1/16T', 2, 0.6),
  fx: fx(DLY(0.22, '1/8T', 0.35), REV(0.3, 3, 0.65, 0))
}, { vary: [['L.0.mdl', ['Bell', 'Glock', 'Crystal']]], kit: 'bell' });

A('Arps & Sequences', ['Chord Rhythm', 'House Stabs', 'Offbeat Chords', 'Skank Stabs', 'Gospel Stabs'], {
  L: [saw(Object.assign({ uni: 3, det: 12, spr: 0.6 }, lp(0.5, 0.15), F(0.001, 0.15, 0.2, 0.1, 0.25), AMP(0.001, 0.25, 0, 0.1))),
    sq(Object.assign({ crs: 12, lvl: 0.25 }, lp(0.6, 0.1), AMP(0.001, 0.2, 0, 0.1)))],
  arp: ARP('chord', '1/8', 1, 0.4, { vel: [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1] }),
  fx: fx(DLY(0.15, '1/8.', 0.3), REV(0.25, 2.5, 0.6, 0), CHO(0.25, 0.5, 0.5, 0))
}, { vary: [['arp.vel', [[1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1], [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1], [1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0]]]] });

A('Arps & Sequences', ['Ripple', 'Cascade', 'Waterfall', 'Fireflies'], {
  L: [tri(Object.assign({ f1t: 'lp12', f1c: 0.7 }, AMP(0.001, 0.2, 0, 0.15))),
    modal('Crystal', Object.assign({ crs: 12, lvl: 0.3, mdec: 0.6 }, AMP(0.001, 1, 1, 0.2)))],
  arp: ARP('updown', '1/32', 3, 0.6),
  fx: fx(DLY(0.25, '1/16', 0.4, { pp: 1 }), REV(0.35, 4, 0.75, 0))
}, { vary: [['arp.oct', [2, 3, 4]], ['arp.mode', ['updown', 'down', 'random']]] });

/* =========================================================== PULSES & GATES */
A('Pulses & Gates', ['Gated Pad', 'Pulse Pad', 'Heartbeat Pad', 'Morse Pad', 'Stutter Pad', 'Chopped Clouds'], {
  L: [saw(Object.assign({ uni: 5, det: 18, spr: 0.8 }, lp(0.6, 0.12), AMP(0.2, 1, 0.9, 1.2))),
    wt('Glass', 0.3, Object.assign({ crs: 12, lvl: 0.3, uni: 2 }, AMP(0.3, 1, 0.9, 1.2)))],
  fx: fx(GATE([1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1], '1/16', 1, 0.15), REV(0.3, 3.5, 0.7, 0), DLY(0.15, '1/8.', 0.35))
}, { vary: [['fx.gate.pat', GATE_PATS], ['fx.gate.div', ['1/16', '1/8', '1/16T']]], over: { 'Heartbeat Pad': p => { p.fx.gate.pat = [1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0]; p.fx.gate.div = '1/16'; } } });

A('Pulses & Gates', ['Filter Pulse', 'Pumping Pad', 'Sidechain Pad', 'Throb', 'Breathing Pulse'], {
  L: [saw(Object.assign({ uni: 4, det: 15, spr: 0.7 }, lp(0.45, 0.25), AMP(0.1, 1, 0.9, 1)))],
  lfo: [LS(2, '1/4')], mods: [['l1', 'A.f1c', 0.22], ['l1', 'A.lvl', 0.55]],
  fx: fx(REV(0.25, 2.5, 0.6, 0))
}, { vary: [['lfo.0.div', ['1/4', '1/8', '1/8T']], ['lfo.0.shp', [2, 0, 1]]] });

A('Pulses & Gates', ['Bubbles', 'Ticking', 'Clockwork Pulse', 'Raindrop Pulse'], {
  L: [sine(Object.assign({ fm: 0.1, fmr: 2 }, lp(0.6, 0.3), AMP(0.05, 1, 0.9, 0.6)))],
  me: [ME(0.005, 0.12, 0, 0.1, 1)], mods: [['e1', 'A.f1c', 0.3], ['e1', 'A.fm', 0.3], ['e1', 'A.lvl', 0.5]],
  fx: fx(DLY(0.25, '1/8.', 0.45), REV(0.3, 3, 0.65, 0))
}, { vary: [['me.0.d', 0.06, 0.25]] });

A('Pulses & Gates', ['Pulse Bass', 'Train Bass', 'Motor Bass', 'Locomotive'], {
  L: [saw(Object.assign({ sub: 0.4 }, lp(0.35, 0.3), AMP(0.005, 0.5, 1, 0.1)))],
  g: { vmode: 2 },
  fx: fx(GATE([1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0], '1/16', 1, 0.1), DRV(0.25, 1, 0.6), DRY)
}, { vary: [['fx.gate.pat', [[1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0], [1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0], [0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1]]]], kit: 'bass' });

/* =========================================================== DRONES */
A('Drones', ['Dark Matter', 'Black Hole', 'Earth Hum', 'Leviathan', 'Monolith'], {
  L: [saw(Object.assign({ crs: -12, uni: 5, det: 30, spr: 0.8, drift: 0.6 }, lp(0.32, 0.4), AMP(3, 2, 1, 6))),
    noise(0, Object.assign({ ncol: 0.3, kt: 0, lvl: 0.2 }, bp(0.4, 0.4, { f1k: 0 }), AMP(4, 2, 1, 6))),
    sine(Object.assign({ crs: -24, lvl: 0.35 }, AMP(3, 2, 1, 6)))],
  lfo: [LF(6, 0.05), LF(0, 0.03)], mods: [['l1', 'A.f1c', 0.12], ['l2', 'B.f1c', 0.15]],
  fx: fx(REV(0.55, 9, 0.9, 3), DLY(0.2, '1/2', 0.5))
}, { vary: [['L.0.f1c', 0.25, 0.4], ['L.0.det', 20, 45]] });

A('Drones', ['Overtone Drone', 'Tanpura Drone', 'Throat Song', 'Sun Drone', 'Harmonic Field'], {
  L: [wt('Harmonic Rise', 0.3, Object.assign({ uni: 3, det: 6, spr: 0.8, f1t: 'off' }, AMP(2.5, 2, 1, 5))),
    vox(Object.assign({ crs: -12, lvl: 0.35, vow: 0.5, uni: 2 }, AMP(3, 2, 1, 5)))],
  lfo: [LF(1, 0.04), LF(6, 0.07)], mods: [['l1', 'A.pos', 0.4], ['l2', 'B.vow', 0.45]],
  fx: fx(REV(0.5, 8, 0.9, 3), CHO(0.2, 0.2, 0.5, 0))
}, { vary: [['L.0.wt', ['Harmonic Rise', 'Odd Rise', 'Octave Stack']]] });

A('Drones', ['Bowed Gong', 'Metal Drone', 'Ship Hull', 'Iron Lung', 'Rust Field'], {
  L: [modal('Gong', Object.assign({ mbow: 0.8, mdec: 6, mbri: 0.5, mhard: 0.3, spr: 0.7 }, AMP(2, 2, 1, 5))),
    noise(4, Object.assign({ ncol: 0.3, kt: 0, lvl: 0.15 }, AMP(3, 2, 1, 5)))],
  fx: fx(REV(0.55, 9, 0.9, 3))
}, { vary: [['L.0.mdl', ['Gong', 'Plate', 'Bell', 'Drum']]] });

A('Drones', ['Light Drone', 'Glacier', 'Aurora Drone', 'Morning Star'], {
  L: [wt('Glass', 0.5, Object.assign({ uni: 5, det: 12, spr: 0.9 }, lp12(0.75), AMP(3, 2, 1, 6))),
    modal('Crystal', Object.assign({ crs: 12, lvl: 0.3, mbow: 0.6, mdec: 5 }, AMP(3, 2, 1, 6)))],
  lfo: [LF(0, 0.04)], mods: [['l1', 'A.pos', 0.35]],
  fx: fx(REV(0.6, 10, 0.95, 3), DLY(0.2, '1/4.', 0.5))
}, { vary: [['L.0.wt', ['Glass', 'Ghost', 'Octave Stack']]] });

/* =========================================================== TEXTURES & NOISE */
A('Textures & Noise', ['Vinyl Crackle', 'Dusty Record', 'Needle Drop', 'Old Shellac'], {
  L: [noise(1, Object.assign({ ncol: 0.4, kt: 0, spr: 0.4 }, lp12(0.8), AMP(0.02, 1, 1, 0.5))),
    noise(0, Object.assign({ ncol: 0.55, kt: 0, lvl: 0.12 }, bp(0.75, 0.1, { f1k: 0 }), AMP(0.02, 1, 1, 0.5)))],
  fx: fx(DRY)
}, { vary: [['L.0.ncol', 0.25, 0.7]] });

A('Textures & Noise', ['Rain Window', 'Heavy Rain', 'Drizzle', 'Thunder Night'], {
  L: [noise(2, Object.assign({ ncol: 0.5, kt: 0, spr: 0.7 }, AMP(0.5, 1, 1, 1.5))),
    noise(0, Object.assign({ ncol: 0.2, kt: 0, lvl: 0.25 }, lp(0.3, 0.1, { f1k: 0 }), AMP(1, 1, 1, 2)))],
  fx: fx(REV(0.3, 3, 0.7, 0))
}, { vary: [['L.0.ncol', 0.3, 0.9]], over: { 'Heavy Rain': p => { p.L[0].ncol = 0.95; }, 'Drizzle': p => { p.L[0].ncol = 0.15; p.L[1].lvl = 0.1; }, 'Thunder Night': p => { p.L[1].lvl = 0.6; p.L[1].f1c = 0.22; } } });

A('Textures & Noise', ['Wind Tunnel', 'Breeze', 'Mountain Wind', 'Night Air'], {
  L: [noise(4, Object.assign({ ncol: 0.5, kt: 0, spr: 0.8 }, AMP(1, 1, 1, 2)))],
  fx: fx(REV(0.4, 5, 0.8, 0))
}, { vary: [['L.0.ncol', 0.2, 0.8]] });

A('Textures & Noise', ['Radio Static', 'Tape Hiss', 'Shortwave', 'Transmission'], {
  L: [noise(3, Object.assign({ ncol: 0.3 }, bp(0.62, 0.4), AMP(0.05, 1, 1, 0.4))),
    noise(0, Object.assign({ ncol: 0.8, kt: 0, lvl: 0.3 }, bp(0.7, 0.3, { f1k: 0 }), AMP(0.05, 1, 1, 0.4)))],
  lfo: [LF(5, 6)], mods: [['l1', 'A.f1c', 0.15]],
  fx: fx(LOFI(1, { bits: 7, rate: 3, wow: 0.1, flut: 0.1, noise: 0.3, tone: 0.6 }), REV(0.15, 1.5, 0.4, 1))
}, { vary: [['lfo.0.rate', 2, 12]] });

A('Textures & Noise', ['Metal Grains', 'Broken Glass', 'Sand', 'Insects', 'Electric Field', 'Gravel'], {
  L: [gran('Metal', Object.assign({ gpos: 0.3, gspr: 0.6, gsz: 0.25, gden: 0.75, gpit: 0.5, spr: 0.9, kt: 0 }, AMP(0.3, 1, 1, 1.5)))],
  fx: fx(REV(0.35, 4, 0.75, 0), DLY(0.15, '1/16', 0.3))
}, { vary: [['L.0.src', ['Metal', 'Bells', 'Glass Pad', 'Kalimba', 'Rain']], ['L.0.gsz', 0.1, 0.35], ['L.0.gden', 0.6, 0.9]] });

/* =========================================================== GRANULAR */
A('Granular', ['Grain Strings', 'Time Stretch', 'Frozen Strings', 'Stretched Orchestra'], {
  L: [gran('Strings', Object.assign({ gpos: 0.4, gspr: 0.15, gsz: 0.65, gden: 0.6, spr: 0.8, gscan: 0.1 }, lp12(0.75), AMP(0.6, 1, 1, 2.5)))],
  fx: fx(REV(0.4, 5, 0.8, 0))
}, { vary: [['L.0.gscan', [0, 0.05, 0.15, -0.05]], ['L.0.gsz', 0.55, 0.75]], over: { 'Frozen Strings': p => { p.L[0].gscan = 0; } } });

A('Granular', ['Grain Piano', 'Piano Dust', 'Spectral Piano', 'Memory Piano'], {
  L: [gran('Felt Piano', Object.assign({ gpos: 0.1, gspr: 0.12, gsz: 0.55, gden: 0.6, spr: 0.8, gscan: 0.08 }, lp12(0.7), AMP(0.3, 1, 1, 3)))],
  fx: fx(REV(0.45, 6, 0.85, 0), DLY(0.15, '1/4', 0.4))
}, { vary: [['L.0.gpos', 0.05, 0.4]] });

A('Granular', ['Chord Cloud', 'Smeared Chords', 'Grain Stabs', 'Liquid Chords'], {
  L: [gran('Synth Chord', Object.assign({ gpos: 0.3, gspr: 0.2, gsz: 0.5, gden: 0.65, spr: 0.85 }, lp(0.7, 0.1), AMP(0.2, 1, 1, 2)))],
  fx: fx(REV(0.4, 4.5, 0.8, 0), CHO(0.3, 0.4, 0.5, 0))
}, { vary: [['L.0.src', ['Synth Chord', 'Organ Chord', 'Choir Chord']]] });

A('Granular', ['Sparkle Grains', 'Fairy Dust', 'Glitter Field', 'Star Field'], {
  L: [gran('Kalimba', Object.assign({ crs: 12, gpos: 0.4, gspr: 0.8, gsz: 0.2, gden: 0.7, spr: 1 }, AMP(0.3, 1, 1, 2.5)))],
  fx: fx(REV(0.5, 6, 0.85, 0), DLY(0.25, '1/8.', 0.45))
}, { vary: [['L.0.src', ['Kalimba', 'Bells', 'Glass Pad']], ['L.0.gsz', 0.12, 0.3]] });

A('Granular', ['Reverse Voices', 'Vox Particles', 'Backwards Choir', 'Whisper Grains'], {
  L: [gran('Vox', Object.assign({ gpos: 0.4, gspr: 0.5, gsz: 0.55, gden: 0.55, grev: 0.6, spr: 0.8 }, AMP(0.4, 1, 1, 2)))],
  fx: fx(REV(0.45, 5, 0.8, 0), DLY(0.15, '1/4', 0.4))
}, { vary: [['L.0.grev', 0.3, 0.9], ['L.0.src', ['Vox', 'Choir Aah']]], over: { 'Backwards Choir': p => { p.L[0].src = 'Choir Chord'; p.L[0].grev = 1; } } });

/* =========================================================== SWEEPS & FX */
A('Sweeps & FX', ['Uplifter', 'Tension Riser', 'Noise Riser', 'Hyper Riser', 'Spin Up'], {
  L: [noise(0, Object.assign({ ncol: 0.7, kt: 0, spr: 0.8 }, bp(0.3, 0.4, { f1k: 0 }), AMP(0.01, 1, 1, 0.5))),
    saw(Object.assign({ uni: 5, det: 25, spr: 0.8, lvl: 0.5 }, lp(0.3, 0.2), AMP(0.01, 1, 1, 0.5)))],
  me: [ME(6, 1, 1, 0.5)], mods: [['e1', 'A.f1c', 0.55], ['e1', 'B.f1c', 0.5], ['e1', 'B.pitch', 0.5], ['e1', '*.lvl', 0.3]],
  fx: fx(REV(0.45, 5, 0.8, 0), DLY(0.2, '1/16', 0.45))
}, { vary: [['me.0.a', 3, 10]], over: { 'Noise Riser': p => { p.L[1].on = 0; } } });

A('Sweeps & FX', ['Impact', 'Downlifter', 'Sub Drop', 'Boom Hit', 'Crash Down'], {
  L: [sine(Object.assign({ vs: 0.2 }, AMP(0.001, 3, 0, 1))),
    noise(0, Object.assign({ ncol: 0.6, kt: 0, lvl: 0.5 }, lp(0.8, 0.1, { f1k: 0 }), F(0.001, 1.5, 0, 1, -0.5), AMP(0.001, 2, 0, 1)))],
  me: [ME(0.001, 2.5, 0, 1)], mods: [['e1', 'A.pitch', 0.5]],
  fx: fx(DRV(0.3, 0, 0.6), REV(0.5, 6, 0.85, 0))
}, { vary: [['me.0.d', 1, 4]], over: { 'Sub Drop': p => { p.L[1].on = 0; p.me[0].d = 3; } } });

A('Sweeps & FX', ['Laser', 'Zap', 'Pew', 'Ray Gun', 'Blaster'], {
  L: [lay('analog', Object.assign({ shp: 0.4, f1t: 'off' }, AMP(0.001, 0.25, 0, 0.1)))],
  me: [ME(0, 0.15, 0, 0.1)], mods: [['e1', 'A.pitch', 1.5]],
  fx: fx(DLY(0.2, '1/16', 0.3), REV(0.15, 1.5, 0.4, 1))
}, { vary: [['me.0.d', 0.05, 0.3], ['mods.0.a', 0.8, 2]] });

A('Sweeps & FX', ['Siren', 'Dub Siren', 'Alarm', 'Air Raid'], {
  L: [lay('analog', Object.assign({ shp: 0.5 }, lp(0.7, 0.2), AMP(0.01, 1, 1, 0.3)))],
  lfo: [LF(1, 0.8)], mods: [['l1', 'A.pitch', 0.25]],
  fx: fx(DLY(0.35, '1/8.', 0.55), REV(0.25, 2.5, 0.6, 0))
}, { vary: [['lfo.0.rate', 0.3, 3], ['lfo.0.shp', [1, 2, 4]]] });

A('Sweeps & FX', ['Filter Sweep', 'Jet Sweep', 'Phaser Sweep', 'Ocean Sweep', 'Slow Opening'], {
  L: [saw(Object.assign({ uni: 5, det: 20, spr: 0.8 }, lp(0.15, 0.4), F(4, 4, 1, 3, 0.65), AMP(0.5, 1, 1, 3)))],
  fx: fx(REV(0.4, 5, 0.8, 0))
}, { vary: [['L.0.fa', 2, 8]], over: { 'Jet Sweep': p => { Object.assign(p.fx.cho, { on: 1, mode: 2, mix: 0.6, rate: 0.12, depth: 0.9, fb: 0.7 }); }, 'Ocean Sweep': p => { p.L[0].nz = 0.4; } } });

/* =========================================================== TRAP & DRILL */
A('Trap & Drill', ['Trap Bell', 'Dark Bells', 'Money Bells', 'Ice Chain Bells', 'Plugg Bell', 'Mansion Bells'], {
  L: [modal('Bell', Object.assign({ crs: 12, mdec: 2.2, mbri: 0.6, mhard: 0.65 }, AMP(0.001, 1, 1, 0.6))),
    sine(Object.assign({ crs: 12, fmr: 3.5, lvl: 0.35 }, AMP(0.001, 1.5, 0, 0.5)))],
  me: [ME(0, 0.6, 0, 0.3)], mods: [['e1', 'B.fm', 0.4]],
  fx: fx(DLY(0.25, '1/8T', 0.35), REV(0.35, 3.5, 0.7, 0))
}, { vary: [['L.0.mdl', ['Bell', 'Glock', 'Crystal']], ['L.1.fmr', [3.5, 7, 2.76]]] });

A('Trap & Drill', ['Dark Piano', 'Haunted Keys', 'Night Keys', 'Cold Piano', 'Villain Keys', 'Eerie Upright'], {
  L: [modal('Piano', Object.assign({ mdec: 4, mbri: 0.3, mhard: 0.4, minh: 0.6 }, lp12(0.55, 0.05, { f1v: 0.5 }), AMP(0.001, 1, 1, 0.6)))],
  fx: fx(REV(0.42, 4.5, 0.8, 0), DLY(0.15, '1/8.', 0.35), LOFI(0.3, { bits: 12, rate: 1, wow: 0.25, flut: 0.1, noise: 0.1, tone: 0.55 }))
}, { vary: [['L.0.mbri', 0.22, 0.4]], kit: 'keys' });

A('Trap & Drill', ['Trap Flute', 'Smoke Flute', 'Desert Flute', 'Pan Pipe Trap', 'Purple Flute'], {
  L: [lay('analog', Object.assign({ shp: 0.15, nz: 0.06 }, lp12(0.68, 0.1), AMP(0.05, 0.4, 0.95, 0.3))),
    noise(0, Object.assign({ ncol: 0.7, lvl: 0.25 }, bp(0.7, 0.35, { f1k: 1 }), AMP(0.04, 0.25, 0.45, 0.2)))],
  lfo: [LF(0, 5.6, { rtg: 1, fade: 0.3 })], mods: [['l1', '*.pitch', 0.008]],
  g: { vmode: 2, glide: 0.05 },
  fx: fx(DLY(0.25, '1/8.', 0.4), REV(0.35, 3.5, 0.7, 0))
}, { vary: [['L.1.lvl', 0.15, 0.35]], kit: 'lead' });

A('Trap & Drill', ['Drill Strings', 'Drill Choir', 'UK Drill Bells', 'Slide Strings', 'Menace'], {
  L: [wt('Strings', 0.5, Object.assign({ uni: 4, det: 12, spr: 0.8 }, lp(0.6, 0.1), AMP(0.01, 0.3, 0.3, 0.3))),
    vox(Object.assign({ lvl: 0.3, vow: 0.05, uni: 3 }, AMP(0.02, 0.4, 0.4, 0.3)))],
  g: { glide: 0.06 },
  fx: fx(REV(0.35, 3.5, 0.7, 0), DLY(0.15, '1/8T', 0.3))
}, { vary: [['L.1.lvl', 0.15, 0.45]], kit: 'strings', over: { 'UK Drill Bells': p => { p.L[1] = Object.assign(p.L[1], { mode: 'modal', mdl: 'Bell', crs: 12, mdec: 1.5, mbri: 0.6, mhard: 0.6, lvl: 0.5, as: 1, ar: 0.5 }); }, 'Drill Choir': p => { p.L[1].lvl = 0.6; p.L[0].lvl = 0.45; } } });

A('Trap & Drill', ['Rage Lead', 'Rage Synth', 'Plugg Lead', 'Pluggnb Keys', 'Sad Pluggnb'], {
  L: [saw(Object.assign({ uni: 7, det: 22, spr: 0.7 }, lp(0.72, 0.1), AMP(0.002, 0.6, 0.8, 0.3))),
    sq(Object.assign({ crs: 12, lvl: 0.2 }, AMP(0.002, 0.4, 0.6, 0.3)))],
  lfo: [LF(0, 5.5, { rtg: 1, fade: 0.3 })], mods: [['l1', '*.pitch', 0.01]],
  fx: fx(DRV(0.3, 0, 0.65), DLY(0.25, '1/8.', 0.35), REV(0.25, 2.5, 0.6, 0))
}, { vary: [['L.0.det', 15, 30]], kit: 'lead', over: { 'Pluggnb Keys': p => { for (const L of p.L) { L.ad = 1.2; L.as = 0.2; L.ar = 0.5; } p.L[0].uni = 3; p.L[0].f1c = 0.55; }, 'Sad Pluggnb': p => { for (const L of p.L) { L.ad = 1.5; L.as = 0.3; } p.L[0].f1c = 0.5; p.fx.rev.mix = 0.4; } } });

A('Trap & Drill', ['Astro Pad', 'Cactus Haze', 'Utopia Pad', 'Rodeo Night', 'Psychedelic Haze'], {
  L: [saw(Object.assign({ uni: 5, det: 16, spr: 0.85, drift: 0.5 }, lp(0.55, 0.15), AMP(0.6, 1, 0.9, 2.5))),
    wt('Vowel', 0.4, Object.assign({ lvl: 0.35, uni: 3, det: 12, spr: 0.9 }, AMP(0.8, 1, 0.9, 2.5)))],
  lfo: [LF(6, 0.15)], mods: [['l1', 'B.pos', 0.3], ['l1', 'A.f1c', 0.06]],
  fx: fx(CHO(0.4, 0.35, 0.6, 1), PHA(0.25, 0.12, 0.6, 0.3), REV(0.45, 5.5, 0.85, 0), DLY(0.2, '1/4.', 0.4))
}, { vary: [['L.0.f1c', 0.48, 0.62]], kit: 'pad' });

/* =========================================================== SOUL & BOOM BAP */
A('Soul & Boom Bap', ['Soul Rhodes', 'Seventies Soul', 'Detroit Keys', 'Philly Soul', 'Velvet Rope'], {
  L: [modal('Tine', Object.assign({ mdec: 5.5, mbri: 0.45, mhard: 0.5, vs: 0.7 }, lp12(0.62, 0.08, { f1v: 0.5 }), AMP(0.001, 1, 1, 0.35))),
    sine(Object.assign({ lvl: 0.3 }, AMP(0.002, 4.5, 0, 0.3)))],
  lfo: [LF(0, 4.2)], mods: [['l1', '*.pan', 0.55]],
  fx: fx(DRV(0.2, 1, 0.6), REV(0.2, 2, 0.5, 1), LOFI(0.35, { bits: 12, rate: 1, wow: 0.2, flut: 0.15, noise: 0.1, tone: 0.6 }))
}, { vary: [['lfo.0.rate', 3, 6]] });

A('Soul & Boom Bap', ['Boom Bap Horns', 'Dusty Horn Stab', 'Crate Horns', 'Sampled Brass'], {
  L: [wt('Brass', 0.55, Object.assign({ uni: 3, det: 9, spr: 0.7 }, lp(0.55, 0.1), F(0.01, 0.3, 0.5, 0.2, 0.15), AMP(0.01, 0.35, 0.5, 0.2)))],
  fx: fx(LOFI(0.85, { bits: 10, rate: 2, wow: 0.3, flut: 0.2, noise: 0.2, tone: 0.5 }), REV(0.2, 1.8, 0.45, 1), COMP(-18, 3, 3))
}, { vary: [['L.0.pos', 0.4, 0.8]], kit: 'brass' });

A('Soul & Boom Bap', ['Jazz Vibes', 'Cocktail Vibes', 'Smoky Vibes'], {
  L: [modal('Vibes', Object.assign({ mdec: 3, mbri: 0.4, mhard: 0.4 }, AMP(0.001, 1, 1, 0.6)))],
  lfo: [LF(0, 5)], mods: [['l1', '*.lvl', 0.2]],
  fx: fx(LOFI(0.5, { bits: 12, rate: 1.5, wow: 0.3, flut: 0.15, noise: 0.15, tone: 0.55 }), REV(0.25, 2.2, 0.55, 1))
}, { vary: [['L.0.mbri', 0.3, 0.5]], kit: 'bell' });

A('Soul & Boom Bap', ['Smoky Organ', 'Blue Room Organ', 'Church Basement Organ'], {
  L: [wt('Drawbars', 0.3, Object.assign({ crs: 12, f1t: 'off' }, AMP(0.004, 0.2, 1, 0.08)))],
  lfo: [LF(0, 5.5)], mods: [['l1', 'A.pan', 0.3]],
  fx: fx(CHO(0.35, 5, 0.3, 0), DRV(0.25, 1, 0.55), LOFI(0.7, { bits: 11, rate: 1.5, wow: 0.3, flut: 0.2, noise: 0.2, tone: 0.5 }), REV(0.2, 1.8, 0.45, 1))
}, { vary: [['L.0.pos', 0.1, 0.5]], kit: 'organ' });

A('Soul & Boom Bap', ['Soul Choir Chop', 'Gospel Chop', 'Church Chop', 'Sample Choir'], {
  L: [vox(Object.assign({ vow: 0.05, uni: 4, det: 12, spr: 0.8, brth: 0.15 }, AMP(0.01, 0.4, 0.3, 0.2)))],
  fx: fx(LOFI(0.8, { bits: 10, rate: 2, wow: 0.3, flut: 0.2, noise: 0.2, tone: 0.5 }), REV(0.3, 2.5, 0.6, 1))
}, { vary: [['L.0.vow', 0, 0.3]], kit: 'vox' });

A('Soul & Boom Bap', ['Wonky Synth', 'Drunk Keys', 'Off Grid Lead', 'Wobbly Moog', 'Lazy Bass'], {
  L: [lay('analog', Object.assign({ shp: 0.5, drift: 1, uni: 2, det: 8, spr: 0.3 }, ladderF(0.48, 0.3), F(0.001, 0.4, 0.4, 0.3, 0.2), AMP(0.005, 0.6, 0.7, 0.25)))],
  lfo: [LF(6, 0.6)], mods: [['l1', '*.pitch', 0.012]],
  g: { glide: 0.05 },
  fx: fx(LOFI(0.7, { bits: 11, rate: 1.5, wow: 0.5, flut: 0.3, noise: 0.15, tone: 0.55 }), REV(0.15, 1.6, 0.4, 1))
}, { vary: [['L.0.shp', 0.3, 0.8]], kit: 'lead', over: { 'Lazy Bass': p => { p.L[0].crs = -12; p.g.vmode = 2; p.fx.rev.mix = 0.04; } } });

A('Soul & Boom Bap', ['Soul Strings', 'Sample Strings', 'Crate Strings', 'Dusty Orchestra'], {
  L: [wt('Strings', 0.45, Object.assign({ uni: 4, det: 12, spr: 0.8 }, lp(0.55, 0.08), AMP(0.25, 1, 0.9, 0.8)))],
  lfo: [LF(0, 5, { rtg: 1, fade: 0.6 })], mods: [['l1', '*.pitch', 0.005]],
  fx: fx(LOFI(0.8, { bits: 11, rate: 2, wow: 0.35, flut: 0.2, noise: 0.2, tone: 0.5 }), CHO(0.3, 0.4, 0.5, 1), REV(0.25, 2.5, 0.6, 1))
}, { kit: 'strings' });

/* =========================================================== CINEMATIC */
A('Cinematic', ['Braam', 'Tension Brass', 'Titan Horn', 'Doom Hit'], {
  L: [saw(Object.assign({ crs: -12, uni: 5, det: 15, spr: 0.7 }, lp(0.35, 0.25), F(0.2, 2, 0.6, 2, 0.25), AMP(0.15, 2, 0.9, 3))),
    sq(Object.assign({ crs: -24, lvl: 0.5 }, lp(0.3, 0.1), AMP(0.1, 2, 0.9, 3)))],
  fx: fx(DRV(0.45, 1, 0.5), REV(0.5, 6, 0.9, 0))
}, { vary: [['L.0.f1c', 0.28, 0.42]] });

A('Cinematic', ['Tremolo Strings', 'Suspense', 'Dread', 'Creeping Strings'], {
  L: [wt('Strings', 0.4, Object.assign({ uni: 4, det: 10, spr: 0.8 }, lp(0.58, 0.08), AMP(0.4, 1, 0.9, 1.2)))],
  lfo: [LF(1, 12)], mods: [['l1', 'A.lvl', 0.45]],
  fx: fx(REV(0.4, 4.5, 0.8, 0))
}, { vary: [['lfo.0.rate', 8, 16]] });

A('Cinematic', ['Reverse Swell', 'Inhale', 'Approach', 'Rising Tide', 'Event Horizon'], {
  L: [saw(Object.assign({ uni: 5, det: 15, spr: 0.85 }, lp(0.4, 0.15), F(2.5, 1, 0.8, 2, 0.25), AMP(2.5, 0.5, 0.8, 1.5))),
    wt('Choir', 0.4, Object.assign({ lvl: 0.4, uni: 3 }, AMP(3, 0.5, 0.8, 1.5)))],
  fx: fx(REV(0.55, 7, 0.9, 3))
}, { vary: [['L.0.aa', 1.5, 4]] });

A('Cinematic', ['Epic Horizon', 'Last Stand', 'Wide Screen', 'Heroes Theme', 'Dune Sea'], {
  L: [saw(Object.assign({ uni: 5, det: 14, spr: 0.85 }, lp(0.6, 0.1), AMP(0.5, 1, 0.9, 2))),
    saw(Object.assign({ crs: -12, uni: 3, det: 10, lvl: 0.5 }, lp(0.5, 0.1), AMP(0.5, 1, 0.9, 2))),
    vox(Object.assign({ crs: 12, lvl: 0.25, vow: 0.05, uni: 3 }, AMP(0.8, 1, 0.9, 2)))],
  fx: fx(REV(0.45, 5.5, 0.85, 0), CHO(0.2, 0.3, 0.5, 1))
}, {});

A('Cinematic', ['Piano Strings', 'Score Keys', 'Credits Roll'], {
  L: [modal('Piano', Object.assign({ mdec: 4, mbri: 0.4, mhard: 0.4, minh: 0.5 }, AMP(0.001, 1, 1, 0.8))),
    wt('Strings', 0.4, Object.assign({ lvl: 0.35, uni: 4, det: 10, spr: 0.8 }, lp(0.55, 0.05), AMP(0.8, 1, 0.9, 1.5)))],
  fx: fx(REV(0.4, 4.5, 0.8, 0))
}, {});


/* =========================================================== LIBRARY EXPANSION (v2) */
A('Pads', ['Analog Dusk', 'Oberheim Sky', 'Brass Pad Warm', 'Slow Ladder', 'Amber Room', 'Copper Clouds', 'Tube Glow Pad', 'Evening Voltage', 'Mahogany Pad'], {
  L: [saw(Object.assign({ uni: 2, det: 9, spr: 0.6, drift: 0.5 }, ladderF(0.42, 0.2, { f1k: 0.4 }), F(0.8, 2, 0.6, 2, 0.16), AMP(0.5, 1.5, 0.85, 2))),
    saw(Object.assign({ crs: -12, fin: 6, lvl: 0.5, drift: 0.5 }, ladderF(0.4, 0.1), AMP(0.6, 1.5, 0.85, 2)))],
  fx: fx(CHO(0.3, 0.35, 0.5, 0), REV(0.28, 3.5, 0.65, 0), DRV(0.12, 1, 0.6))
}, { vary: [['L.0.f1c', 0.36, 0.52], ['L.0.f1e', 0.08, 0.24], ['L.1.crs', [-12, 0, 7]], ['L.0.shp', [0.667, 0.85, 0.5]]] });

A('Pads', ['Hollow Choir Pad', 'Reed Organ Pad', 'Woodwind Clouds', 'Clarinet Fog', 'Hollow Moon', 'Pipe Dream Pad', 'Paper Lantern', 'Oboe Mist'], {
  L: [wt('Hollow', 0.3, Object.assign({ uni: 4, det: 11, spr: 0.8 }, lp12(0.62, 0.08), AMP(0.9, 1.5, 0.9, 2.6))),
    wt('Reed', 0.4, Object.assign({ crs: 12, lvl: 0.3, uni: 2, det: 8, spr: 0.9 }, lp12(0.7), AMP(1.2, 1.5, 0.85, 2.8)))],
  lfo: [LF(6, 0.1)], mods: [['l1', 'A.pos', 0.3], ['l1', 'B.pos', 0.2]],
  fx: fx(REV(0.38, 5, 0.8, 0), CHO(0.25, 0.3, 0.5, 1))
}, { vary: [['L.0.pos', 0.1, 0.7], ['L.0.f1c', 0.52, 0.7], ['L.1.wt', ['Reed', 'Odd Rise', 'Brass']]] });

A('Pads', ['FM Glass Pad', 'DX Heaven', 'Electric Halo', 'Tine Clouds', 'Digital Dawn', 'Chrome Sky', 'Soft Operators', 'Crystal Method Pad'], {
  L: [sine(Object.assign({ fm: 0.2, fmr: 2, uni: 3, det: 9, spr: 0.8 }, AMP(0.8, 2, 0.85, 2.8))),
    sine(Object.assign({ crs: 12, fm: 0.15, fmr: 3, lvl: 0.4, uni: 2, det: 7, spr: 0.9 }, AMP(1.2, 2, 0.8, 3)))],
  lfo: [LF(0, 0.12), LF(6, 0.2)], mods: [['l1', 'A.fm', 0.08], ['l2', 'B.fm', 0.1]],
  fx: fx(CHO(0.4, 0.4, 0.5, 0), REV(0.4, 5, 0.8, 0), DLY(0.12, '1/4.', 0.35))
}, { vary: [['L.0.fmr', [1, 2, 3, 4]], ['L.1.fmr', [2, 3, 5, 7]], ['L.0.fm', 0.1, 0.3]] });

A('Pads', ['Noise Swell Pad', 'Sea Spray', 'Breath of Winter', 'White Dunes', 'Static Bloom', 'Pink Horizon', 'Tide Pool'], {
  L: [saw(Object.assign({ uni: 5, det: 16, spr: 0.85, nz: 0.12 }, lp(0.5, 0.15), F(1.5, 2.5, 0.6, 2.5, 0.15), AMP(1.2, 2, 0.9, 3))),
    noise(0, Object.assign({ ncol: 0.6, kt: 0, lvl: 0.22, spr: 0.8 }, bp(0.66, 0.3, { f1k: 0 }), AMP(1.8, 2, 0.8, 3)))],
  lfo: [LF(6, 0.09)], mods: [['l1', 'B.f1c', 0.12], ['l1', 'A.f1c', 0.05]],
  fx: fx(REV(0.42, 6, 0.85, 0), CHO(0.3, 0.3, 0.5, 0))
}, { vary: [['L.1.ncol', 0.35, 0.85], ['L.1.lvl', 0.12, 0.32], ['L.0.f1c', 0.42, 0.6]] });

A('Atmospheres', ['Cave Drip', 'Frozen Cavern', 'Underpass', 'Stone Chamber', 'Sewer Echo', 'Catacombs', 'Mineral Room'], {
  L: [modal('Plate', Object.assign({ mbow: 0.5, mdec: 6, mbri: 0.4, mhard: 0.3, spr: 0.8 }, AMP(1.5, 2, 1, 4))),
    noise(2, Object.assign({ ncol: 0.2, kt: 0, lvl: 0.3, spr: 0.9 }, AMP(1, 1, 1, 3)))],
  fx: fx(REV(0.6, 9, 0.95, 0), DLY(0.25, '1/4.', 0.5, { hc: 3000 }))
}, { vary: [['L.0.mdl', ['Plate', 'Gong', 'Glass', 'Drum']], ['L.0.mbri', 0.3, 0.6], ['L.1.ncol', 0.1, 0.5]] });

A('Atmospheres', ['City at 4AM', 'Rooftop Rain', 'Subway Dream', 'Neon Puddles', 'Night Bus', 'Parking Garage', 'Empty Platform', 'Corner Store Glow'], {
  L: [saw(Object.assign({ uni: 4, det: 14, spr: 0.8, drift: 0.5 }, lp(0.4, 0.2), AMP(1.6, 2, 0.9, 3.5))),
    noise(2, Object.assign({ ncol: 0.45, kt: 0, lvl: 0.25, spr: 0.8 }, AMP(1, 1, 1, 3))),
    sine(Object.assign({ crs: 24, lvl: 0.12, fm: 0.1, fmr: 3.5 }, AMP(2.5, 3, 0.5, 4)))],
  lfo: [LF(6, 0.07)], mods: [['l1', 'A.f1c', 0.08], ['l1', 'C.lvl', 0.5]],
  fx: fx(LOFI(0.4, { bits: 12, rate: 1, wow: 0.4, flut: 0.15, noise: 0.15, tone: 0.5 }), REV(0.5, 7, 0.85, 0), DLY(0.18, '1/4', 0.45))
}, { vary: [['L.0.f1c', 0.32, 0.5], ['L.1.ntyp', [2, 1, 4]], ['L.2.crs', [12, 19, 24]]] });

A('Atmospheres', ['Dream Sequence', 'Memory Fade', 'Slow Dissolve', 'Half Asleep', 'Lucid Loop', 'Daydream Tape', 'Blurred Photo'], {
  L: [gran('Synth Chord', Object.assign({ gpos: 0.4, gspr: 0.4, gsz: 0.7, gden: 0.55, gpit: 0.02, spr: 0.9 }, lp12(0.66), AMP(1.5, 2, 1, 4))),
    tri(Object.assign({ crs: 12, lvl: 0.22, uni: 3, det: 10, spr: 0.9, f1t: 'off' }, AMP(2.5, 2, 0.8, 4)))],
  lfo: [LF(6, 0.05)], mods: [['l1', 'A.gpos', 0.25]],
  fx: fx(LOFI(0.5, { bits: 12, rate: 1, wow: 0.6, flut: 0.2, noise: 0.1, tone: 0.5 }), REV(0.55, 8, 0.9, 3))
}, { vary: [['L.0.src', ['Synth Chord', 'Felt Piano', 'Strings', 'Guitar', 'Organ Chord']], ['L.0.gpos', 0.15, 0.7]] });

A('Keys', ['FM Tines', 'Bell Piano', 'Glass Keys', 'Chime Keys', 'Stage Tines', 'Sparkle EP', 'Winter Keys', 'Frosted Tines'], {
  L: [modal('Tine', Object.assign({ mdec: 4.5, mbri: 0.55, mhard: 0.6, vs: 0.75 }, AMP(0.001, 1, 1, 0.4))),
    modal('Glock', Object.assign({ crs: 12, lvl: 0.22, mdec: 1.2, mbri: 0.5, mhard: 0.5 }, AMP(0.001, 1, 1, 0.4))),
    sine(Object.assign({ lvl: 0.3 }, AMP(0.002, 3.5, 0, 0.35)))],
  fx: fx(CHO(0.4, 0.5, 0.45, 0), REV(0.25, 2.8, 0.6, 0))
}, { vary: [['L.1.mdl', ['Glock', 'Crystal', 'Music Box', 'Vibes']], ['L.1.lvl', 0.12, 0.32], ['L.0.mbri', 0.42, 0.65]] });

A('Keys', ['Organ Keys Soft', 'Synth Piano', 'Saw Keys', 'Juno Keys', 'Soft Poly', 'Analog Piano', 'Square Keys', 'Wooly Keys'], {
  L: [sq(Object.assign({ pw: 0.42, drift: 0.3, vs: 0.6 }, lp(0.5, 0.15, { f1v: 0.5, f1k: 0.5 }), F(0.001, 0.8, 0.3, 0.4, 0.22), AMP(0.002, 1.6, 0.4, 0.4))),
    tri(Object.assign({ crs: -12, lvl: 0.35, f1t: 'off' }, AMP(0.002, 1.4, 0.4, 0.4)))],
  fx: fx(CHO(0.4, 0.45, 0.55, 0), REV(0.2, 2, 0.5, 1))
}, { vary: [['L.0.shp', [1, 0.667, 0.85]], ['L.0.f1c', 0.42, 0.6], ['L.0.fd', 0.4, 1.2]] });

A('Keys', ['Harpsichord', 'Baroque Keys', 'Plucked Keys', 'Clavichord', 'Spinet', 'Court Keys'], {
  L: [pluck(Object.assign({ pdec: 1.8, pdmp: 0.12, ppos: 0.06, pexc: 0.9, vs: 0.3 }, AMP(0.001, 1, 1, 0.15))),
    pluck(Object.assign({ crs: 12, lvl: 0.4, pdec: 1.2, pdmp: 0.1, ppos: 0.05, pexc: 0.9 }, AMP(0.001, 1, 1, 0.15)))],
  fx: fx(REV(0.25, 2.2, 0.55, 1))
}, { vary: [['L.1.lvl', 0.15, 0.55], ['L.0.pdmp', 0.05, 0.3], ['L.0.ppos', 0.04, 0.12]], kit: 'pluck' });

A('Lo-Fi & Dusty', ['Dusty Wurli', 'Tape Wurli', 'Worn Reeds', 'Diner Keys', 'Motel Lobby', 'Radio Keys', 'Laundromat'], {
  L: [lay('analog', Object.assign({ shp: 0.85, pw: 0.42, fold: 0.12, vs: 0.75 }, lp12(0.48, 0.12, { f1v: 0.5 }), F(0.001, 0.35, 0.2, 0.3, 0.2), AMP(0.002, 2.2, 0.08, 0.3)))],
  lfo: [LF(0, 5)], mods: [['l1', '*.lvl', 0.14]],
  fx: fx(DRV(0.3, 1, 0.5), LOFI(0.9, { bits: 10, rate: 2.2, wow: 0.45, flut: 0.25, noise: 0.25, tone: 0.48 }), REV(0.2, 1.8, 0.45, 1))
}, { vary: [['fx.lofi.bits', 8, 12], ['fx.lofi.wow', 0.3, 0.6], ['L.0.f1c', 0.42, 0.55]] });

A('Lo-Fi & Dusty', ['Dusty Choir', 'Tape Voices', 'Worn Hymnal', 'Thrift Choir', 'AM Radio Choir', 'Sunday Tape'], {
  L: [vox(Object.assign({ vow: 0.08, uni: 4, det: 13, spr: 0.8, brth: 0.15 }, lp12(0.62), AMP(0.25, 1, 0.9, 0.9)))],
  lfo: [LF(0, 5, { rtg: 1, fade: 0.6 })], mods: [['l1', '*.pitch', 0.006]],
  fx: fx(LOFI(0.9, { bits: 10, rate: 2.2, wow: 0.5, flut: 0.25, noise: 0.25, tone: 0.45 }), REV(0.3, 3, 0.65, 0))
}, { vary: [['L.0.vow', 0, 0.9], ['fx.lofi.wow', 0.3, 0.7]], kit: 'vox' });

A('Lo-Fi & Dusty', ['Dusty Bass', 'Tape Sub', 'Cassette Bass', 'Worn Upright', 'Crate Bass', 'SP Bass'], {
  L: [tri(Object.assign({ sub: 0.4, vs: 0.5 }, lp(0.36, 0.15), F(0.001, 0.3, 0.2, 0.2, 0.2), AMP(0.003, 0.8, 0.7, 0.15)))],
  g: { vmode: 2, glide: 0.03 },
  fx: fx(DRV(0.25, 1, 0.5), LOFI(0.8, { bits: 10, rate: 2.5, wow: 0.3, flut: 0.15, noise: 0.15, tone: 0.45 }), DRY)
}, { vary: [['L.0.shp', [0.333, 0.5, 0.667]], ['L.0.f1c', 0.3, 0.42]], kit: 'bass' });

A('Lo-Fi & Dusty', ['Dusty Strings Loop', 'Tape Orchestra', 'Worn Violins', 'Faded Score', 'Old Movie Strings', 'Attic Strings'], {
  L: [wt('Strings', 0.4, Object.assign({ uni: 5, det: 14, spr: 0.8, drift: 0.5 }, lp(0.5, 0.08), AMP(0.4, 1, 0.9, 1)))],
  lfo: [LF(0, 5, { rtg: 1, fade: 0.7 })], mods: [['l1', '*.pitch', 0.006]],
  fx: fx(LOFI(0.9, { bits: 10, rate: 2.2, wow: 0.55, flut: 0.3, noise: 0.3, tone: 0.45 }), REV(0.3, 3, 0.65, 0))
}, { vary: [['L.0.pos', 0.2, 0.7], ['fx.lofi.bits', 8, 12]], kit: 'strings' });

A('Bells & Mallets', ['Crystal Bowls', 'Glass Mallets', 'Ice Xylophone', 'Wine Glasses', 'Frozen Chimes', 'Glass Marimba', 'Prism Mallets'], {
  L: [modal('Glass', Object.assign({ mdec: 2.4, mbri: 0.5, mhard: 0.45, spr: 0.5, vs: 0.6 }, AMP(0.001, 1, 1, 1.2)))],
  fx: fx(REV(0.35, 3.5, 0.7, 0), DLY(0.12, '1/8.', 0.3))
}, { vary: [['L.0.mdl', ['Glass', 'Crystal', 'Plate']], ['L.0.mdec', 1.2, 4], ['L.0.mhard', 0.3, 0.7]] });

A('Bells & Mallets', ['Xylophone', 'Wood Block Mallets', 'Balafon', 'Dry Xylo', 'Toy Xylophone', 'Log Mallets'], {
  L: [modal('Wood', Object.assign({ crs: 12, mdec: 0.5, mbri: 0.5, mhard: 0.7, vs: 0.7 }, AMP(0.001, 1, 1, 0.2)))],
  fx: fx(REV(0.18, 1.5, 0.4, 1))
}, { vary: [['L.0.crs', [0, 12]], ['L.0.mdec', 0.3, 0.9], ['L.0.mhard', 0.5, 0.9]] });

A('Bells & Mallets', ['Gamelan', 'Bali Bronze', 'Temple Gongs', 'Bronze Kettle', 'Gong Ageng', 'Metallophone', 'Singing Bronze'], {
  L: [modal('Gong', Object.assign({ mdec: 3.5, mbri: 0.5, mhard: 0.6, minh: 0.2, spr: 0.5 }, AMP(0.001, 1, 1, 2))),
    modal('Bell', Object.assign({ crs: 12, fin: 12, lvl: 0.3, mdec: 2, mbri: 0.5 }, AMP(0.001, 1, 1, 1.5)))],
  fx: fx(REV(0.35, 4, 0.75, 0))
}, { vary: [['L.1.fin', 5, 22], ['L.0.mdl', ['Gong', 'Plate', 'Steel Pan']], ['L.0.mbri', 0.4, 0.65]] });

A('Bells & Mallets', ['Hand Pan', 'Hang Drum', 'Tongue Drum', 'Steel Tongue', 'Meditation Pan', 'Tank Drum'], {
  L: [modal('Steel Pan', Object.assign({ mdec: 2.4, mbri: 0.36, mhard: 0.3, spr: 0.4, vs: 0.7 }, AMP(0.001, 1, 1, 1))),
    sine(Object.assign({ lvl: 0.25 }, AMP(0.002, 1.2, 0, 0.5)))],
  fx: fx(REV(0.35, 3.5, 0.7, 0))
}, { vary: [['L.0.mbri', 0.28, 0.48], ['L.0.mdec', 1.5, 3.5]] });

A('Plucks', ['Pizz Pluck', 'Muted Pluck', 'Palm Pluck', 'Thumb Pluck', 'Dead Note Pluck', 'Short String', 'Rubber Band'], {
  L: [pluck(Object.assign({ pdec: 0.5, pdmp: 0.6, ppos: 0.25, pexc: 0.5, vs: 0.7 }, lp12(0.62, 0.1), AMP(0.001, 1, 1, 0.12)))],
  fx: fx(DLY(0.18, '1/8.', 0.3), REV(0.2, 1.8, 0.5, 1))
}, { vary: [['L.0.pdec', 0.3, 0.9], ['L.0.pdmp', 0.45, 0.75], ['L.0.f1c', 0.5, 0.72]] });

A('Plucks', ['FM Pluck', 'Glass FM Pluck', 'Marimba FM', 'Bottle Pluck', 'DX Pluck', 'Tube Pluck', 'Plonk', 'Wooden FM'], {
  L: [sine(Object.assign({ fmr: 3, vs: 0.6 }, AMP(0.001, 0.6, 0, 0.3)))],
  me: [ME(0, 0.12, 0, 0.1)], mods: [['e1', 'A.fm', 0.5], ['vel', 'A.fm', 0.15]],
  fx: fx(DLY(0.2, '1/8.', 0.35), REV(0.22, 2.2, 0.55, 0))
}, { vary: [['L.0.fmr', [1, 2, 3, 3.5, 5, 7]], ['me.0.d', 0.05, 0.3], ['L.0.ad', 0.3, 1.2]] });

A('Plucks', ['Trance Pluck', 'Super Pluck', 'Festival Pluck', 'Rave Pluck', 'Stadium Pluck', 'Offbeat Pluck', 'Hands Up Pluck'], {
  L: [saw(Object.assign({ uni: 7, det: 24, spr: 0.85, vs: 0.5 }, lp(0.36, 0.2), F(0.001, 0.3, 0.05, 0.3, 0.42), AMP(0.001, 0.6, 0, 0.35)))],
  fx: fx(DLY(0.25, '1/8.', 0.4), REV(0.28, 3, 0.65, 0))
}, { vary: [['L.0.det', 16, 34], ['L.0.fd', 0.18, 0.5], ['L.0.f1c', 0.3, 0.44]] });

A('Plucks', ['Vowel Pluck', 'Talking Pluck', 'Yah Pluck', 'Wow Pluck', 'Chatter Pluck', 'Mouth Harp'], {
  L: [wt('Vowel', 0.1, Object.assign({ uni: 2, det: 8, spr: 0.5 }, lp(0.7, 0.1), AMP(0.001, 0.5, 0, 0.2)))],
  me: [ME(0, 0.2, 0, 0.15)], mods: [['e1', 'A.pos', 0.7]],
  fx: fx(DLY(0.2, '1/8', 0.3), REV(0.18, 1.8, 0.45, 1))
}, { vary: [['L.0.wt', ['Vowel', 'Vocal Pulse', 'Choir']], ['mods.0.a', -0.7, 0.8], ['me.0.d', 0.1, 0.35]] });

A('Leads', ['Flute Lead Synth', 'Ocarina Lead', 'Triangle Lead', 'Soft Lead', 'Lullaby Lead', 'Pure Lead', 'Hollow Lead', 'Recorder Lead'], {
  L: [tri(Object.assign({ vs: 0.4 }, lp12(0.72, 0.1), AMP(0.02, 0.4, 1, 0.25))),
    wt('Hollow', 0.3, Object.assign({ lvl: 0.3 }, lp12(0.7), AMP(0.03, 0.4, 1, 0.25)))],
  g: { vmode: 2, glide: 0.05 },
  lfo: [LF(0, 5.3, { rtg: 1, fade: 0.3 })], mods: [['l1', '*.pitch', 0.008]],
  fx: fx(DLY(0.25, '1/8.', 0.4), REV(0.3, 3.2, 0.65, 0))
}, { vary: [['L.1.lvl', 0.1, 0.5], ['L.0.shp', 0.15, 0.4], ['g.glide', 0.02, 0.1]] });

A('Leads', ['Acid Lead', 'Squelch Lead', 'Resonant Lead', 'Rubber Lead', 'Zappy Lead', '303 Scream', 'Liquid Acid'], {
  L: [saw(Object.assign({ vs: 0.6 }, ladderF(0.4, 0.75, { f1v: 0.5, f1k: 0.5 }), F(0.001, 0.25, 0.2, 0.2, 0.4), AMP(0.002, 0.4, 0.9, 0.12)))],
  g: { vmode: 2, glide: 0.06 },
  fx: fx(DRV(0.4, 0, 0.65), DLY(0.2, '1/8.', 0.35), REV(0.12, 1.5, 0.4, 1))
}, { vary: [['L.0.shp', [0.667, 1]], ['L.0.f1r', 0.6, 0.9], ['L.0.fd', 0.15, 0.45]] });

A('Leads', ['Detuned Fifths', 'Fifth Lead', 'Power Lead', 'Stack Lead', 'Organum Lead', 'Rock Synth Lead'], {
  L: [saw(Object.assign({ uni: 2, det: 10, spr: 0.4 }, lp(0.66, 0.15), AMP(0.003, 0.5, 1, 0.25))),
    saw(Object.assign({ crs: 7, lvl: 0.6, uni: 2, det: 10, spr: 0.6 }, lp(0.66, 0.15), AMP(0.003, 0.5, 1, 0.25)))],
  g: { vmode: 2, glide: 0.04 },
  fx: fx(DRV(0.25, 0, 0.7), DLY(0.22, '1/8.', 0.35), REV(0.18, 2, 0.5, 1))
}, { vary: [['L.1.crs', [7, 12, -5]], ['L.0.f1c', 0.58, 0.76]] });

A('Leads', ['FM Lead', 'Metal Lead', 'Bell Lead', 'Digital Lead', 'Chrome Lead', 'Glassy Lead', 'Needle Lead'], {
  L: [sine(Object.assign({ fm: 0.3, fmr: 2, vs: 0.5 }, AMP(0.003, 0.5, 1, 0.25)))],
  g: { vmode: 2, glide: 0.04 },
  me: [ME(0, 0.4, 0.3, 0.3)], mods: [['e1', 'A.fm', 0.2], ['mw', 'A.fm', 0.3]],
  lfo: [LF(0, 5.6, { rtg: 1, fade: 0.3 })],
  fx: fx(DLY(0.25, '1/8.', 0.4), REV(0.22, 2.5, 0.55, 0), CHO(0.2, 0.5, 0.4, 0))
}, { vary: [['L.0.fmr', [1, 2, 3, 4, 7]], ['L.0.fm', 0.18, 0.42]] });

A('Synth Bass', ['Sub Saw Bass', 'Warm Sub Bass', 'Deep House Bass', 'Garage Bass', 'Round Saw', 'Low Rider', 'Basement Bass', 'Velvet Bass'], {
  L: [saw(Object.assign({ vs: 0.5 }, lp(0.26, 0.12), F(0.001, 0.25, 0.3, 0.15, 0.14), AMP(0.003, 0.6, 0.85, 0.12))),
    sine(Object.assign({ lvl: 0.7 }, AMP(0.003, 0.6, 0.9, 0.12)))],
  g: { vmode: 2, glide: 0.02 },
  fx: fx(DRY)
}, { vary: [['L.0.f1c', 0.2, 0.34], ['L.1.lvl', 0.45, 0.85], ['L.0.shp', [0.667, 1, 0.5]]] });

A('Synth Bass', ['Acid Bass', 'Squelch Bass', '303 Bass', 'Rubber Acid', 'Warehouse Bass', 'Liquid Bass'], {
  L: [saw(Object.assign({ vs: 0.6 }, ladderF(0.28, 0.75, { f1v: 0.6 }), F(0.001, 0.2, 0.05, 0.1, 0.45), AMP(0.001, 0.4, 0.5, 0.08)))],
  g: { vmode: 2, glide: 0.05 },
  fx: fx(DRV(0.4, 0, 0.6), DRY)
}, { vary: [['L.0.shp', [0.667, 1]], ['L.0.f1r', 0.6, 0.9], ['L.0.f1e', 0.3, 0.55], ['L.0.fd', 0.12, 0.35]] });

A('Synth Bass', ['Organ Bass', 'Sine Stack Bass', 'Hollow Bass', 'Square Sub', 'Chip Bass', 'Tuba Synth', 'Pipe Bass'], {
  L: [wt('Hollow', 0.2, Object.assign({ sub: 0.4 }, lp(0.42, 0.1), AMP(0.003, 0.5, 0.9, 0.1)))],
  g: { vmode: 2, glide: 0.02 },
  fx: fx(DRV(0.15, 1, 0.6), DRY)
}, { vary: [['L.0.wt', ['Hollow', 'Drawbars', 'Odd Rise', 'PWM', 'Reed']], ['L.0.pos', 0.05, 0.5], ['L.0.f1c', 0.34, 0.5]] });

A('Synth Bass', ['Fold Bass', 'Gnarly Bass', 'Buzz Bass', 'Chainsaw Bass', 'Hornet Bass', 'Dirty South Bass'], {
  L: [lay('analog', Object.assign({ shp: 0.2, fold: 0.5, sub: 0.4 }, lp(0.5, 0.15), F(0.001, 0.3, 0.3, 0.2, 0.15), AMP(0.002, 0.5, 0.9, 0.12)))],
  g: { vmode: 2, glide: 0.03 },
  me: [ME(0, 0.3, 0.2, 0.2)], mods: [['e1', 'A.fold', 0.25]],
  fx: fx(DRV(0.35, 0, 0.6), DRY)
}, { vary: [['L.0.fold', 0.3, 0.75], ['L.0.f1c', 0.4, 0.6]] });

A('808 & Sub', ['808 Glide King', '808 Rattle', '808 Velvet', '808 Midnight', '808 Detroit', '808 Bay', '808 Texas', '808 Soft Clip', '808 Tube', '808 Fuzz', '808 Chicago', '808 Long Tail', '808 Stab', '808 Dark', '808 Warm'], {
  L: [sine(Object.assign({ vs: 0.35 }, AMP(0.001, 2.2, 0, 0.2)))],
  me: [ME(0, 0.055, 0, 0.05)], mods: [['e1', 'A.pitch', 0.42]],
  g: { vmode: 2, glide: 0.07, gmode: 1 },
  fx: fx(DRV(0.3, 1, 0.55), COMP(-14, 3, 2), DRY)
}, { vary: [['L.0.ad', 0.8, 4.2], ['mods.0.a', 0.2, 0.6], ['me.0.d', 0.03, 0.08], ['fx.drv.amt', 0.1, 0.7], ['fx.drv.type', [0, 1, 1, 3]], ['g.glide', 0.04, 0.14], ['L.0.fold', [0, 0, 0.15, 0.3]]], kit: 's808' });

A('Strings', ['Chamber Strings', 'String Quartet', 'Intimate Strings', 'Viola Section', 'Close Strings', 'Parlor Strings', 'Autumn Strings'], {
  L: [wt('Strings', 0.5, Object.assign({ uni: 3, det: 7, spr: 0.6, drift: 0.5 }, lp(0.64, 0.08, { f1k: 0.4 }), AMP(0.3, 1, 0.95, 0.6))),
    noise(0, Object.assign({ ncol: 0.7, lvl: 0.06 }, bp(0.72, 0.2, { f1k: 1 }), AMP(0.2, 0.5, 0.6, 0.4)))],
  lfo: [LF(0, 5.4, { rtg: 1, fade: 0.5 })], mods: [['l1', 'A.pitch', 0.007], ['vel', 'A.f1c', 0.12]],
  fx: fx(REV(0.28, 2.6, 0.6, 1))
}, { vary: [['L.0.pos', 0.3, 0.8], ['L.0.aa', 0.15, 0.6]], over: { 'Viola Section': p => { p.L[0].crs = -5; } } });

A('Strings', ['Synth Strings Bright', 'Analog Strings', 'PWM Strings', 'Ensemble Saw', 'Polysynth Strings', 'OB Strings', 'Silver Strings'], {
  L: [sq(Object.assign({ pw: 0.4, uni: 4, det: 13, spr: 0.8, drift: 0.3 }, lp(0.62, 0.06), AMP(0.25, 1, 0.9, 0.8))),
    saw(Object.assign({ crs: 12, lvl: 0.35, uni: 2, det: 9, spr: 0.8 }, lp(0.66, 0.05), AMP(0.3, 1, 0.9, 0.8)))],
  lfo: [LF(1, 0.6)], mods: [['l1', 'A.pw', 0.25]],
  fx: fx(CHO(0.5, 0.4, 0.6, 1), REV(0.3, 3.2, 0.7, 0))
}, { vary: [['L.0.f1c', 0.54, 0.72], ['lfo.0.rate', 0.3, 1.1]] });

A('Brass & Winds', ['French Horns', 'Horn Swell', 'Mellow Horns', 'Distant Horns', 'Valhalla Horns', 'Soft Brass', 'Flugel Pad'], {
  L: [wt('Brass', 0.3, Object.assign({ uni: 3, det: 7, spr: 0.7, vs: 0.6 }, lp(0.46, 0.08, { f1v: 0.6 }), F(0.25, 0.8, 0.6, 0.4, 0.14), AMP(0.12, 0.6, 0.9, 0.5)))],
  lfo: [LF(0, 5, { rtg: 1, fade: 0.6 })], mods: [['l1', 'A.pitch', 0.005]],
  fx: fx(REV(0.4, 4.2, 0.8, 0))
}, { vary: [['L.0.pos', 0.2, 0.5], ['L.0.aa', 0.06, 0.3], ['L.0.f1c', 0.4, 0.54]] });

A('Brass & Winds', ['Muted Trumpet', 'Jazz Trumpet', 'Harmon Mute', 'Noir Trumpet', 'Lonely Horn', 'Blue Trumpet'], {
  L: [wt('Brass', 0.7, Object.assign({ vs: 0.6, f2t: 'bp', f2c: 0.68, f2r: 0.35, f2k: 0.2 }, lp(0.7, 0.1), AMP(0.03, 0.4, 0.9, 0.2)))],
  g: { vmode: 2, glide: 0.03 },
  lfo: [LF(0, 5.5, { rtg: 1, fade: 0.4 })], mods: [['l1', 'A.pitch', 0.008]],
  fx: fx(REV(0.3, 2.8, 0.6, 1), DLY(0.1, '1/8.', 0.25))
}, { vary: [['L.0.f2c', 0.6, 0.76], ['L.0.pos', 0.5, 0.9]], kit: 'brass' });

A('Brass & Winds', ['Synth Sax', 'Tenor Synth', 'Alto Reed', 'Soprano Reed', 'Bari Synth', 'Breathy Reed'], {
  L: [wt('Reed', 0.5, Object.assign({ fold: 0.08, vs: 0.6 }, lp(0.62, 0.12, { f1v: 0.5 }), F(0.03, 0.4, 0.6, 0.2, 0.12), AMP(0.04, 0.4, 0.95, 0.2))),
    noise(0, Object.assign({ ncol: 0.65, lvl: 0.12 }, bp(0.66, 0.3, { f1k: 1 }), AMP(0.04, 0.3, 0.4, 0.2)))],
  g: { vmode: 2, glide: 0.04 },
  lfo: [LF(0, 5.2, { rtg: 1, fade: 0.4 })], mods: [['l1', 'A.pitch', 0.008]],
  fx: fx(REV(0.25, 2.4, 0.55, 1))
}, { vary: [['L.0.pos', 0.3, 0.8], ['L.1.lvl', 0.06, 0.2]], over: { 'Bari Synth': p => { p.L[0].crs = -12; p.L[1].crs = -12; } } });

A('Choir & Vox', ['Choir Eeh', 'Bright Choir', 'Soprano Section', 'Gospel Shout', 'Alto Voices', 'Mixed Choir', 'Tenor Section', 'Requiem'], {
  L: [vox(Object.assign({ vow: 0.3, uni: 5, det: 13, spr: 0.85, brth: 0.1 }, lp12(0.8), AMP(0.3, 1, 1, 1.2)))],
  lfo: [LF(0, 5.1, { rtg: 1, fade: 0.6 })], mods: [['l1', '*.pitch', 0.006]],
  fx: fx(REV(0.42, 4.5, 0.8, 0))
}, { vary: [['L.0.vow', 0.15, 0.7], ['L.0.fsh', 0.35, 0.72], ['L.0.aa', 0.1, 0.6]] });

A('Choir & Vox', ['Humming', 'Mmm Pad', 'Lullaby Hum', 'Monk Hum', 'Night Hum', 'Closed Mouth Choir'], {
  L: [vox(Object.assign({ vow: 1, fsh: 0.38, uni: 4, det: 10, spr: 0.8, brth: 0.04 }, lp(0.52, 0.05), AMP(0.4, 1, 1, 1.2)))],
  lfo: [LF(0, 4.8, { rtg: 1, fade: 0.8 })], mods: [['l1', '*.pitch', 0.005]],
  fx: fx(REV(0.4, 4.2, 0.8, 0))
}, { vary: [['L.0.fsh', 0.3, 0.46], ['L.0.f1c', 0.46, 0.6]], over: { 'Monk Hum': p => { p.L[0].crs = -12; } } });

A('Choir & Vox', ['Solo Voice', 'Lead Vox Synth', 'Siren Song', 'Opera Ghost', 'Lonely Voice', 'Falsetto Voice', 'Night Singer'], {
  L: [vox(Object.assign({ vow: 0.08, fsh: 0.58, brth: 0.14, vs: 0.5 }, AMP(0.06, 0.4, 1, 0.35)))],
  g: { vmode: 2, glide: 0.08 },
  lfo: [LF(0, 5.5, { rtg: 1, fade: 0.35 }), LF(6, 0.8)], mods: [['l1', 'A.pitch', 0.012], ['l2', 'A.vow', 0.18]],
  fx: fx(DLY(0.22, '1/4', 0.4), REV(0.4, 4.2, 0.75, 0))
}, { vary: [['L.0.vow', 0, 0.5], ['L.0.fsh', 0.45, 0.7]] });

A('Organs', ['Percussive Organ', 'Click Organ', 'Jimmy Smith Organ', 'Bebop Organ', 'Ballpark Organ', 'Skating Rink', 'Lounge Organ'], {
  L: [wt('Drawbars', 0.25, Object.assign({ crs: 12, vs: 0.2, f1t: 'off' }, AMP(0.004, 0.2, 1, 0.06))),
    sine(Object.assign({ crs: 31, lvl: 0.35 }, AMP(0.001, 0.25, 0, 0.1)))],
  lfo: [LF(0, 6.4)], mods: [['l1', 'A.pan', 0.3]],
  fx: fx(CHO(0.35, 6, 0.3, 0), DRV(0.2, 1, 0.6), REV(0.18, 1.6, 0.45, 1))
}, { vary: [['L.0.pos', 0.1, 0.6], ['L.1.crs', [31, 24, 36]], ['L.1.lvl', 0.2, 0.5]] });

A('Organs', ['Harmonium', 'Pump Organ', 'Accordion', 'Melodica', 'Reed Box', 'Squeeze Box', 'Bandoneon'], {
  L: [wt('Reed', 0.5, Object.assign({ uni: 2, det: 9, spr: 0.4, vs: 0.4 }, lp(0.7, 0.08), AMP(0.05, 0.3, 1, 0.15))),
    wt('Reed', 0.7, Object.assign({ crs: 12, lvl: 0.3 }, lp(0.72, 0.08), AMP(0.06, 0.3, 1, 0.15)))],
  lfo: [LF(0, 5)], mods: [['l1', '*.lvl', 0.06]],
  fx: fx(REV(0.22, 2, 0.5, 1))
}, { vary: [['L.0.det', 5, 16], ['L.1.lvl', 0.12, 0.5], ['L.0.pos', 0.3, 0.8]] });

A('Guitars & World', ['Sitar', 'Raga String', 'Buzz String', 'Tanpura Pluck', 'Electric Sitar', 'Bombay Pluck'], {
  L: [pluck(Object.assign({ pdec: 3, pdmp: 0.12, ppos: 0.04, pexc: 0.95, f2t: 'comb', f2c: 0.55, f2r: 0.5, f2k: 1 }, lp12(0.86), AMP(0.001, 1, 1, 0.5)))],
  lfo: [LF(0, 0.3)], mods: [['l1', 'A.f2c', 0.01]],
  fx: fx(REV(0.28, 2.8, 0.6, 0), DLY(0.12, '1/8', 0.25))
}, { vary: [['L.0.f2r', 0.3, 0.7], ['L.0.pdec', 2, 4.5]] });

A('Guitars & World', ['Banjo', 'Mandolin', 'Bouzouki', 'Oud', 'Charango', 'Cavaquinho', 'Dulcimer', 'Ukulele'], {
  L: [pluck(Object.assign({ pdec: 1.1, pdmp: 0.25, ppos: 0.1, pexc: 0.8, uni: 2, det: 8, spr: 0.4, vs: 0.6, f2t: 'peak', f2c: 0.56, f2r: 0.4, f2k: 0 }, lp12(0.82), AMP(0.001, 1, 1, 0.2)))],
  fx: fx(REV(0.2, 1.8, 0.5, 1))
}, { vary: [['L.0.pdec', 0.6, 2.2], ['L.0.pdmp', 0.15, 0.5], ['L.0.f2c', 0.46, 0.66], ['L.0.uni', [1, 2, 2]]] });

A('Guitars & World', ['Muted Guitar', 'Funk Mute', 'Palm Mute', 'Reggae Chop', 'Chicken Pick', 'Disco Guitar', 'Staccato Guitar'], {
  L: [pluck(Object.assign({ pdec: 0.35, pdmp: 0.5, ppos: 0.14, pexc: 0.6, vs: 0.7 }, lp12(0.7, 0.1), AMP(0.001, 1, 1, 0.08)))],
  fx: fx(CHO(0.2, 0.6, 0.4, 0), COMP(-18, 3, 3), REV(0.15, 1.4, 0.4, 1))
}, { vary: [['L.0.pdec', 0.2, 0.6], ['L.0.pdmp', 0.35, 0.65]] });

A('Guitars & World', ['Overdriven Guitar', 'Crunch Guitar', 'Fuzz Guitar', 'Garage Guitar', 'Blues Lead Guitar', 'Power Chord Guitar'], {
  L: [pluck(Object.assign({ pdec: 4, pdmp: 0.3, ppos: 0.12, pexc: 0.7, vs: 0.5 }, lp12(0.68, 0.15), AMP(0.001, 1, 1, 0.25)))],
  fx: fx(DRV(0.6, 0, 0.5), EQ(0, 3, 1400, -3), REV(0.2, 2, 0.5, 1), DLY(0.1, '1/8.', 0.25))
}, { vary: [['fx.drv.amt', 0.4, 0.85], ['fx.drv.type', [0, 1, 3]], ['L.0.f1c', 0.58, 0.76]], kit: 'guitar' });

A('Arps & Sequences', ['Pluck Arp Soft', 'Harp Arp', 'String Arp', 'Koto Runner', 'Guitar Arp', 'Lute Steps', 'Nylon Runner'], {
  L: [pluck(Object.assign({ pdec: 1.4, pdmp: 0.35, ppos: 0.14, pexc: 0.6 }, lp12(0.8), AMP(0.001, 1, 1, 0.3)))],
  arp: ARP('updown', '1/16', 2, 0.9),
  fx: fx(DLY(0.25, '1/8.', 0.4), REV(0.3, 3, 0.65, 0))
}, { vary: [['arp.mode', ['up', 'updown', 'down', 'order', 'random']], ['arp.oct', [1, 2, 3]], ['L.0.pdmp', 0.2, 0.5], ['arp.div', ['1/16', '1/8', '1/8T']]], kit: 'pluck' });

A('Arps & Sequences', ['FM Sequence', 'Digital Steps', 'Marimba Machine', 'Minimalist', 'Phase Music', 'Pattern Glass', 'Clockwork Arp', 'Mallet Machine'], {
  L: [sine(Object.assign({ fmr: 3 }, AMP(0.001, 0.35, 0, 0.2)))],
  me: [ME(0, 0.1, 0, 0.1)], mods: [['e1', 'A.fm', 0.45]],
  arp: ARP('up', '1/16', 2, 0.6),
  fx: fx(DLY(0.28, '1/8.', 0.45, { pp: 1 }), REV(0.25, 2.5, 0.6, 0))
}, { vary: [['L.0.fmr', [1, 2, 3, 5, 7]], ['arp.mode', ['up', 'updown', 'order', 'random']], ['arp.oct', [1, 2, 3]]] });

A('Arps & Sequences', ['Octave Bass Arp', 'Disco Bass', 'Italo Bass', 'Moroder Line', 'Synth Pop Bass', 'Bounce Octaves'], {
  L: [saw(Object.assign({ sub: 0.3 }, lp(0.36, 0.25), F(0.001, 0.15, 0.1, 0.1, 0.3), AMP(0.001, 0.25, 0.3, 0.08)))],
  g: { vmode: 1 },
  arp: ARP('up', '1/16', 2, 0.5),
  fx: fx(DRV(0.2, 1, 0.6), DLY(0.1, '1/8.', 0.25), REV(0.06, 1.2, 0.3, 1))
}, { vary: [['L.0.f1c', 0.3, 0.44], ['arp.div', ['1/16', '1/8']], ['L.0.shp', [0.667, 1]]], kit: 'bass' });

A('Arps & Sequences', ['Swing Arp', 'Shuffle Keys', 'Dilla Steps', 'Lazy Arp', 'Drunk Arp', 'Offbeat Runner'], {
  L: [modal('Tine', Object.assign({ mdec: 1.6, mbri: 0.45, mhard: 0.5 }, lp12(0.66), AMP(0.001, 1, 1, 0.3)))],
  arp: ARP('updown', '1/16', 1, 0.8, { swing: 0.6, vel: [1, 0.5, 0.8, 0.5, 1, 0.5, 0.7, 0.6, 1, 0.5, 0.8, 0.5, 1, 0.6, 0.7, 0.5] }),
  fx: fx(LOFI(0.5, { bits: 12, rate: 1.5, wow: 0.3, flut: 0.15, noise: 0.15, tone: 0.55 }), REV(0.22, 2, 0.5, 1))
}, { vary: [['arp.swing', 0.35, 0.85], ['L.0.mdl', ['Tine', 'Vibes', 'Marimba', 'Kalimba']], ['arp.oct', [1, 2]]], kit: 'keys' });

A('Pulses & Gates', ['Tremolo Keys Pad', 'Helicopter Pad', 'Chopper', 'Square Tremolo', 'Rotor Pad', 'Strobe Pad', 'Flicker'], {
  L: [saw(Object.assign({ uni: 4, det: 14, spr: 0.8 }, lp(0.56, 0.12), AMP(0.15, 1, 0.9, 1)))],
  lfo: [LS(4, '1/16')], mods: [['l1', 'A.lvl', 0.8]],
  fx: fx(REV(0.3, 3, 0.65, 0), DLY(0.15, '1/8.', 0.35))
}, { vary: [['lfo.0.div', ['1/16', '1/8', '1/16T', '1/8T', '1/32']], ['lfo.0.shp', [4, 0, 3]], ['L.0.f1c', 0.46, 0.66]] });

A('Pulses & Gates', ['Rhythm Vowels', 'Talking Gate', 'Robot Rhythm', 'Vowel Sequence', 'Chatterbox', 'Mouth Rhythm'], {
  L: [wt('Vowel', 0.2, Object.assign({ uni: 3, det: 10, spr: 0.7 }, lp(0.7, 0.1), AMP(0.05, 1, 0.9, 0.6)))],
  lfo: [LS(5, '1/8')], mods: [['l1', 'A.pos', 0.5]],
  fx: fx(GATE([1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1], '1/16', 0.9, 0.15), REV(0.25, 2.5, 0.6, 0))
}, { vary: [['fx.gate.pat', GATE_PATS], ['lfo.0.div', ['1/8', '1/16', '1/4']], ['L.0.wt', ['Vowel', 'Vocal Pulse', 'Choir']]] });

A('Pulses & Gates', ['Resonant Sequence', 'Filter Steps', 'Stepped Pad', 'Staircase', 'Random Filter Pad', 'Sample and Hold Pad', 'Computer Pad'], {
  L: [saw(Object.assign({ uni: 3, det: 12, spr: 0.7 }, lp(0.42, 0.55), AMP(0.1, 1, 0.9, 0.8)))],
  lfo: [LS(5, '1/16')], mods: [['l1', 'A.f1c', 0.22]],
  fx: fx(DLY(0.2, '1/8.', 0.4), REV(0.25, 2.5, 0.6, 0))
}, { vary: [['lfo.0.div', ['1/16', '1/8', '1/8T']], ['L.0.f1r', 0.4, 0.7], ['mods.0.a', 0.14, 0.3]] });

A('Drones', ['Organ Drone', 'Shruti Box', 'Bagpipe Drone', 'Cathedral Hum', 'Harmonium Drone', 'Reed Drone'], {
  L: [wt('Reed', 0.4, Object.assign({ uni: 3, det: 6, spr: 0.7 }, lp(0.6, 0.08), AMP(2, 2, 1, 4))),
    wt('Reed', 0.6, Object.assign({ crs: 7, lvl: 0.4, uni: 2, det: 5 }, lp(0.6, 0.08), AMP(2.5, 2, 1, 4))),
    wt('Drawbars', 0.2, Object.assign({ crs: -12, lvl: 0.5, f1t: 'off' }, AMP(2, 2, 1, 4)))],
  lfo: [LF(6, 0.06)], mods: [['l1', 'A.pos', 0.2], ['l1', 'B.lvl', 0.3]],
  fx: fx(REV(0.45, 6, 0.85, 0))
}, { vary: [['L.1.crs', [7, 12, 19]], ['L.0.pos', 0.2, 0.7]] });

A('Drones', ['Feedback Drone', 'Comb Drone', 'Resonator Hum', 'Pipe Resonance', 'Tunnel Tone', 'Wire Drone', 'Power Line'], {
  L: [noise(0, Object.assign({ ncol: 0.5, spr: 0.6, f1t: 'comb', f1c: 0.42, f1r: 0.94, f1k: 1, f2t: 'lp12', f2c: 0.7 }, AMP(2, 2, 1, 4)))],
  lfo: [LF(6, 0.05)], mods: [['l1', 'A.f1c', 0.004], ['l1', 'A.ncol', 0.3]],
  fx: fx(REV(0.5, 7, 0.9, 3), CHO(0.3, 0.2, 0.6, 0))
}, { vary: [['L.0.f1r', 0.88, 0.97], ['L.0.f2c', 0.55, 0.8], ['L.0.f1t', ['comb', 'combn']]] });

A('Drones', ['Fifth Drone', 'Open Fifths', 'Perfect Drone', 'Dawn Drone', 'Raga Bed', 'Meditation Drone'], {
  L: [tri(Object.assign({ uni: 3, det: 6, spr: 0.8, f1t: 'off' }, AMP(3, 2, 1, 5))),
    tri(Object.assign({ crs: 7, lvl: 0.6, uni: 3, det: 6, spr: 0.9, f1t: 'off' }, AMP(3.5, 2, 1, 5))),
    sine(Object.assign({ crs: -12, lvl: 0.5 }, AMP(3, 2, 1, 5)))],
  lfo: [LF(0, 0.07), LF(0, 0.05)], mods: [['l1', 'A.lvl', 0.3], ['l2', 'B.lvl', 0.3]],
  fx: fx(REV(0.5, 8, 0.9, 3), CHO(0.3, 0.25, 0.5, 0))
}, { vary: [['L.0.shp', 0.1, 0.5], ['L.1.crs', [7, 19, 12]]] });

A('Textures & Noise', ['Fire Crackle', 'Bonfire', 'Geiger', 'Popcorn Static', 'Dust Storm', 'Hail'], {
  L: [noise(1, Object.assign({ ncol: 0.9, kt: 0, spr: 0.7 }, AMP(0.1, 1, 1, 0.8))),
    noise(0, Object.assign({ ncol: 0.15, kt: 0, lvl: 0.2 }, lp(0.4, 0.1, { f1k: 0 }), AMP(0.3, 1, 1, 1)))],
  fx: fx(REV(0.15, 1.5, 0.4, 1))
}, { vary: [['L.0.ncol', 0.5, 1], ['L.1.lvl', 0.05, 0.4]] });

A('Textures & Noise', ['Tuned Noise', 'Whistling Wind', 'Bottle Blow', 'Pipe Air', 'Flute Noise', 'Singing Wires', 'Ghost Flute'], {
  L: [noise(0, Object.assign({ ncol: 0.6, spr: 0.4, lvl: 1.5 }, bp(0.6, 0.85, { f1k: 1 }), AMP(0.3, 1, 1, 0.8)))],
  lfo: [LF(6, 0.4)], mods: [['l1', 'A.f1c', 0.01], ['l1', 'A.lvl', 0.3]],
  fx: fx(REV(0.4, 4, 0.75, 0), DLY(0.15, '1/4', 0.4))
}, { vary: [['L.0.f1r', 0.7, 0.95], ['L.0.f1c', 0.52, 0.68], ['L.0.aa', 0.05, 0.8]] });

A('Textures & Noise', ['Bit Rot', 'Data Corruption', 'Modem', 'Broken Console', 'Cartridge Glitch', 'Dial Up'], {
  L: [noise(3, Object.assign({ ncol: 0.7 }, AMP(0.005, 1, 1, 0.2))),
    wt('Bit Sine', 0.6, Object.assign({ lvl: 0.4, ring: 0.5, rngr: 3.3 }, AMP(0.005, 1, 1, 0.2)))],
  lfo: [LF(5, 9)], mods: [['l1', 'A.pitch', 0.3], ['l1', 'B.pos', 0.3]],
  fx: fx(LOFI(1, { bits: 5, rate: 6, wow: 0, flut: 0, noise: 0.1, tone: 0.75 }), DLY(0.15, '1/16', 0.3))
}, { vary: [['lfo.0.rate', 3, 18], ['fx.lofi.bits', 3, 7], ['L.1.rngr', [3.3, 1.5, 5.1, 7.7]]] });

A('Granular', ['Grain Guitar', 'Guitar Clouds', 'String Dust', 'Shattered Strum', 'Nylon Mist', 'Six String Haze'], {
  L: [gran('Guitar', Object.assign({ gpos: 0.15, gspr: 0.2, gsz: 0.55, gden: 0.6, spr: 0.85, gscan: 0.06 }, lp12(0.78), AMP(0.3, 1, 1, 2.5)))],
  fx: fx(REV(0.42, 5, 0.8, 0), DLY(0.18, '1/4', 0.4))
}, { vary: [['L.0.gpos', 0.05, 0.5], ['L.0.gsz', 0.4, 0.7], ['L.0.gscan', [0, 0.04, 0.1, -0.05]]] });

A('Granular', ['Octave Cloud', 'Shimmer Grains', 'Angel Dust', 'Rising Particles', 'Halo Grains', 'Pitch Rain'], {
  L: [gran('Glass Pad', Object.assign({ gpos: 0.4, gspr: 0.4, gsz: 0.5, gden: 0.6, spr: 0.9 }, AMP(0.5, 1, 1, 3))),
    gran('Glass Pad', Object.assign({ crs: 12, lvl: 0.5, gpos: 0.5, gspr: 0.5, gsz: 0.35, gden: 0.55, gpit: 0.02, spr: 1 }, AMP(0.8, 1, 1, 3)))],
  fx: fx(REV(0.55, 7, 0.9, 0), DLY(0.2, '1/8.', 0.45))
}, { vary: [['L.0.src', ['Glass Pad', 'Choir Aah', 'Strings', 'Bells']], ['L.1.crs', [12, 19, 24]]], over: { 'Octave Cloud': p => { p.L[1].src = p.L[0].src; } } });

A('Granular', ['Stutter Grains', 'Glitch Cloud', 'Skipping CD', 'Buffer Freeze', 'Micro Loop', 'Broken Sampler', 'Jammed Tape'], {
  L: [gran('Synth Chord', Object.assign({ gpos: 0.3, gspr: 0.02, gsz: 0.3, gden: 0.3, spr: 0.4, lvl: 1.5 }, AMP(0.01, 1, 1, 0.3)))],
  lfo: [LS(5, '1/8')], mods: [['l1', 'A.gpos', 0.3]],
  fx: fx(DLY(0.15, '1/16', 0.3), REV(0.2, 2, 0.5, 1))
}, { vary: [['L.0.src', ['Synth Chord', 'Vox', 'Kalimba', 'Felt Piano', 'Organ Chord']], ['L.0.gden', 0.2, 0.42], ['lfo.0.div', ['1/8', '1/16', '1/4']]] });

A('Sweeps & FX', ['Tape Stop', 'Power Down', 'Wind Down', 'Battery Dying', 'Slow Halt'], {
  L: [saw(Object.assign({ uni: 3, det: 12, spr: 0.7 }, lp(0.7, 0.1), AMP(0.005, 1.6, 0, 0.3)))],
  me: [ME(0.001, 1.8, 0, 0.3)], mods: [['e1', 'A.pitch', 0.9], ['e1', 'A.f1c', 0.3]],
  fx: fx(REV(0.2, 2, 0.5, 1))
}, { vary: [['me.0.d', 0.8, 3], ['L.0.shp', [0.667, 1, 0.4]]], over: {} });

A('Sweeps & FX', ['UFO', 'Flying Saucer', 'Theremin Wobble', 'Alien Signal', 'Tractor Beam', 'Sci-Fi Sweep', 'Probe'], {
  L: [sine(Object.assign({ ring: 0.4, rngr: 1.01 }, AMP(0.2, 1, 1, 1)))],
  lfo: [LF(0, 4), LF(0, 0.3)], mods: [['l1', 'A.pitch', 0.06], ['l2', 'G.l1r', 0.5], ['l2', 'A.pitch', 0.15]],
  fx: fx(DLY(0.3, '1/8.', 0.5), REV(0.35, 4, 0.75, 3), PHA(0.4, 0.2, 0.7, 0.5))
}, { vary: [['lfo.0.rate', 2, 9], ['lfo.1.rate', 0.1, 0.8], ['L.0.rngr', [1.01, 1.5, 2.02]]] });

A('Sweeps & FX', ['Reverse Cymbal', 'Suck Up', 'Breath In', 'White Riser', 'Air Swell', 'Vacuum'], {
  L: [noise(0, Object.assign({ ncol: 0.9, kt: 0, spr: 0.8 }, hp(0.5, 0.2, { f1k: 0 }), AMP(2.5, 0.1, 0, 0.1)))],
  me: [ME(2.5, 0.1, 0, 0.1)], mods: [['e1', 'A.f1c', 0.3]],
  fx: fx(REV(0.3, 2.5, 0.6, 0))
}, { vary: [['L.0.aa', 1, 5], ['L.0.ncol', 0.6, 1]], over: {} });

A('Sweeps & FX', ['Glitch Hit', 'Digital Stab', 'Error', 'Bit Crash', 'Crunch Hit'], {
  L: [wt('Digital Saw', 0.7, Object.assign({ ring: 0.6, rngr: 2.7 }, AMP(0.001, 0.3, 0, 0.1)))],
  me: [ME(0, 0.1, 0, 0.1)], mods: [['e1', 'A.pitch', -0.6], ['e1', 'A.pos', -0.5]],
  fx: fx(LOFI(1, { bits: 5, rate: 5, wow: 0, flut: 0, noise: 0, tone: 0.8 }), DLY(0.2, '1/16', 0.4), REV(0.2, 1.5, 0.4, 1))
}, { vary: [['L.0.rngr', [2.7, 1.3, 4.4, 6.1]], ['mods.0.a', -1, 0.8], ['me.0.d', 0.04, 0.25]] });

A('Trap & Drill', ['Trap Pluck', 'Dark Pluck', 'Opium Pluck', 'Vamp Pluck', 'Night Pluck', 'Ice Pick', 'Detroit Pluck', 'Cold Keys Pluck'], {
  L: [sq(Object.assign({ pw: 0.3, uni: 2, det: 8, spr: 0.5 }, lp(0.4, 0.2), F(0.001, 0.2, 0, 0.15, 0.35), AMP(0.001, 0.5, 0, 0.3))),
    modal('Glock', Object.assign({ crs: 12, lvl: 0.3, mdec: 0.6, mbri: 0.55 }, AMP(0.001, 1, 1, 0.2)))],
  fx: fx(DLY(0.25, '1/8T', 0.35), REV(0.3, 3, 0.65, 0))
}, { vary: [['L.0.pw', 0.15, 0.5], ['L.0.f1c', 0.32, 0.5], ['L.1.lvl', 0.1, 0.45]], kit: 'pluck' });

A('Trap & Drill', ['Trap Choir', 'Evil Choir', 'Dark Monks', 'Gregorian Trap', 'Vampire Choir', 'Cult Choir', 'Demon Hymn'], {
  L: [vox(Object.assign({ crs: -12, vow: 0.85, fsh: 0.4, uni: 5, det: 14, spr: 0.85, brth: 0.1 }, lp12(0.66), AMP(0.15, 1, 1, 1))),
    vox(Object.assign({ lvl: 0.4, vow: 0.1, uni: 3, det: 12, spr: 0.9 }, lp12(0.7), AMP(0.25, 1, 1, 1)))],
  lfo: [LF(0, 4.8, { rtg: 1, fade: 0.7 })], mods: [['l1', '*.pitch', 0.005]],
  fx: fx(REV(0.45, 5, 0.85, 0), EQ(2, 0, 1000, -2))
}, { vary: [['L.0.vow', 0.6, 1], ['L.1.lvl', 0.2, 0.55]], kit: 'vox' });

A('Trap & Drill', ['Trap Guitar', 'Sad Guitar Loop', 'Emo Trap Guitar', 'Spanish Trap', 'Lonely Guitar', 'Rain Guitar', 'Heartbreak Guitar'], {
  L: [pluck(Object.assign({ pdec: 2.4, pdmp: 0.45, ppos: 0.16, pexc: 0.5, vs: 0.7 }, lp12(0.66, 0.05), AMP(0.001, 1, 1, 0.35)))],
  fx: fx(CHO(0.3, 0.5, 0.5, 0), DLY(0.22, '1/8.', 0.4), REV(0.4, 4, 0.75, 0), LOFI(0.3, { bits: 12, rate: 1, wow: 0.3, flut: 0.1, noise: 0.08, tone: 0.55 }))
}, { vary: [['L.0.pdmp', 0.3, 0.6], ['L.0.f1c', 0.56, 0.74]], kit: 'guitar' });

A('Trap & Drill', ['Trap Brass', 'War Horns', 'Victory Brass', 'Triumph Horns', 'Gladiator Brass', 'Boss Horns'], {
  L: [saw(Object.assign({ uni: 5, det: 14, spr: 0.75 }, lp(0.44, 0.12), F(0.04, 0.5, 0.5, 0.3, 0.3), AMP(0.02, 0.5, 0.9, 0.3))),
    wt('Brass', 0.6, Object.assign({ crs: -12, lvl: 0.5, uni: 2 }, lp(0.5, 0.1), AMP(0.03, 0.5, 0.9, 0.3)))],
  fx: fx(DRV(0.25, 1, 0.55), REV(0.32, 3, 0.7, 0), COMP(-16, 3, 3))
}, { vary: [['L.0.f1e', 0.2, 0.4], ['L.0.f1c', 0.38, 0.52]], kit: 'brass' });

A('Trap & Drill', ['Jersey Bounce Keys', 'Club Stab', 'Bed Squeak Lead', 'Jersey Chord', 'Philly Club Keys', 'Bmore Stab'], {
  L: [saw(Object.assign({ uni: 3, det: 12, spr: 0.6 }, lp(0.56, 0.2), F(0.001, 0.14, 0.1, 0.1, 0.28), AMP(0.001, 0.22, 0, 0.1))),
    sq(Object.assign({ crs: 12, lvl: 0.3, pw: 0.4 }, lp(0.62, 0.1), AMP(0.001, 0.2, 0, 0.1)))],
  fx: fx(REV(0.2, 1.8, 0.5, 1), DLY(0.12, '1/8', 0.25), COMP(-18, 4, 4))
}, { vary: [['L.0.ad', 0.14, 0.35], ['L.0.f1c', 0.48, 0.66]], kit: 'pluck' });

A('Soul & Boom Bap', ['Boom Bap Keys', 'Rhodes Stab', 'Chopped Rhodes', 'Sample Keys', 'Filtered Keys', 'Golden Era Keys', 'Crate Rhodes', 'Soul Stab'], {
  L: [modal('Tine', Object.assign({ mdec: 1.8, mbri: 0.4, mhard: 0.5, vs: 0.6 }, lp(0.46, 0.2), AMP(0.001, 0.6, 0.2, 0.15))),
    sine(Object.assign({ lvl: 0.3 }, AMP(0.002, 0.6, 0.1, 0.15)))],
  fx: fx(DRV(0.2, 1, 0.5), LOFI(0.8, { bits: 10, rate: 2.5, wow: 0.3, flut: 0.2, noise: 0.25, tone: 0.45 }), COMP(-18, 4, 4), REV(0.15, 1.4, 0.4, 1))
}, { vary: [['L.0.f1c', 0.38, 0.56], ['fx.lofi.bits', 8, 12], ['L.0.ad', 0.35, 1]] });

A('Soul & Boom Bap', ['Jazz Guitar Chop', 'Wes Chop', 'Soul Guitar', 'Sample Guitar', 'Blue Note Guitar', 'Dusty Jazz Box'], {
  L: [pluck(Object.assign({ pdec: 1.6, pdmp: 0.65, ppos: 0.2, pexc: 0.4, vs: 0.7 }, lp12(0.54, 0.08), AMP(0.001, 1, 1, 0.2)))],
  fx: fx(LOFI(0.75, { bits: 11, rate: 2, wow: 0.3, flut: 0.2, noise: 0.2, tone: 0.48 }), REV(0.2, 1.8, 0.5, 1))
}, { vary: [['L.0.pdmp', 0.5, 0.75], ['L.0.f1c', 0.46, 0.6]], kit: 'guitar' });

A('Soul & Boom Bap', ['Upright Sample', 'Jazz Bass Chop', 'Blue Note Bass', 'Ron Carter Bass', 'Smoky Bass', 'Filtered Bassline'], {
  L: [pluck(Object.assign({ crs: -12, pdec: 1.4, pdmp: 0.7, ppos: 0.24, pexc: 0.3, vs: 0.7 }, lp(0.4, 0.15, { f1k: 0.5 }), AMP(0.001, 1, 1, 0.12))),
    sine(Object.assign({ crs: -12, lvl: 0.45 }, AMP(0.002, 0.7, 0.3, 0.12)))],
  fx: fx(LOFI(0.6, { bits: 11, rate: 2, wow: 0.25, flut: 0.15, noise: 0.15, tone: 0.45 }), COMP(-18, 3, 3), DRY)
}, { vary: [['L.0.f1c', 0.34, 0.48], ['L.1.lvl', 0.3, 0.6]], kit: 'bass' });

A('Soul & Boom Bap', ['Flute Sample', 'Jazz Flute Chop', 'Soul Flute', 'Blaxploitation Flute', 'Crate Flute', 'Dusty Flute'], {
  L: [lay('analog', Object.assign({ shp: 0.12, nz: 0.07, vs: 0.5 }, lp12(0.66, 0.1), AMP(0.05, 0.4, 0.9, 0.2))),
    noise(0, Object.assign({ ncol: 0.7, lvl: 0.2 }, bp(0.68, 0.35, { f1k: 1 }), AMP(0.04, 0.25, 0.4, 0.2)))],
  lfo: [LF(0, 5.4, { rtg: 1, fade: 0.3 })], mods: [['l1', '*.pitch', 0.009]],
  fx: fx(LOFI(0.8, { bits: 10, rate: 2.2, wow: 0.35, flut: 0.25, noise: 0.25, tone: 0.48 }), REV(0.25, 2.2, 0.55, 1))
}, { vary: [['L.1.lvl', 0.1, 0.3], ['fx.lofi.bits', 8, 12]], kit: 'lead' });

A('Cinematic', ['Low Pulse', 'Heartbeat Sub', 'Ticking Tension', 'Chase Pulse', 'Countdown', 'Thriller Pulse', 'Pursuit'], {
  L: [saw(Object.assign({ crs: -12, uni: 2, det: 8 }, lp(0.3, 0.3), AMP(0.005, 0.5, 0.8, 0.2))),
    sine(Object.assign({ crs: -24, lvl: 0.5 }, AMP(0.005, 0.5, 0.9, 0.2)))],
  lfo: [LS(3, '1/8')], mods: [['l1', 'A.f1c', 0.14], ['l1', '*.lvl', 0.6]],
  fx: fx(DRV(0.2, 1, 0.5), REV(0.25, 2.5, 0.6, 0))
}, { vary: [['lfo.0.div', ['1/8', '1/16', '1/8T', '1/4']], ['L.0.f1c', 0.24, 0.4]] });

A('Cinematic', ['Glass Tension', 'High Anxiety', 'Icy Cluster', 'Horror Glass', 'Whistle Tone', 'Cold Sweat', 'Needle Strings'], {
  L: [modal('Glass', Object.assign({ crs: 12, mbow: 0.8, mdec: 5, mbri: 0.7, minh: 0.4, spr: 0.8 }, AMP(1.5, 1, 1, 2.5))),
    wt('Strings', 0.8, Object.assign({ crs: 13, lvl: 0.25, uni: 4, det: 30, spr: 0.9 }, hp(0.6, 0.1), AMP(2, 1, 0.9, 2.5)))],
  lfo: [LF(6, 0.3)], mods: [['l1', 'B.pitch', 0.01]],
  fx: fx(REV(0.5, 6, 0.85, 3))
}, { vary: [['L.1.crs', [13, 11, 18]], ['L.0.minh', 0.2, 0.8], ['L.1.det', 20, 45]] });

A('Cinematic', ['Hybrid Hit', 'Trailer Slam', 'Big Drum Hit', 'Thunder Hit', 'Cannon', 'Vault Door', 'Earth Shaker'], {
  L: [modal('Drum', Object.assign({ crs: -12, mdec: 2.5, mbri: 0.35, mhard: 0.8, spr: 0.6 }, AMP(0.001, 1, 1, 2))),
    sine(Object.assign({ crs: -12 }, AMP(0.001, 1.5, 0, 0.6))),
    noise(0, Object.assign({ ncol: 0.5, kt: 0, lvl: 0.5, spr: 0.8 }, lp(0.7, 0.1, { f1k: 0 }), F(0.001, 0.8, 0, 0.5, -0.4), AMP(0.001, 1.2, 0, 0.6)))],
  me: [ME(0, 0.12, 0, 0.1)], mods: [['e1', 'B.pitch', 0.5]],
  fx: fx(DRV(0.35, 0, 0.55), REV(0.45, 5, 0.9, 0), COMP(-14, 4, 3))
}, { vary: [['L.0.mdl', ['Drum', 'Gong', 'Plate', 'Wood']], ['L.0.mdec', 1.2, 4], ['L.2.lvl', 0.25, 0.7]] });

A('Cinematic', ['Hopeful Dawn', 'New Beginning', 'First Light', 'Homecoming', 'Wonder', 'Open Sky', 'Morning Mission'], {
  L: [wt('Strings', 0.5, Object.assign({ uni: 4, det: 10, spr: 0.85 }, lp(0.62, 0.06), AMP(0.8, 1, 0.9, 2))),
    modal('Piano', Object.assign({ lvl: 0.5, mdec: 4, mbri: 0.42, mhard: 0.35, minh: 0.5 }, AMP(0.001, 1, 1, 1))),
    tri(Object.assign({ crs: 12, lvl: 0.18, uni: 3, det: 9, spr: 0.9, f1t: 'off' }, AMP(1.5, 1, 0.8, 2.5)))],
  fx: fx(REV(0.45, 5, 0.85, 0), DLY(0.12, '1/4.', 0.35))
}, { vary: [['L.1.lvl', 0.3, 0.7], ['L.0.aa', 0.4, 1.4]] });


/* =========================================================== LIBRARY EXPANSION (v3) */
/* ---- Pads ---- */
A('Pads', ['Morning Haze', 'Linen Pad', 'Peach Sky', 'Cotton Field', 'Lilac Air', 'Soft Harbor', 'Pillow Talk', 'Warm Static', 'Honey Drift', 'Rose Quartz', 'Calm Water', 'Sunday Light'], {
  L: [tri(Object.assign({ uni: 4, det: 12, spr: 0.8, drift: 0.4, vs: 0.3 }, lp(0.48, 0.1), F(1, 2, 0.6, 2.5, 0.12), AMP(1.1, 2, 0.9, 3))),
    saw(Object.assign({ crs: 12, lvl: 0.3, uni: 3, det: 14, spr: 0.9 }, lp12(0.55, 0.05), AMP(1.4, 2, 0.85, 3.2)))],
  lfo: [LF(0, 0.08)], mods: [['l1', 'A.f1c', 0.05], ['l1', 'B.pan', 0.3]],
  fx: fx(CHO(0.4, 0.3, 0.55, 1), REV(0.36, 5, 0.75, 0))
}, { vary: [['L.0.f1c', 0.4, 0.56], ['L.1.crs', [12, 7, 19, 0]], ['L.0.det', 8, 18], ['fx.rev.decay', 4, 7]] });

A('Pads', ['Vapor Pad', 'Pastel Grid', 'Mall Fountain', 'Plaza Dream', 'Arcade Sunset', 'Laser Disc', 'Hologram Pad', 'Chrome Lagoon', 'VHS Heaven', 'Floppy Pad', 'Dial-Up Dream', 'Virtual Plaza'], {
  L: [sq(Object.assign({ pw: 0.42, uni: 3, det: 10, spr: 0.7, drift: 0.3 }, lp(0.5, 0.18), AMP(0.5, 1.5, 0.85, 2))),
    wt('Digital Saw', 0.3, Object.assign({ lvl: 0.35, uni: 2, det: 8, spr: 0.8 }, lp12(0.6), AMP(0.6, 1.5, 0.85, 2.2)))],
  lfo: [LF(1, 0.4)], mods: [['l1', 'A.pw', 0.2]],
  fx: fx(CHO(0.5, 0.45, 0.6, 1), LOFI(0.45, { bits: 11, rate: 1.4, wow: 0.45, flut: 0.2, noise: 0.08, tone: 0.55 }), REV(0.35, 4, 0.7, 0))
}, { vary: [['L.0.f1c', 0.42, 0.58], ['L.1.wt', ['Digital Saw', 'Bit Sine', 'PWM']], ['fx.lofi.wow', 0.2, 0.6]] });

A('Pads', ['Gospel Pad', 'Church Clouds', 'Sanctuary', 'Revival Pad', 'Choir Loft', 'Sunday Service', 'Praise Wash', 'Altar Light', 'Pew Glow', 'Amen Pad', 'Hymnal Air', 'Steeple Sky'], {
  L: [wt('Drawbars', 0.45, Object.assign({ uni: 3, det: 7, spr: 0.7 }, lp12(0.62), AMP(0.7, 1.5, 0.9, 2.2))),
    vox(Object.assign({ lvl: 0.35, vow: 0.05, uni: 4, det: 12, spr: 0.85, brth: 0.08 }, AMP(1, 1.5, 0.9, 2.4)))],
  lfo: [LF(0, 5.2, { rtg: 1, fade: 1 })], mods: [['l1', '*.pitch', 0.004]],
  fx: fx(CHO(0.3, 0.6, 0.5, 1), REV(0.42, 5.5, 0.85, 0))
}, { vary: [['L.0.pos', 0.3, 0.6], ['L.1.vow', 0, 0.2], ['L.1.lvl', 0.2, 0.45]] });

/* ---- Atmospheres ---- */
A('Atmospheres', ['Desert Night', 'Heat Shimmer', 'Mirage Air', 'Canyon Wind', 'Dust Devil', 'Dry Lake', 'Salt Flats', 'Red Rock', 'Sand Whisper', 'Oasis Fog', 'Sun Bleach', 'Cactus Moon'], {
  L: [noise(4, Object.assign({ ncol: 0.5, kt: 0, spr: 0.9 }, bp(0.55, 0.4, { f1k: 0 }), AMP(2, 2, 1, 4))),
    wt('Ghost', 0.4, Object.assign({ uni: 3, det: 12, spr: 0.85, lvl: 0.4 }, lp12(0.55), AMP(2.5, 2, 0.8, 4)))],
  lfo: [LF(6, 0.06), LF(0, 0.13)], mods: [['l1', 'A.f1c', 0.15], ['l2', 'B.pos', 0.3]],
  fx: fx(REV(0.5, 8, 0.9, 3), DLY(0.15, '1/4.', 0.5, { hc: 3500 }))
}, { vary: [['L.0.ncol', 0.3, 0.7], ['L.1.wt', ['Ghost', 'Hollow', 'Glass', 'Vowel']], ['L.1.crs', [0, 12, -12]]] });

A('Atmospheres', ['Deep Space Radio', 'Satellite Hum', 'Orbit Station', 'Moon Base', 'Signal Lost', 'Starlink', 'Radio Telescope', 'Cosmic Drift', 'Quiet Nebula', 'Event Horizon Air', 'Dark Matter Hum', 'Solar Breeze'], {
  L: [sine(Object.assign({ fm: 0.25, fmr: 1.41, uni: 3, det: 8, spr: 0.9 }, AMP(3, 2, 1, 5))),
    noise(3, Object.assign({ ncol: 0.6, kt: 0, lvl: 0.15, spr: 0.8 }, bp(0.6, 0.5, { f1k: 0 }), AMP(1, 1, 1, 4)))],
  lfo: [LF(6, 0.05), LF(5, 0.4)], mods: [['l1', 'A.fm', 0.15], ['l2', 'B.f1c', 0.2]],
  fx: fx(REV(0.55, 10, 0.95, 3), PHA(0.3, 0.05, 0.7, 0.5))
}, { vary: [['L.0.fmr', [1.41, 2.76, 0.5, 3.5]], ['L.0.crs', [0, -12, 12]], ['L.1.ntyp', [3, 0, 4]]] });

A('Atmospheres', ['Forest Floor', 'Morning Dew', 'Rainforest Air', 'Mossy Stones', 'Birch Grove', 'Fern Valley', 'Wild Meadow', 'Pine Needles', 'Creek Bed', 'Bamboo Air', 'Willow Shade', 'Firefly Field'], {
  L: [gran('Glass Pad', Object.assign({ gpos: 0.5, gspr: 0.3, gsz: 0.5, gden: 0.55, spr: 0.9, gscan: 0.03, gpit: 0.1 }, AMP(2, 2, 1, 4))),
    noise(2, Object.assign({ ncol: 0.5, kt: 0, lvl: 0.2, spr: 0.9 }, AMP(1.5, 1, 1, 4)))],
  lfo: [LF(6, 0.1)], mods: [['l1', 'A.gpos', 0.2]],
  fx: fx(REV(0.5, 7, 0.85, 0), DLY(0.2, '1/4.', 0.45))
}, { vary: [['L.0.src', ['Glass Pad', 'Kalimba', 'Bells', 'Choir Aah']], ['L.1.ntyp', [2, 4, 1]], ['L.0.gpit', 0, 0.3]] });

/* ---- Keys ---- */
A('Keys', ['Velvet Rhodes', 'Late Set Keys', 'Smoky Tines', 'Lounge Rhodes', 'Mellow Stage', 'Satin Keys', 'Uptown Tines', 'Quiet Storm Keys', 'Candle Rhodes', 'Brownstone Keys', 'Silk Suite', 'After Hours EP'], {
  L: [modal('Tine', Object.assign({ mbri: 0.4, mdec: 2.8, mhard: 0.4, vs: 0.7 }, AMP(0.001, 1, 1, 0.5))),
    sine(Object.assign({ lvl: 0.35, fm: 0.12, fmr: 1, vs: 0.8 }, AMP(0.001, 1.6, 0.2, 0.4)))],
  lfo: [LF(0, 4.5)], mods: [['l1', '*.pan', 0.25]],
  fx: fx(CHO(0.2, 0.5, 0.4, 0), REV(0.22, 2.4, 0.55, 1))
}, { vary: [['L.0.mbri', 0.3, 0.55], ['L.0.mdec', 2, 4], ['L.1.fm', 0.05, 0.2]] });

A('Keys', ['Upright Felt', 'Parlor Piano', 'Home Piano', 'Attic Upright', 'Practice Room', 'Living Room Keys', 'Soft Pedal', 'Night Light Piano', 'Grandma Piano', 'Woody Upright', 'Saloon Keys', 'Rehearsal Piano'], {
  L: [modal('Piano', Object.assign({ mbri: 0.42, mdec: 3, mhard: 0.4, minh: 0.4, vs: 0.75 }, lp12(0.72), AMP(0.001, 1, 1, 0.6))),
    modal('Piano', Object.assign({ fin: 7, lvl: 0.35, mbri: 0.35, mdec: 2.5, mhard: 0.3, minh: 0.5, vs: 0.75 }, lp12(0.65), AMP(0.001, 1, 1, 0.6)))],
  fx: fx(REV(0.22, 2, 0.5, 1), EQ(1, -1, 2500, -1))
}, { vary: [['L.0.mbri', 0.32, 0.55], ['L.1.fin', 4, 12], ['L.0.minh', 0.2, 0.6]] });

A('Keys', ['Clav Funk', 'Clavinet Chop', 'Pocket Clav', 'Wah Strut', 'Stank Face', 'Superstition Pluck', 'Clav Strut', 'Rubber Band Clav', 'Go-Go Clav', 'Chicken Scratch Keys', 'Backbeat Clav', 'Groove Clav'], {
  L: [sq(Object.assign({ pw: 0.2, vs: 0.75 }, lp(0.45, 0.35, { f1v: 0.4 }), F(0.001, 0.25, 0.15, 0.1, 0.35), AMP(0.001, 0.6, 0.4, 0.08))),
    pluck(Object.assign({ pdec: 0.8, pdmp: 0.6, ppos: 0.08, pexc: 0.8, lvl: 0.4 }, AMP(0.001, 1, 1, 0.08)))],
  fx: fx(PHA(0.2, 0.6, 0.6, 0.3), DRV(0.15, 1, 0.7), REV(0.12, 1.2, 0.4, 1))
}, { vary: [['L.0.pw', 0.12, 0.3], ['L.0.f1e', 0.25, 0.45], ['fx.pha.mix', 0, 0.35]] });

/* ---- Lo-Fi & Dusty ---- */
A('Lo-Fi & Dusty', ['Dust Jacket Wurli', 'Cassette Keys', 'Warped Rhodes', 'Old Tape Piano', 'Goodwill Keys', 'Sepia Keys', 'Basement Session', 'Homework Keys', 'Wet Glass Keys', 'Sleepy Keys', 'Worn Grooves', 'Pawn Shop Piano'], {
  L: [modal('Tine', Object.assign({ mbri: 0.35, mdec: 2.4, mhard: 0.35, vs: 0.6 }, lp12(0.55), AMP(0.001, 1, 1, 0.45))),
    noise(1, Object.assign({ ncol: 0.3, kt: 0, lvl: 0.12 }, AMP(0.001, 1, 1, 0.4)))],
  fx: fx(LOFI(0.8, { bits: 10, rate: 2, wow: 0.55, flut: 0.3, noise: 0.25, tone: 0.42 }), REV(0.25, 2.2, 0.5, 1), EQ(2, 0, 1000, -4))
}, { vary: [['L.0.mdl', ['Tine', 'Piano', 'Kalimba', 'Vibes']], ['fx.lofi.wow', 0.3, 0.7], ['fx.lofi.tone', 0.3, 0.55]] });

A('Lo-Fi & Dusty', ['Dusty Choir Loop', 'Flea Market Choir', 'Vinyl Voices', 'Crate Aahs', 'Warped Hymn', 'Old Record Choir', 'Tape Angels', 'Bedroom Gospel', 'Faded Vocal', 'Sampled Sanctuary', 'Dollar Bin Voices', 'Crackle Choir'], {
  L: [gran('Choir Chord', Object.assign({ gpos: 0.45, gspr: 0.1, gsz: 0.6, gden: 0.6, spr: 0.6 }, lp12(0.6), AMP(0.15, 1, 1, 1))),
    noise(1, Object.assign({ ncol: 0.35, kt: 0, lvl: 0.1 }, AMP(0.001, 1, 1, 0.6)))],
  fx: fx(LOFI(0.85, { bits: 9, rate: 2.2, wow: 0.5, flut: 0.3, noise: 0.3, tone: 0.4 }), REV(0.25, 2.5, 0.55, 0))
}, { vary: [['L.0.gpos', 0.3, 0.6], ['L.0.src', ['Choir Chord', 'Choir Aah', 'Vox']], ['fx.lofi.bits', 8, 12]] });

A('Lo-Fi & Dusty', ['Lo-Fi Guitar Loop', 'Dusty Six String', 'Bedroom Strum', 'Tape Guitar', 'Warbly Pluck', 'Rainy Guitar', 'Old Folk Pick', 'Washed Strings', 'Cassette Guitar', 'Hazy Pluck', 'Porch Guitar', 'Lazy Sunday Pick'], {
  L: [pluck(Object.assign({ pdec: 2, pdmp: 0.6, ppos: 0.18, pexc: 0.4, vs: 0.6 }, lp12(0.55), AMP(0.001, 1, 1, 0.35)))],
  fx: fx(LOFI(0.75, { bits: 11, rate: 1.6, wow: 0.5, flut: 0.25, noise: 0.2, tone: 0.45 }), CHO(0.2, 0.3, 0.4, 0), REV(0.22, 2, 0.5, 1))
}, { vary: [['L.0.pdmp', 0.5, 0.7], ['L.0.f1c', 0.45, 0.62], ['fx.lofi.wow', 0.3, 0.7]] });

/* ---- Bells & Mallets ---- */
A('Bells & Mallets', ['Toy Glock', 'Nursery Bells', 'Tiny Chimes', 'Lullaby Bells', 'Snow Globe', 'Wind-Up Bells', 'Jewel Box', 'Pocket Glock', 'Pixie Chime', 'Candy Chime', 'Twinkle Box', 'Sugar Bells'], {
  L: [modal('Music Box', Object.assign({ crs: 12, mdec: 1.6, mbri: 0.6, mhard: 0.7 }, AMP(0.001, 1, 1, 0.6)))],
  fx: fx(REV(0.35, 3.5, 0.7, 2), DLY(0.18, '1/8.', 0.35))
}, { vary: [['L.0.mdl', ['Music Box', 'Glock', 'Crystal']], ['L.0.mdec', 1, 2.5], ['L.0.mbri', 0.5, 0.75]] });

A('Bells & Mallets', ['Temple Bowl', 'Healing Bowl', 'Prayer Bell', 'Monk Bell', 'Brass Bowl', 'Zen Gong', 'Mountain Bell', 'Ritual Chime', 'Incense Bell', 'Lotus Bowl', 'Copper Bowl', 'Quiet Temple'], {
  L: [modal('Bell', Object.assign({ mdec: 7, mbri: 0.35, mhard: 0.3, mbow: 0.25, spr: 0.6 }, AMP(0.001, 2, 1, 4)))],
  lfo: [LF(0, 0.3)], mods: [['l1', 'A.mbri', 0.08]],
  fx: fx(REV(0.45, 7, 0.9, 0))
}, { vary: [['L.0.mdl', ['Bell', 'Gong', 'Glass', 'Crystal']], ['L.0.mbow', 0, 0.5], ['L.0.crs', [0, -12, 12]]] });

A('Bells & Mallets', ['Soft Vibes', 'Village Vibes', 'Spinning Vibes', 'Late Show Vibes', 'Cool Vibraphone', 'Smoke Vibes', 'Bebop Bars', 'Velvet Mallets', 'Blue Note Vibes', 'Club Vibes', 'Dream Vibes', 'Tremolo Bars'], {
  L: [modal('Vibes', Object.assign({ mdec: 2.5, mbri: 0.45, mhard: 0.35, vs: 0.7 }, AMP(0.001, 1, 1, 0.8)))],
  lfo: [LF(0, 5)], mods: [['l1', 'A.lvl', 0.25]],
  fx: fx(REV(0.28, 2.6, 0.6, 1), CHO(0.12, 0.4, 0.3, 0))
}, { vary: [['lfo.0.rate', 3.5, 6.5], ['mods.0.a', 0.1, 0.35], ['L.0.mbri', 0.35, 0.55]] });

/* ---- Plucks ---- */
A('Plucks', ['Pizz Pop', 'Bubble Pluck', 'Pop Pluck', 'Drop Pluck', 'Lollipop Pluck', 'Trampoline', 'Gum Pluck', 'Pebble', 'Click Pluck', 'Tick Tock', 'Plink', 'Popcorn'], {
  L: [saw(Object.assign({ uni: 2, det: 8, spr: 0.6 }, lp(0.3, 0.3), F(0.001, 0.14, 0, 0.1, 0.45), AMP(0.001, 0.25, 0, 0.12)))],
  fx: fx(DLY(0.22, '1/8.', 0.35), REV(0.2, 1.8, 0.5, 1))
}, { vary: [['L.0.f1e', 0.35, 0.55], ['L.0.fd', 0.08, 0.22], ['L.0.shp', [0.667, 1, 0.333]], ['L.0.f1r', 0.15, 0.45]] });

A('Plucks', ['Koto Pluck', 'Folk Harp', 'Concert Zither', 'Dulcimer Pluck', 'Lute Pick', 'Kantele', 'Lyre', 'Psaltery', 'Highland Harp', 'Pipa Pluck', 'Bouzouki Pick', 'Autoharp'], {
  L: [pluck(Object.assign({ pdec: 2.5, pdmp: 0.3, ppos: 0.12, pexc: 0.65, uni: 2, det: 5, spr: 0.6 }, AMP(0.001, 1, 1, 0.6)))],
  fx: fx(REV(0.3, 3, 0.65, 0), DLY(0.12, '1/8', 0.3))
}, { vary: [['L.0.pdmp', 0.2, 0.45], ['L.0.ppos', 0.08, 0.2], ['L.0.pdec', 1.5, 4]] });

A('Plucks', ['Glass Pluck', 'Crystal Tap', 'Icicle', 'Prism Pluck', 'Shard', 'Frost Pick', 'Diamond Pluck', 'Mirror Pluck', 'Quartz Tap', 'Clear Drop', 'Sparkler', 'Halo Pluck'], {
  L: [wt('Glass', 0.4, Object.assign({ uni: 2, det: 7, spr: 0.7 }, lp12(0.45, 0.2), F(0.001, 0.3, 0, 0.2, 0.4), AMP(0.001, 0.6, 0, 0.4))),
    sine(Object.assign({ crs: 24, lvl: 0.2 }, AMP(0.001, 0.25, 0, 0.2)))],
  fx: fx(DLY(0.25, '1/8.', 0.4), REV(0.3, 3, 0.7, 2))
}, { vary: [['L.0.pos', 0.2, 0.7], ['L.0.wt', ['Glass', 'FM Bell', 'Metallic', 'Octave Stack']], ['L.1.crs', [24, 19, 31]]] });

/* ---- Leads ---- */
A('Leads', ['Tin Whistle', 'Flute Lead', 'Clean Air Lead', 'Clay Ocarina', 'Bird Call', 'Soft Whistle', 'Reed Whistle', 'Pan Pipe Lead', 'Breathy Lead', 'Air Lead', 'Silver Whistle', 'Night Whistle'], {
  L: [sine(Object.assign({ nz: 0.06, vs: 0.5 }, AMP(0.05, 1, 0.9, 0.25)))],
  lfo: [LF(0, 5.5, { rtg: 1, fade: 0.4 })], mods: [['l1', 'A.pitch', 0.01]],
  g: { vmode: 2, glide: 0.06 },
  fx: fx(REV(0.28, 2.6, 0.6, 0), DLY(0.2, '1/8.', 0.35))
}, { vary: [['L.0.nz', 0.02, 0.12], ['L.0.shp', [0, 0.1, 0.2]], ['g.glide', 0.03, 0.1]], kit: 'lead' });

A('Leads', ['Square Hero', 'Chip Lead', 'Bit Lead', 'Continue Screen', 'Coin Lead', '8-Bit Hero', 'Pixel Lead', 'Boss Stage', 'Cartridge Lead', 'Level Up', 'High Score', 'Joystick'], {
  L: [sq(Object.assign({ pw: 0.25 }, AMP(0.001, 0.4, 0.8, 0.08)))],
  lfo: [LF(4, 8, { rtg: 1, fade: 0.3 })], mods: [['l1', 'A.pitch', 0.004]],
  g: { vmode: 1, glide: 0.02 },
  fx: fx(LOFI(0.5, { bits: 8, rate: 3, wow: 0, flut: 0, noise: 0, tone: 0.8 }), DLY(0.18, '1/8', 0.3), DRY)
}, { vary: [['L.0.pw', [0.125, 0.25, 0.5]], ['fx.lofi.bits', 4, 9]], kit: 'lead' });

A('Leads', ['Talk To Me', 'Zapp Lead', 'Robot Soul', 'Vocoder Lead', 'Cali Talk', 'Funk Talk', 'Mouth Lead', 'Roger Lead', 'Bounce Talk', 'Westcoast Talk', 'Tube Talk', 'Gloss Talk'], {
  L: [vox(Object.assign({ vow: 0.3, fsh: 0.55, brth: 0.02, uni: 1, vs: 0.5 }, AMP(0.01, 0.5, 0.9, 0.15))),
    saw(Object.assign({ lvl: 0.35 }, lp(0.5, 0.2), AMP(0.01, 0.5, 0.9, 0.15)))],
  lfo: [LF(6, 2.2, { rtg: 1 })], mods: [['l1', 'A.vow', 0.35], ['mw', 'A.vow', 0.5]],
  g: { vmode: 2, glide: 0.08 },
  fx: fx(DRV(0.2, 1, 0.6), CHO(0.2, 0.5, 0.4, 0), REV(0.2, 2, 0.5, 1))
}, { vary: [['lfo.0.rate', 1.5, 4], ['L.0.fsh', 0.45, 0.65], ['mods.0.a', 0.2, 0.5]], kit: 'lead' });

/* ---- Synth Bass ---- */
A('Synth Bass', ['Funk Machine Bass', 'Slap Happy', 'Pocket Bass', 'Strut Bass', 'Boogie Bass', 'Groove Bass', 'Disco Octave', 'Elastic Bass', 'Bump Bass', 'Snap Bass', 'Rubber Pocket', 'Cruise Bass'], {
  L: [saw(Object.assign({ vs: 0.7 }, lp(0.35, 0.3, { f1v: 0.4 }), F(0.001, 0.2, 0.2, 0.1, 0.4), AMP(0.001, 0.4, 0.7, 0.08))),
    sq(Object.assign({ crs: -12, lvl: 0.4 }, lp(0.3), AMP(0.001, 0.4, 0.7, 0.08)))],
  g: { vmode: 1, glide: 0.03 },
  fx: fx(DRV(0.2, 1, 0.6), COMP(-16, 4, 3), DRY)
}, { vary: [['L.0.f1e', 0.3, 0.5], ['L.0.f1r', 0.2, 0.45], ['L.0.fd', 0.12, 0.3]], kit: 'bass' });

A('Synth Bass', ['Moog Thump', 'Ladder Low', 'Minimoog Floor', 'Tape Bass', 'Analog Thump', 'Warm Ladder', 'Fat Finger', 'Old School Low', 'Studio A Bass', 'Round Moog', 'Tube Thump', 'Classic Low'], {
  L: [saw(Object.assign({ drift: 0.3 }, ladderF(0.3, 0.25, { f1k: 0.3, f1v: 0.3 }), F(0.001, 0.35, 0.3, 0.2, 0.3), AMP(0.001, 0.5, 0.85, 0.12))),
    sq(Object.assign({ crs: -12, lvl: 0.5, drift: 0.3 }, ladderF(0.3), AMP(0.001, 0.5, 0.85, 0.12)))],
  g: { vmode: 2, glide: 0.04 },
  fx: fx(DRV(0.25, 1, 0.55), DRY)
}, { vary: [['L.0.f1c', 0.22, 0.4], ['L.0.f1r', 0.1, 0.4], ['L.1.crs', [-12, 0, -24]]], kit: 'bass' });

A('Synth Bass', ['Neuro Low', 'Grime Bass', 'Gutter Bass', 'Growl Unit', 'Snarl Bass', 'Metal Jaw', 'Ripper', 'Static Bass', 'Bite Bass', 'Wire Bass', 'Grit Floor', 'Riot Bass'], {
  L: [wt('Growl', 0.4, Object.assign({ fold: 0.15 }, lp(0.45, 0.3), AMP(0.001, 0.5, 0.9, 0.1))),
    sine(Object.assign({ crs: -12, lvl: 0.55 }, AMP(0.001, 0.5, 1, 0.1)))],
  lfo: [LS(0, '1/8', { rtg: 1 })], mods: [['l1', 'A.pos', 0.35], ['l1', 'A.f1c', 0.15]],
  g: { vmode: 1, glide: 0.02 },
  fx: fx(DRV(0.45, 3, 0.6), COMP(-18, 4, 3), DRY)
}, { vary: [['lfo.0.div', ['1/8', '1/16', '1/4', '1/8T']], ['L.0.wt', ['Growl', 'Comb', 'Phase Dist', 'FM 2:1']], ['L.0.fold', 0, 0.3]], kit: 'bass' });

/* ---- 808 & Sub ---- */
A('808 & Sub', ['808 Zone 6', '808 Third Ward', '808 Orange Mound', '808 Miami', '808 New York', '808 London', '808 Toronto', '808 Philly', '808 Baltimore', '808 DMV', '808 Oakland', '808 New Orleans'], {
  L: [sine(Object.assign({ vs: 0.35 }, AMP(0.001, 2.5, 0, 0.2)))],
  me: [ME(0, 0.05, 0, 0.05)], mods: [['e1', 'A.pitch', 0.38]],
  g: { vmode: 2, glide: 0.08, gmode: 1 },
  fx: fx(DRV(0.35, 1, 0.5), COMP(-14, 3, 2), DRY)
}, { vary: [['L.0.ad', 1, 4], ['mods.0.a', 0.2, 0.55], ['fx.drv.amt', 0.15, 0.7], ['fx.drv.type', [0, 1, 3]], ['g.glide', 0.04, 0.15], ['L.0.shp', [0, 0, 0.1, 0.2]]], kit: 's808' });

A('808 & Sub', ['Sub Hum', 'Floor Shaker', 'Low Rumble', 'Basement Sub', 'Pulse Sub', 'Pressure Sub', 'Quiet Sub', 'Round Sub', 'Clean Sub', 'Deep Pocket Sub', 'Weight', 'Earthquake Sub'], {
  L: [sine(Object.assign({ vs: 0.25, sub: 0.15 }, AMP(0.005, 0.5, 1, 0.15)))],
  g: { vmode: 2, glide: 0.03 },
  fx: fx(DRV(0.08, 1, 0.4), DRY)
}, { vary: [['L.0.shp', [0, 0, 0.1]], ['L.0.sub', 0, 0.3], ['fx.drv.amt', 0, 0.2]], kit: 's808' });

A('808 & Sub', ['808 Distorted Glide', '808 Clipper', '808 Blown Out', '808 Fuzz Box', '808 Grind', '808 Crushed', '808 Overdrive', '808 Saturated', '808 Hot', '808 Redline', '808 Burnt', '808 Smash'], {
  L: [sine(Object.assign({ vs: 0.3, fold: 0.2 }, AMP(0.001, 2, 0.3, 0.2)))],
  me: [ME(0, 0.04, 0, 0.04)], mods: [['e1', 'A.pitch', 0.45]],
  g: { vmode: 2, glide: 0.1, gmode: 1 },
  fx: fx(DRV(0.65, 2, 0.5), COMP(-16, 4, 1), DRY)
}, { vary: [['fx.drv.type', [1, 2, 3, 4]], ['fx.drv.amt', 0.45, 0.85], ['L.0.fold', 0.05, 0.35], ['L.0.ad', 1, 3]], kit: 's808' });

/* ---- Strings ---- */
A('Strings', ['Sound of Philly', 'Philly Strings', 'Motown Strings', 'Studio 54 Strings', 'Silk Section', 'Sweet Strings', 'Velvet Section', 'Ballad Strings', 'Golden Strings', 'Uptown Strings', 'Love Strings', 'Satin Section'], {
  L: [saw(Object.assign({ uni: 6, det: 13, spr: 0.9, drift: 0.4, vs: 0.5 }, lp(0.55, 0.05), AMP(0.25, 1, 0.9, 0.9))),
    saw(Object.assign({ crs: 12, lvl: 0.3, uni: 4, det: 11, spr: 0.9 }, lp(0.6), AMP(0.3, 1, 0.9, 0.9)))],
  lfo: [LF(0, 5.2, { rtg: 1, fade: 0.8 })], mods: [['l1', '*.pitch', 0.005]],
  fx: fx(CHO(0.3, 0.5, 0.5, 1), REV(0.32, 3, 0.7, 0), EQ(-2, 0, 1000, 2))
}, { vary: [['L.0.f1c', 0.48, 0.62], ['L.1.lvl', 0.15, 0.4], ['L.0.aa', 0.1, 0.4]] });

A('Strings', ['Pizzicato Synth', 'Pizz Section', 'Plucky Cellos', 'Pizz Choir Section', 'Tip Toe Strings', 'Sneaky Pizz', 'Cartoon Pizz', 'Bright Pizz', 'Short Strings', 'Spiccato Synth', 'Staccato Section', 'Pizz Stabs'], {
  L: [pluck(Object.assign({ pdec: 0.6, pdmp: 0.55, ppos: 0.3, pexc: 0.5, uni: 3, det: 8, spr: 0.8, vs: 0.7 }, lp12(0.6), AMP(0.001, 0.5, 0, 0.2)))],
  fx: fx(REV(0.3, 2.5, 0.7, 0))
}, { vary: [['L.0.pdec', 0.4, 1], ['L.0.pdmp', 0.45, 0.7], ['L.0.crs', [0, -12, 12]]] });

A('Strings', ['Tremolo Section', 'Shiver Strings', 'Suspense Strings', 'Nervous Bows', 'Thriller Strings', 'Flutter Section', 'Trembling Air', 'Storm Strings', 'Anxious Bows', 'Haunted Section', 'Frozen Tremolo', 'Shadow Strings'], {
  L: [saw(Object.assign({ uni: 5, det: 12, spr: 0.9, drift: 0.4 }, lp(0.5, 0.08), AMP(0.15, 1, 0.9, 0.8)))],
  lfo: [LS(4, '1/32', { rtg: 1 })], mods: [['l1', 'A.lvl', 0.45]],
  fx: fx(REV(0.4, 4, 0.8, 0))
}, { vary: [['lfo.0.div', ['1/32', '1/16T', '1/16']], ['L.0.f1c', 0.4, 0.6], ['L.0.crs', [0, -12]]] });

/* ---- Brass & Winds ---- */
A('Brass & Winds', ['Muted Horn', 'Wah Mute', 'Bebop Trumpet', 'Smoky Horn', 'Cool Trumpet', 'Late Trumpet', 'Club Horn', 'Velvet Trumpet', 'Lonesome Horn', 'Midnight Mute', 'Quiet Horn', 'Cup Mute'], {
  L: [wt('Brass', 0.35, Object.assign({ vs: 0.6 }, bp(0.55, 0.3, { f1v: 0.4 }), F(0.04, 0.4, 0.6, 0.2, 0.15), AMP(0.03, 0.5, 0.85, 0.2)))],
  lfo: [LF(0, 5.5, { rtg: 1, fade: 0.5 })], mods: [['l1', 'A.pitch', 0.008]],
  g: { vmode: 2, glide: 0.04 },
  fx: fx(REV(0.28, 2.6, 0.6, 1))
}, { vary: [['L.0.pos', 0.2, 0.5], ['L.0.f1c', 0.48, 0.62]] });

A('Brass & Winds', ['Alto Sax Synth', 'Tenor Sax Synth', 'Smooth Sax', 'Bar Sax', 'Breathy Sax', 'Night Sax', 'Soul Sax', 'Honk Sax', 'Velvet Reed', 'Street Sax', 'Lounge Reed', 'Silk Reed'], {
  L: [wt('Reed', 0.45, Object.assign({ vs: 0.6, nz: 0.04 }, lp(0.5, 0.12, { f1v: 0.5 }), F(0.03, 0.5, 0.6, 0.2, 0.2), AMP(0.03, 0.5, 0.9, 0.2)))],
  lfo: [LF(0, 5, { rtg: 1, fade: 0.4 })], mods: [['l1', 'A.pitch', 0.01], ['at', 'A.f1c', 0.15]],
  g: { vmode: 2, glide: 0.05 },
  fx: fx(DRV(0.1, 1, 0.6), REV(0.25, 2.4, 0.6, 1))
}, { vary: [['L.0.pos', 0.3, 0.6], ['L.0.crs', [0, -12]], ['L.0.nz', 0.02, 0.08]] });

A('Brass & Winds', ['Go-Go Horns', 'Chuck Brown Horns', 'Pocket Horns', 'Crank Horns', 'Rare Essence Stab', 'Backyard Horns', 'Bounce Horns', 'Conga Horns', 'Street Band', 'Block Party Horns', 'Carnival Brass', 'March Stab'], {
  L: [saw(Object.assign({ uni: 4, det: 12, spr: 0.7, vs: 0.6 }, lp(0.45, 0.15, { f1v: 0.5 }), F(0.02, 0.3, 0.5, 0.15, 0.35), AMP(0.01, 0.35, 0.7, 0.12))),
    wt('Brass', 0.6, Object.assign({ crs: 12, lvl: 0.35, uni: 2, det: 8 }, lp(0.55), AMP(0.02, 0.35, 0.7, 0.12)))],
  fx: fx(DRV(0.15, 1, 0.65), REV(0.2, 1.8, 0.5, 1), COMP(-16, 3, 2))
}, { vary: [['L.0.f1e', 0.25, 0.45], ['L.1.crs', [12, 7, 0]]] });

/* ---- Choir & Vox ---- */
A('Choir & Vox', ['R&B Stack', 'Background Oohs', 'Harmony Stack', 'Silk Oohs', 'Hook Stack', 'Pillow Oohs', 'Velvet Stack', 'Late Night Oohs', 'Group Hum', 'Stacked Mmm', 'Choir Mmm', 'Warm Oohs'], {
  L: [vox(Object.assign({ vow: 0.85, fsh: 0.55, uni: 5, det: 12, spr: 0.9, brth: 0.1 }, lp12(0.7), AMP(0.25, 1, 1, 1))),
    vox(Object.assign({ crs: 12, lvl: 0.3, vow: 0.8, fsh: 0.6, uni: 3, det: 10, spr: 0.9, brth: 0.12 }, lp12(0.75), AMP(0.35, 1, 1, 1)))],
  lfo: [LF(0, 5, { rtg: 1, fade: 0.6 })], mods: [['l1', '*.pitch', 0.005]],
  fx: fx(REV(0.4, 4, 0.8, 0), CHO(0.25, 0.4, 0.5, 1))
}, { vary: [['L.0.vow', 0.7, 1], ['L.1.crs', [12, 7, 4]], ['L.0.fsh', 0.45, 0.62]] });

A('Choir & Vox', ['Vocal Chop Ah', 'Chop Vox', 'Pitched Vocal', 'Hook Chop', 'Uh Chop', 'Vox Stab', 'Soul Vox Chop', 'Flip Vox', 'Cut Vocal', 'Slice Vox', 'Bounce Vox', 'Club Chop'], {
  L: [vox(Object.assign({ vow: 0.05, fsh: 0.6, brth: 0.1, vs: 0.6 }, AMP(0.005, 0.3, 0, 0.12)))],
  lfo: [LF(6, 3, { rtg: 1 })], mods: [['l1', 'A.vow', 0.3]],
  fx: fx(DLY(0.2, '1/8.', 0.3), REV(0.25, 2, 0.6, 2), COMP(-18, 3, 2))
}, { vary: [['L.0.vow', 0, 0.5], ['L.0.ad', 0.15, 0.5], ['L.0.fsh', 0.5, 0.7]] });

A('Choir & Vox', ['Whisper Choir', 'Breath Voices', 'Ghost Whisper', 'Secret Choir', 'Hushed Voices', 'Wind Voices', 'Shadow Choir', 'Night Breath', 'Phantom Aahs', 'Murmur', 'Vapor Voices', 'Hollow Whisper'], {
  L: [vox(Object.assign({ vow: 0.1, brth: 0.75, uni: 4, det: 15, spr: 0.9 }, lp12(0.7), AMP(1, 1.5, 1, 2))),
    noise(0, Object.assign({ ncol: 0.7, kt: 0, lvl: 0.12 }, bp(0.7, 0.3, { f1k: 0 }), AMP(1, 1, 1, 2)))],
  lfo: [LF(6, 0.2)], mods: [['l1', 'A.vow', 0.3]],
  fx: fx(REV(0.55, 7, 0.9, 0))
}, { vary: [['L.0.brth', 0.6, 0.95], ['L.0.vow', 0, 0.4]] });

/* ---- Organs ---- */
A('Organs', ['Gospel B3', 'Sunday Organ', 'Church Drawbars', 'Praise Organ', 'Shout Organ', 'Revival Organ', 'Holy Ghost Organ', 'Deacon Organ', 'Testify', 'Pentecost Organ', 'Choir Loft Organ', 'Amen Corner'], {
  L: [wt('Drawbars', 0.7, Object.assign({ vs: 0.2 }, AMP(0.005, 1, 1, 0.08))),
    sine(Object.assign({ crs: 19, lvl: 0.12 }, AMP(0.001, 0.12, 0, 0.05)))],
  lfo: [LF(0, 6.5)], mods: [['l1', '*.pan', 0.4], ['l1', '*.pitch', 0.002]],
  fx: fx(DRV(0.2, 1, 0.7), CHO(0.35, 0.8, 0.5, 1), REV(0.22, 2, 0.6, 1))
}, { vary: [['L.0.pos', 0.55, 0.9], ['lfo.0.rate', 0.8, 6.8], ['fx.drv.amt', 0.1, 0.35]] });

A('Organs', ['Combo Fizz', 'Garage Band Organ', 'Farfisa Fizz', 'Wave Rider Organ', 'Sixties Organ', 'Psych Organ', 'Vox Continental', 'Mod Organ', 'Teen Dance', 'Fuzz Organ', 'Beach Party', 'Spy Organ'], {
  L: [sq(Object.assign({ pw: 0.35, vs: 0.1 }, lp12(0.75), AMP(0.003, 1, 1, 0.05))),
    sq(Object.assign({ crs: 12, pw: 0.5, lvl: 0.4 }, lp12(0.8), AMP(0.003, 1, 1, 0.05)))],
  lfo: [LF(0, 6)], mods: [['l1', '*.pitch', 0.004]],
  fx: fx(DRV(0.2, 2, 0.75), REV(0.2, 1.6, 0.5, 2))
}, { vary: [['L.0.pw', 0.25, 0.5], ['L.1.crs', [12, 19, 24]]] });

A('Organs', ['Grand Pipe Organ', 'Basilica Pipes', 'Bach Organ', 'Chapel Pipes', 'Wedding Organ', 'Grand Pipes', 'Requiem Organ', 'Stone Pipes', 'Vespers', 'Abbey Organ', 'Wake Organ', 'Organ Loft'], {
  L: [wt('Octave Stack', 0.6, Object.assign({ uni: 2, det: 3, spr: 0.6 }, AMP(0.05, 1, 1, 0.6))),
    wt('Hollow', 0.3, Object.assign({ crs: -12, lvl: 0.5 }, AMP(0.08, 1, 1, 0.6)))],
  fx: fx(REV(0.5, 7, 0.9, 0))
}, { vary: [['L.0.pos', 0.4, 0.9], ['L.1.crs', [-12, 7, 12]]] });

/* ---- Guitars & World ---- */
A('Guitars & World', ['Clean Strat', 'Neck Pickup', 'Soul Rhythm', 'Rhythm Clean', 'Funk Strum', 'Shimmer Strat', 'Archtop', 'Clean Arpeggio', 'Motown Guitar', 'Tremolo Guitar', 'Spring Guitar', 'Vintage Clean'], {
  L: [pluck(Object.assign({ pdec: 2.2, pdmp: 0.45, ppos: 0.1, pexc: 0.55, vs: 0.7 }, lp12(0.62, 0.15), AMP(0.001, 1, 1, 0.25)))],
  fx: fx(DRV(0.08, 1, 0.6), CHO(0.25, 0.6, 0.4, 0), REV(0.22, 2, 0.5, 2))
}, { vary: [['L.0.ppos', 0.06, 0.18], ['L.0.pdmp', 0.35, 0.55], ['fx.cho.mix', 0, 0.35]] });

A('Guitars & World', ['Sitar Drone', 'Tanpura', 'Sarod', 'Raga Pluck', 'Veena', 'Santoor', 'Jaw Harp', 'Desert Oud', 'Saz', 'Rubab', 'Kora', 'Ngoni'], {
  L: [pluck(Object.assign({ pdec: 4, pdmp: 0.2, ppos: 0.05, pexc: 0.75, uni: 2, det: 4, spr: 0.5 }, AMP(0.001, 1, 1, 1))),
    noise(0, Object.assign({ ncol: 0.8, kt: 1, lvl: 0.08 }, bp(0.75, 0.6), AMP(0.001, 0.08, 0, 0.05)))],
  fx: fx(REV(0.3, 3.5, 0.7, 0), PHA(0.12, 0.2, 0.5, 0.3))
}, { vary: [['L.0.pdmp', 0.1, 0.35], ['L.0.ppos', 0.03, 0.12], ['L.0.pdec', 2.5, 6]] });

A('Guitars & World', ['Steel Pan Party', 'Island Pan', 'Calypso Pan', 'Trinidad Pan', 'Soca Pan', 'Beach Pan', 'Carnival Pan', 'Reggae Pan', 'Palm Pan', 'Harbor Pan', 'Tropic Pan', 'Sunset Pan'], {
  L: [modal('Steel Pan', Object.assign({ mdec: 1.6, mbri: 0.55, mhard: 0.55, vs: 0.6 }, AMP(0.001, 1, 1, 0.5)))],
  fx: fx(CHO(0.15, 0.4, 0.3, 0), REV(0.25, 2.2, 0.6, 2), DLY(0.12, '1/8', 0.3))
}, { vary: [['L.0.mbri', 0.45, 0.7], ['L.0.mdec', 1, 2.5]] });

/* ---- Arps & Sequences ---- */
A('Arps & Sequences', ['Berlin Sequence', 'Tangerine Steps', 'Krautrock Run', 'Motorik Arp', 'Cosmic Sequence', 'Sequencer Dream', 'Klaus Arp', 'Analog Runner', 'Phaedra Steps', 'Midnight Sequencer', 'Space Runner', 'Chrome Sequence'], {
  L: [saw(Object.assign({ drift: 0.3 }, ladderF(0.3, 0.45), F(0.001, 0.2, 0, 0.1, 0.35), AMP(0.001, 0.25, 0, 0.12)))],
  lfo: [LF(1, 0.06)], mods: [['l1', 'A.f1c', 0.12]],
  arp: ARP('updown', '1/16', 2, 0.5),
  fx: fx(DLY(0.3, '1/8.', 0.45), REV(0.28, 3.5, 0.7, 0))
}, { vary: [['arp.mode', ['up', 'updown', 'order', 'random']], ['arp.oct', [1, 2, 3]], ['L.0.f1r', 0.3, 0.6]] });

A('Arps & Sequences', ['Pluck Cascade', 'Waterfall Arp', 'Rain Arp', 'Glass Cascade', 'Silver Steps', 'Morning Arp', 'Ribbon Arp', 'Spiral Arp', 'Ladder of Light', 'Sky Steps', 'Cloud Runner', 'Bright Cascade'], {
  L: [pluck(Object.assign({ pdec: 1.2, pdmp: 0.35, ppos: 0.15, pexc: 0.6, spr: 0.6 }, AMP(0.001, 1, 1, 0.3)))],
  arp: ARP('up', '1/16', 3, 0.6),
  fx: fx(DLY(0.35, '1/8.', 0.5), REV(0.35, 4, 0.8, 0))
}, { vary: [['arp.mode', ['up', 'down', 'updown']], ['arp.div', ['1/16', '1/16T', '1/32']], ['L.0.pdmp', 0.25, 0.5]] });

A('Arps & Sequences', ['Trap Arp', 'Drill Steps', 'Bell Runner', 'Triplet Bells', 'Flex Arp', 'Ice Steps', 'Drip Arp', 'Glitter Arp', 'Chain Runner', 'Plugg Arp', 'Rage Arp', 'Night Runner'], {
  L: [modal('Bell', Object.assign({ crs: 12, mdec: 1.2, mbri: 0.6, mhard: 0.6 }, AMP(0.001, 1, 1, 0.3)))],
  arp: ARP('updown', '1/16T', 2, 0.5),
  fx: fx(DLY(0.25, '1/8T', 0.35), REV(0.3, 3, 0.7, 0))
}, { vary: [['L.0.mdl', ['Bell', 'Glock', 'Crystal', 'Music Box']], ['arp.mode', ['up', 'updown', 'random']], ['arp.div', ['1/16T', '1/16', '1/8T']]] });

/* ---- Pulses & Gates ---- */
A('Pulses & Gates', ['Gated Choir', 'Stutter Voices', 'Chopped Aahs', 'Pulse Choir', 'Gate Gospel', 'Rhythm Voices', 'Vocal Gate', 'Pulse Hymn', 'Tick Choir', 'Motion Voices', 'Strobe Choir', 'Beating Voices'], {
  L: [vox(Object.assign({ vow: 0.05, uni: 4, det: 12, spr: 0.9, brth: 0.1 }, lp12(0.75), AMP(0.1, 1, 1, 0.6)))],
  fx: fx(GATE([1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1], '1/16', 1, 0.2), REV(0.35, 3.5, 0.75, 0))
}, { vary: [['fx.gate.pat', GATE_PATS], ['fx.gate.div', ['1/16', '1/8', '1/16T']], ['L.0.vow', 0, 0.3]] });

A('Pulses & Gates', ['Pulse Bass Pad', 'Throb Bass', 'Pump Low', 'Motor Pulse', 'Engine Pulse', 'Pulse Drive', 'Sidechain Low', 'Thud Pulse', 'Low Strobe', 'Deep Pump', 'Rumble Gate', 'Undertow Pulse'], {
  L: [saw(Object.assign({ uni: 3, det: 10, spr: 0.6 }, lp(0.35, 0.2), AMP(0.01, 1, 1, 0.2))),
    sine(Object.assign({ crs: -12, lvl: 0.5 }, AMP(0.01, 1, 1, 0.2)))],
  lfo: [LS(3, '1/4', { rtg: 0 })], mods: [['l1', '*.lvl', 0.6]],
  fx: fx(DRV(0.2, 1, 0.5), REV(0.15, 2, 0.5, 1))
}, { vary: [['lfo.0.div', ['1/4', '1/8', '1/8.']], ['L.0.f1c', 0.28, 0.45]] });

A('Pulses & Gates', ['Morse Bells', 'Signal Pulse', 'Telegraph', 'Ping Pulse', 'Sonar Gate', 'Blip Pattern', 'Beacon Pulse', 'Radar Pulse', 'Dot Dash', 'Transmitter', 'Pulse Code', 'Relay'], {
  L: [sine(Object.assign({ crs: 12, fm: 0.2, fmr: 3.5 }, AMP(0.005, 1, 1, 0.3)))],
  fx: fx(GATE([1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0], '1/16', 1, 0.05), DLY(0.3, '1/8.', 0.45), REV(0.3, 3, 0.7, 2))
}, { vary: [['fx.gate.pat', GATE_PATS], ['L.0.fmr', [3.5, 2, 7, 1.41]], ['L.0.crs', [12, 24, 0]]] });

/* ---- Drones ---- */
A('Drones', ['Cello Drone', 'Bowed Drone', 'Low Bow', 'Rosin Drone', 'Gut String Drone', 'Bass Bow Hum', 'Resin Hum', 'Long Bow', 'Viola Drone', 'Deep Bow', 'String Hum', 'Horsehair'], {
  L: [modal('Harmonic', Object.assign({ mbow: 0.8, mdec: 8, mbri: 0.35, mhard: 0.2, crs: -12, spr: 0.6 }, AMP(2, 2, 1, 4))),
    saw(Object.assign({ crs: -12, lvl: 0.25, uni: 3, det: 8, spr: 0.8 }, lp(0.35), AMP(3, 2, 1, 4)))],
  lfo: [LF(6, 0.07)], mods: [['l1', 'A.mbri', 0.15]],
  fx: fx(REV(0.45, 7, 0.9, 0))
}, { vary: [['L.0.mdl', ['Harmonic', 'Piano', 'Plate']], ['L.0.mbow', 0.6, 1], ['L.0.crs', [-12, -24, 0]]] });

A('Drones', ['Tanpura Hum', 'Om Drone', 'Stillness', 'Root Hum', 'Open Fifth', 'Shruti Hum', 'Pump Harmonium', 'Temple Hum', 'Ashram', 'Breath Drone', 'Mantra', 'Sacred Hum'], {
  L: [wt('Reed', 0.4, Object.assign({ uni: 3, det: 5, spr: 0.7 }, lp12(0.55), AMP(2, 2, 1, 4))),
    wt('Reed', 0.5, Object.assign({ crs: 7, lvl: 0.45, uni: 2, det: 5, spr: 0.8 }, lp12(0.55), AMP(2.5, 2, 1, 4)))],
  lfo: [LF(0, 0.15)], mods: [['l1', 'A.pos', 0.2]],
  fx: fx(REV(0.5, 8, 0.9, 0), CHO(0.2, 0.2, 0.4, 1))
}, { vary: [['L.1.crs', [7, 12, -5]], ['L.0.pos', 0.2, 0.6]] });

A('Drones', ['Machine Drone', 'Factory Hum', 'Transformer', 'Generator', 'High Voltage', 'Server Room', 'Ventilation', 'Turbine', 'Engine Room', 'Reactor Hum', 'Substation', 'Steel Mill'], {
  L: [saw(Object.assign({ uni: 4, det: 30, spr: 0.8, ring: 0.3, rngr: 1.5 }, lp(0.35, 0.3), AMP(2, 2, 1, 4))),
    noise(4, Object.assign({ ncol: 0.3, kt: 0, lvl: 0.3, spr: 0.8 }, AMP(2, 2, 1, 4)))],
  lfo: [LF(5, 0.5), LF(6, 0.05)], mods: [['l1', 'A.f1c', 0.05], ['l2', 'A.ring', 0.3]],
  fx: fx(DRV(0.3, 3, 0.4), REV(0.45, 6, 0.85, 3))
}, { vary: [['L.0.rngr', [1.5, 2.3, 0.7, 3.1]], ['L.0.crs', [-12, -24, 0]], ['L.1.ntyp', [4, 0, 3]]] });

/* ---- Textures & Noise ---- */
A('Textures & Noise', ['Tape Hiss Bed', 'Reel Noise', 'Cassette Bed', 'Spool', 'Head Noise', 'Tape Drift', 'Magnetic Air', 'Oxide', 'Leader Tape', 'Capstan', 'Erase Head', 'Bias Noise'], {
  L: [noise(0, Object.assign({ ncol: 0.75, kt: 0, spr: 0.9 }, lp12(0.65, 0.05, { f1k: 0 }), AMP(0.5, 1, 1, 1.5)))],
  lfo: [LF(6, 0.3)], mods: [['l1', 'A.lvl', 0.25]],
  fx: fx(LOFI(0.6, { bits: 14, rate: 1, wow: 0.6, flut: 0.4, noise: 0.4, tone: 0.5 }), REV(0.2, 2, 0.5, 1))
}, { vary: [['L.0.ncol', 0.6, 0.9], ['L.0.f1c', 0.5, 0.75]] });

A('Textures & Noise', ['Rain on Tin', 'Storm Drain', 'Gutter Rain', 'Window Rain', 'Light Drizzle', 'Thunder Air', 'Downpour', 'Rain Tent', 'Wet Street', 'Puddle Splash', 'Rain Shelter', 'Monsoon'], {
  L: [noise(2, Object.assign({ ncol: 0.55, kt: 0, spr: 0.9 }, AMP(0.5, 1, 1, 2))),
    noise(0, Object.assign({ ncol: 0.2, kt: 0, lvl: 0.25, spr: 0.8 }, lp12(0.4, 0.05, { f1k: 0 }), AMP(1, 1, 1, 2)))],
  fx: fx(REV(0.35, 3, 0.7, 0))
}, { vary: [['L.0.ncol', 0.35, 0.85], ['L.1.lvl', 0.1, 0.4]] });

A('Textures & Noise', ['Bit Crush Air', 'Data Flow', 'Modem Noise', 'Digital Rain', 'Packet Loss', 'Glitch Bed', 'Corrupted Air', 'Bitstream', 'Error Bed', 'Static Code', 'Byte Noise', 'Signal Decay'], {
  L: [noise(3, Object.assign({ ncol: 0.5, kt: 1, spr: 0.8 }, AMP(0.2, 1, 1, 1)))],
  lfo: [LS(5, '1/16', { rtg: 0 })], mods: [['l1', 'A.ncol', 0.3]],
  fx: fx(LOFI(0.7, { bits: 6, rate: 6, wow: 0, flut: 0, noise: 0, tone: 0.7 }), DLY(0.2, '1/16', 0.4), REV(0.25, 2, 0.6, 2))
}, { vary: [['fx.lofi.bits', 4, 9], ['fx.lofi.rate', 3, 12], ['lfo.0.div', ['1/16', '1/32', '1/8']]] });

/* ---- Granular ---- */
A('Granular', ['Grain Kalimba', 'Thumb Cloud', 'Kalimba Mist', 'Scattered Tines', 'Mbira Dust', 'Tine Swarm', 'Thumb Rain', 'Kalimba Fog', 'Plucked Grains', 'Kalimba Stretch', 'Tine Glitter', 'African Dust'], {
  L: [gran('Kalimba', Object.assign({ gpos: 0.4, gspr: 0.4, gsz: 0.4, gden: 0.6, spr: 0.9, gscan: 0.05, gpit: 0.05 }, AMP(0.5, 1, 1, 2.5)))],
  fx: fx(REV(0.4, 5, 0.85, 0), DLY(0.2, '1/8.', 0.4))
}, { vary: [['L.0.gsz', 0.25, 0.6], ['L.0.gden', 0.45, 0.75], ['L.0.gscan', [0, 0.05, 0.1, -0.05]]] });

A('Granular', ['Reverse Bloom', 'Reverse Choir', 'Rewind Cloud', 'Inverted Air', 'Mirror Grains', 'Retrograde', 'Back Spin Pad', 'Reverse Strings', 'Tape Reverse', 'Unwind', 'Time Reversal', 'Echo Rewind'], {
  L: [gran('Strings', Object.assign({ gpos: 0.4, gspr: 0.2, gsz: 0.7, gden: 0.55, grev: 0.8, spr: 0.9 }, lp12(0.7), AMP(1, 1, 1, 3)))],
  fx: fx(REV(0.5, 6, 0.9, 0))
}, { vary: [['L.0.src', ['Strings', 'Choir Aah', 'Glass Pad', 'Synth Chord', 'Felt Piano']], ['L.0.grev', 0.5, 1]] });

A('Granular', ['Organ Grains', 'Drawbar Cloud', 'Church Dust', 'Organ Freeze', 'Pipe Swarm', 'Hammond Haze', 'Organ Mist', 'Chapel Grains', 'Rotary Dust', 'Organ Drift', 'Holy Grains', 'Keyclick Cloud'], {
  L: [gran('Organ Chord', Object.assign({ gpos: 0.5, gspr: 0.2, gsz: 0.6, gden: 0.6, spr: 0.85, gscan: 0.02 }, lp12(0.7), AMP(0.8, 1, 1, 2.5)))],
  lfo: [LF(0, 0.1)], mods: [['l1', 'A.gpos', 0.15]],
  fx: fx(CHO(0.3, 0.6, 0.5, 1), REV(0.45, 5, 0.85, 0))
}, { vary: [['L.0.gsz', 0.45, 0.75], ['L.0.gpit', 0, 0.2]] });

/* ---- Sweeps & FX ---- */
A('Sweeps & FX', ['Down Lifter', 'Drop Fall', 'Sink', 'Down Sweep', 'Gravity Fall', 'Collapse', 'System Off', 'Descent', 'Sunset Fall', 'Tape Stop FX', 'Spin Down', 'Free Fall'], {
  L: [noise(0, Object.assign({ ncol: 0.7, kt: 0, spr: 0.8 }, bp(0.8, 0.4, { f1k: 0 }), AMP(0.001, 1, 1, 0.5))),
    saw(Object.assign({ uni: 4, det: 20, spr: 0.8, lvl: 0.5 }, lp(0.7, 0.2), AMP(0.001, 1, 1, 0.5)))],
  me: [ME(0.001, 4, 0, 0.5)], mods: [['e1', 'A.f1c', 0.4], ['e1', 'B.pitch', 0.5], ['e1', 'B.f1c', 0.3]],
  fx: fx(REV(0.45, 5, 0.85, 0), DLY(0.2, '1/16', 0.4))
}, { vary: [['me.0.d', 2, 6]] });

A('Sweeps & FX', ['Impact Boom', 'Floor Drop', 'Cinema Hit', 'Thud Impact', 'Trailer Boom', 'Low Impact', 'Deep Hit', 'Crash Boom', 'Earth Hit', 'Judgment Hit', 'Titan Hit', 'Bass Drop FX'], {
  L: [sine(Object.assign({ crs: -12, vs: 0.3 }, AMP(0.001, 2.5, 0, 1.5))),
    noise(0, Object.assign({ ncol: 0.4, kt: 0, lvl: 0.5 }, lp12(0.5, 0.1, { f1k: 0 }), AMP(0.001, 0.6, 0, 0.6)))],
  me: [ME(0, 1.2, 0, 0.5)], mods: [['e1', 'A.pitch', 0.5]],
  fx: fx(DRV(0.35, 1, 0.5), REV(0.45, 5, 0.85, 0))
}, { vary: [['mods.0.a', 0.3, 0.7], ['me.0.d', 0.5, 2]] });

A('Sweeps & FX', ['Laser Zap', 'Sci-Fi Zap', 'Ray Blaster', 'Pew Pew', 'Space Blaster', 'Photon', 'Zapper', 'Phaser Shot', 'Plasma Bolt', 'Ion Shot', 'Beam Up', 'Teleport'], {
  L: [sq(Object.assign({ pw: 0.3, fm: 0.3, fmr: 2 }, AMP(0.001, 0.35, 0, 0.2)))],
  me: [ME(0, 0.3, 0, 0.1)], mods: [['e1', 'A.pitch', 1], ['e1', 'A.fm', 0.4]],
  fx: fx(DLY(0.3, '1/16', 0.5), REV(0.3, 2.5, 0.7, 2))
}, { vary: [['me.0.d', 0.1, 0.6], ['mods.0.a', 0.5, 1], ['L.0.crs', [0, 12, 24]]] });

/* ---- Trap & Drill ---- */
A('Trap & Drill', ['Drill Slide Lead', 'UK Drill Bell', 'Brooklyn Bell', 'Drill Glide', 'Dark Drill Keys', 'Grime Bell', 'Opp Pack', 'Drill Hymn', 'Bronx Bell', 'Slide Piano', 'Drill Section', 'Night Patrol'], {
  L: [modal('Glock', Object.assign({ mdec: 1.6, mbri: 0.45, mhard: 0.5 }, AMP(0.001, 1, 1, 0.5))),
    saw(Object.assign({ crs: -12, lvl: 0.3, uni: 3, det: 10, spr: 0.8 }, lp(0.4), AMP(0.2, 1, 0.8, 0.8)))],
  g: { vmode: 2, glide: 0.12 },
  fx: fx(DLY(0.2, '1/8T', 0.35), REV(0.35, 3.5, 0.75, 0))
}, { vary: [['L.0.mdl', ['Glock', 'Bell', 'Vibes', 'Crystal']], ['g.glide', 0.06, 0.18], ['L.1.lvl', 0.15, 0.4]] });

A('Trap & Drill', ['Rage Saw', 'Rage Riot', 'Opium Lead', 'Distorted Saw Lead', 'Mosh Lead', 'Vamp Lead', 'Rage Stab', 'Punk Trap', 'Fuzz Lead', 'Blown Lead', 'Rager', 'Club Rage'], {
  L: [saw(Object.assign({ uni: 7, det: 28, spr: 0.9 }, lp(0.6, 0.15), AMP(0.001, 0.5, 0.9, 0.15)))],
  g: { vmode: 1, glide: 0.04 },
  fx: fx(DRV(0.6, 2, 0.6), REV(0.25, 2, 0.6, 1), DLY(0.15, '1/8', 0.3))
}, { vary: [['L.0.det', 18, 40], ['fx.drv.amt', 0.4, 0.8], ['L.0.f1c', 0.5, 0.75]], kit: 'lead' });

A('Trap & Drill', ['Plugg Synth', 'Plugg Pad', 'PluggnB Glow Keys', 'Cloud Plugg', 'Plugg Flute', 'Plugg Choir', 'Dreamy Plugg', 'Plugg Glow', 'Soft Plugg', 'Pink Plugg', 'Plugg Haze', 'Plugg Nights'], {
  L: [sq(Object.assign({ pw: 0.4, uni: 3, det: 10, spr: 0.8 }, lp12(0.55), AMP(0.01, 1, 0.8, 0.6))),
    sine(Object.assign({ crs: 12, lvl: 0.3 }, AMP(0.01, 0.6, 0.5, 0.5)))],
  fx: fx(CHO(0.35, 0.5, 0.5, 1), REV(0.35, 3.5, 0.75, 0), DLY(0.2, '1/8', 0.35))
}, { vary: [['L.0.pw', 0.3, 0.5], ['L.0.f1c', 0.45, 0.65], ['L.1.crs', [12, 19, 24]]] });

/* ---- Soul & Boom Bap ---- */
A('Soul & Boom Bap', ['Crate Dig Rhodes', 'Dilla Keys', 'Donut Keys', 'Wavy Rhodes', 'Conant Gardens Keys', 'Slum Keys', 'Ruff Rhodes', 'Ninety Three Keys', 'Brooklyn Keys', 'Dusty EP', 'MPC Keys', 'SP Keys'], {
  L: [modal('Tine', Object.assign({ mbri: 0.38, mdec: 2.6, mhard: 0.4, vs: 0.65 }, lp12(0.58), AMP(0.001, 1, 1, 0.4)))],
  lfo: [LF(0, 4.2)], mods: [['l1', '*.pan', 0.2]],
  fx: fx(LOFI(0.6, { bits: 12, rate: 1.8, wow: 0.35, flut: 0.2, noise: 0.15, tone: 0.48 }), COMP(-16, 3, 2), REV(0.2, 1.8, 0.5, 1))
}, { vary: [['L.0.mbri', 0.3, 0.5], ['fx.lofi.bits', 10, 13], ['fx.lofi.rate', 1, 2.6]] });

A('Soul & Boom Bap', ['Soul Sample Chord', 'Chopped Strings', 'Sample Flip', 'Diggers Delight', 'Dollar Bin Strings', 'Vinyl Orchestra', 'Loop Digger', 'Soul Chop', 'Flip This', 'Record Store', 'Breakbeat Strings', 'Heavy Rotation'], {
  L: [gran('Synth Chord', Object.assign({ gpos: 0.4, gspr: 0.05, gsz: 0.7, gden: 0.65, spr: 0.6 }, lp12(0.6), AMP(0.005, 1, 1, 0.3))),
    noise(1, Object.assign({ ncol: 0.3, kt: 0, lvl: 0.12 }, AMP(0.001, 1, 1, 0.3)))],
  fx: fx(LOFI(0.75, { bits: 11, rate: 2, wow: 0.3, flut: 0.2, noise: 0.25, tone: 0.42 }), COMP(-18, 4, 3), EQ(2, 0, 1000, -3))
}, { vary: [['L.0.src', ['Synth Chord', 'Strings', 'Choir Chord', 'Felt Piano', 'Organ Chord']], ['L.0.gpos', 0.3, 0.6]] });

A('Soul & Boom Bap', ['Jazz Bass Upright', 'Walking Upright', 'Upright Thump', 'Bebop Bass', 'Cellar Bass', 'Double Bass', 'Boom Bap Upright', 'Smoky Upright', 'Fretless Soul', 'Thumb Bass', 'Pocket Upright', 'Club Upright'], {
  L: [pluck(Object.assign({ crs: -12, pdec: 1.5, pdmp: 0.7, ppos: 0.25, pexc: 0.35, vs: 0.6 }, lp12(0.45), AMP(0.001, 1, 1, 0.15))),
    sine(Object.assign({ crs: -12, lvl: 0.35 }, AMP(0.001, 0.8, 0.4, 0.15)))],
  g: { vmode: 1, glide: 0.02 },
  fx: fx(COMP(-16, 3, 2), REV(0.12, 1.2, 0.4, 1))
}, { vary: [['L.0.pdmp', 0.6, 0.8], ['L.0.f1c', 0.38, 0.55]], kit: 'bass' });

/* ---- Cinematic ---- */
A('Cinematic', ['Heroic Brass', 'Victory Horns', 'Battle Brass', 'Epic Fanfare', 'Kingdom Horns', 'Legend Brass', 'Triumph', 'Warriors', 'Final Stand', 'Banner Horns', 'Valor', 'Crown Brass'], {
  L: [saw(Object.assign({ uni: 5, det: 12, spr: 0.8, vs: 0.6 }, lp(0.45, 0.1, { f1v: 0.5 }), F(0.1, 0.8, 0.6, 0.5, 0.25), AMP(0.08, 0.8, 0.9, 0.6))),
    wt('Brass', 0.6, Object.assign({ crs: -12, lvl: 0.5, uni: 3, det: 8 }, lp(0.45), AMP(0.1, 0.8, 0.9, 0.6)))],
  fx: fx(REV(0.4, 4.5, 0.85, 0), COMP(-16, 3, 2))
}, { vary: [['L.0.f1e', 0.15, 0.35], ['L.1.crs', [-12, 0, 7]]] });

A('Cinematic', ['Dark Tension', 'Creeping Dread', 'Horror Pad', 'Shadow Bed', 'Nightmare', 'The Basement', 'Unease', 'Low Threat', 'Cellar Door', 'Haunting', 'Abyss Bed', 'Something Wrong'], {
  L: [saw(Object.assign({ uni: 5, det: 35, spr: 0.9, ring: 0.2, rngr: 1.06 }, lp(0.3, 0.3), AMP(2, 2, 1, 4))),
    noise(4, Object.assign({ ncol: 0.3, kt: 0, lvl: 0.2, spr: 0.9 }, AMP(2, 2, 1, 4)))],
  lfo: [LF(6, 0.08)], mods: [['l1', 'A.f1c', 0.1], ['l1', 'A.ring', 0.2]],
  fx: fx(REV(0.55, 9, 0.95, 3), DLY(0.15, '1/4.', 0.5))
}, { vary: [['L.0.crs', [-12, 0, -24]], ['L.0.rngr', [1.06, 1.5, 0.94, 2.1]]] });

A('Cinematic', ['Piano Score', 'Emotional Piano', 'Film Piano', 'Down Memory Lane', 'Goodbye Piano', 'Rain Score', 'Tender Keys', 'Letters Home', 'First Snow', 'Distant Piano', 'Closing Credits', 'Old Photographs'], {
  L: [modal('Piano', Object.assign({ mbri: 0.4, mdec: 4, mhard: 0.3, minh: 0.4, vs: 0.75 }, lp12(0.7), AMP(0.001, 1, 1, 1.2))),
    saw(Object.assign({ lvl: 0.15, uni: 4, det: 10, spr: 0.9 }, lp(0.35), AMP(1.5, 2, 0.8, 3)))],
  fx: fx(REV(0.45, 5.5, 0.85, 0), DLY(0.12, '1/4.', 0.35, { hc: 3500 }))
}, { vary: [['L.0.mbri', 0.3, 0.5], ['L.1.lvl', 0.05, 0.25]] });

/* =========================================================== MODERN HIP HOP */
/* Rage, plugg, bell and dark-trap palettes plus a full one-shot drum kit. Names describe the sound, not any artist. */

/* ---- rage leads: wide detuned saws, clipped and roomy ---- */
A('Modern Hip Hop', ['Rage Anthem', 'Vamp Supersaw', 'Stage Dive', 'Crowd Surf', 'Red Leather', 'Blackout Saw', 'Chrome Rage', 'Moshpit Lead', 'Vamp Anthem', 'Riot Saw', 'Arena Rage', 'Leather Jacket'], {
  L: [saw(Object.assign({ uni: 7, det: 32, spr: 0.95, drift: 0.25 }, lp(0.68, 0.12), AMP(0.003, 0.6, 0.9, 0.35))),
    saw(Object.assign({ crs: 12, lvl: 0.35, uni: 5, det: 24, spr: 0.95 }, lp(0.62), AMP(0.003, 0.6, 0.85, 0.35)))],
  g: { vmode: 0, glide: 0 },
  fx: fx(DRV(0.55, 2, 0.62), REV(0.32, 2.8, 0.75, 0), DLY(0.16, '1/8', 0.32, { hc: 4200 }))
}, { vary: [['L.0.det', 22, 42], ['fx.drv.amt', 0.35, 0.75], ['L.1.crs', [12, 7, 0, 19]], ['L.0.f1c', 0.55, 0.78]], kit: 'lead', tags: ['rage', 'opium', 'supersaw'] });

/* ---- tonka bells: bright, struck, a little dirty ---- */
A('Modern Hip Hop', ['Tonka Bell', 'Money Bell', 'Wealth Chime', 'Rich Bell', 'Sky Bell', 'Vault Bell', 'Diamond Tonka', 'Rager Bell', 'Ice Tonka', 'Bell Toll', 'Chrome Chime', 'Gold Toll'], {
  L: [modal('Bell', Object.assign({ crs: 12, mdec: 2.2, mbri: 0.7, mhard: 0.75, spr: 0.4 }, AMP(0.001, 1, 1, 0.9))),
    sine(Object.assign({ crs: 24, fm: 0.35, fmr: 3.5, lvl: 0.35 }, AMP(0.001, 0.5, 0, 0.4)))],
  fx: fx(DRV(0.3, 1, 0.7), REV(0.3, 3, 0.7, 2), DLY(0.18, '1/8.', 0.35))
}, { vary: [['L.0.mdl', ['Bell', 'Glock', 'Crystal', 'Tine']], ['L.0.mdec', 1.4, 3.2], ['L.1.fmr', [3.5, 2, 7, 1.41]], ['fx.drv.amt', 0.1, 0.45]], kit: 'bell', tags: ['bell', 'tonka', 'rage'] });

/* ---- plugg: soft squares and flutes with gentle vibrato ---- */
A('Modern Hip Hop', ['Cloud Plugg Lead', 'Baby Plugg', 'Pluggy Flute', 'Starry Plugg', 'Dreamland Lead', 'Video Game Plugg', 'Pastel Square', 'Bubble Plugg', 'Soft Arcade', 'Sugar Lead', 'Plugg Whistle', 'Daydream Square'], {
  L: [sq(Object.assign({ pw: 0.35, vs: 0.45 }, lp12(0.62, 0.05), AMP(0.01, 0.6, 0.8, 0.35))),
    sine(Object.assign({ crs: 12, lvl: 0.35 }, AMP(0.01, 0.4, 0.6, 0.3)))],
  lfo: [LF(0, 5.6, { rtg: 1, fade: 0.35 })], mods: [['l1', '*.pitch', 0.006]],
  g: { vmode: 2, glide: 0.05 },
  fx: fx(CHO(0.3, 0.6, 0.45, 0), REV(0.32, 3, 0.75, 0), DLY(0.24, '1/8', 0.4))
}, { vary: [['L.0.pw', 0.2, 0.5], ['L.0.shp', [1, 0.333, 0]], ['L.1.crs', [12, 19, 24]]], kit: 'lead', tags: ['plugg', 'pluggnb'] });

/* ---- dark trap pads: big, filtered, reverb-soaked ---- */
A('Modern Hip Hop', ['Astro Haze', 'Night Drive Pad', 'Smoke Pad', 'Underworld Pad', 'Purple Haze Pad', 'Space Ghost Pad', 'Afterhours Pad', 'Moody Wash', 'Dark Sky Pad', 'Night Vision', 'Rodeo Haze', 'Cave Choir Pad'], {
  L: [saw(Object.assign({ uni: 6, det: 16, spr: 0.9, drift: 0.35 }, lp(0.36, 0.2), F(1.2, 2.5, 0.6, 2.5, 0.1), AMP(0.8, 2, 0.9, 3.5))),
    vox(Object.assign({ lvl: 0.35, vow: 0.85, uni: 4, det: 12, spr: 0.9, brth: 0.15 }, AMP(1.2, 2, 0.9, 3.5)))],
  lfo: [LF(6, 0.09)], mods: [['l1', 'A.f1c', 0.06]],
  fx: fx(REV(0.5, 7, 0.9, 3), DLY(0.15, '1/4.', 0.45, { hc: 3000 }), CHO(0.25, 0.3, 0.5, 1))
}, { vary: [['L.0.f1c', 0.28, 0.45], ['L.1.vow', 0.6, 1], ['L.0.crs', [0, -12]]], kit: 'pad', tags: ['dark', 'ambient', 'trap'] });

/* ---- distorted 808s with glide ---- */
A('Modern Hip Hop', ['808 Rage Clip', '808 Opium', '808 Tonka', '808 Smash Glide', '808 Vamp', '808 Mosh', '808 Rager', '808 Blackout', '808 Ruptured', '808 Punk', '808 Red Light', '808 Wreck'], {
  L: [sine(Object.assign({ vs: 0.3, fold: 0.22, sub: 0.2 }, AMP(0.001, 2.6, 0.35, 0.25)))],
  me: [ME(0, 0.045, 0, 0.04)], mods: [['e1', 'A.pitch', 0.4]],
  g: { vmode: 2, glide: 0.11, gmode: 1 },
  fx: fx(DRV(0.7, 2, 0.55), COMP(-16, 4, 1), DRY)
}, { vary: [['fx.drv.type', [2, 1, 3, 4]], ['fx.drv.amt', 0.5, 0.88], ['L.0.fold', 0.08, 0.38], ['L.0.ad', 1.4, 3.6], ['g.glide', 0.06, 0.16]], kit: 's808', tags: ['808', 'distorted', 'rage'] });

/* ---- hyper and digi leads: bright, crushed, pitch-bent ---- */
A('Modern Hip Hop', ['Digicore Lead', 'Hyperspeed Lead', 'Glitch Pop Lead', 'Pixel Heart', 'Cyber Love', 'Laptop Lead', 'Crushed Lead', 'Bubblegum Lead', 'Neon Crush', 'Hyper Bell Lead', 'Static Love', 'Screen Glow'], {
  L: [wt('Digital Saw', 0.4, Object.assign({ uni: 3, det: 14, spr: 0.8 }, lp(0.72, 0.15), AMP(0.003, 0.4, 0.85, 0.2))),
    sq(Object.assign({ crs: 12, pw: 0.25, lvl: 0.35 }, AMP(0.003, 0.3, 0.7, 0.2)))],
  me: [ME(0, 0.12, 0, 0.1)], mods: [['e1', '*.pitch', 0.06]],
  g: { vmode: 2, glide: 0.07 },
  fx: fx(LOFI(0.5, { bits: 9, rate: 2.2, wow: 0, flut: 0, noise: 0, tone: 0.85 }), CHO(0.3, 0.8, 0.5, 0), REV(0.25, 2.2, 0.6, 2), DLY(0.18, '1/16', 0.3))
}, { vary: [['L.0.wt', ['Digital Saw', 'Bit Sine', 'Sync Saw', 'PWM']], ['fx.lofi.bits', 6, 11], ['mods.0.a', 0.03, 0.1]], kit: 'lead', tags: ['hyperpop', 'digicore'] });

/* ---- vocal textures for hooks and adlib beds ---- */
A('Modern Hip Hop', ['Astro Vox', 'Hum Chop', 'Vox Drift', 'Ghost Adlib', 'Choir Rage', 'Angel Chop', 'Falsetto Pad', 'Space Hum', 'Auto Choir', 'Melody Vox', 'Air Ooh', 'Halo Vox'], {
  L: [vox(Object.assign({ vow: 0.8, fsh: 0.6, uni: 3, det: 10, spr: 0.9, brth: 0.12 }, AMP(0.08, 1, 0.9, 0.9)))],
  lfo: [LF(0, 5.2, { rtg: 1, fade: 0.4 })], mods: [['l1', 'A.pitch', 0.005]],
  g: { vmode: 2, glide: 0.06 },
  fx: fx(CHO(0.3, 0.5, 0.5, 1), REV(0.42, 4.5, 0.85, 0), DLY(0.2, '1/4', 0.45))
}, { vary: [['L.0.vow', 0.5, 1], ['L.0.fsh', 0.5, 0.75], ['L.0.crs', [0, 12]]], kit: 'vox', tags: ['vocal', 'melodic'] });

/* ===================== percussion: one-shots, same pitch on every key ===================== */
const DRUM = function () { return Object.assign.apply(null, [{ kt: 0, vs: 0.55 }].concat(Array.prototype.slice.call(arguments))); };

/* ---- kicks: sine with a fast pitch drop plus a click ---- */
A('Modern Hip Hop', ['Punch Kick', 'Rage Kick', 'Club Kick', 'Tight Kick', 'Thump Kick', 'Hard Kick', 'Dirty Kick', 'Sub Kick', 'Box Kick', 'Smack Kick', 'Knock Kick', 'Clipped Kick'], {
  L: [sine(DRUM({ crs: -29, sub: 0.1 }, AMP(0.001, 0.32, 0, 0.12))),
    noise(0, DRUM(Object.assign({ ncol: 0.9, lvl: 0.35 }, hp(0.6, 0.1, { f1k: 0 }), AMP(0.001, 0.012, 0, 0.01))))],
  me: [ME(0, 0.055, 0, 0.05)], mods: [['e1', 'A.pitch', 1]],
  g: { poly: 4 },
  fx: fx(DRV(0.35, 1, 0.6), COMP(-14, 3, 2), DRY)
}, { vary: [['L.0.crs', -32, -26], ['L.0.ad', 0.18, 0.6], ['me.0.d', 0.03, 0.09], ['fx.drv.amt', 0.15, 0.7], ['L.1.lvl', 0.15, 0.5]], kit: 'drum', tags: ['drums', 'kick'] });

/* ---- snares: tone body plus filtered noise ---- */
A('Modern Hip Hop', ['Crack Snare', 'Rage Snare', 'Trap Snare', 'Tight Snare', 'Dirty Snare', 'Rimshot Snare', 'Smack Snare', 'Dusty Snare', 'Pop Snare', 'Crispy Snare', 'Hard Snare', 'Dark Snare'], {
  L: [tri(DRUM(Object.assign({ crs: -3 }, AMP(0.001, 0.11, 0, 0.08)))),
    noise(0, DRUM(Object.assign({ ncol: 0.8, lvl: 0.85 }, hp(0.55, 0.15, { f1k: 0 }), AMP(0.001, 0.18, 0, 0.12))))],
  me: [ME(0, 0.03, 0, 0.03)], mods: [['e1', 'A.pitch', 0.3]],
  g: { poly: 4 },
  fx: fx(DRV(0.25, 1, 0.65), COMP(-16, 3, 2), REV(0.12, 1.1, 0.4, 1))
}, { vary: [['L.0.crs', -6, 2], ['L.1.ad', 0.1, 0.3], ['L.1.f1c', 0.45, 0.68], ['fx.drv.amt', 0.1, 0.55], ['fx.rev.mix', 0.05, 0.22]], kit: 'drum', tags: ['drums', 'snare'] });

/* ---- claps: noise flammed by a fast square for the first few milliseconds ---- */
A('Modern Hip Hop', ['Stack Clap', 'Rage Clap', 'Wide Clap', 'Snap Clap', 'Room Clap', 'Tight Clap', 'Dirty Clap', 'Big Clap', 'Crowd Clap', 'Gloss Clap', 'Layered Clap', 'Slap Clap'], {
  L: [noise(0, DRUM(Object.assign({ ncol: 0.82, spr: 0.6, lvl: 1.5 }, bp(0.66, 0.3, { f1k: 0 }), AMP(0.001, 0.2, 0, 0.14))))],
  lfo: [LF(4, 105, { rtg: 1 })], me: [ME(0, 0.03, 0, 0.03)], mods: [['l1', 'A.lvl', 0.95, 'e1']],
  g: { poly: 4 },
  fx: fx(COMP(-16, 3, 2), REV(0.2, 1.3, 0.5, 1))
}, { vary: [['lfo.0.rate', 80, 140], ['L.0.ad', 0.12, 0.32], ['L.0.f1c', 0.58, 0.72], ['fx.rev.mix', 0.08, 0.32]], kit: 'drum', tags: ['drums', 'clap'] });

/* ---- closed hats: bright noise with a metallic ring ---- */
A('Modern Hip Hop', ['Tick Hat', 'Rage Hat', 'Tight Hat', 'Trap Hat', 'Crisp Hat', 'Dusty Hat', 'Metal Hat', 'Shaker Hat', 'Zip Hat', 'Glass Hat', 'Ghost Hat', 'Sizzle Hat'], {
  L: [noise(0, DRUM(Object.assign({ ncol: 1, lvl: 1.5 }, hp(0.82, 0.2, { f1k: 0 }), AMP(0.001, 0.045, 0, 0.035)))),
    sq(DRUM(Object.assign({ crs: 28, fm: 0.6, fmr: 1.47, ring: 0.6, rngr: 2.3, lvl: 0.22 }, hp(0.8, 0.1, { f1k: 0 }), AMP(0.001, 0.03, 0, 0.025))))],
  g: { poly: 6 },
  fx: fx(DRY)
}, { vary: [['L.0.ad', 0.025, 0.09], ['L.0.f1c', 0.76, 0.88], ['L.1.lvl', 0.15, 0.6], ['L.0.ncol', 0.85, 1]], kit: 'drum', tags: ['drums', 'hihat'] });

/* ---- open hats and cymbals ---- */
A('Modern Hip Hop', ['Open Hat Wash', 'Rage Open Hat', 'Air Open Hat', 'Sizzle Open', 'Crash Wash', 'Trash Crash', 'Ride Ping', 'Splash Hit', 'Dark Crash', 'Bright Crash', 'Swell Cymbal', 'Long Open Hat'], {
  L: [noise(0, DRUM(Object.assign({ ncol: 1, spr: 0.6 }, hp(0.78, 0.15, { f1k: 0 }), AMP(0.001, 0.45, 0, 0.3)))),
    wt('Metallic', 0.6, DRUM(Object.assign({ crs: 26, lvl: 0.25, uni: 3, det: 40, spr: 0.8 }, hp(0.75, 0.1, { f1k: 0 }), AMP(0.001, 0.35, 0, 0.25))))],
  g: { poly: 6 },
  fx: fx(REV(0.18, 2, 0.6, 2))
}, { vary: [['L.0.ad', 0.3, 1.8], ['L.1.lvl', 0.1, 0.4], ['L.0.f1c', 0.7, 0.85]], kit: 'drum', tags: ['drums', 'cymbal'],
  over: { 'Swell Cymbal': p => { p.L[0].aa = 1.2; p.L[1].aa = 1.2; p.L[0].ad = 0.2; p.L[0].ar = 0.15; p.L[1].ar = 0.15; } } });

/* ---- percs: rims, woods, cowbell, toms, snaps ---- */
A('Modern Hip Hop', ['Rim Click', 'Wood Perc', 'Bell Perc', 'Cowbell Hit', 'Finger Snap', 'Tom Drop', 'Pluck Perc', 'Metal Perc', 'Bongo Hit', 'Laser Perc', 'Knock Perc', 'Glass Perc'], {
  L: [modal('Wood', DRUM({ crs: 12, mdec: 0.15, mbri: 0.6, mhard: 0.8 }, AMP(0.001, 1, 1, 0.1))),
    noise(0, DRUM(Object.assign({ ncol: 0.9, lvl: 0.25 }, bp(0.75, 0.3, { f1k: 0 }), AMP(0.001, 0.01, 0, 0.01))))],
  g: { poly: 6 },
  fx: fx(REV(0.12, 1.2, 0.45, 1), COMP(-16, 3, 1))
}, { vary: [['L.0.crs', 0, 24], ['L.0.mdec', 0.08, 0.4]], kit: 'drum', tags: ['drums', 'perc'],
  over: {
    'Bell Perc': p => { p.L[0].mdl = 'Bell'; p.L[0].mdec = 0.5; },
    'Cowbell Hit': p => { Object.assign(p.L[0], { mode: 'analog', shp: 1, pw: 0.5, crs: 13, f1t: 'bp', f1c: 0.62, f1r: 0.3, f1k: 0, aa: 0.001, ad: 0.22, as: 0, ar: 0.1 });
      Object.assign(p.L[1], { mode: 'analog', shp: 1, pw: 0.5, crs: 20, ntyp: 0, lvl: 0.6, f1t: 'bp', f1c: 0.66, f1r: 0.3, f1k: 0, aa: 0.001, ad: 0.18, as: 0, ar: 0.1 }); },
    'Finger Snap': p => { Object.assign(p.L[0], { mode: 'noise', ntyp: 0, ncol: 0.85, lvl: 1.5, f1t: 'bp', f1c: 0.72, f1r: 0.45, f1k: 0, aa: 0.001, ad: 0.07, as: 0, ar: 0.05 }); },
    'Tom Drop': p => { Object.assign(p.L[0], { mdl: 'Drum', crs: -14, mdec: 0.5, mbri: 0.4 }); },
    'Metal Perc': p => { p.L[0].mdl = 'Plate'; p.L[0].mdec = 0.3; },
    'Bongo Hit': p => { Object.assign(p.L[0], { mdl: 'Drum', crs: 2, mdec: 0.25, mbri: 0.55 }); },
    'Laser Perc': p => { Object.assign(p.L[0], { mode: 'analog', shp: 0, crs: 24, f1t: 'off', aa: 0.001, ad: 0.12, as: 0, ar: 0.05 }); p.me[0] = { a: 0, d: 0.1, s: 0, r: 0.05, loop: 0 };
      p.mods.push({ s: 'e1', d: 'A.pitch', a: 0.9, v: '' }); },
    'Glass Perc': p => { p.L[0].mdl = 'Glass'; p.L[0].mdec = 0.35; p.L[0].crs = 24; }
  } });

/* ---- transitions and ear candy ---- */
A('Modern Hip Hop', ['Rage Riser', 'Tape Stop Drop', 'Siren Rage', 'Uplifter Rage', 'Impact Rage', 'Downer FX', 'Laser Siren', 'Air Sweep Up', 'Stutter Rise', 'Vinyl Brake', 'Crowd Roar', 'Bass Drop Rage'], {
  L: [noise(0, Object.assign({ ncol: 0.75, kt: 0, spr: 0.8 }, bp(0.4, 0.45, { f1k: 0 }), AMP(1.5, 1, 1, 0.6))),
    saw(Object.assign({ uni: 5, det: 30, spr: 0.9, lvl: 0.5 }, lp(0.3, 0.3), AMP(1.5, 1, 1, 0.6)))],
  me: [ME(3, 0.1, 1, 0.5)], mods: [['e1', 'A.f1c', 0.5], ['e1', 'B.f1c', 0.5], ['e1', 'B.pitch', 0.5]],
  fx: fx(DRV(0.3, 2, 0.6), REV(0.4, 4, 0.8, 0), DLY(0.2, '1/8', 0.45))
}, { vary: [['me.0.a', 2, 5]], kit: 'fx', tags: ['fx', 'transition'],
  over: {
    'Tape Stop Drop': p => { p.me[0] = { a: 0, d: 1, s: 0, r: 0.2, loop: 0 }; p.mods = [{ s: 'e1', d: 'B.pitch', a: 0.6, v: '' }]; p.L[0].on = 0; p.L[1].aa = 0.001; },
    'Impact Rage': p => { p.me[0] = { a: 0, d: 1.2, s: 0, r: 0.5, loop: 0 }; p.mods = [{ s: 'e1', d: 'B.pitch', a: 0.5, v: '' }]; Object.assign(p.L[1], { mode: 'analog', shp: 0, crs: -24, aa: 0.001, ad: 2, as: 0 }); p.L[0].aa = 0.001; p.L[0].ad = 0.6; p.L[0].as = 0; },
    'Downer FX': p => { p.me[0] = { a: 0.001, d: 3, s: 0, r: 0.5, loop: 0 }; p.L[0].aa = 0.001; p.L[1].aa = 0.001; },
    'Bass Drop Rage': p => { p.me[0] = { a: 0, d: 1.5, s: 0, r: 0.5, loop: 0 }; p.mods = [{ s: 'e1', d: 'B.pitch', a: 1, v: '' }]; Object.assign(p.L[1], { mode: 'analog', shp: 0, crs: -12, uni: 1, aa: 0.001, ad: 2.5, as: 0 }); p.L[0].on = 0; },
    'Vinyl Brake': p => { p.me[0] = { a: 0, d: 0.8, s: 0, r: 0.2, loop: 0 }; p.mods = [{ s: 'e1', d: 'B.pitch', a: 0.7, v: '' }]; p.L[1].aa = 0.001; p.fx.lofi = { on: 1, mix: 0.7, bits: 12, rate: 1.5, wow: 0.6, flut: 0.3, noise: 0.4, tone: 0.45 }; },
    'Crowd Roar': p => { Object.assign(p.L[0], { ntyp: 4, ncol: 0.5, aa: 0.8 }); p.L[1].lvl = 0.15; },
    'Siren Rage': p => { p.lfo[0] = { shp: 1, rate: 0.6, sync: 0, div: 1, fade: 0, rtg: 1, ph: 0 }; p.mods = [{ s: 'l1', d: 'B.pitch', a: 0.3, v: '' }]; p.L[1].aa = 0.05; p.L[0].on = 0; },
    'Laser Siren': p => { p.lfo[0] = { shp: 2, rate: 3, sync: 0, div: 1, fade: 0, rtg: 1, ph: 0 }; p.mods = [{ s: 'l1', d: 'B.pitch', a: 0.4, v: '' }]; p.L[1].aa = 0.02; p.L[0].on = 0; },
    'Stutter Rise': p => { p.fx.gate = { on: 1, pat: [1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1], div: '1/16', depth: 1, smooth: 0.1 }; }
  } });

function init() {
  const p = D.normalizePatch({ L: [saw(Object.assign({}, lp(0.8, 0.1)))] });
  p.name = 'Init Patch'; p.cat = 'Init';
  p.mac = [{ n: 'Macro 1', v: 0 }, { n: 'Macro 2', v: 0 }, { n: 'Macro 3', v: 0 }, { n: 'Macro 4', v: 0 }];
  return p;
}
return { list: out, CATS: CATS, KITS: KITS, init: init };
}

module.exports = AETHER_PRESETS;
