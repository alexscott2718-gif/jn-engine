# Picture-flag wiring plan

**Goal:** consume the pickup gating data the shipped `.gam` levels already
carry, replacing the ad-hoc tool model's silent pass-through with the
original's picture-flag economy — and open a certifiable `linked` aspect on
`C3DPickupItem`, which `docs/linkage_certificates.csv` currently records as
having "no decompiled-body fidelity to certify".

Status: **phases 0-5 implemented** (0-3 2026-08-19 §8, 4-5 2026-08-20 §9-10); 6 open. Measurements below were taken
2026-08-19 against `assets/gam/*.gam` (35 files) with `tools/gam_parser.py`.

---

## 1. The premise was wrong, and that is good news

`docs/gam_schema.md` says props marked ✗ "are dropped today". For the pickup
gating fields that is **not true**, and the plan is much smaller because of it.

`src/engine/assets/gam_loader.c` ends both its float and int branches with a
`prop_bag_add()` fallback, so every unmapped numeric property already lands in
`Entity.props[40]`, and every unmapped string in `Entity.strs[32]`. They are
readable right now through `gam_prop_i()`, `gam_prop_f()` and `gam_str()`
(`src/engine/world.h:114-117`).

`behavior_item.c` already proves it: it reads `PIC_NUMBER` today for the
baseball special case, and `behavior_ai_trigger.c:182-190` already dispatches
`ToggleObject` / `NextTrigger` by tag lookup.

**So there is no loader work in this plan.** The data is in memory on every
level load. What is missing is a place to keep the flags and the logic to
consume them. Fixing the `gam_schema.md` sentence is Phase 0, because a
contributor reading it today would scope this job several times too large.

## 2. What the corpus actually holds

| Measure | Value |
|---|---|
| Rows awarding a picture (`PIC_NUMBER >= 0`) | **237** — `3PIC` 218, `3FIS` 12, `3GIR` 5, `3DIN` 2 |
| Rows gating on one (`RequiredPicNum >= 0`) | **62** — all `3PIC` |
| Distinct picture ids awarded | 21, range `0..72` (id 16 is awarded 67×) |
| Distinct picture ids required | 13, range `2..27` |
| `ReqPicNumAmount` | `1`×343, `2`×28, `3`×3 — a **count**, not a bit |
| Rows with a real `NeedMoreSound` | 46 |
| Side effects on `3PIC` | `NextTrigger` 43, `ToggleObject` 32, `ActivateObject` 22 |
| Densest numeric row vs `ENTITY_MAX_PROPS` | 38 of 40 (`Level1.gam`, `3MCA`) |

Two findings shape the design:

**The gating graph closes globally.** Every one of the 13 required ids is
awarded by some row somewhere. Implementing the gate cannot strand content.

**It does not close per level.** Four of ten levels require a picture no row in
that level awards:

| Level | Needs | Awarded in |
|---|---|---|
| `level1a` | 10 | level1c, level2a, level2b, level3a, level4, level4a, level5 |
| `level1b` | 12, 14 | 12: level1, level1c, level2, level2a, level4 · 14: **level2, level2b** |
| `level1c` | 23 | level1, level2, level2b |
| `level4a` | 8 | level1, level1a, level1c, level2, level2a, level2b, level3a, level4, level5 |

`level1b` requiring picture 14, which is only awarded in `level2` and
`level2b`, is the tell: these flags are **save-global and outlive a level**,
and the original expects the player to come back. That single fact drives three
design decisions below (persistence, re-collection, and the level-selector
stance).

## 3. Design

### 3.1 Keep the tool model

`TOOL_GRANTS` is load-bearing: the watergun, jetpack and keys gate real
progression in the native port today. The picture economy is **additive**, not
a replacement. Ripping out a working mechanism to install a more faithful one
is how a playable build stops being playable. Both run; only the picture path
is new.

### 3.2 The store

In `gamestate`, alongside the existing inventory:

