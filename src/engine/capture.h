#ifndef CAPTURE_H
#define CAPTURE_H

/* Demo-side render-stream capture (M6).
 *
 * Emits the *same* OMTC v1 wire protocol as the OMT2 proxy
 * (instrument/proxy/protocol.h), so the original game and jn-engine produce
 * directly comparable .omtc files for diff/diff.py.
 *
 * Compiled in only under -DJN_CAPTURE (Makefile target `make capture`).
 * When the define is absent every entry point degrades to a no-op so the
 * call sites in renderer.c / ground.c / tex_loader.c / main.c need no #ifdef.
 *
 * Runtime control (only meaningful in a -DJN_CAPTURE build):
 *   JN_CAPTURE         output .omtc path; capture is OFF unless this is set
 *   JN_CAPTURE_FRAMES  number of frames to record then auto-stop (default 120)
 *   JN_CAPTURE_CAMERA  M7a camera descriptor file; when set, capture_init
 *                      installs a renderer camera override so the demo renders
 *                      from a camera extracted from an original capture.
 *
 * Matrices passed in are jn-engine's GL column-major Mat4; capture.c
 * transposes them to the D3D row-major layout the wire protocol uses, so a
 * single transform convention covers both engines in diff.py.
 */

#ifdef JN_CAPTURE

void capture_init(void);
int  capture_active(void);
/* Returns 1 once the JN_CAPTURE_FRAMES budget has been hit (capture stream
   already closed). main.c polls this after capture_end_frame() to break its
   loop and run the normal shutdown path. */
int  capture_should_exit(void);
/* If JN_CAPTURE_CAMERA loaded a descriptor with screen dimensions, copy them
   into the w and h out-params and return 1; otherwise return 0. */
int  capture_camera_screen(int *w, int *h);
void capture_begin_frame(unsigned int seq, unsigned int t_ms);
void capture_set_camera(const float view[16], const float proj[16]);
void capture_note_texture(unsigned int gl_id, const char *name, int w, int h);
void capture_draw(const float model[16], unsigned int gl_tex,
                  const float aabb_min[3], const float aabb_max[3]);
void capture_end_frame(void);
void capture_shutdown(void);

#else  /* !JN_CAPTURE — no-op stubs */

/* No-op stubs. Each consumes its arguments via (void) casts so that capture-
   only locals at the call sites do not trip -Wunused in a non-capture build. */
#define capture_init()                ((void)0)
#define capture_active()              (0)
#define capture_should_exit()         (0)
#define capture_camera_screen(w, h)   ((void)(w), (void)(h), 0)
#define capture_begin_frame(s, t)     ((void)(s), (void)(t))
#define capture_set_camera(v, p)      ((void)(v), (void)(p))
#define capture_note_texture(i, n, w, h) \
        ((void)(i), (void)(n), (void)(w), (void)(h))
#define capture_draw(m, t, lo, hi) \
        ((void)(m), (void)(t), (void)(lo), (void)(hi))
#define capture_end_frame()           ((void)0)
#define capture_shutdown()            ((void)0)

#endif  /* JN_CAPTURE */

#endif  /* CAPTURE_H */
