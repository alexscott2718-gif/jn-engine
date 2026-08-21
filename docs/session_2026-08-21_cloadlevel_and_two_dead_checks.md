# Session note — 2026-08-21: CLoadLevel, and two checks that could not fail

Branch `feat/loadlevel-gate-return`, twelve commits on top of `chore/cleanup-2`
(PR #28).
`make check` and `make check-assets` green in the repo Docker image at every
commit; `level1` and `fixture0` goldens byte-identical throughout (no golden
was regenerated, and none moved).

The session started on the `activate-load` linkage row, whose note says in as
many words: *"Returns to native-port: port the recovered 00457ec0/00458370
semantics 1:1, then write the oracle."* It ended somewhere else, twice, because
two things in the tree turned out not to be true.

---

## What landed

### 1. `CLoadLevel`'s contact gate, ported (5bb7922, dffcf57)

`behavior_load.c` was a functional bridge. It is now a transcription of the
recovered `00457ec0` order.

**Evidence:** `docs/decomp/evidence/cloadlevel_gate_00457ec0.md` (the recovered
body), `docs/decomp/CLoadLevel.md` (field map, `ActivateLoad`), and the 97
shipped `LOAD` rows across the 35 `.gam` files.

Three branches had no native counterpart, and each has shipped rows behind it:

| Branch | Rows | Native before |
|---|---:|---|
| `LevelName == "RETURN"` | 10, all in VR01..VR08 | asked the loader for a level named "RETURN"; every VR level was a dead end |
| `LevelName == "none"` | 1 (level4.gam, which authors `StartPoint="level1.gam"` — the fields are filled in the wrong order) | asked for a level named "none" |
| gate arithmetic | 36 gated rows | `ExactLevel` took precedence over `RequiredLevel`; `RequiredLevel == -1` blocked; the window applied even with `RequiredTask "none"` |

The gate separation is not academic. `Level3.gam`'s `carlcapt` authors
`RequiredLevel 380` **and** `ExactLevel 0`, which under the recovered body can
never both hold — that portal is dead in the original and was live here.
`level1c.gam`'s `yokian2` authors `ExactLevel 0` with `RequiredTask "none"`,
which the original never reads at all.

Also removed, both native inventions with nothing in the decomp behind them:
the spawn-time visibility gate (the recovered body evaluates prerequisites on
contact and never touches the portal until it fires — and since `LOAD` has no
visual, what that flag actually decided was *contact*, at the wrong time and by
the wrong rule), and the substitution of radius 60 for an authored
`Radius <= 0`, which changes exactly one row (`Level2.gam` `rockspaceb`,
authored 0.0, whose neighbours in the corpus author 1.0 — they read as
designer-disabled portals, not as unset).

**Inferred, and labelled as such in the code:** the RETURN departure pair. The
recovered handoff (player slot `0x168`) takes four strings; the normal path
passes `LevelName`/`StartPoint` then the player's `+0x88c`/`+0x8f0`, and the
RETURN path passes the player's two twice. That the player's two are "the level
you left and the start point you entered it at" follows from the call shape;
the writer of `+0x88c`/`+0x8f0` is not recovered. Recording it on *every* swap
rather than only on a LOAD fire is native-defined, and is what makes the ten
VR rows work: nothing in the corpus loads a VR level through a portal, so the
only route in is the main menu.

**Verified by observation.** `JN_TEST_LOAD=1` prints every `LOAD` row in a
level, the gate verdict at the current story state, then fires each portal and
reports what the loader was actually asked for:

```
--level level1 JN_TEST_SET_SCENE=0     all 11 gated portals SHUT
--level level1 JN_TEST_SET_SCENE=108   req<=108 open; req 180/320 still SHUT
--level level3 JN_TEST_SET_SCENE=380   carlcapt SHUT, fires nothing
--level level4                         the "none" row fires nothing
--level vr01 JN_TEST_LOAD_RETURN=level1b:lab
                                       both RETURN portals request level1b/lab,
                                       and the second still does — the pair
                                       survives the first request, as the body
                                       requires
```

`dffcf57` puts the `IsA("C3DJIMMY")` test back, which 5bb7922 left out on the
grounds that `physics.c` only ever hands `on_trigger` the player. True of the
physics dispatch, not of the engine: `behavior_button`, `behavior_ai_trigger`
and `behavior_laser_trigger` all forward whatever toucher they were handed.

### 2. An oracle for it (e0840a7)

`tools/linkage_oracles/CLoadLevel_gate.py` compiles the real, unmodified
`behavior_load_gate_allows` and drives it over all 97 shipped rows at 54 story
states — every `RequiredLevel` and `ExactLevel` value authored anywhere in the
corpus with its two neighbours, plus 0, a value past the top of the story, and
the store-absent case — diffing all 5238 verdicts against the recovered gate
evaluated on each row's own properties. The story state is the real `CTaskList`
store seeded through the real `game_flow_test_seed_state`; the reference side
models the table from `docs/decomp/CTaskList.md`, so the two sides do not share
a source. Row identity is keyed on the file index over *every* object in the
`.gam`, so a record-walk desync shows up as a mislabelled row rather than a
silently shifted comparison.

Mutation-tested, and each mutant dies on exactly the shipped row that motivated
the port: `ExactLevel` precedence on `Level3.gam` `carlcapt`, a blocking
missing task on `Level1.gam` row 2, an ungated row put through the window on
`level1c.gam` `yokian2`.

**Wired to a certificate row** — `CLoadLevel`/`contact-gate`, `linked`, after
the owner approved it. See "The certificate row" below for what it does and
does not claim.

### 3. Three mutation tests that never ran (d1f2b20)

This is the one worth reading twice.

Four linkage oracles carry a `--selftest` that perturbs the ported source and
asserts the oracle goes red. **Three of them never executed the perturbed
code.** The mutant is written into a temp directory and compiled in place of
the real source, and a quoted `#include` resolves against the directory of the
file containing it — so from a temp directory `#include "behaviors.h"` and
`#include "camera_record.h"` do not resolve, and every mutant failed to build.
Each harness then counted the build failure as a rejection.

`C3DAnimated_runtime` said so out loud in its own output, every run:

```
SELFTEST ok: base update dispatch removed rejected (dumper failed to compile)
```

Two of the three back `linked` rows whose certificate notes cite the mutation
test as evidence (`C3DPickupItem/collection`, `CGameType/initgame-camera-record-seed`).
The sensitivity those notes describe is real — every mutant is genuinely
rejected now that they run — but it was not being verified.

`C3DAnimated_runtime.py` had a second problem: it has not built since 265c3b6
("3TAR: bind the moving target, not the shadow", earlier on this same branch)
made `behavior_projectile.c` call `behavior_moving_target_take_hit`, which is
not in the oracle's fixed file list. Nothing noticed, because the `C3DAnimated`
rows are `linked-blocked` so the certificate gate never runs it, and its
selftest was reporting the same build failure as a pass. This is the hazard the
brief names — *"the oracles compile a fixed file list… that is the contract
telling you something"* — and it is worth noting that an oracle no gate runs
will rot silently. Mine is currently in that position.

Fixed with `-iquote` for the real source's directory, a stub for the
cross-aspect symbol, and — the part that matters — **a mutant that fails to
build is now a selftest failure, not a rejection.**

### 4. The sound/fade tail, settled from arithmetic (99b3055)

The recovered gate body reads its sound and fade arguments from three fixed
`.rdata` addresses, which read literally is nonsense (the first four bytes of a
string constant are never `-1`). The evidence doc asserted they were
`this+0x614`/`+0x61c`/`+0x620` without showing why. They are, and the offsets
prove it: all three share the base `0x4ECB20`, which is the `"C3DJIMMY"` string
the same body loads for its `IsA` test — Ghidra folded `[reg + 0x614]` into an
absolute after losing what `reg` held.

Which three properties they are comes from a second source. **The field map's
Offset column is in dwords, the registrar's own units.** `LevelName 0x148 →
+0x520`, `StartPoint 0x15c → +0x570`, `RequiredTask 0x170 → +0x5c0`,
`RequiredLevel 0x184 → +0x610`, `ExactLevel 0x186 → +0x618` — five hits, no
misses, all read at exactly that byte offset by this body. The three remaining
ids land on `0x614`, `0x61c`, `0x620`: the three folded addresses, same order,
same stride. `C3DArrow` corroborates independently — its `InitObject` registers
`this + 0x149` / `0x15d` / `0x15e`, and the same 80-byte string-field stride
appears there too.

This generalises past `CLoadLevel`: the Offset columns in the class specs are
dword indices, not byte offsets, and the class doc now says so where the next
reader will hit it.

### 5. The catalog's headline gap list was seven phantoms (e3b86ab)

`unresolved.md`'s first section — the one this brief points at as work source
#1 — listed seven FourCCs that "would draw a box", including `3TRE` at 169
instances across 12 levels and `3ARR` at 47 across 19.

None of them draw a box. All 311 instances author a `SpriteDatabase` and a
`SpriteIndex`, none is authored invisible, and `resolve_sprite_db` draws every
one. Booting all 35 shipped levels headless and reading the engine's own boot
rollup gives `[entity_visual] all entities resolved` 35 times, zero placeholder
boxes.

One regex. `parse_sprite_chunk_map` expected `{ "db", 33, "path" }, /* name */`;
ac61e0d (2026-08-20, the day before this session) moved the canvas name into
the struct as a fourth field. The pattern stopped matching, the map parsed as
**zero rows**, every authored sprite reference resolved to no path, and every
sprite-visual FourCC fell through to `no_visual`. 859fe1d regenerated and
committed the result. The generator prints `sprite_chunk_map: 0` in its own log
and carries on.

Fixed, and the guard is the point: entries are read a line at a time, and an
entry line that does not parse — or a map that comes out empty — is now an
error. After regeneration: map 0 → 200 rows, status `sprite` 7 → 15,
`no_visual` 91 → 84, "would draw a box" 7 → 0, matching the runtime sweep.

### 6. The race circuit the corpus authors and the checkpoint spec does not list (1132b8b)

`C3DCheckPoint.md` asks in its own open questions: *"Map non-finish checkpoint
progress (does crossing one arm the next / update a lap?)"*. All **22** shipped
`3CHK` rows author a `Next` string the Field Map does not list, and in the two
racing levels they form a closed ordered circuit:

```
Level2b  startline -> CHECK1 -> CHECK2 -> CHECK2_5 -> CHECK3 -> CHECK4 ->
         CHECK5 -> CHECK6 -> CHECK7 -> FINISHLINE -> STARTLINE (= startline)
         plus check1a -> CHECK1, which nothing points at
level2a  STARTLINE -> CHECK2 .. CHECK9 -> FINISHLINE -> STARTLINE
VR04     one checkpoint, Next = "none"
```

The links resolve case-insensitively, matching the class's own `__strcmpi`
compare against `FINISHLINE`. `FINISHLINE` **closes** the loop in both levels
rather than ending it — which fits a lap counter, and fits `UpdateCheckPoint`
returning after the finish branch instead of advancing progress. `CheckAvail`
is 0 on all 22 rows, so it is per-run state, which is what `Reset` re-arms.

**Not established, and the note says so:** whether `InitObject` (`00414aa0`)
registers `Next` at all. An authored property no registrar declares is
serialized and never read, so a dead chain survives the data equally well.
`Next` is a *per-class* registration in this engine — `C3DYokDoor::InitObject`
registers it at dword `0x181`, `C3DLaserTrigger::InitObject` beside
`ItemActive` and `Toggle` — which is what makes it worth asking. Falsifier:
re-read the registrar calls at `00414aa0`.

Evidence only; `behavior_checkpoint.c` is unchanged. Porting the circuit would
be design, since the consumer of `Next` is not in the recovered body.

### 7. `C3DShadow` still claims the 22 rows that turned out to be the moving target (47bafac)

265c3b6 rebound `3TAR` to `C3DMovingTarget` on the shipped data. The engine was
fixed; `C3DShadow.md` was not. It still describes itself as a placeable placed
22 times with 25 harvested properties — all of which are the moving target's
(16 rows in `Level3C`, 6 in `VR07`'s shooting range, every one tagged
`C3DMOVINGTARGET`, authoring `StartPos*`/`DestPos*`/`Speed`/`HitsRequired`/
`RespawnTime`/`NumPoints`).

Nothing flags it, and the reason is worth writing down: `spec_check` fires
`FOURCC_OWNED_BY_ANOTHER_CLASS` when a spec disagrees with the registrar scan,
and the scan agrees with `C3DShadow` — `3TAR` is registered twice and the scan
attributes it to the site where a name was captured. **So the class that is
right about `3TAR` is the one that gets flagged**, the annotated baseline entry
sits under `C3DMovingTarget`, and the wrong half of the pair was never
corrected.

The `3TAR` cell stays — this class really is one of the two registrars — but
the page now claims none of the instances, and records that whether a
`C3DShadow` is ever placed at all is open.

### The sweep that produced 6 and 7, and why it did not become a check

`spec_check` has `NOPROPS_BUT_HARVESTED` — a spec claiming *no* `.gam`
properties when the corpus authors some. It has nothing for the partial case: a
field map that lists five properties while the corpus authors eight. I measured
it (spread-filtered so inherited base properties do not count as omissions):

**8 specs flagged, 2 real.** `3CHK`'s `Next` (§6) and `3TAR`'s ten
moving-target properties (§7, a mis-attribution rather than a field-map gap).
The other six are false positives of literal name matching: `3AIT`, `3MCA` and
`3MUS` document theirs in range notation (`ActivateObject0..4`,
`MusicIndex0…MusicIndex4`), `3CAM`'s and `STRT`'s are inherited
`C3DTriggerType`/base properties, and `C3DMerryGo.md` explicitly defers to
`gam_schema.md` instead of listing any.

A permanent rule would need range-notation expansion and an inheritance model
to get that ratio down, and would then be worth roughly two findings. I fixed
the two and did not land the check. If someone wants it later, the shape is
right and the false-positive classes above are the work.

---

## Contradictions found and not resolved

### The task getter cannot return the value `CLoadLevel` checks for (742f7f6)

The recovered gate calls `FUN_0045fea0(RequiredTask)` and branches on `== -1`,
logging `ERROR: Task %s not found in in %s` and falling through. The evidence
doc reads that as "task missing". **Two specs that decompiled the getter itself
say it returns `0`, not `-1`, on no-match** — `docs/decomp/_scene_sequencer.md`
("return `*(entry+100)` for the name, else `0`") and `docs/decomp/CTaskList.md`
("`0` (not `-1`) if no entry matches"), which already flags native
`task_entity_state`'s `-1` as a real divergence.

If the getter is right, that branch is dead code and an unmatched
`RequiredTask` reads as state `0` — which **blocks** every row authoring
`RequiredLevel > 0` instead of passing it through. That is the opposite outcome
for the 36 gated rows on a cold entry.

Nothing else in the tree discriminates. Of the other recovered callers —
`C3DFowl` (`0x121 < s < 0x136`), `C3DYokCargo` (`0x1e9 < s`), `C3DSchoolDoor`
(`s < 0x1a4`), `C3DBus` (`DINO < 10 && SCENE >= 260`) — not one compares
against `-1`; they range-test the value, and every tag they name is in the
`NewGame` table, so `0` and `-1` behave identically in all of them.
`CLoadLevel` is the only recovered caller that would notice, and it is the one
in question.

**Unresolved, and it needs the executable.** The workstation has no
`Neutron.exe` and no Ghidra project — the same blocker the `CGameType`
pause/help row records. It is a five-line function; reading its no-match return
settles this branch and `CTaskList.md`'s open getter divergence at once.

Native falls through (gate open), and the code says that this is a product
decision rather than a port: `task_entity_state` returns `-1`, a direct
`--level` run has no store at all, and blocking here would shut 11 of level1's
13 portals on a cold entry. The oracle certifies the gate's *response to a
state* and says explicitly that the state-for-an-unmatched-tag mapping is the
getter's question, deliberately outside it.

### An oracle nobody runs

`C3DAnimated_runtime.py` rotted because its certificate row is `linked-blocked`
and nothing else runs it. `CLoadLevel_gate.py` is in that position today. This
is a structural gap, not a one-off: the certificate gate runs only the oracles
`linked` rows name.

---

## Found and deliberately not fixed

* **`C3DArrow` still uses the shared window helper** (`behavior_base.c`
  `level_window_allows`), whose rule I have just shown is not `CLoadLevel`'s.
  `C3DArrow`'s owned decompiled methods only *register* `RequiredTask` /
  `RequiredLevel` / `ExactLevel`; **its consumer is not recovered**. Aligning it
  with `CLoadLevel`'s rule would be inference by analogy across classes, and
  each recovered gate in this engine is inlined in its own body rather than
  shared, so the analogy is weak. It also touches 47 visible billboards across
  19 levels. Left alone; the divergence is now documented at both ends.
* **The `SoundIndex`/`FadeType`/`FadeTime` tail is identified but not ported.**
  The engine has no fade, 95 of 97 rows author `SoundIndex -1`, and the callees
  (`FUN_0047d390`/`FUN_0047dc80`/`FUN_00403c10`) are unrecovered.
* **`ActivateLoad`'s own `+0x17a` request block** through global slot `0x100`,
  the `DAT_004f0588` game-mode switch, and player slots
  `0x178`/`0x11c`/`0x2c4`. Native defers the swap through `gamestate` instead.
* **`chore/cleanup-2` has six finished commits pushed with no PR open.** Not
  mine to propose; flagged in case it was an oversight.

---

## The certificate row (owner-approved, landed)

`CLoadLevel_gate.py` was left unreferenced pending the owner's call, since
wiring an oracle in is a certificate status change. **Approved and landed:**

```
CLoadLevel,contact-gate,progression / objectives,linked,tools/linkage_oracles/CLoadLevel_gate.py,docs/decomp/CLoadLevel.md,"…"
```

A **new, narrower aspect** rather than a move of `activate-load` — which stays
`linked-blocked`, because the recovered body splits cleanly into a verdict
(certifiable against every shipped row) and a set of side effects that are not:
the sound/fade tail, the `DAT_004f0588` mode switch, player slots
`0x178`/`0x11c`/`0x2c4`, and `ActivateLoad`'s own `+0x17a` request block. One
sentence was appended to `activate-load`'s note so it stops asking for a port
that has since happened; its status is untouched, and no other row changed
(field-level diff against the pre-edit manifest: one field).

Scoreboard: **17 linked**, 15 linked-blocked. `make check-assets` green with
the gate running the new oracle, and `check_linkage_certificates.py --selftest`
still proves the gate rejects a failing oracle.

That also closes the "an oracle no gate runs will rot" exposure for this one —
`C3DAnimated_runtime.py` remains in that position, since its rows are
`linked-blocked` by design.

---

## What I would pick up next

1. **The `RETURN` departure pair deserves a runtime test of the real path.**
   Today `JN_TEST_LOAD_RETURN` seeds it directly. A test that enters a VR level
   through the menu route and then fires the portal would exercise the
   promotion in `gamestate_request_level_swap` end to end, which is the part
   that is inferred rather than recovered.
2. **`C3DCheckPoint`, now that §6 has mapped the circuits.** `UpdateCheckPoint`
   (`00414410`) is recovered, `vt_checkpoint` is a deliberate simplification by
   its own comment, and the certificate says porting the `FINISHLINE` /
   race-timer (`DAT_004eefc8`) mechanism is real behavior work. The two things
   that would make it portable rather than invented: the registrar calls at
   `00414aa0` (does it read `Next`?) and `FUN_004073b0`/`DAT_004eefc8`. Both
   need the executable. Until then a race port would be design wearing a port's
   clothes, which is why §6 stopped at evidence.
3. **`CTrigger`'s watched-list latch** is fully recovered (target 5) and native
   `vt_trig` is a one-shot log stub — but the corpus has one `TRIG` row and no
   static registrar, so a faithful port would be inert. Worth doing for the
   record, not for the game; decide which of those the project wants.
4. **Check the other generated-header parsers.** `parse_sprite_chunk_map` was
   the only naive-regex one in `build_asset_catalog.py` — the rest use a brace
   tokenizer — but `extract_hud_layout.py`, `gen_picture_pregrants.py` and
   `build_campaign_actor_catalog.py` all straddle the same seam.
   `check_pictures.py` already asserts its header is current; nothing does for
   the others.
