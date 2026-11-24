/* FRACTALSCREEN cosmic focus music, generated live in the browser.
   Deep, slow and spacious: warm pads drifting through four chords, a soft sub drone,
   glassy "stars", solar wind and, now and then, a gentle pulse. It never loops.
   There is no volume control of its own: it follows the device's volume. */
class Music {
  constructor() { this.ctx = null; this.playing = false; this.timers = []; this.live = new Set(); }
  ensure() {
    const AC = window.AudioContext || window.webkitAudioContext; if (!AC) return false;
    if (!this.ctx) { this.ctx = new AC(); this.build(); }
    if (this.ctx.state === "suspended") this.ctx.resume().catch(() => {});
    return true;
  }
  get unlocked() { return !!this.ctx && this.ctx.state === "running"; }
  setVolume() {}   // kept for compatibility: the device's own volume controls the music

  build() {
    const c = this.ctx;
    this.master = c.createGain(); this.master.gain.value = 0;
    // Gentle glue compression, then a soft limiter so chord changes never clip
    const comp = c.createDynamicsCompressor(); comp.threshold.value = -26; comp.knee.value = 12; comp.ratio.value = 4; comp.attack.value = .03; comp.release.value = .5;
    const soft = c.createWaveShaper(), curve = new Float32Array(2048);
    for (let i = 0; i < curve.length; i++) { const x = i / (curve.length - 1) * 2 - 1; curve[i] = Math.tanh(1.4 * x) / Math.tanh(1.4); }
    soft.curve = curve; soft.oversample = "2x";
    this.out = c.createGain(); this.out.gain.value = .65;
    this.master.connect(comp); comp.connect(soft); soft.connect(this.out); this.out.connect(c.destination);

    // Long, dark hall reverb from a generated impulse
    const sec = 7, len = Math.floor(c.sampleRate * sec), ir = c.createBuffer(2, len, c.sampleRate);
    for (let ch = 0; ch < 2; ch++) {
      const d = ir.getChannelData(ch); let lp = 0;
      for (let i = 0; i < len; i++) { lp += .35 * ((Math.random() * 2 - 1) - lp); d[i] = lp * Math.pow(1 - i / len, 3.2) * (i < 400 ? i / 400 : 1); }
    }
    this.verb = c.createConvolver(); this.verb.buffer = ir;
    this.verbIn = c.createGain(); this.verbIn.gain.value = .9;
    const verbOut = c.createGain(); verbOut.gain.value = .75;
    this.verbIn.connect(this.verb); this.verb.connect(verbOut); verbOut.connect(this.master);

    // Wide ping-pong echo
    this.echoIn = c.createGain(); this.echoIn.gain.value = .5;
    const dl = c.createDelay(2), dr = c.createDelay(2), fb = c.createGain(), lp = c.createBiquadFilter(), merge = c.createChannelMerger(2);
    dl.delayTime.value = .48; dr.delayTime.value = .72; fb.gain.value = .38; lp.type = "lowpass"; lp.frequency.value = 2600;
    this.echoIn.connect(dl); dl.connect(dr); dr.connect(lp); lp.connect(fb); fb.connect(dl);
    dl.connect(merge, 0, 0); dr.connect(merge, 0, 1); merge.connect(this.master); merge.connect(this.verbIn);

    // Shared slow filter movement for the pads
    this.lfo = c.createOscillator(); this.lfo.frequency.value = .025;
    this.lfoAmt = c.createGain(); this.lfoAmt.gain.value = 420; this.lfo.connect(this.lfoAmt); this.lfo.start();

    this.padBus = c.createGain(); this.padBus.gain.value = 1; this.padBus.connect(this.master); this.padBus.connect(this.verbIn);

    const nl = c.sampleRate * 4; this.noise = c.createBuffer(1, nl, c.sampleRate);
    const nd = this.noise.getChannelData(0); for (let i = 0; i < nl; i++) nd[i] = Math.random() * 2 - 1;

    // D minor / Dorian: Dm9, Bbmaj9(#11), Fmaj9, Gm9
    this.chords = [
      { bass: 38, pad: [53, 57, 60, 64, 69] },
      { bass: 34, pad: [50, 53, 57, 60, 64] },
      { bass: 41, pad: [53, 57, 60, 64, 67] },
      { bass: 43, pad: [53, 55, 58, 62, 69] }
    ];
    this.stars = [62, 64, 65, 69, 72, 74, 76, 77, 81, 84, 86];
    this.pulse = [0, 2, 1, 3, 2, 4, 3, 1];
  }
  hz(m) { return 440 * Math.pow(2, (m - 69) / 12); }
  track(node, stopAt) { this.live.add(node); node.onended = () => this.live.delete(node); node.start(); node.stop(stopAt); }
  later(fn, ms) { const id = setTimeout(() => { this.timers = this.timers.filter(t => t !== id); if (this.playing) fn(); }, ms); this.timers.push(id); }

