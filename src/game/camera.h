#ifndef GAME_CAMERA_H
#define GAME_CAMERA_H

#include "../engine/world.h"
#include "../engine/renderer.h"

/* Smoothed third-person follow camera. */

typedef struct {
    float yaw;            /* orbital yaw around player (radians) */
    float pitch;          /* orbital pitch (radians; downward = negative) */
    float distance;       /* distance from player */
    float height;         /* vertical offset above player's feet */
    float smoothing;      /* 0 = no smoothing, 1 = no movement */
    int   manual_yaw;     /* if non-zero, mouse-drag overrides yaw */
} FollowCam;

void follow_cam_init(FollowCam *fc);
/* `world` is optional — pass NULL to disable wall-clamping. */
void follow_cam_update(FollowCam *fc, Camera *cam, const Entity *target, const World *world, float dt);
/* Snap camera to its desired pose immediately, ignoring smoothing. */
void follow_cam_snap(FollowCam *fc, Camera *cam, const Entity *target);

#endif
