#!/usr/bin/env python3
"""Tiny HTTP/HTTPS forward proxy, so the tablet can reach package mirrors
through this machine's network over the USB link.

Listens on 0.0.0.0:8899. Plain requests are forwarded, CONNECT is tunnelled.
"""
import select
import socket
import socketserver
import sys
import urllib.request


class Handler(socketserver.StreamRequestHandler):
    timeout = 60

    def handle(self):
        line = self.rfile.readline(65536)
        if not line:
            return
        try:
            method, target, version = line.decode("latin-1").split()
        except ValueError:
            return
        headers = []
        while True:
            h = self.rfile.readline(65536)
            if h in (b"\r\n", b"\n", b""):
                break
            headers.append(h)
        sys.stderr.write(f"{method} {target}\n")

        if method.upper() == "CONNECT":
            host, _, port = target.partition(":")
            try:
                up = socket.create_connection((host, int(port or 443)), timeout=20)
            except OSError:
                self.wfile.write(b"HTTP/1.1 502 Bad Gateway\r\n\r\n")
                return
            self.wfile.write(b"HTTP/1.1 200 Connection established\r\n\r\n")
            self.wfile.flush()
            self._tunnel(up)
            return

        # Plain HTTP: fetch the absolute URI ourselves and relay the response.
        body = b""
        length = 0
        for h in headers:
            if h.lower().startswith(b"content-length:"):
                length = int(h.split(b":", 1)[1].strip())
        if length:
            body = self.rfile.read(length)
        req = urllib.request.Request(target, data=body or None, method=method)
        for h in headers:
            name, _, value = h.decode("latin-1").partition(":")
            if name.lower() in ("proxy-connection", "connection", "host",
                                "content-length", "accept-encoding"):
                continue
            req.add_header(name, value.strip())
        try:
            resp = urllib.request.urlopen(req, timeout=60)
        except urllib.error.HTTPError as e:
            resp = e
        except OSError:
            self.wfile.write(b"HTTP/1.1 502 Bad Gateway\r\n\r\n")
            return
        data = resp.read()
        self.wfile.write(f"HTTP/1.1 {resp.status} {resp.reason}\r\n".encode())
        for k, v in resp.headers.items():
            if k.lower() in ("connection", "transfer-encoding", "content-length"):
                continue
            self.wfile.write(f"{k}: {v}\r\n".encode())
        self.wfile.write(f"Content-Length: {len(data)}\r\n\r\n".encode())
        self.wfile.write(data)

    def _tunnel(self, up):
        self.connection.setblocking(False)
        up.setblocking(False)
        peers = {self.connection: up, up: self.connection}
        while True:
            try:
                r, _, _ = select.select(list(peers), [], [], 120)
            except OSError:
                return
            if not r:
                return
            for sock in r:
                try:
                    data = sock.recv(65536)
                except OSError:
                    return
                if not data:
                    return
                try:
                    peers[sock].sendall(data)
                except OSError:
                    return


class Server(socketserver.ThreadingTCPServer):
    daemon_threads = True
    allow_reuse_address = True


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8899
    Server(("0.0.0.0", port), Handler).serve_forever()
