# Picture-flag wiring plan

**Goal:** consume the pickup gating data the shipped `.gam` levels already
carry, replacing the ad-hoc tool model's silent pass-through with the
original's picture-flag economy — and open a certifiable `linked` aspect on
`C3DPickupItem`, which `docs/linkage_certificates.csv` currently records as
having "no decompiled-body fidelity to certify".

Status: **phases 0-6 complete** (0-3 2026-08-19 §8, 4-6 2026-08-20 §9-11), merged as PR #23. C3DPickupItem/collection is `linked`. Original-game observations 2026-08-20 in §15 closed two open questions and opened one new unported mechanism (`ShowArrow`, §15.3). Measurements below were taken
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

The mechanism is worth stating exactly, because it is not "the engine used to
show these and now hides them". `behavior_trigger_spawn_base` **already**
cleared `visible` for an `InitallyActive=0` row — but
`behavior_animated_update_base` sets `visible = 1` again on the very next tick,
so the sprite reappeared one frame later and the golden encodes it as **shown**.
What actually moved the pixels was the first attempt also setting `alive = 0`,
which stops `item_on_update` and so makes the hide stick. The shipped look has
always been "visible"; nothing was ever really hidden.

That matters because the two halves of "inactive" are not equally supported.
*Not collectible* is solid: the field is the "initial active state",
`ActivateObject` names the transition out of it, and the vending data only works
that way. *Invisible* is not: the recovered slot-266 body describes states 0 and
1, both of which **show** the pickup, and never says what the inactive state
looks like — `set_state_inactive()` is a name in the doc's pseudocode, not a
recovered body.

