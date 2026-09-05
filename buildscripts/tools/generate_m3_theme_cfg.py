#!/usr/bin/env python3
"""
Regenerate Audacity's four theme configuration files from a Material 3 seed.

The muse framework reads ":/configs/<code>.cfg" as a flat JSON object. The
first block of snake_case keys maps onto the muse ThemeStyleKey enum and is
surfaced in QML as ui.theme.<camelCaseName>. Every other key lands in
ui.theme.extra["<key>"].

This script derives the mapped keys from Material 3 colour roles built out of a
single seed colour, keeps the functional data colours (clip colours, meters,
rulers, dynamics and so on) that carry meaning rather than brand, raises their
contrast in the two high contrast themes, and appends an "m3_<role>" key for
every Material 3 colour role so that QML outside this module can read the same
tokens through ui.theme.extra.

Usage:
    python3 buildscripts/tools/generate_m3_theme_cfg.py
    python3 buildscripts/tools/generate_m3_theme_cfg.py --seed "#926BFF" --variant tonal_spot
    python3 buildscripts/tools/generate_m3_theme_cfg.py --check

The colour maths lives in m3_hct.py next to this file and is mirrored value for
value by src/uicomponents/components/m3hct.cpp.
"""

import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import m3_hct as hct  # noqa: E402

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
CONFIG_DIR = os.path.join(REPO_ROOT, "src", "app", "configs")

DEFAULT_SEED = "#926BFF"
DEFAULT_VARIANT = "tonal_spot"

SCHEMES = [
    ("light.cfg", "light"),
    ("dark.cfg", "dark"),
    ("high_contrast_white.cfg", "high_contrast_white"),
    ("high_contrast_black.cfg", "high_contrast_black"),
]

# muse ThemeStyleKey mapped keys, expressed as Material 3 roles.
MAPPED_ROLES = {
    "background_primary_color": "surface",
    "background_secondary_color": "surface_container",
    "background_tertiary_color": "surface_container_high",
    "background_quarternary_color": "inverse_surface",
    "popup_background_color": "surface_container_high",
    "project_tab_color": "secondary_container",
    "text_field_color": "surface_container_highest",
    "accent_color": "primary",
    "stroke_color": "outline_variant",
    "stroke_secondary_color": "outline",
    "button_color": "secondary_container",
    "font_primary_color": "on_surface",
    "font_secondary_color": "on_secondary_container",
    "link_color": "primary",
    "focus_color": "primary",
    "error_text_color": "error",
}

# Functional transport colours keep their hue but take a scheme aware tone so
# that they stay legible on every background.
FUNCTIONAL_HUES = {
    # key: (source colour, light tone, dark tone, hc white tone, hc black tone)
    "play_color": ("#18A999", 38, 72, 28, 82),
    "record_color": ("#EF476F", 45, 75, 32, 85),
}

# Colours that act as a surface behind other content. In the high contrast
# themes they are pushed to the extreme instead of being contrast boosted.
HC_SURFACE_SUFFIXES = (
    "_background_color",
    "_overlay_color",
    "_fill_color",
    "_fill_semitransparent_color",
)

HC_SURFACE_KEYS = {
    "track_header_color",
    "track_header_hover_color",
    "track_header_active_color",
    "classic_clip_background_color",
    "classic_clip_header_color",
    "classic_clip_header_hover_color",
    "meter_background_color",
    "save_option_background_color",
    "dynamics_background_color",
    "selection_highlight_color",
    "play_region_active_color",
    "play_region_inactive_color",
}

# Keys that are pure black or pure white on purpose and must not move.
FROZEN_KEYS = {"white_color", "black_color"}

HC_MIN_CONTRAST = 4.5


def split_alpha(value):
    """Return ("#RRGGBB", "AA" or "") for a 6 or 8 digit hex string."""
    text = value.strip()
    if not text.startswith("#"):
        return None, ""
    digits = text[1:]
    if len(digits) == 8:
        return "#" + digits[0:6], digits[6:8]
    if len(digits) == 6:
        return "#" + digits, ""
    if len(digits) == 3:
        return "#" + "".join(ch * 2 for ch in digits), ""
    return None, ""


def is_colour(value):
    return isinstance(value, str) and split_alpha(value)[0] is not None


def retone(hex_colour, tone, chroma_boost=1.0):
    argb = hct.argb_from_hex(hex_colour)
    colour = hct.Hct.from_argb(argb)
    chroma = min(120.0, colour.chroma * chroma_boost)
    return hct.hex_from_argb(hct.solve_to_argb(colour.hue, chroma, tone))


def boost_contrast(hex_colour, background_hex, minimum=HC_MIN_CONTRAST):
    """Move a colour's tone until it reaches the requested contrast ratio."""
    background = hct.argb_from_hex(background_hex)
    source = hct.Hct.from_argb(hct.argb_from_hex(hex_colour))
    background_light = hct.lstar_from_argb(background) > 50.0
    chroma = min(120.0, source.chroma * 1.15)

    tones = range(int(round(source.tone)), -1, -1) if background_light \
        else range(int(round(source.tone)), 101)

    best = hex_colour
    for tone in tones:
        candidate = hct.solve_to_argb(source.hue, chroma, float(tone))
        best = hct.hex_from_argb(candidate)
        if hct.contrast_ratio(candidate, background) >= minimum:
            return best
    return best


