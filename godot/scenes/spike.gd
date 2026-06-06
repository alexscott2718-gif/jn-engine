extends Node3D

## Phase-0 spike orchestrator (bridge plan §6 Phase-0 — the go/no-go).
## Builds environment + light in code, loads level1 via LevelLoader, drops the
## data-driven Player at the manifest spawn. Everything is code-only so the
## scene file stays a trivial wrapper (easy to diff / regenerate).
##
## Goal of this scene: walk Retroville and judge coords / scale / feel against
## the C engine + capture, then FREEZE CoordSpace (§4).

const LEVEL_PATH := "res://levels/level1.json"

func _ready() -> void:
	_build_environment()
	var loader := _build_loader()
	_build_player(loader)
	_print_help()
	# Optional screenshot-then-quit for headed verification + §4 capture diffing.
	# JN_SHOT=/abs/path.png  (also reused later to diff a Godot frame vs capture).
	if OS.get_environment("JN_BOUNDS") != "":
		_print_bounds(loader)
		get_tree().quit()
		return
	if OS.get_environment("JN_OVERVIEW") != "":
		_overview_camera(loader)
	var shot := OS.get_environment("JN_SHOT")
	if shot != "":
		_screenshot_after(shot, 120)   # ~2s: let the player fall + settle

## Bright markers at known landmarks, for handedness comparison vs the native
## labeled top-down. Expected (right=+X, up=-Z): labshak left+above JIM1 spawn,
## HOUSE BASE far left.
func _landmark_marks(loader: LevelLoader) -> void:
	_mark(loader.spawn_pos, Color(1, 0, 0))          # JIM1 spawn  - RED
	var root := loader.get_node_or_null("Placements")
	if root == null:
		return
	var named := {"labshak": Color(0, 1, 1), "HOUSE BASE": Color(0, 1, 0), "JHOUSE": Color(1, 0, 1)}
	for c in root.get_children():
		if named.has(str(c.name)):
			_mark(c.position, named[str(c.name)])

func _mark(pos: Vector3, color: Color) -> void:
	var mi := MeshInstance3D.new()
	var s := SphereMesh.new()
	s.radius = 8.0
	s.height = 16.0
	mi.mesh = s
	mi.position = pos
	mi.position.y = 60.0   # float above town so it's never occluded from above
	var mat := StandardMaterial3D.new()
	mat.albedo_color = color
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mi.material_override = mat
	add_child(mi)

## Diagnostic: combined global-space AABB of every placement MeshInstance, plus
## the largest single mesh — tells us the true world extent vs the centroid span.
func _print_bounds(loader: LevelLoader) -> void:
	var root := loader.get_node_or_null("Placements")
	if root == null:
		print("[bounds] no Placements")
		return
	var meshes: Array = []
	_gather_meshes(root, meshes)
	if meshes.is_empty():
		print("[bounds] no meshes")
		return
	var total: AABB = (meshes[0].global_transform * meshes[0].get_aabb())
	var biggest_vol := -1.0
	var biggest_name := ""
	for mi in meshes:
		var w: AABB = mi.global_transform * mi.get_aabb()
		total = total.merge(w)
		var vol := w.get_volume()
		if vol > biggest_vol:
			biggest_vol = vol
			biggest_name = mi.get_parent().name if mi.get_parent() else mi.name
	print("[bounds] meshes=%d total.pos=%s total.size=%s" % [meshes.size(), total.position, total.size])
	print("[bounds] biggest=%s size=%s" % [biggest_name, (meshes[0].global_transform * meshes[0].get_aabb()).size if false else _biggest_size(meshes)])

func _biggest_size(meshes: Array) -> Vector3:
	var bv := -1.0
	var bs := Vector3.ZERO
	for mi in meshes:
		var w: AABB = mi.global_transform * mi.get_aabb()
		if w.get_volume() > bv:
			bv = w.get_volume()
			bs = w.size
	return bs

func _gather_meshes(node: Node, acc: Array) -> void:
	if node is MeshInstance3D and node.mesh != null:
		acc.append(node)
	for c in node.get_children():
		_gather_meshes(c, acc)

