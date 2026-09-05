/*
* Audacity: A Digital Audio Editor
*
* Colour format translation for M3ColorPicker.
*
* Converts one colour between the named list, HEX, HEX8, RGB, RGBA, HSL, HSLA,
* HSV, HWB, CIELAB, LCH, OKLab, OKLCH and CMYK. Every function works on plain
* numbers so that the picker itself stays free of colour maths.
*/
.pragma library

var FORMATS = ["named", "hex", "hex8", "rgb", "rgba", "hsl", "hsla",
               "hsv", "hwb", "lab", "lch", "oklab", "oklch", "cmyk"]

// A small, widely recognised subset of the CSS named colours.
var NAMED = {
    "black": [0, 0, 0], "white": [255, 255, 255], "red": [255, 0, 0],
    "green": [0, 128, 0], "lime": [0, 255, 0], "blue": [0, 0, 255],
    "yellow": [255, 255, 0], "cyan": [0, 255, 255], "magenta": [255, 0, 255],
    "silver": [192, 192, 192], "gray": [128, 128, 128], "maroon": [128, 0, 0],
    "olive": [128, 128, 0], "purple": [128, 0, 128], "teal": [0, 128, 128],
    "navy": [0, 0, 128], "orange": [255, 165, 0], "pink": [255, 192, 203],
    "brown": [165, 42, 42], "gold": [255, 215, 0], "indigo": [75, 0, 130],
    "violet": [238, 130, 238], "coral": [255, 127, 80], "salmon": [250, 128, 114]
}

function clamp(value, low, high) {
    return Math.max(low, Math.min(high, value))
}

function round(value, digits) {
    var factor = Math.pow(10, digits === undefined ? 0 : digits)
    return Math.round(value * factor) / factor
}

function pad2(value) {
    var text = Math.round(value).toString(16).toUpperCase()
    return text.length < 2 ? "0" + text : text
}

// --- RGB and HSV / HSL / HWB -----------------------------------------------

function rgbToHsv(r, g, b) {
    r /= 255; g /= 255; b /= 255
    var max = Math.max(r, g, b)
    var min = Math.min(r, g, b)
    var delta = max - min
    var h = 0
    if (delta !== 0) {
        if (max === r) {
            h = 60 * (((g - b) / delta) % 6)
        } else if (max === g) {
            h = 60 * ((b - r) / delta + 2)
        } else {
            h = 60 * ((r - g) / delta + 4)
        }
    }
    if (h < 0) {
        h += 360
    }
    return [h, max === 0 ? 0 : delta / max, max]
}

function hsvToRgb(h, s, v) {
    h = ((h % 360) + 360) % 360
    var c = v * s
    var x = c * (1 - Math.abs(((h / 60) % 2) - 1))
    var m = v - c
    var rgb
    if (h < 60) {
        rgb = [c, x, 0]
    } else if (h < 120) {
        rgb = [x, c, 0]
    } else if (h < 180) {
        rgb = [0, c, x]
    } else if (h < 240) {
        rgb = [0, x, c]
    } else if (h < 300) {
        rgb = [x, 0, c]
    } else {
        rgb = [c, 0, x]
    }
    return [(rgb[0] + m) * 255, (rgb[1] + m) * 255, (rgb[2] + m) * 255]
}

function rgbToHsl(r, g, b) {
    r /= 255; g /= 255; b /= 255
    var max = Math.max(r, g, b)
    var min = Math.min(r, g, b)
    var l = (max + min) / 2
    var delta = max - min
    var h = 0
    var s = 0
    if (delta !== 0) {
        s = delta / (1 - Math.abs(2 * l - 1))
        if (max === r) {
            h = 60 * (((g - b) / delta) % 6)
        } else if (max === g) {
            h = 60 * ((b - r) / delta + 2)
        } else {
            h = 60 * ((r - g) / delta + 4)
        }
    }
    if (h < 0) {
        h += 360
    }
    return [h, s, l]
}

function rgbToHwb(r, g, b) {
    var hsv = rgbToHsv(r, g, b)
    var white = Math.min(r, g, b) / 255
    var black = 1 - Math.max(r, g, b) / 255
    return [hsv[0], white, black]
}

// --- sRGB, XYZ, CIELAB, LCH -------------------------------------------------

