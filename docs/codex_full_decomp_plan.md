# Full-Tier Decomp Plan — All 208 Gameplay Classes (Neutron.exe)

> ✅ **DONE / HISTORICAL (2026-06-22).** All 208 specs are at `status=spec`; this plan's
> deliverable is complete. Its **successor is [`native_port_plan.md`](./native_port_plan.md)**,
> which consumes the specs to build runtime behavior in the native C engine. **Note the
> framing below is superseded:** the Godot game it calls "the primary artifact" was *retired*
> — the C engine in `src/` is the product. Read this only for how the spec corpus was built.

> **Audience: Codex.** This is a shared, committed plan (per the anti-silo policy in
> `~/CLAUDE.md`). Claude scoped it on 2026-06-07; Codex executes it. Keep the **ledger**
> (`docs/decomp_ledger.csv`) and `docs/PROJECT_HISTORY.md` updated as you go — that is the
> resumable state both agents read. Do **not** keep progress only in tool-private memory.

## 0. What this is

Reverse-engineer **every gameplay class in `Neutron.exe`** into a faithful, documented
behavioral spec (and reference notes), so the Godot game (the primary artifact, per
[`godot_bridge_plan.md`](./godot_bridge_plan.md) §8) can re-express each one without
guessing. This is the **full tier**: all **208 `C*` classes**, not just the playable core.

