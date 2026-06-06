extends Node

## Autoload singleton — game-wide state (items, tools, checkpoint, pending level
## swap). Phase-0 stub; grows in Phase 3 (gameplay systems) to mirror the C
## engine's gamestate.c. Lives in Godot only — never touches the foundry.

var checkpoint: Vector3 = Vector3.ZERO
var has_checkpoint: bool = false
var items: Dictionary = {}            ## item_id -> count
var pending_swap: Dictionary = {}     ## {"level": String, "start_point": String}

func set_checkpoint(world_pos: Vector3) -> void:
	checkpoint = world_pos
	has_checkpoint = true

func collect(item_id: String, n: int = 1) -> void:
	items[item_id] = items.get(item_id, 0) + n

func request_swap(level_name: String, start_point: String) -> void:
	# A LOAD trigger fired. LevelLoader picks this up (Phase 3).
	pending_swap = {"level": level_name, "start_point": start_point}