function srgbToLinear(value) {
    value /= 255
    return value <= 0.04045 ? value / 12.92 : Math.pow((value + 0.055) / 1.055, 2.4)
}

function linearToSrgb(value) {
    var out = value <= 0.0031308 ? value * 12.92 : 1.055 * Math.pow(value, 1 / 2.4) - 0.055
    return out * 255
}

function rgbToXyz(r, g, b) {
    var rl = srgbToLinear(r)
    var gl = srgbToLinear(g)
    var bl = srgbToLinear(b)
    return [
        (0.4124564 * rl + 0.3575761 * gl + 0.1804375 * bl) * 100,
        (0.2126729 * rl + 0.7151522 * gl + 0.0721750 * bl) * 100,
        (0.0193339 * rl + 0.1191920 * gl + 0.9503041 * bl) * 100
    ]
}

function xyzToRgb(x, y, z) {
    x /= 100; y /= 100; z /= 100
    return [
        linearToSrgb(3.2404542 * x - 1.5371385 * y - 0.4985314 * z),
        linearToSrgb(-0.9692660 * x + 1.8760108 * y + 0.0415560 * z),
        linearToSrgb(0.0556434 * x - 0.2040259 * y + 1.0572252 * z)
    ]
}

var WHITE_D65 = [95.047, 100.0, 108.883]

function xyzToLab(x, y, z) {
    function f(t) {
        return t > 0.008856 ? Math.pow(t, 1 / 3) : (7.787 * t + 16 / 116)
    }
    var fx = f(x / WHITE_D65[0])
    var fy = f(y / WHITE_D65[1])
    var fz = f(z / WHITE_D65[2])
    return [116 * fy - 16, 500 * (fx - fy), 200 * (fy - fz)]
}

function labToXyz(l, a, b) {
    function fInv(t) {
        var t3 = t * t * t
        return t3 > 0.008856 ? t3 : (t - 16 / 116) / 7.787
    }
    var fy = (l + 16) / 116
    var fx = fy + a / 500
    var fz = fy - b / 200
    return [fInv(fx) * WHITE_D65[0], fInv(fy) * WHITE_D65[1], fInv(fz) * WHITE_D65[2]]
}

function rgbToLab(r, g, b) {
    var xyz = rgbToXyz(r, g, b)
    return xyzToLab(xyz[0], xyz[1], xyz[2])
}

function labToRgb(l, a, b) {
    var xyz = labToXyz(l, a, b)
    return xyzToRgb(xyz[0], xyz[1], xyz[2])
}

function labToLch(l, a, b) {
    var c = Math.sqrt(a * a + b * b)
    var h = Math.atan2(b, a) * 180 / Math.PI
    if (h < 0) {
        h += 360
    }
    return [l, c, h]
}

function lchToLab(l, c, h) {
    var rad = h * Math.PI / 180
    return [l, c * Math.cos(rad), c * Math.sin(rad)]
}

// --- OKLab and OKLCH --------------------------------------------------------

function rgbToOklab(r, g, b) {
    var rl = srgbToLinear(r)
    var gl = srgbToLinear(g)
    var bl = srgbToLinear(b)

    var l = Math.cbrt(0.4122214708 * rl + 0.5363325363 * gl + 0.0514459929 * bl)
    var m = Math.cbrt(0.2119034982 * rl + 0.6806995451 * gl + 0.1073969566 * bl)
    var s = Math.cbrt(0.0883024619 * rl + 0.2817188376 * gl + 0.6299787005 * bl)

    return [
        0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
        1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
        0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s
    ]
}

function oklabToRgb(okL, okA, okB) {
    var l = okL + 0.3963377774 * okA + 0.2158037573 * okB
    var m = okL - 0.1055613458 * okA - 0.0638541728 * okB
    var s = okL - 0.0894841775 * okA - 1.2914855480 * okB

    l = l * l * l
    m = m * m * m
    s = s * s * s

    return [
        linearToSrgb(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s),
        linearToSrgb(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s),
        linearToSrgb(-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s)
    ]
}

function oklabToOklch(l, a, b) {
    var c = Math.sqrt(a * a + b * b)
    var h = Math.atan2(b, a) * 180 / Math.PI
    if (h < 0) {
        h += 360
    }
    return [l, c, h]
}

