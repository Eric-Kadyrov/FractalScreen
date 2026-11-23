/* FRACTALSCREEN - native Windows screensaver (.scr)
 * Build:  x86_64-w64-mingw32-gcc -O2 -mwindows -DPREMIUM fractalscreen.c res.o -lopengl32 -lgdi32 -ladvapi32 -lshell32 -lcomctl32
 * Args:   /s = run full screen, /p <hwnd> = preview in the settings window, /c = settings dialog
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <wchar.h>
#include <winhttp.h>
#include <time.h>
#include <stdio.h>

/* Licensing server (Cloudflare Pages Functions in website/functions/api) */
#ifndef API_BASE
#define API_BASE L"https://fractalscreen.com/api"
#endif
#define TRIAL_DAYS 28          /* Free edition runs for 4 weeks */
#define GRACE_DAYS 3           /* Premium keeps working offline this long after the paid period */
#define RECHECK_SECONDS (12 * 3600)

#ifndef SITE_URL
#define SITE_URL L"https://fractalscreen.com/#pricing"
#endif

/* ---------- OpenGL 2.0 entry points ---------- */
typedef char GLchar;
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER   0x8B31
#define GL_COMPILE_STATUS  0x8B81
#define GL_LINK_STATUS     0x8B82
typedef GLuint (APIENTRY *PFNCREATESHADER)(GLenum);
typedef void   (APIENTRY *PFNSHADERSOURCE)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void   (APIENTRY *PFNCOMPILESHADER)(GLuint);
typedef GLuint (APIENTRY *PFNCREATEPROGRAM)(void);
typedef void   (APIENTRY *PFNATTACHSHADER)(GLuint, GLuint);
typedef void   (APIENTRY *PFNLINKPROGRAM)(GLuint);
typedef void   (APIENTRY *PFNUSEPROGRAM)(GLuint);
typedef GLint  (APIENTRY *PFNGETUNIFORMLOCATION)(GLuint, const GLchar*);
typedef void   (APIENTRY *PFNUNIFORM1F)(GLint, GLfloat);
typedef void   (APIENTRY *PFNUNIFORM2F)(GLint, GLfloat, GLfloat);
typedef void   (APIENTRY *PFNUNIFORM3F)(GLint, GLfloat, GLfloat, GLfloat);
typedef void   (APIENTRY *PFNGETSHADERIV)(GLuint, GLenum, GLint*);
typedef void   (APIENTRY *PFNGETPROGRAMIV)(GLuint, GLenum, GLint*);
typedef BOOL   (WINAPI  *PFNSWAPINTERVAL)(int);
static PFNCREATESHADER pCreateShader; static PFNSHADERSOURCE pShaderSource; static PFNCOMPILESHADER pCompileShader;
static PFNCREATEPROGRAM pCreateProgram; static PFNATTACHSHADER pAttachShader; static PFNLINKPROGRAM pLinkProgram;
static PFNUSEPROGRAM pUseProgram; static PFNGETUNIFORMLOCATION pGetUniformLocation; static PFNUNIFORM1F pUniform1f;
static PFNUNIFORM2F pUniform2f; static PFNUNIFORM3F pUniform3f; static PFNGETSHADERIV pGetShaderiv; static PFNGETPROGRAMIV pGetProgramiv;

/* ---------- fractals and palettes ---------- */
typedef struct { const wchar_t *name; int shader; } Fractal; /* shader: 0..3 GPU type, -1 = tree */
static const Fractal ALL_FRACTALS[] = {
  {L"Mandelbrot dive", 0}, {L"Julia drift", 1}, {L"Burning ship", 2}, {L"Newton bloom", 3}, {L"Fractal tree", -1}
};
#ifdef PREMIUM
static const int FRACTAL_IDS[] = {0, 1, 2, 3, 4};
static const int PALETTE_IDS[] = {0, 1, 2, 3};
#define EDITION_NAME L"Premium"
#else
static const int FRACTAL_IDS[] = {0, 1, 4};
static const int PALETTE_IDS[] = {0, 2};
#define EDITION_NAME L"Free"
#endif
#define NFRACT ((int)(sizeof(FRACTAL_IDS)/sizeof(int)))
#define NPAL   ((int)(sizeof(PALETTE_IDS)/sizeof(int)))

typedef struct { const wchar_t *name; float a[3], b[3], c[3], d[3]; } Palette;
static const Palette ALL_PALETTES[] = {
  {L"Aurora", {.5f,.5f,.5f}, {.5f,.5f,.5f}, {1,1,1},     {0,.33f,.67f}},
  {L"Ember",  {.5f,.5f,.5f}, {.5f,.5f,.5f}, {1,1,.5f},   {.8f,.9f,.3f}},
  {L"Ocean",  {.5f,.5f,.5f}, {.5f,.5f,.5f}, {1,.7f,.4f}, {0,.15f,.2f}},
  {L"Neon",   {.8f,.5f,.4f}, {.2f,.4f,.2f}, {2,1,1},     {0,.25f,.25f}}
};

/* ---------- settings (HKCU\Software\FractalScreen) ---------- */
static int gFractalSel = 0;   /* index into FRACTAL_IDS, or NFRACT = cycle (Premium) */
static int gPaletteSel = 0;   /* index into PALETTE_IDS */
static int gSpeedSel = 1;     /* 0 slow, 1 normal, 2 fast */
static const float SPEEDS[] = {0.5f, 1.0f, 2.0f};

