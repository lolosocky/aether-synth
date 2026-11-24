function AETHER_DSP() {
'use strict';

const TAU = Math.PI * 2, CB = 32, TBL = 2048, TMASK = 2047, NLEV = 18;
const MIPH = [];
for (let i = 0; i < NLEV; i++) MIPH.push(Math.max(1, Math.floor(512 * Math.pow(2, -i / 2))));
const VOICE_GAIN = 0.3, POOL = 20;
const STEAL_FADE = 0.015;   // seconds; how fast a voice taken for a new note fades out (4 ms was an audible click)

let _s = 0x2545F491 | 0;
function rnd() { _s ^= _s << 13; _s ^= _s >>> 17; _s ^= _s << 5; return (_s >>> 0) / 4294967296; }
function seed(v) { _s = (v | 0) || 0x2545F491; }
function hash01(i) {
  let x = Math.imul(i | 0, 0x9E3779B1) ^ 0x85EBCA6B;
  x = Math.imul(x ^ (x >>> 15), 0x2C1B3C6D); x = Math.imul(x ^ (x >>> 12), 0x297A2D39); x ^= x >>> 15;
  return (x >>> 0) / 4294967296;
}
function clamp(x, a, b) { return x < a ? a : (x > b ? b : x); }
function ftanh(x) { if (x <= -3) return -1; if (x >= 3) return 1; const x2 = x * x; return x * (27 + x2) / (27 + 9 * x2); }
function mtof(m) { return 440 * Math.pow(2, (m - 69) / 12); }

/* ------------------------------------------------------------------ FFT + tables */
function fft(re, im, inv) {
  const n = re.length;
  for (let i = 1, j = 0; i < n; i++) {
    let bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) { let t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
  }
  for (let len = 2; len <= n; len <<= 1) {
    const half = len >> 1, ang = (inv ? TAU : -TAU) / len, wr = Math.cos(ang), wi = Math.sin(ang);
    for (let i = 0; i < n; i += len) {
      let cr = 1, ci = 0;
      for (let k = 0; k < half; k++) {
        const a = i + k, b = a + half;
        const br = re[b] * cr - im[b] * ci, bi = re[b] * ci + im[b] * cr;
        re[b] = re[a] - br; im[b] = im[a] - bi; re[a] += br; im[a] += bi;
        const t = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = t;
      }
    }
  }
  if (inv) for (let i = 0; i < n; i++) { re[i] /= n; im[i] /= n; }
}
function specHarm(amp, ph) {
  const re = new Float64Array(TBL), im = new Float64Array(TBL), H = TBL / 2;
  for (let h = 1; h < amp.length && h < H; h++) {
    const a = amp[h]; if (!a) continue;
    const p = ph ? ph[h] : 0;
    re[h] = H * a * Math.sin(p); im[h] = -H * a * Math.cos(p);
    re[TBL - h] = re[h]; im[TBL - h] = -im[h];
  }
  return [re, im];
}
function specCycle(fn) {
  const re = new Float64Array(TBL), im = new Float64Array(TBL);
  for (let i = 0; i < TBL; i++) { const v = fn(i / TBL); re[i] = v === v ? v : 0; }
  fft(re, im, false);
  return [re, im];
}
const _fr = new Float64Array(TBL), _fi = new Float64Array(TBL);
function writeMips(sp, out, base) {
  const sr = sp[0], si = sp[1];
  let scale = 1;
  for (let l = 0; l < NLEV; l++) {
    const M = MIPH[l];
    _fr.fill(0); _fi.fill(0);
    for (let h = 1; h <= M; h++) { _fr[h] = sr[h]; _fi[h] = si[h]; _fr[TBL - h] = sr[TBL - h]; _fi[TBL - h] = si[TBL - h]; }
    fft(_fr, _fi, true);
    if (l === 0) {
      let pk = 0;
      for (let i = 0; i < TBL; i++) { const v = Math.abs(_fr[i]); if (v > pk) pk = v; }
      scale = pk > 1e-9 ? 1 / pk : 0;
    }
    const o = base + l * TBL;
    for (let i = 0; i < TBL; i++) out[o + i] = _fr[i] * scale;
  }
}
const SINE = new Float32Array(TBL + 1);
for (let i = 0; i <= TBL; i++) SINE[i] = Math.sin(TAU * i / TBL);
const SAW = new Float32Array(NLEV * TBL), TRI = new Float32Array(NLEV * TBL);
(function () {
  const a = new Float64Array(1024), b = new Float64Array(1024);
  for (let h = 1; h < 1024; h++) {
    a[h] = -2 / (Math.PI * h);
    if (h & 1) b[h] = (8 / (Math.PI * Math.PI)) * ((((h - 1) >> 1) & 1) ? -1 : 1) / (h * h);
  }
  writeMips(specHarm(a), SAW, 0); writeMips(specHarm(b), TRI, 0);
})();
function rdT(t, o, p) { const x = p * TBL, i = x | 0, f = x - i, a = t[o + i]; return a + f * (t[o + ((i + 1) & TMASK)] - a); }
function lvlFor(freq, sr) {
  const mh = 0.5 * sr / (freq > 1 ? freq : 1);
  if (mh >= 512) return 0;
  const l = Math.ceil(2 * Math.log2(512 / mh));
  return l > NLEV - 1 ? NLEV - 1 : l;
}

/* ------------------------------------------------------------------ vowels */
const VOWELS = [
  [[800, 1150, 2900, 3900], [1, 0.5, 0.25, 0.1]],
  [[400, 1600, 2700, 3300], [1, 0.3, 0.2, 0.1]],
  [[270, 2140, 2950, 3900], [1, 0.25, 0.2, 0.1]],
  [[450, 800, 2830, 3800], [1, 0.5, 0.15, 0.08]],
  [[325, 700, 2530, 3500], [1, 0.3, 0.08, 0.04]]
];
const VBW = [80, 90, 120, 130];
const _vF = [0, 0, 0, 0], _vG = [0, 0, 0, 0];
function vowelAt(v) {
  const x = clamp(v, 0, 1) * 4, i = Math.min(3, x | 0), f = x - i, A = VOWELS[i], B = VOWELS[i + 1];
  for (let k = 0; k < 4; k++) { _vF[k] = A[0][k] + f * (B[0][k] - A[0][k]); _vG[k] = A[1][k] + f * (B[1][k] - A[1][k]); }
  return [_vF, _vG];
}
function formantAmp(hz, F, G, bw) {
  let s = 0;
  for (let k = 0; k < 4; k++) { const d = (hz - F[k]) / (VBW[k] * bw); s += G[k] / (1 + d * d); }
  return s;
}

/* ------------------------------------------------------------------ wavetables */
function harm(fn) { const a = new Float64Array(1024); for (let h = 1; h < 1024; h++) { const v = fn(h); a[h] = (v > 0 || v < 0) ? v : 0; } return specHarm(a); }
const RSP = [];
for (let j = 0; j < 4; j++) { const a = new Float64Array(1024); for (let h = 1; h <= 96; h++) a[h] = Math.pow(hash01(j * 977 + h * 31), 2) / Math.pow(h, 0.6); RSP.push(a); }
const DB_H = [1, 3, 2, 4, 6, 8, 10, 12, 16];
const DB_REG = [[0,0,8,0,0,0,0,0,0],[8,8,8,0,0,0,0,0,0],[8,8,8,8,0,0,0,0,0],[6,8,8,6,0,0,0,0,0],[8,8,8,8,8,8,0,0,0],[8,6,8,8,6,8,4,4,8],[8,8,8,8,8,8,8,8,8]];
const METAL_SET = [1, 3, 7, 11, 13, 17, 23, 29, 31, 37, 41];

const WT_DEFS = {
  'Classic': t => specCycle(p => {
    const s = Math.sin(TAU * p), tri = p < 0.25 ? 4 * p : p < 0.75 ? 2 - 4 * p : 4 * p - 4, saw = 2 * p - 1, sq = p < 0.5 ? 1 : -1, x = t * 4;
    if (x < 1) return s + (tri - s) * x;
    if (x < 2) return tri + (saw - tri) * (x - 1);
    if (x < 3) return saw + (sq - saw) * (x - 2);
    return p < 0.5 - 0.42 * (x - 3) ? 1 : -1;
  }),
  'PWM': t => specCycle(p => (p < 0.5 - 0.46 * t ? 1 : -1)),
  'Sync Saw': t => specCycle(p => { const q = (p * (1 + 7 * t)) % 1; return 2 * q - 1; }),
  'Harmonic Rise': t => { const N = 1 + 63 * t * t; return harm(h => (h <= N ? 1 / h : (h < N + 1 ? (N + 1 - h) / h : 0))); },
  'Odd Rise': t => { const N = 1 + 63 * t * t; return harm(h => ((h & 1) ? (h <= N ? 1 / h : (h < N + 2 ? (N + 2 - h) / (2 * h) : 0)) : 0)); },
  'Vowel': t => { const v = vowelAt(t); const F = v[0].slice(), G = v[1].slice(); return harm(h => formantAmp(h * 110, F, G, 1) / Math.sqrt(h)); },
  'Choir': t => { const v = vowelAt(0.75 - 0.75 * t); const F = v[0].slice(), G = v[1].slice(); return harm(h => formantAmp(h * 130, F, G, 1.6) * Math.exp(-h / 40) / Math.sqrt(h)); },
  'Vocal Pulse': t => { const v = vowelAt(t); const F = v[0].slice(), G = v[1].slice(); return harm(h => Math.abs(Math.sin(Math.PI * h * 0.3)) / h * formantAmp(h * 120, F, G, 1.2)); },
  'Drawbars': t => {
    const x = t * (DB_REG.length - 1), j = Math.min(DB_REG.length - 2, x | 0), f = x - j, lev = [];
    for (let k = 0; k < 9; k++) lev.push(DB_REG[j][k] + f * (DB_REG[j + 1][k] - DB_REG[j][k]));
    return harm(h => { let s = 0; for (let k = 0; k < 9; k++) if (DB_H[k] === h) { const L = lev[k]; if (L > 0.01) s += Math.pow(10, (L - 8) * 0.15) * Math.min(1, L); } return s; });
  },
  'Fold': t => specCycle(p => Math.sin(Math.PI / 2 * Math.sin(TAU * p) * (1 + 9 * t))),
  'FM 1:1': t => specCycle(p => Math.sin(TAU * p + 6 * t * Math.sin(TAU * p))),
  'FM 2:1': t => specCycle(p => Math.sin(TAU * p + 5 * t * Math.sin(TAU * 2 * p))),
  'FM 3:1': t => specCycle(p => Math.sin(TAU * p + 4 * t * Math.sin(TAU * 3 * p))),
  'FM Bell': t => specCycle(p => Math.sin(TAU * p + 3 * t * Math.sin(TAU * 7 * p) + 1.5 * t * Math.sin(TAU * 2 * p))),
  'Phase Dist': t => specCycle(p => (1 - p) * Math.sin(TAU * p * (1 + 14 * t))),
  'Growl': t => { const hc = 2 + 40 * t * t, Q = 5; return harm(h => { const x = h / hc; return Math.pow(1 / Math.sqrt((1 - x * x) * (1 - x * x) + (x / Q) * (x / Q)), 1.5) / h; }); },
  'Comb': t => harm(h => (1 / h) * (0.5 + 0.5 * Math.cos(TAU * h * (0.05 + 0.25 * t)))),
  'Bit Sine': t => specCycle(p => { const lv = Math.max(1, Math.round(Math.pow(2, 5 - 4.5 * t))); return Math.round(Math.sin(TAU * p) * lv) / lv; }),
  'Digital Saw': t => specCycle(p => { const st = Math.round(64 - 58 * t); return 2 * Math.floor(p * st) / st - 1; }),
  'Glass': t => { const c1 = 3 + 8 * t, c2 = 7 + 18 * t; return harm(h => (h === 1 ? 1 : 0) + 0.7 * Math.exp(-((h - c1) * (h - c1)) / 2) + 0.5 * Math.exp(-((h - c2) * (h - c2)) / 3)); },
  'Metallic': t => harm(h => { const i = METAL_SET.indexOf(h); if (i < 0) return 0; return (1 - t) * Math.pow(0.7, i) + t * Math.pow(0.85, Math.abs(i - 5)); }),
  'Random Spectra': t => { const x = t * 3, j = Math.min(2, x | 0), f = x - j; return harm(h => RSP[j][h] * (1 - f) + RSP[j + 1][h] * f); },
  'Hollow': t => harm(h => ((h & 1) ? 1 / h : t * 0.8 / h)),
  'Reed': t => harm(h => ((h & 1) ? 1 / h : (0.15 + 0.5 * t) / h) * Math.exp(-h / (6 + 30 * t))),
  'Brass': t => harm(h => Math.exp(-h / (1.5 + 25 * t)) / Math.sqrt(h)),
  'Strings': t => harm(h => (1 / h) * (1 + 1.5 * Math.exp(-((h - (4 + 4 * t)) * (h - (4 + 4 * t))) / 6)) * Math.exp(-h / (20 + 60 * t))),
  'Octave Stack': t => harm(h => (h === 1 ? 1 : h === 2 ? 0.8 * t : h === 4 ? 0.6 * t * t : h === 8 ? 0.45 * t * t * t : h === 16 ? 0.3 * t * t * t * t : 0)),
  'Ghost': t => harm(h => (h === 1 ? 1 : (h < 64 ? t * 0.18 * hash01(h * 7 + 3) / Math.sqrt(h) : 0)))
};
const WT_NAMES = Object.keys(WT_DEFS);
const WTC = {};
function buildWT(name) {
  if (WTC[name]) return WTC[name];
  const def = WT_DEFS[name] || WT_DEFS.Classic, F = 16;
  const data = new Float32Array(F * NLEV * TBL);
  for (let i = 0; i < F; i++) writeMips(def(i / (F - 1)), data, i * NLEV * TBL);
  return (WTC[name] = { name: name, F: F, data: data });
}

/* ------------------------------------------------------------------ modal models */
const MODELS = {
  'Marimba': { r: [1, 3.99, 10.65, 18.8], a: [1, 0.35, 0.12, 0.04], d: [1, 0.4, 0.22, 0.12] },
  'Vibes': { r: [1, 3.98, 9.2, 15.6], a: [1, 0.45, 0.15, 0.06], d: [1, 0.55, 0.3, 0.18] },
  'Glock': { r: [1, 2.756, 5.404, 8.933, 13.344, 19.93], a: [1, 0.55, 0.35, 0.22, 0.14, 0.08], d: [1, 0.75, 0.55, 0.4, 0.3, 0.22] },
  'Bell': { r: [0.5, 1, 1.183, 1.506, 2, 2.514, 2.662, 3.011, 4.166, 5.433, 6.796, 8.215], a: [0.55, 1, 0.65, 0.4, 0.65, 0.3, 0.28, 0.3, 0.22, 0.15, 0.1, 0.07], d: [1.5, 1.1, 0.9, 0.75, 0.85, 0.55, 0.5, 0.45, 0.35, 0.28, 0.22, 0.18] },
  'Tine': { r: [1, 2, 3, 4, 7.15, 11.3], a: [1, 0.3, 0.1, 0.05, 0.18, 0.06], d: [1, 0.55, 0.4, 0.3, 0.08, 0.05] },
  'Kalimba': { r: [1, 5.95, 13.1, 2.01], a: [1, 0.25, 0.07, 0.05], d: [1, 0.25, 0.12, 0.5] },
  'Glass': { r: [1, 2.32, 4.25, 6.63, 9.38, 12.53], a: [1, 0.5, 0.35, 0.25, 0.16, 0.1], d: [1, 0.9, 0.8, 0.7, 0.6, 0.5] },
  'Steel Pan': { r: [1, 2, 3, 4.03, 5.08, 6.1], a: [1, 0.7, 0.4, 0.22, 0.12, 0.07], d: [1, 0.8, 0.55, 0.4, 0.3, 0.22] },
  'Gong': { r: [1, 1.42, 1.83, 2.27, 2.71, 3.2, 3.72, 4.29, 4.9, 5.6, 6.3, 7.1], a: [1, 0.8, 0.75, 0.6, 0.55, 0.5, 0.4, 0.35, 0.3, 0.25, 0.2, 0.15], d: [1, 0.95, 0.9, 0.85, 0.8, 0.75, 0.7, 0.62, 0.55, 0.5, 0.45, 0.4] },
  'Drum': { r: [1, 1.593, 2.136, 2.296, 2.653, 2.918, 3.156, 3.501], a: [1, 0.8, 0.65, 0.6, 0.5, 0.42, 0.35, 0.3], d: [1, 0.6, 0.5, 0.45, 0.4, 0.36, 0.32, 0.28] },
  'Harmonic': { r: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12], a: [1, 0.5, 0.33, 0.25, 0.2, 0.17, 0.14, 0.12, 0.11, 0.1, 0.09, 0.08], d: [1, 0.85, 0.72, 0.62, 0.54, 0.47, 0.41, 0.36, 0.32, 0.28, 0.25, 0.22] },
  'Piano': { r: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10], a: [1, 0.6, 0.35, 0.25, 0.18, 0.12, 0.09, 0.07, 0.05, 0.04], d: [1, 0.8, 0.62, 0.5, 0.4, 0.33, 0.28, 0.24, 0.2, 0.17] },
  'Music Box': { r: [1, 6.27, 17.55, 2], a: [1, 0.3, 0.1, 0.08], d: [1, 0.3, 0.12, 0.6] },
  'Crystal': { r: [1, 2.01, 3.03, 4.08, 5.15, 6.24], a: [1, 0.6, 0.45, 0.3, 0.2, 0.12], d: [1, 0.95, 0.9, 0.85, 0.8, 0.75] },
  'Plate': { r: [1, 1.71, 2.47, 3.12, 3.94, 4.73, 5.51, 6.3], a: [1, 0.7, 0.6, 0.5, 0.42, 0.35, 0.3, 0.25], d: [1, 0.8, 0.7, 0.6, 0.52, 0.45, 0.4, 0.35] },
  'Wood': { r: [1, 2.57, 4.9, 7.6], a: [1, 0.4, 0.2, 0.1], d: [1, 0.3, 0.15, 0.08] }
};
const MODEL_NAMES = Object.keys(MODELS);

