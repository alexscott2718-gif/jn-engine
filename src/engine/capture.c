#include "capture.h"

#ifdef JN_CAPTURE

#include "renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   /* getpid */

/* ---- OMTC v1 wire protocol (mirror of instrument/proxy/protocol.h) ---- */
#define OMTC_MAGIC    0x434d544fu   /* "OMTC" */
#define OMTC_VERSION  1

#define REC_FRAME_BEGIN     1
#define REC_FRAME_END       2
#define REC_SET_TRANSFORM   3
#define REC_SET_TEXTURE     5
#define REC_TEXTURE_DEF     6
#define REC_DRAW_PRIMITIVE 11

#define XF_WORLD 0
#define XF_VIEW  1
#define XF_PROJ  2

#define FVF_152  0x152u   /* XYZ·NORMAL·DIFFUSE·TEX1, 36-byte vertex */

#define MAX_TEX 512

struct tex_entry {
    unsigned int gl_id;
    int          w, h;
    int          emitted;   /* TEXTURE_DEF written to the current stream */
    char         name[256];
};

static FILE        *g_out;
static FILE        *g_tex_sidecar;
static int          g_active;
static int          g_done;              /* frame budget hit — main loop should exit */
static int          g_frames_left;
static unsigned int g_draw_count;          /* draws in the current frame */
static struct tex_entry g_tex[MAX_TEX];
static int          g_tex_count;

/* M7a camera override loaded from JN_CAPTURE_CAMERA. */
static int          g_have_cam;
static int          g_cam_sw = 1280, g_cam_sh = 720;

/* Read the next non-blank, non-comment line into buf; returns 0 at EOF. */
static int next_cfg_line(FILE *f, char *buf, size_t n) {
    while (fgets(buf, (int)n, f)) {
        char *p = buf;
        while (*p == ' ' || *p == '\t') p++;
        if (*p && *p != '#' && *p != '\n' && *p != '\r')
            return 1;
    }
    return 0;
}

/* Load an M7a camera descriptor (see extract_camera.py). Installs a renderer
   camera override on success. Format: line-based, GL column-major matrices.
     screen <w> <h>
     view   <16 floats>
     proj   <16 floats>
   Other keys (fovy/near/far/eye/residual/...) are informational and skipped. */
static void load_camera_descriptor(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[capture] cannot open camera descriptor %s\n", path);
        return;
    }
    float view[16], proj[16];
    int have_view = 0, have_proj = 0;
    char line[1024];
    while (next_cfg_line(f, line, sizeof(line))) {
        if (strncmp(line, "screen", 6) == 0) {
            sscanf(line + 6, "%d %d", &g_cam_sw, &g_cam_sh);
        } else if (strncmp(line, "view", 4) == 0) {
            float *m = view;
            have_view = (sscanf(line + 4,
                "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                &m[0],&m[1],&m[2],&m[3],&m[4],&m[5],&m[6],&m[7],
                &m[8],&m[9],&m[10],&m[11],&m[12],&m[13],&m[14],&m[15]) == 16);
        } else if (strncmp(line, "proj", 4) == 0) {
            float *m = proj;
            have_proj = (sscanf(line + 4,
                "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                &m[0],&m[1],&m[2],&m[3],&m[4],&m[5],&m[6],&m[7],
                &m[8],&m[9],&m[10],&m[11],&m[12],&m[13],&m[14],&m[15]) == 16);
        }
    }
    fclose(f);
    if (have_view && have_proj) {
        renderer_set_camera_override(view, proj);
        g_have_cam = 1;
        fprintf(stderr, "[capture] camera override from %s (%dx%d)\n",
                path, g_cam_sw, g_cam_sh);
    } else {
        fprintf(stderr, "[capture] %s missing view/proj — override not set\n",
                path);
    }
}

int capture_camera_screen(int *w, int *h) {
    if (!g_have_cam) return 0;
    if (w) *w = g_cam_sw;
    if (h) *h = g_cam_sh;
    return 1;
}

