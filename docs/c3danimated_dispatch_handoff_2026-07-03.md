# C3DAnimated dispatch port — session handoff (2026-07-03)

For the next agent session (Opus) continuing the `C3DAnimated`
event-animation-dispatch port on the `linked` branch of `~/jn-engine`.
Read this INSTEAD of re-deriving; everything below is evidence-backed and the
sources are cited. Companion docs: `docs/c3danimated_dispatch_port_plan.md`
(the plan), `docs/c3danimated_dispatch_fresh_prompt.md` (the original kickoff),
`docs/decomp/C3DAnimated.md` (updated this session), and
`docs/decomp/evidence/c3danimated_target7.md` (L1 raw dumps).

## Where the session stopped

Done, committed:

1. Commit `5c419c8` — the plan + fresh-prompt docs (previously untracked).
2. Commit (this handoff's companion) — `docs/decomp/C3DAnimated.md`
   corrections from the OMT-source resolution (field map, vtable rows 56/57
   and 241, per-frame pseudocode, target-7 key-composition phrasing), plus
   this handoff.

Baseline gates were run and are green: linkage gate 14 linked / 15
linked-blocked, `make` clean, `audit_faithfulness.py` 0 findings. Gate reruns
churn only the `Generated <date>` line of `docs/linkage_progress.md` and
`docs/vtable_linkage_audit.md` — `git checkout --` them (done once already).

NOT done yet (the remaining work, in commit order):

1. **Finish Phase 1**: replace the `## Native Linkage` section of
   `docs/decomp/C3DAnimated.md` with the full map — a complete drafted outline
   is in "Drafted Native Linkage content" below. Also update the doc's Open
   questions (see below). Commit as docs-only.
2. **Phase 2**: `src/game/animated_dispatch.h/.c` + one-line `Entity` field +
   `tools/linkage_oracles/c3danimated_dispatch_dump.c` +
   `tools/linkage_oracles/C3DAnimated_dispatch.py` (design fully specified
   below). Commit module+oracle together, no runtime wiring.
3. Append a PROJECT_HISTORY.md entry; rerun all gates; restore date churn.
4. Do **NOT** flip `docs/linkage_certificates.csv` — the plan's commit split
   puts the flip after runtime wiring (steps 4-6). Optionally append to the
   row-11 note that the module+oracle landed.

## The decisive discovery: OMediaAnim resolved from OMT 2.5 source

`~/omt-src/open-media-toolkit-master/sources/OMTClasses/World/Anim/OMediaAnim.{h,cpp}`
(the same LGPL source already used to recover `OMedia3DVector::angles` for the
walkcam row). The embedded `OMediaAnim` sits at adjusted `+0x90`. VC6 member
layout ⇒ adjusted offsets: `+0x94 anim_def`, `+0x98 current_frame`,
`+0x9c next_frame`, `+0xa0 current_sequence`, `+0xa4 updatecount`,
`+0xa8 pause_count`, `+0xac play_timebased`, `+0xad play_loop`,
`+0xae play_reverse`, `+0xaf play_started`, `+0xb0 play_pingpong`,
`+0xb4 current_frame_tbcount`. Vtable (OMediaClassStreamer base = slots 0-2
dtor/read_class/write_class): `+0xc set_anim_def`, `+0x10
setcurrentsequence(long, bool restart)`, `+0x1c getcurrentframe_pos()`,
`+0x24 setplay_timebased(bool)`. All three call sites in the recovered bodies
match these slots exactly.

Consequences (all now reflected in `docs/decomp/C3DAnimated.md`):

- **The completion gate is `play_loop`, not pause.** `UpdateAnimated`'s byte
  check `this1[-5]+1` = adjusted `+0xad` = `play_loop` (`this1 = adjusted4 +
  0xc0`, proven by the slot-65 call through `this1[-0x30]+0x104` and the
  record read `this1[0x15b]+0x48` = adjusted `+0x62c`). The AnimEnded hook
  fires only for **non-looping** clips. `+0x654` is only `SetAnim3DPaused`'s
  edge-guard mirror; pause stops frame advance (`update_logic` returns while
  `pause_count != 0`) but NOT the completion check → a paused one-shot already
  at its last frame keeps firing.
- **`SetAnim3DByName(name, loop_flag)`** — the second arg forwards to slot 57
  which calls `setcurrentsequence(rec->id, loop_flag)` (loop doubles as the
  `restart` bool), writes `play_loop = loop_flag`, then forces
  `setplay_timebased(true)` (which resets `play_started` → timer phase reset).
- **No completion latch.** `advance_frame` holds at the last frame for
  non-loop clips (returns "can't advance"); the hook re-fires every update.
  `C3DPlayer::OnPlayerAnimEnded` self-latches by switching anims.
- **Re-selection semantics.** `setcurrentsequence` no-ops for the same
  sequence; `set_anim_def` resets frame/sequence only when the def pointer
  changes. Selecting a *different* record ⇒ frame cursor resets to clip start
  (via the def swap; note the original applies the def AFTER selecting the
  sequence, so the sequence selection runs against the OLD def and gets
  clamped/clobbered — collapses away under single-sequence defs, see
  deviation 1). Re-selecting the *same* record ⇒ frame position preserved,
  timer phase reset, loop flag updated, clock zeroed.
- **`anim_clock` mystery solved**: adjusted `+0x584` (`this4[0x161]`, zeroed
  unconditionally by `SetAnim3DByName` after its ready-flag guard) is the same
  field `UpdateAnimated` accumulates `dt` into (`this1[0x131]`). So the clock
  resets on every lookup attempt — found or NOT — while a missed lookup leaves
  the current record/selection untouched.
- **Frame-count refresh** (slot 57): `rec->frame_count(+0x48) =
  def->sequences[current_sequence].size()` — VC6 Dinkumware `list` layout
  `{allocator, head, size}` puts size at `+8`, matching the raw dump's
  `*(seq_ptr + 8)` read. Refresh only runs when current record + def + a
  non-empty sequences vector exist.
- **`getcurrentframe_pos()` returns 0** when there is no def/current frame —
  so a selected record with `frame_count` 0 fires the hook immediately
  (`0 - 1 <= 0`). Single-frame one-shots fire on their first update too
  (`advance_frame` returns true for size==1; pos 0, fc 1).
- **`update_logic` time-based walk** (transcribe verbatim, forward-only):
  priming tick (first update after `play_started=false` only loads
  `tb_count = ms_per_frame`, no advance), then per update:
  `mel = elapsed_ms; loop { if (mel >= tb_count) { mel -= tb_count; if
  (advance_frame()) { tb_count = ms_per_frame; break; } tb_count =
  ms_per_frame; if (!tb_count) break; } else { tb_count -= mel; break; } }`,
  all guarded by `ms_per_frame != 0`. `omt_WMilliSec` is a **float** typedef
  (`OMediaWorldUnits.h:41`) — no integer truncation anywhere.
  `advance_frame` (forward): NULL frame → resolve to first and fall through;
  size==1 → return true; not at last → ++; at last && loop → 0; else return
  true. `pause(false)` with `pause_count==1` resets `play_started`.
- **Lookup-key composition — the raw dump contradicts the old doc prose.**
  The copy idioms distinguish strcpy (dest = `local_50` start) from strcat
  (dest = end-of-string scan). The shape-suffix copy is a **strcpy**:
  key = `DAT_004ed3dc + name` (mode 0), `DAT_004ed3e0 + name` (mode 1), or
  `DAT_004f81a8 + name` (any other mode word). Mode word = short at adjusted
  `+0x580`, seeded 0 by `InitAnim3DDatabase`. The three globals' contents are
  unrecovered data → native carries them as settable strings. Key buffer is
  `local_50[80]`. In modes 0/1 the caller name is also strcpy'd into the
  visible name buffer (`+0x17a` region) BEFORE the lookup (even on a miss);
  in other modes only the on-success post-lookup copy happens.
- **`CreateAnim3DRecord(name, path)` quirks to preserve**: record appended at
  tail; name buffer initialized with the `DAT_004f81a8` prefix; `id(+0x44) =
  count-at-creation`; on FAILED file-open the function returns early leaving a
  prefix-named record in the list with the id **not consumed** (count++ only on
  success → the next successful record reuses the same id;
  `GetCurrentAnim3DRecord` returns the first id match). On success the
  caller name overwrites the record name, truncated at 63 chars — the original
  truncates the CALLER's buffer in place (don't copy that; deviation 4).