```c
#define PIC_ID_MAX 96        /* corpus tops out at 72; headroom for jnvsjn */

int  gamestate_pic_count(int id);           /* how many of picture `id` held */
void gamestate_pic_award(int id, int n);    /* award n (n >= 1) */
int  gamestate_pic_consume(int id, int n);  /* consume n, 0 if short */

/* Collected-state table. Keyed by level *and* index -- see below. */
int  gamestate_pickup_taken(const char *level, int pickup_index);
void gamestate_pickup_mark(const char *level, int pickup_index);
```

Counts, not bits — `ReqPicNumAmount` is 2 or 3 on 31 rows.

**Persistence is the whole point.** `gamestate_reset_for_new_level()` must
*not* clear either structure. There is precedent one line away: that function
already preserves `items_collected` as a lifetime tally. Only a new game
clears them.

**`pickup_taken` is not optional.** Native tracks collection in per-entity
`user_flag`, which resets on every level load. Award-on-touch without a global
table means re-entering a level re-awards every picture in it — an infinite
farm that trivially defeats the gate we are adding.

**And it must be keyed per level.** Measured: 456 distinct `PickupIndex`
values, never repeated *within* a level, but **22 collide across levels** —
`1901..1905` and others appear in both `Level3` and `level5a`. A flat
index-keyed table would mark `level5a`'s pickups collected because the player
took `Level3`'s. Key on `(level, index)`; a small open-addressed set is
plenty at this size.

### 3.3 The collection path

Restructure `item_on_trigger` to mirror `HandlePickupCollection` (`00435ce0`)
in *order*, which is where the observable behaviour lives:

1. once-only guard — `user_flag` **and** `gamestate_pickup_taken(PickupIndex)`
2. gate: `RequiredPicNum == -1` → pass; else `pic_consume(req, amount)`; on
   failure play `NeedMoreSound` and **return without collecting**
3. mark `pickup_taken`, hide (or swap to `PickedUpIndex`)
4. award `PIC_NUMBER`, award `Points`, typed counters, tools (existing)
5. dispatch `ActivateObject` / `ToggleObject` / `NextTrigger`
6. sound

Step 2 returning early is the first time this build ever refuses a pickup.

### 3.4 Side effects

Reuse, do not reinvent: `behavior_ai_trigger.c:182-190` already resolves
`ToggleObject` and `NextTrigger` to entities by tag and acts on them. Lift that
into a shared helper both call, rather than a second copy that drifts.

## 4. Phases

Each phase is independently shippable and leaves the build playable.

| # | Work | Done when |
|---|---|---|
| 0 | Correct the `gam_schema.md` ✗ legend; note the bag fallback | Sentence says "captured in the prop bag, unconsumed" |
| 1 | The store + `--selftest` | `tools/check_pictures.py --selftest` passes; store survives a swap, clears on new game |
| 2 | Award path: `3PIC` + `vt_creature` for `3FIS`/`3GIR`/`3DIN`; `pickup_taken` table | Headless run of all 10 story levels shows the 21 expected ids awarded, no double-award on re-entry |
| 3 | Gate path + `NeedMoreSound` + level-selector stance (§5) | The 62 gating rows refuse when short; a scripted run collects the prerequisite then succeeds |
| 4 | `ActivateObject`/`ToggleObject`/`NextTrigger` via a shared helper | The 97 authored side-effect rows fire; `behavior_ai_trigger` still passes its tests |
| 5 | HUD picture counter (`C2DInGameMenu.md` documents three counters + `DAT_004f83c0`) | Counter tracks the store |
| 6 | Oracle + mutation test on the collection path | A real `linked` aspect on `C3DPickupItem` replaces `linked-blocked` |

Phases 0–3 are the "wiring problem" proper. 4–6 are what turn it into
something the certification process will accept.

## 5. The decision I need from you

Implementing the gate faithfully makes four levels partly uncompletable **when
entered cold from the level selector we just shipped** — `level1a`, `level1b`,
`level1c`, `level4a` each require a picture awarded elsewhere. In a linear
save this is fine (and `level1b`/14 implies backtracking even in the original).
From the selector it is a soft-lock on content that is reachable today.

