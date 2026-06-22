#include "behavior_flying.h"

void behavior_flying_spawn_base(Entity *e) {
    if (!e) return;
    behavior_animated_spawn_base(e);
    e->move_speed = 0.0f;
    e->move_vert = 0.0f;
    e->move_lean = 0.0f;
    e->home[1] = e->y;  /* C3DFlyingObject reset seeds ground/initial Y. */
}

int behavior_flying_update_base(Entity *e, World *w,
                                const MovementFlyingInput *input, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return 0;
    if (!input) return 1;

    MovementFlyingParams params;
    movement_base_flying_params_from_gam(e, &params);
    movement_base_flying_step(e, &params, input, dt);
    return 1;
}
