#!/usr/bin/env python3
"""Build the vtable linkage audit from the decomp ledger.

This report is intentionally classification-oriented. It does not claim that a
native EntityVTable means the decompiled semantics are linked; the statuses below
separate structural routing from behavioral parity.
"""

from __future__ import annotations

import csv
import re
import subprocess
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from datetime import date
from pathlib import Path


REPO = Path(__file__).resolve().parents[1]
LEDGER = REPO / "docs" / "decomp_ledger.csv"
ENTITIES = REPO / "src" / "game" / "entities.c"
CLASSIDS = REPO / "docs" / "_gam_classids.tsv"
OUT = REPO / "docs" / "vtable_linkage_audit.md"

DOMAINS = [
    "camera / cutscene",
    "player movement",
    "menus / UI flow",
    "inventory / items / gadgets",
    "progression / objectives",
    "triggers / story sequencing",
    "animation / actor pose",
    "AI / pathing",
    "vehicles / special movement",
    "cosmetic / deferred",
]

DOMAIN_HEADINGS = {
    "camera / cutscene": "Camera / Cutscene",
    "player movement": "Player Movement",
    "menus / UI flow": "Menus / UI Flow",
    "inventory / items / gadgets": "Inventory / Items / Gadgets",
    "progression / objectives": "Progression / Objectives",
    "triggers / story sequencing": "Triggers / Story Sequencing",
    "animation / actor pose": "Animation / Actor Pose",
    "AI / pathing": "AI / Pathing",
    "vehicles / special movement": "Vehicles / Special Movement",
    "cosmetic / deferred": "Cosmetic / Deferred",
}

VEHICLE_CLASSES = {
    "C3DVehicle",
    "C3DCar",
    "C3DJeep",
    "C3DRocketShip",
    "C3DFlyingObject",
    "C3DSailBoat",
    "C3DBus",
    "C3DAICar",
    "C3DAISuv",
    "C3DNeuCar",
    "C3DNeuCar2",
}

VALID_STATUSES = {
    "linked",
    "approximated",
    "must-link",
    "defer",
    "unused",
    "wontfix-faithful",
    "linked-blocked",
}


@dataclass(frozen=True)
class Override:
    domain: str
    status: str
    reason: str
    next_action: str
    native: str | None = None