Options:

- **A. Selector pre-grants.** Entering a level from the menu grants every
  picture that level requires. Faithful during normal play, forgiving on a
  cold jump. Costs one lookup table (4 levels, 5 ids). *My recommendation.*
- **B. `--allpics` flag,** off by default. Purest, but a QA build where four
  levels dead-end unless you know the flag is a support burden.
- **C. Gate only where self-contained** (skip those 5 rows). Least faithful;
  quietly diverges again, which is what we are trying to stop doing.

## 6. Risks

- **Prop-bag headroom is 2.** The densest row uses 38 of 40 slots. This plan
  adds no props, but anything that does should raise `ENTITY_MAX_PROPS` first —
  overflow silently drops the *last*-authored props with no diagnostic.
- **`3FIS`/`3GIR`/`3DIN` route to `vt_creature`,** not `vt_item`. Their award
  path is a separate (small) edit; do not assume one call site covers all 237.
- ~~`PickupIndex` uniqueness~~ — measured and resolved: it collides across
  levels (22 shared between `Level3` and `level5a`), so the table is keyed on
  `(level, index)` per §3.2. Left here because a flat table is the obvious
  first implementation and it is silently wrong.
- **Phase 3 changes difficulty.** First build where a pickup can refuse you.
  Worth a Discord heads-up to DICK and the crew before it lands in a QA zip.

## 7. Effort

Phases 0–3 are roughly a focused day: the store and its test are the bulk, the
collection rewrite is ~80 lines in one file, and the measurements above are
already done. Phases 4–6 are a second pass — 6 in particular is an oracle plus
mutation test, which the project treats as its own kind of work.

---

## 8. Phases 0–3 as built (2026-08-19)

Status: phases 0–3 **implemented**; 4–6 remain a separate pass. Everything below
was measured on this checkout, not carried over from §2.

### 8.1 What landed

| Phase | Where |
|---|---|
| 0 | `tools/gam_schema.py` legend + regenerated `docs/gam_schema.md` |
| 1 | `gamestate.{h,c}` store; `tools/pictures_dump.c` + `tools/check_pictures.py --selftest` (in `make check`) |
| 2 | `behavior_pickup_core.c`; `behavior_item.c` rewrite; `behavior_creature.c` award path |
| 3 | the gate + `NeedMoreSound` in `behavior_pickup_core.c`; `tools/gen_picture_pregrants.py` → `src/game/picture_pregrants_generated.h`; cold-entry wiring in `main.c` |

Runtime evidence: `tools/verify_picture_economy.py` (not in `make check` — it
needs assets *and* a built binary, and loads every level twice).

### 8.2 Corrections to §3.3

The decompiled order in `docs/decomp/C3DPickupItem.md` is not the order §3.3
lists. `HandlePickupCollection` (00435ce0) is:

```
if toucher != player            return
if !CheckRequiredPicAndConsume  return      <- gate BEFORE the collected check
if PickupIndex > 0 and state[PickupIndex] != 0  return
update state + visibility (or the PickedUpIndex sprite)
fire ActivateObject / ToggleObject          <- BEFORE the award
award PIC_NUMBER + score
fire NextTrigger                            <- AFTER the award
play the pickup sound
```

Two consequences the plan did not carry:

- **The gate runs before the collected-state check.** That is only safe because
  a collected pickup is hidden and disabled *at load* — `PostLoadPickupItem`
  (00436200) and `ResetPickupItemVisibility` (00435b20) both read the same
  table. §3.2 described the table as a write on collection and never said where
  it is read; without the load-time half it is write-only, re-entry re-awards
  every picture, **and** — because the gate is first — walking back into a
  pickup you already took charges you for it again. `behavior_pickup_restore_taken`
  is that half, called from both `item_on_spawn` and `creature_on_spawn`.
- **The side effects straddle the award.** Phase 4 has two insertion points, not
  one; both are marked `PHASE 4 goes here` in `behavior_item.c`.