static DWORD regGet(const wchar_t *name, DWORD def) {
  HKEY k; DWORD v = def, sz = sizeof(v), type;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\FractalScreen", 0, KEY_READ, &k) == ERROR_SUCCESS) {
    if (RegQueryValueExW(k, name, NULL, &type, (BYTE*)&v, &sz) != ERROR_SUCCESS || type != REG_DWORD) v = def;
    RegCloseKey(k);
  }
  return v;
}
static void regSet(const wchar_t *name, DWORD v) {
  HKEY k;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\FractalScreen", 0, NULL, 0, KEY_WRITE, NULL, &k, NULL) == ERROR_SUCCESS) {
    RegSetValueExW(k, name, 0, REG_DWORD, (const BYTE*)&v, sizeof(v));
    RegCloseKey(k);
  }
}
static void loadSettings(void) {
  int maxF = NFRACT;
#ifdef PREMIUM
  maxF = NFRACT + 1;
#endif
  gFractalSel = (int)regGet(L"Fractal", 0); if (gFractalSel < 0 || gFractalSel >= maxF) gFractalSel = 0;
  gPaletteSel = (int)regGet(L"Palette", 0); if (gPaletteSel < 0 || gPaletteSel >= NPAL) gPaletteSel = 0;
  gSpeedSel = (int)regGet(L"Speed", 1); if (gSpeedSel < 0 || gSpeedSel > 2) gSpeedSel = 1;
}


/* ---------- registry helpers ---------- */
static DWORD regGetK(const wchar_t *key, const wchar_t *name, DWORD def) {
  HKEY k; DWORD v = def, sz = sizeof(v), type;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_READ, &k) == ERROR_SUCCESS) {
    if (RegQueryValueExW(k, name, NULL, &type, (BYTE*)&v, &sz) != ERROR_SUCCESS || type != REG_DWORD) v = def;
    RegCloseKey(k);
  }
  return v;
}
static void regSetK(const wchar_t *key, const wchar_t *name, DWORD v) {
  HKEY k;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, NULL, 0, KEY_WRITE, NULL, &k, NULL) == ERROR_SUCCESS) {
    RegSetValueExW(k, name, 0, REG_DWORD, (const BYTE*)&v, sizeof(v)); RegCloseKey(k);
  }
}
static int regGetStr(const wchar_t *name, wchar_t *buf, DWORD cch) {
  HKEY k; DWORD sz = cch * sizeof(wchar_t), type; int ok = 0; buf[0] = 0;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\FractalScreen", 0, KEY_READ, &k) == ERROR_SUCCESS) {
    if (RegQueryValueExW(k, name, NULL, &type, (BYTE*)buf, &sz) == ERROR_SUCCESS && type == REG_SZ) { buf[cch - 1] = 0; ok = buf[0] != 0; }
    else buf[0] = 0;
    RegCloseKey(k);
  }
  return ok;
}
static void regSetStr(const wchar_t *name, const wchar_t *v) {
  HKEY k;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\FractalScreen", 0, NULL, 0, KEY_WRITE, NULL, &k, NULL) == ERROR_SUCCESS) {
    RegSetValueExW(k, name, 0, REG_SZ, (const BYTE*)v, (DWORD)((wcslen(v) + 1) * sizeof(wchar_t))); RegCloseKey(k);
  }
}

/* "now" never moves backwards, so winding the clock back doesn't extend a trial */
#define TRIAL_KEY L"Software\\FractalScreenTrial"
static DWORD effectiveNow(void) {
  DWORD now = (DWORD)time(NULL), seen = regGetK(TRIAL_KEY, L"Seen", 0);
  if (now > seen) { regSetK(TRIAL_KEY, L"Seen", now); return now; }
  return seen;
}
static void formatDate(DWORD unix, wchar_t *out, int cch) {
  ULONGLONG ll = ((ULONGLONG)unix + 11644473600ULL) * 10000000ULL;
  FILETIME ft, lft; SYSTEMTIME st;
  ft.dwLowDateTime = (DWORD)ll; ft.dwHighDateTime = (DWORD)(ll >> 32);
  FileTimeToLocalFileTime(&ft, &lft); FileTimeToSystemTime(&lft, &st);
  if (!GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, out, cch)) swprintf(out, cch, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
}

