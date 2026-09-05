"""
Material Design 3 colour math for Audacity.

This is a self contained port of the core of material-color-utilities:
sRGB to CIE XYZ, the CAM16 appearance model, the HCT colour space
(hue, chroma, tone), tonal palettes and the Material 3 scheme variants.

It is kept dependency free on purpose so that the generated theme files can be
reproduced by anyone with a stock Python 3 interpreter, and so that the C++
implementation in src/uicomponents/components/m3hct.cpp can be checked against
it value for value.

Reference: https://github.com/material-foundation/material-color-utilities
Licensed under the Apache License 2.0 by Google LLC. Ported to Python for
Audacity's build tooling.
"""

import math

# ---------------------------------------------------------------------------
# Basic colour utilities
# ---------------------------------------------------------------------------

SRGB_TO_XYZ = (
    (0.41233895, 0.35762064, 0.18051042),
    (0.2126, 0.7152, 0.0722),
    (0.01932141, 0.11916382, 0.95034478),
)

XYZ_TO_SRGB = (
    (3.2413774792388685, -1.5376652402851851, -0.49885366846268053),
    (-0.9691452513005321, 1.8758853451067872, 0.04156585616912061),
    (0.05562093689691305, -0.20395524564742123, 1.0571799111220335),
)

WHITE_POINT_D65 = (95.047, 100.0, 108.883)


def argb_from_rgb(red, green, blue):
    return (255 << 24) | ((red & 255) << 16) | ((green & 255) << 8) | (blue & 255)


def red_from_argb(argb):
    return (argb >> 16) & 255


def green_from_argb(argb):
    return (argb >> 8) & 255


def blue_from_argb(argb):
    return argb & 255


def linearized(rgb_component):
    normalized = rgb_component / 255.0
    if normalized <= 0.040449936:
        return normalized / 12.92 * 100.0
    return math.pow((normalized + 0.055) / 1.055, 2.4) * 100.0


def delinearized(rgb_component):
    normalized = rgb_component / 100.0
    if normalized <= 0.0031308:
        delin = normalized * 12.92
    else:
        delin = 1.055 * math.pow(normalized, 1.0 / 2.4) - 0.055
    return clamp_int(0, 255, round(delin * 255.0))


def clamp_int(low, high, value):
    return max(low, min(high, value))


def clamp_double(low, high, value):
    return max(low, min(high, value))


def matrix_multiply(row, matrix):
    a = row[0] * matrix[0][0] + row[1] * matrix[0][1] + row[2] * matrix[0][2]
    b = row[0] * matrix[1][0] + row[1] * matrix[1][1] + row[2] * matrix[1][2]
    c = row[0] * matrix[2][0] + row[1] * matrix[2][1] + row[2] * matrix[2][2]
    return (a, b, c)


def xyz_from_argb(argb):
    r = linearized(red_from_argb(argb))
    g = linearized(green_from_argb(argb))
    b = linearized(blue_from_argb(argb))
    return matrix_multiply((r, g, b), SRGB_TO_XYZ)


def argb_from_xyz(x, y, z):
    linear = matrix_multiply((x, y, z), XYZ_TO_SRGB)
    return argb_from_rgb(delinearized(linear[0]),
                         delinearized(linear[1]),
                         delinearized(linear[2]))


def lab_f(t):
    e = 216.0 / 24389.0
    kappa = 24389.0 / 27.0
    if t > e:
        return math.pow(t, 1.0 / 3.0)
    return (kappa * t + 16.0) / 116.0


def lab_inv_f(ft):
    e = 216.0 / 24389.0
    kappa = 24389.0 / 27.0
    ft3 = ft * ft * ft
    if ft3 > e:
        return ft3
    return (116.0 * ft - 16.0) / kappa


def lstar_from_y(y):
    return lab_f(y / 100.0) * 116.0 - 16.0


def y_from_lstar(lstar):
    return 100.0 * lab_inv_f((lstar + 16.0) / 116.0)


def lstar_from_argb(argb):
    return lstar_from_y(xyz_from_argb(argb)[1])


def argb_from_lstar(lstar):
    y = y_from_lstar(lstar)
    component = delinearized(y)
    return argb_from_rgb(component, component, component)


def sanitize_degrees_double(degrees):
    degrees = math.fmod(degrees, 360.0)
    if degrees < 0:
        degrees += 360.0
    return degrees


