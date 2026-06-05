# Rethink: faithful-to-original vs. imitation (architecture)

Written 2026-05-22, prompted by the Phase-12 fidelity work. The recurring
struggles this session — lighting (guessed), ground/terrain (synthesized),
water (absent), texture resolution (OMT material RE), and finally the failed
cross-engine texture matching — are **not independent bugs. They are symptoms of
one architectural choice: we built an *imitation*, not a faithful engine.**

## What we built (imitation)

```
OMT/ASE/GAM assets ──parse──▶ OUR scene model ──OUR GL renderer──▶ pixels
                              (Entity, AseModel,    (own shaders,
                               WorldPlacement)       own lighting,
                                                     own batching)
        ▲ guessed/measured params fed in: lighting, ground, terrain, tile_repeat…
```

We re-derive the original's *output* and try to match it: measure `LIGHTING=OFF`
and hardcode flat; synthesize a heightfield to hit a Y-span; reverse-engineer the
OMT canvas table to guess each mesh's texture. Every gap is a separate
guess-and-check against the capture.

## Why this is the bottleneck

1. **Lossy translation chain.** `OMT → omt_mesh_export.py → ASE → ase_loader →
   GL`. Each hop drops fidelity: the `mid0` single-material binding, the
   canvas-rid boundary, the 38-byte canvas-less materials — all are *round-trip
   artifacts*, not properties of the original. The original engine reads OMT
   once, natively, with the binding logic built in.
2. **Behavioral approximation.** Our renderer doesn't *do* what the original
   does; it tries to *look* like it. Lighting, material modulation, draw order,
   transparency, terrain — each is a reimplementation that must be independently
   tuned to the capture.
3. **Two engines, two conventions.** Original = D3D7 (left-handed, row-vector,
   its own batching). Ours = GL (right-handed, column-vector, per-mesh draws).
   The diff is noisy *by design* (905-unit alignment), and today's texture
   matching failed outright: 253 demo draws vs 2289 original draws can't be put
   in 1:1 correspondence, so we can't carry the original's textures across.
4. **The authoritative logic is in the binary.** Ghidra notes already locate the
   real render path: `OMediaDXRenderPort::draw_shape → DrawPrimitive` in
   OMT2.dll. The material→canvas→texture binding, the transform stack, the render
   states — they exist, exactly, there. We've been re-deriving what we could
   instead read.

The texture-matching dead-end is the cleanest proof: **if the engine consumed the
original's own render decisions, there would be nothing to "match."**

## The fidelity ladder

| Level | Approach | Fidelity | Effort | Interactive? |
|---|---|---|---|---|
| 1 (now) | Imitation: own model + renderer, params guessed | low, gap-by-gap | ongoing | yes |
| 2 | **Native-OMT runtime**: load OMT directly into the original's data model; port `draw_shape` material/render-state logic from OMT2.dll | high | medium | yes |
| 3 | **D3D7 command-stream renderer**: render the exact captured/emitted D3D7 stream | pixel-exact | medium | replay now; interactive when paired with logic |
| 4 | Full decomp of game + OMT2.dll, recompiled cross-platform | exact | very high | yes |

## The key reframe (Level 3) — and why it dissolves these bottlenecks

We already have a D3D7 **capture** proxy (`ddraw.dll`) and a stream format
(`.omtc`) carrying every transform, texture (pixels + SHA), render state, light,
and draw. **That stream *is* the original engine's output.** A renderer that
*consumes* the D3D7 command stream is faithful by construction:

- Textures: bound *in the stream* (TEXTURE_DEF pixels). → the whole Phase-12
  texture-resolution problem and today's matching simply do not exist.
- Lighting / render states / fog / material: in the stream. → no guessing.
- Geometry / transforms / draw order / transparency: in the stream. → no
  synthesized terrain, no batching mismatch, no convention reconciliation.

The capture/diff infrastructure built this session is exactly the foundation for
this. The proxy already translates D3D7 → a portable stream; the missing piece is
a **D3D7-semantics renderer** (GL/Vulkan/WebGL) that replays it.

### Splitting render fidelity from game logic
Level 3 renders a *recording*. To be a standalone interactive engine you still
need the **simulation** (entities, physics, triggers, AI), which our GAM/TSK
parsing + behaviors already approximate reasonably. The clean target architecture:

```
 Simulation layer (port of game logic: GAM/TSK + behaviors)
        │  emits D3D7-equivalent draw commands (the way the original does)
        ▼
 D3D7 command renderer (faithful render semantics)  ──▶ GL/WebGL/Vulkan
```

The original game *is* structured this way (game logic → OMedia render port →
D3D7). Mirroring that split is what "faithful to the original engine" means here.

## Recommendation / migration (incremental, keeps value)

We do **not** throw away this session's work — the capture proxy, `.omtc`
format, protocol, and diff tooling are the load-bearing pieces of the faithful
path. Proposed direction:

1. **Build a `.omtc` renderer** (Level 3): replay the captured Level-1 stream in
   a GL/WebGL viewer with correct D3D7 semantics (left-handed, render states,
   bound textures, transparency). This yields a *pixel-faithful* Level 1
   immediately — no OMT material RE, no texture matching. It also becomes the
   gold reference the imitation can be retired against.
2. **Promote the proxy to a live translation layer** (optional): proxy →
   socket → renderer = the original game's render, live, on a modern host.
3. **Port the material/scene model from OMT2.dll** (Level 2) so the engine can
   drive itself from OMT without a recording — replacing the lossy
   `OMT→ASE→GL` chain with the original's actual binding logic. This is where the
   `mid0`/canvas questions get *answered* (read from `draw_shape`) instead of
   guessed.
4. **Keep the simulation port** (GAM/TSK/behaviors) and reconnect it to the new
   render layer.

What to **keep**: capture proxy, `.omtc` protocol, diff/extract tooling, asset
parsers (OMT/GAM/ASE), the simulation/behavior work.
What to **retire**: the OMT→ASE export, the imitation renderer's
guessed lighting/ground/terrain, per-mesh texture RE.

## Bottom line
The imitation can be patched gap-by-gap forever and still diverge. A faithful
engine consumes the original's own render decisions — the D3D7 command stream we
already capture — so fidelity is structural, not tuned. The fastest proof is a
`.omtc` renderer for Level 1; if it looks right with zero parameter-tuning, that
validates the whole pivot.
