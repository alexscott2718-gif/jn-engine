class_name CoordSpace
extends RefCounted

## THE locked native-GL -> Godot transform rule (bridge plan §4).
##
## The exporter (tools/export_godot_level.py) emits every position in the C
## engine's *native-GL* world space (Y-up, -Z forward): placements baked to
## (x, 0, -z), entities (x, y, z) as-is. GL and Godot share the same handedness
## (right-handed, Y-up, -Z forward), so the mapping SHOULD be close to identity
## plus a world scale -- but glTF import basis and winding can still introduce a
## single axis flip. That is exactly what the Phase-0 spike exists to pin down.
##
## FROZEN 2026-06-05 (Phase-0 handedness lock). Verified against the native
## engine top-down (build/track0_level1_topdown_labeled.png, header: "right=+X
## world, up=-Z world"): rendered-house labels sort 6/6 monotonic on the up axis
## (up = -render_z) and 5/6 on X (lone outlier = a label-read miss, not a flip).
## No axis flips; WORLD_SCALE=0.01 makes native units (~thousands) sane meters.
## The native-GL world maps to Godot as a pure positive scale, identity on axes.

const WORLD_SCALE := 0.01   ## native units (~thousands) -> sane Godot meters
const FLIP_X := false       ## FROZEN — see header
const FLIP_Y := false       ## FROZEN — see header
const FLIP_Z := false       ## FROZEN — see header

## Map a native-GL position to a Godot world position.
static func to_godot(p: Vector3) -> Vector3:
	var x := -p.x if FLIP_X else p.x
	var y := -p.y if FLIP_Y else p.y
	var z := -p.z if FLIP_Z else p.z
	return Vector3(x, y, z) * WORLD_SCALE

## Scale a native scalar (speed, distance, jump rate) into Godot units.
static func scalar(v: float) -> float:
	return v * WORLD_SCALE

## Parse a JSON [x, y, z] array into a native-GL Vector3.
static func vec_from(a) -> Vector3:
	if a is Array and a.size() >= 3:
		return Vector3(float(a[0]), float(a[1]), float(a[2]))
	return Vector3.ZERO
