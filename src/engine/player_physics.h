#ifndef PLAYER_PHYSICS_H
#define PLAYER_PHYSICS_H

/* Player movement/physics constants, parsed at level load from the .gam
   C3DPlayer block (see gam_loader.c). The original game is data-driven from
   these; defaults below are the measured Level1.gam values so the engine is
   sane even if a level omits them. Provenance: docs/jimmy_animation_plan.md
   (Neutron.exe C3DPlayer @ 0x40ed40 / InitObject). */
typedef struct {
    float max_speed;         /* top ground speed (units/s) */
    float accel_rate;        /* ground acceleration */
    float decel_rate;        /* ground deceleration (low -> long momentum coast) */
    float max_height;        /* jump height cap */
    float up_rate;           /* jump ascent rate */
    float down_rate;         /* descent rate (negative) */
    float max_vert_velocity; /* vertical speed cap */
    float accel_lean;        /* body lean into acceleration/turns */
    float decel_lean;        /* body lean into deceleration */
    int   loaded;            /* 1 once populated from a .gam */
} PlayerPhysics;

/* Reset to the measured Level1.gam defaults (call at the start of gam_load). */
void player_physics_reset_defaults(void);
PlayerPhysics *player_physics_mutable(void);
const PlayerPhysics *player_physics_get(void);

#endif /* PLAYER_PHYSICS_H */
