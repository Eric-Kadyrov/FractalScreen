/* FRACTALSCREEN cosmic focus music for Windows.
 * Generated live (no audio files): warm pads drifting through Dm9, Bbmaj9, Fmaj9 and Gm9,
 * a soft sub drone, glassy FM "stars", filtered solar wind and, now and then, a gentle pulse,
 * all in a long hall reverb and a wide ping-pong echo. The same design as the Web Player's music.js.
 * Plays through waveOut, so the Windows volume (and the app's slider in the volume mixer) controls it.
 *
 *   music_start();   // fades in over 6 seconds
 *   music_stop();    // quick fade out, then closes the audio device
 */
#ifndef FRACTALSCREEN_MUSIC_H
#define FRACTALSCREEN_MUSIC_H

#include <windows.h>
#include <mmsystem.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define MU_SR      44100
#define MU_FRAMES  4096            /* ~93 ms per buffer */
#define MU_NBUF    4
#define MU_TAU     6.28318530718f
#define MU_MAXPAD  20
#define MU_MAXBELL 16
#define MU_MAXPUL  6

typedef struct { int on; float f, ph[3], det[2], amp, peak; long age, att, hold, rel; float gl, gr, cut, ic1, ic2; int sine; } MuPad;
typedef struct { int on; float f, pc, pm, ratio, idx0, vel, decay; long age; float gl, gr; } MuBell;
typedef struct { int on; float f, ph, lp; long age; } MuPulse;
typedef struct { float *b; int n, w; } MuLine;

static struct {
  HWAVEOUT wo; WAVEHDR hdr[MU_NBUF]; short buf[MU_NBUF][MU_FRAMES * 2];
  HANDLE evt, thread; volatile LONG run, fadeOut;
  unsigned rng; long long n;                 /* random state, sample counter */
  MuPad pad[MU_MAXPAD]; MuBell bell[MU_MAXBELL]; MuPulse pul[MU_MAXPUL];
  int chord; long long nextChord, nextStar, nextWind, nextPulseToggle, nextPulse; int pulseOn, step;
  float windF, windFt, windG, windGt, windP, windPt, wic1, wic2;
  MuLine fdn[8]; float damp[8], fdg[8]; MuLine ap[2]; MuLine echoL, echoR; float echoLp;
  float master, env;                          /* fade gain, limiter envelope */
} mu;

static const int MU_CH_BASS[4] = { 38, 34, 41, 43 };
static const int MU_CH_PAD[4][5] = { {53,57,60,64,69}, {50,53,57,60,64}, {53,57,60,64,67}, {53,55,58,62,69} };
static const int MU_STARS[11] = { 62,64,65,69,72,74,76,77,81,84,86 };
static const int MU_PULSE[8] = { 0,2,1,3,2,4,3,1 };

static float mu_rand(void) { mu.rng ^= mu.rng << 13; mu.rng ^= mu.rng >> 17; mu.rng ^= mu.rng << 5; return (mu.rng & 0xFFFFFF) / 16777216.0f; }
static float mu_hz(float m) { return 440.0f * powf(2.0f, (m - 69.0f) / 12.0f); }
static void mu_pan(float p, float *l, float *r) { float a = (p + 1.0f) * 0.785398f; *l = cosf(a); *r = sinf(a); }
static int mu_line(MuLine *L, int n) { L->b = (float*)calloc(n, sizeof(float)); L->n = n; L->w = 0; return L->b != NULL; }
static float mu_read(MuLine *L, int d) { int i = L->w - d; while (i < 0) i += L->n; return L->b[i]; }
static void mu_write(MuLine *L, float x) { L->b[L->w] = x; if (++L->w >= L->n) L->w = 0; }
static float mu_blep(float t, float dt) {
  if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
  if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
  return 0.0f;
}