#ifndef PREMIUM
/* ---------- Free edition: 4-week trial ---------- */
static DWORD trialStart(void) {
  DWORD s = regGetK(TRIAL_KEY, L"Start", 0);
  if (!s) { s = (DWORD)time(NULL); regSetK(TRIAL_KEY, L"Start", s); }
  return s;
}
static long trialSecondsLeft(void) {
  long long left = (long long)trialStart() + TRIAL_DAYS * 86400LL - (long long)effectiveNow();
  return left > 0 ? (long)left : 0;
}
#else
/* ---------- Premium edition: subscription license ---------- */
static int httpGet(const wchar_t *pathQuery, char *out, int cap) {
  URL_COMPONENTS uc; wchar_t host[256], path[512]; int ok = 0, n = 0;
  ZeroMemory(&uc, sizeof(uc)); uc.dwStructSize = sizeof(uc);
  uc.lpszHostName = host; uc.dwHostNameLength = 256; uc.lpszUrlPath = path; uc.dwUrlPathLength = 512;
  if (!WinHttpCrackUrl(API_BASE, 0, 0, &uc)) return 0;
  wchar_t full[1024]; swprintf(full, 1024, L"%ls%ls", path, pathQuery);
  HINTERNET ses = WinHttpOpen(L"FRACTALSCREEN/1.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!ses) return 0;
  WinHttpSetTimeouts(ses, 5000, 5000, 5000, 5000);
  HINTERNET con = WinHttpConnect(ses, host, uc.nPort, 0);
  HINTERNET req = con ? WinHttpOpenRequest(con, L"GET", full, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                    uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0) : NULL;
  if (req && WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(req, NULL)) {
    DWORD code = 0, sz = sizeof(code);
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &code, &sz, WINHTTP_NO_HEADER_INDEX);
    DWORD got = 0;
    while (n < cap - 1 && WinHttpReadData(req, out + n, cap - 1 - n, &got) && got > 0) n += got;
    out[n] = 0;
    ok = code == 200 || code == 404;   /* 404 = unknown key, still a real answer */
  }
  if (req) WinHttpCloseHandle(req); if (con) WinHttpCloseHandle(con); WinHttpCloseHandle(ses);
  return ok;
}
/* 1 = active, 0 = not active, -1 = couldn't reach the server */
static int verifyLicense(const wchar_t *key, DWORD *until) {
  wchar_t clean[96]; int j = 0;
  for (int i = 0; key[i] && j < 95; i++) { wchar_t c = key[i];
    if ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') || c == L'_' || c == L'-') clean[j++] = c; }
  clean[j] = 0;
  if (!j) return 0;
  wchar_t q[160]; swprintf(q, 160, L"/verify.php?key=%ls", clean);
  char buf[2048];
  if (!httpGet(q, buf, sizeof(buf))) return -1;
  const char *u = strstr(buf, "\"until_unix\":"); *until = u ? (DWORD)strtoul(u + 13, NULL, 10) : 0;
  return strstr(buf, "\"active\":true") ? 1 : 0;
}
static void storeVerification(int r, DWORD until) {
  if (r < 0) return;
  regSet(L"LicenseUntil", r == 1 ? until : 0);
  regSet(L"LastCheck", (DWORD)time(NULL));
}
static int licenseOK(void) {
  wchar_t key[96];
  if (!regGetStr(L"LicenseKey", key, 96)) return 0;
  DWORD until = regGet(L"LicenseUntil", 0);
  return until && effectiveNow() <= until + GRACE_DAYS * 86400u;
}
static DWORD WINAPI bgVerify(LPVOID p) {
  (void)p; wchar_t key[96]; DWORD until = 0;
  if (regGetStr(L"LicenseKey", key, 96)) storeVerification(verifyLicense(key, &until), until);
  return 0;
}
#endif

/* ---------- may the fractals run? ---------- */
static const wchar_t *gLockTitle = NULL, *gLockText = NULL;
static void decideAccess(void) {
#ifdef PREMIUM
  wchar_t key[96];
  if (!regGetStr(L"LicenseKey", key, 96)) {
    gLockTitle = L"Activate FRACTALSCREEN Premium";
    gLockText = L"Open FRACTALSCREEN settings from the Start menu and enter your license key.";
    return;
  }
  DWORD until = regGet(L"LicenseUntil", 0), last = regGet(L"LastCheck", 0), now = (DWORD)time(NULL);
  if (!until || !licenseOK()) {
    DWORD u = 0; int r = verifyLicense(key, &u); storeVerification(r, u);   /* ask the server right now */
    if (!licenseOK()) {
      if (r < 0) { gLockTitle = L"Couldn\u2019t check your subscription"; gLockText = L"Connect to the internet, then try again."; }
      else { gLockTitle = L"Your subscription has ended"; gLockText = L"Renew FRACTALSCREEN Premium to bring the fractals back."; }
    }
  } else if (now - last > RECHECK_SECONDS) {
    HANDLE th = CreateThread(NULL, 0, bgVerify, NULL, 0, NULL); if (th) CloseHandle(th);  /* refresh quietly */
  }
#else
  if (trialSecondsLeft() <= 0) {
    gLockTitle = L"Your 4-week free trial has ended";
    gLockText = L"Get FRACTALSCREEN Premium to keep the fractals running.";
  }
#endif
}

/* ---------- shaders ---------- */
static const char *VS = "void main(){gl_Position=gl_Vertex;}";
static const char *FS =
"uniform vec2 uRes;uniform float uTime;uniform float uType;uniform vec3 uA,uB,uC,uD;\n"
"vec3 pal(float t){return clamp(uA+uB*cos(6.28318*(uC*t+uD)),0.,1.);}\n"
"vec2 cmul(vec2 a,vec2 b){return vec2(a.x*b.x-a.y*b.y,a.x*b.y+a.y*b.x);}\n"
"vec2 cdiv(vec2 a,vec2 b){return vec2(a.x*b.x+a.y*b.y,a.y*b.x-a.x*b.y)/dot(b,b);}\n"
"mat2 rot(float a){float c=cos(a),s=sin(a);return mat2(c,-s,s,c);}\n"
"void main(){\n"
" vec2 uv=(gl_FragCoord.xy-.5*uRes)/min(uRes.x,uRes.y);float t=uTime;vec3 col=vec3(0.);\n"
" if(uType<.5){\n"
"  float ph=.5-.5*cos(t*.05);float sc=3.2*pow(1.5e-4,ph);\n"
"  vec2 c=vec2(-.743643887,.131825904)+rot(t*.03)*uv*sc;vec2 z=vec2(0.);float n=0.;bool e=false;\n"
"  for(int i=0;i<500;i++){z=vec2(z.x*z.x-z.y*z.y,2.*z.x*z.y)+c;if(dot(z,z)>256.){e=true;break;}n+=1.;}\n"
"  if(e){float s=n-log2(log2(dot(z,z)))+4.;col=pal(.025*s+t*.04);}\n"
" }else if(uType<1.5){\n"
"  vec2 c=.7885*vec2(cos(t*.12),sin(t*.12));vec2 z=rot(t*.02)*uv*2.6;float n=0.;bool e=false;\n"
"  for(int i=0;i<300;i++){z=vec2(z.x*z.x-z.y*z.y,2.*z.x*z.y)+c;if(dot(z,z)>256.){e=true;break;}n+=1.;}\n"
"  if(e){float s=n-log2(log2(dot(z,z)))+4.;col=pal(.035*s+t*.05);}else{col=pal(t*.05+.5)*.12;}\n"
" }else if(uType<2.5){\n"
"  float ph=.5-.5*cos(t*.06);float sc=3.4*pow(.025,ph);\n"
"  vec2 ctr=mix(vec2(-.45,-.5),vec2(-1.7615,-.0280),smoothstep(0.,.35,ph));\n"
"  vec2 c=ctr+vec2(uv.x,-uv.y)*sc;vec2 z=vec2(0.);float n=0.;bool e=false;\n"
"  for(int i=0;i<300;i++){z=abs(z);z=vec2(z.x*z.x-z.y*z.y,2.*z.x*z.y)+c;if(dot(z,z)>256.){e=true;break;}n+=1.;}\n"
"  if(e){float s=n-log2(log2(dot(z,z)))+4.;col=pal(.03*s+t*.04);}\n"
" }else{\n"
"  float zoom=1.6+.9*sin(t*.07);vec2 z=rot(t*.04)*uv*zoom*2.;float a=1.+.45*sin(t*.18);float n=0.;\n"
"  for(int i=0;i<64;i++){vec2 z2=cmul(z,z);vec2 f=cmul(z2,z)-vec2(1.,0.);if(dot(f,f)<1e-6)break;z-=a*cdiv(f,3.*z2);n+=1.;}\n"
"  float ang=atan(z.y,z.x);float root=floor(mod(ang/2.0944+.5,3.));float sh=pow(1.-n/64.,2.2);\n"
"  col=pal(root/3.+n*.025+t*.03)*(.25+.85*sh);\n"
" }\n"
" float v=1.-.35*dot(uv,uv);gl_FragColor=vec4(col*v,1.);\n"
"}\n";

/* ---------- renderer state ---------- */
static HWND gWnd; static HDC gDC; static HGLRC gRC; static GLuint gProg;
static GLint uRes, uTime, uType, uA, uB, uC, uD;
static int gW = 1, gH = 1, gPreview = 0, gShaderOK = 0, gCur = 0;
static double gT = 0; static LARGE_INTEGER gFreq, gLast; static double gSinceSwitch = 0;
static POINT gStartMouse; static int gHaveMouse = 0; static DWORD gStartTick;

static void *glp(const char *n) { void *p = (void*)wglGetProcAddress(n); return p; }

static int initGL(void) {
  PIXELFORMATDESCRIPTOR pfd = {0};
  pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 0;
  gDC = GetDC(gWnd);
  int pf = ChoosePixelFormat(gDC, &pfd);
  if (!pf || !SetPixelFormat(gDC, pf, &pfd)) return 0;
  gRC = wglCreateContext(gDC); if (!gRC) return 0;
  wglMakeCurrent(gDC, gRC);
  PFNSWAPINTERVAL swap = (PFNSWAPINTERVAL)glp("wglSwapIntervalEXT"); if (swap) swap(1);
  pCreateShader = glp("glCreateShader"); pShaderSource = glp("glShaderSource"); pCompileShader = glp("glCompileShader");
  pCreateProgram = glp("glCreateProgram"); pAttachShader = glp("glAttachShader"); pLinkProgram = glp("glLinkProgram");
  pUseProgram = glp("glUseProgram"); pGetUniformLocation = glp("glGetUniformLocation"); pUniform1f = glp("glUniform1f");
  pUniform2f = glp("glUniform2f"); pUniform3f = glp("glUniform3f"); pGetShaderiv = glp("glGetShaderiv"); pGetProgramiv = glp("glGetProgramiv");
  if (pCreateShader && pShaderSource && pCompileShader && pCreateProgram && pAttachShader && pLinkProgram &&
      pUseProgram && pGetUniformLocation && pUniform1f && pUniform2f && pUniform3f && pGetShaderiv && pGetProgramiv) {
    GLint ok = 0;
    GLuint vs = pCreateShader(GL_VERTEX_SHADER); pShaderSource(vs, 1, &VS, NULL); pCompileShader(vs);
    GLuint fs = pCreateShader(GL_FRAGMENT_SHADER); pShaderSource(fs, 1, &FS, NULL); pCompileShader(fs);
    pGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (ok) {
      gProg = pCreateProgram(); pAttachShader(gProg, vs); pAttachShader(gProg, fs); pLinkProgram(gProg);
      pGetProgramiv(gProg, GL_LINK_STATUS, &ok);
      if (ok) {
        gShaderOK = 1;
        uRes = pGetUniformLocation(gProg, "uRes"); uTime = pGetUniformLocation(gProg, "uTime"); uType = pGetUniformLocation(gProg, "uType");
        uA = pGetUniformLocation(gProg, "uA"); uB = pGetUniformLocation(gProg, "uB"); uC = pGetUniformLocation(gProg, "uC"); uD = pGetUniformLocation(gProg, "uD");
      }
    }
  }
  return 1;
}

static void palColor(const Palette *p, float t, float out[3]) {
  for (int i = 0; i < 3; i++) {
    float v = p->a[i] + p->b[i] * cosf(6.28318f * (p->c[i] * t + p->d[i]));
    out[i] = v < 0 ? 0 : (v > 1 ? 1 : v);
  }
}

/* Six radial fractal trees, drawn breadth-first with additive lines */
typedef struct { float x, y, a; } Branch;
static Branch gBufA[6 << 10], gBufB[6 << 10];
static void drawTree(const Palette *p, float t) {
  if (gShaderOK) pUseProgram(0);
  glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, gW, gH, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  float bg[3]; palColor(p, t * .03f + .5f, bg);
  glDisable(GL_BLEND);
  glBegin(GL_TRIANGLE_FAN);
  glColor3f(bg[0] * .18f, bg[1] * .18f, bg[2] * .18f); glVertex2f(gW * .5f, gH * .5f);
  glColor3f(0, 0, 0);
  for (int i = 0; i <= 48; i++) { float a = i * 6.28318f / 48; float r = sqrtf((float)gW * gW + (float)gH * gH) * .5f;
    glVertex2f(gW * .5f + cosf(a) * r, gH * .5f + sinf(a) * r); }
  glEnd();
  glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_LINE_SMOOTH); glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  const int D = 10; float base = (gW < gH ? gW : gH) * .13f, len = base;
  Branch *cur = gBufA, *nxt = gBufB; int n = 6;
  for (int k = 0; k < 6; k++) { cur[k].x = gW * .5f; cur[k].y = gH * .5f; cur[k].a = k * 6.28318f / 6 + t * .06f; }
  for (int d = 0; d < D; d++) {
    float spread = .32f + .22f * sinf(t * .35f + d * .45f), twist = .12f * sinf(t * .22f + d * .8f);
    float col[3]; palColor(p, (float)d / D * .9f + t * .04f, col);
    float w = (D - d) * base / 90.f; if (w < 1) w = 1; if (w > 10) w = 10;
    glLineWidth(w);
    glColor4f(col[0], col[1], col[2], .55f + .45f * (1 - (float)d / D));
    glBegin(GL_LINES);
    int m = 0;
    for (int i = 0; i < n; i++) {
      float ex = cur[i].x + cosf(cur[i].a) * len, ey = cur[i].y + sinf(cur[i].a) * len;
      glVertex2f(cur[i].x, cur[i].y); glVertex2f(ex, ey);
      if (d < D - 1) {
        nxt[m].x = ex; nxt[m].y = ey; nxt[m].a = cur[i].a - spread + twist; m++;
        nxt[m].x = ex; nxt[m].y = ey; nxt[m].a = cur[i].a + spread + twist; m++;
      }
    }
    glEnd();
    Branch *tmp = cur; cur = nxt; nxt = tmp; n = m; len *= .74f;
  }
  glDisable(GL_BLEND);
}

