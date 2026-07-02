# Ghidra Function-Recovery Plan (open new L1s)

Written 2026-07-02, at the close of the `linked` branch's static sweep
(10 linked / 11 linked-blocked). Every remaining certifiable row is now
blocked on the same thing: **entry points that exist as code in `Neutron.exe`
but are not function-defined in the committed Ghidra project**
(`~/ghidra-projects/JN_decomp`), so `DumpFunctions.java` reports
`(function not found)` and the class docs carry prose from raw `objdump`
instead of recovered bodies. This plan opens those L1s.

> Infrastructure: Ghidra at `~/ghidra`, headless invocation pattern in
> `docs/codex_full_decomp_plan.md` (§ analyzeHeadless), scripts in
> `tools/ghidra/` (`DumpClass.java`, `DumpFunctions.java`, etc.).

## Target inventory (priority order)

Progress note (2026-07-02): target 1 (`00472980`) is now function-defined and
dumped by `tools/ghidra/CreateFunctions.java`; see
`docs/decomp/evidence/transform_local_00472980.md`. It opened L1 but did not
make the existing native cutscene placement certifiable, because native still
uses a yaw-only `entity_local_to_world` helper instead of the recovered
three-axis transform. The full-placement aspects are recorded as
`linked-blocked` in `docs/linkage_certificates.csv`.

| # | Entry points | Why / what it unblocks | Hardness |
|---|---|---|---|
| 1 | `transform_local` vtable `+0x384` slot (resolved addr `00472980`, tried 2026-07-02) | Blocks the exact orbit/dolly camera position for BOTH cutscene rows (`C3DCutSceneCamera`, `C3DMultiCutSceneCamera`) — highest-leverage single function; would also retire the native `entity_local_to_world` yaw-only approximation | Medium — one function, known address |
| 2 | CLoadLevel gate caller: find who evaluates `Radius`/`RequiredTask`/`RequiredLevel`/`ExactLevel` and calls `ActivateLoad` (`00458370`) — xref hunt, then define/decompile the caller | Reopens `CLoadLevel`/activate-load; also the ground truth for `behavior_base.c`'s level-window gate used by many behaviors | Medium — xref hunt, unknown boundary |
| 3 | C3DPlayer movement/anim bodies: `00437c40` (UpdateGroundMoveA), `00437f90` (UpdateJumpFallMove), `00438bc0`/`00439900` (UpdateWalkingCameraA/B), `0043aff0` (SetPlayerAnimationState); plus the doc's remaining raw list (`00437890`, `0043a120`, `0043a420`, `0043a5d0`, `0043a790`, `0043a7f0`, `0043b5a0`, `0043b820`) | The L1 half of player movement-logic (see `C3DPlayer.md` Native Linkage). NOTE: even with L1 recovered, certifying requires a 1:1 port replacing the approved tank-turn movement — a product decision staged for together, not autonomous work | Hard — large helpers, interleaved camera/movement state |
| 4 | Menu-manager slots: `LoadMyMenu`/`displayMenu`/activation graph (trace strings at `.rdata:004ec620`+), `CMenuElement::UpdateItemLogic` target dispatch | Opens the menu state-graph aspects (`CMainMenu`, `CMenuElement`, `CGameType` pause/help) beyond the routing table certified today | Medium-hard — screen-graph state across objects |
| 5 | `C3DTriggerType::RunTriggerTypeNextTarget` cleanup (body exists but "still raw decompiler output") + TRIG RTTI resolution (`gam_schema.md` "name pending Phase 0") | Reopens `CTrigger`/enter-exit-latch family | Medium |
| 6 | C3DJimmy: repair raw function boundaries for `UpdateJimmyFrame`/`UpdateJimmyActiveController` | Gadget/vehicle-mode dispatch, `0xa18` gadget controller path (inventory domain) | Hard — explicitly flagged in the plan as needing a stronger model or human |
| 7 | C3DAnimated event->anim dispatch (anim-ended hooks `0040e050`/`0040dd90` area) | The 53-row animation-dispatch batch (wave 7's other half) | Unknown until probed |

## Method (per target)

1. **Define + dump (mechanical).** New script `tools/ghidra/CreateFunctions.java`:
   `disassemble(addr)` if needed, `createFunction(addr, name)`, then decompile
   and dump — same output format as `DumpFunctions.java`. Run headless against
   `JN_decomp`. Commit the `.rep` project change + raw dump under
   `/tmp/dumps*` -> `docs/decomp/` evidence notes.
2. **Interpret (the hard 20%).** Deepen the class doc from prose to a recovered
   body: control flow, field/offset semantics, constants with addresses. This
   is L1; it gates everything.
3. **Transcribe + prove (established loop).** 1:1 native port where the project
   wants one + `tools/linkage_oracles/<Class>.py` on the existing template +
   mutation test + gate. Rows whose native side is a deliberate design
   (player tank-turn, native HUD) stop after step 2 with the L1 recorded —
   porting them is a native-port/product decision, not a linked-branch task.

Discipline unchanged: every claim lands in the class doc + certificates CSV;
gate + `make` + `audit_faithfulness` green per commit; no gameplay/QA.

## Model assignment

- **Sonnet 5 handles:** step 1 wholesale (script writing + headless runs are
  runbook work), step 3 wholesale (the plan already assigns the
  transcribe+prove loop to Sonnet; ten exemplar oracles now exist), and
  target 1 end-to-end if the decompiled body comes out clean (single known
  address, oracle template ready).
- **Fable 5 (medium effort) handles:** step 2 for targets 2–7 — interpreting
  raw decompiler output into field-semantic bodies, boundary repair (C3DJimmy
  is explicitly flagged), xref hunts with judgment (the gate caller), and all
  certify-vs-blocked dispositions. Medium effort has been sufficient for
  every judgment call this branch has needed; escalate to high only if a
  body resists interpretation, not preemptively.
- **User decides:** whether a recovered player movement body ever replaces
  the approved tank-turn movement (step 3 for target 3).