**Measured scope (2026-06-07, Ghidra):**
- `Neutron.exe`: **2,350** functions (716 thunks, 174 "big" ≥400B), **208 `C*` gameplay classes**.
- `OMT2.dll`: 3,512 functions, 110 `OMedia*` **engine** classes — **out of scope** here
  (already RE'd in Phases 2/11; it is *replaced* by Godot/glTF, not ported). Touch it only
  to resolve a vtable call that lands in the engine.

**Already extracted, do NOT re-derive from the binary** (bounds the work):
assets (4,900+ catalogued), level layout/placements (`.gam`), player/physics constants
(`.gam`, registrar confirmed faithful — see [JN data-driven physics] + `gam_loader.c`),
HUD layout, render pipeline.

**The `.gam` accelerator (read this — it changes the per-class workflow).** The `.gam`
files are a generic named-property serialization: per object, a FourCC + the exact
properties that class registers in `InitObject` (`vtable+0x3fc`). Harvested across all 35
levels that is **93 placeable object types, 3,299 instances**, each carrying 14–63 named
props with real values — i.e. the **field map, constants, and object wiring** for every
*placeable* class, by name, for free. Artifacts (committed):
- [`gam_schema.md`](./gam_schema.md) — full per-FourCC property tables (name, type, value
  range/samples; ✓ = already read by `gam_loader.c`, ✗ = dropped tuning/wiring) **plus the
  FourCC↔class map** (52/93 named, rest with `InitObject fn` pinned).
- `docs/_gam_classids.tsv` — raw Ghidra class-id scan; regenerate the doc with
  `python3 tools/gam_schema.py` (reads it). Scanner: `tools/ghidra/Scan_ClassIds.java`.

Consequence: for a placeable class the RE job shrinks to recovering the **consuming logic**
(the update method), not the data layout or parameter meaning. The full trigger/cutscene/
activation graph (`ActivateButton`→`ObjectTag`, `NextTrigger`, `ToggleObject`, `AITarget`,
`CameraTarget0..7`) is already in the data — no RE needed for wiring. **Hierarchy note from
the scan:** `3FLY` = `C3DFlyingObject` (`FUN_00419f70`) is the **movement base** (registers
MaxSpeed/UpRate/DownRate/NewGravity/lean) that C3DPlayer/C3DJimmy inherit — do this base early.

**Capture (`.omtc`) is the complement:** it carries runtime dynamics `.gam` can't — per-frame
`SetTransform(WORLD)`, animation timing, AI paths *as executed*, real cutscene camera motion.
Its role here is **validator**: drive the RE'd logic with `.gam` params, diff produced motion
against captured frames (same method used to validate the player jump arc).

## 1. Definition of done (per class)

A class is **DONE** when `docs/decomp/<ClassName>.md` exists and contains:
1. **Identity** — RTTI name, vftable address(es), ctor/dtor addresses, base-class chain.
2. **Field map** — every used object offset → name + type + meaning (and source: `.gam`
   property name if registered, else inferred). Reuse base-class fields, don't redocument them.
3. **Vtable method table** — one row per slot: address, inferred name, 1-line behavior,
   "trivial / non-trivial / TODO".
4. **Per-frame behavior** — pseudocode of the update/animate method(s): what it reads,
   what it writes, state machine, timers (× dt), interactions with other classes.
5. **Constants** — `.gam` property names it registers (with offsets + type code).
6. **Assets** — `.ase`/canvas/sound names it references.
7. **Confidence** — High / Medium / Low + open questions, and whether runtime-validated.

A reference C port is **optional** and only worth writing for behaviorally non-trivial
classes; the **spec markdown is the durable deliverable**. (Godot porting is downstream and
not part of this plan's "done".)

## 2. The leverage idea: hierarchy first

208 classes form an inheritance DAG. Decompile **shared base behavior once**; every leaf is
then a thin override. Order of work is therefore strictly: **bases → families → leaves.**
Never start a leaf before its bases are DONE.

## 3. Tooling (Ghidra headless)

Project: `~/ghidra-projects/JN_decomp` (programs `Neutron.exe`, `OMT2.dll`). JDK at `~/jdk21`.
Scripts live in `~/ghidra-scripts/` (Claude left `Physics_*.java`, `Scope_Count.java` as
working templates — generalize, don't restart).

Run pattern:
```bash
JAVA_HOME=~/jdk21 PATH=~/jdk21/bin:$PATH \
  ~/ghidra/support/analyzeHeadless ~/ghidra-projects JN_decomp \
  -process Neutron.exe -scriptPath ~/ghidra-scripts -postScript <Script>.java [args]
```

**Important calling-convention caveats** (Claude hit these):
- Methods are `__thiscall` → `this` arrives in **ECX**; the decompiler shows it as
  `int in_ECX` until the function signature/convention is applied. So `*(float*)(in_ECX +
  0x624)` is `this->UpRate`. **Apply class structs (Phase 0.3) to make this readable.**
- A "callee" decompiling to an empty `return;` is usually a **release-build debug-trace stub**
  (e.g. behind a `"Playing Sprite %d"` string) — ignore it.
- `-noanalysis -readOnly` is fine for *reading*, but the campaign should **save** progress
  (renames, applied structs) so it compounds — see Phase 0.
- A `.rdata` **DATA** xref to a function = a **vtable slot** (that's how Claude confirmed the
  player integrator). Use this to map vtables.

Scripts to build in Phase 0 (generalize the `Physics_*` templates):
- `DumpHierarchy.java` — walk RTTI (Class Hierarchy Descriptors) → emit every class's
  base-class chain + vftable address(es) → `/tmp/jn_hierarchy.txt`.
- `DumpClass.java <ClassName|vftableAddr>` — list the vftable slots, decompile each method,
  and dump every distinct `this+offset` access → one file per class.
- `FieldMap.java <vftableAddr>` — aggregate offset accesses across the class's methods with
  read/write and the inferred width (byte/word/dword/float).

## 4. Phases

### Phase 0 — Recon & infrastructure (do once; ~2–4 days)
0.1 **Verify analysis is complete** and run the **Microsoft RTTI analyzer** on `Neutron.exe`
    so functions get grouped under class namespaces and **vftables are labeled**. This single
    step auto-assigns most of the 2,350 functions to their classes — the backbone of the whole
    effort. **Save the project afterward.**
0.2 `DumpHierarchy.java` → the full inheritance DAG. Commit as `docs/decomp/_hierarchy.md`.
0.3 For each class, build a Ghidra **structure type** from its field map and apply it to the
    `this` parameter of its methods (so decomp reads `this->UpRate` not `in_ECX + 0x624`).
    Do bases first; leaves inherit the layout. **Save.** *(This is the single biggest
    readability multiplier — budget real time here.)*
0.4 Generate the **ledger** `docs/decomp_ledger.csv` with one row per class:
    `class, base_chain, vftable, ctor, n_methods, family, wave, status, owner, confidence, notes`.
    `status ∈ {todo, in_progress, spec, ported(optional), validated}`. This is the resumable
    state. Commit it.
0.5 Write `docs/decomp/_TEMPLATE.md` (the §1 structure) and `docs/decomp/README.md`.
0.6 **Wire in the `.gam` accelerator (already built).** `gam_schema.md` + the FourCC↔class
    map exist. In the ledger, tag each class `placeable` if its FourCC appears in the schema
    (93 of them) — these get §1 deliverables 2/5/6 (field map / constants / wiring)
    **pre-filled from the schema**, so their wave work is *logic-only*. The other ~115
    (bases, code-spawned effects/projectiles, level controllers) are full RE. When the
    Phase-0.1 RTTI analyzer names the `InitObject fn`s, back-fill the 41 unnamed FourCC↔class
    rows and re-run `python3 tools/gam_schema.py`.

### Phase 1 — Base / framework classes (~1 week)
The high-leverage roots. Suggested set (refine from 0.2's real DAG):
`CGameObject`, `CLocalGameObject`, `C3DObject`, `C3DOmtObj`, `C3DAIOmtObj`, `C3DAnimated`,
`C3DSprite`, `C3DAnimatedSprite`, `C3DSpriteType`, `C3DPermanentSprite`, `CGameType`,
`C3DVehicle`, `C3DEnemy`, `C3DAI`, `C3DTrigger`, `C3DTriggerType`, `C3DPickupItem`,
`C3DPickupType`, `C3DProjectile`, `C3DCamera`, `C3DCursor`, `CViewPort`, `CGfx`.
DONE here means the common update loop, message/event dispatch, and lifecycle are documented
so leaves can say "inherits update from `C3DAnimated`, overrides only X".

### Phases 2–9 — Leaf families (the bulk; ~2.5–3.5 months)
Work in **waves**, one family per wave, leaves only after their base is DONE. Assign each
class a `wave` in the ledger. Suggested families (counts approximate; reconcile with 0.2):

| Wave | Family | Representative classes |
|---|---|---|
| 2 | Player & friends/NPCs | `C3DPlayer` (start here — Claude already located its integrator `FUN_0041a140`, constant registrar `FUN_00419f70`, anim table `FUN_00422ac0`), `C3DJimmy`, `C3DNeutron`, `C3DRedNeutron`, `C3DSheen`, `C3DCarl`, `C3DLibby`, `C3DCindy`, `C3DGoddard`, `C3DJudy`, `C3DHugh`, `C3DNick`, `C3DBenny`, `C3DKitty`, `C3DHumphrey`, `C3DFriends`, `C3DPirate`, `C3DFleetCommander`, `C3DSumo`, `C3DAbductee` |
| 3 | Enemies & AI | `C3DEnemy`, `C3DEnemyAircraft`, `C3DAICar`, `C3DAISuv`, `C3DAITrigger`, `C3DUltraLord`, `C3DYokian`, `C3DYokianGuard`, `C3DYokianSoldier`, `C3DYokianSpy`, `C3DYokianShield`, `C3DYokianShip`, `C3DYokTurret`, `C3DPod`, `C3DTank`, `C3DHarrier`, `C3DMissile`, `C3DMine`, `C3DTesla`, `C3DShrinkRay`, `C3DLaserTrigger` |
| 4 | Vehicles | `C3DCar`, `C3DNeuCar`, `C3DNeuCar2`, `C3DBus`, `C3DJeep`, `C3DSub`, `C3DSailBoat`, `C3DSkateBoard`, `C3DRocketShip`, `C3DRocket`, `C3DDigger`, `C3DWheel` |
| 5 | Pickups & items | `C3DBalloon`, `C3DBaseball`, `C3DBaseballPickup`, `C3DBubble`, `C3DBubblePickup`, `C3DMetalPickup`, `C3DHelmet`, `C3DPasscard`, `C3DRocketFuel`, `C3DVRTrophy`, `C3DGraplingHook`, `C3DHook` |
| 6 | Mechanisms & moving parts | `C3DMover`, `C3DMovingTarget`, `C3DPendulum`, `C3DMerryGo`, `C3DFerris`, `C3DFerChair`, `C3DValve`, `C3DSwitch`, `C3DButton`, `C3DGate1`, `C3DSwingDoor`, `C3DSchoolDoor`, `C3DDoorUpDown`, `C3DGeyser`, `C3DSteamVent`, `C3DFan`, `C3DTractorBeam`, `C3DYokBigdoor`, `C3DYokCargo`, `C3DYokDoor`, `C3DYokHelmet` |
| 7 | World props & terrain | `C3DTree`, `C3DRock`, `C3DBush`, `C3DCactus`, `C3DLeaves`, `C3DFlag`, `C3DHydrant`, `C3DTrashCan`, `C3DToolChest`, `C3DPhoneBooth`, `C3DGrill`, `C3DCube`, `C3DSphere`, `C3DCone`, `C3DStalagtite`, `C3DSky`, `C3DTerrain`, `C3DShadow`, `C3DLight`, `C3DLightObj`, `C3DLightCone`, `C3DPolygon` |
| 8 | Effects, triggers, nav, cameras, sound | `C3DSmoke`, `C3DSmokePuff`, `C3DNewSmokePuff`, `C3DFireStrato`, `C3DJetpackFire`, `C3DSparkWire`, `C3DCorona`, `C3DTeleportFX`, `C3DCheckPoint`, `C3DWayPoint`, `C3DPatrolPoint`, `C3DStartPoint`, `C3DMusicTrigger`, `C3DSoundEffect`, `C3DArrow`, `CTaskList`, `CTrigger`, `CTriggerTimer`, `C3DCutSceneCamera`, `C3DMultiCutSceneCamera`, `C3DTargetCursor`, `C3DPointCursor` |
| 9 | Creatures & one-off set dressing | `C3DFowl`, `C3DChick`, `C3DSparrow`, `C3DCamel`, `C3DDino`, `C3DDarwinFish`, `C3DMutantFish`, `C3DOctapuke`, `C3DGirlEatingPlant`, `C3DEye`, `C3DBubble`, `C3DCorona`, etc. (lowest behavioral value — spec only, no port) |

### Phase 10 — Level/game controllers (~3–4 weeks)
`CJimmyGame`, `CLoadLevel`, `CMainMenu`, `C2DInGameMenu`, `CMenuElement`, `CEditor`,
`CAweReal`, `C3DLabScreen`, and the ~40 `CLevelNNxGame` / `CLevelVRNN`. These are highly
**repetitive**: spawn tables, win/lose conditions, trigger wiring. Decompile `CLevel01AGame`
*thoroughly* as the exemplar, then the rest are diffs against it (note only what differs).
Cross-check each against its `.gam` file — the controller is the runtime glue over `.gam` data.

## 5. Per-wave workflow (Codex's inner loop)

For each class in the active wave:
1. `git pull` / read the ledger; set `status=in_progress, owner=codex`.
2. **If `placeable`:** open `gam_schema.md` for its FourCC and pre-fill the doc's field
   map / constants / wiring from the property table (names, types, value ranges, links).
   Now you know *what* the class is tuned by before reading any code.
3. `DumpClass.java <ClassName>` → decompile vtable slots + field map; reconcile the offsets
   against the `.gam` property names (the registrar maps name→offset).
4. Resolve unknown callees one level deep (skip engine `OMedia*` unless behaviorally relevant).
5. Write `docs/decomp/<ClassName>.md` from the template; link base-class doc with `[[...]]`.
6. Set `status=spec, confidence=…`, list open questions.
7. **Commit** per class (or per small batch): `decomp(<Class>): spec from Neutron.exe`.
8. At end of wave: append a 1-paragraph era entry to `PROJECT_HISTORY.md` and update the
   ledger summary counts.

## 6. Validation

Full runtime validation of 208 classes across 22 levels is infeasible and not required.
Tier it:
- **Spec review (always):** Alex gates each wave's specs (the project's standing human QA gate
  — see [autonomy & effort checkpoints]). Batch specs for review at wave boundaries.
- **Runtime validation (where it pays):** for classes with capture coverage, validate against
  the Level-1 `.omtc` (the fidelity oracle); for visible gameplay (player, main enemies,
  vehicles), spot-check against the original via XP noVNC. Mark `status=validated` only then.
- **Don't relitigate invariants** (matrix convention, `PROJ[3][3]=1`, no X-mirror,
  `canvas_id=Canv+1`, DIFFUSE alpha 0, no fog) — see `PROJECT_HISTORY.md` §Invariants.

## 7. Anti-silo / handoff rules (mandatory)

- The **ledger** (`docs/decomp_ledger.csv`) + per-class docs + `PROJECT_HISTORY.md` are the
  only source of truth. Anything in Codex-private memory does not count as remembered.
- Commit frequently; small, per-class commits keep the effort resumable and reviewable.
- If you improve a Ghidra script, commit it to a tracked `tools/ghidra/` copy (don't leave it
  only in `~/ghidra-scripts/`, which is machine-local and Claude-visible only by accident).
- When you finish Phase 0.1–0.3, **the Ghidra project itself now carries class structs +
  names** — note in the ledger header that the project is "annotated as of <commit>" so the
  next session doesn't redo it.

## 8. Estimate & cadence

Calibration: Claude located+scoped `C3DPlayer` in ~1h; a *faithful spec* of a non-trivial
class is ~0.25–1 day, a trivial prop ~1–2h, the player/boss/AI classes ~1–3 days each.

| Phase | Scope | Estimate |
|---|---|---|
| 0 | Recon, RTTI, structs, ledger | 2–4 days |
| 1 | ~23 base/framework classes | ~1 week |
| 2–9 | ~120 leaf classes (8 waves) | ~2–3 months* |
| 10 | ~50 level/game controllers | ~2–3 weeks* |
| **Total** | **all 208, spec tier** | **~3.5–5 months solo** |

\* The `.gam` accelerator (§0) cuts Waves 2–9 and Phase 10: ~93 of the leaf/placeable
classes get their field-map/constants/wiring pre-filled from `gam_schema.md`, so per-class
work drops to logic-only, and the level controllers are mostly `.gam`-data glue. Pre-`.gam`
this tier was estimated ~4–6 months; the harvested schema is why it's now ~3.5–5.

Optional reference C ports for the non-trivial subset add ~3–6 weeks. Parallelizing waves
across agents compresses wall-clock, but the **spec-review gate (Alex) does not parallelize** —
that's the real throughput limit, so keep waves review-sized (~10–20 classes).

## 9. Risks / guardrails
- **Decompiler noise** under `__thiscall` — Phase 0.3 structs are the fix; budget for it.
- **False positives** from offset/immediate scans (Claude's integrator search returned
  C3DSmoke/pickup registrars before the real one) — always confirm with the decompiled body,
  not just the heuristic.
- **Scope creep into OMT2.dll** — resist; document the engine call by its `OMedia*` name and
  move on.
- **One-off set dressing** (Wave 9) — spec only; do **not** sink days into `C3DDarwinFish`.
- **Don't break the Godot-artifact split** — this plan produces *specs/foundry knowledge*,
  not engine forks. Porting decisions stay in `godot_bridge_plan.md`.

## 10. Additional data sources beyond `Neutron.exe` (surveyed 2026-06-07)

A full file-type survey of both game installs (`~/xp-jnbg-original`, `~/jnvsjn-original`)
turned up sources we have **not** yet used. Fold these in at the noted phases.

- **`.tsk` — the `CTaskList` / progression layer (HIGH value, untouched).** `NewGame.tsk`
  (JNBG, magic `LV1BA`) is the new-game template; the sequel's `JimmyGame1–10.tsk` are saves.
  It holds the **mission/objective + initial-object-state** data, keyed by the same
  `ObjectTag`s as the `.gam` files (seen: `LIBBY`, `DINO`, `CLONE`, `KITTY1-3`, `REACTOR`,
  `STARTEXP`, `SCENE`) and references `level1b.gam`. This is the *other half* of the `.gam`
  data contract — it's what the `.gam` `TaskName`/`NewTaskState` props point at. It is **not**
  the generic `.gam` property format; write a `tools/tsk_parser.py` + format note as a
  prerequisite to **Phase 10** (level controllers / `CTaskList` / `CGameType`).
- **`NeutronSW.exe` + the OMT2 `OMediaWin*` classes — the software renderer (HIGH value).**
  Both games ship a SW-render build (`NeutronSW.exe` differs from `Neutron.exe` in ~535 KB —
  a real separate build). The CPU rasterization path lives in OMT2.dll's `OMediaWin*` classes
  (`OMediaWinVideoEngine`, `OMediaWinOffscreenBuffer`, …), which Phase 2/11 never read (they
  followed the `OMediaDX*` DirectDraw path). It implements the **exact transform / projection
  / lighting / rasterization in readable C** — a second, independent ground-truth that can
  settle rendering-invariant questions the `.omtc` capture only shows indirectly. This is the
  **one sanctioned exception** to the "don't read OMT2.dll" rule, used as a *fidelity
  cross-check*, not a port target.
- **`menu.dat` (sequel, plaintext) — front-end menu layout.** Rows like
  `MENUITEM,MMVRTANK,MAIN_MENU,x,y,...,index` map menu items → screen/level indices. Feeds
  `CMainMenu` / `CMenuElement` / `C2DInGameMenu` in **Phase 10**.
- **Sequel binaries (medium — only if scope extends to JNvsJN).** `Neutron2.exe` (1.33 MB) is
  the analog of `Neutron.exe` on the **same OMT2 engine**; its class set overlaps JNBG + adds
  Granny/sequel classes — diff shared classes rather than re-RE. The sequel's `OMT2.dll` is
  *updated* (~28 KB bigger, Granny integration); a binary diff vs JNBG's isolates the changes.
- **Confirmed cruft (ignore):** `readme.txt`, `options.opt` (16 B), `.isu`, and the entire
  `_disc/` + `_installshield/` trees (`DATA.TAG`, `lang.dat`, `os.dat`, `SETUP.INI`,
  `setup.lid`, `layout.bin`, `AUTORUN.DAT`, `.cab/.hdr/.ins/.inf/.ex_/.ico`) — InstallShield.

---
*Created 2026-06-07 (Claude). Living doc — Codex updates status here and in the ledger.*
*Cross-refs: `PROJECT_HISTORY.md`, `ARCHITECTURE.md`, `godot_bridge_plan.md`, `ghidra_notes.md`.*