## Top-down camera over the placement footprint — for judging coords / scale /
## handedness against the C engine's native map (§4). Overrides the player cam.
## Looks straight down (-Y) with screen-up = world -Z, so the framing matches a
## conventional top-down map: +X right, +Z toward the bottom of the frame.
## Uses placement node positions only (fast, robust); height is generous so it
## clears even large baked meshes.
func _overview_camera(loader: LevelLoader) -> void:
	var root := loader.get_node_or_null("Placements")
	if root == null:
		return
	# BLOCK* placements are the original's invisible collision hulls (BLOCKTop is
	# a map-wide lid). Hide them so the overview shows the actual town.
	for c in root.get_children():
		if str(c.name).begins_with("BLOCK"):
			c.visible = false
	var meshes: Array = []
	_gather_meshes(root, meshes)
	meshes = meshes.filter(func(mi): return mi.is_visible_in_tree())
	if meshes.is_empty():
		return
	var bounds: AABB = meshes[0].global_transform * meshes[0].get_aabb()
	for mi in meshes:
		bounds = bounds.merge(mi.global_transform * mi.get_aabb())
	var center := bounds.position + bounds.size * 0.5
	var span: float = maxf(bounds.size.x, bounds.size.z)
	var cam := Camera3D.new()
	cam.fov = 55.0
	cam.near = maxf(span * 0.001, 0.05)
	cam.far = span * 8.0
	# Straight down from above the top of the world; screen-up = world -Z.
	cam.position = Vector3(center.x, bounds.position.y + bounds.size.y + span * 0.9, center.z)
	add_child(cam)
	cam.look_at(center, Vector3(0, 0, -1))
	cam.make_current()
	if OS.get_environment("JN_MARKS") != "":
		_landmark_marks(loader)
	print("[spike] overview: center=%s span=%.1f y=[%.1f..%.1f] cam_y=%.1f" % [
		center, span, bounds.position.y, bounds.position.y + bounds.size.y, cam.position.y])

func _screenshot_after(path: String, frames: int) -> void:
	for i in frames:
		await get_tree().process_frame
	if _player:
		print("[spike] player @ %s on_floor=%s" % [_player.global_position, _player.is_on_floor()])
	var img := get_viewport().get_texture().get_image()
	img.save_png(path)
	print("[spike] wrote screenshot -> %s" % path)
	get_tree().quit()

func _build_environment() -> void:
	# Sky tint mirrors renderer_set_sky(bluesky3) in src/game/main.c.
	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	var sky := Sky.new()
	var sky_mat := ProceduralSkyMaterial.new()
	sky_mat.sky_top_color = Color(0.09, 0.40, 0.62)
	sky_mat.sky_horizon_color = Color(0.24, 0.50, 0.68)
	sky_mat.ground_bottom_color = Color(0.20, 0.30, 0.25)
	sky_mat.ground_horizon_color = Color(0.24, 0.50, 0.68)
	sky.sky_material = sky_mat
	env.sky = sky
	env.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	env.ambient_light_energy = 1.0
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)

	# Flat full-bright was the measured original lighting (Phase 12 WI-1); a
	# single soft directional keeps meshes readable without baking shadows.
	var sun := DirectionalLight3D.new()
	sun.rotation = Vector3(deg_to_rad(-55.0), deg_to_rad(35.0), 0.0)
	sun.light_energy = 1.0
	add_child(sun)

func _build_loader() -> LevelLoader:
	var loader := LevelLoader.new()
	loader.name = "Level"
	loader.level_path = LEVEL_PATH
	add_child(loader)
	loader.load_level()
	return loader

func _build_player(loader: LevelLoader) -> void:
	var player := Player.new()
	player.name = "Player"
	player.physics_path = loader.physics_path
	add_child(player)
	# Drop slightly above the resolved spawn so the body settles onto geometry.
	player.global_position = loader.spawn_pos + Vector3(0.0, CoordSpace.scalar(100.0), 0.0)
	player.rotation.y = deg_to_rad(loader.spawn_yaw_deg)
	player.auto_walk = OS.get_environment("JN_AUTOWALK") != ""
	_player = player

var _player: Player

func _print_help() -> void:
	print("=== JN Retroville spike ===")
	print("WASD/arrows move · Space jump · mouse look · Esc free mouse")
	print("CoordSpace: scale=%.3f flips=(%s,%s,%s)  <-- tune in core/CoordSpace.gd"
		% [CoordSpace.WORLD_SCALE, CoordSpace.FLIP_X, CoordSpace.FLIP_Y, CoordSpace.FLIP_Z])
