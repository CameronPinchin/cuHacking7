#!/usr/bin/env python3
"""
Demo web server for the LFD-Net dehazing pipeline (QNX 8.0 / RPi5).

Zero third-party dependencies -- built entirely on Python's stdlib
http.server, so it runs on the stock QNX Python image with no pip/APK
packages required. If you later decide to move to Flask, every handler
below maps 1:1 onto a Flask route.

Endpoints (all GET):
  /                       static/index.html if present, else an endpoint listing
  /api/input.jpg          latest captured frame (pre-dehaze), no-cache
  /api/output.jpg         latest dehazed frame, no-cache
  /api/metrics            JSON: pipeline metrics + file freshness info
  /stream/input           MJPEG live stream of the input frame
  /stream/output          MJPEG live stream of the dehazed frame
  /static/<file>          static assets (drop the UI in ./static/)

Run:  python web_server.py [--host 0.0.0.0] [--port 8000]
Then browse to http://<pi-address>:8000/
"""

import argparse
import json
import os
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# Where the inference loop publishes images/metrics for the web tier.
WEB_ROOT = "/data/share/web"
INPUT_IMAGE = os.path.join(WEB_ROOT, "input.jpg")
OUTPUT_IMAGE = os.path.join(WEB_ROOT, "output.jpg")
METRICS_FILE = os.path.join(WEB_ROOT, "metrics.json")

# UI files live next to this script in ./static
STATIC_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "static")

STREAM_POLL_INTERVAL = 0.05   # seconds between mtime checks for MJPEG
STREAM_KEEPALIVE = 5.0        # resend current frame if nothing new (keeps proxies happy)

CONTENT_TYPES = {
    ".html": "text/html; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".js": "application/javascript; charset=utf-8",
    ".json": "application/json; charset=utf-8",
    ".jpg": "image/jpeg",
    ".jpeg": "image/jpeg",
    ".png": "image/png",
    ".svg": "image/svg+xml",
    ".ico": "image/x-icon",
    ".txt": "text/plain; charset=utf-8",
}


def read_file(path):
    """Read a whole file, or return None if it doesn't exist / is mid-write."""
    try:
        with open(path, "rb") as f:
            return f.read()
    except OSError:
        return None


class DemoHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    server_version = "LFDNetDemo/1.0"

    # ------------------------------------------------------------------ util

    def _send(self, status, content_type, body, cache=False):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        if not cache:
            self.send_header("Cache-Control", "no-store, must-revalidate")
        self.end_headers()
        self.wfile.write(body)

    def _send_json(self, obj, status=200):
        self._send(status, "application/json; charset=utf-8",
                   json.dumps(obj, indent=2).encode("utf-8"))

    def _not_found(self):
        self._send_json({"error": "not found", "path": self.path}, status=404)

    # ---------------------------------------------------------------- routes

    def do_GET(self):
        path = self.path.split("?", 1)[0]

        if path in ("/", "/index.html"):
            self.serve_index()
        elif path == "/api/input.jpg":
            self.serve_image(INPUT_IMAGE)
        elif path == "/api/output.jpg":
            self.serve_image(OUTPUT_IMAGE)
        elif path == "/api/metrics":
            self.serve_metrics()
        elif path == "/stream/input":
            self.serve_mjpeg(INPUT_IMAGE)
        elif path == "/stream/output":
            self.serve_mjpeg(OUTPUT_IMAGE)
        elif path.startswith("/static/"):
            self.serve_static(path[len("/static/"):])
        else:
            self._not_found()

    # ------------------------------------------------------------- handlers

    def serve_index(self):
        index = os.path.join(STATIC_DIR, "index.html")
        body = read_file(index)
        if body is not None:
            self._send(200, "text/html; charset=utf-8", body)
            return
        # No UI yet -- return a plain listing so the endpoints are discoverable.
        listing = (
            "LFD-Net demo backend is running.\n\n"
            "Endpoints:\n"
            "  GET /api/input.jpg    latest captured frame\n"
            "  GET /api/output.jpg   latest dehazed frame\n"
            "  GET /api/metrics      pipeline metrics (JSON)\n"
            "  GET /stream/input     MJPEG stream (input)\n"
            "  GET /stream/output    MJPEG stream (output)\n\n"
            "Drop the UI into ./static/index.html and it will be served at /\n"
        )
        self._send(200, "text/plain; charset=utf-8", listing.encode("utf-8"))

    def serve_image(self, image_path):
        body = read_file(image_path)
        if body is None:
            self._not_found()
        else:
            self._send(200, "image/jpeg", body)

    def serve_metrics(self):
        metrics = {}
        raw = read_file(METRICS_FILE)
        if raw is not None:
            try:
                metrics = json.loads(raw)
            except ValueError:
                pass  # mid-write or corrupt; fall through with file info only

        def file_info(p):
            try:
                st = os.stat(p)
                return {
                    "exists": True,
                    "size_bytes": st.st_size,
                    "age_seconds": round(time.time() - st.st_mtime, 3),
                }
            except OSError:
                return {"exists": False}

        metrics["files"] = {
            "input": file_info(INPUT_IMAGE),
            "output": file_info(OUTPUT_IMAGE),
        }
        metrics["server_time"] = time.time()
        self._send_json(metrics)

    def serve_mjpeg(self, image_path):
        """multipart/x-mixed-replace stream: pushes a new JPEG part whenever
        the file's mtime changes. Works in <img src="/stream/..."> directly."""
        boundary = "lfdnetframe"
        self.send_response(200)
        self.send_header("Content-Type",
                         f"multipart/x-mixed-replace; boundary={boundary}")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()

        last_mtime = None
        last_sent = 0.0
        try:
            while True:
                try:
                    mtime = os.stat(image_path).st_mtime
                except OSError:
                    time.sleep(0.2)
                    continue

                now = time.time()
                if mtime != last_mtime or (now - last_sent) > STREAM_KEEPALIVE:
                    body = read_file(image_path)
                    if body:
                        part = (
                            f"--{boundary}\r\n"
                            "Content-Type: image/jpeg\r\n"
                            f"Content-Length: {len(body)}\r\n\r\n"
                        ).encode("ascii") + body + b"\r\n"
                        self.wfile.write(part)
                        self.wfile.flush()
                        last_mtime = mtime
                        last_sent = now
                time.sleep(STREAM_POLL_INTERVAL)
        except (BrokenPipeError, ConnectionResetError, OSError):
            return  # client went away; thread exits cleanly

    def serve_static(self, rel_path):
        # Resolve and confine to STATIC_DIR (blocks ../ traversal)
        full = os.path.realpath(os.path.join(STATIC_DIR, rel_path))
        if not full.startswith(os.path.realpath(STATIC_DIR) + os.sep):
            self._not_found()
            return
        body = read_file(full)
        if body is None:
            self._not_found()
            return
        ext = os.path.splitext(full)[1].lower()
        ctype = CONTENT_TYPES.get(ext, "application/octet-stream")
        self._send(200, ctype, body, cache=True)


def main():
    ap = argparse.ArgumentParser(description="LFD-Net demo web server (stdlib only)")
    ap.add_argument("--host", default="0.0.0.0", help="bind address")
    ap.add_argument("--port", type=int, default=8000, help="listen port")
    args = ap.parse_args()

    os.makedirs(WEB_ROOT, exist_ok=True)
    os.makedirs(STATIC_DIR, exist_ok=True)

    server = ThreadingHTTPServer((args.host, args.port), DemoHandler)
    print(f"Serving on http://{args.host}:{args.port}/  (web root: {WEB_ROOT})")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
