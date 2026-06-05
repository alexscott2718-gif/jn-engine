# M7c dry run — walkthrough

Five-gap Phase 12 report against the existing v1 `m5_session.omtc` (no fresh
on-XP capture needed). This stresses the full diff pipeline so any plumbing
bugs surface before the M7b on-XP gate.

## Prerequisites

- `~/jn-engine/instrument/m5_session.omtc` (621 MB v1 capture from M5)
- `~/jn-engine/jnengine` binary (built by `make` or `make capture`)
- protocol v2 receiver/diff (already landed — accepts v1 captures via
  `OMTC_VERSION_MIN`)

## Step 1 — Pick a frame

Frame **6844** is the one M7a already validated: matched Retroville street
view, 28/194 inliers, no X-mirror. Safe default for the dry run.

## Step 2 — Run extract_camera in isolation first

This decides whether you need `--eye-y`. From any shell:

```bash
cd ~/jn-engine
python3 instrument/diff/extract_camera.py instrument/m5_session.omtc \
    --frame 6844 --emit /tmp/camera.cam
```

You should see four sections:

1. **PROJECTION** — fovY ~60°, 4:3, near 20
2. **VIEW ROTATION — middle row** — consensus % (want ≥30%; 80%+ is healthy)
3. **STATIC OBJECT CLOUD** — count of distinct WORLD translation rows
4. **REGISTER vs N level1 placements** — inlier counts for identity vs
   X-flipped, then the verdict line: "NO mirroring" or "LEVEL IS X-MIRRORED"

If the script warns about `eye_y` being weakly constrained, that's the M7a
residual. Pin it with `--eye-y H` (eyeball a value from a prior matched
render) and rerun.

## Step 3 — Decide on `make capture`

The driver runs `make capture` by default — that `cleans` first and rebuilds
with `-DJN_CAPTURE`, ~30 s. Pass `--skip-make` if you know `jnengine` is
already capture-built. When in doubt, let it rebuild.

## Step 4 — Run the driver

```bash
cd ~/jn-engine
./instrument/diff/matched_diff.sh instrument/m5_session.omtc \
    --frame 6844 --out-dir ./build/m7c
```

Four labeled steps:

- `[1/4] extracting camera ...` — writes `./build/m7c/camera.cam`
- `[2/4] make capture` — rebuilds `jnengine` with `-DJN_CAPTURE`
- `[3/4] demo single-frame capture` — runs `jnengine`, exits after one
  frame, writes `./build/m7c/demo.omtc`
- `[4/4] diff orig vs demo` — `diff.py` dumps the five-gap report

Useful flags for iterating after the first run:

| Flag | Effect |
|---|---|
| `--eye-y H` | pin eye height through to `extract_camera.py` |
| `--keep-camera` | reuse existing `camera.cam` from a previous run |
| `--skip-make` | skip `make capture` (assume `jnengine` already built) |
| `--skip-demo` | reuse existing `demo.omtc` |
| `--report-only` | only run step 4 (needs both artefacts present) |

## Step 5 — Read the report

Five sections, each maps to a Phase 12 gap:

| Section | Phase 12 gap | What to look for |
|---|---|---|
| 1. Camera | sanity | demo eye/look/FOV agrees with the descriptor |
| 2. Object set | **mirroring** | inlier counts under X/Y/Z negation |
| 3. Textures | **ground texture, water** | original-only textures (bound but absent demo-side) |
| 4. Render state | **lighting** | AMBIENT `0x333333`, LIGHTING OFF, 0 lights vs demo |
| 5. Terrain | **terrain topography** | low-Y primitive count, footprint, Y-spread |

## Likely failure modes

- **Demo loops forever instead of exiting after one frame** — check
  `JN_CAPTURE_FRAMES=1` is honoured in `src/engine/capture.c:173`. Ctrl-C
  and report.
- **`make capture` fails** — usually a stale obj; from `~/jn-engine` run
  `make clean && make capture` directly to see the real error.
- **Diff reports near-empty overlap / nonsense terrain numbers** — eye_y is
  likely wrong. Rerun with `--eye-y H`.
- **`extract_camera` says weak consensus or registration FAILED** — try
  another frame (the driver supports `--frame K` on any frame, but
  perspective-busy gameplay frames work best; 6844 is the validated one).

## After the dry run

If the report comes back clean and the gap signal is what we expect (lighting
off, ground untextured demo-side, no X-mirror), the next ⛔ is M7b: deploy
the v2 proxy to XP, drive the camera live, capture a fresh `.omtc` with a
`mark` tag, and re-run `matched_diff.sh` against that.