def hex_from_argb(argb):
    return "#%02X%02X%02X" % (red_from_argb(argb), green_from_argb(argb), blue_from_argb(argb))


def argb_from_hex(text):
    text = text.lstrip("#")
    if len(text) == 3:
        text = "".join(ch * 2 for ch in text)
    return argb_from_rgb(int(text[0:2], 16), int(text[2:4], 16), int(text[4:6], 16))


# ---------------------------------------------------------------------------
# CAM16
# ---------------------------------------------------------------------------

class ViewingConditions:
    def __init__(self, white_point, adapting_luminance, background_lstar,
                 surround, discounting_illuminant):
        background_lstar = max(0.1, background_lstar)
        xyz = white_point
        r_w = xyz[0] * 0.401288 + xyz[1] * 0.650173 + xyz[2] * -0.051461
        g_w = xyz[0] * -0.250268 + xyz[1] * 1.204414 + xyz[2] * 0.045854
        b_w = xyz[0] * -0.002079 + xyz[1] * 0.048952 + xyz[2] * 0.953127

        f = 0.8 + surround / 10.0
        if f >= 0.9:
            c = lerp(0.59, 0.69, (f - 0.9) * 10.0)
        else:
            c = lerp(0.525, 0.59, (f - 0.8) * 10.0)

        if discounting_illuminant:
            d = 1.0
        else:
            d = f * (1.0 - (1.0 / 3.6) * math.exp((-adapting_luminance - 42.0) / 92.0))
        d = clamp_double(0.0, 1.0, d)
        nc = f

        rgb_d = (
            d * (100.0 / r_w) + 1.0 - d,
            d * (100.0 / g_w) + 1.0 - d,
            d * (100.0 / b_w) + 1.0 - d,
        )

        k = 1.0 / (5.0 * adapting_luminance + 1.0)
        k4 = k * k * k * k
        k4f = 1.0 - k4
        fl = k4 * adapting_luminance + 0.1 * k4f * k4f * math.pow(5.0 * adapting_luminance, 1.0 / 3.0)
        n = y_from_lstar(background_lstar) / white_point[1]
        z = 1.48 + math.sqrt(n)
        nbb = 0.725 / math.pow(n, 0.2)
        ncb = nbb

        rgb_a_factors = tuple(
            math.pow(fl * rgb_d[i] * w / 100.0, 0.42)
            for i, w in enumerate((r_w, g_w, b_w))
        )
        rgb_a = tuple(400.0 * factor / (factor + 27.13) for factor in rgb_a_factors)
        aw = (2.0 * rgb_a[0] + rgb_a[1] + 0.05 * rgb_a[2]) * nbb

        self.n = n
        self.aw = aw
        self.nbb = nbb
        self.ncb = ncb
        self.c = c
        self.nc = nc
        self.rgb_d = rgb_d
        self.fl = fl
        self.fl_root = math.pow(fl, 0.25)
        self.z = z


def lerp(start, stop, amount):
    return (1.0 - amount) * start + amount * stop


VIEWING_CONDITIONS_SRGB = ViewingConditions(
    WHITE_POINT_D65,
    (200.0 / math.pi) * y_from_lstar(50.0) / 100.0,
    50.0,
    2.0,
    False,
)


