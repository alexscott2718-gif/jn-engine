# CLoadLevel

## Identity

| Item | Value |
|---|---|
| RTTI name | `CLoadLevel` |
| Base chain | `C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d024c`, `004d025c`, `004d06ac`, `004d06c0` |
| Ctor(s) | installs the `CLoadLevel` vftables; `InitObject` registers FourCC `LOAD` (`0x4c4f4144`) |
| Dtor(s) | inherited `C3DSprite` deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`CLoadLevel` is the **level-transition portal** — the placeable `LOAD` object that,
when the player enters its radius (and any task/level prerequisites are met), fades the
screen and loads another level at a named start point. It derives from `C3DSprite`
(a 2D canvas/sprite element) — the portal can carry a sprite/marker. It is the
data-driven counterpart to the menu's hard-coded `NewGame.tsk` route: the *in-world*
way the game moves between the 35 `.gam` levels. (FourCC `LOAD`; see
`docs/_gam_classids.tsv` `LOAD -> C3DLoadLevel/CLoadLevel`.)

## Field Map (registered `.gam` properties)

Registered by `InitObject` (`vfunc_01_007`) via the `vftable+0x3fc` registrar; types
are `.gam` type ids (`1=string 3=float 6=int`). Strings resolved from `Neutron.exe`.

**The Offset column is in dwords — the registrar's own units.** Multiply by 4 for
the `this` offset the runtime bodies read: `LevelName` `0x148`→`this+0x520`,
`StartPoint` `0x15c`→`+0x570`, `RequiredTask` `0x170`→`+0x5c0`, `RequiredLevel`
`0x184`→`+0x610`, `SoundIndex` `0x185`→`+0x614`, `ExactLevel` `0x186`→`+0x618`,
`FadeType` `0x187`→`+0x61c`, `FadeTime` `0x188`→`+0x620`. Five of those eight are
read at exactly that byte offset by the recovered gate body, which is what
establishes the rule and pins the other three — see the 2026-08-21 section of
`docs/decomp/evidence/cloadlevel_gate_00457ec0.md`, where it also resolves three
apparent `.rdata` globals in the sound/fade tail into these property reads.

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x148` | string | `LevelName` | Target level `.gam` to load (e.g. `level1d.gam`). |
| `0x15c` | string | `StartPoint` | Named spawn/start-point tag in the target level. |
| `0x170` | string | `RequiredTask` | Task that must be satisfied before the portal will fire. |
| `0x184` | int | `RequiredLevel` | Minimum level/progress gate. |
| `0x186` | int | `ExactLevel` | Exact level-match gate (alternative to `RequiredLevel`). |
| `0x0d` | float | `Radius` | Proximity radius the player must enter to trigger the load. |
| `0x185` | int | `SoundIndex` | Sound played on activation. |
| `0x187` | int | `FadeType` | Screen-fade style used during the transition. |
| `0x188` | float | `FadeTime` | Fade duration. |

`InitObject` also calls the shape subobject (`this[-0x32]`) slot `0xc0` with
`0x4c4f4144` = the FourCC **`LOAD`** (little-endian `'LOAD'`), registering the class id.

See `docs/gam_schema.md` for the per-level `LOAD` rows and the actual
`LevelName`/`StartPoint` values used across the game's level graph.

## Vtable Methods (owned)

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 7 | `00457da0` | `InitObject` | Registers the 9 `.gam` properties above + FourCC `LOAD`. | non-trivial |
| recovered contact/gate body | `00457ec0` | `TouchLoad` / gate caller | Handles Jimmy contact, evaluates `RequiredTask`/`RequiredLevel`/`ExactLevel`, handles `RETURN`, hides the portal on the normal path, plays optional `SoundIndex`, applies optional fade, and dispatches `LevelName`/`StartPoint`. | non-trivial |
| vtable 3 slot 53 | `00458370` | `ActivateLoad` | Fires the transition: hides the portal (slot `0xd8`) and calls the global game object (`*DAT_00509980`) slot `0x100` with the request block at `this+0x17a`, handing off the `LevelName`/`StartPoint`/fade request to the level loader. | non-trivial |

### Activation behavior

```c
CLoadLevel::ActivateLoad():                  // vfunc_03_053 @ 00458370
    reset request block at this+0x18e
    this->hide()                             // slot 0xd8 (C3DSprite hide)
    global_game = *DAT_00509980
    global_game->slot_0x100(&this->load_request)   // this+0x17a -> begin level load