OVERRIDES: dict[str, list[Override]] = {
    "C3DMultiCutSceneCamera": [
        Override(
            "camera / cutscene",
            "must-link",
            "3MCA sequencing owns cutscene shot order, CameraTypeN local offsets, TargetAnimN, audio step timing, and PlayerControlled locks; native carries these fields, and TargetAnimN now dispatches to known non-player actor ASE clips.",
            "Validate sequencer activation/deactivation timing, PlayerControlled restore, and actor animation by-eye on level1b LABEXP3 plus Goddard/Cindy scenes.",
            "src/game/behaviors/behavior_cutscene.c",
        )
    ],
    "C3DCutSceneCamera": [
        Override(
            "camera / cutscene",
            "must-link",
            "3CAM per-frame camera enum is DECODED from Neutron.exe 00415f90 (slot 245) and linked: CameraType==2 static, ViewFromCamera==0 orbit, else dolly, dist=clamp(InitialDist-ZoomSpeed*t,Min,Max). TargetActAnim/LoopActAnim/TargetDeactAnim now dispatch to Jimmy poses or known non-player actor ASE clips. Remaining open: PlayerControlled restore timing and activation message source.",
            "Validate PlayerControlled lock/restore and target animation timing against level1b LABEXP3 plus Goddard/Cindy scenes (by-eye on desktop/noVNC).",
            "src/game/behaviors/behavior_cutscene.c",
        )
    ],
    "C3DPlayer": [
        Override(
            "player movement",
            "must-link",
            "Movement, camera coupling, climb/fence/fall states, and animation-ended callbacks control original feel; native player is explicitly a simple tank-control approximation.",
            "Link UpdatePlayerState, DispatchPlayerModeCamera, walking camera helpers, StopPlayerMotion, and OnPlayerAnimEnded.",
            "src/game/behaviors/behavior_player.c; src/engine/movement_base.c",
        )
    ],
    "C3DJimmy": [
        Override(
            "player movement",
            "must-link",
            "Concrete player owns active-controller, gadget/action locks, level timers, vehicle insertion pose, and runtime handles on top of C3DPlayer.",
            "Repair raw function boundaries for UpdateJimmyFrame/UpdateJimmyActiveController and link input/gadget/vehicle mode dispatch.",
            "src/game/behaviors/behavior_player.c; src/game/player_anim.c",
        ),
        Override(
            "inventory / items / gadgets",
            "must-link",
            "Jimmy dispatches usable tools and action locks; gadget controller pointer semantics are not linked.",
            "Map the 0xa18 gadget controller path and tie baseball/shrink/grapple/tool actions to original slots.",
            "src/game/behaviors/behavior_player.c; src/game/gamestate.c",
        ),
    ],
    "CMainMenu": [
        Override(
            "menus / UI flow",
            "linked-blocked",
            "The level-routing table is linked, but 9a2b908 does not contain the DAT_004f8164 table contents or a complete screen graph; target 4 recovers manager mechanics only.",
            "Recover the executable-backed menu table records, failed rollover helper, activation audio, and level handoff before porting the original layout; do not infer them from the native keyboard list.",
            "src/game/menu.c",
        )
    ],
    "CMenuElement": [
        Override(
            "menus / UI flow",
            "linked-blocked",
            "UpdateItemLogic is recovered, but its DAT_004f8164 canvas owner is absent and the snapshot explicitly leaves hit-state polarity and target-slot semantics unresolved.",
            "Recover the owning canvas records/target protocol and activation-sound caller, then port and oracle the item path as a dependency of CMainMenu.",
            "src/game/menu.c",
        )
    ],
    "C2DInGameMenu": [
        Override(
            "menus / UI flow",
            "linked-blocked",
            "DrawHud constants are decoded, but counter producers, eight owned helper bodies, RestartLevel.tsk, and the death/game-over condition remain unresolved; frame-8881 layout data cannot supply logic from one visual sample.",
            "Recover 00402b40..00407490 producers/conditions and RestartLevel.tsk, then port the four-counter draw/death flow over the existing capture-backed HUD assets.",
            "src/game/hud.c; src/game/game_flow.c",
        )
    ],
    "CGameType": [
        Override(
            "menus / UI flow",
            "linked-blocked",
            "The InitGame camera-record seed is separately linked, but the pause/help/update gate L1 still leaves DAT_005099a9 semantics, the update predicate, and help slots 0x13c/0x140 unresolved.",
            "Recover PauseGame 00475a00/00475a30, the UpdateGameType predicate, and ToggleHelp target slots before wiring menu input locks; capture the help layout rather than approximating it.",
            "src/game/game_flow.c; src/game/menu.c",
        ),
        Override(
            "progression / objectives",
            "linked-blocked",
            "CGameType::InitGame's camera-record seed is linked and oracle-verified, but the broader lifecycle/database/update-target surface is not fully body-backed or ported.",
            "Keep the initgame-camera-record-seed certificate scoped; recover and port UnInitGame/UpdateGameType/database/current-game globals before promoting the controller row.",
            "src/game/game_flow.c",
        ),
    ],
    "CJimmyGame": [
        Override(
            "progression / objectives",
            "must-link",
            "CJimmyGame seeds mission counters and owns task/level lifecycle inherited by every CLevel*Game and CMainMenu.",
            "Link InitGame/update/load-level helpers against CTaskList and CGameType before per-level controller work.",
            "src/game/game_flow.c",
        )
    ],
    "CPickupType": [
        Override(
            "inventory / items / gadgets",
            "must-link",
            "Global pickup state table, RequiredLevel/ExactLevel gates, and visibility state drive collectible persistence.",
            "Name global pickup service slot 0x154 and inherited visibility/collision slots after state-table check.",
            "src/game/behaviors/behavior_item.c; src/game/gamestate.c",
        )
    ],
    "C3DPickupType": [
        Override(
            "inventory / items / gadgets",
            "must-link",
            "Animated/AI pickup base opt-in gates shrink targets and moving pickups through DAT_004f8438.",
            "Identify descendants setting pickup_fields_enabled and link post-load/gating semantics.",
            "src/game/behaviors/behavior_creature.c; src/game/behaviors/behavior_humphrey.c",
        )
    ],
    "C3DPickupItem": [
        Override(
            "inventory / items / gadgets",
            "must-link",
            "3PIC collection owns picture/inventory requirements, pickup state, score, sounds, ActivateObject, ToggleObject, and NextTrigger.",
            "Link HandlePickupCollection, PostLoadPickupItem, CheckRequiredPicAndConsume, and ambient sound handling.",
            "src/game/behaviors/behavior_item.c; src/game/gamestate.c",
        )
    ],
    "C3DShrinkRay": [
        Override(
            "inventory / items / gadgets",
            "must-link",
            "Ray visual is decoded, but the real shrink-on-contact to moving-pickup transition lives in unported hit/contact paths.",
            "Link the contact path that plays HISHRINK, scales targets, and converts C3DEnemy/C3DPickupType descendants into collectable moving pickups.",
            "src/game/behaviors/behavior_creature.c; src/game/behaviors/behavior_humphrey.c",
        )
    ],
    "C3DGraplingHook": [
        Override(
            "inventory / items / gadgets",
            "must-link",
            "Rope/hook visual is decoded but has no current .gam placements; gameplay owner/spawn path is unresolved.",
            "Find the owner that spawns/controls 3GRA and link grapple targeting/action-state semantics.",
            "src/game/behaviors/behavior_player.c",
        )
    ],
    "C3DToolChest": [
        Override(
            "inventory / items / gadgets",
            "must-link",
            "Tool chest binds open/no-tools/closed states; native prop path treats it as static gated dressing.",
            "Link open/notools/closed state transitions and tool availability effects before marking tool UI faithful.",
            "src/game/behaviors/behavior_prop.c",
        )
    ],
    "CTaskList": [
        Override(
            "progression / objectives",
            "must-link",
            ".tsk format is measured for NewGame, but constructor/streamer slot and RestartLevel layout are still open.",
            "Pin the streamer parser, recover RestartLevel.tsk, and link task-state reads/writes to level/load/restart flows.",
            "src/game/task_loader.c; src/game/game_flow.c",
        )
    ],
    "CLoadLevel": [
        Override(
            "progression / objectives",
            "must-link",
            "LOAD portals own in-world level transitions; ActivateLoad is decoded but request-block and inherited proximity/gate caller are open.",
            "Decode request block, RequiredTask/RequiredLevel gate, fade values, and global loader slot 0x100 handoff.",
            "src/game/behaviors/behavior_load.c; src/game/game_flow.c",
        )
    ],
    "C3DStartPoint": [
        Override(
            "progression / objectives",
            "must-link",
            "Start points place the player, camera viewport, music, and StartTrigger on load/respawn.",
            "Link PlacePlayer apply order, music selection, and StartTrigger firing against CTaskList/C3DPlayer.StartPoint.",
            "src/game/game_flow.c; src/game/behaviors/behavior_load.c",
        )
    ],
    "C3DCheckPoint": [
        Override(
            "progression / objectives",
            "must-link",
            "Race checkpoint/finish-line logic affects timed level completion and HUD timing.",
            "Link non-finish checkpoint progress, race timer, and FUN_004073b0 finish state machine.",
            "src/game/behaviors/behavior_checkpoint.c",
        )
    ],
    "CTrigger": [
        Override(
            "triggers / story sequencing",
            "must-link",
            "Proximity primitive is decoded, but enter/exit slot bindings and watched-target registration callers are not mapped.",
            "Map slot 0x54/0x58 concrete actions and the registrar that populates watched targets.",
            "src/game/behaviors/behavior_trig.c",
        )
    ],
    "C3DTriggerType": [
        Override(
            "triggers / story sequencing",
            "must-link",
            "Common ToggleObject/NextTrigger/Fade fields drive trigger chains and camera target records; dispatcher ownership remains open.",
            "Name NextTrigger resolver/global camera fields and link Toggle/Fade consumers.",
            "src/game/behaviors/behavior_base.c; src/game/behaviors/behavior_ai_trigger.c",
        )
    ],
    "C3DAITrigger": [
        Override(
            "triggers / story sequencing",
            "must-link",
            "Native ports a conservative subset and SCENE writes; ActivateObject0..4, AIState/AISpeed, AIAnim, rewards, and menu/story side effects are deferred.",
            "Link full ActivateAITrigger graph after CTaskList/menu/HUD side effects are named.",
            "src/game/behaviors/behavior_ai_trigger.c",
        )
    ],
    "C3DMusicTrigger": [
        Override(
            "triggers / story sequencing",
            "must-link",
            "Music trigger dispatch affects cutscenes, level ambiance, and remaining audio QA issues.",
            "Link trigger activation, music handle lifetime, and overlap/stop semantics.",
            "src/game/behaviors/behavior_music.c",
        )
    ],
    "C3DSoundEffect": [
        Override(
            "triggers / story sequencing",
            "must-link",
            "Sound emitter semantics are central to cutscene/audio parity; some current audio reports remain open.",
            "Link enter/exit/start/stop/lifetime semantics and validate by ear on desktop/noVNC.",
            "src/game/behaviors/behavior_soundfx.c",
        )
    ],
    "C3DAnimated": [
        Override(
            "animation / actor pose",
            "must-link",
            "Base animation loader, level gates, PickupLink, texture slots, and animation-ended hook feed actors, cutscenes, and player state.",
            "Name inherited animation completion hook and adjusted OMedia shape/material slots before actor-pose linkage.",
            "src/game/entity_visual.c; src/game/player_anim.c",
        )
    ],
    "C3DFriends": [
        Override(
            "animation / actor pose",
            "must-link",
            "Friend talk/pulse/reward helpers drive NPC pose and story-state transitions.",
            "Link friend state helpers and talk progress reward slots with CTaskList writes.",
            "src/game/behaviors/behavior_friend.c",
        )
    ],
    "C3DGoddard": [
        Override(
            "animation / actor pose",
            "must-link",
            "Companion controller is partially ported; Goddard texture/mapping remains an open visual fidelity issue.",
            "Track material/UV mismatch separately while linking companion mode/pose transitions from decomp.",
            "src/game/behaviors/behavior_goddard.c; src/game/entity_visual.c",
        )
    ],
    "C3DCarl": [
        Override(
            "animation / actor pose",
            "must-link",
            "Carl has friend/action animation states and likely vehicle-specific insertion/pose requirements; the native walker path does not prove those visual states.",
            "Link Carl's action/vehicle pose routing, especially any vehicle insertion offsets and movement animations shared with C3DVehicle scenes.",
            "src/game/behaviors/behavior_walker.c; src/game/entity_visual.c",
        )
    ],
    "C3DCindy": [
        Override(
            "AI / pathing",
            "defer",
            "Cindy location/pathing is explicitly incomplete; do not fix without original-game evidence or decomp proof.",
            "Collect XP/capture comparison for Level3D 3CIN row and c1/c2 patrol markers before changing behavior.",
            "src/game/behaviors/behavior_cindy.c",
        )
    ],
    "C3DAI": [
        Override(
            "AI / pathing",
            "must-link",
            "Base AI state machine, patrol/facing helpers, FOV/range, attacks, and state transitions cannot be guessed without feel drift.",
            "Create Ghidra functions for raw helper cluster 00408bc0..0040a6d0 and link state enum semantics.",
            "src/game/behaviors/behavior_ai.c",
        )
    ],
    "C3DPatrolPoint": [
        Override(
            "AI / pathing",
            "must-link",
            "742 placed waypoint nodes own wait anim, sound, CallObjectTag, ActivateAnim, and NextPatrolPoint routing.",
            "Link OnArrive dispatch order and how C3DAI consumes wait/sound/call/next fields.",
            "src/game/behaviors/behavior_walker.c; src/game/behaviors/behavior_ai.c",
        )
    ],
    "C3DVehicle": [
        Override(
            "vehicles / special movement",
            "must-link",
            "Vehicle base owns steering, wheel solver, speed integrator, vehicle camera, input predicates, and rider insertion/pose fidelity for Jimmy/Carl.",
            "Define raw vehicle helper functions and link camera/drive-state/wheel transform/rider-insertion clusters before tuning vehicles.",
            "src/game/behaviors/behavior_vehicle.c",
        )
    ],
    "C3DFlyingObject": [
        Override(
            "vehicles / special movement",
            "must-link",
            "Flying movement base affects rocket/special movement and vehicle-camera feel.",
            "Link flight integrator and camera coupling before revisiting rideable rocket feel.",
            "src/game/behaviors/behavior_flying.c; src/game/behaviors/behavior_vehicle.c",
        )
    ],
    "C3DRocketShip": [
        Override(
            "vehicles / special movement",
            "must-link",
            "Rideable rocket is a player-controlled movement mode, not the autonomous C3DRocket AI.",
            "Link C3DRocketShip/FlyingObject entry, exit, steering, camera, and audio loop behavior.",
            "src/game/behaviors/behavior_vehicle.c; src/game/behaviors/behavior_flying.c",
        )
    ],
    "C3DSailBoat": [
        Override(
            "vehicles / special movement",
            "approximated",
            "Native sailboat anchoring was fixed for level1 QA, but base water/AI boat semantics are not fully linked.",
            "Keep current faithfulness result; link only if boat motion/camera/audio becomes a blocker.",
            "src/game/behaviors/behavior_vehicle.c",
        )
    ],
}