/* ------------------------------------------------------------------ names & params */
const MODE_NAMES = ['analog', 'wt', 'pluck', 'modal', 'formant', 'noise', 'gran', 'sample'];
const FTYPES = ['off', 'lp12', 'lp24', 'ladder', 'hp12', 'hp24', 'bp', 'notch', 'peak', 'comb', 'combn', 'vowel'];
const SRC_IDS = ['none', 'e1', 'e2', 'l1', 'l2', 'l3', 'l4', 'vel', 'key', 'mw', 'at', 'rnd', 'm1', 'm2', 'm3', 'm4', 'ox', 'oy', 'alt', 'pb'];
const GLOBAL_SRCS = ['mw', 'at', 'm1', 'm2', 'm3', 'm4', 'ox', 'oy', 'pb'];
const LDST = ['pitch', 'shp', 'pw', 'sync', 'pos', 'fm', 'ring', 'fold', 'sub', 'nz', 'det', 'spr', 'pdmp', 'mbri', 'vow', 'fsh', 'brth', 'ncol', 'gpos', 'gsz', 'gden', 'gspr', 'gpit', 'sst', 'f1c', 'f1r', 'f1d', 'f2c', 'f2r', 'fbal', 'lvl', 'pan', 'aatk', 'adec', 'arel', 'fdec'];
const NLD = LDST.length;
const I = {}; LDST.forEach((k, i) => { I[k] = i; });
const GDST = ['l1r', 'l2r', 'l3r', 'l4r'];
const FXDST = ['drv.amt', 'lofi.mix', 'cho.mix', 'pha.mix', 'gate.depth', 'dly.mix', 'dly.fb', 'rev.mix', 'eq.low', 'eq.high', 'width'];
const DIVS = [['8 bar', 32], ['4 bar', 16], ['2 bar', 8], ['1 bar', 4], ['1/2', 2], ['1/2T', 4 / 3], ['1/4.', 1.5], ['1/4', 1], ['1/4T', 2 / 3], ['1/8.', 0.75], ['1/8', 0.5], ['1/8T', 1 / 3], ['1/16.', 0.375], ['1/16', 0.25], ['1/16T', 1 / 6], ['1/32', 0.125]];
const DIVM = {}; DIVS.forEach(d => { DIVM[d[0]] = d[1]; });
function divBeats(n) { return DIVM[n] || 1; }
const SCALES = {
  'Major': [0, 2, 4, 5, 7, 9, 11], 'Minor': [0, 2, 3, 5, 7, 8, 10], 'Dorian': [0, 2, 3, 5, 7, 9, 10], 'Phrygian': [0, 1, 3, 5, 7, 8, 10],
  'Lydian': [0, 2, 4, 6, 7, 9, 11], 'Mixolydian': [0, 2, 4, 5, 7, 9, 10], 'Harmonic Minor': [0, 2, 3, 5, 7, 8, 11], 'Melodic Minor': [0, 2, 3, 5, 7, 9, 11],
  'Pentatonic Major': [0, 2, 4, 7, 9], 'Pentatonic Minor': [0, 3, 5, 7, 10], 'Blues': [0, 3, 5, 6, 7, 10], 'Phrygian Dominant': [0, 1, 4, 5, 7, 8, 10],
  'Hirajoshi': [0, 2, 3, 7, 8], 'Whole Tone': [0, 2, 4, 6, 8, 10]
};
const CHORDS = {
  'Major': [0, 4, 7], 'Minor': [0, 3, 7], 'Maj7': [0, 4, 7, 11], 'Min7': [0, 3, 7, 10], 'Dom7': [0, 4, 7, 10], 'Min9': [0, 3, 7, 10, 14],
  'Maj9': [0, 4, 7, 11, 14], 'Min11': [0, 3, 7, 10, 14, 17], 'Sus2': [0, 2, 7], 'Sus4': [0, 5, 7], 'Add9': [0, 4, 7, 14], 'Min6': [0, 3, 7, 9],
  'Dim': [0, 3, 6], 'Dim7': [0, 3, 6, 9], 'Aug': [0, 4, 8], 'Power': [0, 7, 12], 'Octave': [0, 12],
  'Diatonic Triad': 3, 'Diatonic 7th': 4, 'Diatonic 9th': 5
};
const ARP_MODES = ['up', 'down', 'updown', 'downup', 'random', 'order', 'chord'];

function defLayer(on) {
  return {
    on: on ? 1 : 0, mode: 'analog', lvl: 0.8, pan: 0, crs: 0, fin: 0, kt: 1, vs: 0.6,
    shp: 0.667, pw: 0.5, sync: 0, wt: 'Classic', pos: 0, fm: 0, fmr: 1, ring: 0, rngr: 2, fold: 0, sub: 0, nz: 0,
    uni: 1, det: 15, spr: 0.5, drift: 0.15,
    pdec: 2, pdmp: 0.4, ppos: 0.2, pexc: 0.6,
    mdl: 'Bell', mbri: 0.5, mdec: 1.5, mhard: 0.5, minh: 0, mbow: 0,
    vow: 0, fsh: 0.5, brth: 0.1, ntyp: 0, ncol: 0.6,
    src: 'Choir Aah', gpos: 0.3, gspr: 0.1, gsz: 0.45, gden: 0.55, gpit: 0, grev: 0, gscan: 0, sst: 0, sloop: 1,
    f1t: 'lp24', f1c: 0.8, f1r: 0.1, f1d: 0, f1k: 0.5, f1e: 0, f1v: 0,
    f2t: 'off', f2c: 0.6, f2r: 0.1, f2d: 0, f2k: 0.5, f2e: 0, f2v: 0, frt: 0, fbal: 0.5,
    aa: 0.005, ad: 0.5, as: 1, ar: 0.3, fa: 0.005, fd: 0.5, fs: 0.3, fr: 0.5
  };
}
function defPatch() {
  return {
    name: 'Init', cat: 'Init', tags: [],
    L: [defLayer(1), defLayer(0), defLayer(0), defLayer(0)],
    g: { poly: 8, vmode: 0, glide: 0, gmode: 0, bend: 2, vol: 0.8, trim: 1 },
    me: [{ a: 0.005, d: 0.5, s: 0, r: 0.5, loop: 0 }, { a: 0.5, d: 1, s: 0.5, r: 1, loop: 0 }],
    lfo: [0, 1, 2, 3].map(i => ({ shp: 0, rate: [5, 0.3, 0.15, 0.4][i], sync: 0, div: '1/4', fade: 0, rtg: i === 0 ? 1 : 0, ph: 0 })),
    mods: [],
    mac: [{ n: 'Macro 1', v: 0 }, { n: 'Macro 2', v: 0 }, { n: 'Macro 3', v: 0 }, { n: 'Macro 4', v: 0 }],
    orb: { x: 0, y: 0, ch: 0, sp: 0.3 },
    arp: { on: 0, mode: 'up', div: '1/16', oct: 1, gate: 0.6, swing: 0, len: 16, latch: 0, vel: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1], tr: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    chord: { on: 0, type: 'Min7', spread: 0 }, scale: { on: 0, root: 0, type: 'Minor' },
    fx: {
      drv: { on: 0, type: 0, amt: 0.3, tone: 0.75, mix: 1 },
      lofi: { on: 0, bits: 12, rate: 1, wow: 0.25, flut: 0.15, noise: 0.15, tone: 0.7, mix: 1 },
      gate: { on: 0, div: '1/16', depth: 1, smooth: 0.25, pat: [1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0] },
      cho: { on: 0, mode: 0, rate: 0.5, depth: 0.5, fb: 0, mix: 0.4 },
      pha: { on: 0, rate: 0.25, depth: 0.7, fb: 0.4, mix: 0.5 },
      eq: { on: 0, low: 0, mid: 0, mf: 1000, high: 0 },
      dly: { on: 0, sync: 1, div: '1/8.', time: 0.35, fb: 0.35, pp: 1, lc: 250, hc: 5000, mod: 0.15, mix: 0.2 },
      rev: { on: 1, type: 0, size: 0.6, decay: 3, damp: 0.5, pre: 0.02, lc: 180, mix: 0.22 },
      comp: { on: 0, thr: -18, ratio: 3, gain: 3 },
      width: 1
    }
  };
}
const MERGE_IDX = { L: 1, me: 1, lfo: 1, mac: 1 };
function merge(dst, src) {
  for (const k in src) {
    const sv = src[k], dv = dst[k];
    if (sv && typeof sv === 'object' && !Array.isArray(sv) && dv && typeof dv === 'object' && !Array.isArray(dv)) merge(dv, sv);
    else if (Array.isArray(sv) && Array.isArray(dv) && MERGE_IDX[k]) {
      for (let i = 0; i < sv.length && i < dv.length; i++) {
        if (sv[i] == null) continue;
        if (typeof sv[i] === 'object' && typeof dv[i] === 'object') merge(dv[i], sv[i]); else dv[i] = sv[i];
      }
    } else dst[k] = Array.isArray(sv) ? sv.slice() : sv;
  }
  return dst;
}
function normalizePatch(p) { return merge(defPatch(), JSON.parse(JSON.stringify(p || {}))); }
function getPath(o, path) { const ks = path.split('.'); for (const k of ks) { if (o == null) return undefined; o = o[k]; } return o; }
function setPath(o, path, v) {
  const ks = path.split('.');
  for (let i = 0; i < ks.length - 1; i++) { if (o[ks[i]] == null) o[ks[i]] = {}; o = o[ks[i]]; }
  o[ks[ks.length - 1]] = v;
}

