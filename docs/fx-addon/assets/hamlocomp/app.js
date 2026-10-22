/* HamloComp — Soviet-style dynamics compressor (Web Audio) */
(() => {
'use strict';

/* ---------- parameter model ---------- */
const lerp = (a, b, t) => a + (b - a) * t;
const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
const lin = (min, max) => ({ to: n => lerp(min, max, n), from: v => (v - min) / (max - min) });
const log = (min, max) => ({ to: n => min * Math.pow(max / min, n), from: v => Math.log(v / min) / Math.log(max / min) });

const PARAMS = [
  { id: 'threshold', label: 'Threshold', map: lin(-60, 0),    def: -32, fmt: v => v.toFixed(1) + ' dB' },
  { id: 'ratio',     label: 'Ratio',     map: log(1, 20),     def: 4,   fmt: v => v.toFixed(1) + ':1' },
  { id: 'knee',      label: 'Knee',      map: lin(0, 40),     def: 6,   fmt: v => v.toFixed(1) + ' dB' },
  { id: 'attack',    label: 'Attack',    map: log(0.1, 100),  def: 5,   fmt: v => v.toFixed(2) + ' ms' },
  { id: 'release',   label: 'Release',   map: log(10, 1000),  def: 120, fmt: v => Math.round(v) + ' ms' },
  { id: 'makeup',    label: 'Makeup',    map: lin(0, 24),     def: 0,   fmt: v => v.toFixed(1) + ' dB' },
];
const SWITCHES = [
  { id: 'detector',  label: 'Detector',    opts: ['Peak', 'RMS'],              def: 0 },
  { id: 'character', label: 'Character',   opts: ['Clean', 'Warm', 'Tube'],    def: 0 },
  { id: 'auto',      label: 'Auto Makeup', opts: ['Off', 'On'],                def: 0 },
];

const state = { bypassed: false };
const norm = {};   // knob 0..1
const sel = {};    // switch index
const val = id => PARAMS.find(p => p.id === id).map.to(norm[id]);

/* ---------- build knobs ---------- */
const knobsEl = document.getElementById('knobs');
const knobUI = {};

function tickSVG() {
  const c = 75, n = 21;
  let s = `<svg viewBox="0 0 150 150" aria-hidden="true">`;
  for (let i = 0; i < n; i++) {
    const a = (-135 + (270 / (n - 1)) * i) * Math.PI / 180;
    const major = i % 2 === 0, r1 = 62, r2 = major ? 71 : 67;
    const x1 = c + Math.sin(a) * r1, y1 = c - Math.cos(a) * r1;
    const x2 = c + Math.sin(a) * r2, y2 = c - Math.cos(a) * r2;
    s += `<line x1="${x1.toFixed(1)}" y1="${y1.toFixed(1)}" x2="${x2.toFixed(1)}" y2="${y2.toFixed(1)}" stroke="#efe3bd" stroke-width="${major ? 2.4 : 1.4}" stroke-linecap="round"/>`;
  }
  return s + '</svg>';
}

PARAMS.forEach(p => {
  norm[p.id] = p.map.from(p.def);
  const cell = document.createElement('div');
  cell.className = 'cell';
  cell.innerHTML = `
    <span class="engr">${p.label}</span>
    <div class="knob" id="knob-${p.id}" tabindex="0" role="slider" aria-label="${p.label}" aria-valuemin="0" aria-valuemax="1">
      ${tickSVG()}
      <div class="ring"></div>
      <div class="body" style="transform:rotate(0deg)"><span class="pointer"></span></div>
      <div class="gloss"></div>
    </div>
    <div class="readout" id="read-${p.id}"></div>`;
  knobsEl.appendChild(cell);
  const knob = cell.querySelector('.knob');
  knobUI[p.id] = { knob, body: knob.querySelector('.body'), read: cell.querySelector('.readout') };

  let startY = 0, startN = 0;
  knob.addEventListener('pointerdown', e => {
    knob.setPointerCapture(e.pointerId);
    startY = e.clientY; startN = norm[p.id]; knob.classList.add('drag'); knob.focus();
  });
  knob.addEventListener('pointermove', e => {
    if (!knob.classList.contains('drag')) return;
    const k = e.shiftKey ? 900 : 200;
    setKnob(p.id, startN + (startY - e.clientY) / k * (getScale() ? 1 / getScale() : 1));
  });
  const end = () => knob.classList.remove('drag');
  knob.addEventListener('pointerup', end);
  knob.addEventListener('pointercancel', end);
  knob.addEventListener('dblclick', () => setKnob(p.id, p.map.from(p.def)));
  knob.addEventListener('wheel', e => {
    e.preventDefault();
    setKnob(p.id, norm[p.id] - Math.sign(e.deltaY) * (e.shiftKey ? 0.004 : 0.02));
  }, { passive: false });
  knob.addEventListener('keydown', e => {
    const step = e.shiftKey ? 0.005 : 0.02;
    if (e.key === 'ArrowUp' || e.key === 'ArrowRight') { setKnob(p.id, norm[p.id] + step); e.preventDefault(); }
    if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') { setKnob(p.id, norm[p.id] - step); e.preventDefault(); }
    if (e.key === 'Home') setKnob(p.id, 0);
    if (e.key === 'End') setKnob(p.id, 1);
  });
});

function setKnob(id, n) {
  norm[id] = clamp(n, 0, 1);
  const p = PARAMS.find(x => x.id === id), ui = knobUI[id];
  ui.body.style.transform = `rotate(${-135 + norm[id] * 270}deg)`;
  ui.read.textContent = p.fmt(p.map.to(norm[id]));
  ui.knob.setAttribute('aria-valuenow', norm[id].toFixed(3));
  ui.knob.setAttribute('aria-valuetext', ui.read.textContent);
  applyAudio();
}

/* ---------- build switches ---------- */
const swEl = document.getElementById('switches');
const swUI = {};
SWITCHES.forEach(s => {
  sel[s.id] = s.def;
  const n = s.opts.length, spread = n === 2 ? 34 : 38;
  const angle = i => n === 1 ? 0 : -spread + (2 * spread / (n - 1)) * i;
  const el = document.createElement('div');
  el.className = 'sw';
  el.innerHTML = `<span class="engr">${s.label}</span>
    <div class="sw-dial" id="sw-${s.id}" tabindex="0" role="radiogroup" aria-label="${s.label}">
      ${s.opts.map((o, i) => `<span class="pos" data-i="${i}" style="left:${n === 1 ? 50 : 12 + i * (76 / (n - 1))}%">${o}</span>`).join('')}
      <div class="base"></div>
      <div class="lever"><i></i><b></b></div>
    </div>`;
  swEl.appendChild(el);
  const dial = el.querySelector('.sw-dial');
  swUI[s.id] = { dial, lever: el.querySelector('.lever'), pos: [...el.querySelectorAll('.pos')], angle };
  dial.addEventListener('click', e => {
    const p = e.target.closest('.pos');
    setSwitch(s.id, p ? +p.dataset.i : (sel[s.id] + 1) % n);
  });
  dial.addEventListener('keydown', e => {
    if (['ArrowRight', 'ArrowUp'].includes(e.key)) setSwitch(s.id, Math.min(n - 1, sel[s.id] + 1));
    if (['ArrowLeft', 'ArrowDown'].includes(e.key)) setSwitch(s.id, Math.max(0, sel[s.id] - 1));
    if (e.key === ' ' || e.key === 'Enter') { setSwitch(s.id, (sel[s.id] + 1) % n); e.preventDefault(); }
  });
});
function setSwitch(id, i) {
  sel[id] = i;
  const ui = swUI[id];
  ui.lever.style.transform = `rotate(${ui.angle(i)}deg)`;
  ui.pos.forEach((p, k) => { p.classList.toggle('active', k === i); });
  applyAudio();
}

/* ---------- audio engine ---------- */
let ctx, inputBus, analyser, comp, shaper, makeup, wet, dry, master, srcNode, micStream;
let sourceKind = 'demo', fileBuffer = null, demoBuffer = null, playing = false;
let tdata;

function ensureCtx() {
  if (ctx) return;
  ctx = new (window.AudioContext || window.webkitAudioContext)();
  inputBus = ctx.createGain();
  analyser = ctx.createAnalyser(); analyser.fftSize = 1024; tdata = new Float32Array(analyser.fftSize);
  comp = ctx.createDynamicsCompressor();
  shaper = ctx.createWaveShaper(); shaper.oversample = '2x';
  makeup = ctx.createGain(); wet = ctx.createGain(); dry = ctx.createGain(); master = ctx.createGain();
  master.gain.value = 0.8;
  inputBus.connect(analyser);
  inputBus.connect(comp); comp.connect(shaper); shaper.connect(makeup); makeup.connect(wet); wet.connect(master);
  inputBus.connect(dry); dry.connect(master);
  master.connect(ctx.destination);
  applyAudio();
}

function curve(kind) {
  if (kind === 0) return null;                       // Clean
  const N = 2048, c = new Float32Array(N);
  for (let i = 0; i < N; i++) {
    const x = i / (N - 1) * 2 - 1;
    if (kind === 1) { const k = 1.8; c[i] = Math.tanh(k * x) / Math.tanh(k); }              // Warm: symmetric soft clip
    else { const k = 2.6, b = 0.18; c[i] = (Math.tanh(k * (x + b)) - Math.tanh(k * b)) / (Math.tanh(k * (1 + b)) - Math.tanh(k * b)); } // Tube: asymmetric
  }
  return c;
}
let lastChar = -1;

function applyAudio() {
  if (!ctx) return;
  const t = ctx.currentTime, T = 0.02;
  const rms = sel.detector === 1;
  const thr = val('threshold') + (rms ? 3 : 0);       // RMS detector reads ~3 dB lower than peak
  const ratio = val('ratio');
  comp.threshold.setTargetAtTime(thr, t, T);
  comp.ratio.setTargetAtTime(ratio, t, T);
  comp.knee.setTargetAtTime(val('knee'), t, T);
  comp.attack.setTargetAtTime(val('attack') / 1000, t, T);
  comp.release.setTargetAtTime(val('release') / 1000, t, T);
  const auto = sel.auto === 1 ? clamp(-thr * (1 - 1 / ratio) * 0.5, 0, 18) : 0;
  makeup.gain.setTargetAtTime(Math.pow(10, (val('makeup') + auto) / 20), t, T);
  wet.gain.setTargetAtTime(state.bypassed ? 0 : 1, t, 0.01);
  dry.gain.setTargetAtTime(state.bypassed ? 1 : 0, t, 0.01);
  if (lastChar !== sel.character) { shaper.curve = curve(sel.character); lastChar = sel.character; }
}

function makeDemo(c) {
  const sr = c.sampleRate, bpm = 110, beat = 60 / bpm, len = Math.round(sr * beat * 8);
  const buf = c.createBuffer(2, len, sr), L = buf.getChannelData(0), R = buf.getChannelData(1);
  const seed = (() => { let s = 1; return () => (s = (s * 16807) % 2147483647) / 2147483647 * 2 - 1; })();
  const bass = [55, 55, 65.41, 49, 55, 55, 73.42, 49];
  for (let i = 0; i < len; i++) {
    const t = i / sr, b = t / beat, bi = Math.floor(b), fb = (b - bi) * beat;
    // kick on every beat
    const kf = 45 + 90 * Math.exp(-fb * 28);
    const kick = Math.sin(2 * Math.PI * kf * fb) * Math.exp(-fb * 7) * 0.95;
    // snare on 2 & 4
    const sn = (bi % 2 === 1) ? (seed() * 0.45 + Math.sin(2 * Math.PI * 190 * fb) * 0.3) * Math.exp(-fb * 18) : 0;
    // hats on off-beats
    const ob = ((b + 0.5) % 1) * beat;
    const hat = seed() * 0.18 * Math.exp(-ob * 60);
    // bass line, swelling dynamically over 8 beats
    const f = bass[bi % 8], ph = (t * f) % 1;
    const bs = (ph * 2 - 1) * 0.28 * (0.55 + 0.45 * Math.sin(Math.PI * b / 8)) * Math.exp(-fb * 1.6);
    const chord = (Math.sin(2 * Math.PI * 220 * t) + Math.sin(2 * Math.PI * 277.18 * t) + Math.sin(2 * Math.PI * 329.63 * t)) * 0.035 * (0.5 + 0.5 * Math.sin(Math.PI * b / 4));
    const s = kick + sn + hat + bs;
    L[i] = s + chord; R[i] = s + chord * 0.8;
  }
  return buf;
}

function stopSource() {
  if (srcNode) { try { srcNode.stop && srcNode.stop(); } catch (e) {} try { srcNode.disconnect(); } catch (e) {} srcNode = null; }
  if (micStream) { micStream.getTracks().forEach(t => t.stop()); micStream = null; }
}

async function startPlayback() {
  ensureCtx();
  if (ctx.state === 'suspended') await ctx.resume();
  stopSource();
  if (sourceKind === 'mic') {
    try {
      micStream = await navigator.mediaDevices.getUserMedia({ audio: { echoCancellation: false, noiseSuppression: false, autoGainControl: false } });
      srcNode = ctx.createMediaStreamSource(micStream);
    } catch (e) { alert('Microphone is not available: ' + e.message); return; }
  } else {
    let buf = sourceKind === 'file' ? fileBuffer : (demoBuffer = demoBuffer || makeDemo(ctx));
    if (!buf) { document.getElementById('file-input').click(); return; }
    srcNode = ctx.createBufferSource(); srcNode.buffer = buf; srcNode.loop = true; srcNode.start();
  }
  srcNode.connect(inputBus);
  playing = true; playBtn.textContent = '■ Stop'; playBtn.classList.add('running');
}
function stopPlayback() {
  stopSource(); playing = false; playBtn.textContent = '▶ Play'; playBtn.classList.remove('running');
}

const playBtn = document.getElementById('play');
playBtn.addEventListener('click', () => playing ? stopPlayback() : startPlayback());

document.querySelectorAll('.chip[data-src]').forEach(b => b.addEventListener('click', () => {
  sourceKind = b.dataset.src;
  document.querySelectorAll('.chip[data-src]').forEach(x => x.classList.toggle('active', x === b));
  if (sourceKind === 'file' && !fileBuffer) { document.getElementById('file-input').click(); return; }
  if (playing) startPlayback();
}));
document.getElementById('file-input').addEventListener('change', async e => {
  const f = e.target.files[0]; if (!f) return;
  ensureCtx();
  try {
    fileBuffer = await ctx.decodeAudioData(await f.arrayBuffer());
    document.getElementById('src-file').textContent = f.name.length > 14 ? f.name.slice(0, 12) + '…' : f.name;
    sourceKind = 'file';
    document.querySelectorAll('.chip[data-src]').forEach(x => x.classList.toggle('active', x.dataset.src === 'file'));
    startPlayback();
  } catch (err) { alert('Could not decode this file.'); }
});

const bypassBtn = document.getElementById('bypass');
bypassBtn.addEventListener('click', () => {
  state.bypassed = !state.bypassed;
  bypassBtn.classList.toggle('on', !state.bypassed);
  bypassBtn.setAttribute('aria-pressed', String(!state.bypassed));
  applyAudio();
});

/* ---------- meters ---------- */
const DPR = Math.min(2, window.devicePixelRatio || 1);
const W = 300, H = 200, PX = W / 2, PY = H + 60, A = 0.9, R = 190;

const rngSeed = s => () => { s |= 0; s = s + 0x6D2B79F5 | 0; let t = Math.imul(s ^ s >>> 15, 1 | s); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; };

function makeMeter(canvas, cfg) {
  canvas.width = W * DPR; canvas.height = H * DPR;
  const g = canvas.getContext('2d');
  const face = document.createElement('canvas'); face.width = canvas.width; face.height = canvas.height;
  const f = face.getContext('2d'); f.scale(DPR, DPR);
  const ang = v => -A + (v - cfg.min) / (cfg.max - cfg.min) * 2 * A;
  const pt = (v, r) => [PX + Math.sin(ang(v)) * r, PY - Math.cos(ang(v)) * r];
  const rnd = rngSeed(cfg.seed || 7);

  // aged, yellowed dial paper
  const bg = f.createRadialGradient(W / 2, H * 0.5, 15, W / 2, H * 0.55, 230);
  bg.addColorStop(0, '#f1dfa0'); bg.addColorStop(0.55, '#dcc57c'); bg.addColorStop(1, '#a98b4a');
  f.fillStyle = bg; f.fillRect(0, 0, W, H);
  for (let i = 0; i < 16; i++) {   // brown water stains
    const x = rnd() * W, y = rnd() * H, r = 14 + rnd() * 50;
    const s = f.createRadialGradient(x, y, 0, x, y, r);
    s.addColorStop(0, `rgba(120,80,20,${0.05 + rnd() * 0.09})`); s.addColorStop(1, 'rgba(120,80,20,0)');
    f.fillStyle = s; f.fillRect(x - r, y - r, r * 2, r * 2);
  }
  for (let i = 0; i < 160; i++) {   // dust and foxing specks
    f.fillStyle = `rgba(70,45,15,${0.12 + rnd() * 0.3})`;
    f.fillRect(rnd() * W, rnd() * H, 0.8 + rnd() * 1.2, 0.8 + rnd() * 1.2);
  }
  // arc
  f.lineCap = 'butt';
  f.strokeStyle = '#1b1710'; f.lineWidth = 3;
  f.beginPath(); f.arc(PX, PY, R, -Math.PI / 2 + ang(cfg.min), -Math.PI / 2 + ang(cfg.redFrom)); f.stroke();
  f.strokeStyle = '#a82820'; f.lineWidth = 6;
  f.beginPath(); f.arc(PX, PY, R - 1, -Math.PI / 2 + ang(cfg.redFrom), -Math.PI / 2 + ang(cfg.max)); f.stroke();
  f.strokeStyle = '#1b1710'; f.lineWidth = 1.6;
  f.beginPath(); f.arc(PX, PY, R - 12, -Math.PI / 2 + ang(cfg.min), -Math.PI / 2 + ang(cfg.max)); f.stroke();
  // ticks
  f.font = '600 15px Oswald, sans-serif'; f.textAlign = 'center'; f.textBaseline = 'middle';
  cfg.ticks.forEach(t => {
    const red = t.v >= cfg.redFrom;
    f.strokeStyle = red ? '#a82820' : '#1b1710'; f.lineWidth = t.label ? 2.4 : 1.4;
    const [x1, y1] = pt(t.v, R - 12), [x2, y2] = pt(t.v, R + (t.label ? 10 : 5));
    f.beginPath(); f.moveTo(x1, y1); f.lineTo(x2, y2); f.stroke();
    if (t.label) { const [lx, ly] = pt(t.v, R - 28); f.fillStyle = red ? '#a82820' : '#1b1710'; f.fillText(t.label, lx, ly); }
  });
  // captions
  f.fillStyle = '#1b1710'; f.textAlign = 'center';
  f.font = `700 ${cfg.titleSize || 21}px Oswald, sans-serif`; f.fillText(cfg.title, W / 2, 124);
  f.font = '500 14px Oswald, sans-serif'; f.fillText('dB', W / 2, 144);

  // glass layer: scratches, glare, edge shadow (drawn above the needle)
  const glass = document.createElement('canvas'); glass.width = canvas.width; glass.height = canvas.height;
  const gl = glass.getContext('2d'); gl.scale(DPR, DPR);
  for (let i = 0; i < 26; i++) {
    const x = rnd() * W, y = rnd() * H, a = rnd() * Math.PI, l = 8 + rnd() * (i < 4 ? 130 : 38);
    gl.strokeStyle = `rgba(255,255,255,${0.1 + rnd() * 0.28})`; gl.lineWidth = 0.5 + rnd() * 0.5;
    gl.beginPath(); gl.moveTo(x, y); gl.lineTo(x + Math.cos(a) * l, y + Math.sin(a) * l); gl.stroke();
  }
  const gg = gl.createLinearGradient(0, 0, W, H);
  gg.addColorStop(0, '#ffffff40'); gg.addColorStop(0.4, '#ffffff12'); gg.addColorStop(0.41, '#ffffff00');
  gl.fillStyle = gg; gl.fillRect(0, 0, W, H);
  const vg = gl.createRadialGradient(W / 2, H / 2, 80, W / 2, H / 2, 215);
  vg.addColorStop(0, '#0000'); vg.addColorStop(1, '#2a1a0a66');
  gl.fillStyle = vg; gl.fillRect(0, 0, W, H);

  let cur = cfg.rest;
  function draw(target) {
    // ballistics + tiny analog tremor
    const up = cfg.rise(target, cur);
    cur += (target - cur) * up;
    const now = performance.now();
    const tremor = (Math.sin(now / 83) + Math.sin(now / 31) * 0.6) * 0.03 * (cfg.max - cfg.min) / 46;
    g.setTransform(1, 0, 0, 1, 0, 0);
    g.clearRect(0, 0, canvas.width, canvas.height);
    g.drawImage(face, 0, 0);
    g.scale(DPR, DPR);
    const a = ang(clamp(cur + tremor, cfg.min - 2, cfg.max + 1.5));
    const [tx, ty] = [PX + Math.sin(a) * (R + 6), PY - Math.cos(a) * (R + 6)];
    // needle shadow
    g.strokeStyle = '#2a1a0a55'; g.lineWidth = 3; g.lineCap = 'round';
    g.beginPath(); g.moveTo(PX + 4, PY + 4); g.lineTo(tx + 4, ty + 5); g.stroke();
    g.strokeStyle = '#0d0b08'; g.lineWidth = 2.2;
    g.beginPath(); g.moveTo(PX, PY); g.lineTo(tx, ty); g.stroke();
    // bottom plate hides pivot
    const pl = g.createLinearGradient(0, H - 30, 0, H);
    pl.addColorStop(0, '#2d2a22'); pl.addColorStop(1, '#0c0b09');
    g.fillStyle = pl;
    g.beginPath(); g.moveTo(W / 2 - 62, H); g.quadraticCurveTo(W / 2, H - 46, W / 2 + 62, H); g.closePath(); g.fill();
    g.fillStyle = '#b79b4a'; g.beginPath(); g.arc(W / 2, H - 9, 6, 0, 7); g.fill();
    g.fillStyle = '#6a5520'; g.fillRect(W / 2 - 4, H - 10, 8, 2);
    // scratched glass on top
    g.drawImage(glass, 0, 0, W, H);
  }
  draw(cfg.rest);
  return { draw };
}

const mkTicks = (list, majors) => list.map(v => ({ v, label: majors.includes(v) ? (v > 0 ? '+' + v : String(v)) : null }));
let inMeter, grMeter;
function buildMeters() {
  inMeter = makeMeter(document.getElementById('m-in'), {
    min: -40, max: 6, redFrom: 0, title: 'INPUT', seed: 11, rest: -42,
    ticks: mkTicks([-40, -35, -30, -25, -20, -15, -10, -8, -6, -4, -2, 0, 2, 4, 6], [-40, -30, -20, -10, 0, 6]),
    rise: (t, c) => t > c ? 0.55 : 0.09,
  });
  grMeter = makeMeter(document.getElementById('m-gr'), {
    min: -20, max: 0, redFrom: -6, title: 'GAIN REDUCTION', titleSize: 17, seed: 23, rest: 0,
    ticks: mkTicks([-20, -18, -16, -14, -12, -10, -8, -6, -5, -4, -3, -2, -1, 0], [-20, -10, -5, -2, 0]),
    rise: (t, c) => t < c ? 0.45 : 0.1,
  });
}
buildMeters();
if (document.fonts && document.fonts.ready) document.fonts.ready.then(buildMeters);

const grReadout = document.getElementById('gr-readout');
let grShown = 0, lastText = 0;
function loop(now) {
  let inDb = -60, gr = 0;
  if (ctx && playing) {
    analyser.getFloatTimeDomainData(tdata);
    if (sel.detector === 1) {
      let s = 0; for (let i = 0; i < tdata.length; i++) s += tdata[i] * tdata[i];
      inDb = 20 * Math.log10(Math.sqrt(s / tdata.length) + 1e-6) + 3;
    } else {
      let p = 0; for (let i = 0; i < tdata.length; i++) p = Math.max(p, Math.abs(tdata[i]));
      inDb = 20 * Math.log10(p + 1e-6);
    }
    gr = state.bypassed ? 0 : Math.min(0, comp.reduction);
  }
  inMeter.draw(inDb);
  grMeter.draw(gr);
  grShown += (gr - grShown) * 0.2;
  if (now - lastText > 80) { grReadout.textContent = 'GR ' + Math.abs(grShown).toFixed(1) + ' dB'; lastText = now; }
  requestAnimationFrame(loop);
}

/* ---------- worn steel overlay ---------- */
function drawWear() {
  const cv = document.getElementById('wear');
  const w = 1148, h = 668;
  cv.width = w * DPR; cv.height = h * DPR;
  const c = cv.getContext('2d'); c.scale(DPR, DPR);
  const r = rngSeed(2024);
  const edgeBias = () => { const e = r(); return e < 0.5 ? r() : (r() < 0.5 ? r() * 0.12 : 1 - r() * 0.12); };
  // oily smudges
  for (let i = 0; i < 14; i++) {
    const x = r() * w, y = r() * h, rad = 40 + r() * 140;
    const gr = c.createRadialGradient(x, y, 0, x, y, rad);
    const lite = r() < 0.5;
    gr.addColorStop(0, lite ? 'rgba(200,190,160,0.045)' : 'rgba(0,0,0,0.22)'); gr.addColorStop(1, 'rgba(0,0,0,0)');
    c.fillStyle = gr; c.fillRect(x - rad, y - rad, rad * 2, rad * 2);
  }
  // rust blooms, mostly near edges
  for (let i = 0; i < 46; i++) {
    const nearX = r() < 0.5;
    const x = nearX ? edgeBias() * w : r() * w, y = nearX ? r() * h : edgeBias() * h, rad = 4 + r() * 34;
    const gr = c.createRadialGradient(x, y, 0, x, y, rad);
    gr.addColorStop(0, `rgba(${105 + r() * 30 | 0},${48 + r() * 16 | 0},18,${0.12 + r() * 0.2})`);
    gr.addColorStop(0.6, 'rgba(90,40,14,0.12)'); gr.addColorStop(1, 'rgba(90,40,14,0)');
    c.fillStyle = gr; c.fillRect(x - rad, y - rad, rad * 2, rad * 2);
  }
  // chipped paint showing bare steel along the edges
  for (let i = 0; i < 30; i++) {
    const side = r() * 4 | 0, t = r();
    const x = side === 0 ? t * w : side === 1 ? w - r() * 8 : side === 2 ? t * w : r() * 8;
    const y = side === 0 ? r() * 8 : side === 1 ? t * h : side === 2 ? h - r() * 8 : t * h;
    const s = 1.5 + r() * 5;
    c.beginPath();
    for (let k = 0; k < 7; k++) {
      const a = k / 7 * Math.PI * 2, rr = s * (0.5 + r() * 0.7);
      c[k ? 'lineTo' : 'moveTo'](x + Math.cos(a) * rr, y + Math.sin(a) * rr * 0.7);
    }
    c.closePath();
    c.fillStyle = r() < 0.55 ? 'rgba(120,118,110,0.5)' : 'rgba(95,46,18,0.6)'; c.fill();
    c.strokeStyle = 'rgba(0,0,0,0.35)'; c.lineWidth = 0.6; c.stroke();
  }
  // scratches
  for (let i = 0; i < 380; i++) {
    const x = r() * w, y = r() * h, a = r() * Math.PI * 2;
    const l = i < 24 ? 120 + r() * 380 : 6 + r() * 70;
    c.strokeStyle = `rgba(${200 + r() * 40 | 0},${195 + r() * 40 | 0},${170 + r() * 30 | 0},${0.05 + r() * (i < 24 ? 0.16 : 0.26)})`;
    c.lineWidth = 0.4 + r() * 0.7;
    c.beginPath(); c.moveTo(x, y);
    c.quadraticCurveTo(x + Math.cos(a) * l * 0.5 + (r() - 0.5) * 6, y + Math.sin(a) * l * 0.5 + (r() - 0.5) * 6, x + Math.cos(a) * l, y + Math.sin(a) * l);
    c.stroke();
  }
  // pitting & dust
  for (let i = 0; i < 900; i++) {
    c.fillStyle = r() < 0.5 ? `rgba(0,0,0,${0.2 + r() * 0.4})` : `rgba(210,200,170,${0.05 + r() * 0.2})`;
    c.fillRect(r() * w, r() * h, 0.7 + r() * 1.6, 0.7 + r() * 1.6);
  }
  // edge darkening
  const v = c.createRadialGradient(w / 2, h / 2, h * 0.35, w / 2, h / 2, w * 0.62);
  v.addColorStop(0, 'rgba(0,0,0,0)'); v.addColorStop(1, 'rgba(0,0,0,0.38)');
  c.fillStyle = v; c.fillRect(0, 0, w, h);
}
drawWear();

/* ---------- fit to viewport ---------- */
let scale = 1;
const getScale = () => scale;
function fit() {
  scale = Math.min(window.innerWidth / 1220, window.innerHeight / 740, 1.6);
  document.getElementById('stage').style.transform = `scale(${scale})`;
}
window.addEventListener('resize', fit); fit();

/* ---------- init ---------- */
PARAMS.forEach(p => setKnob(p.id, norm[p.id]));
SWITCHES.forEach(s => setSwitch(s.id, s.def));
requestAnimationFrame(loop);
})();