  start() {
    if (!this.ensure() || this.playing) return;
    const c = this.ctx, t = c.currentTime; this.playing = true; this.ci = 0; this.pulseOn = false; this.step = 0;
    this.master.gain.cancelScheduledValues(t); this.master.gain.setValueAtTime(this.master.gain.value, t);
    this.master.gain.linearRampToValueAtTime(.6, t + 6);
    this.chord(); this.wind(); this.star(); this.later(() => this.togglePulse(), 50000);
  }
  stop() {
    if (!this.ctx || !this.playing) return; this.playing = false;
    const c = this.ctx, t = c.currentTime;
    this.timers.forEach(clearTimeout); this.timers = [];
    this.master.gain.cancelScheduledValues(t); this.master.gain.setValueAtTime(this.master.gain.value, t);
    this.master.gain.linearRampToValueAtTime(0, t + 1.5);
    setTimeout(() => { if (!this.playing) { this.live.forEach(n => { try { n.stop(); } catch (e) {} }); this.live.clear(); } }, 1700);
  }

  // Pads and sub drone: one chord every 18 s, with long overlapping fades
  chord() {
    const c = this.ctx, ch = this.chords[this.ci % this.chords.length], t = c.currentTime + .05, dur = 18, att = 5, rel = 7;
    ch.pad.forEach((m, i) => {
      const f = c.createBiquadFilter(); f.type = "lowpass"; f.frequency.value = 700 + i * 90; f.Q.value = .5; this.lfoAmt.connect(f.frequency);
      const g = c.createGain(), pan = c.createStereoPanner ? c.createStereoPanner() : null, peak = .028;
      g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(peak, t + att);
      g.gain.setValueAtTime(peak, t + dur); g.gain.linearRampToValueAtTime(0, t + dur + rel);
      f.connect(g);
      if (pan) { pan.pan.value = (i / (ch.pad.length - 1)) * 1.2 - .6; g.connect(pan); pan.connect(this.padBus); } else g.connect(this.padBus);
      [-9, 9].forEach(cents => { const o = c.createOscillator(); o.type = "sawtooth"; o.frequency.value = this.hz(m); o.detune.value = cents + (Math.random() * 4 - 2); o.connect(f); this.track(o, t + dur + rel + .1); });
      const s = c.createOscillator(); s.type = "sine"; s.frequency.value = this.hz(m + 12); const sg = c.createGain(); sg.gain.value = .35; s.connect(sg); sg.connect(g); this.track(s, t + dur + rel + .1);
      setTimeout(() => { try { this.lfoAmt.disconnect(f.frequency); } catch (e) {} }, (dur + rel + 1) * 1000);
    });
    const sub = c.createOscillator(), sg = c.createGain(); sub.type = "sine"; sub.frequency.value = this.hz(ch.bass + 12);
    sg.gain.setValueAtTime(0, t); sg.gain.linearRampToValueAtTime(.11, t + att); sg.gain.setValueAtTime(.11, t + dur); sg.gain.linearRampToValueAtTime(0, t + dur + rel);
    sub.connect(sg); sg.connect(this.master); this.track(sub, t + dur + rel + .1);
    this.ci++;
    this.later(() => this.chord(), dur * 1000);
  }