TOP25 = [
    ("1", "camera / cutscene", "C3DMultiCutSceneCamera sequencer", "3MCA CameraTypeN/TargetAnimN/SoundIndexN/PlayerControlled", "Partial native runtime; CameraType table, field plumbing, selector enumeration, and non-player TargetAnimN visual dispatch are linked, but timing semantics remain open.", "Validate activation/deactivation timing, PlayerControlled restore, and actor animation by-eye."),
    ("2", "camera / cutscene", "C3DCutSceneCamera::ViewFromCamera", "3CAM ViewFromCamera, CameraType, offsets, zoom/distance", "Standalone 3CAM enum behavior is decoded and linked from Neutron.exe 00415f90.", "Validate remaining activation source and restore timing before scene-specific tuning."),
    ("3", "camera / cutscene", "C3DCutSceneCamera target/facing slots", "TargetActAnim, LoopActAnim, TargetDeactAnim, FaceObject", "FaceObject and target animation dispatch are plumbed for Jimmy plus known non-player actor ASE clips; exact animation-ended callbacks remain open.", "Validate deactivate cleanup semantics and unknown actor aliases."),
    ("4", "player movement", "C3DPlayer::UpdatePlayerState", "00437940", "Native player is simple tank-control approximation.", "Port main update gate and mode/animation dispatch."),
    ("5", "player movement", "C3DPlayer::DispatchPlayerModeCamera", "00438a60", "Manual follow camera currently stands in.", "Link player_mode camera dispatch and camera bump timing."),
    ("6", "player movement", "C3DPlayer::UpdateWalkingCameraA", "00438bc0", "Large movement/camera helper unlinked.", "Port walk-speed, edge/vertical state, and DAT_00509a50 updates."),
    ("7", "player movement", "C3DPlayer::UpdateWalkingCameraB", "00439900", "Companion helper unlinked.", "Port acceleration/deceleration, turn latches, and probe updates."),
    ("8", "player movement", "C3DPlayer::UpdateSittingOrSmoothCamera", "0043a120", "Smooth/sitting camera path unlinked.", "Port smoothing offsets and turn-input camera behavior."),
    ("9", "player movement", "C3DPlayer motion/animation callbacks", "0043a420, 0043a5d0, 0043a750, 0043a900", "Rotate/project/stop/AnimEnded cluster is not linked.", "Port rotate-to-target, noisy camera target, stop, and fence/ladder completion."),
    ("10", "player movement", "C3DJimmy active controller", "00424600, 00425870, 00426030", "Raw Jimmy action/gadget/input/vehicle-mode cluster only partially understood.", "Repair function boundaries and link active-player/gadget locks plus vehicle insertion poses."),
    ("11", "menus / UI flow", "CMainMenu menu manager", "LoadMyMenu/displayMenu/Activating Item traces", "Routing is linked; the screen graph is linked-blocked because DAT_004f8164 contents and rollover/audio bodies are absent.", "Recover the executable-backed table records and failed helper before porting layout."),
    ("12", "menus / UI flow", "CMenuElement::UpdateItemLogic", "0045e650", "Structurally recovered but linked-blocked on the missing canvas owner, conservative hit polarity, target semantics, and sound caller.", "Recover the CMainMenu owner/target protocol before a runtime port/oracle."),
    ("13", "menus / UI flow", "C2DInGameMenu HUD/death path", "00406690 plus 00402b40..00407490 helpers", "Draw constants are known; counter producers, death predicate, eight helper bodies, and RestartLevel.tsk are linked-blocked.", "Recover the producers/helpers/task file before replacing the current capture-backed HUD."),
    ("14", "menus / UI flow", "CGameType pause/help/update gates", "00475a00..00475ce0", "InitGame's camera seed is linked; pause/help/update remains linked-blocked on unresolved globals, predicate, and target slots.", "Recover PauseGame, the update predicate, and help target slots before wiring input locks."),
    ("15", "inventory / items / gadgets", "CPickupType state table", "0045f1f0", "PickupIndex visibility/state table identified.", "Name global pickup service and inherited state toggles."),
    ("16", "inventory / items / gadgets", "C3DPickupType animated pickup gate", "00436b10, 00436b80", "Opt-in AI pickup table gates shrink targets.", "Find descendants enabling pickup fields and link moving-pickup state."),
    ("17", "inventory / items / gadgets", "C3DPickupItem::HandlePickupCollection", "00435ce0, 00436200, 00436830", "Collection path is only approximated in native.", "Port required-picture consume, score, sound, object activation, NextTrigger."),
    ("18", "inventory / items / gadgets", "Gadget/tool state cluster", "C3DShrinkRay, C3DGraplingHook, C3DToolChest", "Visual setup known; gameplay ownership/action states incomplete.", "Link shrink contact, grapple owner, and tool chest open/no-tools states."),
    ("19", "progression / objectives", "CTaskList streamer/parser", "CTaskList + NewGame.tsk/RestartLevel.tsk", "NewGame format measured; RestartLevel and streamer slot open.", "Recover RestartLevel and connect task store to level/restart flows."),
    ("20", "progression / objectives", "CLoadLevel/C3DStartPoint/C3DCheckPoint", "00458370, 00442740, 00414410", "Transition/spawn/checkpoint pieces decoded but not fully linked.", "Link request blocks, StartTrigger/music, checkpoint race progress."),
    ("21", "progression / objectives", "CGameType/CJimmyGame campaign lifecycle", "InitGame/UpdateGameType/CLevel*Game inheritance", "CGameType's camera-record seed and CJimmyGame mission seed are linked; the broader lifecycle remains incomplete.", "Recover and port UpdateGameType/current-game/database lifecycle before broad campaign promotion."),
    ("22", "triggers / story sequencing", "CTrigger/C3DTriggerType core graph", "0047dfa0, 00447a70", "Proximity and NextTrigger decoded; concrete action binding open.", "Map enter/exit slots, watched-target registration, Toggle/Fade consumers."),
    ("23", "triggers / story sequencing", "C3DAITrigger/music/sound activation", "C3DAITrigger, C3DMusicTrigger, C3DSoundEffect", "SCENE subset linked; audio/ActivateObject/AIAnim graph incomplete.", "Link full activation graph after task/menu/HUD side effects are named."),
    ("24", "animation / actor pose", "C3DAnimated and actor pose callbacks", "0040e050, 0040dd90, animation-ended hooks", "Base loader/gates known; pose callbacks and material slots need names.", "Link animation completion, actor facing, talk/idle/action transitions; keep Goddard texture issue tracked."),
    ("25", "AI / pathing; vehicles / special movement", "C3DAI/C3DPatrolPoint/C3DVehicle/C3DCarl feel clusters", "00408bc0..0040a6d0, 00434ea0, 00465220..00467f70", "AI pathing, Carl action poses, vehicle integrators, and rider insertion are raw/approximated.", "Link AI state helpers, patrol arrival, steering/wheel/camera/rider-insertion helpers; defer Cindy until evidence."),
]