/* ------------------------------------------------------------------ filters */
function mkF() { return { a: 0, b: 0, c: 0, d: 0, s1: 0, s2: 0, s3: 0, s4: 0, g: -1, buf: null, w: 0, v: new Float64Array(8) }; }
function resetF(f) { f.a = f.b = f.c = f.d = f.s1 = f.s2 = f.s3 = f.s4 = 0; f.g = -1; if (f.buf) f.buf.fill(0); f.v.fill(0); }
function flushF(f) {
  if (f.a < 1e-18 && f.a > -1e-18) f.a = 0; if (f.b < 1e-18 && f.b > -1e-18) f.b = 0;
  if (f.c < 1e-18 && f.c > -1e-18) f.c = 0; if (f.d < 1e-18 && f.d > -1e-18) f.d = 0;
  if (f.s1 < 1e-18 && f.s1 > -1e-18) f.s1 = 0; if (f.s2 < 1e-18 && f.s2 > -1e-18) f.s2 = 0;
  if (f.s3 < 1e-18 && f.s3 > -1e-18) f.s3 = 0; if (f.s4 < 1e-18 && f.s4 > -1e-18) f.s4 = 0;
}
function cutHz(c, sr) { const hz = 20 * Math.pow(2, c * 9.9658); return hz > sr * 0.45 ? sr * 0.45 : hz; }
function svf1(f, buf, n, g0, g1, k, mode) {
  let ic1 = f.a, ic2 = f.b, g = g0;
  const dg = (g1 - g0) / n;
  for (let i = 0; i < n; i++) {
    g += dg;
    const a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
    const v0 = buf[i], v3 = v0 - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
    buf[i] = mode === 0 ? v2 : mode === 1 ? v0 - k * v1 - v2 : mode === 2 ? k * v1 : mode === 3 ? v0 - k * v1 : 2 * v2 - v0 + k * v1;
  }
  f.a = ic1; f.b = ic2;
}
function svf2(f, buf, n, g0, g1, k1, k2, hp) {
  let a1s = f.a, a2s = f.b, b1s = f.c, b2s = f.d, g = g0;
  const dg = (g1 - g0) / n;
  for (let i = 0; i < n; i++) {
    g += dg;
    let a1 = 1 / (1 + g * (g + k1)), a2 = g * a1, a3 = g * a2;
    let v0 = buf[i], v3 = v0 - a2s, v1 = a1 * a1s + a2 * v3, v2 = a2s + a2 * a1s + a3 * v3;
    a1s = 2 * v1 - a1s; a2s = 2 * v2 - a2s;
    const y1 = hp ? v0 - k1 * v1 - v2 : v2;
    a1 = 1 / (1 + g * (g + k2)); a2 = g * a1; a3 = g * a2;
    v0 = y1; v3 = v0 - b2s; v1 = a1 * b1s + a2 * v3; v2 = b2s + a2 * b1s + a3 * v3;
    b1s = 2 * v1 - b1s; b2s = 2 * v2 - b2s;
    buf[i] = hp ? v0 - k2 * v1 - v2 : v2;
  }
  f.a = a1s; f.b = a2s; f.c = b1s; f.d = b2s;
}
function ladder(f, buf, n, g0, g1, k) {
  let s1 = f.s1, s2 = f.s2, s3 = f.s3, s4 = f.s4, g = g0;
  const dg = (g1 - g0) / n, comp = 1 + k * 0.42;
  for (let i = 0; i < n; i++) {
    g += dg;
    const G = g / (1 + g), b = 1 / (1 + g), G2 = G * G, G3 = G2 * G, G4 = G3 * G;
    const S = G3 * b * s1 + G2 * b * s2 + G * b * s3 + b * s4;
    const x = buf[i] * comp;
    const y4e = (G4 * x + S) / (1 + k * G4);
    const u = ftanh(x - k * y4e);
    let v = (u - s1) * G; const y1 = v + s1; s1 = y1 + v;
    v = (y1 - s2) * G; const y2 = v + s2; s2 = y2 + v;
    v = (y2 - s3) * G; const y3 = v + s3; s3 = y3 + v;
    v = (y3 - s4) * G; const y4 = v + s4; s4 = y4 + v;
    buf[i] = y4;
  }
  f.s1 = s1; f.s2 = s2; f.s3 = s3; f.s4 = s4;
}
function comb(f, buf, n, D, fb) {
  if (!f.buf) { f.buf = new Float32Array(4096); f.w = 0; }
  const B = f.buf, att = 1 - Math.abs(fb) * 0.6;
  let w = f.w;
  for (let i = 0; i < n; i++) {
    let rp = w - D; if (rp < 0) rp += 4096;
    const i0 = rp | 0, fr = rp - i0, a = B[i0], d = a + fr * (B[(i0 + 1) & 4095] - a);
    let y = buf[i] + fb * d;
    if (y > 4) y = 4; else if (y < -4) y = -4;
    B[w] = y; w = (w + 1) & 4095;
    buf[i] = y * att;
  }
  f.w = w;
}
const _vg = new Float64Array(4), _vk = new Float64Array(4), _va = new Float64Array(12), _vw = new Float64Array(4);
function vowelF(f, buf, n, pos, res, sr, shift) {
  const fa = vowelAt(pos), F = fa[0], G = fa[1], v = f.v, bw = 1.6 - res * 1.2, sh = shift || 1;
  for (let k = 0; k < 4; k++) {
    const hz = Math.min(F[k] * sh, sr * 0.45), g = Math.tan(Math.PI * hz / sr), kk = Math.min(1.9, VBW[k] * bw * sh / hz);
    const a1 = 1 / (1 + g * (g + kk));
    _va[k * 3] = a1; _va[k * 3 + 1] = g * a1; _va[k * 3 + 2] = g * g * a1; _vk[k] = kk; _vw[k] = G[k] * kk;
  }
  for (let i = 0; i < n; i++) {
    const x = buf[i]; let out = 0;
    for (let k = 0; k < 4; k++) {
      const j = k * 2, ic1 = v[j], ic2 = v[j + 1], a1 = _va[k * 3], a2 = _va[k * 3 + 1], a3 = _va[k * 3 + 2];
      const v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
      v[j] = 2 * v1 - ic1; v[j + 1] = 2 * v2 - ic2;
      out += _vw[k] * v1;
    }
    buf[i] = out * 1.7;
  }
  for (let j = 0; j < 8; j++) if (v[j] < 1e-18 && v[j] > -1e-18) v[j] = 0;
}
function filt(f, type, buf, n, cut, res, drv, sr) {
  if (drv > 0.001) { const dg = 1 + drv * 7, nm = 1 / Math.sqrt(dg); for (let i = 0; i < n; i++) buf[i] = ftanh(buf[i] * dg) * nm; }
  const hz = cutHz(cut, sr);
  if (type >= 1 && type <= 8) {
    const g1 = Math.tan(Math.PI * hz / sr), g0 = f.g < 0 ? g1 : f.g;
    f.g = g1;
    switch (type) {
      case 1: svf1(f, buf, n, g0, g1, 1.414 - 1.374 * res, 0); break;
      case 2: svf2(f, buf, n, g0, g1, 1.848, 0.765 - 0.725 * res, 0); break;
      case 3: ladder(f, buf, n, g0, g1, res * 3.9); break;
      case 4: svf1(f, buf, n, g0, g1, 1.414 - 1.374 * res, 1); break;
      case 5: svf2(f, buf, n, g0, g1, 1.848, 0.765 - 0.725 * res, 1); break;
      case 6: svf1(f, buf, n, g0, g1, 1.5 - 1.45 * res, 2); break;
      case 7: svf1(f, buf, n, g0, g1, 2 - 1.9 * res, 3); break;
      case 8: svf1(f, buf, n, g0, g1, 1.414 - 1.36 * res, 4); break;
    }
    flushF(f);
  } else if (type === 9 || type === 10) {
    comb(f, buf, n, clamp(sr / hz, 2, 4000), (type === 9 ? 1 : -1) * res * 0.96);
  } else if (type === 11) {
    vowelF(f, buf, n, cut, res, sr, 1);
  }
}

/* ------------------------------------------------------------------ envelopes, LFOs */
class Env {
  constructor() { this.st = 0; this.v = 0; }
  on() { this.st = 1; }
  off() { if (this.st !== 0) this.st = 4; }
  tick(a, d, s, r, n, sr, loop) {
    switch (this.st) {
      case 1:
        if (a <= 0.0005) { this.v = 1; this.st = 2; }
        else { this.v += (1.3 - this.v) * (1 - Math.exp(-n * 1.466 / (a * sr))); if (this.v >= 1) { this.v = 1; this.st = 2; } }
        break;
      case 2:
        this.v += (s - this.v) * (1 - Math.exp(-n * 4 / ((d > 0.001 ? d : 0.001) * sr)));
        if (Math.abs(this.v - s) < 1e-3) {
          this.v = s;
          if (loop) this.st = 1;
          else if (s < 1e-4) { this.st = 0; this.v = 0; }
          else this.st = 3;
        }
        break;
      case 3: this.v = s; if (s < 1e-4) { this.st = 0; this.v = 0; } break;
      case 4:
        this.v -= this.v * (1 - Math.exp(-n * 5 / ((r > 0.001 ? r : 0.001) * sr)));
        if (this.v < 1e-4) { this.v = 0; this.st = 0; }
        break;
    }
    return this.v;
  }
}
class LFO {
  constructor() { this.ph = 0; this.v = 0; this.sh = 0; this.r0 = 0; this.r1 = 0; this.t = 0; this.k = -1; }
  reset(p) { this.ph = p.ph || 0; this.t = 0; this.sh = rnd() * 2 - 1; this.r0 = this.sh; this.r1 = rnd() * 2 - 1; this.k = -1; }
  tick(p, id, hz, n, sr, gtime) {
    let ph;
    if (p.rtg) {
      this.ph += hz * n / sr;
      if (this.ph >= 1) { this.ph -= Math.floor(this.ph); this.sh = rnd() * 2 - 1; this.r0 = this.r1; this.r1 = rnd() * 2 - 1; }
      ph = this.ph;
    } else {
      const x = gtime * hz + (p.ph || 0), k = Math.floor(x);
      ph = x - k;
      if (k !== this.k) { this.k = k; this.sh = hash01(k * 7 + id * 1013) * 2 - 1; this.r0 = hash01(k * 13 + id * 7919) * 2 - 1; this.r1 = hash01((k + 1) * 13 + id * 7919) * 2 - 1; }
    }
    this.t += n / sr;
    let v;
    switch (p.shp | 0) {
      case 0: v = SINE[(ph * TBL) | 0]; break;
      case 1: v = ph < 0.25 ? 4 * ph : ph < 0.75 ? 2 - 4 * ph : 4 * ph - 4; break;
      case 2: v = 2 * ph - 1; break;
      case 3: v = 1 - 2 * ph; break;
      case 4: v = ph < 0.5 ? 1 : -1; break;
      case 5: v = this.sh; break;
      case 6: { const s = 0.5 - 0.5 * Math.cos(Math.PI * ph); v = this.r0 + (this.r1 - this.r0) * s; } break;
      default: v = 0;
    }
    if (p.fade > 0.001) { const f = this.t / p.fade; if (f < 1) v *= f; }
    return (this.v = v);
  }
}

/* ------------------------------------------------------------------ voice */
class LayerVoice {
  constructor() {
    this.aenv = new Env(); this.fenv = new Env();
    this.ph = new Float64Array(8); this.mph = new Float64Array(8); this.fph = new Float64Array(8); this.rph = new Float64Array(8);
    this.drift = new Float64Array(8); this.incs = new Float64Array(8); this.gl = new Float64Array(8); this.gr = new Float64Array(8);
    this.subph = 0; this.g0l = 0; this.g0r = 0;
    this.f1 = mkF(); this.f1b = mkF(); this.f2 = mkF(); this.f2b = mkF(); this.fo = mkF(); this.fob = mkF();
    this.done = true; this.mode = -1; this.quiet = 0;
    this.str = null; this.sw = 0; this.slp = new Float64Array(3);
    this.my1 = new Float64Array(12); this.my2 = new Float64Array(12); this.mc1 = new Float64Array(12); this.mc2 = new Float64Array(12);
    this.mg = new Float64Array(12); this.mb = new Float64Array(12); this.mpl = new Float64Array(12); this.mpr = new Float64Array(12);
    this.mkey = -1; this.mn = 0; this.imp = 0; this.exn = 0; this.exd = 0;
    this.pk = new Float64Array(8); this.brn = new Float64Array(2); this.hold = new Float64Array(2); this.clk = new Float64Array(2); this.hp = new Float64Array(2);
    this.nph = 0; this.wc = 0.5; this.wv = 0;
    this.gpos = new Float64Array(32); this.ginc = new Float64Array(32); this.gage = new Float64Array(32); this.gdur = new Float64Array(32);
    this.ggl = new Float64Array(32); this.ggr = new Float64Array(32); this.gn = 0; this.gcnt = 0; this.head = 0;
    this.spos = 0; this.sdone = false; this.sfirst = false;
  }
}

