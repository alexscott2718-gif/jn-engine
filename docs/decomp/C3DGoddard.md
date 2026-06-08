# C3DGoddard

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DGoddard` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0049f470`, `0049f480`, `0049f8d0`, `0049f90c`, `0049f920` |
| Ctor(s) | constructor/factory block `FUN_0041c810`; registers class id `3GOD` at `0041c8cf` |
| Dtor(s) | adjusted scalar deleting destructor at `0041cbb0`; cleanup/vtable reset helper at `0041cbe0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DGoddard` is the Goddard companion/NPC leaf over `C3DAI`. It has no direct `3GOD` or `DOG3` object-type rows in the generated `.gam` schema, but it is referenced by trigger/camera data through tags such as `C3DGODDARD`. The class is therefore documented as an engine-spawned AI companion with fixed constructor defaults, fixed animation assets, mode/path helpers, and player interaction logic.

## Field Map

Offsets below are byte offsets from the primary `C3DAI`/Goddard pointer used by vtable-1 methods unless marked `outer`. Vtable-4 hooks commonly enter with the outer allocation pointer, so those fields keep the `outer` marker until full structs are applied.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x600` | pointer | `target_object` | `C3DAI`; `0041dee0`, `0041eda0` | Normal AI target, usually the player. Goddard state `4` uses it for distance/relative-position checks. |
| inherited `0x608` | int | `current_state` | `C3DAI`; ctor `0041c810`; `0041dee0` | Constructor seeds state `4`; update/touch logic has special branches when this remains `4`. |
| inherited `0x614..0x61c` | vec3 | `mode_vector_or_offset` | `0041cc30`, `0041e9e0` | Mode setter copies one of six static vectors into this inherited AI vector. |
| `0x5fc` | float | `goddard_state_elapsed` | `0041dee0` | Per-frame timer accumulated while Goddard is updating. |
| `0x604` | float | `goddard_motion_speed` | ctor `0041c810`, `0041d0c0`, `0041dee0`, `0041dcd0` | Vertical/forward speed or distance tuning used by Goddard's custom movement output. Observed values include `0`, `40`, `450`, and `600`. |
| `0x824` | bool | `player_contact_latched` | `0041eda0`, `0041dee0`, `0041ee10` | Set when Goddard touches the player/target in state `4`; auto-clears after a short timer or through slot 220. |
| `0x8d4` | pointer | `follow_marker_object` | ctor `0041c810`, `0041dee0` | Constructor-created helper object; update hides/shows it around player proximity paths. Exact class is unresolved. |
| `0x8e0` | short/int | `anim_context_mode` | `0041dee0` | A small animation/path context checked before forcing some animation changes. |
| `0x8e4` | bool | `animation_override_active` | `0041dee0` | Gates whether the update loop calls `C3DAI::UpdateAIStateMachine` or `C3DAnimated::UpdateAnimated` directly. |
| `0x8e8..0x8f0` | vec3 | `mode_target_position` | `0041ddd0`, `0041d0c0` | Target point used to compute facing and motion offsets. |
| `0x8fc` | int | `selected_animation_family` | `0041d0c0` | Tracks the currently selected family: raw code maps cases to `WALK`, `RUN`, and `FLY` string slots. |
| `0x904` | pointer | `red_tracking_marker` | ctor `0041c810`, `0041d0c0` | Constructor-created helper object positioned near a found `3RED` object when that link exists. |
| `0x908` | pointer/bool overlap | `pickup_tracking_marker_or_ready` | ctor `0041c810`, `0041d0c0`, `0041e720` | Constructor-created helper object used for `3PIC` tracking; low byte is also tested by the mode transition helper. Needs struct cleanup. |
| `0x90c` | pointer | `current_3pic_target` | `0041d0c0` | Result of a `FUN_00458e40(3PIC)` lookup for pickup-related tracking. |
| `0x910` | pointer | `current_3red_target` | `0041d0c0` | Result of a `FUN_00458e40(3RED)` lookup for red-neutron-related tracking. |
| `0x914` | bool | `needs_player_reposition_sync` | ctor `0041c810`, `0041dee0`, `0041e590` | One-shot gate for placing Goddard relative to the player. |
| `0x918` | float | `level_tracking_range` | ctor `0041c810`, `0041e590`, `0041d0c0` | Defaults to `1500.0`; slot 259 raises it to `3500.0` in `VR06`. |
| `0x91c`, `0x920` | float | `input_or_drift_accumulators` | `0041d0c0` | Updated by signed input/random deltas and damped back toward zero. |
| `0x924` | float | `contact_latch_elapsed` | `0041eda0`, `0041dee0` | Timer for clearing `player_contact_latched`. |
| `0x928` | bool | `level_gate_disabled` | `0041e590` | Set by slot 259 when Goddard is disabled/hidden for selected levels. |
| `outer 0x998` | handle/id | `secondary_effect_handle` | ctor `0041c810`, `0041e6b0` | Initialized to `-1`; helper clears it back to `-1`. Exact release owner not yet known. |
| `outer 0x99c` | handle/id | `looping_effect_handle` | ctor `0041c810`, `0041e6d0` | Released through `FUN_00458a00(handle, 0)`. |
| `outer 0x9a0` | short | `goddard_mode` | ctor `0041c810`, `0041cc30`, `0041e720` | Current mode index `0..5` used by vector selection and transition testing. |
| `outer 0x9a2` | short | `next_goddard_mode` | ctor `0041c810`, `0041e720` | Mode transition target written before probing movement/clearance. |
| `outer 0x9c0` | handle/id | `mode_effect_handle` | ctor `0041c810`, `0041dcd0` | Initialized to `-1`; toggled by the mode/effect helper using effect id `0xcc`. |
| `outer 0x9c4`, `outer 0x9c8` | pointer | `cursor_helper_a/b` | ctor `0041c810`, `0041dcd0` | Constructor-created `C3DCursor`-like helper objects hidden at startup and restored by `0041dcd0(false)`. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `0041cd70` | `InitObjectGoddard` | Logs `InitObject()`, runs `C3DAI::InitObject`, initializes inherited animation DB state, registers Goddard animation ASEs, loads `goddard02.png`, binds the texture/material, and selects the default animation. | non-trivial |
| 10 | `0041e6a0` | `ResetAIStateThunk` | Tail-jumps to `C3DAI::ResetAIState` at `00407eb0`. | inherited thunk |
| 11 | `0042ab50` | `UpdateObjectWhenNotSuppressed` | Shared-looking guard. If byte `0x49e` is clear, jumps to a `C3DObject` update/helper at `004623d0`; otherwise returns. | shared/raw |
| 16 | `0041eda0` | `HandleGoddardTouch` | Calls `C3DAI::HandleAITouch`; if the touched object matches the player tag and Goddard is in state `4`, latches contact, resets `contact_latch_elapsed`, and raises the target Y cache. | raw block |
| 220 | `0041ee10` | `ClearGoddardContactLatch` | Clears `player_contact_latched`; returns with a five-argument adjusted calling convention. | trivial |
| 221 | `0041e700` | `State4CompletionHook` | If `current_state == 4`, calls an inherited outer hook at vtable offset `0x18c`. | raw block |
| 223 | `0041ee20` | `CommonNoOpOrTraceThunk` | Tail-jumps to `CGameObject::vfunc_00_013` at `00472970`. | inherited thunk |
| 241 | `0041dee0` | `UpdateGoddardStateMachine` | Main Goddard update override. Updates contact timers, handles player-relative positioning, drives state `4`, selects animation strings, tracks nearby `3RED`/`3PIC` objects, and falls back to `C3DAI::UpdateAIStateMachine` or `C3DAnimated::UpdateAnimated` depending on local flags. | raw block |
| 243 | `0041d0c0` | `IntegrateGoddardCompanionMotion` | Custom movement integrator. Applies input/random drift, clamps vertical/forward speed, moves helper objects toward `3RED`/`3PIC` targets, and writes player-relative tracking data. | raw block |
| 259 | `0041e590` | `PostLoadGoddard` | Runs `C3DAI::PostLoadAI`, sets `level_tracking_range`, and toggles inherited visibility/update slots for levels `LV1C`, `LV1D`, `LV4A`, `LV6A`, `LEV6`, `LEV7`, and `VR06`. | non-trivial |
| vtable 3 slot 2 | `0041cbb0` | scalar deleting destructor | Runs Goddard cleanup/vtable reset, destroys the embedded streamer at `outer+0x9f0`, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 79 | `0041e580` | `AIHelper78Thunk` | Tail-jumps to inherited `C3DAI` helper `00409890`. | inherited thunk |
| vtable 4 slot 82 | `0041e530` | `MaintainVerticalMotionIfAllowed` | Reads a child/current Y value and emits a movement output when local flag `0x6f4` is clear. | raw block |
| vtable 4 slots 84, 86 | `0041e6b0` | `ClearSecondaryEffectHandle` | Clears `outer+0x998` to `-1` when set. | trivial |
| vtable 4 slot 88 | `0041e6d0` | `ReleaseLoopingEffectHandle` | Releases `outer+0x99c` through `FUN_00458a00(handle, 0)` and resets it to `-1`. | non-trivial |
| vtable 4 slot 92 | `0041ddd0` | `FaceModeTargetPosition` | Computes `mode_target_position - current_position`, derives a facing angle, and emits a rotation/facing command unless a local suppress flag is set. | raw block |
| vtable 4 slot 95 | `0041cc30` | `SetGoddardModeVector` | Switches on mode `0..5`, copies one of six global static vectors into inherited offset storage, and records `goddard_mode`. | non-trivial |
| vtable 4 slot 96 | `0041e9e0` | `TryTransitionToGoddardMode` | Given a mode, projects the corresponding static vector from Goddard's transform, checks it through collision/clearance helpers, and if clear calls the mode setter at slot `0x17c`. Returns success as a byte. | raw block |
| vtable 4 slot 97 | `0041cea0` | `ApplyGoddardOffsetMotion` | Computes a bob/orbit offset from current motion vectors and emits an inherited movement output; decays a local float back toward zero. | raw block |
| vtable 4 slot 98 | `0041dcd0` | `ToggleGoddardModeEffect` | With `true`, moves a helper/attachment, forces a high material/facing value, and creates effect id `0xcc` if no handle exists. With `false`, restores cursor helpers, sets `goddard_motion_speed = 450.0`, and releases the effect handle. | raw block |
| vtable 4 slot 99 | `0041e720` | `AdvanceGoddardModeState` | Mode transition state machine. For modes `0..5`, writes `next_goddard_mode`, probes candidate modes through slot `0x180`, and calls slot `0x17c` when the ready flag allows a transition; mode `4` includes a random branch. | raw block |

## Per-Frame Behavior

Goddard is not a simple `C3DFriends` talk leaf. Its update extends `C3DAI` with companion-specific motion and player attachment behavior:

```c
C3DGoddard::PostLoadGoddard():
    C3DAI::PostLoadAI()
    level_tracking_range = 1500.0

    if current_level == "VR06":
        level_tracking_range = 3500.0

    if current_level in {"LV1C", "LV1D", "LV4A", "LV6A", "LEV6", "LEV7"}:
        toggle inherited visibility/update hooks for the disabled level path
        level_gate_disabled = true
    else:
        toggle inherited visibility/update hooks for the active path
        needs_player_reposition_sync = true
        inherited_range_or_scale_slot(130.0)
        level_gate_disabled = false