def read_ledger() -> list[dict[str, str]]:
    with LEDGER.open(newline="") as f:
        return list(csv.DictReader(f))


def _require(parsed, what: str, src) -> None:
    """Refuse to build a report out of a source that came back empty.

    This report is where the project's own coverage numbers come from, so a
    parse that silently reads nothing does not produce a small report -- it
    produces a confident one that says zero. The same shape cost
    build_asset_catalog.py a committed checklist of seven phantom FourCCs when
    a generated struct grew a field its regex did not expect.
    """
    if not parsed:
        raise SystemExit(
            "build_vtable_parity_report: parsed 0 %s from %s -- the format "
            "changed and this parser did not." % (what, src))


def parse_entities() -> tuple[dict[str, tuple[str, str]], dict[str, list[str]]]:
    fourcc_to_vt: dict[str, tuple[str, str]] = {}
    vt_to_fourcc: dict[str, list[str]] = defaultdict(list)
    row_re = re.compile(r'\{\s*"([0-9A-Za-z]{3,4})"\s*,\s*"([^"]*)"\s*,\s*&(\w+)\s*\}')
    for line in ENTITIES.read_text().splitlines():
        m = row_re.search(line)
        if not m:
            continue
        fourcc, desc, vt = m.groups()
        fourcc_to_vt[fourcc] = (vt, desc)
        vt_to_fourcc[vt].append(fourcc)
    _require(fourcc_to_vt, "entity_types[] rows", ENTITIES)
    return fourcc_to_vt, vt_to_fourcc