class Cam16:
    def __init__(self, hue, chroma, j, q, m, s, jstar, astar, bstar):
        self.hue = hue
        self.chroma = chroma
        self.j = j
        self.q = q
        self.m = m
        self.s = s
        self.jstar = jstar
        self.astar = astar
        self.bstar = bstar

    def distance(self, other):
        d_j = self.jstar - other.jstar
        d_a = self.astar - other.astar
        d_b = self.bstar - other.bstar
        d_e_prime = math.sqrt(d_j * d_j + d_a * d_a + d_b * d_b)
        return 1.41 * math.pow(d_e_prime, 0.63)

    @staticmethod
    def from_int(argb):
        return Cam16.from_int_in_viewing_conditions(argb, VIEWING_CONDITIONS_SRGB)

    @staticmethod
    def from_int_in_viewing_conditions(argb, vc):
        red = (argb & 0x00ff0000) >> 16
        green = (argb & 0x0000ff00) >> 8
        blue = argb & 0x000000ff
        red_l = linearized(red)
        green_l = linearized(green)
        blue_l = linearized(blue)
        x = 0.41233895 * red_l + 0.35762064 * green_l + 0.18051042 * blue_l
        y = 0.2126 * red_l + 0.7152 * green_l + 0.0722 * blue_l
        z = 0.01932141 * red_l + 0.11916382 * green_l + 0.95034478 * blue_l
        return Cam16.from_xyz_in_viewing_conditions(x, y, z, vc)

    @staticmethod
    def from_xyz_in_viewing_conditions(x, y, z, vc):
        r_c = 0.401288 * x + 0.650173 * y - 0.051461 * z
        g_c = -0.250268 * x + 1.204414 * y + 0.045854 * z
        b_c = -0.002079 * x + 0.048952 * y + 0.953127 * z

        r_d = vc.rgb_d[0] * r_c
        g_d = vc.rgb_d[1] * g_c
        b_d = vc.rgb_d[2] * b_c

        r_af = math.pow(vc.fl * abs(r_d) / 100.0, 0.42)
        g_af = math.pow(vc.fl * abs(g_d) / 100.0, 0.42)
        b_af = math.pow(vc.fl * abs(b_d) / 100.0, 0.42)
        r_a = math.copysign(400.0 * r_af / (r_af + 27.13), r_d)
        g_a = math.copysign(400.0 * g_af / (g_af + 27.13), g_d)
        b_a = math.copysign(400.0 * b_af / (b_af + 27.13), b_d)

        a = (11.0 * r_a + -12.0 * g_a + b_a) / 11.0
        b = (r_a + g_a - 2.0 * b_a) / 9.0
        u = (20.0 * r_a + 20.0 * g_a + 21.0 * b_a) / 20.0
        p2 = (40.0 * r_a + 20.0 * g_a + b_a) / 20.0

        atan2 = math.atan2(b, a)
        atan_degrees = atan2 * 180.0 / math.pi
        hue = sanitize_degrees_double(atan_degrees)
        hue_radians = hue * math.pi / 180.0

        ac = p2 * vc.nbb
        j = 100.0 * math.pow(ac / vc.aw, vc.c * vc.z)
        q = (4.0 / vc.c) * math.sqrt(j / 100.0) * (vc.aw + 4.0) * vc.fl_root

        hue_prime = hue + 360.0 if hue < 20.14 else hue
        e_hue = 0.25 * (math.cos(hue_prime * math.pi / 180.0 + 2.0) + 3.8)
        p1 = 50000.0 / 13.0 * e_hue * vc.nc * vc.ncb
        t = p1 * math.sqrt(a * a + b * b) / (u + 0.305)
        alpha = math.pow(t, 0.9) * math.pow(1.64 - math.pow(0.29, vc.n), 0.73)
        c = alpha * math.sqrt(j / 100.0)
        m = c * vc.fl_root
        s = 50.0 * math.sqrt((alpha * vc.c) / (vc.aw + 4.0))

        jstar = (1.0 + 100.0 * 0.007) * j / (1.0 + 0.007 * j)
        mstar = 1.0 / 0.0228 * math.log(1.0 + 0.0228 * m)
        astar = mstar * math.cos(hue_radians)
        bstar = mstar * math.sin(hue_radians)
        return Cam16(hue, c, j, q, m, s, jstar, astar, bstar)

    @staticmethod
    def from_jch(j, c, h):
        return Cam16.from_jch_in_viewing_conditions(j, c, h, VIEWING_CONDITIONS_SRGB)

    @staticmethod
    def from_jch_in_viewing_conditions(j, c, h, vc):
        q = (4.0 / vc.c) * math.sqrt(j / 100.0) * (vc.aw + 4.0) * vc.fl_root
        m = c * vc.fl_root
        alpha = c / math.sqrt(j / 100.0) if j > 0 else 0.0
        s = 50.0 * math.sqrt((alpha * vc.c) / (vc.aw + 4.0))
        hue_radians = h * math.pi / 180.0
        jstar = (1.0 + 100.0 * 0.007) * j / (1.0 + 0.007 * j)
        mstar = 1.0 / 0.0228 * math.log(1.0 + 0.0228 * m)
        astar = mstar * math.cos(hue_radians)
        bstar = mstar * math.sin(hue_radians)
        return Cam16(h, c, j, q, m, s, jstar, astar, bstar)

    def viewed(self, vc):
        alpha = 0.0 if (self.chroma == 0.0 or self.j == 0.0) else self.chroma / math.sqrt(self.j / 100.0)
        t = math.pow(alpha / math.pow(1.64 - math.pow(0.29, vc.n), 0.73), 1.0 / 0.9)
        h_rad = self.hue * math.pi / 180.0
        e_hue = 0.25 * (math.cos(h_rad + 2.0) + 3.8)
        ac = vc.aw * math.pow(self.j / 100.0, 1.0 / vc.c / vc.z)
        p1 = e_hue * (50000.0 / 13.0) * vc.nc * vc.ncb
        p2 = ac / vc.nbb

        h_sin = math.sin(h_rad)
        h_cos = math.cos(h_rad)

        gamma = 23.0 * (p2 + 0.305) * t / (23.0 * p1 + 11.0 * t * h_cos + 108.0 * t * h_sin)
        a = gamma * h_cos
        b = gamma * h_sin
        r_a = (460.0 * p2 + 451.0 * a + 288.0 * b) / 1403.0
        g_a = (460.0 * p2 - 891.0 * a - 261.0 * b) / 1403.0
        b_a = (460.0 * p2 - 220.0 * a - 6300.0 * b) / 1403.0

        r_c_base = max(0.0, (27.13 * abs(r_a)) / (400.0 - abs(r_a)))
        r_c = math.copysign((100.0 / vc.fl) * math.pow(r_c_base, 1.0 / 0.42), r_a)
        g_c_base = max(0.0, (27.13 * abs(g_a)) / (400.0 - abs(g_a)))
        g_c = math.copysign((100.0 / vc.fl) * math.pow(g_c_base, 1.0 / 0.42), g_a)
        b_c_base = max(0.0, (27.13 * abs(b_a)) / (400.0 - abs(b_a)))
        b_c = math.copysign((100.0 / vc.fl) * math.pow(b_c_base, 1.0 / 0.42), b_a)

        r_f = r_c / vc.rgb_d[0]
        g_f = g_c / vc.rgb_d[1]
        b_f = b_c / vc.rgb_d[2]

        x = 1.86206786 * r_f - 1.01125463 * g_f + 0.14918677 * b_f
        y = 0.38752654 * r_f + 0.62144744 * g_f - 0.00897398 * b_f
        z = -0.01584150 * r_f - 0.03412294 * g_f + 1.04996444 * b_f

        return argb_from_xyz(x, y, z)


