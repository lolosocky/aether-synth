// Loudness trims for the Modern Hip Hop category, calibrated to the existing library's levels.
const fs = require('fs');
const D = require('../web/src/dsp.js')(); const T = require('../web/src/trims.json');
const L = require('../web/src/presets.js')(D, {});
const SR = 48000;
function meas(p, notes, win, offAt) {
  const e = new D.Engine(SR); e.msg({ t: 'patch', p }); D.seed(7);
  for (const n of notes) e.msg({ t: 'on', n, v: 0.8 });
  const N = Math.round(win * SR), Lb = new Float32Array(128), Rb = new Float32Array(128); let s = 0, c = 0, pk = 0, nan = 0, off = false;
  for (let o = 0; o < N; o += 128) {
    if (!off && offAt && o >= offAt * SR) { for (const n of notes) e.msg({ t: 'off', n }); off = true; }
    e.process(Lb, Rb, 128);
    for (let i = 0; i < 128; i++) { if (!isFinite(Lb[i]) || !isFinite(Rb[i])) nan++; s += Lb[i] * Lb[i] + Rb[i] * Rb[i]; c++; pk = Math.max(pk, Math.abs(Lb[i]), Math.abs(Rb[i])); }
  }
  return { rms: Math.sqrt(s / c / 2), pk, nan };
}
const fam = p => (p.tags || []).join(' ');
const out = {}; const rows = [];
for (const p of L.list.filter(x => x.cat === 'Modern Hip Hop')) {
  p.g.trim = 1; const t = fam(p); let trim, m;
  if (/drums/.test(t)) { m = meas(p, [60], 0.6, 0.4); const target = /kick/.test(t) ? 0.85 : /cymbal/.test(t) ? 0.55 : 0.7; trim = target / Math.max(m.pk, 1e-6); }
  else if (/808/.test(t)) { m = meas(p, [36], 0.5); trim = 0.128 / m.rms; }
  else if (/fx/.test(t)) { m = meas(p, [60], 3); trim = 0.08 / m.rms; }
  else if (/ambient/.test(t)) { m = meas(p, [48, 51, 55, 58], 2); trim = 0.07 / m.rms; }
  else if (/bell/.test(t)) { m = meas(p, [60, 67], 0.5); trim = 0.10 / m.rms; }
  else { m = meas(p, [60], 0.5); trim = 0.125 / m.rms; }
  trim = Math.min(8, Math.max(0.05, trim));
  // never let a single hit run hot
  if (m.pk * trim > 0.95) trim = 0.95 / m.pk;
  out[p.name] = +trim.toFixed(3);
  rows.push([p.name, t, m.rms.toFixed(4), m.pk.toFixed(3), trim.toFixed(3), m.nan]);
}
for (const r of rows) console.log(r.join(' | '));
const all = Object.assign({}, T, out);
fs.writeFileSync(__dirname + '/../web/src/trims.json', JSON.stringify(all));
console.log('trims', Object.keys(all).length);