def parse_fourcc_to_class() -> dict[str, str]:
    out: dict[str, str] = {}
    for line in CLASSIDS.read_text().splitlines():
        fields = line.split()
        if len(fields) < 5 or fields[0] == "imm_fourcc":
            continue
        fourcc = fields[1]
        cls = re.sub(r"\(.*$", "", fields[4]).strip()
        if fourcc and cls and cls != "??":
            out.setdefault(fourcc, cls)
    _require(out, "FourCC-to-class rows", CLASSIDS)
    return out


def parse_vtable_sources() -> dict[str, str]:
    out: dict[str, str] = {}
    for path in (REPO / "src" / "game" / "behaviors").glob("*.c"):
        rel = path.relative_to(REPO).as_posix()
        text = path.read_text(errors="replace")
        for m in re.finditer(r"const\s+EntityVTable\s+(vt_\w+)\s*=", text):
            out[m.group(1)] = rel
        for m in re.finditer(r"DEF_PICKUP_VT\((vt_\w+),", text):
            out[m.group(1)] = rel
    _require(out, "EntityVTable definitions", REPO / "src" / "game" / "behaviors")
    return out


def class_native_entries() -> dict[str, str]:
    fourcc_to_vt, _ = parse_entities()
    fourcc_to_class = parse_fourcc_to_class()
    vt_sources = parse_vtable_sources()
    by_class: dict[str, list[str]] = defaultdict(list)
    for fourcc, cls in fourcc_to_class.items():
        vt_desc = fourcc_to_vt.get(fourcc)
        if not vt_desc:
            continue
        vt, _desc = vt_desc
        src = vt_sources.get(vt, "src/game/entities.c")
        by_class[cls].append(f"{fourcc}->{vt} ({src})")
    return {cls: "; ".join(vals) for cls, vals in by_class.items()}


