# OMT2 Live Instrumentation — Implementation Plan

**Status:** plan, 2026-05-16. Updated 2026-05-19 with Phase B infrastructure.
Supersedes the preparation notes in `omt2_instrumentation_notes.md` (those remain
valid as background).
**Track:** standalone instrumentation track; foundation for Phase 12 parity
work (terrain, ground texture, water, level mirroring, lighting).
**Recommended executor:** Opus 4.7, high effort — novel-architecture work
(COM proxying, DX7, win32 cross-compile, binary protocol).

**2026-05-19 Update:** XP Command Server Phase B complete (file transfer +
Python client library). Deployment now via `xp_client.upload()`; batched control
via `xp_client.exec()`. Eliminates Samba/VNC friction and freeSSHd risk.

---

## 1. Goal & relationship to Phase 12

Build a **live render-stream capture** of the original *Jimmy Neutron: Boy
Genius* (JNBG) running on the XP machine, decode it into structured objects on
Debian, and diff it against `jn-engine`'s own draw stream.

Phase 12 (the five gaps surfaced by Phase 11's XP comparison — untextured/flat
ground, no terrain topography, no stream/water, suspected X-mirroring, dark
lighting) **consumes** this instrument's output. Eyeball VNC diffing is not
rigorous enough; this gives measured ground truth instead.

---

## 2. Resolved up front — `ddraw` import/export surface

`objdump -p` on the original binaries (this session):

- **`OMT2.dll` imports exactly two symbols from `DDRAW.dll`:**
  `DirectDrawCreateEx` and `DirectDrawEnumerateA`.
- **`Neutron.exe` does not import `ddraw` at all** — only `OMT2.dll`,
  `DSOUND.dll`, `USER32.dll`, `KERNEL32.dll`.

⇒ Only those two functions need to be *implemented*.

**CORRECTION (M1, 2026-05-16): the proxy must still EXPORT the full ddraw
surface.** The original claim here — "export only those two functions, the
others are not on any code path" — was wrong and crashed the game. When the
game creates a Direct3D device, the runtime loads **`d3dim700.dll`**, which
imports ddraw *internals* (`AcquireDDThreadLock`, `D3DParseUnknownCommand`,
`DDInternalLock`, …) by name **and by ordinal**. Those imports resolve against
*our* module. So the proxy must mirror the real `ddraw.dll`'s **entire export
table — all 22 symbols at their exact ordinals** (base 1; `DirectDrawCreateEx`
@11, `DirectDrawEnumerateA` @12). The 20 we don't implement are `.def`
forwarders to `ddraw_orig.dll`. The XP `ddraw.dll` export table is captured in
`instrument/proxy/ddraw_proxy.def`.

Open question #1 from the notes is **closed** (with the correction above).

---

## 3. Architecture

```
XP machine (<XP_HOST>)                Debian (<DEBIAN_HOST>)
┌──────────────────────────┐
│ Neutron.exe              │
│  └─ OMT2.dll             │
│      └─ ddraw.dll  ←──────┼─ our proxy (32-bit PE)
│          ├─ forwards all  │      ├─ COM wrapper chain
│          │  calls to the  │      │   IDirectDraw7 → IDirect3D7
│          │  real ddraw    │      │   → IDirect3DDevice7 → surfaces
│          └─ real ddraw    │      ├─ decodes DrawPrimitive / SetTexture /
│             (system32)    │      │   SetTransform / render+stage state
│                           │      └─ ring buffer + send thread
└───────────────┼───────────┘                 │
                 └──────── TCP (own socket) ───┼──→ receiver/receive.py
                                               │      → structured Frame objects
                                               │      → .omtc session file
                       jn-engine (demo)  ──────┼──→ same protocol (JN_CAPTURE)
                                               │
                                          diff/diff.py
                                          → original-vs-demo report
```

Key principle: intercept at the **stable DX7 API boundary**. The proxy needs
zero further OMT2-internals RE — the middleware does the work, we read what it
emits.

### Repo layout (new code)

```
jn-engine/instrument/
  proxy/
    protocol.h        # record schema — shared truth (C side)
    ddraw_proxy.c     # DllMain, DirectDrawCreateEx/EnumerateA exports
    com_wrappers.c    # IDirectDraw7 / IDirect3D7 / IDirect3DDevice7 / surface wrappers
    capture.c         # record encoding, ring buffer, Winsock send thread
    ddraw_proxy.def   # export list (undecorated names + forwarders)
    build.sh          # zig cc -target x86-windows-gnu
  receiver/
    protocol.py       # Python mirror of protocol.h (kept in lockstep)
    receive.py        # TCP listener → Frame objects → .omtc file
  diff/
    diff.py           # original-vs-demo derived-fact comparison
docs/
  omt2_instrumentation_plan.md   # this file
```

The demo-side capture lives in the engine itself: `src/engine/capture.c`
(+ `.h`), compiled in only under `-DJN_CAPTURE`.

---

## 4. Wire protocol (`protocol.h`)

Little-endian. Stream = header, then a record stream. Versioned so the Python
mirror can reject mismatches.

**Stream header:** `magic u32 'OMTC' · version u16 · pid u32 · screen_w u16 ·
screen_h u16`

**Record:** `type u8 · len u24` (payload length) · payload.

