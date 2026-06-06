class_name LevelLoader
extends Node3D

## Builds a level scene at runtime from a foundry manifest (bridge plan §2a +
## the LevelLoader row of §3). Static OMT placements load as MeshInstances via
## GLTFDocument (runtime load -> no .import churn for 193 procedurally-listed
## glbs); gam entities become debug placeholders for now (Phase 2 swaps in real
## scenes). All positions pass through CoordSpace (§4).

@export var level_path: String = "res://levels/level1.json"
@export var build_placements: bool = true
@export var build_entities: bool = true
@export var build_collision: bool = true   ## trimesh static bodies on placements
@export var build_safety_floor: bool = true

var manifest: Dictionary = {}
var spawn_pos: Vector3 = Vector3.ZERO          ## Godot-space spawn
var spawn_yaw_deg: float = 0.0
var physics_path: String = "res://physics/level1.json"

var _placements_root: Node3D
var _entities_root: Node3D

func load_level() -> bool:
	manifest = _read_json(level_path)
	if manifest.is_empty():
		push_error("LevelLoader: could not read %s" % level_path)
		return false

	if manifest.has("physics"):
		physics_path = "res://" + str(manifest["physics"])

	_compute_spawn()

	if build_placements:
		_placements_root = Node3D.new()
		_placements_root.name = "Placements"
		add_child(_placements_root)
		_build_placements()

	if build_safety_floor:
		_build_safety_floor()

	if build_entities:
		_entities_root = Node3D.new()
		_entities_root.name = "EntityMarkers"
		add_child(_entities_root)
		_build_entities()

	print("[LevelLoader] %s: %d placements, %d entity markers, spawn=%s" % [
		manifest.get("name", "?"),
		_placements_root.get_child_count() if _placements_root else 0,
		_entities_root.get_child_count() if _entities_root else 0,
		spawn_pos])
	return true

func _compute_spawn() -> void:
	var sp = manifest.get("spawn")
	if sp == null:
		return
	spawn_pos = CoordSpace.to_godot(CoordSpace.vec_from(sp.get("pos")))
	var rot = sp.get("rot")
	if rot is Array and rot.size() >= 3:
		spawn_yaw_deg = float(rot[1])

func _build_placements() -> void:
	var ok := 0
	var fail := 0
	for pl in manifest.get("placements", []):
		var node := _load_glb(_foundry_abs(str(pl.get("glb", ""))))
		if node == null:
			fail += 1
			continue
		var pname := str(pl.get("name", "placement"))
		node.name = pname
		# Position is the scaled mesh center; the glb geometry itself is in raw
		# native units (localized to its center), so shrink it by the same
		# WORLD_SCALE to keep geometry and placement spacing coherent (§4).
		node.position = CoordSpace.to_godot(CoordSpace.vec_from(pl.get("pos")))
		node.scale = Vector3.ONE * CoordSpace.WORLD_SCALE
		_placements_root.add_child(node)
		# BLOCK* are the original's invisible collision hulls (walls, fences, the
		# BLOCKTop lid) -- collide but never render.
		var is_block := pname.begins_with("BLOCK")
		for mi in _mesh_instances(node):
			if build_collision:
				mi.create_trimesh_collision()
			if is_block:
				mi.visible = false
		ok += 1
	if fail > 0:
		push_warning("[LevelLoader] %d placements failed to load (%d ok)" % [fail, ok])

## A low infinite-plane backstop so the player can't fall through gaps in the
## OMT ground meshes (mirrors the C engine's safety_floor / ground_init).
func _build_safety_floor() -> void:
	var body := StaticBody3D.new()
	body.name = "SafetyFloor"
	var col := CollisionShape3D.new()
	var plane := WorldBoundaryShape3D.new()
	plane.plane = Plane(Vector3.UP, CoordSpace.scalar(-800.0))   # y = -8 (below town)
	col.shape = plane
	body.add_child(col)
	add_child(body)

func _mesh_instances(node: Node, acc: Array = []) -> Array:
	if node is MeshInstance3D and node.mesh != null:
		acc.append(node)
	for c in node.get_children():
		_mesh_instances(c, acc)
	return acc

func _build_entities() -> void:
	for e in manifest.get("entities", []):
		var fourcc := str(e.get("type", ""))
		if EntityRegistry.is_invisible(fourcc):
			continue
		var marker := _debug_marker(EntityRegistry.debug_color(fourcc))
		marker.name = "%s_%s" % [fourcc, str(e.get("tag", ""))]
		marker.position = CoordSpace.to_godot(CoordSpace.vec_from(e.get("pos")))
		_entities_root.add_child(marker)

## Resolve a foundry-relative path (e.g. "assets/glb/omt/labshak.glb") to an
## absolute path in the C/Python repo (<repo>/assets/...), so Godot reads the
## foundry output directly without importing the 6500+ glb tree. <repo> is the
## parent of the Godot project dir (res://).
func _foundry_abs(rel: String) -> String:
	var repo := ProjectSettings.globalize_path("res://").path_join("..")
	return repo.path_join(rel).simplify_path()

## Runtime glb -> Node3D (parses the raw file; no editor import needed).
func _load_glb(abs_path: String) -> Node3D:
	if not FileAccess.file_exists(abs_path):
		return null
	var doc := GLTFDocument.new()
	var state := GLTFState.new()
	if doc.append_from_file(abs_path, state) != OK:
		return null
	var scene := doc.generate_scene(state)
	return scene as Node3D

## A small unlit cube, tinted, for eyeballing entity placement.
func _debug_marker(color: Color) -> MeshInstance3D:
	var mi := MeshInstance3D.new()
	var box := BoxMesh.new()
	# ~1.5 native-unit-equivalent cube post-scale; visible but not overpowering.
	box.size = Vector3.ONE * CoordSpace.scalar(150.0)
	mi.mesh = box
	var mat := StandardMaterial3D.new()
	mat.albedo_color = color
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mi.material_override = mat
	return mi

func _read_json(res_path: String) -> Dictionary:
	if not FileAccess.file_exists(res_path):
		return {}
	var f := FileAccess.open(res_path, FileAccess.READ)
	var data = JSON.parse_string(f.get_as_text())
	return data if data is Dictionary else {}
