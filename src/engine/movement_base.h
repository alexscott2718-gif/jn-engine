#ifndef MOVEMENT_BASE_H
#define MOVEMENT_BASE_H

#include "world.h"

typedef struct MovementTankInput {
    float turn;
    float forward;
} MovementTankInput;

typedef struct MovementFlyingInput {
    float forward;   /* 0..1 */
    float brake;     /* 0..1 */
    float turn;      /* -1..1 */
    float up;        /* 0..1 */
    float down;      /* 0..1 */
} MovementFlyingInput;

typedef struct MovementFlyingParams {
    float accel_rate;
    float decel_rate;
    float max_speed;
    float max_height;
    float up_rate;
    float down_rate;
    float max_vert_velocity;
    float new_gravity;
    float accel_lean;
    float decel_lean;
} MovementFlyingParams;

MovementTankInput movement_base_tank_drive(Entity *e, float turn, float forward,
                                           float speed, float turn_rate, float dt);

void movement_base_flying_params_from_gam(const Entity *e,
                                          MovementFlyingParams *out);
void movement_base_flying_step(Entity *e, const MovementFlyingParams *p,
                               const MovementFlyingInput *in, float dt);

#endif