| Record | Payload |
|---|---|
| `FRAME_BEGIN` | `seq u32, t_ms u32` |
| `FRAME_END` | `seq u32, draw_count u32, dropped u32` |
| `SET_TRANSFORM` | `which u8 (WORLD/VIEW/PROJ), m f32[16]` |
| `VIEWPORT` | `x,y,w,h i32, minz,maxz f32` |
| `SET_TEXTURE` | `stage u8, tex_id u32` (0 = none) |
| `TEXTURE_DEF` | `tex_id u32, w u16, h u16, d3dfmt u32, sha1 u8[20]` — once per surface |
| `SET_RENDERSTATE` | `state u32, value u32` |
| `SET_TEXSTAGESTATE` | `stage u8, state u32, value u32` |
| `SET_LIGHT` | `index u8, D3DLIGHT7 blob` |
| `SET_MATERIAL` | `D3DMATERIAL7 blob` |
| `DRAW_PRIMITIVE` | `prim_type u8, fvf u32, vtx_count u32, vertices[]` (36-byte verts for FVF 0x152) |
| `DRAW_INDEXED` | as above + `idx_count u32, indices u16[]` |

`protocol.py` mirrors this exactly; a CI-style assert compares a `version`
constant on both sides.

---

## 5. Milestones

Each milestone has a concrete on-XP test and a hard exit criterion. The autonomy
contract applies (accept-all edits between effort checkpoints; **visual QA /
on-XP test is the user gate**) — each milestone exit is a ⛔ checkpoint.

### M1 — Pass-through proxy (game runs, zero behavior change)

**STATUS: ✅ DONE (2026-05-16).** Game runs with the proxy live; `C:\omtc.log`
shows the load line + 3× `DirectDrawCreateEx -> pass-through, hr=0x00000000`;
`vnccap.py` confirms normal rendering. Exit criterion met.

As-built (differs from the original sketch — see notes below):

- `ddraw_proxy.c`: `DllMain` logs load to `C:\omtc.log`. `DirectDrawCreateEx`
  and `DirectDrawEnumerateA` resolve the real ddraw as **`ddraw_orig.dll` from
  the proxy's own directory** (`GetModuleFileNameA` → strip → append). Loading
  by the distinct name `ddraw_orig.dll` is recursion-safe (no need for the
  `system32` absolute path the sketch called for) **and** keeps a single shared
  ddraw module with the forwarders — two real-ddraw copies would have split
  global state (thread locks, surface tables) and crash.
- `ddraw_proxy.def`: exports all **22** ddraw symbols at exact ordinals — 2
  implemented (`DirectDrawCreateEx` @11, `DirectDrawEnumerateA` @12,
  undecorated) + 20 forwarders `Name = ddraw_orig.Name @ord`. See §2 correction.
