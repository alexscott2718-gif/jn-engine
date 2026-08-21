# C3DCheckPoint

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DCheckPoint` |
| Base chain | `C3DSpriteType -> ... -> C3DPolygon -> ... -> CGameObject` (see `docs/decomp_ledger.csv`; update calls `C3DPolygon::vfunc_01_241`) |
| Vftable(s) | see `docs/decomp_ledger.csv` |
| Ctor(s) | installs the `C3DCheckPoint` vftables; `InitObject` registers the properties below |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DCheckPoint` is a placeable **race checkpoint / finish line**: a polygon trigger
volume used by the racing/timed levels. When it is the `FINISHLINE` and a race timer is
active, it ends the race and draws the finish time on the HUD; otherwise it marks
progress. Family `effects_triggers_nav_cameras_sound` (wave 8). It derives from the
polygon/sprite trigger hierarchy (a flat crossable volume), not an animated mesh.

## Field Map (registered `.gam` properties)

Registered by `InitObject` (`vfunc_01_007`).

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x161` | int | `CheckAvail` | Whether this checkpoint is currently active/armed. |
| `0x148` | string | (name) | Checkpoint name/tag (`DAT_004edd18`); compared against `FINISHLINE`. |
| `this[0x162]` | int | `time_seconds` | Race time component drawn on the HUD (printed `/2`). |
| `this[0x163]` | int | `time_minutes` | Race time component drawn on the HUD (printed `/2`). |
| `this+0xe8` | string | `point_name` | Runtime name compared (`__strcmpi`) against `FINISHLINE`. |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 7 | `00414aa0` | `InitObject` | Registers `CheckAvail` + the checkpoint name. | non-trivial |
| vtable 1 slot 241 | `00414410` | `UpdateCheckPoint` | If gated by `FUN_00475ca0`, runs the polygon update; if this is the `FINISHLINE` and the race timer (`DAT_004eefc8`) is running, ends the race and draws the finish time. | non-trivial |
| vtable 1 slot 259 | `00414b20` | `Reset` | Inherited sprite reset; re-arms when `CheckAvail` is clear. | non-trivial |

### Per-frame behavior

```c
C3DCheckPoint::UpdateCheckPoint():           // vfunc_01_241 @ 00414410
    if not FUN_00475ca0(): return            // active-frame gate
    C3DPolygon::Update()                     // crossing/trigger volume update
    if strcmpi(point_name, "FINISHLINE") == 0:
        if race_timer (DAT_004eefc8) > 0:
            FUN_004073b0(1)                  // FINISH the race
            draw_number(0x212, 0x39, fmt, time_minutes/2)   // FUN_00468660 HUD
            draw_number(0x212, 0x58, fmt, time_seconds/2)
            return
        FUN_004073b0(0)                      // race not running
```

So the finish-line checkpoint watches the global race timer `DAT_004eefc8`: while it is
positive it keeps the race "running" and renders the elapsed time at HUD coords; when it
crosses, `FUN_004073b0(1)` finishes the race. Non-finish checkpoints just run the
polygon crossing logic for progress.

## Constants And Wiring

| Item | Source | Notes |
|---|---|---|
| `CheckAvail` | `.gam` int | armed/active flag |
| `FINISHLINE` | name match | special finish-line behavior |
| Race timer | `DAT_004eefc8` | global elapsed-time counter |
| Finish | `FUN_004073b0(1)` | ends the race |
| HUD time | `FUN_00468660` at `(0x212, 0x39/0x58)` | minutes/seconds readout |

## Assets

Inherited sprite/polygon visual; no own ASE/PNG registered.

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DCheckPoint` (`owned_methods=3`); the finish-line
branch, race-timer gate, and HUD time draw are read directly from `UpdateCheckPoint`.
Not runtime-validated.

Open questions:
- Why are `time_minutes`/`time_seconds` printed `/2` — is the timer stored at double
  resolution (half-seconds)?
- Identify `FUN_004073b0` and `DAT_004eefc8` precisely (race state machine + timer).
- ~~Map non-finish checkpoint progress (does crossing one arm the next / update a
  lap?).~~ **The corpus answers the shape of it — see "The authored circuits"
  below. What is still open is whether `InitObject` reads it.**

## The authored circuits (corpus evidence, 2026-08-21)

Every one of the **22** shipped `3CHK` rows authors a `Next` string that the Field
Map above does not list, and they are not decoration: in the two racing levels they
form a **closed ordered circuit**, which is the mechanism the open question above
was asking about.

`Level2b.gam` — 11 rows, a 10-node loop plus one orphan:

    startline -> CHECK1 -> CHECK2 -> CHECK2_5 -> CHECK3 -> CHECK4
              -> CHECK5 -> CHECK6 -> CHECK7 -> FINISHLINE -> STARTLINE (= startline)
    check1a   -> CHECK1        (nothing points at check1a)

