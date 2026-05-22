# DebianXPGateway Infrastructure

The gateway is a Debian 12 machine that serves as a LAN router, development workstation, and application hub. It exposes multiple services via HTTPS.

## Network Interfaces

| Interface | Role | IP |
|---|---|---|
| `enp3s0` | LAN (internal) | <DEBIAN_HOST>/24 |
| `wlp4s0` | WAN (internet) | <PUBLIC_IP> (public) |

nftables masquerades LAN traffic through the WAN interface.

## Services & Access

### Landing Page
- **Port:** 8420 (ISP blocks 443)
- **Local:** `https://<DEBIAN_HOST>:8420`
- **External:** `https://<EXTERNAL_HOST>:8420`
- **Purpose:** Hub page linking all services
- **Files:** `/var/www/gateway/index.html`, `/etc/nginx/sites-available/gateway`
- **Status:** ✅ Live locally, ⏳ pending Nokia router forward `8420 → <DEBIAN_HOST>:8420`

### Web Terminal (ttyd)
- **Port:** 4200 (local), 8443 (external via Nokia)
- **Local:** `https://<DEBIAN_HOST>:4200`
- **External:** `https://<EXTERNAL_HOST>:8443`
- **Purpose:** Browser-based shell access with HTTP Basic Auth
- **Service:** `ttyd.service` (systemd)
- **Binary:** `/usr/local/bin/ttyd`
- **Auth:** `/etc/ttyd.cred` (format: `scotty:password`)
- **Update password:** `sudo bash -c 'read -sp "New password: " p; echo; printf "scotty:%s" "$p" > /etc/ttyd.cred' && sudo systemctl restart ttyd`

### jn-engine WebAssembly Demo
- **Port:** 4300 (local), 8500 (external via Nokia)
- **Local:** `https://<DEBIAN_HOST>:4300`
- **External:** `https://<EXTERNAL_HOST>:8500`
- **Purpose:** Jimmy Neutron game engine running in the browser
- **Files:** `~/jn-engine/web/` (output of `make web`)
- **Build:** `source ~/emsdk/emsdk_env.sh && cd ~/jn-engine && make web`
- **Served by:** nginx, no reload needed after rebuild

### Debian Desktop VNC
- **Port:** 4400 (local), 8400 (external via Nokia)
- **Local:** `https://<DEBIAN_HOST>:4400`
- **External:** `https://<EXTERNAL_HOST>:8400`
- **Purpose:** HTML5 VNC viewer for the Debian display
- **Services:** `x11vnc-gateway.service`, `websockify-gateway.service`
- **X11 capture:** `x11vnc` on display `:0`, port 5900 (passwordless, X-auth)
- **WebSocket bridge:** `websockify` localhost:6080 → localhost:5900

### XP Machine VNC (Pending)
- **Port:** 4401 (local), 8401 (external via Nokia)
- **Local:** `https://<DEBIAN_HOST>:4401`
- **External:** `https://<EXTERNAL_HOST>:8401` (landing page shows "Coming Soon")
- **Purpose:** HTML5 VNC viewer for Windows XP at <XP_HOST>
- **Services:** `websockify-xp.service`
- **Status:** ⏳ Waiting for XP to have VNC server running at :5900
- **Setup:** Install VNC server on XP machine, then add Nokia forward `8401 → <DEBIAN_HOST>:4401`

## TLS / Let's Encrypt