class Voice {
  constructor(e) {
    this.e = e; this.active = false; this.gate = false; this.fading = false; this.fadeG = 1; this.fadeT = 0.004;
    this.note = 60; this.vel = 0.8; this.age = 0; this.pitch = 60; this.target = 60; this.rand = 0; this.alt = 1;
    this.lay = [new LayerVoice(), new LayerVoice(), new LayerVoice(), new LayerVoice()];
    this.m0 = new Env(); this.m1 = new Env();
    this.lfo = [new LFO(), new LFO(), new LFO(), new LFO()];
    this.S = new Float64Array(SRC_IDS.length); this.M = new Float64Array(4 * NLD); this.gm = new Float64Array(4);
  }
  start(n, v, keep) {
    const e = this.e, P = e.patch, g = P.g;
    const was = this.active && !this.fading;
    this.age = 0;
    this.active = true; this.fading = false; this.fadeG = 1; this.gate = true; this.note = n; this.vel = v;
    const glideOn = g.glide > 0.001 && (g.gmode === 0 || keep) && e.lastNote >= 0;
    this.pitch = glideOn ? (was ? this.pitch : e.lastNote) : n;
    this.target = n; e.lastNote = n;
    this.rand = rnd() * 2 - 1; e.altF = -e.altF; this.alt = e.altF;
    this.m0.on(); this.m1.on();
    if (!was) { this.m0.v = 0; this.m1.v = 0; }
    for (let k = 0; k < 4; k++) if (P.lfo[k].rtg) this.lfo[k].reset(P.lfo[k]); else this.lfo[k].t = 0;
    for (let li = 0; li < 4; li++) {
      const L = P.L[li], lv = this.lay[li];
      if (!L.on) { lv.done = true; continue; }
      this.startLayer(li, L, lv, was && !lv.done);
    }
    e.ageCounter++;
  }
  startLayer(li, L, lv, cont) {
    const e = this.e, C = e.lc[li], sr = e.sr;
    const fresh = !cont || lv.mode !== C.mode;
    lv.done = false; lv.quiet = 0;
    lv.aenv.on(); lv.fenv.on();
    if (fresh) {
      resetF(lv.f1); resetF(lv.f1b); resetF(lv.f2); resetF(lv.f2b); resetF(lv.fo); resetF(lv.fob);
      lv.g0l = 0; lv.g0r = 0; lv.aenv.v = 0; lv.fenv.v = 0; lv.mode = C.mode;
      lv.drift.fill(0); lv.subph = 0; lv.gn = 0; lv.my1.fill(0); lv.my2.fill(0); lv.pk.fill(0); lv.brn.fill(0); lv.clk.fill(0); lv.hp.fill(0);
      const uni = L.uni | 0;
      for (let u = 0; u < 8; u++) { const r = uni > 1 ? rnd() : 0; lv.ph[u] = r; lv.mph[u] = r; lv.fph[u] = 0; lv.rph[u] = 0; }
      if (lv.str) { lv.str[0].fill(0); lv.str[1].fill(0); lv.str[2].fill(0); }
      lv.slp.fill(0);
    }
    const freq = mtof((L.kt ? this.pitch : 60) + L.crs + L.fin * 0.01);
    switch (C.mode) {
      case 2: this.excitePluck(L, lv, freq, sr); break;
      case 3: lv.imp = 1; lv.exn = L.mhard * 0.6; lv.exd = Math.exp(-1 / (0.0025 * sr)); lv.mkey = -1; break;
      case 6: lv.gcnt = 0; lv.head = 0; break;
      case 7: lv.sfirst = true; lv.sdone = false; break;
    }
  }
  excitePluck(L, lv, freq, sr) {
    if (!lv.str) lv.str = [new Float32Array(4096), new Float32Array(4096), new Float32Array(4096)];
    const ns = clamp(L.uni | 0, 1, 3), c = 0.04 + 0.96 * L.pexc * L.pexc;
    for (let s = 0; s < ns; s++) {
      const off = ns > 1 ? (s / (ns - 1)) * 2 - 1 : 0;
      const f = freq * Math.pow(2, L.det * off * 0.3 / 1200);
      const P = clamp(Math.round(sr / f), 2, 4000), pp = Math.max(1, Math.round(L.ppos * P));
      const ex = new Float32Array(P);
      let lp = 0, pk = 0;
      for (let j = 0; j < P; j++) { lp += c * ((rnd() * 2 - 1) - lp); ex[j] = lp; }
      for (let j = P - 1; j >= pp; j--) ex[j] -= ex[j - pp];
      for (let j = 0; j < P; j++) { const a = Math.abs(ex[j]); if (a > pk) pk = a; }
      const nm = pk > 1e-9 ? 0.9 / pk : 0, B = lv.str[s];
      for (let j = 0; j < P; j++) { const idx = (lv.sw - P + j) & 4095; B[idx] += ex[j] * nm; }
    }
  }
  legatoTo(n) { const e = this.e; this.note = n; this.target = n; if (e.patch.g.glide <= 0.001) this.pitch = n; e.lastNote = n; }
  release() {
    this.gate = false; this.m0.off(); this.m1.off();
    for (let li = 0; li < 4; li++) { const lv = this.lay[li]; lv.aenv.off(); lv.fenv.off(); }
  }
  startFade(secs = 0.004) { this.fading = true; this.fadeG = 1; this.fadeT = secs; this.gate = false; }
  level() {   // current loudness estimate (envelope x layer level), for picking which voice to steal
    const P = this.e.patch; let m = 0;
    for (let i = 0; i < 4; i++) if (!this.lay[i].done && P.L[i].on) m = Math.max(m, this.lay[i].aenv.v * P.L[i].lvl);
    return m;
  }
  kill() { this.active = false; this.fading = false; this.gate = false; for (let li = 0; li < 4; li++) this.lay[li].done = true; }

  render(off, n) {
    const e = this.e, P = e.patch, sr = e.sr;
    this.age += n;
    if (this.pitch !== this.target) {
      const gt = P.g.glide;
      if (gt <= 0.001) this.pitch = this.target;
      else { this.pitch += (this.target - this.pitch) * (1 - Math.exp(-n * 3 / (gt * sr))); if (Math.abs(this.target - this.pitch) < 0.002) this.pitch = this.target; }
    }
    const me = P.me;
    this.m0.tick(me[0].a, me[0].d, me[0].s, me[0].r, n, sr, me[0].loop);
    this.m1.tick(me[1].a, me[1].d, me[1].s, me[1].r, n, sr, me[1].loop);
    const S = this.S, gm = this.gm;
    for (let k = 0; k < 4; k++) {
      const lp = P.lfo[k];
      let hz = lp.sync ? e.bps / divBeats(lp.div) : lp.rate;
      if (gm[k]) hz *= Math.pow(2, gm[k] * 4);
      S[3 + k] = this.lfo[k].tick(lp, k + 1, hz, n, sr, e.gtime);
    }
    S[1] = this.m0.v; S[2] = this.m1.v; S[7] = this.vel; S[8] = (this.note - 60) / 24; S[9] = e.mw; S[10] = e.at; S[11] = this.rand;
    S[12] = e.mac[0]; S[13] = e.mac[1]; S[14] = e.mac[2]; S[15] = e.mac[3]; S[16] = e.ox; S[17] = e.oy; S[18] = this.alt; S[19] = e.pb;
    const M = this.M; M.fill(0); gm.fill(0);
    const mods = e.mods;
    for (let r = 0; r < mods.length; r++) {
      const m = mods[r];
      const val = S[m[0]] * m[2] * (m[1] ? S[m[1]] : 1);
      if (val === 0) continue;
      const t = m[3], p = m[4];
      if (t < 4) M[t * NLD + p] += val;
      else if (t === 4) { M[p] += val; M[NLD + p] += val; M[2 * NLD + p] += val; M[3 * NLD + p] += val; }
      else gm[p] += val;
    }
    const vL = e.vL, vR = e.vR;
    for (let i = 0; i < n; i++) { vL[i] = 0; vR[i] = 0; }
    let live = false;
    for (let li = 0; li < 4; li++) {
      const L = P.L[li], lv = this.lay[li];
      if (lv.done) continue;
      if (!L.on) { lv.done = true; continue; }
      if (this.renderLayer(li, L, lv, n)) live = true;
    }
    const mL = e.mixL, mR = e.mixR;
    if (this.fading) {
      const g0 = this.fadeG; this.fadeG = Math.max(0, g0 - n / ((this.fadeT || 0.004) * sr));
      const dg = (this.fadeG - g0) / n; let g = g0;
      for (let i = 0; i < n; i++) { g += dg; mL[off + i] += vL[i] * g; mR[off + i] += vR[i] * g; }
      if (this.fadeG <= 0) live = false;
    } else {
      for (let i = 0; i < n; i++) { mL[off + i] += vL[i]; mR[off + i] += vR[i]; }
    }
    if (!live) this.kill();
  }

  renderLayer(li, L, lv, n) {
    const e = this.e, sr = e.sr, C = e.lc[li], M = this.M, b = li * NLD;
    const ta = L.aa * (M[b + 32] ? Math.pow(2, M[b + 32] * 4) : 1);
    const td = L.ad * (M[b + 33] ? Math.pow(2, M[b + 33] * 4) : 1);
    const trl = L.ar * (M[b + 34] ? Math.pow(2, M[b + 34] * 4) : 1);
    lv.aenv.tick(ta, td, L.as, trl, n, sr, 0);
    lv.fenv.tick(L.fa, L.fd * (M[b + 35] ? Math.pow(2, M[b + 35] * 4) : 1), L.fs, L.fr, n, sr, 0);
    const semis = (L.kt ? this.pitch : 60) + L.crs + L.fin * 0.01 + M[b] * 24 + e.bendSemis;
    const freq = 440 * Math.pow(2, (semis - 69) / 12);
    let bL = e.sL, bR = e.sR, stereo = false;
    switch (C.mode) {
      case 0: case 1: stereo = this.oscAW(L, lv, C, freq, n, b); break;
      case 2: stereo = this.oscPluck(L, lv, freq, n, b); break;
      case 3: stereo = this.oscModal(L, lv, C, freq, n, b); break;
      case 4: stereo = this.oscFormant(L, lv, freq, n, b); break;
      case 5: stereo = this.oscNoise(L, lv, freq, n, b); break;
      case 6: stereo = this.oscGran(L, lv, C, semis, n, b); break;
      case 7: stereo = this.oscSample(L, lv, C, semis, n, b); break;
    }
    // filters
    const vel = this.vel, kn = (this.pitch - 60) / 119.6, fe = lv.fenv.v;
    const c1 = clamp(L.f1c + M[b + 24] + fe * L.f1e + L.f1k * kn + L.f1v * (vel - 1) * 0.4, 0, 1);
    const r1 = clamp(L.f1r + M[b + 25], 0, 1), d1 = clamp(L.f1d + M[b + 26], 0, 1);
    const c2 = clamp(L.f2c + M[b + 27] + fe * L.f2e + L.f2k * kn + L.f2v * (vel - 1) * 0.4, 0, 1);
    const r2 = clamp(L.f2r + M[b + 28], 0, 1), d2 = clamp(L.f2d, 0, 1);
    if (!L.frt) {
      if (C.f1) { filt(lv.f1, C.f1, bL, n, c1, r1, d1, sr); if (stereo) filt(lv.f1b, C.f1, bR, n, c1, r1, d1, sr); }
      if (C.f2) { filt(lv.f2, C.f2, bL, n, c2, r2, d2, sr); if (stereo) filt(lv.f2b, C.f2, bR, n, c2, r2, d2, sr); }
    } else if (C.f1 || C.f2) {
      const pL = e.sL2, pR = e.sR2;
      for (let i = 0; i < n; i++) { pL[i] = bL[i]; pR[i] = bR[i]; }
      if (C.f1) { filt(lv.f1, C.f1, bL, n, c1, r1, d1, sr); if (stereo) filt(lv.f1b, C.f1, bR, n, c1, r1, d1, sr); }
      if (C.f2) { filt(lv.f2, C.f2, pL, n, c2, r2, d2, sr); if (stereo) filt(lv.f2b, C.f2, pR, n, c2, r2, d2, sr); }
      const bal = clamp(L.fbal + M[b + 29], 0, 1), ba = 1 - bal;
      for (let i = 0; i < n; i++) { bL[i] = bL[i] * ba + pL[i] * bal; bR[i] = bR[i] * ba + pR[i] * bal; }
    }
    // amp
    const vg = 1 - L.vs + L.vs * Math.pow(vel, 1.6);
    const lm = M[b + 30] ? Math.max(0, 1 + M[b + 30]) : 1;
    const amp = lv.aenv.v * L.lvl * vg * lm * VOICE_GAIN;
    const pan = clamp(L.pan + M[b + 31], -1, 1);
    const gl1 = amp * (pan > 0 ? 1 - pan : 1), gr1 = amp * (pan < 0 ? 1 + pan : 1);
    const dl = (gl1 - lv.g0l) / n, dr = (gr1 - lv.g0r) / n, vL = e.vL, vR = e.vR;
    let gl = lv.g0l, gr = lv.g0r, pk = 0;
    if (stereo) {
      for (let i = 0; i < n; i++) { gl += dl; gr += dr; const a = bL[i], c = bR[i]; vL[i] += a * gl; vR[i] += c * gr; const q = a < 0 ? -a : a; if (q > pk) pk = q; }
    } else {
      for (let i = 0; i < n; i++) { gl += dl; gr += dr; const s = bL[i]; vL[i] += s * gl; vR[i] += s * gr; const q = s < 0 ? -s : s; if (q > pk) pk = q; }
    }
    lv.g0l = gl1; lv.g0r = gr1;
    if (lv.aenv.st === 0 || lv.sdone) { lv.done = true; return false; }
    if ((C.mode === 2 || C.mode === 3) && lv.aenv.st >= 2) {
      if (pk < 3e-5) { if (++lv.quiet > 40) { lv.done = true; return false; } } else lv.quiet = 0;
    }
    return true;
  }