static void mu_add_pad(float midi, float peak, float pan, float cut, long att, long hold, long rel, int sine) {
  for (int i = 0; i < MU_MAXPAD; i++) if (!mu.pad[i].on) {
    MuPad *v = &mu.pad[i]; memset(v, 0, sizeof(*v));
    v->on = 1; v->f = mu_hz(midi); v->peak = peak; v->att = att; v->hold = hold; v->rel = rel; v->cut = cut; v->sine = sine;
    v->det[0] = powf(2.0f, (-9.0f + mu_rand() * 4 - 2) / 1200.0f); v->det[1] = powf(2.0f, (9.0f + mu_rand() * 4 - 2) / 1200.0f);
    v->ph[0] = mu_rand(); v->ph[1] = mu_rand(); v->ph[2] = mu_rand();
    mu_pan(pan, &v->gl, &v->gr); return;
  }
}
static void mu_new_chord(void) {
  int c = mu.chord % 4; long att = 5 * MU_SR, hold = 18 * MU_SR, rel = 7 * MU_SR;
  for (int i = 0; i < 5; i++) mu_add_pad((float)MU_CH_PAD[c][i], 0.028f, (i / 4.0f) * 1.2f - 0.6f, 700.0f + i * 90.0f, att, hold, rel, 0);
  mu_add_pad((float)(MU_CH_BASS[c] + 12), 0.11f, 0.0f, 0, att, hold, rel, 1);
  mu.chord++;
}
static void mu_new_star(void) {
  for (int i = 0; i < MU_MAXBELL; i++) if (!mu.bell[i].on) {
    MuBell *b = &mu.bell[i]; memset(b, 0, sizeof(*b));
    b->on = 1; b->f = mu_hz((float)MU_STARS[(int)(mu_rand() * 11) % 11]);
    b->ratio = mu_rand() < 0.5f ? 2.0f : 3.5f; b->idx0 = b->ratio == 2.0f ? 0.7f : 0.4f;
    b->vel = 0.025f + mu_rand() * 0.03f; b->decay = 3.0f + mu_rand() * 3.0f;
    mu_pan(mu_rand() * 1.6f - 0.8f, &b->gl, &b->gr); return;
  }
}
static void mu_new_pulse(void) {
  int c = (mu.chord + 3) % 4;
  for (int i = 0; i < MU_MAXPUL; i++) if (!mu.pul[i].on) {
    MuPulse *p = &mu.pul[i]; memset(p, 0, sizeof(*p));
    p->on = 1; p->f = mu_hz((float)(MU_CH_PAD[c][MU_PULSE[mu.step % 8]] + 12)); mu.step++; return;
  }
}

/* SVF (topology-preserving) one step; returns low-pass, sets band-pass */
static float mu_svf(float x, float g, float k, float *ic1, float *ic2, float *band) {
  float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
  float v3 = x - *ic2, v1 = a1 * *ic1 + a2 * v3, v2 = *ic2 + a2 * *ic1 + a3 * v3;
  *ic1 = 2.0f * v1 - *ic1; *ic2 = 2.0f * v2 - *ic2; if (band) *band = v1; return v2;
}

