#include "camera.h"
#include <math.h>

void follow_cam_init(FollowCam *fc) {
    fc->yaw       = 0.0f;
    fc->pitch     = -0.30f;
    fc->distance  = 450.0f;
    fc->height    = 180.0f;
    fc->smoothing = 0.12f;
    fc->manual_yaw = 1;
}

void follow_cam_update(FollowCam *fc, Camera *cam, const Entity *target, const World *world, float dt) {
    if (!target) return;
    (void)dt;

    float cy = cosf(fc->yaw), sy = sinf(fc->yaw);
    float cp = cosf(fc->pitch), sp = sinf(fc->pitch);

    /* Aim the camera at the player's head, not their feet. */
    float aim_y = target->y + fc->height;

    float desired_x = target->x - sy * cp * fc->distance;
    float desired_y = aim_y         - sp * fc->distance;
    float desired_z = target->z + cy * cp * fc->distance;

    /* Wall-clamp: sweep from the aim point toward desired; if a solid box
       is in the way, sit just in front of it. */
    if (world) {
        float t = world_query_segment(world, target,
                                      target->x, aim_y, target->z,
                                      desired_x, desired_y, desired_z);
        const float skin = 0.92f;   /* margin so we don't poke through */
        if (t < 1.0f) {
            float clamped = t * skin;
            desired_x = target->x + (desired_x - target->x) * clamped;
            desired_y = aim_y     + (desired_y - aim_y)     * clamped;
            desired_z = target->z + (desired_z - target->z) * clamped;
        }
    }

    /* Exponential smoothing toward the desired position. */
    float a = fc->smoothing;
    if (a <= 0.0f) a = 1.0f;
    cam->pos[0] += (desired_x - cam->pos[0]) * a;
    cam->pos[1] += (desired_y - cam->pos[1]) * a;
    cam->pos[2] += (desired_z - cam->pos[2]) * a;
    cam->yaw    = fc->yaw;
    cam->pitch  = fc->pitch;
}

void follow_cam_snap(FollowCam *fc, Camera *cam, const Entity *target) {
    if (!target) return;
    float cy = cosf(fc->yaw), sy = sinf(fc->yaw);
    float cp = cosf(fc->pitch), sp = sinf(fc->pitch);
    cam->pos[0] = target->x - sy * cp * fc->distance;
    cam->pos[1] = target->y + fc->height - sp * fc->distance;
    cam->pos[2] = target->z + cy * cp * fc->distance;
    cam->yaw    = fc->yaw;
    cam->pitch  = fc->pitch;
}