  oscAW(L, lv, C, freq, n, b) {
    const e = this.e, sr = e.sr, M = this.M, bL = e.sL, bR = e.sR;
    const uni = clamp(L.uni | 0, 1, 7);
    const det = Math.max(0, L.det + M[b + 10] * 100), spr = clamp(L.spr + M[b + 11], 0, 1);
    const base = freq / sr, lo = lvlFor(freq * (uni > 1 ? Math.pow(2, det / 1200) : 1), sr) * TBL;
    const dr = L.drift, nrm = 1 / Math.sqrt(uni);
    for (let u = 0; u < uni; u++) {
      if (dr > 0) lv.drift[u] += (rnd() - 0.5) * 0.6 * dr - lv.drift[u] * 0.002;
      const o = uni > 1 ? (u / (uni - 1)) * 2 - 1 : 0;
      lv.incs[u] = base * Math.pow(2, (det * o + lv.drift[u] * 4 * dr) / 1200);
      const a = (o * spr + 1) * Math.PI / 4;
      lv.gl[u] = Math.cos(a) * nrm * 1.41421; lv.gr[u] = Math.sin(a) * nrm * 1.41421;
    }
    const fmA = clamp(L.fm + M[b + 5], 0, 1), pmS = fmA * fmA * 12 / TAU, fmr = L.fmr;
    const ring = clamp(L.ring + M[b + 6], 0, 1), rr = L.rngr, ring1 = 1 - ring;
    const fold = clamp(L.fold + M[b + 7], 0, 1), fg = 0.25 + fold * 2.5, fmix = Math.min(1, fold * 4);
    const sync = clamp(L.sync + M[b + 3], 0, 1), sratio = 1 + sync * 7;
    const useFM = pmS > 0, useRing = ring > 0, useFold = fold > 0, useSync = sync > 0.001;
    const ph = lv.ph, mph = lv.mph, fph = lv.fph, rph = lv.rph, incs = lv.incs, gla = lv.gl, gra = lv.gr;
    if (C.mode === 0) {
      const shp = clamp(L.shp + M[b + 1], 0, 1), x = shp * 3, seg = x >= 2 ? 2 : (x | 0), w = x - seg, w1 = 1 - w;
      let pw = clamp(L.pw + M[b + 2], 0.02, 0.98);
      for (let i = 0; i < n; i++) {
        let sl = 0, sr2 = 0;
        for (let u = 0; u < uni; u++) {
          const inc = incs[u];
          let p;
          if (useSync) { let m = mph[u] + inc; if (m >= 1) { m -= 1; p = m * sratio; } else p = ph[u] + inc * sratio; mph[u] = m; p -= Math.floor(p); }
          else { p = ph[u] + inc; if (p >= 1) p -= 1; }
          ph[u] = p;
          let q = p;
          if (useFM) { let f = fph[u] + inc * fmr; f -= Math.floor(f); fph[u] = f; q += pmS * SINE[(f * TBL) | 0]; q -= Math.floor(q); }
          let v;
          if (seg === 0) v = rdT(SINE, 0, q) * w1 + rdT(TRI, lo, q) * w;
          else if (seg === 1) v = rdT(TRI, lo, q) * w1 + rdT(SAW, lo, q) * w;
          else { let q2 = q + pw; if (q2 >= 1) q2 -= 1; const s = rdT(SAW, lo, q); v = s * w1 + (s - rdT(SAW, lo, q2)) * w; }
          if (useRing) { let r = rph[u] + inc * rr; r -= Math.floor(r); rph[u] = r; v *= ring1 + ring * SINE[(r * TBL) | 0]; }
          if (useFold) { const xx = v * fg; v += fmix * (rdT(SINE, 0, xx - Math.floor(xx)) - v); }
          sl += v * gla[u]; sr2 += v * gra[u];
        }
        bL[i] = sl; bR[i] = sr2;
      }
    } else {
      const T = C.wt, F = T.F, D = T.data, pos = clamp(L.pos + M[b + 4], 0, 1) * (F - 1);
      const f0 = Math.min(F - 2, pos | 0), w = pos - f0, w1 = 1 - w, oa = f0 * NLEV * TBL + lo, ob = oa + NLEV * TBL;
      for (let i = 0; i < n; i++) {
        let sl = 0, sr2 = 0;
        for (let u = 0; u < uni; u++) {
          const inc = incs[u];
          let p;
          if (useSync) { let m = mph[u] + inc; if (m >= 1) { m -= 1; p = m * sratio; } else p = ph[u] + inc * sratio; mph[u] = m; p -= Math.floor(p); }
          else { p = ph[u] + inc; if (p >= 1) p -= 1; }
          ph[u] = p;
          let q = p;
          if (useFM) { let f = fph[u] + inc * fmr; f -= Math.floor(f); fph[u] = f; q += pmS * SINE[(f * TBL) | 0]; q -= Math.floor(q); }
          const xq = q * TBL, iq = xq | 0, fq = xq - iq, i1 = (iq + 1) & TMASK;
          const a0 = D[oa + iq], b0 = D[ob + iq];
          let v = (a0 + fq * (D[oa + i1] - a0)) * w1 + (b0 + fq * (D[ob + i1] - b0)) * w;
          if (useRing) { let r = rph[u] + inc * rr; r -= Math.floor(r); rph[u] = r; v *= ring1 + ring * SINE[(r * TBL) | 0]; }
          if (useFold) { const xx = v * fg; v += fmix * (rdT(SINE, 0, xx - Math.floor(xx)) - v); }
          sl += v * gla[u]; sr2 += v * gra[u];
        }
        bL[i] = sl; bR[i] = sr2;
      }
    }
    const sub = clamp(L.sub + M[b + 8], 0, 1), nz = clamp(L.nz + M[b + 9], 0, 1);
    if (sub > 0.001) {
      let sp = lv.subph; const si = base * 0.5;
      for (let i = 0; i < n; i++) { sp += si; if (sp >= 1) sp -= 1; const s = rdT(SINE, 0, sp) * sub; bL[i] += s; bR[i] += s; }
      lv.subph = sp;
    }
    if (nz > 0.001) { const a = nz * 0.7; for (let i = 0; i < n; i++) { const w = (rnd() * 2 - 1) * a; bL[i] += w; bR[i] += w; } }
    return uni > 1 && spr > 0.01;
  }

  oscPluck(L, lv, freq, n, b) {
    const e = this.e, sr = e.sr, M = this.M, bL = e.sL, bR = e.sR;
    if (!lv.str) lv.str = [new Float32Array(4096), new Float32Array(4096), new Float32Array(4096)];
    const ns = clamp(L.uni | 0, 1, 3), damp = clamp(L.pdmp + M[b + 12], 0, 1), a = 1 - 0.88 * damp;
    const T60 = Math.max(0.05, L.pdec) * Math.pow(2, -(this.pitch - 60) / 36), spr = clamp(L.spr + M[b + 11], 0, 1);
    for (let i = 0; i < n; i++) { bL[i] = 0; bR[i] = 0; }
    const w0 = lv.sw;
    for (let s = 0; s < ns; s++) {
      const off = ns > 1 ? (s / (ns - 1)) * 2 - 1 : 0;
      const f = freq * Math.pow(2, L.det * off * 0.3 / 1200);
      const D = clamp(sr / f - (1 - a) / a, 2, 4000), g = Math.pow(10, -3 / (T60 * f));
      const pa = (off * spr + 1) * Math.PI / 4, gl = Math.cos(pa) * 1.41421 / Math.sqrt(ns), gr = Math.sin(pa) * 1.41421 / Math.sqrt(ns);
      const B = lv.str[s];
      let w = w0, lp = lv.slp[s];
      for (let i = 0; i < n; i++) {
        let rp = w - D; if (rp < 0) rp += 4096;
        const i0 = rp | 0, fr = rp - i0, x0 = B[i0], y = x0 + fr * (B[(i0 + 1) & 4095] - x0);
        lp += a * (y - lp);
        B[w] = lp * g; w = (w + 1) & 4095;
        bL[i] += y * gl * 2; bR[i] += y * gr * 2;
      }
      lv.slp[s] = (lp < 1e-18 && lp > -1e-18) ? 0 : lp;
    }
    lv.sw = (w0 + n) & 4095;
    return ns > 1 && spr > 0.01;
  }

  oscModal(L, lv, C, freq, n, b) {
    const e = this.e, sr = e.sr, M = this.M, bL = e.sL, bR = e.sR, md = C.mdl;
    const bri = clamp(L.mbri + M[b + 13], 0, 1), spr = clamp(L.spr + M[b + 11], 0, 1);
    const key = freq * 1.000137 + bri * 7.1 + L.mdec * 13.3 + L.minh * 17.7 + L.mhard * 3.3 + spr * 2.9 + L.mbow * 5.1;
    const np = md.r.length;
    if (key !== lv.mkey) {
      lv.mkey = key;
      const B = L.minh * 0.002, ks = Math.pow(2, -(this.pitch - 60) / 24), tilt = (bri - 0.5) * 2, hard = -(1 - L.mhard) * 0.8;
      let m = 0;
      for (let k = 0; k < np; k++) {
        const r0 = md.r[k], rk = r0 * Math.sqrt(1 + B * r0 * r0), wv = TAU * freq * rk / sr;
        if (wv > 2.9) continue;
        const T60 = Math.max(0.02, L.mdec * md.d[k] * ks), R = Math.exp(-6.9078 / (T60 * sr));
        const amp = md.a[k] * Math.pow(rk, tilt + hard);
        lv.mc1[m] = 2 * R * Math.cos(wv); lv.mc2[m] = R * R; lv.mg[m] = amp * Math.sin(wv); lv.mb[m] = amp * Math.sin(wv) * Math.sqrt(2 * (1 - R * R)) * 2.5;
        const pn = (k & 1 ? -1 : 1) * spr * Math.min(1, k * 0.5), pa = (pn + 1) * Math.PI / 4;
        lv.mpl[m] = Math.cos(pa) * 1.41421; lv.mpr[m] = Math.sin(pa) * 1.41421;
        m++;
      }
      lv.mn = m;
    }
    const nm = lv.mn, c1 = lv.mc1, c2 = lv.mc2, gI = lv.mg, gB = lv.mb, y1 = lv.my1, y2 = lv.my2, pl = lv.mpl, pr = lv.mpr;
    const bow = L.mbow > 0.001 ? L.mbow * 0.5 * (0.3 + 0.7 * lv.aenv.v) : 0;
    let imp = lv.imp, exn = lv.exn; const exd = lv.exd;
    for (let i = 0; i < n; i++) {
      let x = imp; imp = 0;
      if (exn > 1e-5) { x += (rnd() * 2 - 1) * exn; exn *= exd; }
      const bw = bow > 0 ? (rnd() * 2 - 1) * bow : 0;
      let sl = 0, sr2 = 0;
      for (let k = 0; k < nm; k++) {
        const y = c1[k] * y1[k] - c2[k] * y2[k] + gI[k] * x + gB[k] * bw;
        y2[k] = y1[k]; y1[k] = y; sl += y * pl[k]; sr2 += y * pr[k];
      }
      bL[i] = sl; bR[i] = sr2;
    }
    for (let k = 0; k < nm; k++) { if (y1[k] < 1e-18 && y1[k] > -1e-18) y1[k] = 0; if (y2[k] < 1e-18 && y2[k] > -1e-18) y2[k] = 0; }
    lv.imp = imp; lv.exn = exn;
    return spr > 0.01;
  }

  oscFormant(L, lv, freq, n, b) {
    const e = this.e, sr = e.sr, M = this.M, bL = e.sL, bR = e.sR;
    const uni = clamp(L.uni | 0, 1, 7), det = Math.max(0, L.det + M[b + 10] * 100), spr = clamp(L.spr + M[b + 11], 0, 1);
    const base = freq / sr, lo = lvlFor(freq * Math.pow(2, det / 1200), sr) * TBL, dr = Math.max(L.drift, 0.2), nrm = 1 / Math.sqrt(uni);
    for (let u = 0; u < uni; u++) {
      lv.drift[u] += (rnd() - 0.5) * 0.6 * dr - lv.drift[u] * 0.002;
      const o = uni > 1 ? (u / (uni - 1)) * 2 - 1 : 0;
      lv.incs[u] = base * Math.pow(2, (det * o + lv.drift[u] * 4 * dr) / 1200);
      const a = (o * spr + 1) * Math.PI / 4;
      lv.gl[u] = Math.cos(a) * nrm * 1.41421; lv.gr[u] = Math.sin(a) * nrm * 1.41421;
    }
    const shp = clamp(L.shp + M[b + 1], 0, 1), brth = clamp(L.brth + M[b + 16], 0, 1) * 1.6, ph = lv.ph, incs = lv.incs, gla = lv.gl, gra = lv.gr;
    for (let i = 0; i < n; i++) {
      let sl = 0, sr2 = 0;
      for (let u = 0; u < uni; u++) {
        let p = ph[u] + incs[u]; if (p >= 1) p -= 1; ph[u] = p;
        let q2 = p + 0.5; if (q2 >= 1) q2 -= 1;
        const s = rdT(SAW, lo, p), v = s + shp * (-rdT(SAW, lo, q2));
        sl += v * gla[u]; sr2 += v * gra[u];
      }
      if (brth > 0) { const w = (rnd() * 2 - 1) * brth; sl += w; sr2 += w * 0.9 + (rnd() * 2 - 1) * brth * 0.1; }
      bL[i] = sl; bR[i] = sr2;
    }
    const vow = clamp(L.vow + M[b + 14], 0, 1), sh = Math.pow(2, (clamp(L.fsh + M[b + 15], 0, 1) - 0.5) * 1.4);
    vowelF(lv.fo, bL, n, vow, 0.5, sr, sh);
    vowelF(lv.fob, bR, n, vow, 0.5, sr, sh);
    for (let i = 0; i < n; i++) { bL[i] *= 1.5; bR[i] *= 1.5; }
    return true;
  }

  oscNoise(L, lv, freq, n, b) {
    const e = this.e, sr = e.sr, M = this.M, bL = e.sL, bR = e.sR;
    const col = clamp(L.ncol + M[b + 17], 0, 1), type = L.ntyp | 0, spr = clamp(L.spr + M[b + 11], 0, 1), st = spr > 0.01;
    if (type === 4) { lv.wv += (rnd() - 0.5) * 0.08; lv.wv *= 0.995; lv.wc = clamp(lv.wc + lv.wv * 0.02, 0, 1); if (lv.wc <= 0 || lv.wc >= 1) lv.wv = -lv.wv; }
    for (let c = 0; c < (st ? 2 : 1); c++) {
      const out = c ? bR : bL, pk = lv.pk, o = c * 4;
      let b0 = pk[o], b1 = pk[o + 1], b2 = pk[o + 2], br = lv.brn[c], cl = lv.clk[c], hp = lv.hp[c];
      switch (type) {
        case 0:
          for (let i = 0; i < n; i++) {
            const w = rnd() * 2 - 1;
            b0 = 0.99765 * b0 + w * 0.099046; b1 = 0.963 * b1 + w * 0.2965164; b2 = 0.57 * b2 + w * 1.0526913;
            const pink = (b0 + b1 + b2 + w * 0.1848) * 0.22;
            br = (br + 0.02 * w) / 1.02; const brown = br * 3.5;
            out[i] = (col < 0.5 ? brown + (pink - brown) * col * 2 : pink + (w * 0.6 - pink) * (col - 0.5) * 2) * 0.8;
          }
          break;
        case 1: {
          const prob = 0.0004 + col * 0.006;
          for (let i = 0; i < n; i++) {
            const w = rnd() * 2 - 1;
            br = (br + 0.02 * w) / 1.02;
            if (rnd() < prob) { const r = rnd(); cl += (rnd() < 0.5 ? -1 : 1) * (0.25 + r * r * 0.75); }
            cl *= 0.62;
            out[i] = (cl + br * 0.25 + w * 0.012) * 3;
          }
        } break;
        case 2: {
          const prob = 0.004 + col * 0.03;
          for (let i = 0; i < n; i++) {
            const w = rnd() * 2 - 1;
            b0 = 0.99765 * b0 + w * 0.099046; b1 = 0.963 * b1 + w * 0.2965164; b2 = 0.57 * b2 + w * 1.0526913;
            if (rnd() < prob) { const r = rnd(); cl += (rnd() < 0.5 ? -1 : 1) * r * r * 0.8; }
            const x = cl; cl *= 0.35;
            hp += 0.15 * (x - hp);
            out[i] = (x - hp) * 1.3 + (b0 + b1 + b2) * 0.03;
          }
        } break;
        case 3: {
          let nph = lv.nph, hold = lv.hold[c]; const inc = freq / sr;
          for (let i = 0; i < n; i++) { nph += inc; if (nph >= 1) { nph -= Math.floor(nph); hold = rnd() * 2 - 1; } out[i] = (hold * (0.4 + col * 0.4) + (rnd() * 2 - 1) * (1 - col) * 0.15) * 1.5; }
          if (!c || !st) lv.nph = nph; lv.hold[c] = hold;
        } break;
        default: {
          const hz = Math.min(250 * Math.pow(2, lv.wc * 3.5 + (col - 0.5) * 2), sr * 0.4), g = Math.tan(Math.PI * hz / sr), k = 0.3, a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
          let i1 = cl, i2 = hp;
          for (let i = 0; i < n; i++) {
            const w = rnd() * 2 - 1;
            b0 = 0.99765 * b0 + w * 0.099046; b1 = 0.963 * b1 + w * 0.2965164; b2 = 0.57 * b2 + w * 1.0526913;
            const x = (b0 + b1 + b2 + w * 0.1848) * 0.22, v3 = x - i2, v1 = a1 * i1 + a2 * v3, v2 = i2 + a2 * i1 + a3 * v3;
            i1 = 2 * v1 - i1; i2 = 2 * v2 - i2;
            out[i] = k * v1 * 3 + x * 0.08;
          }
          cl = i1; hp = i2;
        }
      }
      pk[o] = b0; pk[o + 1] = b1; pk[o + 2] = b2; lv.brn[c] = br; lv.clk[c] = cl; lv.hp[c] = hp;
    }
    return st;
  }

