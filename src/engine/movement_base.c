#include "movement_base.h"
#include "physics.h"
#include <math.h>
#include <stddef.h>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

MovementTankInput movement_base_tank_drive(Entity *e, float turn, float forward,
                                           float speed, float turn_rate, float dt) {
    MovementTankInput out;
    out.turn = clampf(turn, -1.0f, 1.0f);
    out.forward = clampf(forward, -1.0f, 1.0f);
    if (!e) return out;

    e->ry -= out.turn * turn_rate * dt;
    if (fabsf(out.forward) > 0.0001f) {
        e->vx = sinf(e->ry) * speed * out.forward;
        e->vz = cosf(e->ry) * speed * out.forward;
    } else {
        e->vx = 0.0f;
        e->vz = 0.0f;
    }
    return out;
}

void movement_base_flying_params_from_gam(const Entity *e,
                                          MovementFlyingParams *out) {
    if (!out) return;
    out->accel_rate = gam_prop_f(e, "AccelRate", 400.0f);
    out->decel_rate = gam_prop_f(e, "DecelRate", 1.0f);
    out->max_speed = gam_prop_f(e, "MaxSpeed", 1400.0f);
    out->max_height = gam_prop_f(e, "MaxHeight", 4000.0f);
    out->up_rate = gam_prop_f(e, "UpRate", 650.0f);
    out->down_rate = gam_prop_f(e, "DownRate", -650.0f);
    out->max_vert_velocity = gam_prop_f(e, "MaxVertVelocity", 650.0f);
    out->new_gravity = gam_prop_f(e, "NewGravity", 0.0f);
    out->accel_lean = gam_prop_f(e, "AccelLean", 20.0f);
    out->decel_lean = gam_prop_f(e, "DecelLean", -20.0f);
}

void movement_base_flying_step(Entity *e, const MovementFlyingParams *p,
                               const MovementFlyingInput *in, float dt) {
    if (!e || !p || !in || dt <= 0.0f) return;

    float accel = p->accel_rate;
    float decel = p->decel_rate;
    if (accel < 0.0f) accel = 0.0f;
    if (decel < 0.0f) decel = -decel;

    if (in->forward > 0.0f || in->up > 0.0f) {
        e->move_speed += accel * dt;
        if (e->move_speed > p->max_speed) e->move_speed = p->max_speed;
    } else if (e->move_speed > p->max_speed * 0.909f) {
        e->move_speed -= decel * 0.5f * dt;
    }

    if (in->brake > 0.0f || in->down > 0.0f) {
        e->move_speed -= decel * (1.0f + in->brake + in->down) * dt;
        if (e->move_speed < 0.0f) e->move_speed = 0.0f;
    }

    float turn = clampf(in->turn, -1.0f, 1.0f);
    float lean_target = 0.0f;
    if (turn != 0.0f)
        lean_target = turn * (in->forward > 0.0f ? p->accel_lean : -p->decel_lean);
    else if (in->forward > 0.0f || in->up > 0.0f)
        lean_target = p->accel_lean;
    else if (in->brake > 0.0f || in->down > 0.0f)
        lean_target = p->decel_lean;
    e->move_lean += (lean_target - e->move_lean) * clampf(dt * 6.0f, 0.0f, 1.0f);
    e->move_lean = clampf(e->move_lean, -45.0f, 45.0f);

    if (in->up > 0.0f) {
        e->move_vert = p->up_rate;
    } else if (in->down > 0.0f) {
        e->move_vert = p->down_rate;
    } else if (p->new_gravity != 0.0f) {
        e->move_vert -= p->new_gravity * dt;
    } else {
        e->move_vert -= PHYSICS_GRAVITY * dt;
    }
    e->move_vert = clampf(e->move_vert, -p->max_vert_velocity, p->max_vert_velocity);

    if (p->max_height > 0.0f && e->y >= p->max_height) {
        e->y = p->max_height;
        if (e->move_vert > 0.0f) e->move_vert = 0.0f;
    }

    e->vx = sinf(e->ry) * e->move_speed;
    e->vz = cosf(e->ry) * e->move_speed;
    e->vy = e->move_vert;
    e->rz = e->move_lean * 0.017453292519943295f;
}