So the visual half was withdrawn. `InitallyActive=0` clears the trigger flag and
latches the once-only guard, and leaves `visible`/`alive` alone. The vending
machines behave identically (the product still cannot be taken until its
machine's `Toggle=1` write arrives) and the golden is byte-identical. The open
question — does the original hide an inactive pickup? — belongs in a capture
comparison, not in a guess. Owner playtest 2026-08-20: the candy is not
noticeable in the tray in the native build either way, so nothing about the
current behaviour looks wrong on screen.

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

---

## 11. Phase 6 as built (2026-08-20) — the aspect is linked

Status: phase 6 **implemented**. `C3DPickupItem` / `collection` moved from
`linked-blocked` to **`linked`**; the scoreboard goes 15 → **16** oracle-verified
aspects. Phases 0–6 are complete.

### 11.1 The row invited this

The blocked row's own closing line was "A future pass that ports the actual
`RequiredPicNum`/`PickupIndex`/`ActivateObject` mechanism could open a real
`linked` aspect here." Phases 2–4 were that pass, so promoting the row is
following the certificate, not editing one to make room for a feature.

### 11.2 The oracle

`tools/linkage_oracles/C3DPickupItem.py` compiles the real, unmodified
`behavior_item.c` / `behavior_pickup_core.c` / `gamestate.c` (plus
`behavior_ai_trigger.c` for the shared tag dispatch) and drives them over
**every one of the 383 shipped `3PIC` rows** in the 35 levels. Per row it
diffs four things against expectations computed from the recovered bodies and
that row's own authored properties — never from a tuned constant:

1. **Load gate.** After the real `on_spawn`, a row authoring
   `InitallyActive=0` is latched uncollectible and one authoring 1 is not.
2. **Order and effects, funded.** Seed exactly `ReqPicNumAmount` of
   `RequiredPicNum`, touch the row, and diff the *whole ordered event
   sequence* — gate, each state dispatch and its outcome, the award, the
   next-trigger, the sound — against what the decompiled order predicts.
3. **Refusal.** Seed one short: the gate must refuse, play `NeedMoreSound`
   where one is authored, emit nothing else, and consume nothing (no partial
   drain).
4. **Gate before collected-check.** Mark the row collected, fund it, touch it
   again — the currency must still be taken, because the gate runs first. A
   port that reordered those two leaves the count untouched and is rejected.

`--selftest` mutation-tests the oracle against three defects it must catch:
swapping the gate and the collected-state check, moving the award ahead of the
side-effect dispatch, and consuming on a refusal. All three turn it red.

The dumper reads the engine's own `[PICGATE]` / `[PICSTATE]` / `[PICFIRE]` /
`[PICAWARD]` lines rather than paraphrasing them, so the instrumentation cannot
drift from the behaviour; it adds only what the engine cannot say for itself
(post-spawn state, score delta, the stubbed sound's position, and the three
probe separators).

### 11.3 Two real defects the oracle found

Both while it was being written, which is the point of building one:

- **`Points` is a phantom property.** `gam_loader.c` mapped a property named
  `Points` onto `Entity.points`. That name appears nowhere in the 35-file
  corpus — the authored field is `PointValue` (383 rows, class doc offset
  `0x620`, awarded through `FUN_0042adc0`). `Entity.points` had therefore always
  been 0 and **no pickup in the native port had ever awarded score**. §8.5
  recorded this and deliberately left it; certifying `HandlePickupCollection`
  made it in-scope, because the recovered award step is "PIC_NUMBER *and*
  score" and excluding half of it would certify a different function.
- **Unset scored negative.** With `PointValue` loading, the award tested
  truthiness (`if (e->points)`), and `-1` is the format's universal unset
  convention — so every row authoring no score subtracted a point. Now it
  awards only a positive value.

### 11.4 What the certificate does not claim

Carried into the certificate note and the class doc, not buried here:
`PickedUpIndex`'s replacement-sprite swap; `TimesToTrigger`/`trigger_count`
repeat limiting (native latches once-only); `IsAmbient`; `PassThru`/`ShowArrow`
(no isolated consumer in the decomp either); the state slot on every class
except the pickup family, so non-`3PIC` `ActivateObject`/`ToggleObject` targets
and **all** `NextTrigger` targets resolve and find no native body; the sound
*mix*, of which only the sequence position is certified; whether the original
hides an `InitallyActive=0` pickup (§9.3); and the `3FIS`/`3GIR`/`3DIN`
creature leaf, which is a different FourCC on a different vtable.

### 11.5 Where this leaves the plan

Phases 0–6 are done. The open items are no longer wiring:

- the count shortfall in §8.3 — four levels beyond the pre-grant table cannot
  meet their own picture demand, and four vending machines are unaffordable on
  a cold single-level entry. An owner call, not a bug.
- whether an inactive pickup is invisible (§9.3) — needs a capture.
- `3RED` (70 rows) and `3ANI` (6) carry `PickupIndex` and share `CPickupType`
  but do not consult the collected-state table, so they re-collect on every
  level entry.
- the `DrawHud` counter producers (§10), which would let the native readout be
  replaced by the real one.

---

## 12. Owner playtest, 2026-08-20

The first time a human drove this, Level 1A announced **LEVEL CLEARED** after the
second candy. Two defects, both introduced by phase 4, neither reachable by any
existing check:

**`InitallyActive` had two owners.** `behavior_trigger_spawn_base` already
handled the field (`behavior_base.c`), so phase 4's
`behavior_pickup_spawn_gate` was a second copy — and the base's copy cleared
`ENTITY_FLAG_TRIGGER` *before* `item_on_spawn` could read it to decide whether
the row belongs in the level's item tally. Level 1A therefore counted **3**
items instead of 5 and cleared three pickups early. `InitallyActive` is a
`CPickupType` field and only `3PIC` authors it (376 rows, 28 of them 0), so the
block was inert for every other caller of the shared trigger base — button,
checkpoint, laser trigger, load, neutron, pickup, switch, trig, trophy. It now
lives only in the pickup core.

**A re-armed machine was counted every purchase.** `items_collected` ran past
`items_total` ("collected 4 / 3") because a vending machine can be bought
repeatedly. A pickup now counts once, tracked by `Entity.pickup_counted` — a
flag distinct from `user_flag` precisely because `SetPickupItemState` state 1
clears that one to re-arm the product.

That fix would in turn have broken the pickup animation, which
`behavior_player.c` triggers by watching `items_collected` rise: a second
purchase would no longer have animated. Two questions were riding on one
counter — "how much of this level is done" (count once) and "did something just
get picked up" (every time). They are now separate: `gamestate_pickup_events()`
is the edge, `items_collected` is the total.

**Why nothing caught this.** The `C3DPickupItem` oracle stubs
`behavior_trigger_spawn_base`, so it could not see the duplicated handling — the
bug lived exactly in the seam the stub covers. `verify_picture_economy.py`
drives collection through the sweep, which ignores the item tally entirely. A
useful reminder that a green oracle certifies the body it compiles, not the
lifecycle around it.

### Still open from the playtest

`jimpickup.ASE` is the odd clip out: **792 faces where every other player
animation has 814**, and it is the only clip besides `jimstop` that carries a
real texture reference (`jimycarl.png`; the rest export `tex='(none)'`). Every
player clip except `jimstop` also lacks `MESH_TVERT` and gets object-space UVs
generated at load. That is an asset/export problem, not a picture-economy one,
and it is the likely source of the long-standing "pickup animation is broken"
report — swapping to a differently-topologised mesh mid-play would visibly pop.
Not investigated further here.

---

## 13. Owner playtest round 2, 2026-08-20

### 13.1 `InitallyActive` had a *third* owner, and it was the renderer

The empty vending tray was not a decision, it was a bug. `draw_scene`
(`main.c`) skipped any entity whose **authored** `InitallyActive` is 0 —
forever. That is correct at boot (2026-06-11 QA established the original does
not draw these) but it reads a `.gam` property, not runtime state, so nothing
could ever un-hide one. The candy stayed invisible in the tray even after the
machine released it.

Together with §12's discovery in `behavior_trigger_spawn_base`, the field had
**three** owners: the spawn base, the renderer, and phase 4's pickup gate. It
now has one. `behavior_pickup_spawn_gate` raises `Entity.pickup_inactive` from
the authored property at spawn; `SetPickupItemState` (004360b0) lowers it,
because both recovered states show the pickup; the renderer reads the flag.
Boot behaviour is unchanged — the `level1` golden is byte-identical — and
activation now actually reveals the product, which is the entire point of an
activation.

That also settles §9.3's open question in the only direction the evidence
allows. The original *does* hide these at boot (QA-established); what was
missing was the un-hide on activation, not the hide.

### 13.2 The pickup card

Owner request: *"typically when you pick up, you get a popup with the item card
with a number in the top right hand corner of the card to show how many of that
item you have."*

The **hook** is decomp-supported. `C3DPickupItem`'s Assets table lists
`FUN_004061b0` / `FUN_004061c0` / `FUN_004061d0` as the picture/inventory
service its collection path calls, and `docs/decomp/_scene_sequencer.md` names
`FUN_004061d0(id, _)` as the "On-screen `+counter` notify queue". So a pickup
really does raise a counter notification, at the point we now raise one.

The **layout** is not. The original draws it from the `C2DInGameMenu` canvas
records the `hud-draw` certificate is still `linked-blocked` on, and
`assets/omt/inventory.omt`'s twelve 50x50 / 40x40 / 20x100 chunks cannot be
tied to picture ids without the reward-grid table (`FUN_004038c0(list, slot,
v)`), which is also unrecovered. So the card is native chrome — same stance as
§10 — and it shows **the pickup's own sprite**, which we do know, because
`SpriteIndex` resolves through the generated chunk map. Nothing unrecovered is
invented.

It sits under the picture readout, holds for 2.5 s, fades over the last half
second, and carries the count in its top-right corner. A pickup that awards no
picture gets the card without a number rather than a misleading zero.

### 13.3 The pickup animation is an asset defect, and it is now pinned

`assets/ase/jimpickup.ASE` is broken as exported, in two ways:

- It contains **28 `GEOMOBJECT`s**: the character mesh `01jimmy` *plus the
  entire Biped rig* (`Bip01`, `Bip01 Head`, `Bip01 L Calf`, `Bip01 Footsteps`,
  …). The loader takes only `01jimmy`, so the bone soup is not drawn — but the
  export is plainly wrong.
- Its `01jimmy` is **407 vertices / 792 faces**, where every other player clip
  is **426 / 814**, and the file has **no `MESH_TVERTEX` at all**.

Only `jimstop.ase` ships real texture coordinates (3551 tverts); every other
clip gets them at runtime from `player_anim.c`'s `copy_shared_jimmy_uvs`, which
copies UVs from `jimstop` — and **bails when the vertex counts differ**. That is
true for exactly one clip: the pickup. So `jimpickup` alone renders with the
loader's generated object-space UVs, which is why Jimmy's head becomes a flat
slab and his body a jumble of colour blocks for the 0.45 s the pose is up.

**Not fixed here, deliberately.** The honest repairs are (a) re-export the clip
with UVs and without the rig, or (b) transfer UVs from `jimstop` by nearest
vertex. (b) is tempting and cheap, but the two meshes are in *different poses*
at frame 0 — idle versus reaching down — so nearest-position matching would tie
a hand vertex to a knee. That is guessing at a hero character's texturing, which
is exactly the class of thing this project refuses to do without evidence. It
wants the source asset, or a decision to accept an approximation.

---

## 14. The pickup animation was never a shipped animation (2026-08-20)

§13.3 recorded `assets/ase/jimpickup.ASE` as a broken export and offered to
re-export it. **There is nothing to re-export from, and the owner's reading is
the better one: it is a leftover.**

### 14.1 The search for a source, and what it found

| Candidate | Result |
|---|---|
| `/home/scotty/xp-jnbg-original/` | 8 `VR*.omt` files only — no character source |
| `.grn` (Granny) anywhere in `assets/` | none |
| `assets/glb/ase/jimpickup.glb` | has `TEXCOORD_0`, but **one distinct UV pair, all zeros** — a placeholder, generated from the same broken ASE |
| `assets/glb/ase/jimstop.glb` | same: all-zero UVs |
| repo history | the ASEs arrived in one bulk `phase12 baseline` commit; no extractor reproduces them |

`jimstop.ase` is the **only** Jimmy file in the tree carrying real texture
coordinates (3551 tverts). Every other clip borrows them by vertex index at
runtime. Nothing here can supply UVs for a 407-vertex mesh.

### 14.2 It is the odd one out on every axis

| | the other 21 clips | `jimpickup` |
|---|---|---|
| mesh | 426 verts / 814 faces | **407 / 792** |
| `GEOMOBJECT`s | 1 (`01jimmy`) | **28** — `01jimmy` *plus the whole Biped rig* (`Bip01`, `Bip01 Head`, `Bip01 L Calf`, `Bip01 Footsteps`, …) |
| pose space | shares `jimstop`'s | **different**: 0 of 407 vertices coincide with `jimstop`'s, median nearest-vertex distance **11.8 units** on a ~145-unit character, and the bbox spans differ on every axis |
| texture coords | none (borrowed by index) | none, and **cannot** borrow — the vertex counts differ, so `copy_shared_jimmy_uvs` bails |

A clip that is alone in shipping the raw Max rig, alone in its topology, and
alone in its pose space is not a damaged member of the set — it never went
through the pipeline the other 21 did. Combined with the owner's report that no
pickup animation appears in an original playthrough, the conclusion is that this
is an unused leftover asset.

### 14.3 What was removed

The player no longer selects `PA_PICKUP`. The 0.45 s countdown, the
`items_collected` edge watch that armed it, and `gamestate`'s pickup-event
counter all go with it — that counter existed only to keep this animation firing
once the level tally became count-once (§12), so with the animation gone it had
no consumer.

Kept on purpose: the `PA_PICKUP` enum value and its `PICKUP`/`HIPICKUP` dispatch
alias, so pose ids do not shift and the `C3DAnimated` dispatch surface is
undisturbed; and `jimpickup.ASE` on disk, because `entity_visual.c` uses it as
the `3PIC`/`ITEM` fallback mesh.

### 14.4 The hovering pickups

Unrelated bug, found in the same pass. `item_on_update` advanced a
**function-static** clock shared by every pickup, incremented once per pickup
per frame — so the hover ran N times too fast in a level with N pickups, and
changed speed whenever one was collected. Each entity already carries its own
clock (`anim_time`, advanced once per frame by the animated base); the bob now
uses that, with the existing `x` term keeping neighbours out of phase.

This is a real change to what level 1 looks like in frame 0, so the `level1`
goldens were regenerated — in the repository Docker image via
`make regen-goldens`, never on a physical GPU. The change was isolated first:
reverting only the bob line and re-running `make check-assets` turns everything
green, which proves the animation removal contributes nothing to the pixels and
the bob is the sole cause. Delta: **559 px (0.061%)** in frame 0 and **681 px
(0.074%)** in frame 1, confined to the band the pickups occupy. The 8 `fixture0`
goldens regenerated byte-identical.

---

## 15. Original-game observations, 2026-08-20

The owner brought the XP machine up and ran the original. Three things the port
could not settle from static evidence are now settled. (Getting there took a
detour: XP had a static `192.168.18.41` that another device had taken, and
Windows drops an adapter's IP on conflict detection — so the box answered ARP
and ICMP but refused every TCP port, before *and* after a firewall change. It
reads exactly like a firewall problem and is not one; the TTL fingerprint
(`ttl=255`, appliance-like, not Windows' `128`) is what broke it open. XP is now
DHCP-reserved at `192.168.18.3`.)

### 15.1 An inactive pickup is hidden — §13.1 confirmed

The candy is **not** visible in the machine's tray before you pay. That is what
the port now does: `behavior_pickup_spawn_gate` raises `Entity.pickup_inactive`
from the authored `InitallyActive`, the renderer skips it while set, and
`SetPickupItemState` lowers it when the machine dispenses. **No change needed** —
the behaviour was already right, and the open question in §9.3/§13.1 closes.

### 15.2 There is no pickup animation — §14 confirmed

Jimmy plays no pickup pose in the original. That independently confirms the call
in §14 to retire `jimpickup.ASE`, which was reached from asset evidence alone
(the only clip on a divergent mesh, the only one shipping the raw Biped rig, the
only one with no UVs). Two independent lines of evidence, same conclusion.

### 15.3 Pickups display a 3D red translucent arrow — `ShowArrow`, unported

**This is new, and it resolves an open question in the class spec.**
`docs/decomp/C3DPickupItem.md` lists `ShowArrow` (0x6a0, ctor default 1) as
"registered and defaulted here but not consumed by the owned methods examined".
The owner's observation names the consumer: a pickup shows a **3D red
translucent arrow**.

Every piece of static evidence lines up:

| Evidence | Value |
|---|---|
| rows authoring `ShowArrow` | 353, **all of them `3PIC`** |
| value distribution | `1` x347, `0` x5, `-1` x1 |
| the 6 exceptions | all invisible or inactive pickups — `mumticket`, `mumticket2`, `refill` author the "hidden" sprite canvas 106; `gcan`, `rescue` author `InitallyActive=0` |
| the asset | `assets/ase/3Darrow.ASE`, 60 verts / 28 faces, real UVs |
| its own export path | `D:\Jimmy (ken)\`**`3D Items (pick up)`**`\3D ARROW\3Darrow.bmp` |
| its texture | `assets/png/3Darrow.png`, red — `rgba(255,89,89)` dominant |
| native usage | **none** — `3Darrow.ASE` is never referenced |

Note it is a *different* arrow from `3ARR`/`C3DArrow`, which is a
`C3DSpriteType` billboard drawing sprites.omt canvas 33 (the big yellow
direction arrow). The `3Darrow.ASE` row was deliberately removed from the `3ARR`
mapping on 2026-06-12 (QA #3) for exactly that reason — but nothing ever picked
the mesh up for pickups, which is what it was made for.

There is already one hardcoded arrow in `main.c`: `draw_goddard_dish_arrow`
reads `ShowArrow` but gates on a single row (`3PIC` + hidden sprite +
`RequiredPicNum==4` + `ToggleObject=="refill"` + `NextTrigger=="godeat"`, i.e.
level1b's `gdish`) and draws the **2D** canvas-33 billboard. That is a
special-case stand-in for the general mechanism, and it should fold into it.

**Still unmeasured, and required before porting:** the arrow's height above the
pickup, its scale, whether it bobs or spins, and how it is blended — the PNG has
no alpha channel at all (every pixel is `255`), so the translucency comes from a
material or blend state, not the texture. Porting without those would be
fabricating a visual, which is the thing this project refuses to do.

---

## 16. `ShowArrow` ported, and the nav arrow's pulse (2026-08-20)

§15.3 recorded the red pickup arrow as observed-but-unported. It is ported now,
against a second round of owner comparison with the original running on XP.

### 16.1 The asset was `pointarrow.ASE`, not `3Darrow.ASE`

§15.3 named `3Darrow.ASE`, reasoning from its export path
(`D:\Jimmy (ken)\3D Items (pick up)\3D ARROW\3Darrow.bmp`). Wrong file. Owner:
"it is 3d" — and `3Darrow.ASE` is not:

| | `3Darrow.ASE` | `pointarrow.ASE` |
|---|---|---|
| node | `Line04` (a spline) | `pointarrow` |
| size | 102 x **0** x 103 — flat | 55 x **13** x 76 |
| verts | 60 | 11 |
| `*BITMAP` | `…\3D Items (pick up)\3D ARROW\3Darrow.bmp` | `D:\neutron\run\png\3Darrow.png` |

Both reference the same red texture, which is what made the asset catalog's
`textures/3Darrow` line look like it pointed at `3ARR`. `3Darrow.ASE` is the
flat source outline; `pointarrow.ASE` is the real arrow.

**And the artist left the measurements in the file.** `pointarrow.ASE` contains
two objects — the arrow *and a Jimmy reference model*, posed together:

- `01jimmy` spans up `0.6 → 215.3`; the arrow spans `39.7 → 115.2`
- arrow height **75.5 = 35% of Jimmy** — the observed "size of his torso"
- the arrow sits **40–115 above the origin**, so the mesh is authored with the
  **item at the origin** and the arrow already floating over it

So the port needs no height constant: draw the mesh at the pickup's position.
Its top lands ~5 units above Jimmy's head, matching "starts slightly shorter
than the top of his head". The loader takes the arrow and rejects the Jimmy
object as a mismatched frame.

### 16.2 The arrow marks what you can take *now*

Observed: at the vending machine the arrow sits at the machine's mid height, and
only once the item is dispensed does it sit above the item. That is not a
special case — it falls out of one rule. Before paying, the available pickup is
the machine trigger (`cmach`, y=249, arrow spanning 289–364 against a machine
spanning 103–506); the product is inactive and shows nothing. After paying the
product is live and carries its own.

This retired two earlier guesses of mine: that hidden-canvas pickups should be
exempt, and that inactive pickups should show an arrow. The plain
"uncollected and active" test was right.

### 16.3 Items do not bounce

Owner: the original does not hover its pickups at all. So §14.4's "the hover
runs N times too fast" fix was treating the symptom of a mechanism that should
not exist. `ITEM_BOB` is gone. It was also what made the arrow bob, since the
arrow draws at `e->y`.

### 16.4 Two new renderer capabilities, one kept

**`renderer_set_model_alpha`** — the marker is translucent, and the model path
had no alpha or tint parameter (only billboards did), while `3Darrow.png` has no
alpha channel at all. The lit shader already wrote a hardcoded `1.0` output
alpha, so that constant became a uniform defaulting to `1.0`; blending is
enabled only while a caller asks for less. Writing `1.0` is bit-identical to the
literal, which the untouched `fixture0` goldens confirm.

That exposed a second bug: the arrows vanished entirely at first. `draw_scene`
renders entities and *then* static placements, so a blended draw with depth
writes masked was painted over by the vending machine. Translucent geometry has
to come last, so the arrows now draw in their own final pass.

**`renderer_set_billboard_wave`** — a travelling V-displacement so the nav
arrow's wave could pass *along* the graphic. Built, demonstrated, and then
**reverted at owner preference**: the simpler uniform vertical stretch reads
better. Recorded here because the finding is worth keeping even though the code
is not — if the travelling wave is ever wanted, the shape of it (and the need to
window it by `sin(pi*v)` so it does not shear the sprite's tip off) is known.

### 16.5 `C3DArrow` (3ARR), the yellow nav arrow

Separately observed: it pulses and carries a red tint. It is a `C3DSpriteType`
billboard on sprites.omt canvas 33, and the billboard path already carries a
tint, so both are a change at the draw site. The sprite stretches along its
**height only** (width holds) by ±38% at 6.5 rad/s, and the red tint rides the
same phase as a glow rather than being painted on constantly.

All of these numbers are calibration against the owner's eye, not measurements —
the mechanisms are observed, the constants are tuned. They sit together at the
top of the draw code.

### 16.6 Not settled

Whether the arrow also appears over a dispensed item *before* it is picked up.
The current rule says yes.

---

## 17. The gadget inventory and the action menu (2026-08-20)

The picture economy is one of two things `C3DPickupItem` does. This section is
the other one: what a pickup *gives* you, and the menu the original used to
choose between them.

### 17.1 The old tool table was mostly invention

`behavior_item.c` carried a `TOOL_GRANTS` table mapping `.gam` ObjectTags to
inventory tools. Scanned against all 35 levels, four of its nine entries --
`watergun`, `jetpack`, `burpgun`, `glasses` -- match **no ObjectTag anywhere in
the corpus.** Its comment claimed they "gate real progression in the native
port today"; nothing in the tree gates on them. The only consumer of the
inventory is `behavior_player.c`, and it asks only for `"baseball"`.

### 17.2 Two rules separate a gadget from the other eighty pickups

A scan of every named `3PIC` row produces ~80 tags. Two properties in the data
sort them, and neither is the name:

**A row that awards or requires a picture is an economy item, not a gadget.**
`wrench1` and `wrench2` award `PIC_NUMBER 18`; `hydrant` and `water2` in the
same level carry `RequiredPicNum 18`. So the wrench is *spent at the hydrant*,
not carried in a pocket — the old table modelling it as a permanent tool was
wrong in both directions at once. `passcard` (`PIC_NUMBER 25`) has the same
shape.

**A row drawn on `sprites.omt` chunk 106 is an invisible trigger volume, not an
item.** Every `RequiredPicNum`-gated machine in the corpus uses 106 — `cmach`,
`fmach`, `mach`, `mdiam`, `fp`, `cm`, `cjar`, `book`, `kitty`, `nest1/2`. This
is what a vending machine actually *is*: a blank trigger sitting over the
machine's model. It also retro-explains §16.2 — the machine's `ShowArrow`
marker floats at mid-machine height because the pickup there is the invisible
trigger, and there is no visible item for it to sit above until one is
dispensed.

What survives both rules is small:

| tag | level | sprite | kind |
|---|---|---:|---|
| `shrinkray` | level1b | 99 | gadget |
| `invisibility` | level5 | 114 | gadget |
| `bubblepickup` | level7 | 26 | gadget |
| `scooterpart` | level1c | 111 | part |
| `sewerpart` | level1a | 134 | part |
| `foil` | level1b | 183 | part |
| `godphone` | level1 | 184 | part |

Gadget vs part is drawn from wiring rather than names: `sewerpart` fires
`movegoddard` on pickup, and `scooterpart` is the scooter the AMI mode table
already knows about. `applepie` x3 and `vertitem` are visible and picture-free
but carry only a `PointValue` and no wiring at all, so there is no evidence they
are carried rather than simply scored; they stay out.

The icon is the pickup's own `SpriteIndex`. The original fills its inventory
from `C2DInGameMenu` canvas records that the `hud-draw` certificate is still
blocked on, but the sprite id is right there in the row — so the art is
authentic even though the layout is ours.

### 17.3 The HUD already had a gadget panel, drawing nothing

`hud_layout_generated.h` has carried four capture-backed quads tagged `gadget`
since the frame-8881 extraction — a 64x64 body with its right edge, bottom edge
and corner, bottom-left of the screen. They have been drawing as an empty bezel
ever since, because nothing ever put anything in them. The selected gadget's
sprite now goes inside, inset so the frame still reads as a frame.

Empty-handed draws nothing extra, which is both what the panel always looked
like and why `level1`'s goldens are unmoved: those frames are captured before
anything has been picked up.

### 17.4 The action menu — "AMI", the original's own word for it

`JimmyEnterActionMenuLock` (`00425ef0`) and its reverse (`00425b20`) are
recovered, and `SelectJimmyGadgetOrVRMode` (`00428d50`) logs `"CAll in AMI %d"`
on entry and `"Exiting AMI"` on the way out.

Most of both bodies is traffic to Jimmy's `0xa18` — a code-created
`C2DInGameMenu` that is simultaneously the HUD overlay and the gadget command
endpoint, addressed through eighteen vtable slots. **None of that is ported and
none of it should be.** The `C3DJimmy/gadget-mode-dispatch` certificate is
`linked-blocked` precisely because native has no such controller; building a
fake one would certify a different design.

What survives translation is the observable half, in the decompiled order: the
`DAT_004ec494 && !DAT_004f8181` guard, the open latch, the pause, the cursor
(shown on enter unless the argument is `2` — what `2` means was not recovered,
so it stays a parameter rather than being folded away), the gadget-cooldown
clear, and the `DAT_004f8434` / `DAT_004f8182` flips.

The pause is not a new mechanism. `main` already freezes the entire simulation
while the front-end menu or the QA browser is open; the action menu joins that
same gate, which is the native analogue of global game slot `0x168(1)`.

### 17.5 The AMI tables, and a free cross-check

`00428d50` is one switch over the request id. Every arm writes `DAT_004f0588`
and, when the game-type probe returns `2`, routes to a VR level through
`(vrNN.gam, "PHONEBOOTH", ...)`:

| id | mode | VR route |
|---:|---:|---|
| 0 | 0 *(or -1 on the other arm)* | vr01 |
| 1 | 1 — Rocket | vr02 |
| 2 | 2 *(or -1 on the other arm)* | vr03 |
| 3 | *(default arm, no mode write)* | vr04 |
| 4 | 4 | vr05 |
| 5 | 5 | vr06 |
| 6 | 6 — aim/shoot | vr07 |
| 7 | 7 | vr08 |
| 8 | *(no mode write)* | — |

The routes are a clean `id -> vr(id+1)` ladder, and that is worth stating
because it is an **independent cross-check**: `CMainMenu`'s already-`linked`
level-routing table carries the same eight VR levels, recovered separately, and
the two agree.

Only two modes are named in the port. Mode 1 is Rocket — its arm traces
`"Activating Rocket"` / `"ACT 2 Rocket"` and plays `DRIVE`. Mode 6 is aim/shoot
— the controller update clamps pitch to `[0,45]`, builds `(0, aim+80, 45)`
through slot `0x384`, and uses `SHOOT`. The rest keep their numbers with the
evidence in a comment; naming them would be interpretation dressed as fact.

### 17.6 What is deliberately missing

**Which AMI id a given gadget corresponds to.** That mapping lives in the
`C2DInGameMenu` canvas records the `hud-draw` certificate is blocked on. So the
menu selects a gadget (native chrome, our layout) and the AMI table is exposed
separately for callers holding a real id. The two are not joined by a guess,
because a guess there is exactly what would make a future oracle certify the
wrong thing.

### 17.7 Evidence

`tools/verify_gadget_menu.py` holds its own copy of both the AMI table and the
expected grants, read off the evidence rather than off the engine, so a
transcription slip in either place shows up as a disagreement rather than as
agreement with itself.

- `JN_TEST_AMI=1` drives all nine ids through the real `ami_dispatch()`; all
  nine modes and routes match.
- Seven tag/level pairs collect through the `JN_TEST_PICTURES` sweep and land
  in the inventory with the right kind.
- The negative holds: `wrench1`, `wrench2`, `passcard`, `water2` and `hydrant`
  grant no inventory slot at all.
- `foil` and `refill` are `InitallyActive=0` and correctly stay out of a cold
  sweep's reach — the gate from §16.2 doing its job.

### 17.8 Not settled

Whether the `2` that suppresses the cursor on enter is the same `2` that
suppresses hiding it on exit (`controller+0x4d4`). Both are unrecovered, and
native has no controller to ask, so the exit path always restores the cursor.
