# OMT2 Live Instrumentation — Preparation Notes

**Status:** preparation notes, 2026-05-15. NOT a plan yet.
**Next step:** a fresh session builds a robust implementation plan from this
document. This file is written to be self-contained — a cold session with no
prior context should be able to pick it up.

---

## 1. Goal

Build a **live binary-introspection capability** for the original *Jimmy
Neutron: Boy Genius* (JNBG) PC game running on the Windows XP machine, so its
real-time render data can be read into structured, readable objects and
diffed against the `jn-engine` reimplementation.

Primary use: rigorous **parity verification** — observe exactly what the
original engine draws each frame and compare it against what our demo draws.
Secondary (later): **control** over the original — drive camera, entities,
level state — for cross-verification and experimentation.

This is, in effect, Phase 11's deferred "Stage C" promoted into a standalone
instrumentation track and broadened. It is the foundation for the Phase 12
parity work (terrain, water, level mirroring, lighting).

---

## 2. Why now / motivation

- `jn-engine` is a C / SDL2 / OpenGL reimplementation of JNBG (2001 THQ PC
  game). Repo: `~/jn-engine`.
- Phase 11 ground-truthed the texture/material pipeline by **static** Ghidra
  analysis of `OMT2.dll`. That answered a fixed set of texture unknowns but
  is a one-shot read.
- The original renderer is **OMT2.dll = "Open Media Toolkit 2.0 DR4"** — an
  obscure, long-dead piece of licensed middleware. The name string is baked
  into `OMT2.dll` (`create_texture` error path:
  `C:\Open Media Toolkit 2.0 DR4 So...`). There is essentially **no public
  documentation** for OMT2 or for how it sits on DirectDraw7.
- Phase 12 parity work needs a reliable original-vs-demo comparison. Eyeball
  diffing of VNC screenshots is not rigorous enough.
- A live instrument lets the original middleware do the work; we just read
  what it emits at a stable API boundary, instead of reversing every internal
  structure first.

---

## 3. What we already know (Phase 11 Stage B — Ghidra static analysis)

- All 3D rendering lives in **OMT2.dll**, not `Neutron.exe`.
- `OMT2.dll` ships with **named symbols** (PDB/.DEF leak) — full class and
  method visibility.
- The renderer targets **DirectDraw7 / Direct3D7** (NOT D3D8). `OMT2.dll`
  imports `DDRAW.DLL` (`DirectDrawCreateEx`, `DirectDrawEnumerateA`).
- Render entry chain:
  `OMedia3DShapeElement::render_geometry`
  → `OMediaDXRenderPort::draw_shape` (`0x100371c0`)
  → `IDirect3DDevice7::DrawPrimitive` (device vtable slot `0x64`).
- Vertex format: **FVF `0x152`** = `D3DFVF_XYZ | D3DFVF_NORMAL |
  D3DFVF_DIFFUSE | D3DFVF_TEX1` → 9 DWORDs / **36-byte vertex**
  (pos f32×3, normal f32×3, diffuse u32, uv f32×2).
- The engine uses **DrawPrimitive** (triangles pre-expanded into a vertex
  buffer), not DrawIndexedPrimitive.
- FVF uses `XYZ` (not `XYZRHW`) → geometry is **not** pre-transformed → the
  game uses `SetTransform` for WORLD/VIEW/PROJECTION. Those matrices are
  capturable.
- Other relevant device vtable offsets observed: `SetRenderState` `0x50`,
  `SetTexture` `0x8c`, `SetTextureStageState` `0x94`.
- Texture wrap mode = `D3DTSS_ADDRESS = WRAP`. Textures clamped to 256×256.
- No runtime BMP loader — textures come from OMT canvases.

**Ghidra assets:** project `~/ghidra-projects/JN_decomp` (contains both
`Neutron.exe` and `OMT2.dll`). Scripts: `~/ghidra-scripts/Phase11_OMT2_*.java`.
Decompile output: `/tmp/phase11_OMT2_*.txt` (regenerable; `/tmp` is volatile).
Java for headless Ghidra: `JAVA_HOME=~/jdk21`. Headless runner:
`~/ghidra/support/analyzeHeadless`.

See also: `docs/ghidra_notes.md` (Phase 11 section), `docs/omt_3dsp_format.md`
(canvas table, UV convention).

---

## 4. XP environment

- XP machine: **<XP_HOST>**. Debian gateway/dev box: **<DEBIAN_HOST>**.
- Game: `C:\Program Files\THQ\Jimmy Neutron\Jimmy Neutron Boy Genius\Neutron.exe`
  (PE32, i386). `OMT2.dll` sits alongside it.
- Original install also copied to Debian at `~/xp-jnbg-original/`.
- XP TightVNC: `<XP_HOST>:5900`, RFB 3.8, VNC auth, password `<VNC_PASSWORD>`.
- VNC screenshot tool: **`tools/vnccap.py`** (minimal RFB client; copied into
  the repo this session). Usage: `python3 tools/vnccap.py <out.png> [password]`.
  Captures whatever is on the XP framebuffer — game must be foreground.
- `freeSSHd` on XP is **fragile** — do not hammer it. `~/xp_cmd.py` sends
  one-off commands via the `xp-daemon.service` persistent SSH session.
  **The instrumentation must stream over its own TCP socket, not SSH.**