# ---------------------------------------------------------------------------
# HCT
# ---------------------------------------------------------------------------

def _find_cam_by_j(hue, chroma, tone):
    low = 0.0
    high = 100.0
    best_dl = 1000.0
    best_de = 1000.0
    best_cam = None
    while abs(low - high) > 0.01:
        mid = low + (high - low) / 2.0
        cam_before_clip = Cam16.from_jch(mid, chroma, hue)
        clipped = cam_before_clip.viewed(VIEWING_CONDITIONS_SRGB)
        clipped_lstar = lstar_from_argb(clipped)
        d_l = abs(tone - clipped_lstar)
        if d_l < 0.2:
            cam_clipped = Cam16.from_int(clipped)
            d_e = cam_clipped.distance(Cam16.from_jch(cam_clipped.j, cam_clipped.chroma, hue))
            if d_e <= 1.0 and d_e <= best_de:
                best_dl = d_l
                best_de = d_e
                best_cam = cam_clipped
        if best_dl == 0.0 and best_de == 0.0:
            break
        if clipped_lstar < tone:
            low = mid
        else:
            high = mid
    return best_cam


def solve_to_argb(hue, chroma, tone):
    """Return the sRGB colour closest to the requested HCT coordinates."""
    if chroma < 1.0 or round(tone) <= 0 or round(tone) >= 100:
        return argb_from_lstar(tone)

    hue = sanitize_degrees_double(hue)
    high = chroma
    low = 0.0
    answer = _find_cam_by_j(hue, chroma, tone)
    if answer is not None:
        return answer.viewed(VIEWING_CONDITIONS_SRGB)

    while high - low > 0.4:
        mid = low + (high - low) / 2.0
        candidate = _find_cam_by_j(hue, mid, tone)
        if candidate is None:
            high = mid
        else:
            low = mid
            answer = candidate

    if answer is None:
        return argb_from_lstar(tone)
    return answer.viewed(VIEWING_CONDITIONS_SRGB)