static void render(void) {
  LARGE_INTEGER now; QueryPerformanceCounter(&now);
  double dt = (double)(now.QuadPart - gLast.QuadPart) / gFreq.QuadPart; gLast = now;
  if (dt > .1) dt = .1;
  gT += dt * SPEEDS[gSpeedSel];
#ifdef PREMIUM
  if (gFractalSel == NFRACT) { gSinceSwitch += dt; if (gSinceSwitch > 25) { gSinceSwitch = 0; gCur = (gCur + 1) % NFRACT; } }
#endif
  const Fractal *f = &ALL_FRACTALS[FRACTAL_IDS[gCur]];
  const Palette *p = &ALL_PALETTES[PALETTE_IDS[gPaletteSel]];
  glViewport(0, 0, gW, gH);
  if (f->shader < 0 || !gShaderOK) {
    drawTree(p, (float)gT);
  } else {
    pUseProgram(gProg);
    pUniform2f(uRes, (float)gW, (float)gH); pUniform1f(uTime, (float)gT); pUniform1f(uType, (float)f->shader);
    pUniform3f(uA, p->a[0], p->a[1], p->a[2]); pUniform3f(uB, p->b[0], p->b[1], p->b[2]);
    pUniform3f(uC, p->c[0], p->c[1], p->c[2]); pUniform3f(uD, p->d[0], p->d[1], p->d[2]);
    glBegin(GL_TRIANGLES); glVertex2f(-1, -1); glVertex2f(3, -1); glVertex2f(-1, 3); glEnd();
  }
  SwapBuffers(gDC);
}