/* ---- little-endian scalar writers ---- */
static void w_u8 (unsigned int v) { fputc((int)(v & 0xff), g_out); }
static void w_u16(unsigned int v) { w_u8(v); w_u8(v >> 8); }
static void w_u32(unsigned int v) { w_u8(v); w_u8(v >> 8); w_u8(v >> 16); w_u8(v >> 24); }
static void w_f32(float f)        { unsigned int u; memcpy(&u, &f, 4); w_u32(u); }

/* Record header: type u8, len_hi u8, len_lo u16 (24-bit payload length). */
static void w_rec_hdr(unsigned int type, unsigned int payload_len) {
    w_u8(type);
    w_u8((payload_len >> 16) & 0xff);
    w_u16(payload_len & 0xffff);
}

/* Transpose a GL column-major Mat4 into the D3D row-major layout the wire
   protocol uses, then write the 16 floats. d[r*4+c] = g[c*4+r]. */
static void w_matrix_transposed(const float g[16]) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            w_f32(g[c * 4 + r]);
}

static struct tex_entry *tex_find(unsigned int gl_id) {
    for (int i = 0; i < g_tex_count; i++)
        if (g_tex[i].gl_id == gl_id) return &g_tex[i];
    return NULL;
}

void capture_note_texture(unsigned int gl_id, const char *name, int w, int h) {
    if (!gl_id) return;
    struct tex_entry *t = tex_find(gl_id);
    if (!t) {
        if (g_tex_count >= MAX_TEX) return;
        t = &g_tex[g_tex_count++];
        t->gl_id = gl_id;
        t->emitted = 0;
    }
    t->w = w;
    t->h = h;
    if (name) {
        strncpy(t->name, name, sizeof(t->name) - 1);
        t->name[sizeof(t->name) - 1] = '\0';
    }
}

void capture_init(void) {
    /* M7a: load the camera descriptor first so the stream header reports the
       matched screen size and the renderer override is live before frame 0.
       This is independent of JN_CAPTURE — a camera override is useful even
       when not recording (e.g. eyeballing a pose). */
    const char *cam = getenv("JN_CAPTURE_CAMERA");
    if (cam && cam[0])
        load_camera_descriptor(cam);

    const char *path = getenv("JN_CAPTURE");
    if (!path || !path[0]) return;

    g_out = fopen(path, "wb");
    if (!g_out) {
        fprintf(stderr, "[capture] cannot open %s for writing\n", path);
        return;
    }

    const char *frames = getenv("JN_CAPTURE_FRAMES");
    g_frames_left = frames && frames[0] ? atoi(frames) : 120;
    if (g_frames_left <= 0) g_frames_left = 120;

    /* Stream header: magic u32, version u16, pid u32, screen_w u16, screen_h u16.
       Screen size matches the override's when a camera descriptor was loaded. */
    w_u32(OMTC_MAGIC);
    w_u16(OMTC_VERSION);
    w_u32((unsigned int)getpid());
    w_u16((unsigned int)g_cam_sw);
    w_u16((unsigned int)g_cam_sh);

    /* Texture-name sidecar: diff.py reads it because the wire TEXTURE_DEF has
       no name field and the demo's SHA-1 would not match the proxy's. */
    char side[512];
    snprintf(side, sizeof(side), "%s.tex", path);
    g_tex_sidecar = fopen(side, "w");

    g_active = 1;
    fprintf(stderr, "[capture] recording %d frames to %s\n", g_frames_left, path);
}

int capture_active(void) { return g_active; }

void capture_begin_frame(unsigned int seq, unsigned int t_ms) {
    if (!g_active) return;
    g_draw_count = 0;
    w_rec_hdr(REC_FRAME_BEGIN, 8);
    w_u32(seq);
    w_u32(t_ms);
}

void capture_set_camera(const float view[16], const float proj[16]) {
    if (!g_active) return;
    w_rec_hdr(REC_SET_TRANSFORM, 65);
    w_u8(XF_VIEW);
    w_matrix_transposed(view);
    w_rec_hdr(REC_SET_TRANSFORM, 65);
    w_u8(XF_PROJ);
    w_matrix_transposed(proj);
}