class Hct:
    def __init__(self, hue, chroma, tone):
        self.hue = hue
        self.chroma = chroma
        self.tone = tone

    @staticmethod
    def from_argb(argb):
        cam = Cam16.from_int(argb)
        return Hct(cam.hue, cam.chroma, lstar_from_argb(argb))

    def to_argb(self):
        return solve_to_argb(self.hue, self.chroma, self.tone)


# ---------------------------------------------------------------------------
# Tonal palettes and schemes
# ---------------------------------------------------------------------------

class TonalPalette:
    def __init__(self, hue, chroma):
        self.hue = hue
        self.chroma = chroma
        self._cache = {}

    @staticmethod
    def from_argb(argb):
        hct = Hct.from_argb(argb)
        return TonalPalette(hct.hue, hct.chroma)

    def tone(self, tone):
        tone = clamp_double(0.0, 100.0, float(tone))
        key = round(tone * 100.0)
        if key not in self._cache:
            self._cache[key] = solve_to_argb(self.hue, self.chroma, tone)
        return self._cache[key]

    def hex(self, tone):
        return hex_from_argb(self.tone(tone))


VARIANTS = ("tonal_spot", "vibrant", "expressive", "neutral", "monochrome", "fidelity")


def _rotate(hue, rotation):
    return sanitize_degrees_double(hue + rotation)


def build_palettes(seed_argb, variant):
    """Build the five key tonal palettes for a Material 3 scheme variant."""
    source = Hct.from_argb(seed_argb)
    h = source.hue
    c = source.chroma

    if variant == "tonal_spot":
        return {
            "primary": TonalPalette(h, 36.0),
            "secondary": TonalPalette(h, 16.0),
            "tertiary": TonalPalette(_rotate(h, 60.0), 24.0),
            "neutral": TonalPalette(h, 6.0),
            "neutral_variant": TonalPalette(h, 8.0),
        }
    if variant == "vibrant":
        return {
            "primary": TonalPalette(h, 200.0),
            "secondary": TonalPalette(_rotate(h, 15.0), 24.0),
            "tertiary": TonalPalette(_rotate(h, 45.0), 32.0),
            "neutral": TonalPalette(h, 10.0),
            "neutral_variant": TonalPalette(h, 12.0),
        }
    if variant == "expressive":
        return {
            "primary": TonalPalette(_rotate(h, 240.0), 40.0),
            "secondary": TonalPalette(_rotate(h, 300.0), 24.0),
            "tertiary": TonalPalette(_rotate(h, 30.0), 32.0),
            "neutral": TonalPalette(_rotate(h, 15.0), 8.0),
            "neutral_variant": TonalPalette(_rotate(h, 15.0), 12.0),
        }
    if variant == "neutral":
        return {
            "primary": TonalPalette(h, 12.0),
            "secondary": TonalPalette(h, 8.0),
            "tertiary": TonalPalette(h, 16.0),
            "neutral": TonalPalette(h, 2.0),
            "neutral_variant": TonalPalette(h, 2.0),
        }
    if variant == "monochrome":
        return {
            "primary": TonalPalette(h, 0.0),
            "secondary": TonalPalette(h, 0.0),
            "tertiary": TonalPalette(h, 0.0),
            "neutral": TonalPalette(h, 0.0),
            "neutral_variant": TonalPalette(h, 0.0),
        }
    if variant == "fidelity":
        return {
            "primary": TonalPalette(h, c),
            "secondary": TonalPalette(h, max(c - 32.0, c * 0.5)),
            "tertiary": TonalPalette(_rotate(h, 60.0), c * 0.75),
            "neutral": TonalPalette(h, c / 8.0),
            "neutral_variant": TonalPalette(h, c / 8.0 + 4.0),
        }
    raise ValueError("unknown scheme variant: %s" % variant)


ERROR_PALETTE = TonalPalette(25.0, 84.0)