static void mu_render(short *out, int frames) {
  const float inv = 1.0f / MU_SR;
  for (int s = 0; s < frames; s++, mu.n++) {
    /* --- scheduler --- */
    if (mu.n >= mu.nextChord) { mu_new_chord(); mu.nextChord = mu.n + 18LL * MU_SR; }
    if (mu.n >= mu.nextStar) { mu_new_star(); mu.nextStar = mu.n + (long long)((1.4f + mu_rand() * 3.6f) * MU_SR); }
    if (mu.n >= mu.nextWind) { mu.windFt = 250 + mu_rand() * 1600; mu.windGt = 0.02f + mu_rand() * 0.04f; mu.windPt = mu_rand() * 1.4f - 0.7f;
                               mu.nextWind = mu.n + (long long)((7 + mu_rand() * 5) * MU_SR); }
    if (mu.n >= mu.nextPulseToggle) { mu.pulseOn = !mu.pulseOn; mu.nextPulse = mu.n;
                               mu.nextPulseToggle = mu.n + (long long)(((mu.pulseOn ? 60 : 80) + mu_rand() * 40) * MU_SR); }
    if (mu.pulseOn && mu.n >= mu.nextPulse) { mu_new_pulse(); mu.nextPulse = mu.n + (long long)(0.441f * MU_SR); }

    float L = 0, R = 0, verb = 0, echo = 0;
    float lfo = sinf(MU_TAU * 0.025f * (float)mu.n * inv) * 420.0f;

    /* --- pads and sub drone --- */
    for (int i = 0; i < MU_MAXPAD; i++) {
      MuPad *v = &mu.pad[i]; if (!v->on) continue;
      float e;
      if (v->age < v->att) e = (float)v->age / v->att;
      else if (v->age < v->hold) e = 1.0f;
      else if (v->age < v->hold + v->rel) e = 1.0f - (float)(v->age - v->hold) / v->rel;
      else { v->on = 0; continue; }
      v->age++;
      float x;
      if (v->sine) {
        x = sinf(MU_TAU * v->ph[0]); v->ph[0] += v->f * inv; if (v->ph[0] >= 1) v->ph[0] -= 1;
        float y = x * v->peak * e; L += y * 0.707f; R += y * 0.707f; continue;
      }
      x = 0;
      for (int k = 0; k < 2; k++) {
        float dt = v->f * v->det[k] * inv; x += 2.0f * v->ph[k] - 1.0f - mu_blep(v->ph[k], dt);
        v->ph[k] += dt; if (v->ph[k] >= 1) v->ph[k] -= 1;
      }
      float shine = 0.35f * sinf(MU_TAU * v->ph[2]); v->ph[2] += 2.0f * v->f * inv; if (v->ph[2] >= 1) v->ph[2] -= 1;
      float fc = v->cut + lfo; if (fc < 120) fc = 120;
      /* saws through a gently resonant low-pass; the octave sine shines through unfiltered */
      float y = (mu_svf(x, tanf(3.14159265f * fc * inv), 0.94f, &v->ic1, &v->ic2, NULL) + shine) * v->peak * e;
      L += y * v->gl; R += y * v->gr; verb += y * 0.9f;
    }

    /* --- stars (phase-modulated bells) --- */
    for (int i = 0; i < MU_MAXBELL; i++) {
      MuBell *b = &mu.bell[i]; if (!b->on) continue;
      float t = (float)b->age * inv; b->age++;
      if (t > b->decay + 0.1f) { b->on = 0; continue; }
      float a = (t < 0.015f ? t / 0.015f : 1.0f) * expf(-t * 6.2f / b->decay) * b->vel;
      float y = sinf(MU_TAU * b->pc + b->idx0 * expf(-t * 1.7f) * sinf(MU_TAU * b->pm)) * a;
      b->pc += b->f * inv; if (b->pc >= 1) b->pc -= 1; b->pm += b->f * b->ratio * inv; if (b->pm >= 1) b->pm -= 1;
      L += y * b->gl; R += y * b->gr; verb += y * 0.9f; echo += y * 0.5f;
    }

    /* --- gentle pulse --- */
    for (int i = 0; i < MU_MAXPUL; i++) {
      MuPulse *p = &mu.pul[i]; if (!p->on) continue;
      float t = (float)p->age * inv; p->age++;
      if (t > 0.7f) { p->on = 0; continue; }
      float tri = 4.0f * fabsf(p->ph - 0.5f) - 1.0f; p->ph += p->f * inv; if (p->ph >= 1) p->ph -= 1;
      p->lp += 0.18f * (tri - p->lp);
      float y = p->lp * (t < 0.01f ? t / 0.01f : 1.0f) * expf(-t * 7.0f) * 0.032f;
      L += y * 0.707f; R += y * 0.707f; verb += y * 0.9f; echo += y * 0.5f;
    }

    /* --- solar wind --- */
    {
      const float gl = 1.0f / (4.0f * MU_SR);
      mu.windF += (mu.windFt - mu.windF) * gl; mu.windG += (mu.windGt - mu.windG) * gl; mu.windP += (mu.windPt - mu.windP) * gl;
      float nz = mu_rand() * 2.0f - 1.0f, band;
      mu_svf(nz, tanf(3.14159265f * mu.windF * inv), 0.2f, &mu.wic1, &mu.wic2, &band);
      float y = band * mu.windG * 0.2f, pl, pr; mu_pan(mu.windP, &pl, &pr);
      L += y * pl; R += y * pr; verb += y * 0.9f;
    }

    /* --- ping-pong echo: 0.48 s left, 0.72 s right --- */
    {
      float dl = mu_read(&mu.echoL, (int)(0.48f * MU_SR)), dr = mu_read(&mu.echoR, (int)(0.72f * MU_SR));
      mu.echoLp += 0.31f * (dr - mu.echoLp);
      mu_write(&mu.echoL, echo + 0.38f * mu.echoLp); mu_write(&mu.echoR, dl);
      L += dl; R += dr; verb += (dl + dr) * 0.45f;
    }

    /* --- long hall reverb: 2 diffusers into an 8-line feedback delay network --- */
    {
      float x = verb * 0.35f;
      for (int a = 0; a < 2; a++) { int d = mu.ap[a].n - 1; float z = mu_read(&mu.ap[a], d), y = -0.6f * x + z; mu_write(&mu.ap[a], x + 0.6f * y); x = y; }
      float o[8], h[8];
      for (int k = 0; k < 8; k++) { o[k] = mu_read(&mu.fdn[k], mu.fdn[k].n - 1); mu.damp[k] += 0.45f * (o[k] - mu.damp[k]); }
      for (int k = 0; k < 8; k++) h[k] = mu.damp[k];
      for (int len = 1; len < 8; len <<= 1)             /* fast Hadamard mix */
        for (int i = 0; i < 8; i += len << 1)
          for (int j = i; j < i + len; j++) { float a = h[j], b = h[j + len]; h[j] = a + b; h[j + len] = a - b; }
      for (int k = 0; k < 8; k++) mu_write(&mu.fdn[k], h[k] * 0.35355339f * mu.fdg[k] + ((k & 1) ? -x : x));
      L += (o[0] - o[2] + o[4] - o[6]) * 0.42f; R += (o[1] - o[3] + o[5] - o[7]) * 0.42f;
    }

    /* --- fade, gentle limiter, output --- */
    float target = InterlockedCompareExchange(&mu.fadeOut, 0, 0) ? 0.0f : 0.6f;
    mu.master += (target - mu.master) * (target > mu.master ? 1.0f / (2.0f * MU_SR) : 1.0f / (0.06f * MU_SR));
    L *= mu.master * 3.2f; R *= mu.master * 3.2f;
    float pk = fabsf(L) > fabsf(R) ? fabsf(L) : fabsf(R);
    mu.env += (pk - mu.env) * (pk > mu.env ? 0.01f : 0.00005f);
    float gain = mu.env > 0.35f ? 0.35f / mu.env : 1.0f;
    L = tanhf(L * gain * 1.4f) / tanhf(1.4f) * 0.65f; R = tanhf(R * gain * 1.4f) / tanhf(1.4f) * 0.65f;
    out[2 * s] = (short)(L * 32000.0f); out[2 * s + 1] = (short)(R * 32000.0f);
  }
}