- `build.sh`: **two-step** build — compile (needs zig's MinGW headers), then
  link with `-nostdlib` (a normal link pulls the UCRT `api-ms-win-crt-*.dll`,
  absent on XP → load failure). `DllMain` is wired as the raw DLL entry point;
  the proxy uses zero CRT. `objdump -p` verification gate before deploy.
- `fix_forwarders.py`: zig/lld prepends a stray `_` to `.def` forwarder targets
  on i386 (`_ddraw_orig.Name`); this rewrites them in place, length-preserving.
- Deploy: copy real `system32\ddraw.dll` → game dir as `ddraw_orig.dll`; copy
  proxy → game dir as `ddraw.dll`. Deployed via **`xp_client.py` file transfer**
  (`xp_client.upload()` / `xp_client.download()`) — Phase B (2026-05-19)
  infrastructure upgrade eliminates Samba/VNC friction.
- Kill-switch: `C:\omtc_disable` is detected and logged at load. (No behavioural
  effect in M1 — already pure pass-through — but the mechanism is in place for
  later milestones.)

**Exit ⛔:** game visually identical, proxy confirmed loaded. ✅

### M2 — COM wrapper chain (still zero behavior change)

**STATUS: ✅ DONE (2026-05-16).** Game renders identically with all 4 interfaces
wrapped; `C:\omtc.log` shows `COM self-check: ... vtbl offset=0x64 ... vtbl OK`
and `DirectDraw7 wrapped`; `vnccap.py` confirms 3D geometry (textured, lit
character) rendering correctly through the wrapper chain. Exit criterion met.

As-built:

- Uses mingw-w64 `ddraw.h` / `d3d.h` (bundled with zig) for the real interface
  + vtable struct definitions.
- Wrapper pattern (C): uniform `ComWrap { const void *lpVtbl; void *real; }` —
  `lpVtbl` first so a `ComWrap*` is ABI-identical to any `IFoo*`. One static
  `const <IF>Vtbl` of `__stdcall` thunks per interface.
- **The 136 thunks are machine-generated** — `gen_wrappers.py` parses the four
  `DECLARE_INTERFACE_` blocks from the same headers `com_wrappers.c` includes
  (so the vtable layout is guaranteed to match the compiler's struct view) and
  emits `com_wrappers_gen.inc`. The generator auto-derives arg handling from the
  parsed types: a single-pointer-to-wrapped-interface input arg is run through
  `unwrap()`; a double-pointer output arg is run through the matching `wrap_*()`
  after the call. `QueryInterface`/`Release` route to hand-written generics.
- Chain wrapped: `DirectDrawCreateEx` → `IDirectDraw7`; `QueryInterface` →
  `IDirect3D7`; `CreateDevice` → `IDirect3DDevice7`; `CreateSurface` (and every
  other surface-returning method) → `IDirectDrawSurface7`.
- Identity table: a fixed static `ComWrap` pool (CRT-free — no malloc), keyed on
  `(real, vtbl)` so a given real object always yields the same wrapper.
  `generic_Release` frees pool slots when the real refcount hits 0.
- **Cross-check:** both compile-time (`_Static_assert` on
  `__builtin_offsetof(IDirect3DDevice7Vtbl, DrawPrimitive) == 0x64`) and runtime
  (`omtc_com_selfcheck` logs the offset). Both confirmed `0x64` / `vtbl OK`.
- Kill-switch (`C:\omtc_disable`) now has real effect: it forces
  `DirectDrawCreateEx` to skip wrapping (pure pass-through).

**Exit ⛔:** identical render with the full wrapper chain live. ✅

### M3 — Frame markers + draw counts (to file)

**STATUS: ✅ DONE (2026-05-16).** A ~22-minute level-1 session (38 000+ frames)
ran smoothly; `C:\omtc.log` shows steady cadence and plausible per-frame draw
counts. Exit criterion met.

As-built:

- A generic **pre-forward hook table** in `gen_wrappers.py` (`HOOKS`, keyed on
  `(interface, method)`) — each entry emits a `void(void)` hook call before the
  generated thunk forwards. Forwarding/unwrap/wrap logic is untouched; the hooks
  are pure side effects. Reusable for M4+.
- Hooks (`com_wrappers.c`, defined ahead of the generated `#include`):
  `BeginScene`→`omtc_frame_begin`, `EndScene`→`omtc_frame_end`,
  `DrawPrimitive`→`omtc_count_draw_dp`, `DrawIndexedPrimitive`→
  `omtc_count_draw_dip`, `IDirect3D7::CreateDevice`→`omtc_note_createdevice`.
- Counters are plain (no Interlocked) — DX7 rendering is single-threaded here.
- Output to `C:\omtc.log`: first 5 frames logged individually, then a rolling
  summary every 300 frames (fps, min/max/avg draws/frame, DP/DIP split). All
  maths is 32-bit — the proxy is CRT-free and 64-bit division would need libgcc
  helpers `-nostdlib` does not link. The 0.26 fps idle window (1 153 110 ms)
  confirmed the long-interval path is overflow-safe.

Findings (feed M4–M6):

- **Device-acquisition path = `IDirect3D7::CreateDevice`** (not `QueryInterface`).
  Notes open question closed.
- **OMT2 never calls `DrawIndexedPrimitive`** — DIP = 0 across the whole
  session; all geometry goes through `DrawPrimitive`. The `DRAW_INDEXED` record
  path can be treated as dead in M4.
- Per-frame `DrawPrimitive` volume: ~6 in menus, **1 000–5 000 in level-1
  gameplay** — sizes the M5 ring buffer.
- D3D-layer frame rate is uncapped (~100–1000+ fps); the game's frame limiter
  is elsewhere.

**Exit ⛔:** frame/draw counts logged, game still smooth. ✅

### M4 — Full render-state decode (binary, to file)

**STATUS: ✅ DONE (2026-05-19).** `capture.c`/`capture.h` decode `DrawPrimitive`
(FVF/prim/vertices), all transforms, viewport, render states, texture-stage
states, lights, material, and emit per-surface `TEXTURE_DEF` with a self-
contained SHA-1 of the locked pixels. Records go to `C:\omtc.bin`; the
`receiver/` Python (`protocol.py` + `receive.py`) parses the file and dumps a
human-readable frame. Build is clean and XP-safe (no UCRT). The session was cut
off by an automated Usage-Policy classifier false-positive right after the
receiver compile-check passed (`req_011CbBV6WbARstmCqaE7nPAG`) — the code itself
was complete and verified-buildable, only the wrap-up summary was lost. The
M4-exit on-XP decode test folds into the M5 live-capture test below.

- `DrawPrimitive`: decode FVF, prim type, vertex count; copy the 36-byte
  vertices (FVF `0x152` = XYZ·NORMAL·DIFFUSE·TEX1).
- Capture `SetTransform` (WORLD/VIEW/PROJ), `SetRenderState`,
  `SetTextureStageState`, lights, material, viewport.
- `SetTexture`: map surface → `tex_id` via the wrapper identity table.
- **Texture identity (pixel-hash, per decision):** on first sighting of a
  surface, `Lock` it `DDLOCK_READONLY|DDLOCK_WAIT`, SHA-1 the pixels, emit one
  `TEXTURE_DEF`. Cache by surface — never re-hash. If a bound surface is not
  lockable, fall back to hashing at `CreateSurface`/`Load`/`Blt` time, or to an
  attached system-memory surface. (See R3.)
- Records go to a binary file `C:\omtc.bin` for now.

**Test:** `receiver/receive.py --file` parses `C:\omtc.bin`; dumps one frame as
readable objects; vertex positions match level-1 world scale.
**Exit ⛔:** one frame fully decoded and human-readable on Debian.

### M5 — TCP streaming + Debian receiver

**STATUS: ✅ DONE (2026-05-19) — on-XP live-capture exit test passed.** M5 proxy
deployed to the XP game dir via `xp_client.upload()` (SHA-1 verified); loaded
cleanly (`omtc.log`: COM self-check vtbl OK, `DirectDrawCreateEx hr=0`). A live
level-1 play session streamed to `receive.py serve` on Debian: frames decoded
end-to-end into structured objects, **game ran at ~140 fps — matches the M1/M3
baseline, timing unaffected** (user confirmed smooth visually). The raw stream
saved to `instrument/m5_session.omtc` (621 MB). The run surfaced — and the
session then fixed — a receiver bug: `Session` retained every decoded Frame
(`receive.py` `self.frames`), reaching 6.3 GB RSS and swap-thrashing the host
(R8 was only half-mitigated — the bounded ring protects the *proxy*, not the
receiver). Fix: `Session.keep_frames` defaults `False`; both `serve` and
`--file` decode one frame at a time and keep only running counters. Re-decode
of the 621 MB capture validated the fix — **peak RSS 618 MB** (was 6.3 GB),
6896 frames / 4,719,113 draw calls / 256 textures, 19,168 frames dropped by the
proxy under level-1 throughput (~26% retained; whole-frame drop, every retained
frame intact). `capture.c` was rewritten around a lock-free SPSC ring buffer
(32 MiB, free-running `uint32` byte counters) drained by a dedicated Winsock
send thread. The game render thread only enqueues — it never touches the
socket — so receiver latency cannot distort timing. The proxy connects OUT to
`<DEBIAN_HOST>:7070`, sends the stream header, and reconnects every 1 s if the
receiver is down. Frames are written tentatively and published as a unit at
`FRAME_END`; if a frame would overflow the ring it is dropped whole (never a
partial record) and the running count rides in the next `FRAME_END.dropped`.
Textures first seen in a dropped frame are rolled back so their `TEXTURE_DEF`
re-emits later; the texture table also re-emits on each fresh receiver
connection (epoch bump). `receiver/receive.py` gained a `serve` mode: TCP
listener → incremental record parser → `.omtc` session file + live per-frame
print. Build is clean and XP-safe (`ws2_32.dll` added — ships with XP, no
UCRT). Verified on Debian with a synthetic stream over loopback, including
chunk-split partial records and `.omtc` round-trip; an M4-era `TextureDef`
struct-format bug in `protocol.py` was fixed in passing.
**Texture naming caveat:** the proxy SHA-1s the *raw locked surface pixels*;
the on-disk PNGs hash differently (PNG container vs decoded pixels, plus
format/pitch/mip differences). `receive.py` attempts a best-effort match
(Pillow-optional, multi-format) and falls back to stable synthetic names
(`tex_<hash8>_WxH`). True PNG-name resolution is deferred to M6's diff tooling.
**Exit ⛔ still open:** the on-XP live-capture run (folds in the M4 decode
test) — deploy the proxy via `xp_client.upload()`, run `receive.py serve` on
Debian, play level-1, confirm N frames decode and frame rate matches the M1
baseline.

- Replace the file sink with Winsock. **Dedicated send thread + ring buffer;
  the render thread only enqueues, never blocks on `send`** — blocking it would
  distort game timing and invalidate the capture.
- Proxy **connects out** to `<DEBIAN_HOST>:<port>` (XP has no inbound-firewall
  problem; connecting out is simpler). Reconnect if the receiver is down.
- Backpressure: on buffer overflow, drop **whole frames** (never partial
  records) and report the count in `FRAME_END.dropped`.
- `receiver/receive.py`: TCP listener → decodes into `Frame` objects
  `{seq, transforms, viewport, render_states, textures{}, draws[]}` → writes a
  `.omtc` session file; optional live print.
- Textures resolved to real names by matching `TEXTURE_DEF.sha1` against SHA-1s
  of the 126 known PNGs in `~/xp-jnbg-original/png/` (precomputed table).

**Test:** live capture of a level-1 play session; receiver reconstructs N
frames with **named** textures. Frame rate vs the M1 baseline confirms timing
is unaffected.
**Exit ⛔:** end-to-end live structured capture; game timing unaffected. ✅ MET
(2026-05-19) — see STATUS above.

**Throughput note carried into M6:** the Python receiver decodes only ~5
heavy (~3500-draw) frames/sec, so level-1 capture drops ~74% of frames
proxy-side. Not a correctness issue (retained frames are whole and intact, and
M6 camera-matched diffing needs representative frames, not all of them), but if
dense capture is later wanted the receiver needs a faster decoder or
capture-to-raw-then-decode-offline (the `.omtc` is already replayable). The
`--file` reader also still does a whole-file `read()` — fine at 621 MB, would
need streaming for multi-GB captures.

### M6 — Demo-side capture + diff tooling

**STATUS: ✅ DONE (2026-05-19) — code complete; the camera-matched capture run
is the open ⛔ (user gate).**

As-built:

- **Demo side** — `src/engine/capture.c`/`.h` emit the OMTC v1 wire protocol;
  wired into `renderer.c` (`renderer_draw_model`/`_billboard`/`_box`),
  `ground.c`, `tex_loader.c` (texture-name sidecar) and `main.c`
  (init/begin/end/shutdown). All entry points degrade to no-ops without
  `-DJN_CAPTURE`. Makefile `capture` target builds it; `JN_CAPTURE=<path>`
  + optional `JN_CAPTURE_FRAMES` record an `.omtc` (verified: a 30-frame demo
  capture round-trips through `receiver/receive.py`).
- **Diff tool** — `instrument/diff/diff.py` compares two `.omtc` captures by
  derived facts: camera (eye/look/FOV), object set + mirroring (per-axis
  negation nearest-neighbour test), texture set (presence diff, water/ground
  hint flags), render state / lighting (ambient, lights, fog, material), and
  ground/terrain primitives (flatness + footprint + Y-spread). Auto-detects
  capture convention (a demo capture has a `<path>.tex` sidecar), auto-picks
  the busiest 3D frame, and has a `--camera` mode for the matched-camera
  workflow. Incremental reader — never loads the 621 MB `m5_session.omtc`
  whole.
- **Key finding (M6):** OMT2 **never emits a VIEW transform** — 0 VIEW records
  across the whole 621 MB capture; it bakes the camera into every WORLD
  matrix (classic DX7 pattern). So the original's WORLD already maps
  model→*view* space. `diff.py` consequently compares in camera-relative
  (view) space — folding the demo through `VIEW·WORLD` and normalizing the
  GL↔D3D Z handedness — and the camera-eye decomposition is only available
  demo-side. Rigorous object/terrain numbers therefore need a **camera-matched
  demo capture** (the workflow below); without it `diff.py` correctly reports
  mirroring as "inconclusive".
- First report against `m5_session.omtc` (original, frame 6844) vs a demo
  capture already yields measured gap signal: original AMBIENT `0x333333`
  with LIGHTING **OFF** and 0 lights (feeds the "too dark"/lighting gap);
  original ground-class primitives all textured vs demo's untextured (feeds
  the ground-texture gap).

**Open ⛔:** capture the original at a chosen frame and the demo at that
frame's matched camera, then `diff.py orig.omtc demo.omtc` — the report then
quantifies all five gaps exactly. (Demo camera-matching has no input path yet;
that is M7's camera channel, or a manual camera seed.)

Original sketch (retained):

- **Demo side:** `src/engine/capture.c` (+`.h`), compiled only under
  `-DJN_CAPTURE` (new Makefile target `make capture`). `renderer.c` draw entry
  points (`renderer_begin_frame`/`end_frame`, `renderer_draw_model`,
  `renderer_draw_billboard`, `renderer_draw_box`) emit the **same protocol**
  records to a `.omtc` file (or socket).
- **Coordinate normalization:** the original is D3D7 (left-handed, row-major);
  `jn-engine` is GL (right-handed, column-major). `diff.py` normalizes both
  into one convention before comparing.
- **`diff/diff.py` compares derived facts, not raw draw calls** (the two engines
  batch geometry completely differently — raw call diffing is noise):
  - **Camera:** decompose VIEW + PROJ → eye position, look direction, FOV.
  - **Object world set:** WORLD transform × mesh AABB centre → world-space point
    cloud. Compare sets. **Directly answers the mirroring question** — an X-flip
    shows as a high correlation under X-negation.
  - **Texture set per frame:** which textures are bound — presence diff. Flags
    the missing **ground texture** and **water** (`water*`-type canvases the
    demo never binds).
  - **Render states:** ambient colour, lighting enable, fog → feeds the
    **lighting** gap with numbers.
  - **Low-Y geometry:** large ground/terrain primitives in the original absent
    from the demo → feeds the **terrain topography** gap.
- **Frame matching workflow:** load original frame K → set the demo camera to
  K's captured eye/look/FOV → capture the demo → diff. Removes camera drift as a
  confound.

**Test:** capture both engines at a matched camera; `diff.py` emits a report
that concretely answers all five Phase 12 gaps with measurements.
**Exit ⛔:** a diff report quantifying terrain / ground texture / water /
mirroring / lighting deltas.

### M7 — Camera channel

**STATUS: planned (re-planned 2026-05-19, post-M5/M6).** The original sketch
assumed approach A — the proxy hijacks `SetTransform(VIEW)` and substitutes a
commanded matrix. **M6 killed that premise:** OMT2 *never emits a VIEW
transform* — it bakes the camera into every WORLD matrix (0 VIEW records in the
621 MB capture). There is no VIEW call to intercept. Two consequences:

1. **Closing M6's open ⛔ does not require driving the original's camera at
   all** — it requires matching the *demo's* camera to a chosen original frame.
   That is a demo-side + offline-extraction job (M7a).
2. **The XP control channel still works**, via a different mechanism: since
   `WORLD_baked = VIEW · MODEL`, left-multiplying every `SetTransform(WORLD)` by
   a commanded delta `D` re-aims the camera by `D⁻¹` in view space — still zero
   RE. **Caveat:** limited to *small* nudges — the game's own frustum culling
   runs before draw, so geometry it already culled will not reappear (M7b).

Scope decided with the user (2026-05-19): full track — camera-matching path
*and* the XP control channel; camera recovery by **object-anchored solve**.

#### M7a — Camera extractor + demo injection (closes M6's ⛔ — critical path)

- **`instrument/diff/extract_camera.py`** (new): given an `.omtc` + frame K —
  - decompose the captured PROJECTION → fov_y / aspect / near / far;
  - identify ≥2 known **static anchor objects** in the frame: texture-SHA-1 →
    PNG name via `receive.py`'s best-effort matcher, plus vertex-AABB extent as
    a shape fingerprint; cross-reference the `entity_visual` table;
  - for each anchor, solve `VIEW = WORLD_baked · MODEL_world_true⁻¹`, where
    `MODEL_world_true` is the anchor's placement from `level1.omt` (Phase 8/9/10
    AABB-centre placements); robust multi-anchor fit (require ≥2 agreeing
    anchors; residual flags bad matches);
  - **solve with and without an X-flip of the OMT placements — the low-residual
    variant is simultaneously the rigorous answer to the Phase 12 mirroring
    question** (the extractor doubles as the mirroring measurement);
  - convert D3D-LH/row-major → GL-RH/column-major (reuse `diff.py`'s
    normalizer); emit a camera descriptor file (`eye`, full `view[16]`,
    `proj[16]`, fov, near, far).
- **Demo injection:** `renderer_set_camera_override(view, proj)` in
  `renderer.c`; `begin_frame` loads the supplied matrices instead of
  `mat4_lookat`/`mat4_perspective` from `g_cam` — a full 4×4, so no yaw/pitch
  decomposition loss and any camera roll is preserved. `capture.c` reads
  `JN_CAPTURE_CAMERA=<file>` and installs the override; `main.c` skips
  `follow_cam_update` while it is active. Viewport is set from the OMTC stream
  header's `screen_w/h` so the projection aspect matches the original. All
  guarded by `-DJN_CAPTURE` — no-op in normal builds.

**Test:** extract the camera from an original frame, render the demo from it,
confirm the demo camera pose agrees (eye/look/fov) with the descriptor.
**Exit ⛔:** demo renders an arbitrary chosen frame from an extracted original
camera; poses agree.

**STATUS: implemented 2026-05-19 — exit substantially met (5 of 6 DOF).**
Demo injection complete: `renderer_set_camera_override()` in renderer.c/.h,
`begin_frame` uses the override verbatim, `capture.c` loads a
`JN_CAPTURE_CAMERA` descriptor (line-based text, GL column-major matrices) and
installs it, `main.c` skips `follow_cam_update` + sizes the window to the
descriptor's screen. Builds clean both with and without `-DJN_CAPTURE`;
verified on the gateway display — the demo renders a coherent matched
Retroville street view from an extracted camera.

`extract_camera.py` implemented and run against `m5_session.omtc` frame 6844.
**Two plan premises were corrected by the real data:**
  1. *PROJECTION decode* — OMT2 re-points PROJ at an orthographic HUD matrix at
     the end of every frame; the extractor takes the projection live at the
     frame's *first perspective draw*. Decodes cleanly: fovY 59.99°, 4:3, near
     20.
  2. *Static chunks are NOT placed by pure translation* — each carries its own
     Y-axis (yaw) rotation, so `WORLD_baked`'s rotation block is `M_yaw·R`, not
     `R`. But the *middle row* of that block is yaw-invariant (= R's middle row,
     recovered by 80.6% consensus) and the *translation row* is yaw-invariant
     (= `origin·R + tv`). With R's middle row fixed a rotation has 1 DOF (camera
     yaw); the extractor sweeps it and Hough-votes `tv`, registering the level1
     placements against the captured translation cloud. No texture-SHA-1 anchor
     identification was needed — point-cloud registration replaced it and is
     more robust.