def doc_link(cls: str) -> str:
    path = REPO / "docs" / "decomp" / f"{cls}.md"
    if path.exists():
        return f"[{cls}.md](./decomp/{cls}.md)"
    return ""


def md_escape(s: str) -> str:
    return (s or "").replace("|", "\\|").replace("\n", " ")


def default_domain(row: dict[str, str]) -> str:
    cls = row["class"]
    family = row.get("family", "")
    base = row.get("base_chain", "")
    notes = row.get("notes", "")

    if any(x in cls for x in ("CutSceneCamera", "Camera", "ViewPort")):
        return "camera / cutscene"
    if cls in {"C3DPlayer", "C3DJimmy"}:
        return "player movement"
    if "Menu" in cls:
        return "menus / UI flow"
    if any(x in cls for x in ("Pickup", "Neutron", "Helmet", "ToolChest", "ShrinkRay", "GraplingHook", "Baseball")):
        return "inventory / items / gadgets"
    if cls == "CGameType" or cls == "CJimmyGame" or cls == "CTaskList" or cls.startswith("CLevel") or cls in {"CLoadLevel", "C3DStartPoint", "C3DCheckPoint"}:
        return "progression / objectives"
    if "Trigger" in cls or "SoundEffect" in cls or "Music" in cls:
        return "triggers / story sequencing"
    if cls in VEHICLE_CLASSES or "vehicles" in family:
        return "vehicles / special movement"
    if "C3DAI" in cls or "PatrolPoint" in cls or "C3DEnemy" in base or "enemies_ai" in family:
        return "AI / pathing"
    if "C3DAnimated" in base or any(x in cls for x in ("Friends", "Goddard", "Carl", "Cindy", "Yokian", "Benny", "Libby", "Sheen", "Nick")):
        return "animation / actor pose"
    if "no current gam rows" in notes.lower():
        return "cosmetic / deferred"
    return "cosmetic / deferred"