```

```c
C3DGoddard::UpdateGoddardStateMachine(dt):
    if player_contact_latched:
        contact_latch_elapsed += dt
        if contact_latch_elapsed >= threshold:
            player_contact_latched = false
            contact_latch_elapsed = 0

    if engine_allows_update():
        if no valid local update state:
            show follow_marker_object and return

        if global player pointer is a player:
            clear an inherited active flag while Goddard is attached to player

        if state needs player-relative sync:
            place Goddard relative to player or log "ERROR: no pco for C3DGoddard"

        if animation_override_active:
            C3DAnimated::UpdateAnimated(dt)
        else:
            C3DAI::UpdateAIStateMachine(dt)

        if target_object and current_state == 4:
            update target distance, speed, player-contact behavior, and animation family
```

`IntegrateGoddardCompanionMotion` is a separate long raw integrator. It accumulates signed drift values, clamps vertical movement, tracks `3RED` and `3PIC` helper targets through `FUN_00458e40`, and writes movement/rotation outputs to the inherited C3D/OMedia transform slots.

## Constants And Wiring

### `.gam` Placement

The generated `.gam` schema has no direct object-type section for `3GOD` or `DOG3`, and `docs/decomp_ledger.csv` therefore does not tag this class as placeable. The binary still registers class id `3GOD` from the `DOG3` immediate site:

| Evidence | Value |
|---|---|
| Class-id scan row | `DOG3  3GOD  @0041c8cf  FUN_0041c810  C3DGoddard()` |
| Constructor registration | `push 0x33474f44` at `0041c8cf` |
| Schema references | Other rows use strings such as `C3DGODDARD`, `GOGODDARD`, `PUTGODDARD`, and `GODDARDDIS` in AI/camera/cutscene wiring. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3GOD` | Class id registered by Goddard constructor. | `0041c8cf`; `_gam_classids.tsv` |
| `C3DGoddard()` | Constructor/class string. | `.data:004ee970`; constructor `0041c810` |
| `C3DGODDARD` | Referenced object/tag string in `.gam` camera/AI data. | `docs/gam_schema.md` references |
| `3RED`, `3PIC` | Runtime lookups for red neutron and pickup targets in the custom integrator. | `0041d6ea`, `0041d8a7` immediates in `0041d0c0` |
| `LV1C`, `LV1D`, `LV4A`, `LV6A`, `LEV6`, `LEV7`, `VR06` | Level gates in `PostLoadGoddard`. | raw `0041e590` |
| `1500.0`, `3500.0`, `130.0` | Level tracking range defaults and inherited range/scale call. | `0041e590` |
| `0xcc` | Effect/sound id allocated by `ToggleGoddardModeEffect(true)`. | `0041dcd0`; `FUN_00458980(-1, 0xcc, 1)` |
| mode ids `0..5` | Goddard vector/mode selector. | `0041cc30`, `0041e720`, `0041e9e0` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `goddard02.png` | `0041cd70`; string `.data:004ee980` | Loaded into texture slot `0`. |
| animation | `HIFLY` -> `godfly.ASE` | `0041cd70` | Flight animation. |
| animation | `HIEAT` -> `godeat.ASE` | `0041cd70` | Eating animation. |
| animation | `HIRUN` -> `godrun.ASE` | `0041cd70` | Run animation. |
| animation | `HISIT` -> `godsit.ASE` | `0041cd70` | Sit animation. |
| animation | `HISCOOT` -> `godscooter.ASE` | `0041cd70` | Scooter/ride animation. |
| animation | `HIWAG` -> `godwag.ASE` | `0041cd70` | Tail wag animation. |
| animation | `HIGROWL` -> `godgrowl.ASE` | `0041cd70` | Growl animation. |
| animation | `HIROCK` -> `godrocket.ASE` | `0041cd70` | Rocket animation. |
| animation | `HIPOINT` -> `godpoint.ASE` | `0041cd70` | Point animation. |
| animation default | `WALK` / inherited `C3DAI` strings | `0041cd70`, `0041d0c0`, `0041dee0` | The update path switches among inherited `WALK`, `RUN`, and `FLY` aliases through the C3DAnimated selector. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe`, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Apply full `C3DGoddard` structs so primary-pointer fields and outer vtable-4 fields stop overlapping ambiguously around `0x904..0x9c8`.
- Resolve the constructor-created helper object classes at primary offsets `0x8d4`, `0x904`, and `0x908`.
- Name the six Goddard modes and the global static vectors behind `DAT_004f81b0..DAT_004f8208`; they look runtime-initialized or BSS-backed in the PE image.
- Identify whether effect id `0xcc` is sound, particle, or another runtime handle class.
- Runtime-check which `PostLoadGoddard` branch corresponds to visible/active versus hidden/disabled before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DGoddard /tmp/decomp_C3DGoddard.md` (`slots=396`, `owned_methods=5`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DGoddard_funcs.md`, and local `objdump` windows over `0041c810..0041ee20` plus `0042ab50`.
- Goddard's `3GOD` registration is present in the binary even though the current `.gam` corpus has no direct object rows for it; treat it as a runtime companion object unless later runtime validation finds hidden placement data.