Result for frame 6844: 28/194 placements register as inliers, **no X-mirroring**
(28 vs 8 inliers for the flipped variant — a rigorous Phase 12 answer: the
level is *not* X-mirrored). X / Z / yaw / pitch / FOV are all locked and
visually confirmed. **Open residual:** eye *height* is weakly constrained — the
placement file stores mesh-AABB centres, not the original's chunk model
origins. `--eye-y <h>` pins it from one eyeballed matched render; closing it
properly (vertex-AABB Y fingerprint, or OMT2 RE of chunk placement) is folded
into M7c.

#### M7b — XP control channel

- Make the existing capture TCP socket **bidirectional** — the proxy already
  holds it open outbound (no XP inbound-firewall problem); the receiver sends
  command records back on the same connection. The proxy's Winsock send thread
  gains a `recv` path; commands are applied on the render thread via a small
  lock-free command queue (same SPSC discipline as the capture ring).
- Protocol bumped to **v2** (`protocol.h` + `protocol.py` mirror, version
  assert): command records `CMD_CAMERA_DELTA` (`m f32[16]`), `CMD_CAMERA_CLEAR`,
  `CMD_CAPTURE_START` / `CMD_CAPTURE_STOP`, `CMD_MARK_FRAME` (tag u32),
  `CMD_REDUMP_TEXTURES`, `CMD_KILLSWITCH`.