- Guard sets: `SetAnim3DByName` requires `+0x634` then `+0x635` (early return,
  NO side effects). `Find`/`GetCurrent` require both. `SelectIndex` requires
  only `+0x635` and `seq_id >= 0`. `UpdateAnimated`'s completion path requires
  both. The writer of `+0x634/+0x635` is NOT located yet
  (`InitAnim3DDatabase` writes a byte at adjusted `+0x570`); open question —
  native just sets both in init.

## Native module design (agreed, ready to implement)

Files: `src/game/animated_dispatch.h/.c`. The Makefile wildcards
`src/game/*.c` — no Makefile change. `Entity` (in `src/engine/world.h`,
~line 100 near the cutscene fields) gains ONE lazily-allocated pointer:
`struct AnimatedDispatch *anim_dispatch;` + a forward decl. `world_add` uses
`calloc` so it zero-inits. Freeing stays with the module
(`animated_dispatch_free_entity`); nothing allocates at runtime until the
wiring commit, so no leak — note that in the header.

Types (with original-offset comments):

- `AnimatedClip { int frame_count; float ms_per_frame; }` — native stand-in
  for one imported `A3dm`; `ms_per_frame` = `OMediaAnimFrame`
  `getmillisecperframe()`.
- `AnimatedRecord { char name[64]; AnimatedRecord *next /*+0x40*/; int seq_id
  /*+0x44*/; int frame_count /*+0x48*/; const AnimatedClip *clip /*+0x4c*/; }`
