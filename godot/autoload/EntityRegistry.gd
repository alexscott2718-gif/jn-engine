extends Node

## Autoload singleton — FourCC(+tag) -> visual record (bridge plan §2c).
## Consumes godot/entity_registry.json, generated from the C resolver tables by
## tools/export_godot_registry.py (stays in lockstep with src/game/entity_visual.c).
##
## Record kinds: "invisible" | "sprite" (png,size,tint?) | "glb" (glb,scale) |
## "placeholder" (ase,scale -> small box until the ASE->glb converter lands).

const REGISTRY_PATH := "res://entity_registry.json"

var _tag: Dictionary = {}
var _type: Dictionary = {}

func _ready() -> void:
	if not FileAccess.file_exists(REGISTRY_PATH):
		push_error("EntityRegistry: missing %s (run tools/export_godot_registry.py)" % REGISTRY_PATH)
		return
	var f := FileAccess.open(REGISTRY_PATH, FileAccess.READ)
	var d = JSON.parse_string(f.get_as_text())
	if d is Dictionary:
		_tag = d.get("tag", {})
		_type = d.get("type", {})

## Resolve an entity to a visual record. Tag override wins, then FourCC default.
func resolve(fourcc: String, tag: String) -> Dictionary:
	var key := "%s|%s" % [fourcc, tag.to_lower()]
	if _tag.has(key):
		return _tag[key]
	if _type.has(fourcc):
		return _type[fourcc]
	return {"kind": "none"}