**Certificate:** `/etc/ttyd-cert.pem` + `/etc/ttyd-key.pem` (Let's Encrypt for `<EXTERNAL_HOST>`)

**Renewal:** `~/.acme.sh/acme.sh --renew -d <EXTERNAL_HOST> --ecc`

**Challenge:** DNS-01 via Cloudflare (ISP blocks inbound 80 & 443, preventing HTTP-01)

**Shared by:** ttyd, nginx (jn-engine, VNC, landing page)

**Reload hook:** Installs cert, reloads nginx, restarts ttyd — runs automatically on renewal

## Nokia Router Port Forwards

| External Port | Internal Target | Service | Status |
|---|---|---|---|
| 8400 | <DEBIAN_HOST>:4400 | Debian VNC | ✅ Done |
| 8401 | <DEBIAN_HOST>:4401 | XP VNC | ⏳ Pending (needs XP VNC first) |
| 8420 | <DEBIAN_HOST>:8420 | Landing page | ⏳ Pending |
| 8443 | <DEBIAN_HOST>:4200 | ttyd | ✅ Done |
| 8500 | <DEBIAN_HOST>:4300 | jn-engine | ✅ Done |

## Systemd Services

| Service | Purpose | Port |
|---|---|---|
| `ttyd.service` | Web terminal | 4200 |
| `nginx.service` | HTTPS proxy for all web services | 4300, 4400, 4401, 8420 |
| `x11vnc-gateway.service` | VNC capture of Debian display :0 | 5900 |
| `websockify-gateway.service` | WebSocket bridge (Debian VNC) | 6080 |
| `websockify-xp.service` | WebSocket bridge (XP VNC) | 6081 |
| `xp-daemon.service` | Persistent SSH to XP (<XP_HOST>) | Unix socket `/tmp/xp_daemon.sock` |
| `ssh.service` | OpenSSH server | 22 |
| `dnsmasq.service` | DHCP/DNS for LAN | 53, 67 |
| `smbd` / `nmbd` | Samba file sharing | 445, 137–139 |

## Firewall (nftables)

**Config:** `/etc/nftables.conf`

**Rules:**
- LAN → Internet: masquerade via wlp4s0 ✅
- Forward: LAN (enp3s0) → WAN (wlp4s0) allowed ✅
- Reverse: WAN → LAN only for established/related connections ✅

**ISP blocks:** Inbound port 80 and 443 (consumer residential service)

## Landing Page Design

Single-file HTML (`/var/www/gateway/index.html`) with inline CSS:

- **Theme:** Dark (#0d0d0d background), monospace font
- **Layout:** Responsive card grid (CSS Grid)
- **Links:**
  - Web Terminal → `:8443`
  - jn-engine → `:8500`
  - Debian Desktop → `:8400`
  - XP Machine → `:8401` (Coming Soon, dimmed, non-clickable)
- **No external deps:** All CSS, HTML, no JavaScript
- **Status badges:** Green "Active", orange "Coming Soon"

## nginx Configuration

**Sites enabled:**
- `/etc/nginx/sites-available/gateway` → landing page (8420)
- `/etc/nginx/sites-available/jn-engine` → WASM (4300)
- `/etc/nginx/sites-available/vnc` → Debian + XP VNC (4400, 4401)

**Global settings:**
- Worker: `www-data`
- TLS: Shared cert pair, TLSv1.2 + TLSv1.3
- HTTP-to-HTTPS redirect: `error_page 497` on SSL ports

**Logs:** `/var/log/nginx/{access,error}.log`

## XP Daemon & Remote Commands

Maintains a persistent SSH shell to the XP machine at <XP_HOST>:

- **Script:** `/home/scotty/xp_daemon.py` (paramiko, keeps session alive)
- **Socket:** `/tmp/xp_daemon.sock` (Unix domain socket)
- **Client:** `/home/scotty/xp_cmd.py` (send commands to socket)
- **Service:** `xp-daemon.service`

## Troubleshooting

### Landing page returns 404

Check nginx config and reload:
```bash
sudo nginx -t
sudo systemctl reload nginx
curl -k https://<DEBIAN_HOST>:8420/
```

### ttyd password not updating

Edit `/etc/ttyd.cred` directly:
```bash
sudo bash -c 'printf "scotty:newpass" > /etc/ttyd.cred'
sudo systemctl restart ttyd
```

### VNC WebSocket bridge down

```bash
sudo systemctl restart websockify-gateway.service  # Debian
sudo systemctl restart websockify-xp.service       # XP
```

### TLS cert renewal failure

Check acme.sh logs and Cloudflare API token:
```bash
~/.acme.sh/acme.sh --info
cat ~/.acme.sh/account.conf | grep CF_
```

### jn-engine WASM build fails

Ensure Emscripten is active and system libs are cached:
```bash
source ~/emsdk/emsdk_env.sh
cd ~/jn-engine && make web
```

## Related Documentation

- **CLAUDE.md** — Full machine configuration, daemon details, port forward table
- **jn-engine/docs/web_build.md** — WASM build process and Emscripten setup
- **jn-engine/docs/phase4_notes.md** — Game engine gameplay loop implementation plan