/* Emit a TEXTURE_DEF once per stream for a bound texture, plus its sidecar
   line. d3dfmt/sha1 are zeroed — the demo has the real name instead. */
static void emit_texture_def(struct tex_entry *t) {
    if (!t || t->emitted) return;
    t->emitted = 1;
    w_rec_hdr(REC_TEXTURE_DEF, 32);
    w_u32(t->gl_id);
    w_u16((unsigned int)t->w);
    w_u16((unsigned int)t->h);
    w_u32(0);                       /* d3dfmt — n/a for the demo */
    for (int i = 0; i < 20; i++) w_u8(0);   /* sha1 — n/a for the demo */
    if (g_tex_sidecar) {
        fprintf(g_tex_sidecar, "%08x %d %d %s\n",
                t->gl_id, t->w, t->h, t->name[0] ? t->name : "(unnamed)");
        fflush(g_tex_sidecar);
    }
}

void capture_draw(const float model[16], unsigned int gl_tex,
                  const float aabb_min[3], const float aabb_max[3]) {
    if (!g_active) return;

    /* WORLD transform for this draw. */
    w_rec_hdr(REC_SET_TRANSFORM, 65);
    w_u8(XF_WORLD);
    w_matrix_transposed(model);

    /* Bound texture (stage 0). Emit its TEXTURE_DEF on first sighting. */
    if (gl_tex) {
        struct tex_entry *t = tex_find(gl_tex);
        if (!t) {                       /* texture never went through tex_load */
            capture_note_texture(gl_tex, "(runtime)", 0, 0);
            t = tex_find(gl_tex);
        }
        emit_texture_def(t);
    }
    w_rec_hdr(REC_SET_TEXTURE, 5);
    w_u8(0);
    w_u32(gl_tex);

    /* DRAW_PRIMITIVE: the 8 local-space AABB corners as FVF-0x152 vertices.
       diff.py reduces every draw to a world-space AABB anyway, so 8 corners
       carry exactly the geometry the derived-fact comparison needs. */
    const unsigned int vtx_count = 8;
    const unsigned int payload = 9 + vtx_count * 36;
    w_rec_hdr(REC_DRAW_PRIMITIVE, payload);
    w_u8(4);                            /* prim_type: D3DPT_TRIANGLELIST */
    w_u32(FVF_152);
    w_u32(vtx_count);
    for (int i = 0; i < 8; i++) {
        float x = (i & 1) ? aabb_max[0] : aabb_min[0];
        float y = (i & 2) ? aabb_max[1] : aabb_min[1];
        float z = (i & 4) ? aabb_max[2] : aabb_min[2];
        w_f32(x); w_f32(y); w_f32(z);   /* pos   */
        w_f32(0); w_f32(0); w_f32(0);   /* normal */
        w_u32(0xffffffffu);             /* diffuse */
        w_f32(0); w_f32(0);             /* uv */
    }
    g_draw_count++;
}

void capture_end_frame(void) {
    if (!g_active) return;
    w_rec_hdr(REC_FRAME_END, 12);
    w_u32(0);                /* seq — receiver pairs with FRAME_BEGIN */
    w_u32(g_draw_count);
    w_u32(0);                /* dropped — the demo never drops */

    if (--g_frames_left <= 0) {
        fprintf(stderr, "[capture] frame budget reached — closing stream\n");
        capture_shutdown();
        g_done = 1;     /* signal main.c to break the loop and run normal teardown */
    }
}

int capture_should_exit(void) { return g_done; }

void capture_shutdown(void) {
    if (!g_active && !g_out) return;
    g_active = 0;
    if (g_out)         { fclose(g_out);         g_out = NULL; }
    if (g_tex_sidecar) { fclose(g_tex_sidecar); g_tex_sidecar = NULL; }
}

#endif  /* JN_CAPTURE */