`level2a.gam` — 10 rows, a closed loop with no node left out:

    STARTLINE -> CHECK2 -> CHECK3 -> CHECK4 -> CHECK5 -> CHECK6
              -> CHECK7 -> CHECK8 -> CHECK9 -> FINISHLINE -> STARTLINE

`VR04.gam` — one lone checkpoint, `Next = "none"`.

Two details worth keeping:

- **The links resolve case-insensitively.** `Level2b`'s `startline` authors
  `Next = "check1"` and the tag it reaches is `CHECK1`; `FINISHLINE` authors
  `"STARTLINE"` and reaches `startline`. That matches the house convention and this
  class's own `__strcmpi` name compare against `FINISHLINE`.
- **`FINISHLINE` closes the loop rather than ending it**, in both levels — which fits
  a lap counter, and fits `UpdateCheckPoint` returning after the finish branch rather
  than advancing progress.

`CheckAvail` is authored `0` on all 22 rows, so it is per-run state and not
configuration: `Reset` (`00414b20`) "re-arms when `CheckAvail` is clear" reads as the
between-runs re-arm of a flag the crossing sets.

### What is NOT established

Whether `C3DCheckPoint::InitObject` (`00414aa0`) actually **registers** `Next`. The
Vtable Methods table above says it registers `CheckAvail` plus the checkpoint name,
and that came off the decompiled body. A `.gam` property that no registrar declares is
still serialized by the editor and simply never read, so both readings survive the
data:

1. `InitObject` registers `Next` and this spec's summary of it is incomplete; or
2. the editor wrote a successor chain the runtime never reads, and the ordering comes
   from somewhere else — or from nowhere.

`Next` is a **per-class** registration in this engine, not an inherited one, which is
what makes the question worth asking: `C3DYokDoor::InitObject` (`00449f70`) registers
it at dword `0x181`, and `C3DLaserTrigger::InitObject` (`0042c2a0`) registers it
alongside `ItemActive` and `Toggle`. Three unrelated classes declaring it separately
is the pattern; a checkpoint chain authored on every shipped instance and read by
nobody would be the exception.

**Falsifier, and it is a small one:** re-read the registrar calls in `00414aa0`. If
`Next` is among them, reading 1 is right and the Field Map above is missing a row. The
workstation has no `Neutron.exe` or Ghidra project, so it cannot be settled here.

### Native gap (unchanged by this note)

`src/game/behaviors/behavior_checkpoint.c` reads none of it — not `Next`, not
`CheckAvail`, not the `FINISHLINE` name. It relocates the respawn point to the
last-touched checkpoint, which its own comment calls a match for the *feel*. Nothing
about the circuits above was ported: doing that would be design, since the consumer of
`Next` is not in the recovered body, and it is recorded here as evidence for whoever
does port the race, not as a change.

## Notes

- Evidence: `DumpClass.java C3DCheckPoint /tmp/dumps2/decomp_C3DCheckPoint.md`.
- Hand-deepened from the decompiled bodies (supersedes the generated skeleton). The
  timed-level finish mechanism; pairs with the VR/race `CLevelVR*` controllers.

## Native Linkage (linked-parity branch)

Aspect: **`progress`** — status `linked-blocked`.
Certificate: `docs/linkage_certificates.csv`.

Investigated 2026-07-02 (linked-parity pass). The native `vt_checkpoint`
(`src/game/behaviors/behavior_checkpoint.c`) is a **deliberate
simplification**, not a port of the decompiled `UpdateCheckPoint`
(`00414410`): its own header comment says it matches "the original's
checkpoint-progression *feel*" via a "last-touched wins" respawn-point
relocation on trigger. It has no `FINISHLINE` name check, no race-timer gate
(`DAT_004eefc8`), no `FUN_004073b0` finish call, and no HUD time draw
(`FUN_00468660`) — none of the decompiled body's actual logic is present to
diff against.

This is the same shape as `CJimmyGame`'s win-bridge exclusion
(`docs/decomp/CJimmyGame.md`): a real, working native behavior that the
project intentionally chose *not* to make 1:1 with the recovered body. No
oracle would prove anything here — there's no claim of fidelity to certify.
Porting the actual `FINISHLINE`/race-timer/HUD mechanism (which would also
need `FUN_004073b0`/`DAT_004eefc8` recovered further, per the doc's Open
Questions) is real behavior-porting work, out of scope for a
linkage-certification pass.

### Not covered / open

- No decompiled-body fidelity to certify — the native behavior is an
  intentional divergence.
- A future pass that ports the actual `FINISHLINE`/race-timer logic could
  open a real `linked` aspect here; until then this stays `linked-blocked`.