- `AnimatedDispatchResult { NOT_FOUND=0 (miss, clock zeroed), SELECTED=1,
  UNCHANGED=2 (guard early-out, no side effects) }`
- `AnimatedDispatch { records, record_count; ready_a /*+0x634*/, ready_b
  /*+0x635*/; shape_mode /*+0x580*/; clock /*+0x584*/; current_index
  /*+0x588*/; const AnimatedClip *current_clip /*+0x58c*/; AnimatedRecord
  *current /*+0x62c*/; char current_name[64] /*+0x17a region*/; play_loop
  /*+0xad*/, paused, play_started, tb_count_ms, cur_frame /* -1 = no frame */;
  void (*on_anim_ended)(Entity*) /* slot 65; NULL = base no-op 00472970 */;
  int anim_ended_fires /* oracle-visible */; }`

API: `init_entity` (idempotent, run-once semantics like `InitAnim3DDatabase`'s
DB-NULL guard: after alloc set `cur_frame=-1`; if ready flags already set,
no-op; else `shape_mode=0; ready_a=ready_b=1`), `free_entity`,
`set_key_strings(suffix_base, suffix_alt, prefix)` (module-level globals),
`create_record(e, name, clip)` (clip==NULL = failed-import shape),
`find_record`, `current_record`, `current_clip`, `select_index(e, seq_id,
loop)`, `set_by_name(e, name, loop)`, `update(e, dt_seconds)` (converts to
float ms for the walk; order: `update_logic` slice FIRST, then `clock += dt`,
then the gate chain `!play_loop && current && (cur_frame<0?0:cur_frame) >=
frame_count-1` → `fires++` + hook), `set_paused(e, p)` (edge-guarded; unpause
resets `play_started`), `set_anim_ended_hook`, `active_alias`, `completed`.

