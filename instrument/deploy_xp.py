#!/usr/bin/env python3
r"""
Deploy the M5 omtc proxy ddraw.dll to the JNBG game on the XP machine.

The proxy is a fake ddraw.dll placed next to Neutron.exe; it wraps the
DirectDraw7/Direct3D7 chain, decodes the render stream and streams it over
TCP to the Debian receiver. The real ddraw is kept beside it as
ddraw_orig.dll (the proxy's 20 forwarders resolve there).

--- M5 on-XP test, run order -------------------------------------------------
  1. terminal A (Debian):  python3 receiver/receive.py serve --out level1.omtc
  2. terminal B (Debian):  python3 deploy_xp.py
  3. launch the game on XP (over your usual remote-desktop, or --launch)
  4. play a level-1 session; watch terminal A fill with per-frame lines
  5. quit the game; terminal A prints the captured-session summary
------------------------------------------------------------------------------

Options:
  (none)      probe XP, then deploy the freshly built proxy
  --status    probe XP state only -- deploy nothing (read-only, safe)
  --launch    also start Neutron.exe after deploying
  --build     run proxy/build.sh before deploying
  --disable   arm the kill-switch (C:\omtc_disable -> proxy pure pass-through)
  --enable    remove the kill-switch
  --revert    restore the stock ddraw.dll (copy ddraw_orig.dll over the proxy)
"""
import os
import sys
import subprocess

sys.path.insert(0, os.path.expanduser("~/xp-command-server"))
from xp_client import XpClient, XpError

XP_HOST  = os.environ.get("XP_HOST")
XP_PORT  = int(os.environ.get("XP_PORT", "9999"))
XP_TOKEN = os.environ.get("XP_TOKEN")
if not XP_HOST or not XP_TOKEN:
    sys.exit("error: set XP_HOST and XP_TOKEN environment variables before running")

GAME_DIR = r"C:\Program Files\THQ\Jimmy Neutron\Jimmy Neutron Boy Genius"
PROXY_LOCAL = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           "proxy", "ddraw.dll")
PROXY_SRCS = ["capture.c", "capture.h", "com_wrappers.c", "ddraw_proxy.c",
              "protocol.h"]

# Receiver endpoint the on-XP proxy connects out to (must match the
# OMTC_RECEIVER_IP/PORT baked into capture.c at proxy build time).
RECEIVER_ENDPOINT = os.environ.get("OMTC_RECEIVER_ENDPOINT", "127.0.0.1:7070")


def q(path):
    return '"' + path + '"'


def game(*parts):
    return os.path.join(GAME_DIR, *parts).replace("/", "\\")


def _one(client, cmd, cwd="C:\\"):
    return client.exec([cmd], cwd=cwd)[0]


def check_local_proxy():
    """Verify the built proxy exists and is newer than its sources."""
    if not os.path.isfile(PROXY_LOCAL):
        print(f"ERROR: {PROXY_LOCAL} not found -- run proxy/build.sh first")
        return False
    proxy_mtime = os.path.getmtime(PROXY_LOCAL)
    srcdir = os.path.dirname(PROXY_LOCAL)
    stale = [s for s in PROXY_SRCS
             if os.path.isfile(os.path.join(srcdir, s))
             and os.path.getmtime(os.path.join(srcdir, s)) > proxy_mtime]
    size = os.path.getsize(PROXY_LOCAL)
    print(f"[local] proxy ddraw.dll: {size} bytes")
    if stale:
        print(f"[local] WARNING: sources newer than the build: "
              f"{', '.join(stale)} -- re-run proxy/build.sh")
        return False
    return True


def run_build():
    print("[build] running proxy/build.sh ...")
    bs = os.path.join(os.path.dirname(PROXY_LOCAL), "build.sh")
    r = subprocess.run(["bash", bs], capture_output=True, text=True)
    tail = "\n".join(r.stdout.splitlines()[-3:])
    print(tail)
    if r.returncode != 0:
        print("[build] FAILED:\n" + r.stderr)
        return False
    return True