```

The proximity/contact test starts in the inherited trigger/collision path; `Radius`
sizes that contact volume. The recovered contact body at `00457ec0` is the missing
gate caller:

```c
TouchLoad(toucher):
    if !toucher.IsA("C3DJIMMY"):
        return

    if RequiredTask != "none":
        state = task_state(RequiredTask)          // FUN_0045fea0
        if state != -1:
            if state < RequiredLevel:
                return
            if ExactLevel != -1 && state != ExactLevel:
                return
        else:
            log_missing_task_and_continue()

    if LevelName == "RETURN":
        run_return_loadpoint_path()
        return

    if LevelName != "none":
        maybe_prepare_jimmy_for_current_level()
        hide_this_load_portal()                  // slot 0xd8
        jimmy_load_handoff(LevelName, StartPoint)
        if SoundIndex != -1:
            play_sound(SoundIndex)
        if FadeType != -1:
            apply_fade(FadeType, FadeTime)
            lock_or_transition_jimmy()
```

`ActivateLoad` (`00458370`) is a smaller direct commit helper: it hides the portal
and hands the request block to the global game controller (`CGameType`/`CJimmyGame`
at `DAT_00509980`). The normal Jimmy contact path above performs more of the
player/fade/sound handoff inline before transition.

## Constants And Wiring

| Item | Value | Evidence |
|---|---|---|
| FourCC | `LOAD` (`0x4c4f4144`) | `InitObject` shape slot `0xc0` immediate |
| Level graph | `LevelName` → target `.gam`, `StartPoint` → spawn tag | `.gam` `LOAD` rows; `docs/gam_schema.md` |
| Gate | `RequiredTask` / `RequiredLevel` / `ExactLevel` | progress prerequisites checked before firing |
| Loader handoff | `*DAT_00509980` slot `0x100` | the current game-mode object performs the swap |

## Assets

| Kind | Name | Notes |
|---|---|---|
| Sprite/marker | inherited `C3DSprite` canvas | Portal may carry a 2D marker; no own ASE/PNG registered in `InitObject`. |

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java CLoadLevel` (`slots=338`, `owned_methods=2`,
`offsets=11`); all 9 properties + the `LOAD` FourCC + the loader-handoff call are read
directly from the decompiled `InitObject`/`ActivateLoad`. Not runtime-validated.

Open questions:
- Decode the exact request-block layout at `this+0x17a` / `this+0x18e` passed to the
  loader slot `0x100`.
- ~~Confirm where the `Radius` proximity + `RequiredTask`/`RequiredLevel` gate is
  evaluated (inherited collision/update slot) and how it calls `ActivateLoad`.~~
  **DONE 2026-07-02**: contact is inherited, and the recovered `00457ec0` body is
  the gate/Jimmy handoff path. It does not literally call `00458370`; it performs
  hide/player/sound/fade dispatch inline for the normal path.
- Tie `FadeType` values to the fade implementation.

## Notes

- Evidence: `DumpClass.java CLoadLevel /tmp/dumps2/decomp_CLoadLevel.md`; FourCC string
  `'LOAD'` and the `level1d.gam` reference confirmed via PE string resolution.
- Hand-written from the decompiled bodies (supersedes the generated skeleton). This is
  the in-world level-graph edge; the menu's `NewGame.tsk`/VR routes (see
  [`CMainMenu.md`](./CMainMenu.md)) are the front-end equivalent.

## Native Linkage (linked-parity branch)

Aspect: **`gam-deserialization`** — status `linked`.
Certificate: `docs/linkage_certificates.csv`; oracle: `tools/linkage_oracles/CLoadLevel.py`.