- **DLL search order:** a `ddraw.dll` placed in the game's own directory is
  loaded ahead of `system32\ddraw.dll` — `ddraw` is not a KnownDLL on XP, so
  the proxy-DLL approach works without any registry changes.

---

## 5. Approaches considered

**A. D3D7 proxy DLL** — *recommended starting point.*
A fake `ddraw.dll` next to `Neutron.exe` that forwards every call to the real
`ddraw.dll` while logging/streaming the render traffic. Intercepts at a
stable API boundary; needs zero further OMT2-internals RE; deterministic.

**B. External memory reader.**
A process that uses `ReadProcessMemory` to walk OMT2's in-memory object graph
(`OMediaWorld` → elements → `OMedia3DShape`s) using Ghidra-derived struct
offsets. Can read state any time, not just at draw; with `WriteProcessMemory`
gives full control. But: needs accurate struct offsets, lots of pointer
chasing, fragile across game states. More RE work.

**C. Injected in-process RPC agent.**
An injected DLL that hooks the game loop and exposes an RPC server (e.g. a
small line/JSON protocol over TCP). Combines A + B and adds control. Most
powerful, most work.

**Recommendation:** start with **A** (read-only render-stream capture), then
layer **C** for control once observation is paying off. Skip B as a primary
path — the proxy gets the render ground truth without it.

---

## 6. Technical specifics for the proxy (approach A)

- Must be **32-bit x86** to match `Neutron.exe`.
- **Cross-compile from Debian:** `zig cc -target x86-windows-gnu` (the repo
  already builds with `~/zig/zig`). No Windows compiler needed — this removes
  the obstacle the original Phase 11 draft worried about.
- The proxy must **export every symbol `ddraw.dll` consumers import**. Get the
  exact list from the import tables of `Neutron.exe` AND `OMT2.dll` (Ghidra,
  or `objdump`/a PE tool). Likely `OMT2.dll` is the only `ddraw` importer.
- COM proxy pattern: export `DirectDrawCreateEx` → call the real one → wrap
  the returned `IDirectDraw7` → wrap its `QueryInterface` so that requests
  for `IDirect3D7`, `IDirect3DDevice7`, surfaces, etc. return wrapped objects.
- Wrap `IDirect3DDevice7` and intercept: `DrawPrimitive`, `DrawIndexedPrimitive`,
  `SetTexture`, `SetTransform`, `SetRenderState`, `SetTextureStageState`,
  surface/texture creation. Everything else passes straight through.
- Decode each `DrawPrimitive`: FVF `0x152`, 36-byte vertices → structured
  pos/normal/diffuse/uv records.
- Frame delineation: hook the present path (`Flip` / `Blt` to primary, or the
  `BeginScene`/`EndScene` equivalent) to mark frame boundaries.
- Texture identity: track `SetTexture` pointer ↔ surface ↔ source canvas so
  draw calls can be labelled with a texture name, not just a pointer.
- **Streaming:** per-frame records over a TCP socket from XP → Debian. Design
  a simple framed binary (or line) protocol. Debian-side: a Python receiver
  parses the stream into objects.

---

## 7. Open questions for the planning session

- Exact `ddraw.dll` import list — which symbols must the proxy export, and
  does `Neutron.exe` import `ddraw` directly or only via `OMT2.dll`?
- Confirm the exact DirectDraw/Direct3D7 interface IIDs and vtable layouts
  used (we know it QIs to `IDirect3DDevice7`).
- How to correlate a bound texture back to an OMT canvas / source name.
- Confirm `SetTransform` is the matrix source; capture WORLD/VIEW/PROJECTION.
- Build/deploy/test loop on XP — deploying the DLL, recovering if the proxy
  crashes the game, keeping the original `ddraw.dll` reachable.
- Stream protocol design (framing, frame markers, schema/versioning).
- Demo-side counterpart: capture `jn-engine`'s equivalent draw stream and the
  alignment/diff tooling for original-vs-demo comparison.
- Control channel design (later phase): command schema, safety.

---

## 8. Suggested milestone shape (rough — refine when planning)

1. Confirm import tables; build a minimal **pass-through** `ddraw.dll` proxy
   the game loads with no behavior change.
2. Wrap the COM chain down to `IDirect3DDevice7`; still no behavior change.
3. Log `DrawPrimitive` counts + frame markers to a file on XP.
4. Decode vertex buffers; add `SetTexture` / `SetTransform` /
   `SetRenderState` capture.
5. TCP streaming + Debian-side Python receiver → structured objects.
6. Demo-side equivalent capture + original-vs-demo diff tooling.
7. (Later) control channel — drive camera/entities in the original.

---

## 9. Pointers

- Memory: `jn-engine-phase11-progress`, `jn-engine-omt-mesh-format`,
  `feedback-autonomy-and-effort-checkpoints` (autonomy contract — accept-all
  edits between effort checkpoints; visual QA is the user gate).
- Docs: `docs/ghidra_notes.md`, `docs/omt_3dsp_format.md`.
- `CLAUDE.md` — "jn-engine Phase 12 (candidate)" entry.
- Tooling already in place: `tools/vnccap.py` (XP screen capture).
