# Proxy + capture rethink: object-centric mesh↔canvas matching

Written 2026-05-29. Companion to `texture_groundtruth_pipeline.md`. That doc
proved the capture *can* give ground-truth mesh→canvas mappings; this doc is
about the **shape of the data** — why the current capture is the wrong shape for
the job, and how to change the proxy (and/or a Debian-side reducer) so the data
is small, object-centric, and easy to walk.

---

## 1. What we're actually trying to extract

One small table:

```
OMT mesh (e.g. fireHydrant01)
   └─ material group 0  → canvas (e.g. firehydrant)
   └─ material group 1  → canvas …
```

~193 meshes × 1–15 groups ≈ a few hundred rows. That is the entire deliverable.
Everything else in the capture is scaffolding we walk through to recover it.

## 2. The shape mismatch (the core problem)

The capture is organised as **frames → per-triangle draws → every render-state
record**. The answer we want is a **flat object→texture table**. We are mining a
few hundred rows out of a stream that repeats the same facts millions of times.

Measured (`build/frame_v4_hudfix.omtc`, one frame; and the v4 sessions):

| | one frame | full session (×2) |
|---|---|---|
| `DRAW_PRIMITIVE` records | 3,523 | ~28,000,000 |
| of which 3-vertex (triangles) | 3,356 | the vast majority |
| **distinct** UV-triangles | 2,099 | ~20k + 27k |
| texture-bounded draw batches | 378 | (repeats every frame) |
| distinct textures bound | 97 | 342 / 616 |

So **~1000× redundancy**: the same static Retroville is re-submitted, triangle
by triangle, every frame, for thousands of frames. The signal (distinct
geometry + its texture) is a rounding error in the file size (1.8–4.9 GB each).

This redundancy is the source of every pain we hit:
- huge files (slow to transfer off XP, slow to walk),
- per-frame *pollution* (menu/cutscene frames bind placeholder textures and
  outvote the real ones in a global tally),
- having to re-implement per-frame-coherent voting just to undo the redundancy.

## 3. The reframe: capture **objects**, not frames-of-triangles

The game already groups its draws. Between two `SET_TEXTURE` calls it emits a run
of `DRAW_PRIMITIVE`s that all share that texture — a **material batch**. Measured
on one frame: 378 batches, mean 9.3 triangles, median 2, max 814 (the ground).
A batch is the natural *object* unit: `(world_matrix, texture_id, vertices[])`.

A batch is almost exactly "one mesh's material group for this draw". That is the
unit our matcher wants — it already votes per material group. If the capture (or
a reducer) hands us **objects** instead of loose triangles, the matching
collapses to a near 1:1 join:

```
object.uv_set  ↔  OMT mesh group.uv_set      (camera-invariant)
object.texture  → decode → canvas
```

…and we dedup objects across frames so each distinct object appears **once**.

## 4. How object-level capture fixes each known failure

| Failure (per-triangle, per-frame) | Fixed by object-centric capture |
|---|---|
| 1000× redundancy, multi-GB files | dedup by object signature → tens of thousands of rows, MBs |
| Menu/cutscene pollution | objects are gameplay batches; tag/keep only in-world (perspective, world-space) objects |
| **Foliage UV-collision** (`treebranch→concrete`) | a billboard is its *own* batch with its *own* quad UVs; per-triangle voting mixed it with the ground batch — per-object it can't collide |
| `TEXTURE_PIXELS` one-shot miss (hydrant/CAR → flat) | force-dump the texture the first time an object binds it |
| Per-process texture handles differ across sessions | key textures by **decoded-content hash**, not handle |
| Hard to "walk the data" | a flat object table is trivially walkable; no frame/state bookkeeping |

The foliage row is the clearest argument: the bug wasn't the data, it was that we
flattened objects into a triangle soup and then couldn't tell the billboard's
triangles from the grass triangles drawn near it. Keep the batch boundary and the
ambiguity disappears.

## 5. Two levers (independent; can do either or both)

### A. Debian-side reducer (no XP risk, works on existing captures) — recommended first

A Python pass that walks an existing `.omtc` **once** and emits an **object
table**:

```
object_id, texture_content_sha1, texture_dims, uv_set_signature,
world_translation_cluster, first_seen_frame, frame_count, vertex_count
```

Dedup rule: an object is `(texture_content_sha1, rounded uv_set)`. Keep the first
full vertex payload; just count repeats. Filter to perspective + world-space +
textured draws (drop ortho HUD, untextured). Output is a few thousand rows, a
few MB — *that* is the file we actually walk and match against the OMT.

Pros: zero XP risk, runs on the 6 sessions we already have, immediately makes the
data legible. Cons: still had to capture/transfer the big stream once.

### B. Proxy-side asset-capture mode (lightweight capture; needs XP redeploy)

A new mode in `ddraw.dll` (`OMTC_ASSET_MODE`, toggled by a receiver command)
that does the reduction **on XP, before streaming**:

1. **Coalesce**: buffer the draws between `SET_TEXTURE` changes into one
   `OBJECT` record (`world_matrix + texture_id + verts`), instead of N
   `DRAW_PRIMITIVE`s.
2. **Dedup**: hash each object; only emit objects not seen before this session.
   (A small hash set on XP; the static world stops emitting after ~1 frame.)
3. **Filter**: skip non-perspective (HUD) and untextured draws.
4. **Force texture dump**: the first time an object references a texture, dump
   its locked surface (`TEXTURE_PIXELS`) — closes the one-shot gap that left the
   hydrant/CAR unnamed.
5. **Drop the noise records** for this mode: no per-frame `SET_TRANSFORM`
   spam, lights, materials, render-states (none are needed for mesh↔canvas).

Result: a near-empty stream after the first frame of each new view; a full
Level-1 sweep becomes a few MB instead of gigabytes, and the receiver writes an
object table directly.

Pros: tiny captures, fast transfer, "only objects" by construction. Cons: real
C work in the proxy + an XP redeploy + re-verify (download-back SHA-1), and the
proxy must stay render-thread-safe (bounded buffer, never block — existing rule).

### Record-type simplification (applies to B, and clarifies A's output)

Today's 16 record types exist to faithfully *replay* a frame. For *asset
matching* we need exactly three:

```
OBJECT        : world_matrix(opt), texture_ref, vertex_count, verts[xyz+uv]
TEXTURE_PIXELS: texture_ref, w, h, fmt, bytes      (once per texture)
(MARK)        : gameplay span delimiter
```

Everything else (transforms-as-state, lights, materials, render-states,
viewport, colorkey) is replay scaffolding and can be omitted from an
asset-capture stream. Decluttering to these is what makes the data "make sense."

## 6. Recommended order

1. **Build the Debian-side reducer (A) now.** It runs on the captures in hand,
   collapses them to an object table, and lets us re-do the mesh↔canvas match at
   the *object* level — which should fix the foliage mis-assignments without any
   XP work. This is the cheapest way to test whether object-centric matching is
   as clean as predicted.
2. **Re-run matching on the object table.** Expect: fewer mis-assignments, same
   or better coverage, far faster. Re-evaluate ground-truth-as-default then.
3. **Only if capture size/transfer is the bottleneck** (e.g. for the fresh
   guided sweep that must cover the 71 uncovered meshes), implement the proxy
   asset-mode (B) so the new capture is small and pixel-complete by construction.

The fresh guided XP capture for coverage (hydrant, CAR, labshak-verify) is
orthogonal and still needed; doing it *through* asset-mode (B) would make it
both small and one-shot-complete, but it can also be done with the current proxy
+ reducer (A) if we don't want to touch XP yet.

## 7. Open questions to settle before coding the proxy

- Are batch boundaries (`SET_TEXTURE` runs) stable enough to equal one mesh
  group, or do multiple meshes share a run? (Measured: mean 9 tris, but 175
  length-1 runs and one 814 run — some batches are sub-mesh or super-mesh.) The
  reducer (A) will answer this empirically before we commit proxy C changes.
- World-matrix availability per object: OMT2 bakes VIEW into WORLD, so the
  per-object world matrix is camera-relative — useful for *clustering* repeats,
  not for absolute placement. UV-set stays the primary key.
- Dedup signature collisions: do distinct meshes ever share an exact UV-set +
  texture? (Identical props do — acceptable, same texture.)