def high_contrast_functional(key, value, scheme):
    """Adjust one functional data colour for a high contrast scheme."""
    base, alpha = split_alpha(value)
    if base is None:
        return value

    white = scheme == "high_contrast_white"
    background_hex = "#FFFFFF" if white else "#000000"

    surface_like = key in HC_SURFACE_KEYS or key.endswith(HC_SURFACE_SUFFIXES)
    if surface_like:
        source = hct.Hct.from_argb(hct.argb_from_hex(base))
        tone = 94.0 if white else 8.0
        adjusted = hct.hex_from_argb(hct.solve_to_argb(source.hue, min(source.chroma, 24.0), tone))
    else:
        adjusted = boost_contrast(base, background_hex)

    return adjusted + alpha


def m3_extra_keys(roles):
    return {"m3_" + role: value for role, value in roles.items()}


def build_config(existing, roles, scheme, seed_hex, variant, functional_source=None):
    """Build one theme file.

    functional_source supplies the untouched base value for every functional
    data colour. The high contrast schemes derive from the light and dark
    schemes rather than from their own previous output, so that repeated runs
    of this script are idempotent.
    """
    base_values = functional_source or existing
    out = {}
    for key, value in existing.items():
        value = base_values.get(key, value)
        if key.startswith("m3_"):
            continue
        if key in MAPPED_ROLES:
            out[key] = roles[MAPPED_ROLES[key]]
            continue
        if key in FUNCTIONAL_HUES:
            hue_source, *tones = FUNCTIONAL_HUES[key]
            tone = tones[hct.SCHEME_INDEX[scheme]]
            out[key] = retone(hue_source, float(tone), chroma_boost=1.0)
            continue
        if key in FROZEN_KEYS or not is_colour(value):
            out[key] = value
            continue
        if scheme in ("high_contrast_white", "high_contrast_black"):
            out[key] = high_contrast_functional(key, value, scheme)
        else:
            out[key] = value

    out.update(m3_extra_keys(roles))
    out["m3_seed_color"] = seed_hex.upper()
    out["m3_variant"] = variant
    out["m3_state_layer_hover_opacity"] = 0.08
    out["m3_state_layer_focus_opacity"] = 0.10
    out["m3_state_layer_pressed_opacity"] = 0.10
    out["m3_state_layer_dragged_opacity"] = 0.16
    out["m3_state_layer_disabled_content_opacity"] = 0.38
    out["m3_state_layer_disabled_container_opacity"] = 0.12
    return out


def serialise(config):
    lines = ["{"]
    items = list(config.items())
    for index, (key, value) in enumerate(items):
        if isinstance(value, str):
            rendered = json.dumps(value)
        elif isinstance(value, bool):
            rendered = "true" if value else "false"
        elif isinstance(value, float):
            rendered = ("%g" % value) if value == int(value) else repr(round(value, 6))
        else:
            rendered = json.dumps(value)
        comma = "," if index < len(items) - 1 else ""
        lines.append('    "%s": %s%s' % (key, rendered, comma))
    lines.append("}")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--seed", default=DEFAULT_SEED, help="seed colour, for example #926BFF")
    parser.add_argument("--variant", default=DEFAULT_VARIANT, choices=hct.VARIANTS)
    parser.add_argument("--config-dir", default=CONFIG_DIR)
    parser.add_argument("--check", action="store_true",
                        help="verify the files on disk match the generated output")
    args = parser.parse_args()

    seed_argb = hct.argb_from_hex(args.seed)
    failures = []

    loaded = {}
    for filename, scheme in SCHEMES:
        path = os.path.join(args.config_dir, filename)
        with open(path, "r", encoding="utf-8") as handle:
            loaded[scheme] = json.load(handle)

    functional_sources = {
        "light": None,
        "dark": None,
        "high_contrast_white": loaded["light"],
        "high_contrast_black": loaded["dark"],
    }

    for filename, scheme in SCHEMES:
        path = os.path.join(args.config_dir, filename)
        existing = loaded[scheme]

        roles = hct.build_scheme(seed_argb, args.variant, scheme)
        config = build_config(existing, roles, scheme, args.seed, args.variant,
                              functional_sources[scheme])
        text = serialise(config)

        if args.check:
            with open(path, "r", encoding="utf-8") as handle:
                if handle.read() != text:
                    failures.append(filename)
        else:
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(text)
            print("wrote %s" % path)

    if args.check:
        if failures:
            print("out of date: %s" % ", ".join(failures))
            return 1
        print("all theme files are up to date")
    return 0


if __name__ == "__main__":
    sys.exit(main())