def default_status_reason_next(row: dict[str, str], domain: str) -> tuple[str, str, str]:
    cls = row["class"]
    family = row.get("family", "")
    notes = row.get("notes", "")
    n_methods = int(row.get("n_methods") or 0)
    notes_l = notes.lower()

    if "no current gam rows" in notes_l or "non-gam" in notes_l:
        return (
            "unused",
            "No current shipped .gam content reaches this class directly.",
            "Keep documented; revisit only if runtime-spawn evidence or new content reaches it.",
        )
    if n_methods == 0:
        return (
            "defer",
            "No owned vtable methods; behavior is inherited and better audited through the owning base class.",
            "Do not port separately unless a per-class constructor/identity mismatch appears.",
        )
    if "destructor only" in notes_l or "per-class destructor only" in notes_l:
        return (
            "defer",
            "Destructor-only or inherited-shell class; no class-owned runtime parity surface identified.",
            "Leave deferred; link the shared base/controller instead.",
        )
    if domain in {
        "camera / cutscene",
        "player movement",
        "menus / UI flow",
        "inventory / items / gadgets",
        "progression / objectives",
        "triggers / story sequencing",
        "animation / actor pose",
        "AI / pathing",
        "vehicles / special movement",
    }:
        return (
            "approximated",
            "Native routing exists or the decomp spec is documented, but this row is not yet proven linked to the original method body.",
            "Promote to must-link only if it affects current feel/progression or if a targeted mismatch points here.",
        )
    if family in {"world_props_terrain", "creatures_one_off_set_dressing"}:
        return (
            "defer",
            "Low-visible set dressing/static prop behavior; current priority is core feel systems.",
            "Keep as deferred unless capture/decomp shows visible gameplay side effects.",
        )
    return (
        "defer",
        "Not in the first parity wave; either cosmetic, low-visible, or dependent on a higher-priority base class.",
        "Reclassify after camera/player/menu/inventory/progression/trigger bases are linked.",
    )


def build_rows(ledger: list[dict[str, str]], native_by_class: dict[str, str]) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    ledger_by_class = {r["class"]: r for r in ledger}

    for row in ledger:
        cls = row["class"]
        overrides = OVERRIDES.get(cls)
        if overrides:
            for o in overrides:
                assert o.domain in DOMAINS
                assert o.status in VALID_STATUSES
                rows.append({
                    "domain": o.domain,
                    "class": cls,
                    "doc": doc_link(cls),
                    "vtable": row.get("vftable", ""),
                    "methods": row.get("n_methods", ""),
                    "status": o.status,
                    "reason": o.reason,
                    "native": o.native or native_by_class.get(cls, ""),
                    "next": o.next_action,
                })
            continue

        domain = default_domain(row)
        status, reason, next_action = default_status_reason_next(row, domain)
        rows.append({
            "domain": domain,
            "class": cls,
            "doc": doc_link(cls),
            "vtable": row.get("vftable", ""),
            "methods": row.get("n_methods", ""),
            "status": status,
            "reason": reason,
            "native": native_by_class.get(cls, ""),
            "next": next_action,
        })

    # Ensure handoff-critical classes are present even if a future ledger changes.
    for cls, overrides in OVERRIDES.items():
        if cls in ledger_by_class:
            continue
        for o in overrides:
            rows.append({
                "domain": o.domain,
                "class": cls,
                "doc": doc_link(cls),
                "vtable": "",
                "methods": "",
                "status": o.status,
                "reason": o.reason,
                "native": o.native or native_by_class.get(cls, ""),
                "next": o.next_action,
            })

    order = {d: i for i, d in enumerate(DOMAINS)}
    status_order = {"must-link": 0, "linked-blocked": 1, "approximated": 2,
                    "linked": 3, "wontfix-faithful": 4, "defer": 5, "unused": 6}
    rows.sort(key=lambda r: (order.get(r["domain"], 99), status_order.get(r["status"], 99), r["class"]))
    return rows