### 8.3 The gate consumes, and the corpus is short in more places than §5 says

`CheckRequiredPicAndConsume` (vtable 3 slot 54, 00436830) "consumes
`ReqPicNumAmount` on success, clears the picture flag when the count reaches
zero". Consume, not threshold — so a level's demand for a picture is the *sum*
of its gates' amounts, not the number of distinct ids.

§5 derives its four levels by set difference (an id the level never awards).
Under consume, eight levels cannot meet their own demand:

| Level | id | needs | awards in level |
|---|---:|---:|---:|
| Level1 | 3 | 2 | 1 |
| Level1a | 10 | 4 | 0 |
| Level3 | 27 | 4 | 3 |
| level1b | 2 | 3 | 1 |
| level1b | 12 | 4 | 0 |
| level1b | 14 | 1 | 0 |
| level1c | 6 | 9 | 4 |
| level1c | 8 | 6 | 1 |
| level1c | 12 | 4 | 2 |
| level1c | 23 | 18 | 0 |
| level4 | 3 | 3 | 2 |
| level4a | 8 | 12 | 0 |
| level5 | 10 | 4 | 1 |

The pre-grant implements the decision exactly as recorded — the same four levels
and five ids — with the count set to that level's whole demand, because granting
one copy of picture 23 would open one of level1c's nine gates. **The remaining
count shortfalls are left alone and need an owner decision.** They are not
soft-locks in linear play: the pictures are awarded elsewhere and the flags are
save-global, which is the design. They *are* differences in what a cold jump can
finish. `level1c` id 23 is the extreme case: 18 demanded, 13 awarded in the
entire corpus, so even a perfect linear playthrough can open at most 6 of its 9
gates — that reads as intended scarcity ("spend your money"), not as a data bug,
but it is worth confirming before phase 5 puts a counter on the HUD.

### 8.4 The creatures award, but nothing may collect them yet

3FIS/3GIR/3DIN authored 19 of the 237 awarding rows, and `vt_creature`'s award
path is built and tested. Its vtable **keeps `flags = 0`**, so the engine's
overlap dispatch never calls it. Adding `ENTITY_FLAG_TRIGGER` would make walking
into a dino collect it, and `behavior_creature.c` already records the owner's
2026-06-23 ground truth: these become pickups only *after* the shrink ray
shrinks them, and the shrink transition is undecompiled. The award path is
therefore reachable only from the eventual shrink caller and from the
`JN_TEST_PICTURES` sweep. Wiring the trigger flag would have invented the
mechanic that note refuses.

### 8.5 Other measured facts

- **`PickupIndex` is wider than the picture economy.** 478 rows carry one:
  3PIC 383, 3RED 70, 3FIS 12, 3ANI 6, 3GIR 5, 3DIN 2. Only 3PIC/3FIS/3GIR/3DIN
  author `PIC_NUMBER`, so award coverage is complete, but 3RED and 3ANI descend
  from `CPickupType` too and their collected state is *not* tracked yet — they
  re-collect on every level entry. Out of scope here; worth a row in phase 4+.
- **No row authors `PickupIndex == 0`,** so the original's special non-table
  pickup branch has no shipped instance. The store still honours it.
