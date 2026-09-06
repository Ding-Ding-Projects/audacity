#!/usr/bin/env python3
"""Acquire a bounded, provenance-backed Ollama library snapshot.

Only ollama.com/library pages are accepted.  The caller supplies the first
page and a terminal receipt URL.  Each fetched page must name its successor
through a ``rel=next`` link, and the final page must match the declared
terminal URL and have no successor.  A broken chain is a failed acquisition,
never a partial catalog called complete.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
import urllib.parse
import urllib.request
from html.parser import HTMLParser


class Links(HTMLParser):
    def __init__(self):
        super().__init__()
        self.next_url = None
        self.models = set()

    def handle_starttag(self, tag, attrs):
        if tag != "a":
            return
        data = dict(attrs)
        href = data.get("href", "")
        rel = {piece.lower() for piece in data.get("rel", "").split()}
        if "next" in rel:
            self.next_url = href
        parsed = urllib.parse.urlparse(href)
        if parsed.path.startswith("/library/") and parsed.path.count("/") == 2:
            self.models.add(parsed.path.rsplit("/", 1)[1])


def canonical(url: str) -> str:
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme != "https" or parsed.netloc != "ollama.com" or not parsed.path.startswith("/library"):
        raise ValueError("only https://ollama.com/library pages are accepted")
    return urllib.parse.urlunparse((parsed.scheme, parsed.netloc, parsed.path, "", parsed.query, ""))


def fetch(url: str) -> tuple[str, bytes]:
    request = urllib.request.Request(url, headers={"User-Agent": "AudacityCatalogReceipt/1"})
    with urllib.request.urlopen(request, timeout=20) as response:
        final_url = canonical(response.geturl())
        return final_url, response.read(2 * 1024 * 1024 + 1)


def detail_receipt(model: str) -> dict:
    url, payload = fetch(canonical(f"https://ollama.com/library/{model}/tags"))
    if len(payload) > 2 * 1024 * 1024:
        raise ValueError("official tag page exceeded the 2 MiB receipt limit")
    links = Links()
    links.feed(payload.decode("utf-8", errors="strict"))
    prefix = f"/library/{model}:"
    tags = sorted({urllib.parse.urlparse(href).path.rsplit("/", 1)[1]
                   for href in _hrefs(payload) if urllib.parse.urlparse(href).path.startswith(prefix)})
    if links.next_url or not tags:
        raise ValueError(f"tag receipt for {model} is paginated or empty; bounded tool cannot prove completeness")
    return {"name": model, "tags": tags, "tagPage": {"url": url, "sha256": hashlib.sha256(payload).hexdigest()}}


def _hrefs(payload: bytes) -> list[str]:
    class Hrefs(HTMLParser):
        def __init__(self): super().__init__(); self.values = []
        def handle_starttag(self, tag, attrs):
            if tag == "a": self.values.append(dict(attrs).get("href", ""))
    parser = Hrefs(); parser.feed(payload.decode("utf-8", errors="strict")); return parser.values


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--first", required=True)
    parser.add_argument("--terminal", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--max-pages", type=int, default=100)
    args = parser.parse_args()
    if args.max_pages < 1 or args.max_pages > 1000:
        raise SystemExit("--max-pages must be 1..1000")
    current, terminal = canonical(args.first), canonical(args.terminal)
    seen, pages, models = set(), [], set()
    while True:
        if current in seen or len(pages) >= args.max_pages:
            raise SystemExit("pagination loop or configured page limit reached")
        seen.add(current)
        final_url, payload = fetch(current)
        if len(payload) > 2 * 1024 * 1024:
            raise SystemExit("official page exceeded the 2 MiB receipt limit")
        links = Links()
        links.feed(payload.decode("utf-8", errors="strict"))
        next_url = canonical(urllib.parse.urljoin(final_url, links.next_url)) if links.next_url else None
        pages.append({"url": final_url, "sha256": hashlib.sha256(payload).hexdigest(), "modelCount": len(links.models)})
        models.update(links.models)
        if next_url is None:
            if final_url != terminal:
                raise SystemExit("pagination ended before the declared terminal receipt")
            break
        if final_url == terminal:
            raise SystemExit("declared terminal receipt still points to another page")
        current = next_url
    if not models:
        raise SystemExit("no model receipts discovered")
    snapshot = {
        "schemaVersion": 1,
        "origin": "https://ollama.com/library",
        "revision": hashlib.sha256("".join(page["sha256"] for page in pages).encode()).hexdigest(),
        "pageCount": len(pages), "pages": pages,
        "models": [detail_receipt(name) for name in sorted(models)],
        "completeness": "model-and-tag-terminal-verified",
    }
    with open(args.output, "w", encoding="utf-8", newline="\n") as output:
        json.dump(snapshot, output, indent=2, sort_keys=True)
        output.write("\n")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, urllib.error.URLError) as error:
        print(f"catalog acquisition failed: {error}", file=sys.stderr)
        raise SystemExit(1)