def status_summary(rows: list[dict[str, str]]) -> str:
    by_status = Counter(r["status"] for r in rows)
    by_domain = defaultdict(Counter)
    for r in rows:
        by_domain[r["domain"]][r["status"]] += 1

    lines = ["| Status | Count |", "|---|---:|"]
    for status in ["must-link", "linked-blocked", "approximated", "linked",
                   "wontfix-faithful", "defer", "unused"]:
        lines.append(f"| `{status}` | {by_status.get(status, 0)} |")
    lines.append("")
    lines.extend(["| Parity domain | must-link | linked-blocked | approximated | linked | wontfix-faithful | defer | unused |", "|---|---:|---:|---:|---:|---:|---:|---:|"])
    for domain in DOMAINS:
        c = by_domain[domain]
        lines.append(
            f"| {domain} | {c.get('must-link', 0)} | {c.get('linked-blocked', 0)} | {c.get('approximated', 0)} | "
            f"{c.get('linked', 0)} | {c.get('wontfix-faithful', 0)} | {c.get('defer', 0)} | {c.get('unused', 0)} |"
        )
    return "\n".join(lines)


def top25_markdown() -> str:
    lines = [
        "| # | Parity domain | Function/class | Evidence | Current native state | Next action |",
        "|---:|---|---|---|---|---|",
    ]
    for num, domain, item, evidence, state, action in TOP25:
        lines.append(
            f"| {num} | {md_escape(domain)} | {md_escape(item)} | {md_escape(evidence)} | "
            f"{md_escape(state)} | {md_escape(action)} |"
        )
    return "\n".join(lines)


def rows_markdown(rows: list[dict[str, str]]) -> str:
    lines: list[str] = []
    for domain in DOMAINS:
        domain_rows = [r for r in rows if r["domain"] == domain]
        if not domain_rows:
            continue
        lines.append(f"## {DOMAIN_HEADINGS[domain]}")
        lines.append("")
        lines.append("| Parity domain | Class | Decomp doc | Vtable address(es) | Owned method count | Current status | Reason | Native entry point, if known | Next action |")
        lines.append("|---|---|---|---|---:|---|---|---|---|")
        for r in domain_rows:
            lines.append(
                f"| {md_escape(r['domain'])} | `{md_escape(r['class'])}` | {r['doc']} | "
                f"`{md_escape(r['vtable'])}` | {md_escape(r['methods'])} | `{r['status']}` | "
                f"{md_escape(r['reason'])} | {md_escape(r['native'])} | {md_escape(r['next'])} |"
            )
        lines.append("")
    return "\n".join(lines).rstrip()


def write_report(rows: list[dict[str, str]]) -> None:
    text = f"""# Vtable Linkage Audit

Generated {date.today().isoformat()} by `tools/build_vtable_parity_report.py`.

Inputs:

- `docs/decomp_ledger.csv`
- `docs/decomp/*.md`
- `src/game/entities.c`
- `src/game/behaviors/*.c`
- `tools/verify_behaviors.py`

This audit starts the vtable parity campaign. It intentionally does **not** treat
the 93/93 native FourCC routing milestone as semantic parity. A row marked
`approximated` may have a native behavior and still need decomp linkage before it
can be called faithful.

Status meanings:

| Status | Meaning |
|---|---|
| `linked` | Native behavior directly follows the recovered decomp behavior. |
| `linked-blocked` | A scoped linkage investigation found missing evidence or an unported dependency; the blocker is recorded in the certificate manifest. |
| `approximated` | Native engine has a manual behavior that works but is not proven faithful. |
| `must-link` | Function/class affects core visual, gameplay, menu, inventory, progression, trigger, AI, animation, or vehicle feel and should be decoded/ported. |
| `defer` | Low-visible, constructor-only, destructor-only, inherited-shell, cosmetic, or blocked on stronger evidence. |
| `unused` | No current shipped `.gam` content reaches it directly. |
| `wontfix-faithful` | Native behavior differs from expectation but matches original-game evidence. |

Important carry-forward constraints:

- Menus are first-class: front-end routing, rollover/activation, pause/help, HUD,
  death/restart, menu audio cues, input locks, and VR/level routing are not
  cosmetic.
- Inventory/items/gadgets and progression are first-class: pickup state tables,
  tool availability, shrink/grapple/tool chest state, tasks, level loads, start
  points, checkpoints, and campaign lifecycle are parity systems.
- Cindy location/pathing remains incomplete and deferred until original-game
  evidence or decomp proof confirms the correct state. This audit deliberately
  does not fix it.
- Goddard texture/mapping remains an open visual fidelity issue and is tracked in
  the animation/actor-pose domain; this audit does not solve it.

## Summary

{status_summary(rows)}

## Top 25 Priority Functions/Classes

{top25_markdown()}

## Domain Audit

{rows_markdown(rows)}
"""
    OUT.write_text(text)


def main() -> int:
    ledger = read_ledger()
    rows = build_rows(ledger, class_native_entries())
    write_report(rows)
    print(f"wrote {OUT.relative_to(REPO)}")
    print(f"rows: {len(rows)}")
    counts = Counter(r["status"] for r in rows)
    print("status counts: " + ", ".join(f"{k}={counts[k]}" for k in sorted(counts)))
    # Linked-parity gate: a `linked` claim is only trusted with a green oracle.
    subprocess.run([sys.executable, str(REPO / "tools" / "check_linkage_certificates.py")], check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