function oklchToOklab(l, c, h) {
    var rad = h * Math.PI / 180
    return [l, c * Math.cos(rad), c * Math.sin(rad)]
}

// --- CMYK -------------------------------------------------------------------

function rgbToCmyk(r, g, b) {
    r /= 255; g /= 255; b /= 255
    var k = 1 - Math.max(r, g, b)
    if (k >= 1) {
        return [0, 0, 0, 1]
    }
    return [(1 - r - k) / (1 - k), (1 - g - k) / (1 - k), (1 - b - k) / (1 - k), k]
}

function cmykToRgb(c, m, y, k) {
    return [255 * (1 - c) * (1 - k), 255 * (1 - m) * (1 - k), 255 * (1 - y) * (1 - k)]
}

// --- Formatting -------------------------------------------------------------

function nearestName(r, g, b) {
    var best = null
    var bestDistance = Infinity
    for (var name in NAMED) {
        var value = NAMED[name]
        var dr = value[0] - r
        var dg = value[1] - g
        var db = value[2] - b
        var distance = dr * dr + dg * dg + db * db
        if (distance < bestDistance) {
            bestDistance = distance
            best = name
        }
    }
    return { "name": best, "exact": bestDistance === 0 }
}

/*
 * Render one colour in the requested format.
 * rgb is [r, g, b] in 0 to 255, alpha is 0 to 1.
 */
function format(formatName, r, g, b, alpha) {
    switch (formatName) {
    case "named": {
        var match = nearestName(r, g, b)
        return match.exact ? match.name : "near " + match.name
    }
    case "hex":
        return "#" + pad2(r) + pad2(g) + pad2(b)
    case "hex8":
        return "#" + pad2(r) + pad2(g) + pad2(b) + pad2(alpha * 255)
    case "rgb":
        return "rgb(" + Math.round(r) + ", " + Math.round(g) + ", " + Math.round(b) + ")"
    case "rgba":
        return "rgba(" + Math.round(r) + ", " + Math.round(g) + ", " + Math.round(b)
                + ", " + round(alpha, 3) + ")"
    case "hsl": {
        var hsl = rgbToHsl(r, g, b)
        return "hsl(" + round(hsl[0], 1) + ", " + round(hsl[1] * 100, 1) + "%, "
                + round(hsl[2] * 100, 1) + "%)"
    }
    case "hsla": {
        var hsla = rgbToHsl(r, g, b)
        return "hsla(" + round(hsla[0], 1) + ", " + round(hsla[1] * 100, 1) + "%, "
                + round(hsla[2] * 100, 1) + "%, " + round(alpha, 3) + ")"
    }
    case "hsv": {
        var hsv = rgbToHsv(r, g, b)
        return "hsv(" + round(hsv[0], 1) + ", " + round(hsv[1] * 100, 1) + "%, "
                + round(hsv[2] * 100, 1) + "%)"
    }
    case "hwb": {
        var hwb = rgbToHwb(r, g, b)
        return "hwb(" + round(hwb[0], 1) + " " + round(hwb[1] * 100, 1) + "% "
                + round(hwb[2] * 100, 1) + "%)"
    }
    case "lab": {
        var lab = rgbToLab(r, g, b)
        return "lab(" + round(lab[0], 2) + "% " + round(lab[1], 2) + " " + round(lab[2], 2) + ")"
    }
    case "lch": {
        var lch = labToLch.apply(null, rgbToLab(r, g, b))
        return "lch(" + round(lch[0], 2) + "% " + round(lch[1], 2) + " " + round(lch[2], 2) + ")"
    }
    case "oklab": {
        var oklab = rgbToOklab(r, g, b)
        return "oklab(" + round(oklab[0], 4) + " " + round(oklab[1], 4) + " "
                + round(oklab[2], 4) + ")"
    }
    case "oklch": {
        var oklch = oklabToOklch.apply(null, rgbToOklab(r, g, b))
        return "oklch(" + round(oklch[0], 4) + " " + round(oklch[1], 4) + " "
                + round(oklch[2], 2) + ")"
    }
    case "cmyk": {
        var cmyk = rgbToCmyk(r, g, b)
        return "cmyk(" + round(cmyk[0] * 100, 1) + "%, " + round(cmyk[1] * 100, 1) + "%, "
                + round(cmyk[2] * 100, 1) + "%, " + round(cmyk[3] * 100, 1) + "%)"
    }
    }
    return "#" + pad2(r) + pad2(g) + pad2(b)
}

