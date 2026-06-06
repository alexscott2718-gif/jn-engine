class_name Player
extends CharacterBody3D

## Phase-0 player — a CharacterBody3D tuned entirely from the foundry's
## physics/<level>.json (the original C3DPlayer constants). The point of the
## spike is to feel whether those numbers + the CoordSpace world scale produce
## sane movement (bridge plan §2b / §3 / §6 Phase-0). Self-contained: it builds
## its own collision shape, a visible body, and a SpringArm follow cam so the
## spike scene stays code-only.
##
## Controls: WASD / arrows move (camera-relative), Space jumps, mouse looks,
## Esc releases the mouse. Tank-turn feel is deferred to Phase 2.

@export var physics_path: String = "res://physics/level1.json"
@export var mouse_sensitivity: float = 0.003
var auto_walk: bool = false   ## scripted forward walk, for headless verification

# Native C3DPlayer constants (loaded from json), kept in native units.
var max_speed_n: float = 1400.0
var accel_n: float = 400.0
var decel_n: float = 1.0
var up_rate_n: float = 650.0
var down_rate_n: float = -650.0
var max_vert_n: float = 650.0

var _yaw: float = 0.0
var _pitch: float = -0.2
var _spring: SpringArm3D
var _cam: Camera3D

func _ready() -> void:
	_load_physics()
	_build_body()
	_build_camera()
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

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
	print("[Player] physics: MaxSpeed=%.0f Accel=%.0f Up=%.0f Down=%.0f (native)" % [
		max_speed_n, accel_n, up_rate_n, down_rate_n])

func _build_body() -> void:
	if get_child_count() > 0:
		return
	var shape := CollisionShape3D.new()
	var cap := CapsuleShape3D.new()
	cap.radius = CoordSpace.scalar(120.0)
	cap.height = CoordSpace.scalar(420.0)
	shape.shape = cap
	add_child(shape)

	var mi := MeshInstance3D.new()
	var mesh := CapsuleMesh.new()
	mesh.radius = cap.radius
	mesh.height = cap.height
	mi.mesh = mesh
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(1.0, 1.0, 0.0)   # yellow = player (matches main.c)
	mi.material_override = mat
	add_child(mi)

func _build_camera() -> void:
	_spring = SpringArm3D.new()
	_spring.spring_length = CoordSpace.scalar(900.0)
	_spring.position.y = CoordSpace.scalar(250.0)
	add_child(_spring)
	_cam = Camera3D.new()
	_cam.near = CoordSpace.scalar(20.0)
	_cam.far = CoordSpace.scalar(28000.0)
	_cam.current = true
	_spring.add_child(_cam)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		_yaw -= event.relative.x * mouse_sensitivity
		_pitch = clamp(_pitch - event.relative.y * mouse_sensitivity, -1.3, 0.4)
	elif event is InputEventKey and event.pressed and event.keycode == KEY_ESCAPE:
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

func _physics_process(delta: float) -> void:
	# Camera rig follows yaw/pitch.
	if _spring:
		rotation.y = _yaw
		_spring.rotation.x = _pitch

	var input := Vector3.ZERO
	if auto_walk:
		input.z -= 1.0
	if Input.is_physical_key_pressed(KEY_W) or Input.is_physical_key_pressed(KEY_UP):
		input.z -= 1.0
	if Input.is_physical_key_pressed(KEY_S) or Input.is_physical_key_pressed(KEY_DOWN):
		input.z += 1.0
	if Input.is_physical_key_pressed(KEY_A) or Input.is_physical_key_pressed(KEY_LEFT):
		input.x -= 1.0
	if Input.is_physical_key_pressed(KEY_D) or Input.is_physical_key_pressed(KEY_RIGHT):
		input.x += 1.0

	var dir := (transform.basis * input).normalized() if input != Vector3.ZERO else Vector3.ZERO
	var max_speed := CoordSpace.scalar(max_speed_n)
	var accel := CoordSpace.scalar(accel_n)
	var decel := CoordSpace.scalar(max(decel_n, accel_n))  # decel_rate is 1.0 in level1; floor to accel for a usable stop

	var hv := Vector3(velocity.x, 0.0, velocity.z)
	if dir != Vector3.ZERO:
		hv = hv.move_toward(dir * max_speed, accel * delta)
	else:
		hv = hv.move_toward(Vector3.ZERO, decel * delta)
	velocity.x = hv.x
	velocity.z = hv.z

	# Vertical: data-driven jump/gravity (down_rate is negative).
	var gravity := CoordSpace.scalar(down_rate_n)        # negative
	var max_vert := CoordSpace.scalar(max_vert_n)
	if is_on_floor():
		if Input.is_physical_key_pressed(KEY_SPACE):
			velocity.y = CoordSpace.scalar(up_rate_n)
		else:
			velocity.y = 0.0
	else:
		velocity.y = clamp(velocity.y + gravity * delta, -max_vert, max_vert)

	move_and_slide()