static LRESULT CALLBACK SaverProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  switch (m) {
  case WM_SIZE: gW = LOWORD(l) ? LOWORD(l) : 1; gH = HIWORD(l) ? HIWORD(l) : 1; return 0;
  case WM_SETCURSOR: if (!gPreview) { SetCursor(NULL); return TRUE; } break;
  case WM_MOUSEMOVE:
    if (!gPreview) {
      POINT pt; GetCursorPos(&pt);
      if (!gHaveMouse) { gStartMouse = pt; gHaveMouse = 1; }
      else if (abs(pt.x - gStartMouse.x) > 8 || abs(pt.y - gStartMouse.y) > 8) { if (GetTickCount() - gStartTick > 800) PostMessage(h, WM_CLOSE, 0, 0); }
    }
    return 0;
  case WM_KEYDOWN: case WM_SYSKEYDOWN: case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
    if (!gPreview && GetTickCount() - gStartTick > 500) PostMessage(h, WM_CLOSE, 0, 0);
    return 0;
  case WM_ACTIVATEAPP: if (!gPreview && !w && GetTickCount() - gStartTick > 1500) PostMessage(h, WM_CLOSE, 0, 0); break;
  case WM_SYSCOMMAND: if (!gPreview && (w == SC_SCREENSAVE || w == SC_CLOSE)) return 0; break;
  case WM_ERASEBKGND: return 1;
  case WM_PAINT:
    if (gLockTitle) {
      PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); RECT rc; GetClientRect(h, &rc);
      HBRUSH bg = CreateSolidBrush(RGB(16, 18, 51)); FillRect(dc, &rc, bg); DeleteObject(bg);
      RECT area = rc;
      if (!gPreview) {   /* centre on the main monitor */
        int vx = GetSystemMetrics(SM_XVIRTUALSCREEN), vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
        area.left = -vx; area.top = -vy; area.right = area.left + GetSystemMetrics(SM_CXSCREEN); area.bottom = area.top + GetSystemMetrics(SM_CYSCREEN);
      }
      int big = (area.bottom - area.top) / (gPreview ? 9 : 16), small = big * 5 / 9;
      if (big < 10) big = 10; if (small < 8) small = 8;
      HFONT f1 = CreateFontW(-big, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
      HFONT f2 = CreateFontW(-small, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
      SetBkMode(dc, TRANSPARENT);
      int pad = (area.right - area.left) / 12;
      RECT t1 = { area.left + pad, area.top, area.right - pad, (area.top + area.bottom) / 2 - small / 2 };
      RECT t2 = { area.left + pad, (area.top + area.bottom) / 2 + small / 2, area.right - pad, area.bottom };
      HGDIOBJ old = SelectObject(dc, f1); SetTextColor(dc, RGB(255, 194, 122));
      RECT m1 = t1; DrawTextW(dc, gLockTitle, -1, &m1, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
      t1.top = t1.bottom - (m1.bottom - m1.top);
      DrawTextW(dc, gLockTitle, -1, &t1, DT_CENTER | DT_WORDBREAK);
      if (!gPreview) {
        SelectObject(dc, f2); SetTextColor(dc, RGB(207, 205, 234));
        DrawTextW(dc, gLockText, -1, &t2, DT_CENTER | DT_WORDBREAK);
      }
      SelectObject(dc, old); DeleteObject(f1); DeleteObject(f2);
      EndPaint(h, &ps); return 0;
    }
    break;
  case WM_CLOSE: DestroyWindow(h); return 0;
  case WM_DESTROY:
    if (gRC) { wglMakeCurrent(NULL, NULL); wglDeleteContext(gRC); gRC = NULL; }
    if (gDC) { ReleaseDC(h, gDC); gDC = NULL; }
    PostQuitMessage(0); return 0;
  }
  return DefWindowProcW(h, m, w, l);
}

static int runSaver(HINSTANCE inst, HWND parent) {
  WNDCLASSW wc = {0};
  wc.style = CS_OWNDC; wc.lpfnWndProc = SaverProc; wc.hInstance = inst; wc.lpszClassName = L"FractalScreenSaver";
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
  RegisterClassW(&wc);
  gPreview = parent != NULL;
  if (gPreview) {
    RECT r; GetClientRect(parent, &r); gW = r.right; gH = r.bottom;
    gWnd = CreateWindowExW(0, wc.lpszClassName, L"FRACTALSCREEN", WS_CHILD | WS_VISIBLE, 0, 0, gW, gH, parent, NULL, inst, NULL);
  } else {
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN), y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    gW = GetSystemMetrics(SM_CXVIRTUALSCREEN); gH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    gWnd = CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, L"FRACTALSCREEN", WS_POPUP | WS_VISIBLE, x, y, gW, gH, NULL, NULL, inst, NULL);
  }
  if (!gWnd) return 1;
  decideAccess();
  if (gLockTitle) {   /* trial over or no active subscription: show a message instead of fractals */
    InvalidateRect(gWnd, NULL, FALSE);
    gStartTick = GetTickCount();
    if (!gPreview) { SetForegroundWindow(gWnd); SetFocus(gWnd); }
    MSG lm;
    while (GetMessageW(&lm, NULL, 0, 0) > 0) { TranslateMessage(&lm); DispatchMessageW(&lm); if (gPreview && !IsWindow(parent)) break; }
    return 0;
  }
  if (!initGL()) { DestroyWindow(gWnd); return 1; }
  gCur = gFractalSel < NFRACT ? gFractalSel : 0;
  gT = (double)(GetTickCount() % 30000) / 1000.0;
  QueryPerformanceFrequency(&gFreq); QueryPerformanceCounter(&gLast); gStartTick = GetTickCount();
  if (!gPreview) { SetForegroundWindow(gWnd); SetFocus(gWnd); }
  MSG msg;
  for (;;) {
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) return 0;
      TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    if (gPreview && !IsWindow(parent)) return 0;
    if (gRC) render();
    if (gPreview) Sleep(30);   /* keep the tiny preview light */
  }
}