  oscGran(L, lv, C, semis, n, b) {
    const e = this.e, sr = e.sr, M = this.M, bL = e.sL, bR = e.sR, src = C.src;
    if (!src) { for (let i = 0; i < n; i++) { bL[i] = 0; bR[i] = 0; } return false; }
    const data = src.data, len = data.length, rsr = src.sr / sr;
    const pos = clamp(L.gpos + M[b + 18], 0, 1), spray = clamp(L.gspr + M[b + 21], 0, 1);
    const sizeS = 0.005 * Math.pow(200, clamp(L.gsz + M[b + 19], 0, 1)), dens = Math.pow(200, clamp(L.gden + M[b + 20], 0, 1));
    const scat = clamp(L.gpit + M[b + 22], 0, 1) * 12, spr = clamp(L.spr + M[b + 11], 0, 1);
    const st = (L.kt ? semis - src.root : semis - 60);
    lv.head += L.gscan * n * rsr / len; lv.head -= Math.floor(lv.head);
    const interval = sr / dens, norm = 1.25 / Math.sqrt(Math.max(1, dens * sizeS)), durS = Math.max(16, sizeS * sr);
    const gp = lv.gpos, gi = lv.ginc, ga = lv.gage, gd = lv.gdur, gl = lv.ggl, gr = lv.ggr;
    let gn = lv.gn, cnt = lv.gcnt;
    for (let i = 0; i < n; i++) {
      if (--cnt <= 0) {
        cnt += interval * (0.5 + rnd());
        if (gn < 32) {
          let p = pos + lv.head + (rnd() * 2 - 1) * spray * 0.5; p -= Math.floor(p);
          gp[gn] = p * (len - 1); gd[gn] = durS; ga[gn] = 0;
          let inc = rsr * Math.pow(2, (st + (rnd() * 2 - 1) * scat) / 12);
          if (L.grev > 0 && rnd() < L.grev) inc = -inc;
          gi[gn] = inc;
          const pa = ((rnd() * 2 - 1) * spr + 1) * Math.PI / 4; gl[gn] = Math.cos(pa) * 1.41421; gr[gn] = Math.sin(pa) * 1.41421;
          gn++;
        }
      }
      let sl = 0, sr2 = 0;
      for (let k = 0; k < gn; k++) {
        let p = gp[k];
        if (p >= len - 1) p -= len - 1; else if (p < 0) p += len - 1;
        const i0 = p | 0, f = p - i0, a = data[i0], s = a + f * (data[i0 + 1] - a);
        const t = ga[k] / gd[k], win = 0.5 - 0.5 * SINE[((t + 0.25) * TBL) & TMASK];
        const v = s * win; sl += v * gl[k]; sr2 += v * gr[k];
        gp[k] = p + gi[k];
        if (++ga[k] >= gd[k]) { gn--; gp[k] = gp[gn]; gi[k] = gi[gn]; ga[k] = ga[gn]; gd[k] = gd[gn]; gl[k] = gl[gn]; gr[k] = gr[gn]; k--; }
      }
      bL[i] = sl * norm; bR[i] = sr2 * norm;
    }
    lv.gn = gn; lv.gcnt = cnt;
    return true;
  }

  oscSample(L, lv, C, semis, n, b) {
    const e = this.e, sr = e.sr, M = this.M, bL = e.sL, src = C.src;
    if (!src) { for (let i = 0; i < n; i++) bL[i] = 0; return false; }
    const data = src.data, len = data.length;
    if (lv.sfirst) { lv.sfirst = false; lv.spos = clamp(L.sst + M[b + 23], 0, 0.99) * len; }
    const st = (L.kt ? semis - src.root : semis - 60);
    const inc = src.sr / sr * Math.pow(2, st / 12);
    const loop = L.sloop && src.le > src.ls + 256, ls = src.ls, le = src.le, ll = le - ls, xf = Math.min(ll * 0.25, src.sr * 0.03);
    let p = lv.spos;
    for (let i = 0; i < n; i++) {
      if (p >= (loop ? le : len - 3)) {
        if (loop) p -= ll;
        else { lv.sdone = true; for (let j = i; j < n; j++) bL[j] = 0; break; }
      }
      let s = herm(data, p, len);
      if (loop && p > le - xf) { const t = (le - p) / xf; s = s * t + herm(data, p - ll, len) * (1 - t); }
      bL[i] = s; p += inc;
    }
    lv.spos = p;
    return false;
  }
}
function herm(d, p, len) {
  const i = p | 0, f = p - i, x0 = d[i > 0 ? i - 1 : 0], x1 = d[i], x2 = d[i + 1 < len ? i + 1 : len - 1], x3 = d[i + 2 < len ? i + 2 : len - 1];
  const c = (x2 - x0) * 0.5, v = x1 - x2, w = c + v, a = w + v + (x3 - x1) * 0.5, bb = w + a;
  return ((a * f - bb) * f + c) * f + x1;
}

/* ------------------------------------------------------------------ arpeggiator */
class Arp {
  constructor(e) { this.e = e; this.held = []; this.play = []; this.running = false; this.k = 0; this.i = 0; this.nb = 0; this.next = 0; this.cur = -1; this.phys = 0; }
  reset() { this.stop(); this.held.length = 0; this.phys = 0; }
  has(n) { for (const h of this.held) if (h.n === n) return true; return false; }
  on(n, v) {
    const A = this.e.patch.arp;
    if (A.latch && this.phys === 0) this.held.length = 0;
    this.phys++;
    for (let i = 0; i < this.held.length; i++) if (this.held[i].n === n) { this.held.splice(i, 1); break; }
    this.held.push({ n: n, v: v });
    if (!this.running) { this.running = true; this.k = 0; this.i = 0; this.nb = this.e.time; this.next = this.e.time; }
  }
  off(n) {
    const A = this.e.patch.arp;
    this.phys = Math.max(0, this.phys - 1);
    if (A.latch) return;
    for (let i = 0; i < this.held.length; i++) if (this.held[i].n === n) { this.held.splice(i, 1); break; }
    if (!this.held.length) this.stop();
  }
  stop() { this.running = false; this.cur = -1; for (const p of this.play) this.e.vOff(p.n); this.play.length = 0; }
  tick(t0, n) {
    const e = this.e, A = e.patch.arp;
    for (let i = this.play.length - 1; i >= 0; i--) { const p = this.play[i]; if (p.off < t0 + n) { e.vOff(p.n); this.play.splice(i, 1); } }
    if (!this.running) return;
    const stepLen = 60 / e.bpm * divBeats(A.div) * e.sr;
    let guard = 0;
    while (this.next < t0 + n && guard++ < 8) {
      this.doStep(stepLen);
      this.k++; this.nb += stepLen;
      this.next = this.nb + ((this.k & 1) ? clamp(A.swing, 0, 1) * stepLen * 0.33 : 0);
    }
  }
  doStep(stepLen) {
    const e = this.e, A = e.patch.arp, len = clamp(A.len | 0, 1, 16), s = this.k % len;
    this.cur = s;
    const vs = A.vel[s];
    if (!(vs > 0) || !this.held.length) return;
    const tr = A.tr[s] | 0, notes = this.pick(A), gl = Math.max(0.05, A.gate) * stepLen;
    for (const nn of notes) {
      const note = clamp(nn.n + tr, 0, 127), vel = clamp(nn.v * (0.35 + 0.65 * vs), 0.05, 1);
      for (let i = this.play.length - 1; i >= 0; i--) if (this.play[i].n === note) this.play.splice(i, 1);
      e.vOn(note, vel);
      this.play.push({ n: note, off: this.next + gl });
    }
  }
  pick(A) {
    const H = this.held, oct = clamp(A.oct | 0, 1, 4);
    if (A.mode === 'chord') return H.slice();
    const base = A.mode === 'order' ? H.slice() : H.slice().sort((x, y) => x.n - y.n);
    const seq = [];
    for (let o = 0; o < oct; o++) for (const h of base) seq.push({ n: h.n + 12 * o, v: h.v });
    const L = seq.length;
    let idx = 0;
    switch (A.mode) {
      case 'down': idx = L - 1 - (this.i % L); break;
      case 'updown': { const per = Math.max(1, 2 * L - 2), k = this.i % per; idx = k < L ? k : per - k; } break;
      case 'downup': { const per = Math.max(1, 2 * L - 2), k = this.i % per; idx = L - 1 - (k < L ? k : per - k); } break;
      case 'random': idx = (rnd() * L) | 0; break;
      default: idx = this.i % L;
    }
    this.i++;
    return [seq[clamp(idx, 0, L - 1)]];
  }
}