`InitObject`'s 9 registered properties (above) are read out of the shared `.gam`
binary record format (magic + object-count header; per-object FourCC + prop-count;
per-property `u8 name_len` pstring + `u32 BE type_id` + `u32 BE val_len` + typed
value; per-object `u32` checksum). `CLoadLevel` doesn't implement that reader itself
— it is the generic `.gam` deserializer (`gam_load`) shared by all 93 placeable
classes — so this aspect proves the generic record parser using `LOAD`'s field set
(all three `.gam` type ids: `1=string 3=float 6=int`) as the concrete exercise.

### L2 — transcription map

| Format element / reference (`tools/gam_parser.py`) | Native (`src/engine/assets/gam_loader.c`) |
|---|---|
| `parse_gam` header: 4-byte magic + BE u32 `object_count` | `gam_load` initial `fread`/`read_u32be` |
| Per-object header: 4-byte FourCC + BE u32 `prop_count` | per-object `fread`/`read_u32be` in the `oi` loop |
| `_read_prop`: `u8 name_len` + name bytes | `name_len` byte + `fread(prop_name,...)` (with the overlong-name guard skipping via `fseek`) |
| type id 1 (string): 1-byte redundant length prefix + `val_len` bytes | `fseek(f,1,...)` then `fread(str_val,...)`, truncating into a 256-byte stack buffer with a compensating `fseek` for the remainder |
| type id 3 (float32 BE) | `read_f32be` (reads BE u32 via `read_u32be`, `memcpy`s the bits into a `float` — a correct BE→native reinterpretation) |
| type id 6 (int32 BE, `-1` = unset) | `read_u32be` cast to `(int32_t)` |
| unknown/raw type ids (2/4/…) | `fseek(f,val_len,...)` — silently skipped (see deviation below) |
| per-object trailing checksum (4 bytes, skipped) | `fseek(f,4,SEEK_CUR)` |
| `LevelName`/`StartPoint` → `props['LevelName']`/`['StartPoint']` | copied unconditionally into `e->target_level`/`e->start_point` (named fields) |
| `RequiredTask`/`RequiredLevel`/`ExactLevel`/`Radius`/`SoundIndex`/`FadeType`/`FadeTime` → `props[...]` | not individually named in `gam_loader.c` (`CLoadLevel` isn't special-cased); captured into the generic `GamProp`/`GamStr` bag (`prop_bag_add`/`str_prop_bag_add`) and read back by name via `gam_prop_f`/`gam_prop_i`/`gam_str` |

### L3 — oracle

`tools/linkage_oracles/CLoadLevel.py` compiles the real `gam_load()` (unmodified)
into a headless dumper (`tools/linkage_oracles/gamload_dump.c`) and runs it over
**all 35 shipped `.gam` files** (`assets/gam/*.gam`, tracked in git — no proprietary
binary needed), diffing against `tools/gam_parser.py::parse_gam`:

- **Object framing** — `(type, ObjectTag)` for every one of the corpus's 3299 object
  instances, in on-disk order. Any record-length or checksum-skip bug anywhere
  upstream of a `LOAD` row would desync this and mismatch immediately, so this
  exercises the generic pstring/type-dispatch/checksum walk far beyond `LOAD` alone.
- **`LOAD` property set** — all 97 real `LOAD` instances across the corpus, all 9
  `docs/decomp/CLoadLevel.md` properties, compared byte-exact (strings, ints) or
  bit-exact (floats — both sides decode the same 4 source bytes to an IEEE-754
  pattern, compared as raw bits, not by lossy formatting).

All values traced above come from real shipped level data or the format constants in
this doc (L4) — the oracle synthesizes nothing.

### Deliberate deviations (native-only; outside the linked aspect)

- **Unknown/raw property types are dropped, not stored.** `gam_parser.py` records
  type ids other than 1/3/6 (e.g. `RotateToDest`, a type-2/4 flag word seen on real
  `LOAD` rows) as hex for inspection; `gam_load` has no consumer for them yet and
  just `fseek`s past the bytes. This is an honest coverage gap (nothing currently
  reads these), not a parse-format deviation — the byte-length skip is still exact,
  which the object-framing check above confirms (a wrong skip length would desync
  every later property/object).
- **`"none"`/empty strings in the generic bag read back as absent, not literal.**
  `str_prop_bag_add` filters authored `"none"`/`""` values out of the bag, so
  `gam_str(e, "RequiredTask", "none")` can't distinguish "authored `none`" from
  "not authored" — both return the harness default. The oracle picks the same
  default on the reference side, so this doesn't affect the diff, but it is a real
  native behavior difference from the raw parse worth flagging for any future
  consumer of `RequiredTask`/similar bagged strings.
- **Comparison-harness defaults for the 2/97 (`ExactLevel`) and 4/97
  (`FadeType`/`FadeTime`) `LOAD` rows that omit a property.** `gam_prop_i`/
  `gam_prop_f` return whatever default the caller passes when a property wasn't
  authored; the oracle picks `-1` (ints) / `0.0` (floats) on both sides purely to
  make the comparison well-defined — this is not a claim about the class's actual
  runtime default for an unauthored `FadeType`/`ExactLevel`.

### Not covered by this aspect (still open)

The proximity/prerequisite gate (`Radius`, `RequiredTask`, `RequiredLevel`,
`ExactLevel` evaluation) and the `ActivateLoad` handoff itself are runtime *behavior*
outside deserialization scope — see the Open Questions above. This aspect certifies
only that the 9 registered properties (and the generic record format they're an
instance of) are read from disk identically to the reference parser.

### Aspect: `activate-load` — status `linked-blocked` (investigated 2026-07-02, updated after Ghidra target 2)

`ActivateLoad` (`00458370`) is decoded — hide the portal (slot `0xd8`), hand
the `+0x17a` request block (`LevelName`/`StartPoint`/fade) to global
game-object slot `0x100`. Target 2 also recovered the missing contact/gate body
at `00457ec0`: Jimmy-only contact, `RequiredTask` lookup via `FUN_0045fea0`,
`RequiredLevel` minimum, optional `ExactLevel`, special `RETURN` path, normal
`LevelName`/`StartPoint` handoff, optional `SoundIndex`, and optional
`FadeType`/`FadeTime`.

The native path (`behavior_load.c` `load_on_trigger` +
`behavior_base.c` gate helpers) is still a functional bridge, not a
transcription: it forwards `LevelName`/`StartPoint` via
`gamestate_request_level_swap`, adds a native fire-once latch, applies its gate
at spawn/update as well as trigger time, has no `RETURN` branch, does not port
the Jimmy handoff slots, and drops the original sound/fade request semantics.
An oracle around the current native path would certify a different design, not
the recovered body. Returns to native-port: port the recovered `00457ec0`/`00458370`
semantics 1:1, then write the oracle.

#### 2026-08-21: the port and its oracle landed; the certificate row has not moved

`behavior_load.c` is now a transcription of the `00457ec0` order — Jimmy-only
contact (satisfied upstream by the engine's trigger dispatch), the
`RequiredTask` gate with **both** the `RequiredLevel` minimum and the optional
`ExactLevel` applied and neither applied when the task is `"none"`, a missing
task falling through rather than blocking, the `RETURN` branch, the
`LevelName == "none"` refusal, then the portal hide and the level handoff. The
gate half is covered by `tools/linkage_oracles/CLoadLevel_gate.py`: 5238
verdicts over all 97 shipped `LOAD` rows at 54 story states, mutation-tested
against `ExactLevel` precedence, a blocking missing task, and a window applied
to an ungated row.

Still not ported, and still why this row is not simply `linked`: the
`SoundIndex`/`FadeType`/`FadeTime` tail (identified but with unrecovered
callees — `FUN_0047d390`/`FUN_0047dc80`/`FUN_00403c10`), the `DAT_004f0588`
game-mode switch, the player slots `0x178`/`0x11c`/`0x2c4`, and `ActivateLoad`'s
own `+0x17a` request-block handoff through global slot `0x100` (native defers
the swap through `gamestate` instead). The `RETURN` departure pair is ported
but its *source* is INFERRED from the handoff call shape; the writer of the
player's `+0x88c`/`+0x8f0` is not recovered.

**The certificate row is deliberately unchanged.** Whether `activate-load` (or
a narrower `contact-gate` aspect) can now go `linked` is the owner's call, not
this session's.