/* ---------- settings dialog (/c) ---------- */
enum { ID_FRACT = 101, ID_PAL, ID_SPEED, ID_OK, ID_CANCEL, ID_UPGRADE, ID_KEY, ID_LSTATUS, ID_PLAY };
static HWND hKey, hLStatus;
static HWND hFract, hPal, hSpeed; static HFONT gFont;
static HWND mk(HWND p, const wchar_t *cls, const wchar_t *txt, DWORD st, int x, int y, int w, int h, int id) {
  HWND c = CreateWindowExW(0, cls, txt, WS_CHILD | WS_VISIBLE | st, x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
  SendMessageW(c, WM_SETFONT, (WPARAM)gFont, TRUE); return c;
}
static LRESULT CALLBACK CfgProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  switch (m) {
  case WM_CREATE: {
    mk(h, L"STATIC", L"FRACTALSCREEN " EDITION_NAME, 0, 20, 16, 300, 22, 0);
    mk(h, L"STATIC", L"Fractal", 0, 20, 52, 90, 20, 0);
    hFract = mk(h, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, 120, 48, 200, 200, ID_FRACT);
    for (int i = 0; i < NFRACT; i++) SendMessageW(hFract, CB_ADDSTRING, 0, (LPARAM)ALL_FRACTALS[FRACTAL_IDS[i]].name);
#ifdef PREMIUM
    SendMessageW(hFract, CB_ADDSTRING, 0, (LPARAM)L"Cycle through all");
#endif
    SendMessageW(hFract, CB_SETCURSEL, gFractalSel, 0);
    mk(h, L"STATIC", L"Colors", 0, 20, 90, 90, 20, 0);
    hPal = mk(h, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 120, 86, 200, 200, ID_PAL);
    for (int i = 0; i < NPAL; i++) SendMessageW(hPal, CB_ADDSTRING, 0, (LPARAM)ALL_PALETTES[PALETTE_IDS[i]].name);
    SendMessageW(hPal, CB_SETCURSEL, gPaletteSel, 0);
    mk(h, L"STATIC", L"Speed", 0, 20, 128, 90, 20, 0);
    hSpeed = mk(h, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 120, 124, 200, 200, ID_SPEED);
    SendMessageW(hSpeed, CB_ADDSTRING, 0, (LPARAM)L"Slow"); SendMessageW(hSpeed, CB_ADDSTRING, 0, (LPARAM)L"Normal"); SendMessageW(hSpeed, CB_ADDSTRING, 0, (LPARAM)L"Fast");
    SendMessageW(hSpeed, CB_SETCURSEL, gSpeedSel, 0);
#ifndef PREMIUM
    { wchar_t msg[200]; long left = trialSecondsLeft();
      if (left > 0) swprintf(msg, 200, L"Free trial: %ld days, %ld hours left.\nPremium unlocks all 5 fractals, 4 color schemes and cycling.", left / 86400, (left % 86400) / 3600);
      else swprintf(msg, 200, L"Your 4-week free trial has ended.\nGet Premium to keep the fractals running.");
      mk(h, L"STATIC", msg, 0, 20, 166, 410, 52, 0); }
    mk(h, L"BUTTON", L"Get Premium", WS_TABSTOP, 20, 236, 110, 30, ID_UPGRADE);
#else
    mk(h, L"STATIC", L"License key", 0, 20, 170, 90, 20, 0);
    { wchar_t key[96]; regGetStr(L"LicenseKey", key, 96);
      hKey = mk(h, L"EDIT", key, WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, 120, 166, 200, 24, ID_KEY); }
    { wchar_t st[160], d[64]; DWORD until = regGet(L"LicenseUntil", 0);
      if (licenseOK()) { formatDate(until, d, 64); swprintf(st, 160, L"Active, renews or ends on %ls", d); }
      else swprintf(st, 160, L"%ls", regGet(L"LastCheck", 0) ? L"Not active. Renew, or check the key." : L"Not activated yet. Paste the key from your download page.");
      hLStatus = mk(h, L"STATIC", st, 0, 120, 196, 220, 34, ID_LSTATUS); }
#endif
#ifndef PREMIUM
    mk(h, L"BUTTON", L"Play now", WS_TABSTOP, 140, 236, 100, 30, ID_PLAY);
#else
    mk(h, L"BUTTON", L"Play now", WS_TABSTOP, 20, 236, 100, 30, ID_PLAY);
#endif
    mk(h, L"BUTTON", L"OK", BS_DEFPUSHBUTTON | WS_TABSTOP, 260, 236, 80, 30, ID_OK);
    mk(h, L"BUTTON", L"Cancel", WS_TABSTOP, 350, 236, 80, 30, ID_CANCEL);
    return 0; }
  case WM_COMMAND:
    switch (LOWORD(w)) {
    case ID_OK:
      regSet(L"Fractal", (DWORD)SendMessageW(hFract, CB_GETCURSEL, 0, 0));
      regSet(L"Palette", (DWORD)SendMessageW(hPal, CB_GETCURSEL, 0, 0));
      regSet(L"Speed", (DWORD)SendMessageW(hSpeed, CB_GETCURSEL, 0, 0));
#ifdef PREMIUM
      { wchar_t nk[96], ok[96]; GetWindowTextW(hKey, nk, 96); regGetStr(L"LicenseKey", ok, 96);
        wchar_t *a = nk; while (*a == L' ') a++; size_t L = wcslen(a); while (L && (a[L-1] == L' ' || a[L-1] == L'\r' || a[L-1] == L'\n')) a[--L] = 0;
        if (*a && (wcscmp(a, ok) != 0 || !licenseOK())) {
          regSetStr(L"LicenseKey", a);
          HCURSOR oc = SetCursor(LoadCursor(NULL, IDC_WAIT)); DWORD u = 0; int r = verifyLicense(a, &u); SetCursor(oc);
          storeVerification(r, u);
          if (r == 1) MessageBoxW(h, L"Thank you! FRACTALSCREEN Premium is active.", L"FRACTALSCREEN", MB_ICONINFORMATION);
          else {
            MessageBoxW(h, r < 0 ? L"Couldn’t reach the license server. Check your internet connection and try again."
                                 : L"This license key isn’t active. Check that you copied it fully, or renew your subscription.",
                        L"FRACTALSCREEN", MB_ICONWARNING);
            return 0;   /* keep the window open so the key can be fixed */
          }
        } else if (!*a) regSetStr(L"LicenseKey", L"");
      }
#endif
      DestroyWindow(h); return 0;
    case ID_CANCEL: DestroyWindow(h); return 0;
    case ID_PLAY: {   /* save the current choices, then run the screensaver full screen */
      regSet(L"Fractal", (DWORD)SendMessageW(hFract, CB_GETCURSEL, 0, 0));
      regSet(L"Palette", (DWORD)SendMessageW(hPal, CB_GETCURSEL, 0, 0));
      regSet(L"Speed", (DWORD)SendMessageW(hSpeed, CB_GETCURSEL, 0, 0));
      wchar_t self[MAX_PATH], cmd[MAX_PATH + 16];
      GetModuleFileNameW(NULL, self, MAX_PATH);
      swprintf(cmd, MAX_PATH + 16, L"\"%ls\" /s", self);
      STARTUPINFOW si; PROCESS_INFORMATION pi; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
      if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) { CloseHandle(pi.hThread); CloseHandle(pi.hProcess); }
      return 0; }
    case ID_UPGRADE: ShellExecuteW(NULL, L"open", SITE_URL, NULL, NULL, SW_SHOWNORMAL); return 0;
    }
    break;
  case WM_CTLCOLORSTATIC: SetBkMode((HDC)w, TRANSPARENT); return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
  case WM_DESTROY: PostQuitMessage(0); return 0;
  }
  return DefWindowProcW(h, m, w, l);
}
static int runConfig(HINSTANCE inst, HWND owner) {
  NONCLIENTMETRICSW ncm = {0}; ncm.cbSize = sizeof(ncm);
  SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
  gFont = CreateFontIndirectW(&ncm.lfMessageFont);
  WNDCLASSW wc = {0};
  wc.lpfnWndProc = CfgProc; wc.hInstance = inst; wc.lpszClassName = L"FractalScreenConfig";
  wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW); wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
  RegisterClassW(&wc);
  RECT r = {0, 0, 450, 286}; AdjustWindowRect(&r, WS_CAPTION | WS_SYSMENU, FALSE);
  int ww = r.right - r.left, hh = r.bottom - r.top;
  HWND h = CreateWindowExW(WS_EX_DLGMODALFRAME, wc.lpszClassName, L"FRACTALSCREEN settings", WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
    (GetSystemMetrics(SM_CXSCREEN) - ww) / 2, (GetSystemMetrics(SM_CYSCREEN) - hh) / 2, ww, hh, owner, NULL, inst, NULL);
  if (!h) return 1;
  if (owner) EnableWindow(owner, FALSE);
  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0) > 0) { if (!IsDialogMessageW(h, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); } }
  if (owner) { EnableWindow(owner, TRUE); SetForegroundWindow(owner); }
  return 0;
}

/* ---------- entry ---------- */
static HWND parseHwnd(const wchar_t *s) {
  while (*s && (*s == ':' || *s == ' ')) s++;
  return (HWND)(INT_PTR)_wcstoui64(s, NULL, 10);
}
int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmd, int show) {
  (void)prev; (void)show;
  loadSettings();
  const wchar_t *c = cmd; while (*c == ' ') c++;
  if (*c == '/' || *c == '-') {
    wchar_t k = (wchar_t)towlower(c[1]);
    if (k == 's') return runSaver(inst, NULL);
    if (k == 'p') { HWND p = parseHwnd(c + 2); return p ? runSaver(inst, p) : 0; }
    if (k == 'c') { HWND o = parseHwnd(c + 2); return runConfig(inst, (o && IsWindow(o)) ? o : NULL); }
  }
  return runConfig(inst, NULL);   /* double-click or right-click > Configure */
}