/* ------------------------------------------------------------------ master FX (in-engine part) */
function rdR(B, w, d) { let rp = w - d; if (rp < 0) rp += 8192; const i0 = rp | 0, f = rp - i0, a = B[i0]; return a + f * (B[(i0 + 1) & 8191] - a); }
const TUBE_B = ftanh(0.25);
class MasterFX {
  constructor(sr) {
    this.sr = sr; this.mod = {};
    this.dl = new Float64Array(2);
    this.lb = [new Float32Array(8192), new Float32Array(8192)]; this.ld = [new Float32Array(8192), new Float32Array(8192)];
    this.lw = 0; this.hc = 0; this.hold = new Float64Array(2); this.wph = 0; this.fph = 0; this.lt = new Float64Array(2); this.ck = new Float64Array(2);
    this.gv = 1;
    this.cb = [new Float32Array(8192), new Float32Array(8192)]; this.cw = 0; this.cph = 0; this.cph2 = 0; this.cfl = new Float64Array(2);
    this.pst = [new Float64Array(12), new Float64Array(12)]; this.plast = new Float64Array(2); this.pph = 0;
  }
  process(e, L, R, n) {
    const F = e.patch.fx, sr = this.sr, Mo = this.mod;
    const d = F.drv;
    if (d.on) {
      const amt = clamp(d.amt + (Mo['drv.amt'] || 0), 0, 1);
      if (amt > 0.002) {
        const pre = 1 + amt * amt * 24, post = 1 / Math.sqrt(pre), mix = clamp(d.mix, 0, 1), type = d.type | 0;
        const lc = 1 - Math.exp(-TAU * Math.min(600 * Math.pow(2, d.tone * 5), sr * 0.45) / sr);
        for (let c = 0; c < 2; c++) {
          const B = c ? R : L; let lp = this.dl[c];
          for (let i = 0; i < n; i++) {
            const x = B[i], u = x * pre; let y;
            switch (type) {
              case 1: y = ftanh(u + 0.25) - TUBE_B; break;
              case 2: y = u > 1 ? 1 : (u < -1 ? -1 : u); break;
              case 3: y = (u > 0 ? 1 - Math.exp(-u * 1.5) : Math.exp(u * 1.5) - 1) * 1.1; break;
              case 4: y = Math.sin(u * 1.5707963); break;
              default: y = ftanh(u);
            }
            lp += lc * (y * post - lp);
            B[i] = x + mix * (lp - x);
          }
          this.dl[c] = (lp < 1e-18 && lp > -1e-18) ? 0 : lp;
        }
      }
    }
    const lf = F.lofi;
    if (lf.on) {
      const mix = clamp(lf.mix + (Mo['lofi.mix'] || 0), 0, 1);
      if (mix > 0.001) {
        const bits = clamp(lf.bits, 2, 16), q = bits >= 15.9 ? 0 : 2 / Math.pow(2, bits), rate = clamp(lf.rate, 1, 40);
        const wow = clamp(lf.wow, 0, 1), flut = clamp(lf.flut, 0, 1), nz = clamp(lf.noise, 0, 1);
        const lc = 1 - Math.exp(-TAU * Math.min(400 * Math.pow(2, lf.tone * 5.5), sr * 0.45) / sr);
        const B0 = this.lb[0], B1 = this.lb[1], D0 = this.ld[0], D1 = this.ld[1];
        let w = this.lw, hc = this.hc, h0 = this.hold[0], h1 = this.hold[1], wp = this.wph, fp = this.fph, l0 = this.lt[0], l1 = this.lt[1], c0 = this.ck[0], c1 = this.ck[1];
        const wi = 0.5 / sr, fi = 6.5 / sr, base = Math.round(0.004 * sr), wd = wow * 0.0028 * sr, fd = flut * 0.00035 * sr;
        for (let i = 0; i < n; i++) {
          const xl = L[i], xr = R[i];
          D0[w] = xl; D1[w] = xr;
          hc += 1; if (hc >= rate) { hc -= rate; h0 = xl; h1 = xr; }
          let yl = rate > 1.01 ? h0 : xl, yr = rate > 1.01 ? h1 : xr;
          if (q > 0) { yl = Math.round(yl / q) * q; yr = Math.round(yr / q) * q; }
          B0[w] = yl; B1[w] = yr;
          wp += wi * (1 + 0.25 * SINE[((wp * 0.37) % 1 * TBL) | 0]); if (wp >= 1) wp -= 1;
          fp += fi; if (fp >= 1) fp -= 1;
          const dd = base + wd * (1 + SINE[(wp * TBL) | 0]) + fd * (1 + SINE[(fp * TBL) | 0]);
          yl = rdR(B0, w, dd); yr = rdR(B1, w, dd);
          const di = (w - base) & 8191, dxl = D0[di], dxr = D1[di];
          w = (w + 1) & 8191;
          if (nz > 0) {
            const hs = (rnd() * 2 - 1) * nz * 0.012;
            if (rnd() < nz * 0.0005) { const a = (rnd() * 2 - 1) * nz * 0.6; c0 += a; c1 += a * (0.6 + rnd() * 0.4); }
            c0 *= 0.55; c1 *= 0.55; yl += hs + c0; yr += hs + c1;
          }
          l0 += lc * (yl - l0); l1 += lc * (yr - l1);
          L[i] = dxl + mix * (l0 - dxl); R[i] = dxr + mix * (l1 - dxr);
        }
        this.lw = w; this.hc = hc; this.hold[0] = h0; this.hold[1] = h1; this.wph = wp; this.fph = fp;
        this.lt[0] = Math.abs(l0) < 1e-18 ? 0 : l0; this.lt[1] = Math.abs(l1) < 1e-18 ? 0 : l1;
        this.ck[0] = Math.abs(c0) < 1e-18 ? 0 : c0; this.ck[1] = Math.abs(c1) < 1e-18 ? 0 : c1;
      }
    }
    const gt = F.gate;
    if (gt.on) {
      const depth = clamp(gt.depth + (Mo['gate.depth'] || 0), 0, 1);
      const stepLen = 60 / e.bpm * divBeats(gt.div) * sr;
      const coef = 1 - Math.exp(-1 / (sr * (0.0008 + clamp(gt.smooth, 0, 1) * 0.04)));
      const t0 = e.time - e.gateStart, pat = gt.pat;
      let gv = this.gv;
      for (let i = 0; i < n; i++) {
        const st = Math.floor((t0 + i) / stepLen) & 15;
        gv += ((pat[st] ? 1 : 1 - depth) - gv) * coef; L[i] *= gv; R[i] *= gv;
      }
      this.gv = gv;
    }
    const ch = F.cho;
    if (ch.on) {
      const mix = clamp(ch.mix + (Mo['cho.mix'] || 0), 0, 1);
      if (mix > 0.001) {
        const mode = ch.mode | 0, B0 = this.cb[0], B1 = this.cb[1];
        let w = this.cw, ph = this.cph, ph2 = this.cph2, fl = this.cfl[0], fr = this.cfl[1];
        const rate = clamp(ch.rate, 0.02, 10), dep = clamp(ch.depth, 0, 1), fb = mode === 2 ? clamp(ch.fb, -0.95, 0.95) : 0;
        const pi = rate / sr, pi2 = 5.7 / sr;
        let base, depS;
        if (mode === 0) { base = 0.0065 * sr; depS = dep * 0.0045 * sr; }
        else if (mode === 1) { base = 0.009 * sr; depS = dep * 0.0032 * sr; }
        else { base = 0.0006 * sr; depS = dep * 0.0045 * sr; }
        const vib = 0.00022 * sr;
        for (let i = 0; i < n; i++) {
          const xl = L[i], xr = R[i];
          B0[w] = xl + fb * fl; B1[w] = xr + fb * fr;
          ph += pi; if (ph >= 1) ph -= 1; ph2 += pi2; if (ph2 >= 1) ph2 -= 1;
          let wl, wr;
          if (mode === 1) {
            const s1 = SINE[(ph * TBL) | 0], s2 = SINE[(((ph + 0.3333) % 1) * TBL) | 0], s3 = SINE[(((ph + 0.6667) % 1) * TBL) | 0];
            const v1 = SINE[(ph2 * TBL) | 0], v2 = SINE[(((ph2 + 0.3333) % 1) * TBL) | 0], v3 = SINE[(((ph2 + 0.6667) % 1) * TBL) | 0];
            const d1 = base + depS * s1 + vib * v1, d2 = base + depS * s2 + vib * v2, d3 = base + depS * s3 + vib * v3;
            wl = (rdR(B0, w, d1) + rdR(B0, w, d2) * 0.7 + rdR(B1, w, d3) * 0.5) * 0.6;
            wr = (rdR(B1, w, d2) + rdR(B1, w, d3) * 0.7 + rdR(B0, w, d1) * 0.5) * 0.6;
          } else {
            const dl = base + depS * (0.5 + 0.5 * SINE[(ph * TBL) | 0]), dr = base + depS * (0.5 + 0.5 * SINE[(((ph + 0.25) % 1) * TBL) | 0]);
            wl = rdR(B0, w, dl); wr = rdR(B1, w, dr);
          }
          fl = wl; fr = wr;
          w = (w + 1) & 8191;
          L[i] = xl + mix * (wl - xl); R[i] = xr + mix * (wr - xr);
        }
        this.cw = w; this.cph = ph; this.cph2 = ph2;
        this.cfl[0] = Math.abs(fl) < 1e-18 ? 0 : fl; this.cfl[1] = Math.abs(fr) < 1e-18 ? 0 : fr;
      }
    }
    const pz = F.pha;
    if (pz.on) {
      const mix = clamp(pz.mix + (Mo['pha.mix'] || 0), 0, 1);
      if (mix > 0.001) {
        const rate = clamp(pz.rate, 0.01, 10), dep = clamp(pz.depth, 0, 1), fb = clamp(pz.fb, -0.95, 0.95);
        let ph = this.pph;
        for (let o = 0; o < n; o += CB) {
          const m = Math.min(CB, n - o);
          ph += rate * m / sr; if (ph >= 1) ph -= 1;
          for (let c = 0; c < 2; c++) {
            const lfv = 0.5 + 0.5 * SINE[(((ph + c * 0.25) % 1) * TBL) | 0];
            const fc = Math.min(160 * Math.pow(2, dep * 5.6 * lfv), sr * 0.4), t = Math.tan(Math.PI * fc / sr), a = (t - 1) / (t + 1);
            const B = c ? R : L, s = this.pst[c];
            let last = this.plast[c];
            for (let i = o; i < o + m; i++) {
              let x = B[i] + fb * last;
              for (let k = 0; k < 6; k++) { const y = a * (x - s[2 * k + 1]) + s[2 * k]; s[2 * k] = x; s[2 * k + 1] = y; x = y; }
              last = x;
              B[i] = B[i] * (1 - 0.5 * mix) + x * 0.5 * mix;
            }
            this.plast[c] = Math.abs(last) < 1e-18 ? 0 : last;
            for (let k = 0; k < 12; k++) if (s[k] < 1e-18 && s[k] > -1e-18) s[k] = 0;
          }
        }
        this.pph = ph;
      }
    }
  }
}

/* ------------------------------------------------------------------ engine */
class Engine {
  constructor(sr, post) {
    this.sr = sr; this.post = post || null;
    this.tables = {}; this.sources = {};
    this.patch = normalizePatch({});
    this.voices = []; for (let i = 0; i < POOL; i++) this.voices.push(new Voice(this));
    this.time = 0; this.gtime = 0; this.bpm = 90; this.bps = 1.5;
    this.mw = 0; this.mwT = 0; this.at = 0; this.atT = 0; this.pb = 0; this.pbT = 0; this.bendSemis = 0;
    this.mac = [0, 0, 0, 0]; this.ox = 0; this.oy = 0; this.orbT = 0;
    this.lastNote = -1; this.altF = 1; this.ageCounter = 0; this.gateStart = 0;
    this.sus = false; this.susSet = new Set(); this.inMap = new Map(); this.cnt = new Int32Array(128); this.mono = [];
    this.arp = new Arp(this); this.fx = new MasterFX(sr);
    this.sL = new Float32Array(CB); this.sR = new Float32Array(CB); this.sL2 = new Float32Array(CB); this.sR2 = new Float32Array(CB);
    this.vL = new Float32Array(CB); this.vR = new Float32Array(CB);
    this.mixL = new Float32Array(256); this.mixR = new Float32Array(256);
    this.statN = 0; this.lastNotesKey = ''; this.lastStep = -2;
    this.mods = []; this.lc = [];
    this.compile();
  }
  getTable(name) {
    if (this.tables[name]) return this.tables[name];
    if (WTC[name]) return (this.tables[name] = WTC[name]);
    return (this.tables[name] = buildWT(WT_DEFS[name] ? name : 'Classic'));
  }
  compile() {
    const P = this.patch;
    this.lc = P.L.map(L => ({
      mode: Math.max(0, MODE_NAMES.indexOf(L.mode)),
      f1: Math.max(0, FTYPES.indexOf(L.f1t)), f2: Math.max(0, FTYPES.indexOf(L.f2t)),
      wt: L.mode === 'wt' && L.on ? this.getTable(L.wt) : null,
      mdl: MODELS[L.mdl] || MODELS.Bell,
      src: this.sources[L.src] || null
    }));
    const ms = [];
    for (const m of P.mods || []) {
      if (!m || !m.a) continue;
      const s = SRC_IDS.indexOf(m.s); if (s <= 0) continue;
      const v = m.v ? SRC_IDS.indexOf(m.v) : 0;
      const d = String(m.d || ''), dot = d.indexOf('.'); if (dot < 0) continue;
      const t = d.slice(0, dot), pn = d.slice(dot + 1);
      let tgt, pi;
      if (t === 'G') { tgt = 5; pi = GDST.indexOf(pn); }
      else if (t === 'FX') continue;
      else { tgt = t === '*' ? 4 : 'ABCD'.indexOf(t); pi = LDST.indexOf(pn); }
      if (tgt < 0 || pi < 0) continue;
      ms.push([s, v > 0 ? v : 0, +m.a, tgt, pi]);
    }
    this.mods = ms;
  }
  msg(m) {
    switch (m.t) {
      case 'patch': this.patch = normalizePatch(m.p); this.compile(); this.allFade(); this.arp.reset(); this.inMap.clear(); this.cnt.fill(0); this.mono.length = 0; this.susSet.clear(); break;
      case 'set': setPath(this.patch, m.k, m.v); this.compile(); if (m.k === 'arp.on' && !m.v) this.arp.stop(); break;
      case 'on': this.noteIn(m.n, m.v); break;
      case 'off': this.noteOut(m.n); break;
      case 'bpm': this.bpm = clamp(+m.v || 90, 20, 300); break;
      case 'mw': this.mwT = m.v; break;
      case 'at': this.atT = m.v; break;
      case 'pb': this.pbT = m.v; break;
      case 'sus': this.setSus(!!m.v); break;
      case 'wt': this.tables[m.name] = { name: m.name, F: m.F, data: m.data }; this.compile(); break;
      case 'src': this.sources[m.name] = { data: m.data, sr: m.sr, root: m.root, ls: m.ls, le: m.le }; this.compile(); break;
      case 'fxmod': this.fx.mod = m.d || {}; break;
      case 'panic': this.panic(); break;
    }
  }
  panic() { for (const v of this.voices) v.kill(); this.arp.reset(); this.inMap.clear(); this.cnt.fill(0); this.mono.length = 0; this.susSet.clear(); }
  allFade() { for (const v of this.voices) if (v.active) v.startFade(); }
  setSus(on) {
    this.sus = on;
    if (!on) { for (const x of this.susSet) if (this.cnt[x] === 0) this.vOff(x); this.susSet.clear(); }
  }
  snap(n) {
    const sc = this.patch.scale, deg = SCALES[sc.type] || SCALES.Minor, rel = (((n - sc.root) % 12) + 12) % 12;
    let best = 0; for (const d of deg) if (d <= rel) best = d;
    return n - (rel - best);
  }
  chordOf(base) {
    const ch = this.patch.chord, def = CHORDS[ch.type] || CHORDS.Min7;
    let iv;
    if (typeof def === 'number') {
      const sc = this.patch.scale, deg = SCALES[sc.type] || SCALES.Minor, b = this.snap(base), rel = (((b - sc.root) % 12) + 12) % 12;
      let idx = deg.indexOf(rel); if (idx < 0) idx = 0;
      iv = []; const L = deg.length;
      for (let j = 0; j < def; j++) { const k = idx + j * 2, o = Math.floor(k / L); iv.push(deg[k % L] + 12 * o - deg[idx]); }
      base = b;
    } else iv = def.slice();
    const sp = this.patch.chord.spread | 0;
    if (sp >= 1 && iv.length >= 3) iv[1] += 12;
    if (sp >= 2 && iv.length >= 4) iv[3] += 12;
    return iv.map(x => clamp(base + x, 0, 127));
  }
  noteIn(n, v) {
    const P = this.patch;
    if (this.inMap.has(n)) this.noteOut(n);
    const base = P.scale.on ? this.snap(n) : n;
    const notes = P.chord.on ? this.chordOf(base) : [base];
    const toArp = !!P.arp.on;
    this.inMap.set(n, { notes: notes, arp: toArp });
    let anyActive = false; for (const x of this.voices) if (x.active && x.gate) { anyActive = true; break; }
    if (!anyActive && !this.arp.running) this.gateStart = this.time;
    for (const x of notes) {
      if (toArp) this.arp.on(x, v);
      else { this.cnt[x]++; this.susSet.delete(x); this.vOn(x, v); }
    }
  }
  noteOut(n) {
    const rec = this.inMap.get(n); if (!rec) return;
    this.inMap.delete(n);
    for (const x of rec.notes) {
      if (rec.arp) this.arp.off(x);
      else {
        this.cnt[x] = Math.max(0, this.cnt[x] - 1);
        if (this.cnt[x] === 0) { if (this.sus) this.susSet.add(x); else this.vOff(x); }
      }
    }
  }
  vOn(n, v) {
    const P = this.patch, g = P.g;
    if (g.vmode > 0) {
      const st = this.mono, i = st.indexOf(n); if (i >= 0) st.splice(i, 1); st.push(n);
      const vc = this.voices[0];
      for (let k = 1; k < this.voices.length; k++) if (this.voices[k].active && !this.voices[k].fading) this.voices[k].startFade(STEAL_FADE);
      if (vc.active && vc.gate && !vc.fading && g.vmode === 2) vc.legatoTo(n);
      else vc.start(n, v, vc.active && vc.gate);
      this.monoVel = v;
      return;
    }
    let vc = null;
    for (const x of this.voices) if (x.active && !x.fading && x.note === n) { vc = x; break; }
    if (!vc) {
      // Over the polyphony limit, the quietest released voice (else the oldest held one) fades out over STEAL_FADE.
      // The new note takes a free slot; only if the whole pool is busy does it take the voice furthest into its fade.
      let count = 0, free = null, quietR = null, old = null, faded = null, qLvl = 0;
      for (const x of this.voices) {
        if (!x.active) { if (!free) free = x; continue; }
        if (x.fading) { if (!faded || x.fadeG < faded.fadeG) faded = x; continue; }
        count++;
        if (!x.gate) { const l = x.level(); if (!quietR || l < qLvl) { quietR = x; qLvl = l; } }
        if (!old || x.age > old.age) old = x;
      }
      const poly = clamp(g.poly | 0, 1, 16);
      let victim = null;
      if (count >= poly) { victim = quietR || old; if (victim) victim.startFade(STEAL_FADE); }
      vc = free || faded || victim || quietR || old;
      if (vc && vc.active) vc.kill();
    }
    if (vc) vc.start(n, v, false);
  }
  vOff(n) {
    const g = this.patch.g;
    if (g.vmode > 0) {
      const st = this.mono, i = st.indexOf(n); if (i >= 0) st.splice(i, 1);
      const vc = this.voices[0];
      if (!vc.active || vc.fading || vc.note !== n) return;
      if (st.length) { const nn = st[st.length - 1]; if (g.vmode === 2) vc.legatoTo(nn); else vc.start(nn, this.monoVel || 0.8, true); }
      else vc.release();
      return;
    }
    for (const x of this.voices) if (x.active && x.gate && x.note === n) x.release();
  }
  tickGlobal(n) {
    const P = this.patch, sr = this.sr;
    this.gtime = this.time / sr; this.bps = this.bpm / 60;
    for (let k = 0; k < 4; k++) { const t = P.mac[k].v; this.mac[k] += (t - this.mac[k]) * 0.12; }
    this.mw += (this.mwT - this.mw) * 0.15; this.at += (this.atT - this.at) * 0.15; this.pb += (this.pbT - this.pb) * 0.3;
    this.bendSemis = this.pb * P.g.bend;
    const o = P.orb;
    this.orbT += n / sr * (0.04 + o.sp * 1.1);
    const t = this.orbT, chs = o.ch;
    const cx = chs * (0.55 * Math.sin(t * 1.3 + Math.sin(t * 0.37) * 2) + 0.35 * Math.sin(t * 2.9 + 1.1));
    const cy = chs * (0.55 * Math.cos(t * 0.91 + Math.sin(t * 0.53) * 1.7) + 0.35 * Math.sin(t * 2.3 + 0.4));
    this.ox += (clamp(o.x + cx, -1, 1) - this.ox) * 0.2; this.oy += (clamp(o.y + cy, -1, 1) - this.oy) * 0.2;
    if (P.arp.on || this.arp.play.length) this.arp.tick(this.time, n);
  }
  process(outL, outR, n) {
    if (n > this.mixL.length) { this.mixL = new Float32Array(n); this.mixR = new Float32Array(n); }
    const mL = this.mixL, mR = this.mixR;
    for (let i = 0; i < n; i++) { mL[i] = 0; mR[i] = 0; }
    for (let off = 0; off < n; off += CB) {
      const len = Math.min(CB, n - off);
      this.tickGlobal(len);
      for (let v = 0; v < POOL; v++) { const vc = this.voices[v]; if (vc.active) vc.render(off, len); }
      this.time += len;
    }
    const g = this.patch.g, gain = clamp(g.vol, 0, 2) * clamp(g.trim, 0.05, 8);
    for (let i = 0; i < n; i++) { mL[i] *= gain; mR[i] *= gain; }
    this.fx.process(this, mL, mR, n);
    for (let i = 0; i < n; i++) { let a = mL[i], b = mR[i]; if (a !== a) a = 0; if (b !== b) b = 0; outL[i] = a; outR[i] = b; }
    if (this.post) {
      this.statN += n;
      if (this.statN >= 1600) {
        this.statN = 0;
        let cnt = 0; const notes = [];
        for (const vc of this.voices) if (vc.active && !vc.fading) { cnt++; if (vc.gate) notes.push(vc.note); }
        notes.sort((a, b) => a - b);
        this.post({ t: 'st', v: cnt, ox: this.ox, oy: this.oy, step: this.arp.running ? this.arp.cur : -1, notes: notes });
      }
    }
  }
}

