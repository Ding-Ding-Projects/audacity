#!/usr/bin/env python3
"""Check coverage/quality of a Qt Linguist (.ts) translation file.

Usage:
    check_translation_coverage.py <file.ts> [--strict]

Prints total messages, translated, unfinished, empty, and placeholder
(%1/%2/.../%n) mismatches between <source> and <translation>. With --strict,
exits non-zero if there is any unfinished, empty, or mismatched message.
"""
import argparse
import re
import sys
import xml.etree.ElementTree as ET

PLACEHOLDER_RE = re.compile(r'%\d+|%n')


def placeholder_counts(text):
    counts = {}
    for m in PLACEHOLDER_RE.findall(text or ''):
        counts[m] = counts.get(m, 0) + 1
    return counts


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('ts_file')
    ap.add_argument('--strict', action='store_true')
    args = ap.parse_args()

    tree = ET.parse(args.ts_file)
    root = tree.getroot()

    total = 0
    translated = 0
    unfinished = 0
    empty = 0
    mismatches = []

    for ctx in root.findall('context'):
        ctx_name_el = ctx.find('name')
        ctx_name = ctx_name_el.text if ctx_name_el is not None else '?'
        for msg in ctx.findall('message'):
            total += 1
            src_el = msg.find('source')
            trans_el = msg.find('translation')
            src_text = src_el.text if src_el is not None else ''
            if trans_el is None:
                empty += 1
                continue

            ttype = trans_el.get('type')
            if ttype == 'unfinished':
                unfinished += 1
                continue
            if ttype == 'vanished':
                # vanished/obsolete messages are not part of active coverage
                total -= 1
                continue

            numerus_forms = trans_el.findall('numerusform')
            if numerus_forms:
                texts = [f.text or '' for f in numerus_forms]
                if any(t.strip() == '' for t in texts):
                    empty += 1
                    continue
                translated += 1
                src_counts = placeholder_counts(src_text)
                for t in texts:
                    if placeholder_counts(t) != src_counts and src_counts:
                        mismatches.append((ctx_name, src_text, t))
                        break
                continue

            trans_text = trans_el.text or ''
            if trans_text.strip() == '':
                empty += 1
                continue

            translated += 1
            src_counts = placeholder_counts(src_text)
            trans_counts = placeholder_counts(trans_text)
            if src_counts != trans_counts:
                mismatches.append((ctx_name, src_text, trans_text))

    print(f'Total messages:      {total}')
    print(f'Translated:          {translated}')
    print(f'Unfinished:          {unfinished}')
    print(f'Empty:               {empty}')
    print(f'Placeholder mismatch:{len(mismatches)}')
    if mismatches:
        print('\nPlaceholder mismatches (context, source, translation):')
        for ctx_name, src_text, trans_text in mismatches[:50]:
            print(f'  [{ctx_name}] {src_text!r} -> {trans_text!r}')
        if len(mismatches) > 50:
            print(f'  ... and {len(mismatches) - 50} more')

    if args.strict and (unfinished or empty or mismatches):
        sys.exit(1)
    sys.exit(0)


if __name__ == '__main__':
    main()
