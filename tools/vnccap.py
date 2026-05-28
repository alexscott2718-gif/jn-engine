#!/usr/bin/env python3
"""Minimal RFB/VNC client: connect, VNC-auth, grab one framebuffer, save PNG."""
import os, socket, struct, sys, zlib
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

HOST = os.environ.get('VNC_HOST')
PORT = int(os.environ.get('VNC_PORT', '5900'))
PASSWORD = sys.argv[2] if len(sys.argv) > 2 else os.environ.get('VNC_PASSWORD')
OUT = sys.argv[1] if len(sys.argv) > 1 else '/tmp/xp_vnc.png'
if not HOST or not PASSWORD:
    sys.exit('usage: vnccap.py <out.png> [password]   (requires VNC_HOST env, and VNC_PASSWORD env if no positional password)')


def recvn(s, n):
    buf = b''
    while len(buf) < n:
        chunk = s.recv(n - len(buf))
        if not chunk:
            raise EOFError(f'wanted {n}, got {len(buf)}')
        buf += chunk
    return buf


def vnc_des_key(pw):
    key = (pw.encode('latin-1') + b'\0' * 8)[:8]
    out = bytearray()
    for b in key:
        r = 0
        for i in range(8):
            if b & (1 << i):
                r |= 1 << (7 - i)
        out.append(r)
    return bytes(out)


def png_write(path, w, h, rgb_rows):
    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + \
               struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)
    ihdr = struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)
    raw = bytearray()
    for row in rgb_rows:
        raw += b'\0' + row
    png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr) +
           chunk(b'IDAT', zlib.compress(bytes(raw), 6)) + chunk(b'IEND', b''))
    open(path, 'wb').write(png)


def main():
    s = socket.create_connection((HOST, PORT), timeout=15)
    s.settimeout(20)
    ver = recvn(s, 12)
    s.sendall(b'RFB 003.008\n')
    nsec = recvn(s, 1)[0]
    sectypes = recvn(s, nsec)
    if 2 not in sectypes:
        raise RuntimeError(f'no VNC auth; offered {list(sectypes)}')
    s.sendall(b'\x02')
    challenge = recvn(s, 16)
    key = vnc_des_key(PASSWORD)
    enc = Cipher(algorithms.TripleDES(key * 3), modes.ECB()).encryptor()
    resp = enc.update(challenge) + enc.finalize()
    s.sendall(resp)
    res = struct.unpack('>I', recvn(s, 4))[0]
    if res != 0:
        raise RuntimeError(f'auth failed (result={res})')
    s.sendall(b'\x01')  # ClientInit: shared
    si = recvn(s, 24)
    fbw, fbh = struct.unpack('>HH', si[:4])
    namelen = struct.unpack('>I', si[20:24])[0]
    recvn(s, namelen)
    print(f'framebuffer {fbw}x{fbh}', flush=True)

    # SetPixelFormat -> 32bpp, true colour, R16 G8 B0, big-endian
    pf = struct.pack('>BBBBHHHBBBxxx', 32, 24, 1, 1, 255, 255, 255, 16, 8, 0)
    s.sendall(b'\x00\x00\x00\x00' + pf)
    # SetEncodings -> raw(0) only
    s.sendall(struct.pack('>BBH', 2, 0, 1) + struct.pack('>i', 0))
    # FramebufferUpdateRequest (full, non-incremental)
    s.sendall(struct.pack('>BBHHHH', 3, 0, 0, 0, fbw, fbh))

    fb = bytearray(fbw * fbh * 4)
    while True:
        mt = recvn(s, 1)[0]
        if mt != 0:
            # skip other messages
            if mt == 2:
                continue
            if mt == 3:
                recvn(s, 3); ln = struct.unpack('>I', recvn(s, 4))[0]; recvn(s, ln); continue
            if mt == 1:
                recvn(s, 5); continue
            continue
        recvn(s, 1)
        nrects = struct.unpack('>H', recvn(s, 2))[0]
        done_pixels = 0
        for _ in range(nrects):
            rx, ry, rw, rh, enc = struct.unpack('>HHHHi', recvn(s, 12))
            data = recvn(s, rw * rh * 4)
            for row in range(rh):
                src = row * rw * 4
                dst = ((ry + row) * fbw + rx) * 4
                fb[dst:dst + rw * 4] = data[src:src + rw * 4]
            done_pixels += rw * rh
        if done_pixels >= fbw * fbh * 0.5:
            break

    rows = []
    for y in range(fbh):
        row = bytearray(fbw * 3)
        for x in range(fbw):
            o = (y * fbw + x) * 4
            # pixel format big-endian 32bpp, R16 G8 B0
            px = struct.unpack('>I', fb[o:o + 4])[0]
            row[x*3]   = (px >> 16) & 0xff
            row[x*3+1] = (px >> 8) & 0xff
            row[x*3+2] = px & 0xff
        rows.append(bytes(row))
    png_write(OUT, fbw, fbh, rows)
    print(f'saved {OUT}', flush=True)
    s.close()


if __name__ == '__main__':
    main()