/* ------------------------------------------------------------------ built-in sources */
const SRC_DEFS = {
  'Choir Aah': { root: 48, len: 4, ls: 1.2, le: 3.5, p: { L: [{ on: 1, mode: 'formant', vow: 0.05, fsh: 0.5, brth: 0.12, uni: 5, det: 14, spr: 0.8, drift: 0.5, aa: 0.35, as: 1, ar: 0.4, f1t: 'lp12', f1c: 0.82 }], lfo: [{ shp: 0, rate: 5.2, rtg: 1, fade: 0.6 }], mods: [{ s: 'l1', d: 'A.pitch', a: 0.008 }] }, ev: [[0, 1, 48, 0.8], [3.7, 0, 48]] },
  'Choir Chord': { root: 48, len: 4.5, ls: 1.4, le: 4, p: { L: [{ on: 1, mode: 'formant', vow: 0.1, fsh: 0.55, brth: 0.15, uni: 3, det: 12, spr: 0.9, aa: 0.5, as: 1, ar: 0.4, f1t: 'lp12', f1c: 0.8 }], lfo: [{ shp: 0, rate: 4.8, rtg: 1, fade: 0.8 }], mods: [{ s: 'l1', d: 'A.pitch', a: 0.007 }] }, ev: [[0, 1, 48, 0.8], [0, 1, 55, 0.7], [0, 1, 60, 0.7], [0, 1, 63, 0.7], [4.2, 0, 48], [4.2, 0, 55], [4.2, 0, 60], [4.2, 0, 63]] },
  'Strings': { root: 48, len: 4, ls: 1, le: 3.6, p: { L: [{ on: 1, mode: 'analog', shp: 0.667, uni: 6, det: 12, spr: 0.9, drift: 0.4, f1t: 'lp24', f1c: 0.62, f1r: 0.05, aa: 0.4, as: 1, ar: 0.5 }], lfo: [{ shp: 0, rate: 5, rtg: 1, fade: 1 }], mods: [{ s: 'l1', d: 'A.pitch', a: 0.005 }] }, ev: [[0, 1, 48, 0.8], [3.8, 0, 48]] },
  'Felt Piano': { root: 48, len: 4.5, ls: 0, le: 0, flat: 0.8, p: { L: [{ on: 1, mode: 'modal', mdl: 'Piano', mbri: 0.42, mdec: 3.5, mhard: 0.3, minh: 0.5, as: 1, ar: 1, f1t: 'lp12', f1c: 0.75 }] }, ev: [[0, 1, 48, 0.75], [0.01, 1, 52, 0.7], [0.02, 1, 55, 0.7], [0.03, 1, 59, 0.7], [0.04, 1, 62, 0.65]] },
  'Synth Chord': { root: 48, len: 3, ls: 0.8, le: 2.6, p: { L: [{ on: 1, mode: 'analog', shp: 0.667, uni: 4, det: 22, spr: 0.9, f1t: 'lp24', f1c: 0.58, f1r: 0.15, aa: 0.01, ad: 1, as: 0.75, ar: 0.3 }] }, ev: [[0, 1, 48, 0.8], [0, 1, 51, 0.8], [0, 1, 55, 0.8], [0, 1, 58, 0.8], [0, 1, 62, 0.8], [2.8, 0, 48], [2.8, 0, 51], [2.8, 0, 55], [2.8, 0, 58], [2.8, 0, 62]] },
  'Kalimba': { root: 60, len: 4.2, ls: 0, le: 0, flat: 0.8, p: { L: [{ on: 1, mode: 'modal', mdl: 'Kalimba', mbri: 0.5, mdec: 1.4, mhard: 0.6, as: 1, ar: 1.2 }] }, ev: [[0, 1, 60, 0.8], [0.45, 1, 63, 0.7], [0.9, 1, 67, 0.75], [1.35, 1, 70, 0.7], [1.8, 1, 72, 0.8], [2.25, 1, 70, 0.6], [2.7, 1, 67, 0.7], [3.15, 1, 65, 0.7]] },
  'Glass Pad': { root: 60, len: 4, ls: 1.2, le: 3.6, p: { L: [{ on: 1, mode: 'modal', mdl: 'Glass', mbri: 0.55, mdec: 4, mhard: 0.2, mbow: 0.7, aa: 0.6, as: 1, ar: 0.6 }] }, ev: [[0, 1, 60, 0.7], [0, 1, 64, 0.7], [0, 1, 67, 0.7], [0, 1, 71, 0.7], [3.7, 0, 60], [3.7, 0, 64], [3.7, 0, 67], [3.7, 0, 71]] },
  'Vox': { root: 55, len: 3.4, ls: 0, le: 0, p: { L: [{ on: 1, mode: 'formant', vow: 0.1, fsh: 0.55, brth: 0.18, uni: 1, aa: 0.05, as: 1, ar: 0.25 }], g: { vmode: 2, glide: 0.08 }, lfo: [{ shp: 0, rate: 5.5, rtg: 1, fade: 0.3 }, { shp: 6, rate: 1.6, rtg: 1 }], mods: [{ s: 'l1', d: 'A.pitch', a: 0.01 }, { s: 'l2', d: 'A.vow', a: 0.45 }] }, ev: [[0, 1, 55, 0.8], [0.7, 1, 58, 0.8], [0.75, 0, 55], [1.5, 1, 60, 0.8], [1.55, 0, 58], [2.3, 1, 58, 0.8], [2.35, 0, 60], [3.2, 0, 58]] },
  'Bells': { root: 60, len: 4.5, ls: 0, le: 0, flat: 0.8, p: { L: [{ on: 1, mode: 'modal', mdl: 'Bell', mbri: 0.5, mdec: 4, mhard: 0.6, as: 1, ar: 2 }] }, ev: [[0, 1, 60, 0.75], [0.02, 1, 67, 0.65], [0.04, 1, 72, 0.6]] },
  'Guitar': { root: 40, len: 3.6, ls: 0, le: 0, flat: 0.8, p: { L: [{ on: 1, mode: 'pluck', pdec: 3, pdmp: 0.35, ppos: 0.18, pexc: 0.55, as: 1, ar: 0.6, f1t: 'lp12', f1c: 0.8 }] }, ev: [[0, 1, 40, 0.8], [0.02, 1, 47, 0.75], [0.04, 1, 50, 0.75], [0.06, 1, 54, 0.7], [0.08, 1, 55, 0.7], [0.1, 1, 59, 0.7]] },
  'Metal': { root: 48, len: 4.5, ls: 0, le: 0, flat: 0.8, p: { L: [{ on: 1, mode: 'modal', mdl: 'Gong', mbri: 0.6, mdec: 5, mhard: 0.7, as: 1, ar: 2 }] }, ev: [[0, 1, 48, 0.85]] },
  'Vinyl': { root: 60, len: 4, ls: 0.2, le: 3.8, p: { L: [{ on: 1, mode: 'noise', ntyp: 1, ncol: 0.4, kt: 0, spr: 0.5, f1t: 'lp12', f1c: 0.85, aa: 0.01, as: 1 }] }, ev: [[0, 1, 60, 0.8]] },
  'Rain': { root: 60, len: 4, ls: 0.2, le: 3.8, p: { L: [{ on: 1, mode: 'noise', ntyp: 2, ncol: 0.6, kt: 0, spr: 0.6, f1t: 'off', aa: 0.01, as: 1 }] }, ev: [[0, 1, 60, 0.8]] },
  'Breath': { root: 60, len: 3, ls: 0, le: 0, p: { L: [{ on: 1, mode: 'noise', ntyp: 0, ncol: 0.7, kt: 0, f1t: 'bp', f1c: 0.72, f1r: 0.3, aa: 0.9, ad: 1, as: 0.6, ar: 0.6 }] }, ev: [[0, 1, 60, 0.8], [2, 0, 60]] },
  'Organ Chord': { root: 48, len: 3, ls: 0.5, le: 2.8, p: { L: [{ on: 1, mode: 'wt', wt: 'Drawbars', pos: 0.33, crs: 12, aa: 0.005, as: 1, ar: 0.1, f1t: 'off' }] }, ev: [[0, 1, 48, 0.8], [0, 1, 55, 0.8], [0, 1, 60, 0.8], [0, 1, 63, 0.8]] }
};
const SRC_NAMES = Object.keys(SRC_DEFS);
function renderSource(name, sr) {
  const d = SRC_DEFS[name]; if (!d) return null;
  const save = _s; seed(hash01(name.length * 131 + name.charCodeAt(0)) * 1e9);
  const e = new Engine(sr);
  e.patch = normalizePatch(d.p); e.compile();
  const N = Math.round(d.len * sr), out = new Float32Array(N), L = new Float32Array(128), R = new Float32Array(128);
  const ev = d.ev.slice().sort((a, b) => a[0] - b[0]);
  let ei = 0;
  for (let off = 0; off < N; off += 128) {
    const t = off / sr;
    while (ei < ev.length && ev[ei][0] <= t) { const x = ev[ei++]; if (x[1]) e.noteIn(x[2], x[3] || 0.8); else e.noteOut(x[2]); }
    e.process(L, R, 128);
    const m = Math.min(128, N - off);
    for (let i = 0; i < m; i++) out[off + i] = (L[i] + R[i]) * 0.5;
  }
  if (d.flat) {
    const W = Math.round(0.03 * sr), env = new Float32Array(N);
    let acc = 0;
    for (let i = 0; i < N; i++) { acc += out[i] * out[i]; if (i >= W) acc -= out[i - W] * out[i - W]; env[i] = Math.sqrt(Math.max(acc, 0) / W); }
    let em = 0, sm = 0; for (let i = 0; i < N; i++) if (env[i] > em) em = env[i];
    const c = 1 - Math.exp(-1 / (0.05 * sr));
    for (let i = 0; i < N; i++) { if (env[i] > sm) sm = env[i]; else sm += (env[i] - sm) * c; const g2 = Math.min(30, Math.pow(em / Math.max(sm, em * 1e-3), d.flat)); out[i] *= g2; }
  }
  let pk = 0; for (let i = 0; i < N; i++) { const a = Math.abs(out[i]); if (a > pk) pk = a; }
  const g = pk > 1e-6 ? 0.9 / pk : 1, fi = Math.round(0.002 * sr), fo = Math.round(0.05 * sr);
  for (let i = 0; i < N; i++) { let w = g; if (i < fi) w *= i / fi; if (i > N - fo) w *= (N - i) / fo; out[i] *= w; }
  _s = save;
  return { name: name, data: out, sr: sr, root: d.root, ls: Math.round(d.ls * sr), le: Math.round(d.le * sr) };
}

return {
  Engine, buildWT, renderSource, normalizePatch, defPatch, defLayer, getPath, setPath, merge, seed, rnd, hash01, clamp, mtof, divBeats,
  WT_NAMES, MODEL_NAMES, SRC_NAMES, SRC_DEFS, MODE_NAMES, FTYPES, SRC_IDS, GLOBAL_SRCS, LDST, GDST, FXDST, DIVS, SCALES, CHORDS, ARP_MODES,
  TBL, NLEV, CB, VOWELS
};
}

module.exports = AETHER_DSP;
