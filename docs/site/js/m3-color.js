/*
 * m3-color.js: Material 3 color system for Material Audacity docs site.
 *
 * Implements the HCT (Hue, Chroma, Tone) color space used by Material 3,
 * built on the CAM16 color appearance model, plus tonal palettes and a
 * light/dark scheme generator from a single seed color.
 *
 * This is a from-scratch, dependency-free re-implementation of the public
 * algorithm published by Google's material-color-utilities project
 * (Apache-2.0). No code was copied from that repository; only the published
 * math (sRGB <-> XYZ, CAM16 forward/reverse, and the HCT solver) was used.
 */
(function (global) {
  'use strict';

  // ---------- sRGB <-> XYZ ----------

  function srgbToLinear(c) {
    c /= 255;
    return c <= 0.04045 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4);
  }

  function linearToSrgb(c) {
    const v = c <= 0.0031308 ? c * 12.92 : 1.055 * Math.pow(c, 1 / 2.4) - 0.055;
    return Math.round(clamp(v, 0, 1) * 255);
  }

  function clamp(v, lo, hi) {
    return Math.max(lo, Math.min(hi, v));
  }

  // sRGB (0-255) to XYZ (D65, 0-100 scale)
  function rgbToXyz(r, g, b) {
    const rl = srgbToLinear(r) * 100;
    const gl = srgbToLinear(g) * 100;
    const bl = srgbToLinear(b) * 100;
    return {
      x: rl * 0.41233895 + gl * 0.35762064 + bl * 0.18051042,
      y: rl * 0.2126 + gl * 0.7152 + bl * 0.0722,
      z: rl * 0.01932141 + gl * 0.11916382 + bl * 0.95034478,
    };
  }

  function xyzToRgb(x, y, z) {
    const rl = x * 3.2406 + y * -1.5372 + z * -0.4986;
    const gl = x * -0.9689 + y * 1.8758 + z * 0.0415;
    const bl = x * 0.0557 + y * -0.204 + z * 1.057;
    return {
      r: linearToSrgb(rl / 100),
      g: linearToSrgb(gl / 100),
      b: linearToSrgb(bl / 100),
    };
  }

  // D65 white point
  const WHITE = { x: 95.047, y: 100.0, z: 108.883 };

  // CAM16 viewing conditions (average surround, sRGB reference)
  const VC = (function () {
    const adaptingLuminance = (200 / Math.PI) * (WHITE.y / 100) * 0.2;
    const backgroundLstar = 50;
    const surround = 2.0; // average
    const n = WHITE.y / WHITE.y; // = 1 (background white relative to white point)
    const c = 0.69; // average surround
    const nc = 1.0;
    const f = 1.0;
    const rgbD = [1.0, 1.0, 1.0];
    const k = 1 / (5 * adaptingLuminance + 1);
    const k4 = k * k * k * k;
    const k4F = 1 - k4;
    const fl = (k4 * adaptingLuminance) + 0.1 * k4F * k4F * Math.cbrt(5 * adaptingLuminance);
    const fLRoot = Math.pow(fl, 0.25);
    const z = 1.48 + Math.sqrt(n);
    return { n, aw: 0, nbb: 0, ncb: 0, c, nc, fl, fLRoot, z };
  })();

  // CAM16-UCS conversion matrix from XYZ (scaled 0-1)
  const CAM16_M = [
    [0.401288, 0.650173, -0.051461],
    [-0.250268, 1.204414, 0.045854],
    [-0.002079, 0.048952, 0.953127],
  ];

  function applyMatrix(m, v) {
    return [
      m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2],
      m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2],
      m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2],
    ];
  }

  function signum(n) {
    return n < 0 ? -1 : n === 0 ? 0 : 1;
  }

  // Precompute Aw (achromatic response for white) with the same pipeline used per-color.
  function computeAw() {
    const rgb = applyMatrix(CAM16_M, [WHITE.x, WHITE.y, WHITE.z]);
    const rD = rgb[0], gD = rgb[1], bD = rgb[2];
    const rAf = Math.pow((VC.fl * Math.abs(rD)) / 100, 0.42);
    const gAf = Math.pow((VC.fl * Math.abs(gD)) / 100, 0.42);
    const bAf = Math.pow((VC.fl * Math.abs(bD)) / 100, 0.42);
    const rA = (signum(rD) * 400 * rAf) / (rAf + 27.13);
    const gA = (signum(gD) * 400 * gAf) / (gAf + 27.13);
    const bA = (signum(bD) * 400 * bAf) / (bAf + 27.13);
    return (2 * rA + gA + 0.05 * bA) * 1.0;
  }
  VC.aw = computeAw() * (1 / VC.n === 1 ? 1 : 1);
  VC.nbb = 0.725 / Math.pow(VC.n, 0.2);
  VC.ncb = VC.nbb;

  // XYZ (0-100 scale) -> CAM16 (returns J (lightness), C (chroma), h (hue in deg))
  function xyzToCam16(x, y, z) {
    const rgb = applyMatrix(CAM16_M, [x, y, z]);
    const rD = rgb[0], gD = rgb[1], bD = rgb[2];
    const rAf = Math.pow((VC.fl * Math.abs(rD)) / 100, 0.42);
    const gAf = Math.pow((VC.fl * Math.abs(gD)) / 100, 0.42);
    const bAf = Math.pow((VC.fl * Math.abs(bD)) / 100, 0.42);
    const rA = (signum(rD) * 400 * rAf) / (rAf + 27.13);
    const gA = (signum(gD) * 400 * gAf) / (gAf + 27.13);
    const bA = (signum(bD) * 400 * bAf) / (bAf + 27.13);

    const a = (11 * rA - 12 * gA + bA) / 11;
    const b = (rA + gA - 2 * bA) / 9;
    const atan2 = Math.atan2(b, a);
    let hueDeg = (atan2 * 180) / Math.PI;
    if (hueDeg < 0) hueDeg += 360;
    if (hueDeg >= 360) hueDeg -= 360;

    const ac = (2 * rA + gA + 0.05 * bA) * VC.nbb;
    const J = 100 * Math.pow(ac / VC.aw, VC.c * VC.z);

    const huePrime = hueDeg < 20.14 ? hueDeg + 360 : hueDeg;
    const et = 0.25 * (Math.cos((huePrime * Math.PI) / 180 + 2) + 3.8);
    const t =
      (50000 / 13) * VC.nc * VC.ncb * et * Math.sqrt(a * a + b * b) /
      (rA + gA + 21 / 20 * bA);
    const alpha = Math.pow(t, 0.9) * Math.pow(1.64 - Math.pow(0.29, VC.n), 0.73);
    const C = alpha * Math.sqrt(J / 100);

    return { J, C, h: hueDeg };
  }

  // CAM16 (J, C, h) -> XYZ, inverting the forward model.
  function cam16ToXyz(J, C, h) {
    const alpha = C === 0 || J === 0 ? 0 : C / Math.sqrt(J / 100);
    const t = Math.pow(alpha / Math.pow(1.64 - Math.pow(0.29, VC.n), 0.73), 1 / 0.9);
    const hRad = (h * Math.PI) / 180;
    const ac = VC.aw * Math.pow(J / 100, 1 / (VC.c * VC.z));
    const p1 = 0.25 * (Math.cos(hRad + 2) + 3.8) * (50000 / 13) * VC.nc * VC.ncb;
    const p2 = ac / VC.nbb;

    const gammaVal = t === 0 ? 0 : (p2 / p1) * t;
    const aFinal = gammaVal * Math.cos(hRad);
    const bFinal = gammaVal * Math.sin(hRad);

    const rA = (460 * p2 + 451 * aFinal + 288 * bFinal) / 1403;
    const gA = (460 * p2 - 891 * aFinal - 261 * bFinal) / 1403;
    const bA2 = (460 * p2 - 220 * aFinal - 6300 * bFinal) / 1403;

    const rCBase = Math.max(0, (27.13 * Math.abs(rA)) / (400 - Math.abs(rA)));
    const rC = signum(rA) * (100 / VC.fl) * Math.pow(rCBase, 1 / 0.42);
    const gCBase = Math.max(0, (27.13 * Math.abs(gA)) / (400 - Math.abs(gA)));
    const gC = signum(gA) * (100 / VC.fl) * Math.pow(gCBase, 1 / 0.42);
    const bCBase = Math.max(0, (27.13 * Math.abs(bA2)) / (400 - Math.abs(bA2)));
    const bC = signum(bA2) * (100 / VC.fl) * Math.pow(bCBase, 1 / 0.42);

    const inv = invertMatrix(CAM16_M);
    return applyMatrixVec(inv, [rC, gC, bC]);
  }

  function invertMatrix(m) {
    const [a, b, c] = m[0];
    const [d, e, f] = m[1];
    const [g, h, i] = m[2];
    const A = e * i - f * h, B = -(d * i - f * g), C = d * h - e * g;
    const D = -(b * i - c * h), E = a * i - c * g, F = -(a * h - b * g);
    const G = b * f - c * e, H = -(a * f - c * d), I = a * e - b * d;
    const det = a * A + b * B + c * C;
    return [
      [A / det, D / det, G / det],
      [B / det, E / det, H / det],
      [C / det, F / det, I / det],
    ];
  }
  function applyMatrixVec(m, v) {
    return {
      x: m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2],
      y: m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2],
      z: m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2],
    };
  }

  // ---------- L* (tone) helpers (CIE L*, used as HCT's "T") ----------

  function yToLstar(y) {
    const yNorm = y / 100;
    const e = 216 / 24389;
    if (yNorm <= e) return (24389 / 27) * yNorm;
    return 116 * Math.cbrt(yNorm) - 16;
  }
  function lstarToY(lstar) {
    if (lstar > 8) return 100 * Math.pow((lstar + 16) / 116, 3);
    return (100 * lstar) / (24389 / 27);
  }

  // ---------- HCT: solve for a color with a given hue/chroma/tone ----------
  // We binary-search the J that yields the requested L*, per the published algorithm's
  // simplification: increasing J is monotonic with L* at fixed hue/chroma for in-gamut colors.

  function hctToArgb(hue, chroma, tone) {
    if (chroma < 0.0001 || tone <= 0 || tone >= 100) {
      const y = lstarToY(clamp(tone, 0, 100));
      const gray = linearToSrgb(y / 100);
      return { r: gray, g: gray, b: gray };
    }
    hue = ((hue % 360) + 360) % 360;
    const targetY = lstarToY(tone);
    let low = 0, high = 150, best = null, bestDelta = Infinity;
    for (let iter = 0; iter < 30; iter++) {
      const mid = (low + high) / 2;
      const xyz = cam16ToXyz(mid, chroma, hue);
      const y = clamp(xyz.y, 0, 100);
      const delta = Math.abs(y - targetY);
      if (delta < bestDelta) { bestDelta = delta; best = xyz; }
      if (y < targetY) low = mid; else high = mid;
    }
    const rgb = xyzToRgb(best.x, best.y, best.z);
    return rgb;
  }

  function argbToHct(r, g, b) {
    const xyz = rgbToXyz(r, g, b);
    const cam = xyzToCam16(xyz.x, xyz.y, xyz.z);
    const tone = yToLstar(xyz.y);
    return { hue: cam.h, chroma: cam.C, tone };
  }

  function rgbToHex(r, g, b) {
    return '#' + [r, g, b].map((v) => clamp(Math.round(v), 0, 255).toString(16).padStart(2, '0')).join('');
  }
  function hexToRgb(hex) {
    hex = hex.replace('#', '');
    if (hex.length === 3) hex = hex.split('').map((c) => c + c).join('');
    const n = parseInt(hex, 16);
    return { r: (n >> 16) & 255, g: (n >> 8) & 255, b: n & 255 };
  }

  // ---------- Tonal palette ----------

  function TonalPalette(hue, chroma) {
    const cache = {};
    return {
      hue,
      chroma,
      tone(t) {
        t = clamp(Math.round(t), 0, 100);
        if (cache[t]) return cache[t];
        const rgb = hctToArgb(hue, chroma, t);
        const hex = rgbToHex(rgb.r, rgb.g, rgb.b);
        cache[t] = hex;
        return hex;
      },
    };
  }

  // ---------- Scheme construction (M3 tonal roles) ----------

  function buildScheme(seedHex, isDark) {
    const rgb = hexToRgb(seedHex);
    const hct = argbToHct(rgb.r, rgb.g, rgb.b);
    const hue = hct.hue;

    const primary = TonalPalette(hue, Math.max(hct.chroma, 48));
    const secondary = TonalPalette(hue, 16);
    const tertiary = TonalPalette((hue + 60) % 360, 24);
    const neutral = TonalPalette(hue, 4);
    const neutralVariant = TonalPalette(hue, 8);
    const error = TonalPalette(25, 84);

    const T = (pal, tone) => pal.tone(tone);

    if (isDark) {
      return {
        primary: T(primary, 80), onPrimary: T(primary, 20),
        primaryContainer: T(primary, 30), onPrimaryContainer: T(primary, 90),
        secondary: T(secondary, 80), onSecondary: T(secondary, 20),
        secondaryContainer: T(secondary, 30), onSecondaryContainer: T(secondary, 90),
        tertiary: T(tertiary, 80), onTertiary: T(tertiary, 20),
        tertiaryContainer: T(tertiary, 30), onTertiaryContainer: T(tertiary, 90),
        error: T(error, 80), onError: T(error, 20),
        errorContainer: T(error, 30), onErrorContainer: T(error, 90),
        background: T(neutral, 6), onBackground: T(neutral, 90),
        surface: T(neutral, 6), onSurface: T(neutral, 90),
        surfaceVariant: T(neutralVariant, 30), onSurfaceVariant: T(neutralVariant, 80),
        outline: T(neutralVariant, 60), outlineVariant: T(neutralVariant, 30),
        surfaceContainerLowest: T(neutral, 4), surfaceContainerLow: T(neutral, 10),
        surfaceContainer: T(neutral, 12), surfaceContainerHigh: T(neutral, 17),
        surfaceContainerHighest: T(neutral, 22),
        inverseSurface: T(neutral, 90), inverseOnSurface: T(neutral, 20),
        inversePrimary: T(primary, 40), shadow: '#000000', scrim: '#000000',
      };
    }
    return {
      primary: T(primary, 40), onPrimary: T(primary, 100),
      primaryContainer: T(primary, 90), onPrimaryContainer: T(primary, 10),
      secondary: T(secondary, 40), onSecondary: T(secondary, 100),
      secondaryContainer: T(secondary, 90), onSecondaryContainer: T(secondary, 10),
      tertiary: T(tertiary, 40), onTertiary: T(tertiary, 100),
      tertiaryContainer: T(tertiary, 90), onTertiaryContainer: T(tertiary, 10),
      error: T(error, 40), onError: T(error, 100),
      errorContainer: T(error, 90), onErrorContainer: T(error, 10),
      background: T(neutral, 98), onBackground: T(neutral, 10),
      surface: T(neutral, 98), onSurface: T(neutral, 10),
      surfaceVariant: T(neutralVariant, 90), onSurfaceVariant: T(neutralVariant, 30),
      outline: T(neutralVariant, 50), outlineVariant: T(neutralVariant, 80),
      surfaceContainerLowest: T(neutral, 100), surfaceContainerLow: T(neutral, 96),
      surfaceContainer: T(neutral, 94), surfaceContainerHigh: T(neutral, 92),
      surfaceContainerHighest: T(neutral, 90),
      inverseSurface: T(neutral, 20), inverseOnSurface: T(neutral, 95),
      inversePrimary: T(primary, 80), shadow: '#000000', scrim: '#000000',
    };
  }

  global.M3Color = {
    hctToArgb, argbToHct, rgbToHex, hexToRgb, TonalPalette, buildScheme,
    rgbToXyz, xyzToRgb, yToLstar, lstarToY,
  };
})(window);