- `CMD_CAMERA_DELTA` applied through a new `SetTransform(WORLD)` entry in
  `gen_wrappers.py`'s `HOOKS` table — left-multiply the matrix by the current
  commanded `D` before forwarding. `receive.py` gains a `control` sub-command /
  CLI to issue commands interactively.

**Test (on-XP):** with the game live, nudge the original's camera via
`CMD_CAMERA_DELTA` (visible in `vnccap.py`); start/stop capture remotely.
**Exit ⛔:** original camera nudged live and capture controlled from Debian;
game still smooth.

#### M7c — Matched capture run + Phase 12 gap report (the real exit ⛔)

- Pick an original frame K (fresh capture or from `m5_session.omtc`);
  `extract_camera.py` → `camera.cam`; `make capture` then run the demo
  single-frame with `JN_CAPTURE=demo.omtc JN_CAPTURE_CAMERA=camera.cam`;
  `diff.py --camera orig.omtc demo.omtc`.

**Exit ⛔:** a diff report quantifying all five Phase 12 gaps with measurements
— terrain topography, ground texture, water, mirroring, lighting.

#### Still deferred (even within M7)

Entity/level control (approach C) — an injected agent calling OMT2
`OMediaWorld`/`OMediaElement` setters — needs Ghidra RE not yet done and is off
the Phase 12 critical path. Remains deferred; not part of M7's exit.