  // Glassy stars: soft FM bells high up, mostly heard through the reverb and echo
  star() {
    const c = this.ctx, t = c.currentTime + .02, m = this.stars[Math.floor(Math.random() * this.stars.length)], f = this.hz(m);
    const car = c.createOscillator(), mod = c.createOscillator(), mg = c.createGain(), g = c.createGain();
    car.frequency.value = f; mod.frequency.value = f * (Math.random() < .5 ? 2.0 : 3.5); mod.connect(mg); mg.connect(car.frequency);
    mg.gain.setValueAtTime(f * 1.4, t); mg.gain.exponentialRampToValueAtTime(f * .02, t + 2.5);
    const vel = .025 + Math.random() * .03, decay = 3 + Math.random() * 3;
    g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(vel, t + .015); g.gain.exponentialRampToValueAtTime(.0003, t + decay);
    car.connect(g);
    let out = g; if (c.createStereoPanner) { const p = c.createStereoPanner(); p.pan.value = Math.random() * 1.6 - .8; g.connect(p); out = p; }
    out.connect(this.master); out.connect(this.verbIn); out.connect(this.echoIn);
    this.track(car, t + decay + .1); this.track(mod, t + decay + .1);
    this.later(() => this.star(), 1400 + Math.random() * 3600);
  }

  // Solar wind: filtered noise slowly sweeping across the stereo field
  wind() {
    const c = this.ctx, src = c.createBufferSource(), bp = c.createBiquadFilter(), g = c.createGain();
    src.buffer = this.noise; src.loop = true; bp.type = "bandpass"; bp.Q.value = 5; bp.frequency.value = 600; g.gain.value = .045;
    src.connect(bp); bp.connect(g);
    let out = g; if (c.createStereoPanner) { this.windPan = c.createStereoPanner(); g.connect(this.windPan); out = this.windPan; }
    out.connect(this.master); out.connect(this.verbIn);
    this.live.add(src); src.start();
    const drift = () => {
      const t = c.currentTime;
      bp.frequency.setTargetAtTime(250 + Math.random() * 1600, t, 4);
      g.gain.setTargetAtTime(.02 + Math.random() * .04, t, 5);
      if (this.windPan) this.windPan.pan.setTargetAtTime(Math.random() * 1.4 - .7, t, 6);
      this.later(drift, 7000 + Math.random() * 5000);
    };
    drift();
  }

  // A gentle pulse that comes and goes every couple of minutes
  togglePulse() {
    this.pulseOn = !this.pulseOn;
    if (this.pulseOn) this.pulseStep();
    this.later(() => this.togglePulse(), (this.pulseOn ? 60000 : 80000) + Math.random() * 40000);
  }
  pulseStep() {
    if (!this.pulseOn) return;
    const c = this.ctx, t = c.currentTime + .02, ch = this.chords[(this.ci + this.chords.length - 1) % this.chords.length];
    const m = ch.pad[this.pulse[this.step % this.pulse.length]] + 12, o = c.createOscillator(), f = c.createBiquadFilter(), g = c.createGain();
    o.type = "triangle"; o.frequency.value = this.hz(m); f.type = "lowpass"; f.frequency.value = 1400;
    g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(.032, t + .01); g.gain.exponentialRampToValueAtTime(.0004, t + .6);
    o.connect(f); f.connect(g); g.connect(this.master); g.connect(this.echoIn); g.connect(this.verbIn);
    this.track(o, t + .7); this.step++;
    this.later(() => this.pulseStep(), 441);   // 68 BPM eighth notes
  }
}
