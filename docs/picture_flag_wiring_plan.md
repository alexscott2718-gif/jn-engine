# Picture-flag wiring plan

**Goal:** consume the pickup gating data the shipped `.gam` levels already
carry, replacing the ad-hoc tool model's silent pass-through with the
original's picture-flag economy — and open a certifiable `linked` aspect on
`C3DPickupItem`, which `docs/linkage_certificates.csv` currently records as
having "no decompiled-body fidelity to certify".

Status: **proposal**, nothing implemented. Measurements below were taken
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
