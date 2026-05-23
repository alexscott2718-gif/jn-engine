# `.omtc` replay v0 — proof results

Goal (per `docs/faithful_engine_rethink.md`): build a renderer that consumes
the original game's exact D3D7 command stream (`.omtc`), so faithfulness is
structural rather than tuned. Test on Level-1 frame 16565 (the marked frame).

## What landed
- **`instrument/diff/extract_frame_capture.py`** — streams the 4 GB capture
  once and emits a self-contained **442 KB** `.omtc` for one frame: prelude
  snapshot of every sticky D3D state (256 TEXTURE_DEFs, 11 render states,
  material, latest WORLD + PROJ — VIEW never emitted, the known Phase-11
  "camera baked into WORLD" finding) followed by the frame's 5401 records.
- **`src/engine/replay.c`** (~300 LOC) — `JN_REPLAY=<path>` mode in `jnengine`.
  Loads the small `.omtc` into RAM, walks records each render tick, decodes
  FVF 0x152 verts (XYZ+NORMAL+DIFFUSE+TEX1), tracks WORLD/VIEW/PROJ +
  TEXTURE_DEF / SET_TEXTURE / a few render states, issues `glDrawArrays` per
  DRAW_PRIMITIVE. Bypasses the entire imitation game loop.
- Verified end-to-end: **3235 GL draws issued, 256 textures registered** per
  frame, GL pipeline runs, screenshot saves.

## What v0 does NOT yet do (the matrix problem)
The visual output is wrong: only a handful of triangles visible, mostly white
sky. Root cause is **matrix-convention subtleties in the captured stream**:

1. **Matrices are column-major / column-vector**, not standard D3D row-vector.
   Confirmed by cross-referencing `diff.py`'s `xform_point(d3d)` which applies
   `M[k*4+r] * pos[k]` — i.e. column-major with column-vector. v0 now matches
   (`mat4_mul_col`, MVP = PROJ·VIEW·WORLD, `transpose=GL_FALSE`).
2. **PROJ has `m[3][3] = 1`** (and `m[2][3]=−20`, `m[3][2]=1`, `m[2][2]=1`).
   Standard D3D LH `D3DXMatrixPerspectiveFovLH` has `m[3][3]=0` and the
   `−Z·n/(Z−n)` term in `m[3][2]`. The captured PROJ is **not the standard
   D3DXMatrix form**. It still produces the expected fovY (~60°) when
   diff/extract_camera consume it, but our naive GL upload makes clip.w
   evaluate to view.z + 1 — which is negative for verts the game treats as
   "in front." Either the original game uses a non-standard projection
   (custom perspective, possibly w-buffering), or the proxy stores it in a
   form that needs pre-multiplication / inversion. Needs RE.
3. **Some WORLDs have zero translation** (7 of 167 SET_TRANSFORMs in frame
   16565; first DIAG draws all hit one of those). Combined with the model
   verts having huge magnitude (50000+), those draws land far from the eye
   regardless of camera position. Those specific draws may be culled-by-design
   in the original (skybox subsections, off-screen prep) — but with the PROJ
   issue, our clip.w sign is also wrong, so they render as small slivers
   instead of being cleanly clipped. The translation-bearing WORLDs (160/167)
   would behave better but still inherit the PROJ issue.

The empirical result: D3D7→GL conversion is *not* "just transpose + clip
control". The captured stream's projection and per-mesh transforms encode
choices specific to the original engine (D3DXMatrixPerspectiveFovLH variants,
possibly w-projection or pre-baked depth), and a faithful replayer must
either reverse those exactly or pair the stream with a capture that also
records the camera convention metadata.

## What the proof *did* establish
- **Architecture is sound.** A few hundred lines of C consume an `.omtc` and
  drive GL — the lossy `OMT→ASE→GL` chain is bypassed. Texture binding /
  render-state tracking work as designed. Once the matrix layer is correct,
  the same 3235 draws would form the original's render.
- **Data path is right.** 256 textures recognized from stream (pixels absent
  as expected — see follow-up). 5401 frame records all parsed without error.
- **Each problem now has a *known* root.** Not "guess again." Concrete
  questions: what's the exact form of OmediaDX's projection, and is the
  proxy's matrix dump transformed before serialization?

## What's needed to finish the proof
1. **RE the proxy's matrix capture path** (look at the ddraw proxy code +
   OmediaDX in Ghidra). Determine whether the captured matrix is the raw
   D3DTS_PROJECTION (standard), or post-something. The `m[3][3]=1` is a
   smoking gun. Likely a w-projection trick.
2. **Augment the capture format** to carry texture pixel payloads on first
   sighting (the SHA-only payload was right for a diff, wrong for a replay).
3. **Once 1+2 are done**, re-render Level 1 from the stream; expected output:
   the original's render exactly, with zero parameter tuning. That's the
   pivot validation.

This v0 turned the rethink from "good idea" into "concrete engineering plan
with known unknowns." It also dissolves Phase-12's per-mesh texture matching
struggle: textures live in the stream, not in our OMT-export pipeline.