- **`NeedMoreSound`:** 46 rows author a real (>= 0) value; 45 of those are
  gating rows. The 46th is on a row with no `RequiredPicNum`, so it can never
  play. (§2's "46" counts all rows; the gate only ever reaches 45.)
- **`Points` is a phantom.** `gam_loader.c:286` maps a property named `Points`
  onto `Entity.points`. That name appears **nowhere** in the 35-file corpus; the
  authored field is `PointValue` (383 rows), which the decomp confirms is the
  score award (offset 0x620, `FUN_0042adc0`). So `e->points` has always been 0
  and no pickup in the native port has ever awarded score. Left unfixed on
  purpose — it is a scoring change in the loader, not part of the picture
  economy, and it deserves its own reviewable change.
- **`tools/gam_schema.py` was not idempotent** against its own committed output:
  regenerating reverted `TRIG` to unnamed, dropping the identity the CTrigger
  certificate recovered. Phase 0 had to regenerate, so the generator learned the
  override; a curated `CLASS_OVERRIDES` row also no longer gets the
  low-confidence `?` marker.

### 8.6 Verification

```
python3 tools/check_pictures.py --selftest        # in make check
python3 tools/check_pictures.py --corpus          # in make check-assets
python3 tools/verify_picture_economy.py           # by hand, needs assets + binary
```

`--selftest` rejects three mutants: a flat index-keyed collected table (the
§6 trap), a level swap that clears the picture store, and an off-by-one in the
required-amount test. `--corpus` re-derives the pre-grant header from the corpus
and fails if it is stale, and re-checks that every required id is awarded
somewhere.

`verify_picture_economy.py` over all 35 levels: **21 of 21** authored picture ids
awarded at runtime; re-entry collects **0** in every level; 105 gate refusals on
cold un-pre-granted entries; 7 levels where a later sweep pass collected a row an
earlier pass had refused (the "collect the prerequisite, then succeed" case); and
all four pre-grant levels go from refusing to clear.

Picture id 0 is the one id no single cold level run can reach: its row (level1c,
`PickupIndex` 411) is the corpus's only row that both awards and gates, and
level1c cannot supply the 3 copies of picture 6 it costs on top of its other
picture-6 gate. Carrying picture 6 in from level2b opens it — a live
demonstration that the flags really are save-global, which the verifier now
performs and asserts.

---

## 9. Phase 4 as built (2026-08-20)

Status: phase 4 **implemented**. It turned out to be bigger and more
interesting than the plan's one-line entry, because the side-effect fields are
not a trigger chain — they are a **vending-machine mechanism**, and wiring them
the obvious way would have been wrong.

### 9.1 The dispatch is a state write, not a trigger forward

`docs/decomp/C3DPickupItem.md` pins `Toggle` (inherited `C3DTriggerType`,
0x584) as "state argument passed to `ToggleObject` and `ActivateObject` targets
through vtable offset `0x428`", and `HandlePickupCollection` calls
`fire_tag(ActivateObject, Toggle)` / `fire_tag(ToggleObject, Toggle)`. That slot
is a **state setter**, not the collision entry point. For a pickup target the
two are opposites: forwarding `on_trigger` *collects* the target, while
`SetPickupItemState` (004360b0) state 1 *clears its collected flag and shows
it*. 36 of the 97 side-effect rows target a `3PIC`, so getting this wrong would
have silently auto-collected pickups across the level.

Native now models the slot: `EntityVTable` gained `on_set_state`, `vt_item`
implements it, and `behavior_trigger_set_state_tag` dispatches through it.
`NextTrigger` keeps the trigger-chain forward, matching the decomp's separate
`fire_next_trigger`.

### 9.2 The vending machines

Twelve authored pairs across nine levels are a two-object exchange:

| Level | machine | product | price | product awards |
|---|---|---|---|---|
| Level1a | `cmach` 204 | `cand` 205 | 2 x pic 10 | pic 8 |
| Level1a | `fmach` 207 | `flurp` 206 | 2 x pic 10 | pic 7 |
| level1b | `mdiam` 303 | `diam` 305 | 1 x pic 19 | pic 16 |
| level1b | `gdish` 317 | `refill` 318 | 1 x pic 4 | — |
| level1c | `piggy1` 424 | `piggy2` 451 | 1 x pic 10 | — |
| level2a | `fp` 1026 | `anewflurp` 1025 | 2 x pic 10 | pic 7 |
| level2a | `cm` 1027 | `cmc` 1028 | 2 x pic 10 | pic 8 |
| level4 | `mach` 2407 | `cand` 2406 | 2 x pic 10 | pic 8 |
| level4 | `fmach` 2409 | `flurp` 2408 | 2 x pic 10 | pic 7 |
| level4a | `cjar` 1501 | `coins2` 1508 | 2 x pic 8 | pic 10 |
| level5 | `cmach` 2739 | `cbar` 2740 | 2 x pic 10 | pic 8 |
| level5 | `fmach` 2747 | `flurp` 2748 | 2 x pic 10 | pic 7 |

The machine authors `InitallyActive=1` and the `RequiredPicNum` gate; the
product authors `InitallyActive=0` and awards a picture. Paying the machine
fires its `ActivateObject`/`ToggleObject` at the product with `Toggle=1` — state
1, which reveals it. Collecting the product fires its `ToggleObject` back at the
machine, re-arming it. **Every one of the twelve is a real cycle in the authored
graph**, and every full pass is picture-negative (-2 coins, +1 product), so the
consume gate is the only thing that terminates it.

This is the strongest evidence yet that §8.3's reading is right: if
`RequiredPicNum` were a threshold rather than a consume, these twelve loops
would be infinite point-and-picture farms.

`level4a`'s `cjar`/`coins2` runs the exchange the other way — pay 2 candy
(pic 8) for 1 coin (pic 10) — and is still negative.

### 9.3 `InitallyActive` had to land with it — and the golden trimmed it

The mechanism does not exist without the other half of `PostLoadPickupItem`:
28 rows author `InitallyActive=0` and must not be collectible until their
machine pays out. Native ignored the field entirely, so every product was free.
`behavior_pickup_spawn_gate` now applies both load-time gates — already
collected, and initially inactive. (Preserve the misspelling; it matches the
executable string and the schema.)

**The first attempt treated inactive as hidden, and the `level1` golden caught
it.** Two `level1` rows author `InitallyActive=0` (`egg2b` 834, `hsounds` 837),
one of them in frame at the capture pose; hiding them changed ~0.05% of both
golden frames (499 and 564 pixels).

That is the gate doing its job, because the two halves of "inactive" are not
equally supported. *Not collectible* is solid: the field is the "initial active
state", `ActivateObject` names the transition out of it, and the vending data
only works that way. *Invisible* is not: the recovered slot-266 body describes
states 0 and 1, both of which **show** the pickup, and never says what the
inactive state looks like — `set_state_inactive()` is a name in the doc's
pseudocode, not a recovered body.

So the visual half was withdrawn. `InitallyActive=0` now clears the trigger flag
and latches the once-only guard, and leaves `visible`/`alive` alone. The vending
machines behave identically (the product still cannot be taken until its
machine's `Toggle=1` write arrives) and the golden is byte-identical again. The
open question — does the original hide an inactive pickup? — belongs in a
capture comparison, not in a guess.

### 9.4 Where the shared helper lives, and why it is not in a new module

`tools/linkage_oracles/C3DAITrigger.py` compiles a **fixed list of `.c` files**
with a fixed set of stubs in `c3daitrigger_dump.c`. Putting the shared dispatch
in a new module would make `behavior_ai_trigger.c` reference a symbol that link
cannot resolve, and the only repair would be editing the oracle. So the helper
lives *in* `behavior_ai_trigger.c` — the file that owns the certified
`dispatch-graph` aspect — and `behaviors.h` exports it. The feature moved; the
contract did not. The oracle passes unchanged.

`behavior_trigger_fire_tag` also carries a native depth cap
(`TRIGGER_DISPATCH_MAX_DEPTH` 16) with no counterpart in the decomp, because the
authored graph has real cycles. It logs when it trips; nothing in the corpus
trips it.

### 9.5 What "the 97 rows fire" actually means

97 authored rows. On a cold corpus-wide sweep, 62 are reached (a row only
dispatches if its own pickup is collected, and refused rows never get there):

| kind | fired | no native slot | unresolved |
|---|---:|---:|---:|
| `ActivateObject` | 8 | 1 | 0 |
| `ToggleObject` | 7 | 9 | 1 |
| `NextTrigger` | 0 | 36 | 0 |

`no-native-slot` is honest coverage, not a silent drop: the target class has no
recovered state or trigger body yet. By target FourCC across all 97 rows,
`ActivateObject` reaches `3PIC` 21 / `3RCK` 1; `ToggleObject` reaches `3PIC` 15
/ `3RCK` 8 / `3OMT` 3 / `3HYD` 2 / `3SWN` 2 / `3KIT` 1 / unresolved 1; and
`NextTrigger` reaches `3CAM` 19 / `3MCA` 20 / `3AIT` 4 — **none** of which has an
`on_trigger` today. Wiring pickups to start cutscenes was considered and
deliberately deferred (owner decision, 2026-08-20): it couples the pickup path
to the cutscene system, and both cutscene-camera aspects are separately
certified.

### 9.6 Verification

`tools/verify_picture_economy.py` now additionally checks that every dispatch
the engine emits is an authored row (no spurious targets), reports the outcome
histogram above, and proves the vending mechanism end to end: **8 of the 12
pairs pay, reveal and re-arm** on a cold entry. The other 4 are unaffordable on
a single-level visit — `level1b` 303, `level4` 2407, `level5` 2739 and 2747 —
which is the §8.3 count shortfall showing up in gameplay rather than a wiring
failure, so the tool asserts the conditional claim (*when the machine's gate
passes, the product is revealed*) and reports affordability separately.

`tools/check_pictures.py --selftest` now rejects **4** mutants; the new one
makes `gamestate_pickup_clear` a no-op, which breaks the re-arm. The clear is a
flag flip, never a slot eviction — evicting from an open-addressed table would
strand every key that probed past the hole, and the unit test pins that too.

---

## 10. Phase 5 as built (2026-08-20)

Status: phase 5 **implemented**, but not the way the plan's one-liner implies.

### 10.1 The plan pointed at a counter that is not identified

Phase 5 reads "HUD picture counter (`C2DInGameMenu.md` documents three counters
+ `DAT_004f83c0`)". That doc does decode `DrawHud` (00406690) to four literal
positions and formats — `this[0x140]` `%3.0d` at `(0x7d, 0x8c)`, `this[0x13f]`
`%3.0d` at `(400, 0x8c)`, `this[0x141]` `%5.0d` at `(0x185, 0x10e)`, and
`DAT_004f83c0` `%6.0d` at `(0x1a9, 0x1b3)` — but it also lists this as an **open
question**:

> Map each `this[0x13f..0x141]` + `DAT_004f83c0` counter to its gameplay meaning
> (score, fuel, gadget count, lives) — the capture shows positions; the source
> values need the producer functions.

So nothing identifies any of the four as the picture counter. The
`C2DInGameMenu/hud-draw` certificate row is `linked-blocked` for exactly this
reason ("doing so from one frame would fabricate parity"), and the extracted
`hud_layout_generated.h` carries only the two counters the capture actually
shows. Feeding the picture store into one of those slots would have invented the
mapping the certificate refuses to invent.

### 10.2 What landed instead

A native readout, in the shipped menu font, in the one screen corner the
extracted layout leaves empty (top right; atom / status icons / gauge are
top-left, the gadget cluster is bottom-left, the score counter bottom-right).
Same drop-shadow idiom as the existing LEVEL CLEAR banner, which is the
precedent for native chrome that makes no parity claim. It reads

```
PICTURES 18   23 x18
```

— total held, then up to six held ids with their counts, then `more`. (The
shipped atlas is A-Z a-z 0-9 only, so an ellipsis would render as three blanks.)

It draws **only while something is held**. That is a design choice — the readout
is about an economy that is idle most of the time — and it is also why the
`level1` golden is untouched: at the capture pose the store is empty, so nothing
is emitted. No counter position, format or producer from `DrawHud` is used, and
`hud_layout_generated.h` is not modified.

### 10.3 Verification

`make check-assets` green: `level1` golden byte-identical, so the new chrome
provably stays out of the capture path. Visual check by screenshot on `level1c`
after a cold pre-granted entry (18 x picture 23) confirms the readout tracks the
store and clears every captured widget.

When the `DrawHud` producers are eventually recovered and one of the four
counters is shown to be the picture count, this readout is the thing to replace
— it is deliberately easy to delete.
