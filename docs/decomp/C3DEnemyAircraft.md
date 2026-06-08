# C3DEnemyAircraft

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DEnemyAircraft` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00499fb0`, `00499fc0`, `0049a410`, `0049a44c`, `0049a460` |
| Ctor(s) | constructor at `00417f00`; calls `C3DAI` constructor and installs only `C3DEnemyAircraft` vtables |
| Dtor(s) | scalar deleting destructor at `00417fb0`; cleanup helper `00417fe0`; adjusted destructor thunks at `00418060`, `00418070`, `00418080` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DEnemyAircraft` is a non-placeable `C3DAI` subclass used as the aircraft-specific base for `C3DHarrier`. It does not register a FourCC or serialized properties directly; the placeable aircraft object is handled by the child class.

## Field Map

`C3DEnemyAircraft` introduces no confirmed gameplay fields. The constructor only calls the `C3DAI` base constructor and overwrites the adjusted vtable pointers.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x600` | pointer | `target_object` | `C3DAI` | Runtime target resolved from `TargetName`. |
| inherited `0x608` | int | `current_state` | `C3DAI` | Active AI state used by inherited update logic. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI` | Target detection range. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI` | Initial patrol/waypoint tag. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI` | Target object tag, defaulting in the base to `JIM1`. |
| inherited `0x80c` | float | `FOV` | `C3DAI` | Inherited facing/visibility tuning. |
| inherited `0x87c` | int | `AIState` | `C3DAI` | Serialized initial AI state. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI` | Inherited wander/search radius. |
| outer `0x998` | subobject/tail | class streamer tail | constructor/destructor scaffolding | Tail cleanup/streamer allocation handled around construction and destruction; not a gameplay field. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `00417f00` | `CtorEnemyAircraft` | Runs exception-frame setup, optionally constructs the tail subobject at outer `0x998`, calls `C3DAI` constructor with a zero argument, then installs vtables `0049a460`, `0049a44c`, `0049a410`, `00499fc0`, and `00499fb0`. | non-trivial |
| 7 | `00407ee0` | `InitObjectAI` | Inherited `C3DAI` property registration for `PatrolPoint`, `VisibleRange`, `FOV`, `TargetName`, `AIState`, and `WanderRange`. | inherited |
| 10 | `00407eb0` | `ResetAIState` | Inherited reset; copies serialized `AIState` into the runtime state and clears the patrol/waypoint cache. | inherited |
| 16 | `0040a3c0` | `HandleAITouch` | Inherited AI touch handler for target/player contact markers and state-dependent target vectors. | inherited |
| 17 | `0040a390` | `ClearAITouchMarker` | Inherited contact-end marker clear. | inherited |
| 241 | `00408000` | `UpdateAIStateMachine` | Inherited per-frame AI update and movement/facing output. | inherited |
| 259 | `00409480` | `PostLoadAI` | Inherited post-load target resolution and initial state sync. | inherited |
| 260 | `0040a6b0` | `StopAIMotion` | Inherited zero-motion output helper. | inherited |
| vtable 3 slot 2 | `00417fb0` | scalar deleting destructor | Adjusts from the secondary subobject pointer, calls cleanup helper `00417fe0`, destroys the tail subobject at outer `0x998`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `00417fe0` | `CleanupEnemyAircraft` | Reinstalls `C3DEnemyAircraft` vtables during destruction and tail-jumps to the `C3DAI` cleanup helper at `00407e60`. | non-trivial |

## Per-Frame Behavior

```c
C3DEnemyAircraft::Update(dt):
    // No owned update override.
    C3DAI::UpdateAIStateMachine(dt)
```

The class has no aircraft-specific state machine in this layer. Aircraft behavior should be read from `C3DHarrier`; this base only gives the child a distinct RTTI/vtable identity above the shared `C3DAI` behavior.

## Constants And Wiring

`C3DEnemyAircraft` has no direct `.gam` rows and no direct FourCC registrar. The `3HAR` rows belong to `C3DHarrier`, which derives from this class.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| inherited `PatrolPoint` | str | `0x648` | child/object rows | Consumed by inherited `C3DAI` patrol resolution. |
| inherited `VisibleRange` | float | `0x644` | child/object rows | Consumed by inherited target-distance checks. |
| inherited `FOV` | float | `0x80c` | child/object rows | Consumed by inherited facing/visibility helpers. |
| inherited `TargetName` | str | `0x6ac` | child/object rows | Resolved by inherited post-load logic. |
| inherited `AIState` | int | `0x87c` | child/object rows | Copied into `current_state` by inherited reset/post-load logic. |
| inherited `WanderRange` | float | `0x89c` | child/object rows | Consumed by inherited wander helpers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `C3DEnemyAircraft` | RTTI/class identity for the aircraft base. | RTTI string at executable file offset `0xee358`; vtables around `00499fb0..0049a460`. |
| `C3DHarrier` | Only child class currently identified. | `docs/decomp/_hierarchy.md`; RTTI/string table; `3HAR` class-id row. |
| `3HAR` | Placeable aircraft FourCC handled by `C3DHarrier`, not this base. | `docs/_gam_classids.tsv` row `RAH3 3HAR @0041f653 FUN_0041f4a0`. |

## Assets

`C3DEnemyAircraft` owns no fixed mesh, texture, sprite, or OMT database asset. The child class and inherited `C3DAnimated` data paths supply the visible aircraft assets.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| inherited animation/state strings | `WALK`, `SIT`, `FLY`, `RUN`, `WAG`, `none` | `C3DAI` constructor | Available to the inherited AI state machine. |
| child placeable object | `C3DHarrier` / `3HAR` | hierarchy and class-id scan | Aircraft-specific placement and asset binding should be documented in the child spec. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump plus local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe` at `00417f00..00418090`; not runtime-validated.

Open questions:
- Confirm whether the optional constructor tail path at outer `0x998` is ever used by normal factory construction for this abstract-like base.
- Name the aircraft-specific fields and behavior in `C3DHarrier`, then check whether any inherited `C3DAI` tuning needs aircraft-specific labels.

## Notes

- Evidence: `DumpClass.java C3DEnemyAircraft /tmp/decomp_C3DEnemyAircraft.md` (`slots=391`, `owned_methods=1`, `offsets=1`), hierarchy row in `docs/decomp/_hierarchy.md`, and local disassembly for constructor/destructor range `00417f00..00418090`.
- The decompiler reports `(this + 0x25a)` in the destructor because it is scaling by the inferred pointer type. The byte-level tail address is outer `0x998`, confirmed in disassembly.