#### Files touched by M7

- new: `instrument/diff/extract_camera.py`
- `instrument/proxy/protocol.h`, `instrument/receiver/protocol.py` — v2 +
  command records
- `instrument/proxy/capture.c` — bidirectional socket, recv + command queue
- `instrument/proxy/com_wrappers.c`, `gen_wrappers.py` — `SetTransform(WORLD)`
  hook for `CMD_CAMERA_DELTA`
- `instrument/receiver/receive.py` — `control` CLI
- `src/engine/renderer.c`/`.h` — `renderer_set_camera_override`, override path
  in `begin_frame`
- `src/engine/capture.c`/`.h` — `JN_CAPTURE_CAMERA` descriptor load
- `src/game/main.c` — skip `follow_cam_update` under camera override

---

## 6. Risks & mitigations

| # | Risk | Mitigation |
|---|---|---|
| R1 | mingw DX7 vtable layout differs from the real ABI → crash | M1/M2 verify pure pass-through *before* any logging; cross-check `DrawPrimitive` slot vs Ghidra `0x64` |
| R2 | Render thread blocked on socket → distorted game timing | Ring buffer + dedicated send thread; drop whole frames, never block |
| R3 | Bound texture surface not lockable for pixel hash | Hash at create/load/Blt time instead; readonly lock; attached sysmem surface; last-resort opaque IDs |
| R4 | Proxy crashes the game; hard to recover | File-copy deploy → revert = delete one file; `C:\omtc_disable` marker forces pass-through; keep `ddraw_orig.dll` |
| R5 | `zig cc` 32-bit PE export name decoration wrong | `.def` with undecorated names; verify built DLL exports with `objdump -p` before deploy |
| R6 | Proxy `LoadLibrary("ddraw.dll")` loads itself | Always load the real ddraw by absolute `C:\WINDOWS\system32\` path |
| R7 | `freeSSHd` on XP is fragile | Instrumentation uses its **own** TCP socket, never SSH; deploy via **xp_client.py** (Phase B, 2026-05-19); launch via `xp_client.exec()` + batched commands |
| R8 | Receiver slower than the game → unbounded memory | Bounded ring buffer; frame-granular drop with reported counts |

---

## 7. M7 history — why it was a gated sketch, now re-planned

M1–M6 were committed in detail up front — all on the Phase 12 critical path.
M7 was held as a gated sketch because a control channel's design depends on
*what the capture reveals*. That gate has now been cleared: M5/M6 are done and
M7 is fully re-planned above. The key finding that forced the re-plan — OMT2
emits no VIEW transform — invalidated the sketch's approach-A premise and is
recorded in the M7 section. Entity/level control (approach C) remains deferred
even within the re-planned M7: it needs OMT2 object-graph Ghidra RE and is off
the Phase 12 critical path.

---

## 8. Open questions carried into implementation

- Is a bound texture surface lockable while bound? (R3 — resolved empirically at M4.)
- Does OMT2 get its device via `QueryInterface` or `EnumDevices`+`CreateDevice`?
  **Closed at M3: `IDirect3D7::CreateDevice`.**
- Exact handedness of the emitted PROJECTION matrix (confirmed at M4 from its
  sign pattern; feeds the M6 normalizer).
- Is the OMT the demo loads actually the *shipped* level 1, or is the apparent
  mirroring a wrong-level artifact? (Resolved by the M6 object-set diff.)

---

## 9. Pointers

- Background: `docs/omt2_instrumentation_notes.md` (preparation notes).
- Phase 11: `docs/ghidra_notes.md`, `docs/omt_3dsp_format.md`; memory
  `jn-engine-phase11-progress`.
- Ghidra project: `~/ghidra-projects/JN_decomp` (Neutron.exe + OMT2.dll).
- Original install: `~/xp-jnbg-original/` (incl. `OMT2.dll`, `png/`).
- XP capture tool: `tools/vnccap.py`. XP TightVNC `<XP_HOST>:5900` pw `<VNC_PASSWORD>`.
- Toolchain: `~/zig/zig` (0.14.0), target `x86-windows-gnu`.
- Autonomy contract: memory `feedback-autonomy-and-effort-checkpoints`.

---

## 10. M7b / M7c — ready-to-execute implementation design

**Status:** designed in detail 2026-05-19, **not yet implemented** (M7a is done;
see §5 M7a STATUS). This section is the concrete build sheet for a later
session — every decision below was worked out against the current code
(`capture.c`, `com_wrappers.c`, `gen_wrappers.py`, `protocol.h`,
`receiver/protocol.py`, `receiver/receive.py`, `diff/diff.py`,
`diff/extract_camera.py`). No code has been written yet.

### 10.1 M7b — XP control channel

**Wire protocol → v2.** `protocol.h` + `protocol.py` mirror:

- `OMTC_VERSION` `1 → 2`; add `OMTC_VERSION_MIN = 1`. **Both `receive.py` and
  `diff.py` must accept the range `[MIN, VERSION]`** — change the strict
  `version != OMTC_VERSION` checks (receive.py `_parse_header`, diff.py:92) to a
  range test — otherwise the existing **621 MB v1 `instrument/m5_session.omtc`
  becomes unreadable** (it is the source frame for the M7c diff).
- New **proxy→receiver** record `OMTC_RECORD_TYPE_FRAME_MARK = 13`, payload
  `{ seq u32, tag u32 }`. Emitted right after `FRAME_BEGIN` when a mark is
  pending. `receive.py` `Session`/`Frame` records the tag; `Frame` gains a
  `mark` field.
- New **receiver→proxy** command records, same `{type u8, len u24, payload}`
  framing, distinct type space to avoid confusion with capture records:
  | Command | Value | Payload |
  |---|---|---|
  | `OMTC_CMD_CAMERA_DELTA` | `0x80` | `float m[16]` (row-major 4×4 delta D) |
  | `OMTC_CMD_CAMERA_CLEAR` | `0x81` | — |
  | `OMTC_CMD_CAPTURE_START` | `0x82` | — |
  | `OMTC_CMD_CAPTURE_STOP` | `0x83` | — |
  | `OMTC_CMD_MARK_FRAME` | `0x84` | `tag u32` |
  | `OMTC_CMD_REDUMP_TEXTURES` | `0x85` | — |
  | `OMTC_CMD_KILLSWITCH` | `0x86` | — |

**`capture.c` — bidirectional socket + SPSC command queue.**

- **Command ring** `g_cmd_ring[32]` of `struct omtc_cmd { uint32_t type, arg;
  float m[16]; }`. Send thread = producer (`omtc_cmd_push`, drops on full),
  render thread = consumer (`omtc_cmd_drain`). Free-running `uint32` head/tail,
  `__atomic` acquire/release — same discipline as the capture ring.
- **`omtc_poll_commands(SOCKET s)`** called by the send thread *after* it drains
  the capture ring each loop: `select()` with a zero timeout to test
  readability, `recv` into a static ~4 KiB accumulation buffer, parse every
  complete command record, `omtc_cmd_push` each. Accumulation buffer is reset on
  each new connection. ~200 ms command latency (the `WaitForSingleObject`
  window) is fine. The socket stays blocking for `send`; `select` only polls
  recv — the send loop is untouched.
- **Render-thread-private state** updated only by `omtc_cmd_drain`, called at
  the **top of `omtc_capture_frame_begin`** — *before* `g_frame_capturing` is
  computed, so `CAPTURE_START` is honoured even while capture is off:
  `g_cam_delta[16]` + `g_cam_delta_active`; `g_capture_enabled` (default **1** —
  behaviour unchanged unless commanded); `g_killed`; pending mark tag;
  `REDUMP_TEXTURES` sets `g_tex_count = 0` so every `TEXTURE_DEF` re-emits.
- `g_frame_capturing` gains `&& g_capture_enabled && !g_killed`.
- `frame_begin` emits a `FRAME_MARK` record after `FRAME_BEGIN` when a tag is
  pending, then clears the pending flag.

**Camera nudge — `SetTransform(WORLD)` rewrite.**

- `omtc_camera_rewrite(uint32_t which, void *m)`: if `g_cam_delta_active` and
  `which == D3DTRANSFORMSTATE_WORLD` (`==1`) and `m`, compute `D · M` (row-major
  4×4, **float-only** — no 64-bit ops; `-nostdlib` links no libgcc helpers)
  into a **static `scratch[16]`** and return it; else return `m` unchanged.
- **Why a scratch buffer, not in-place:** if the game ever re-sends a cached
  WORLD without recomputing it, an in-place `D·M` would compound `D` every
  frame. `SetTransform` copies the matrix synchronously and rendering is
  single-threaded, so one static scratch buffer is safe.
- **`gen_wrappers.py` rewrite support:** extend the `HOOKS` dict spec with an
  optional `"rewrite"` list of `(arg_index, fn_name, [hook_arg_indices])`.
  `gen_thunk` emits `aN = (type)fn(args...);` right after `r = REAL(...)` and
  *before* the side-effect hook call (so the capture hook records what was
  actually drawn). The `SetTransform` entry gains
  `"rewrite": [(1, "omtc_camera_rewrite", [0, 1])]`; the thunk then forwards the
  rewritten `a1`.
- **Convention to resolve empirically (on-XP M7b test):** `WORLD_baked` maps
  model→view (M6 finding). Start with the thunk forwarding `D·WORLD_baked` and
  send a *tiny* translation/yaw `D`; watch `vnccap.py` to confirm sign/axis,
  then fix the convention (and whether `extract_camera.py` should invert). The
  game's frustum culling runs before draw, so **large nudges expose
  already-culled gaps — keep nudges small.**

**`receive.py` control CLI.** `serve` mode spawns a stdin reader thread; typed
lines are encoded as command records and sent on the live `conn`:
`cam <16 floats>` → `CAMERA_DELTA`, `camclear`, `start`, `stop`,
`mark <tag>`, `redump`, `kill`. The `.omtc` file still stores only the
proxy→receiver bytes.

**M7b exit ⛔ (on-XP, user gate):** game live; from `receive.py serve` issue
`cam`/`mark`/`start`/`stop`; confirm the camera nudges in `vnccap.py` and
capture toggles on/off; frame rate still matches the M1 baseline.

### 10.2 M7c — matched capture run + Phase 12 gap report

Driver script **`instrument/diff/matched_diff.sh`** orchestrating the workflow
already enumerated in §5 M7c:

1. pick frame K (from `m5_session.omtc`, or a fresh M7b capture);
2. `extract_camera.py <orig.omtc> --frame K [--eye-y H]` → `camera.cam`;
3. `make capture`;
4. `JN_CAPTURE=demo.omtc JN_CAPTURE_CAMERA=camera.cam JN_CAPTURE_FRAMES=1
   ./jnengine` — single matched frame;
5. `diff.py --camera <orig.omtc> demo.omtc` → the five-gap report.

The script wires the pieces and fails fast on a missing artefact; the **fresh
on-XP capture and the final report sign-off is the M7c exit ⛔ (user gate).**
`--eye-y` still pins eye height until the weak-height residual from M7a is
closed (vertex-AABB Y fingerprint, or OMT2 chunk-placement RE).

### 10.3 Files touched (supersedes the §5 "Files touched by M7" list)

- `instrument/proxy/protocol.h`, `instrument/receiver/protocol.py` — v2,
  `FRAME_MARK`, `OMTC_CMD_*`, `OMTC_VERSION_MIN`.
- `instrument/proxy/capture.c` / `.h` — command ring, `omtc_poll_commands`,
  `omtc_cmd_drain`, render-private state, `omtc_camera_rewrite`, `FRAME_MARK`
  emission.
- `instrument/proxy/gen_wrappers.py` — `"rewrite"` hook spec; `SetTransform`
  rewrite entry.
- `instrument/proxy/com_wrappers.c` — none required (the rewrite is generated);
  optional: declare `omtc_camera_rewrite` via `capture.h`.
- `instrument/receiver/receive.py` — version-range check, `FRAME_MARK` decode,
  `serve` control CLI.
- `instrument/diff/diff.py` — version-range check; `FRAME_MARK`-aware frame
  selection (optional).
- `instrument/diff/matched_diff.sh` — **new**, M7c driver.

M7a is unaffected; `renderer.c`/`capture.c` (demo side) and `main.c` already
carry the camera-override path.
