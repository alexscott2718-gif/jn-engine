#ifndef GAME_CAMERA_H
#define GAME_CAMERA_H

#include "../engine/world.h"
#include "../engine/renderer.h"

/* Third-person follow camera locked behind the player's back (tank-turn).
   The camera yaw tracks the player's heading (cam.yaw = PI - target->ry) so it
   always sits behind Jimmy and turns with him. `yaw_offset` is an optional
   mouse free-look delta layered on top; it decays back to zero (snaps behind)
   while the player is moving or turning. */

typedef struct {
    float yaw;            /* smoothed effective orbital yaw (radians) */
    float pitch;          /* orbital pitch (radians; downward = negative) */
    float distance;       /* distance from player */
    float height;         /* vertical offset above player's feet */
    float smoothing;      /* position smoothing: 0 = snap, ->1 = sluggish */
    float yaw_smoothing;  /* how tightly yaw tracks the player's heading */
    float yaw_offset;     /* mouse free-look delta from behind-the-back (rad) */
} FollowCam;

void follow_cam_init(FollowCam *fc);
/* `world` is optional — pass NULL to disable wall-clamping. */
void follow_cam_update(FollowCam *fc, Camera *cam, const Entity *target, const World *world, float dt);
/* Snap camera to its desired pose immediately, ignoring smoothing. */
void follow_cam_snap(FollowCam *fc, Camera *cam, const Entity *target);

#endif
