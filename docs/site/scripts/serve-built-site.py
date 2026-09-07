"""Serve one immutable staged documentation directory on loopback for UI checks."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


class Handler(SimpleHTTPRequestHandler):
    # Hidden pythonw processes have no stderr. Logging must not break responses.
    def log_message(self, _format, *args):
        pass

    def list_directory(self, path):
        self.send_error(403, "Directory listing is disabled")
        return None

    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        super().end_headers()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", required=True)
    parser.add_argument("--port", type=int, required=True)
    args = parser.parse_args()
    root = Path(args.directory).resolve(strict=True)
    if not (root / "index.html").is_file() or not (root / "data/provenance.json").is_file():
        raise ValueError("Serve a staged build with provenance, not source files")
    if not 1024 <= args.port <= 65535:
        raise ValueError("Choose an unprivileged task-owned loopback port")
    with ThreadingHTTPServer(("127.0.0.1", args.port), partial(Handler, directory=str(root))) as server:
        server.serve_forever(poll_interval=0.2)
