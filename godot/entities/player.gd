class_name Player
extends CharacterBody3D

## Retroville player — a CharacterBody3D driven by the foundry's C3DPlayer
## constants (physics/<level>.json), with tank-turn controls and a behind-the-
## back follow camera (matches the C engine's Retroville feel).
##
## Controls: W/S move forward/back along facing, A/D turn (tank), Space jumps.
## The Jimmy mesh is jimstop.glb (flat-coloured until its texture is sourced).

@export var physics_path: String = "res://physics/level1.json"
@export var model_path: String = "assets/glb/ase/jimstop.glb"   # foundry-relative
@export var turn_rate_deg: float = 140.0
@export var model_yaw_deg: float = 0.0   # spin the mesh if it faces the wrong way
var auto_walk: bool = false              # scripted forward walk (headless checks)

# Player size, in Godot units. The world is WORLD_SCALE'd, so a fire hydrant is
# ~1u; Jimmy reads as a small kid against ~20u houses.
const HEIGHT := 1.6
const RADIUS := 0.4

# Native C3DPlayer constants (loaded from json; kept in native units).
var max_speed_n: float = 1400.0
var accel_n: float = 400.0
var decel_n: float = 1.0
var up_rate_n: float = 650.0
var down_rate_n: float = -650.0
var max_vert_n: float = 650.0

var _cam: Camera3D
var _model: Node3D

func _ready() -> void:
	_load_physics()
	_build_body()
	_build_camera()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

func _load_physics() -> void:
	if not FileAccess.file_exists(physics_path):
		push_warning("Player: no physics json at %s; using C defaults" % physics_path)
		return
	var f := FileAccess.open(physics_path, FileAccess.READ)
	var d = JSON.parse_string(f.get_as_text())
	if not (d is Dictionary) or not d.has("constants"):
		return
	var c: Dictionary = d["constants"]
	max_speed_n = float(c.get("max_speed", max_speed_n))
	accel_n = float(c.get("accel_rate", accel_n))
	decel_n = float(c.get("decel_rate", decel_n))
	up_rate_n = float(c.get("up_rate", up_rate_n))
	down_rate_n = float(c.get("down_rate", down_rate_n))
	max_vert_n = float(c.get("max_vert_velocity", max_vert_n))
	print("[Player] physics: MaxSpeed=%.0f Accel=%.0f Up=%.0f (native)" % [
		max_speed_n, accel_n, up_rate_n])

func _build_body() -> void:
	if get_child_count() > 0:
		return
	var cyl := HEIGHT - 2.0 * RADIUS   # capsule cylinder portion
	var shape := CollisionShape3D.new()
	var cap := CapsuleShape3D.new()
	cap.radius = RADIUS
	cap.height = HEIGHT
	shape.shape = cap
	add_child(shape)

	# Jimmy mesh, scaled to fit HEIGHT with feet seated at the capsule bottom.
	_model = _load_glb(_foundry_abs(model_path))
	if _model:
		add_child(_model)                       # in tree first, so AABB is valid
		var b := _model_local_aabb(_model)
		if b.size.y > 0.0:
			var s := HEIGHT / b.size.y
			_model.scale = Vector3.ONE * s
			# Seat the (scaled) mesh min.y at the capsule bottom (-HEIGHT/2).
			_model.position.y = -HEIGHT * 0.5 - b.position.y * s
		_model.rotation.y = deg_to_rad(model_yaw_deg)
	else:
		var mi := MeshInstance3D.new()   # fallback capsule
		var m := CapsuleMesh.new(); m.radius = RADIUS; m.height = HEIGHT
		mi.mesh = m
		var mat := StandardMaterial3D.new(); mat.albedo_color = Color(1, 1, 0)
		mi.material_override = mat
		add_child(mi)

## Behind-the-back follow camera. As a child of the player it swings with the
## tank turn, always sitting behind + above Jimmy and looking forward over him.
func _build_camera() -> void:
	_cam = Camera3D.new()
	_cam.position = Vector3(0.0, HEIGHT * 0.9, HEIGHT * 2.6)   # +Z = behind (faces -Z)
	_cam.rotation.x = deg_to_rad(-14.0)                        # tilt down toward Jimmy
	_cam.near = 0.05
	_cam.far = CoordSpace.scalar(28000.0)
	_cam.current = true
	add_child(_cam)

func _physics_process(delta: float) -> void:
	# Tank turn: A/D rotate the whole player (camera follows as a child).
	var turn := 0.0
	if Input.is_physical_key_pressed(KEY_A) or Input.is_physical_key_pressed(KEY_LEFT):
		turn += 1.0
	if Input.is_physical_key_pressed(KEY_D) or Input.is_physical_key_pressed(KEY_RIGHT):
		turn -= 1.0
	rotation.y += turn * deg_to_rad(turn_rate_deg) * delta

	# Forward/back along facing.
	var drive := 0.0
	if Input.is_physical_key_pressed(KEY_W) or Input.is_physical_key_pressed(KEY_UP) or auto_walk:
		drive += 1.0
	if Input.is_physical_key_pressed(KEY_S) or Input.is_physical_key_pressed(KEY_DOWN):
		drive -= 1.0
	var forward := -transform.basis.z
	var wish := forward * drive

	var max_speed := CoordSpace.scalar(max_speed_n)
	var accel := CoordSpace.scalar(accel_n)
	var decel := CoordSpace.scalar(maxf(decel_n, accel_n))  # decel_rate=1 in level1; floor to accel
	var hv := Vector3(velocity.x, 0.0, velocity.z)
	if drive != 0.0:
		hv = hv.move_toward(wish * max_speed, accel * delta)
	else:
		hv = hv.move_toward(Vector3.ZERO, decel * delta)
	velocity.x = hv.x
	velocity.z = hv.z

	# Vertical: data-driven jump/gravity (down_rate is negative).
	var gravity := CoordSpace.scalar(down_rate_n)
	var max_vert := CoordSpace.scalar(max_vert_n)
	if is_on_floor():
		velocity.y = CoordSpace.scalar(up_rate_n) if Input.is_physical_key_pressed(KEY_SPACE) else 0.0
	else:
		velocity.y = clampf(velocity.y + gravity * delta, -max_vert, max_vert)

	move_and_slide()

# --- helpers ---------------------------------------------------------------

func _foundry_abs(rel: String) -> String:
	var repo := ProjectSettings.globalize_path("res://").path_join("..")
	return repo.path_join(rel).simplify_path()

func _load_glb(abs_path: String) -> Node3D:
	if not FileAccess.file_exists(abs_path):
		return null
	var doc := GLTFDocument.new()
	var state := GLTFState.new()
	if doc.append_from_file(abs_path, state) != OK:
		return null
	return doc.generate_scene(state) as Node3D

## AABB of all meshes expressed in `root`'s local space (root must be in tree).
func _model_local_aabb(root: Node3D) -> AABB:
	var meshes: Array = []
	_gather(root, meshes)
	if meshes.is_empty():
		return AABB()
	var inv := root.global_transform.affine_inverse()
	var b: AABB = (inv * meshes[0].global_transform) * meshes[0].get_aabb()
	for mi in meshes:
		b = b.merge((inv * mi.global_transform) * mi.get_aabb())
	return b

func _gather(node: Node, acc: Array) -> void:
	if node is MeshInstance3D and node.mesh != null:
		acc.append(node)
	for c in node.get_children():
		_gather(c, acc)
