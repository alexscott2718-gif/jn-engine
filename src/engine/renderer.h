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
/* Prepare camera/projection state without clearing the framebuffer. Used for
   compositing live actors over a capture-backed scene. */
void renderer_begin_overlay(int viewport_w, int viewport_h);
/* texture_id_override = 0 means use m->texture_id (model's bound texture).
   scale is a uniform mesh scale (1.0 = original ASE units). */
void renderer_draw_model(const AseModel *m, unsigned int texture_id_override,
                         float tx, float ty, float tz, float yaw, float scale);
void renderer_draw_model_matrix(const AseModel *m, unsigned int texture_id_override,
                                const float model[16]);
void renderer_draw_model_anim(const AseModel *m, unsigned int texture_id_override,
                              float tx, float ty, float tz, float yaw, float scale,
                              int frame_a, int frame_b, float lerp);
/* As above, plus pitch/roll body lean (radians) applied in the yawed frame. */
void renderer_draw_model_anim_euler(const AseModel *m, unsigned int texture_id_override,
                                    float tx, float ty, float tz,
                                    float yaw, float pitch, float roll, float scale,
                                    int frame_a, int frame_b, float lerp);
void renderer_draw_model_matrix_anim(const AseModel *m, unsigned int texture_id_override,
                                     const float model[16],
                                     int frame_a, int frame_b, float lerp);
void renderer_draw_box(unsigned int vao, int index_count,
                       float tx, float ty, float tz, float scale,
                       float r, float g, float b);
/* Faithful sky dome (bluesky3): centered on the camera, drawn behind all scene
   geometry (pass spin=0 for the static painted-neighborhood backdrop). */
void renderer_draw_sky_dome(const AseModel *m, float spin);
/* Full procedural cloud hemisphere (real sky.png texture), rotated by `spin`
   radians for drifting clouds. Drawn behind the faithful neighborhood backdrop. */
void renderer_draw_cloud_dome(unsigned int tex, float spin);
/* Foliage chroma-key: while enabled, the lit shader discards textured fragments
   within `tol` (squared RGB distance, 0..3) of (r,g,b). Used to punch the baked
   sky-blue out of the 2D_Trees boundary walls so the real sky shows through. */
void renderer_set_color_key(int enable, float r, float g, float b, float tol);
/* Phase 10 Step 2: camera-facing alpha-tested billboard. Width/height in
   world units. Tint multiplies the texel; pass tint_a == 0 for no tint. */
void renderer_draw_billboard(unsigned int tex,
                             float tx, float ty, float tz,
                             float width, float height,
                             float tint_r, float tint_g, float tint_b, float tint_a);
/* Screen-space solid rectangle, coordinates in pixels from the top-left. */
void renderer_draw_screen_rect(int viewport_w, int viewport_h,
                               float x, float y, float width, float height,
                               float r, float g, float b, float a);
void renderer_end_frame(void);

/* Set the sky gradient (top and bottom RGB). Drawn before depth-tested geometry. */
void renderer_set_sky(float top_r, float top_g, float top_b,
                      float bot_r, float bot_g, float bot_b);

/* Phase 1: per-channel ambient/scene tint multiplied onto every textured/flat
 * draw in the lit shader. Default (1,1,1) is no-op. Lift from
 * src/engine/phase1_sky_tint.h for native Level 1. */
void renderer_set_scene_tint(float r, float g, float b);

/* When enabled, untextured groups within multi-material meshes are skipped
 * instead of rendered as flat-tinted slabs. Used by native Level 1 to hide
 * collision-only volumes (BLOCKING_*) and unresolved cross-level canvas
 * slots (SCHOOL's missing slots etc.) that the original game never drew. */
void renderer_set_hide_untextured_groups(int enable);

/* Phase 4: shader-side alpha cutout. When enabled, the lit fragment shader
 * discards fragments whose sampled texture alpha is below the threshold.
 * Used for capture-derived foliage / playground billboards whose alpha
 * channel was decoded by the proxy from the original D3D7 color-key. */
void renderer_set_alpha_cutout(int enable, float threshold);

/* Phase 5: invert v on subsequent renderer_draw_billboard calls. The capture's
 * FVF152 tree quads use v=0 at local +Y (world-up) and v=1 at local -Y; our
 * billboard VAO uses the OpenGL convention v=0 at the bottom corner. Toggling
 * this flag matches the capture's orientation so leaf-cluster textures
 * (transparent top of image, dense leaves at bottom) render with the leaves
 * at the top of the quad. Stateful — set once before the tree pass, reset
 * after. */
void renderer_set_billboard_uv_flip_y(int enable);

Camera *renderer_camera(void);

/* Copy the current projection*view matrix (computed at begin_frame) into out. */
void renderer_get_view_proj(float out[16]);

/* M7a — matched-camera capture. Install an explicit GL column-major view and
   projection; begin_frame then uses them verbatim instead of deriving the
   camera from the follow cam. Pass NULL to clear the override. */
void renderer_set_camera_override(const float view[16], const float proj[16]);
int  renderer_camera_override_active(void);

#endif