`set_by_name` exact order (from `0040dd90`): guard a then b (return UNCHANGED)
→ `clock = 0` → key strcpy lead by mode + name-buffer pre-copy for modes 0/1 →
key strcat name (80-byte snprintf-bounded) → `find_record` (miss → NOT_FOUND,
nothing else) → `current = rec` → name-buffer copy → `select_index(rec->seq_id,
loop)` → apply clip: `if (current_clip != rec->clip) { cur_frame = -1;
play_started = 0; } current_clip = rec->clip` (the set_anim_def
reset-on-change collapse) → SELECTED. The shape-switch side effects
(`set_shape` + anim-position carry via `0x260/0x264`) are visual, out of
scope, documented.

`select_index` (from `0040da30`): guard `ready_b && seq_id >= 0` →
`current_index = seq_id` → (setcurrentsequence collapses under
single-sequence clips) → `play_loop = !!loop` → `play_started = 0` → refresh
`if (current && current->clip) current->frame_count =
current->clip->frame_count`.

## Oracle design (agreed)

Pattern: follow `tools/linkage_oracles/C3DTriggerType.py` +
`c3dtriggertype_dump.c` — the dumper compiles the REAL unmodified
`src/game/animated_dispatch.c` (`cc -O0 dump.c ../../src/game/animated_dispatch.c -lm`),
drives op lines from stdin, prints one full state line after every op; the
Python side implements an independent reference of the same collapsed contract
transcribed from the L1 dump + OMT source (NOT from the C file), simulates the
same ops, compares line-for-line.

Dumper op protocol (tokens, names contain no spaces): `I` init · `K <sb> <sa>
<pfx>` key strings (`-` = empty) · `M <mode>` shape mode · `G <a> <b>` force
ready flags (guard tests) · `C <name> <frames> <ms>` create record ·
`F <name>` failed-import record · `S <name> <loop>` set_by_name ·
`X <id> <loop>` select_index · `U <ms>` update (dumper converts ms→seconds) ·
`P <0|1>` pause · `H <name> <loop>` install consuming hook that calls
set_by_name(name, loop) (mirrors OnPlayerAnimEnded's return-to-STOP; `H -`
uninstalls) · `L` list records (`R <id> <fc> <hasclip> <name>` lines) ·
`Q` free + fresh entity. State line after every op:
`ST <last_result> <alias|-> <current_index> <cur_rec_id|-1> <cur_rec_fc|-1>
<cur_frame> <completed> <fires> <loop> <paused> <started> <tbbits> <clockbits>`
(float fields as hex f32 bits like the exemplar).

Float exactness strategy: use only power-of-two-exact ms values (e.g.
`ms_per_frame` 62.5/64/15.625, dt 15.625/31.25/64/512) so f32 (native) and
f64 (reference) arithmetic are both exact — no epsilon needed; compare
integers exactly and floats by f32 bits. dt seconds = ms/1000 is inexact, so
the DUMPER takes ms and does `dt = ms / 1000.0f` then the module does
`* 1000.0f` — mirror that exact f32 round-trip in the reference with a
struct-based f32() helper for the clock only (the walk itself uses ms).

Scenario coverage (all from the plan §6 + quirks above): exact-case +
case-insensitive lookup; miss (clock zeroed, selection preserved); guard-off
no-side-effects; same-record re-select mid-clip (frame preserved / phase
reset); different-record select (frame reset); loop clip wrap (never fires);
one-shot exact walk (priming tick, completion at last frame, re-fires every
update); dt >> clip (multi-frame catch-up); single-frame one-shot (fires on
first update); zero-frame record (fires immediately); ms_per_frame==0 (never
advances); pause mid-clip / unpause phase reset / pause at end still fires;
shape-mode 0/1/other key leads; mode-0/1 name-buffer pre-copy on miss;
failed-import prefix-named record + id reuse (first-id-match wins); 63-char
name truncation vs 80-char key; select_index guard `seq_id < 0`; consuming
hook self-latch. Mutation selftest: `--selftest` flag copies
`animated_dispatch.c` to tmp and string-replaces (a) `strcasecmp`→`strcmp`,
(b) delete the `cur_frame = -1;` def-change reset, (c) delete the hook-fire
block, (d) invert the `play_loop` gate — each mutant must turn at least one
scenario red (exit non-zero), mirroring `check_linkage_certificates.py
--selftest` philosophy.