# Tone tables. Each entry maps a role to the tone used in the light scheme, the
# dark scheme, the high contrast white scheme and the high contrast black scheme.
# Palette key: p = primary, s = secondary, t = tertiary, e = error,
#              n = neutral, nv = neutral variant.
ROLE_TONES = [
    # role                       palette  light dark hcWhite hcBlack
    ("primary",                  "p",     40,   80,   25,     95),
    ("on_primary",               "p",     100,  20,   100,    10),
    ("primary_container",        "p",     90,   30,   80,     20),
    ("on_primary_container",     "p",     10,   90,   0,      100),
    ("inverse_primary",          "p",     80,   40,   90,     30),
    ("primary_fixed",            "p",     90,   90,   80,     90),
    ("primary_fixed_dim",        "p",     80,   80,   70,     80),
    ("on_primary_fixed",         "p",     10,   10,   0,      10),
    ("on_primary_fixed_variant", "p",     30,   30,   20,     30),

    ("secondary",                "s",     40,   80,   25,     95),
    ("on_secondary",             "s",     100,  20,   100,    10),
    ("secondary_container",      "s",     90,   30,   80,     20),
    ("on_secondary_container",   "s",     10,   90,   0,      100),
    ("secondary_fixed",          "s",     90,   90,   80,     90),
    ("secondary_fixed_dim",      "s",     80,   80,   70,     80),
    ("on_secondary_fixed",       "s",     10,   10,   0,      10),
    ("on_secondary_fixed_variant", "s",   30,   30,   20,     30),

    ("tertiary",                 "t",     40,   80,   25,     95),
    ("on_tertiary",              "t",     100,  20,   100,    10),
    ("tertiary_container",       "t",     90,   30,   80,     20),
    ("on_tertiary_container",    "t",     10,   90,   0,      100),
    ("tertiary_fixed",           "t",     90,   90,   80,     90),
    ("tertiary_fixed_dim",       "t",     80,   80,   70,     80),
    ("on_tertiary_fixed",        "t",     10,   10,   0,      10),
    ("on_tertiary_fixed_variant", "t",    30,   30,   20,     30),

    ("error",                    "e",     40,   80,   25,     95),
    ("on_error",                 "e",     100,  20,   100,    10),
    ("error_container",          "e",     90,   30,   80,     20),
    ("on_error_container",       "e",     10,   90,   0,      100),

    ("background",               "n",     98,   6,    100,    0),
    ("on_background",            "n",     10,   90,   0,      100),
    ("surface",                  "n",     98,   6,    100,    0),
    ("surface_dim",              "n",     87,   6,    90,     0),
    ("surface_bright",           "n",     98,   24,   100,    30),
    ("surface_container_lowest", "n",     100,  4,    100,    0),
    ("surface_container_low",    "n",     96,   10,   98,     4),
    ("surface_container",        "n",     94,   12,   96,     8),
    ("surface_container_high",   "n",     92,   17,   94,     12),
    ("surface_container_highest", "n",    90,   22,   92,     17),
    ("on_surface",               "n",     10,   90,   0,      100),
    ("surface_variant",          "nv",    90,   30,   92,     20),
    ("on_surface_variant",       "nv",    30,   80,   10,     95),
    ("outline",                  "nv",    50,   60,   25,     85),
    ("outline_variant",          "nv",    80,   30,   50,     60),
    ("inverse_surface",          "n",     20,   90,   10,     100),
    ("inverse_on_surface",       "n",     95,   20,   100,    0),
    ("surface_tint",             "p",     40,   80,   25,     95),
    ("scrim",                    "n",     0,    0,    0,      0),
    ("shadow",                   "n",     0,    0,    0,      0),
]

PALETTE_KEYS = {"p": "primary", "s": "secondary", "t": "tertiary",
                "n": "neutral", "nv": "neutral_variant"}

SCHEME_INDEX = {"light": 0, "dark": 1, "high_contrast_white": 2, "high_contrast_black": 3}


def build_scheme(seed_argb, variant, scheme):
    """Return a dict of Material 3 role name to "#RRGGBB" string."""
    palettes = build_palettes(seed_argb, variant)
    index = SCHEME_INDEX[scheme]
    out = {}
    for role, palette_key, *tones in ROLE_TONES:
        tone = tones[index]
        if palette_key == "e":
            palette = ERROR_PALETTE
        else:
            palette = palettes[PALETTE_KEYS[palette_key]]
        out[role] = hex_from_argb(palette.tone(tone))
    return out


# ---------------------------------------------------------------------------
# Contrast helpers
# ---------------------------------------------------------------------------

def relative_luminance(argb):
    r = linearized(red_from_argb(argb)) / 100.0
    g = linearized(green_from_argb(argb)) / 100.0
    b = linearized(blue_from_argb(argb)) / 100.0
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast_ratio(argb_a, argb_b):
    l1 = relative_luminance(argb_a)
    l2 = relative_luminance(argb_b)
    if l1 < l2:
        l1, l2 = l2, l1
    return (l1 + 0.05) / (l2 + 0.05)