function numbersIn(text) {
    var matches = text.match(/-?\d+(\.\d+)?/g)
    if (!matches) {
        return []
    }
    var out = []
    for (var i = 0; i < matches.length; ++i) {
        out.push(parseFloat(matches[i]))
    }
    return out
}

/*
 * Parse a colour written in any supported format.
 * Returns { r, g, b, alpha, clipped } or null when the text is not a colour.
 * "clipped" is true when the requested colour lies outside the sRGB gamut and
 * has been clamped to fit.
 */
function parse(text) {
    if (!text) {
        return null
    }
    var value = text.trim().toLowerCase()

    if (NAMED[value] !== undefined) {
        var named = NAMED[value]
        return { "r": named[0], "g": named[1], "b": named[2], "alpha": 1.0, "clipped": false }
    }

    if (value.charAt(0) === "#") {
        var digits = value.substring(1)
        if (digits.length === 3) {
            digits = digits.charAt(0) + digits.charAt(0) + digits.charAt(1)
                    + digits.charAt(1) + digits.charAt(2) + digits.charAt(2)
        }
        if (digits.length === 6 || digits.length === 8) {
            return {
                "r": parseInt(digits.substring(0, 2), 16),
                "g": parseInt(digits.substring(2, 4), 16),
                "b": parseInt(digits.substring(4, 6), 16),
                "alpha": digits.length === 8 ? parseInt(digits.substring(6, 8), 16) / 255 : 1.0,
                "clipped": false
            }
        }
        return null
    }

    var numbers = numbersIn(value)
    if (numbers.length < 3) {
        return null
    }

    var rgb = null
    var alpha = 1.0

    if (value.indexOf("rgb") === 0) {
        rgb = [numbers[0], numbers[1], numbers[2]]
        alpha = numbers.length > 3 ? numbers[3] : 1.0
    } else if (value.indexOf("hsla") === 0 || value.indexOf("hsl") === 0) {
        var l = numbers[2] / 100
        var s = numbers[1] / 100
        var c = (1 - Math.abs(2 * l - 1)) * s
        var v = l + c / 2
        rgb = hsvToRgb(numbers[0], v === 0 ? 0 : c / v, v)
        alpha = numbers.length > 3 ? numbers[3] : 1.0
    } else if (value.indexOf("hsv") === 0) {
        rgb = hsvToRgb(numbers[0], numbers[1] / 100, numbers[2] / 100)
    } else if (value.indexOf("hwb") === 0) {
        var white = numbers[1] / 100
        var black = numbers[2] / 100
        rgb = hsvToRgb(numbers[0], 1 - white / (1 - black || 1), 1 - black)
    } else if (value.indexOf("oklch") === 0) {
        rgb = oklabToRgb.apply(null, oklchToOklab(numbers[0], numbers[1], numbers[2]))
    } else if (value.indexOf("oklab") === 0) {
        rgb = oklabToRgb(numbers[0], numbers[1], numbers[2])
    } else if (value.indexOf("lch") === 0) {
        rgb = labToRgb.apply(null, lchToLab(numbers[0], numbers[1], numbers[2]))
    } else if (value.indexOf("lab") === 0) {
        rgb = labToRgb(numbers[0], numbers[1], numbers[2])
    } else if (value.indexOf("cmyk") === 0 && numbers.length >= 4) {
        rgb = cmykToRgb(numbers[0] / 100, numbers[1] / 100, numbers[2] / 100, numbers[3] / 100)
    }

    if (rgb === null) {
        return null
    }

    var clipped = false
    for (var i = 0; i < 3; ++i) {
        if (rgb[i] < -0.5 || rgb[i] > 255.5) {
            clipped = true
        }
        rgb[i] = clamp(rgb[i], 0, 255)
    }

    return { "r": rgb[0], "g": rgb[1], "b": rgb[2], "alpha": clamp(alpha, 0, 1), "clipped": clipped }
}