def probe(client):
    """Read-only survey of the XP side. Returns a dict of findings."""
    print(f"[xp] ping {client.ping():.1f} ms")
    f = {}

    r = _one(client, f'dir /b {q(game("Neutron.exe"))}')
    f["game"] = (r["exit_code"] == 0)
    print(f"[xp] Neutron.exe present: {f['game']}")

    r = _one(client, f'dir /b {q(game("ddraw.dll"))}')
    f["ddraw"] = (r["exit_code"] == 0)
    r = _one(client, f'dir /b {q(game("ddraw_orig.dll"))}')
    f["ddraw_orig"] = (r["exit_code"] == 0)
    print(f"[xp] game-dir ddraw.dll present:      {f['ddraw']}")
    print(f"[xp] game-dir ddraw_orig.dll present: {f['ddraw_orig']}")

    r = _one(client, 'tasklist /FI "IMAGENAME eq Neutron.exe"')
    f["running"] = ("Neutron.exe" in r["stdout"])
    print(f"[xp] Neutron.exe running: {f['running']}")

    r = _one(client, 'dir /b C:\\omtc_disable')
    f["kill_switch"] = (r["exit_code"] == 0)
    print(f"[xp] kill-switch (C:\\omtc_disable): {f['kill_switch']}")
    return f


def deploy(client, findings, launch=False):
    if findings["running"]:
        print("ABORT: Neutron.exe is running -- quit the game first "
              "(ddraw.dll would be locked).")
        return False
    if not findings["game"]:
        print(f"ABORT: Neutron.exe not found in {GAME_DIR}")
        return False

    # Ensure the real ddraw is parked as ddraw_orig.dll (idempotent).
    if not findings["ddraw_orig"]:
        print("[deploy] ddraw_orig.dll missing -- copying stock ddraw.dll")
        r = _one(client, f'copy /Y C:\\WINDOWS\\system32\\ddraw.dll '
                          f'{q(game("ddraw_orig.dll"))}')
        if r["exit_code"] != 0:
            print("ABORT: could not stage ddraw_orig.dll:\n" + r["stdout"])
            return False
    else:
        print("[deploy] ddraw_orig.dll already present -- keeping it")

    # Clear stale capture artifacts so the test starts clean.
    client.exec(['del /Q C:\\omtc.log C:\\omtc.bin 2>nul'])

    # Upload the proxy.
    print(f"[deploy] uploading proxy -> {game('ddraw.dll')}")
    n = client.upload(PROXY_LOCAL, game("ddraw.dll"))
    print(f"[deploy] uploaded {n} bytes")

    # Verify the on-disk size matches.
    r = _one(client, f'for %A in ({q(game("ddraw.dll"))}) do @echo %~zA')
    remote_size = r["stdout"].strip()
    if remote_size != str(os.path.getsize(PROXY_LOCAL)):
        print(f"WARNING: remote size {remote_size} != "
              f"local {os.path.getsize(PROXY_LOCAL)}")
    else:
        print(f"[deploy] verified on-disk size: {remote_size} bytes")

    print(f"[deploy] OK -- proxy will stream to {RECEIVER_ENDPOINT}")
    print("[deploy] start receiver/receive.py serve BEFORE launching the game")

    if launch:
        print("[deploy] launching Neutron.exe ...")
        client.exec([f'start "" {q(game("Neutron.exe"))}'], cwd=GAME_DIR)
    return True


def main():
    args = set(sys.argv[1:])

    if "--build" in args:
        if not run_build():
            sys.exit(1)

    status_only = "--status" in args
    want_deploy = not (status_only or {"--disable", "--enable", "--revert"} & args)

    if want_deploy and not check_local_proxy():
        print("Fix the build, or pass --build, then retry.")
        sys.exit(1)

    try:
        with XpClient(XP_HOST, XP_PORT, XP_TOKEN) as c:
            findings = probe(c)

            if "--disable" in args:
                c.exec(['echo. > C:\\omtc_disable'])
                print("[xp] kill-switch armed -- proxy is now pure pass-through")
                return
            if "--enable" in args:
                c.exec(['del /Q C:\\omtc_disable 2>nul'])
                print("[xp] kill-switch removed -- capture re-enabled")
                return
            if "--revert" in args:
                if findings["running"]:
                    print("ABORT: quit Neutron.exe before reverting.")
                    sys.exit(1)
                if not findings["ddraw_orig"]:
                    print("ABORT: no ddraw_orig.dll to restore from.")
                    sys.exit(1)
                r = _one(c, f'copy /Y {q(game("ddraw_orig.dll"))} '
                            f'{q(game("ddraw.dll"))}')
                print("[xp] reverted: stock ddraw.dll restored"
                      if r["exit_code"] == 0 else "[xp] revert FAILED")
                return

            if status_only:
                print("[status] probe only -- nothing deployed")
                return

            if not deploy(c, findings, launch=("--launch" in args)):
                sys.exit(1)
    except XpError as e:
        print(f"ERROR: XP command server: {e}")
        print("Is XPCommandServer running? (sc query XPCommandServer)")
        sys.exit(1)
    except OSError as e:
        print(f"ERROR: cannot reach {XP_HOST}:{XP_PORT}: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
