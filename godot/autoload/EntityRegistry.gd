extends Node

## Autoload singleton — FourCC -> visual resolver (bridge plan §2c).
##
## Phase-0 stub: it only knows which FourCCs are *invisible* markers/triggers
## (mirrored from src/game/entity_visual.c TYPE_TABLE rows flagged invisible).
## Visible entities get a debug placeholder box from LevelLoader so coordinate
## placement can be eyeballed against the C engine. Phase 2 replaces this with a
## generated entity_registry.json + per-FourCC PackedScenes.

## Pure markers / triggers / effects with no mesh in the C resolver.
const INVISIBLE := {
	"3PAT": true, "3AIT": true, "3CAM": true, "3MCA": true, "3SOU": true,
	"3LIO": true, "STRT": true, "TRIG": true, "3SPR": true, "3ANI": true,
	"3LAS": true, "3DAI": true, "3RCK": true, "LOAD": true, "3TSP": true,
	"3FAD": true, "3FOG": true, "3MUS": true, "3HEL": true, "3AIJ": true,
	"3FLY": true, "3GRN": true, "3GRO": true, "3GEM": true, "3TEX": true,
	"3COR": true, "3VOR": true, "3TSU": true, "3SRO": true, "3ELE": true,
	"3BUG": true, "3GWA": true, "3WAT": true, "3ROP": true, "3PEN": true,
	"3TRA": true,
}

## Debug tint per broad category, so the placeholder field is readable in the
## spike (matches main.c entity_color intent loosely).
const DEBUG_COLOR := {
	"3DOR": Color(0.6, 0.3, 0.0),   # doors  - brown
	"3BUT": Color(0.9, 0.9, 0.2),   # buttons- yellow
	"3PIC": Color(1.0, 0.8, 0.2),   # pickups- gold
	"3TRE": Color(0.1, 0.8, 0.2),   # trees  - green
}

func is_invisible(fourcc: String) -> bool:
	return INVISIBLE.has(fourcc)

func debug_color(fourcc: String) -> Color:
	return DEBUG_COLOR.get(fourcc, Color(0.5, 0.5, 0.5))