static DWORD WINAPI mu_thread(LPVOID unused) {
  (void)unused;
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
  for (int i = 0; i < MU_NBUF; i++) { mu_render(mu.buf[i], MU_FRAMES); waveOutWrite(mu.wo, &mu.hdr[i], sizeof(WAVEHDR)); }
  while (InterlockedCompareExchange(&mu.run, 1, 1)) {
    WaitForSingleObject(mu.evt, 250);
    for (int i = 0; i < MU_NBUF; i++)
      if (mu.hdr[i].dwFlags & WHDR_DONE) { mu_render(mu.buf[i], MU_FRAMES); waveOutWrite(mu.wo, &mu.hdr[i], sizeof(WAVEHDR)); }
  }
  return 0;
}

static void music_start(void) {
  if (mu.thread) return;
  memset(&mu, 0, sizeof(mu));
  mu.rng = (unsigned)GetTickCount() | 1u;
  static const int lens[8] = { 2473, 2767, 3217, 3557, 3907, 4127, 4639, 4943 };
  for (int k = 0; k < 8; k++) { if (!mu_line(&mu.fdn[k], lens[k])) return; mu.fdg[k] = powf(10.0f, -3.0f * lens[k] / (7.0f * MU_SR)); }
  if (!mu_line(&mu.ap[0], 142) || !mu_line(&mu.ap[1], 379) || !mu_line(&mu.echoL, MU_SR) || !mu_line(&mu.echoR, MU_SR)) return;
  mu.windF = mu.windFt = 600; mu.windG = 0; mu.windGt = 0.04f;
  mu.nextPulseToggle = 50LL * MU_SR; mu.nextStar = MU_SR / 2;

  WAVEFORMATEX wf; memset(&wf, 0, sizeof(wf));
  wf.wFormatTag = WAVE_FORMAT_PCM; wf.nChannels = 2; wf.nSamplesPerSec = MU_SR; wf.wBitsPerSample = 16;
  wf.nBlockAlign = 4; wf.nAvgBytesPerSec = MU_SR * 4;
  mu.evt = CreateEventW(NULL, FALSE, FALSE, NULL);
  if (!mu.evt || waveOutOpen(&mu.wo, WAVE_MAPPER, &wf, (DWORD_PTR)mu.evt, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) { mu.wo = NULL; return; }
  for (int i = 0; i < MU_NBUF; i++) {
    mu.hdr[i].lpData = (LPSTR)mu.buf[i]; mu.hdr[i].dwBufferLength = MU_FRAMES * 4;
    waveOutPrepareHeader(mu.wo, &mu.hdr[i], sizeof(WAVEHDR));
  }
  mu.run = 1;
  mu.thread = CreateThread(NULL, 0, mu_thread, NULL, 0, NULL);
}

static void music_stop(void) {
  if (!mu.thread) return;
  InterlockedExchange(&mu.fadeOut, 1);
  Sleep(250);                                    /* let the quick fade-out play */
  InterlockedExchange(&mu.run, 0); SetEvent(mu.evt);
  WaitForSingleObject(mu.thread, 2000); CloseHandle(mu.thread); mu.thread = NULL;
  waveOutReset(mu.wo);
  for (int i = 0; i < MU_NBUF; i++) waveOutUnprepareHeader(mu.wo, &mu.hdr[i], sizeof(WAVEHDR));
  waveOutClose(mu.wo); CloseHandle(mu.evt);
}

#endif
