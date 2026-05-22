#ifndef RENDERER_H
#define RENDERER_H

#include "assets/ase_loader.h"

typedef struct {
    float pos[3];    /* camera world position */
    float yaw;       /* horizontal rotation (radians) */
    float pitch;     /* vertical rotation (radians) */
    float fov;       /* field of view (radians) */
    float near_z;
    float far_z;
} Camera;

int  renderer_init(int viewport_w, int viewport_h);
void renderer_destroy(void);

void renderer_begin_frame(int viewport_w, int viewport_h);
/* texture_id_override = 0 means use m->texture_id (model's bound texture).
   scale is a uniform mesh scale (1.0 = original ASE units). */
void renderer_draw_model(const AseModel *m, unsigned int texture_id_override,
                         float tx, float ty, float tz, float yaw, float scale);
void renderer_draw_box(unsigned int vao, int index_count,
                       float tx, float ty, float tz, float scale,
                       float r, float g, float b);
/* Phase 10 Step 2: camera-facing alpha-tested billboard. Width/height in
   world units. Tint multiplies the texel; pass tint_a == 0 for no tint. */
void renderer_draw_billboard(unsigned int tex,
                             float tx, float ty, float tz,
                             float width, float height,
                             float tint_r, float tint_g, float tint_b, float tint_a);
void renderer_end_frame(void);

/* Set the sky gradient (top and bottom RGB). Drawn before depth-tested geometry. */
void renderer_set_sky(float top_r, float top_g, float top_b,
                      float bot_r, float bot_g, float bot_b);

Camera *renderer_camera(void);

/* Copy the current projection*view matrix (computed at begin_frame) into out. */
void renderer_get_view_proj(float out[16]);

/* M7a — matched-camera capture. Install an explicit GL column-major view and
   projection; begin_frame then uses them verbatim instead of deriving the
   camera from the follow cam. Pass NULL to clear the override. */
void renderer_set_camera_override(const float view[16], const float proj[16]);
int  renderer_camera_override_active(void);

#endif