## Drafted Native Linkage section (for docs/decomp/C3DAnimated.md)

Replace the current `## Native Linkage` section body (keep the old
"why blocked" paragraphs at the end as history). Content outline — subsections:
status para (still `linked-blocked`, port in progress, plan pointer); "OMedia
runtime resolution" (the table + 3 numbered consequences from this handoff);
key-composition note (strcpy-vs-strcat correction, flagged explicitly as a
correction of the earlier prose, with the three globals as settable data);
"Method map (original → native)" table:
`InitAnim3DDatabase→animated_dispatch_init_entity` (DB/builder registration +
default-shape seeding collapse), `CreateAnim3DRecord→_create_record` (clip
pointer instead of stream import; failure shape preserved),
`FindAnim3DRecordByName→_find_record`, `SetAnim3DByName→_set_by_name`,
`SelectAnim3DRecordIndex→_select_index`, `GetCurrentAnim3DRecord/
GetCurrentAnim3DObject→_current_record/_current_clip`, `UpdateAnimated
(dispatch slice)→_update` (PickupLink/Update3DObject/CanMove stay with
existing native behaviors), base slot 65→NULL-hook no-op, `C3DPlayer` slot
65→`_set_anim_ended_hook` target (wiring commit), `SetAnim3DPaused→
_set_paused`, `ApplyAnimatedEnabledState/ApplyAnimatedCollisionVisibleState→`
existing `behavior_base.c` gates (not in module scope); "Native state carrier"
field↔offset table (from the struct above); "Deliberate deviations": (1)
single-sequence clips — sequence-vector indirection + positional-mapping +
refresh-vs-clamp interplay collapse; assumption to validate in the
shipped-data pass: exported defs are single-sequence; (2) no OMedia DB;
(3) frame advance merged into the dispatch update, float ms (`omt_WMilliSec`
is float — no truncation gap), `play_reverse/pingpong` never set by recovered
paths, not ported; (4) caller buffers not mutated (63-char truncate is
copy-side); (5) 80-byte key bounded via snprintf instead of stack overrun;
(6) MEMLOG/trace calls dropped; "Scope" para: dispatch/update/completion logic
only, visual fidelity + player movement stay on native-port by-eye track,
`ase-deserialization` row unchanged.

Open-questions updates for the doc: mark the `0x110/0x11c` setters partially
resolved (slot `+0x10c` = shape-selection helper is still unnamed); ADD:
single-sequence-per-A3dm assumption; the `+0x634/+0x635` writer; the slot-48
(`+0xc0`) setter body (assumed = `set_anim_def` application; not in the dump).
Notes section: add the omt-src evidence line (OMediaAnim.h/.cpp,
OMediaWorldUnits.h:40-41, OMediaClassStreamer.h:49-55).

## Operational notes for the next session

- This session drove the gateway from the micro over SSH
  (`wsl -d Ubuntu -u scotty -- ssh scotty@192.168.18.38`); a session running
  ON the gateway in `~/jn-engine` is simpler.
- A partially-edited copy of `docs/decomp/C3DAnimated.md` (the corrections
  listed above, WITHOUT the Native Linkage rewrite) was pushed and committed
  with this handoff — `git log` will show it. Diff that commit before
  continuing the doc work.
- Verify gates after each C change:
  `python3 tools/build_vtable_parity_report.py && make &&
  python3 tools/audit_faithfulness.py` (+ the new oracle); restore the two
  generated docs' date-only churn afterwards.
- Hard boundaries from the fresh prompt still apply: no cert flip this slice,
  no player-movement rewrite, no `ase_loader.c` certification, no gadget/menu
  port, keep catalog refreshes out of behavior commits.
