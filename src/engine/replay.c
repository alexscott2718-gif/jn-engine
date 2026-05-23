/* Faithful .omtc replay (Phase-12 pivot proof).
 *
 * Consumes a single-frame self-contained capture (extract_frame_capture.py
 * snapshots prelude sticky state + one frame's records) and renders it by
 * translating D3D7 commands to GL. The capture format mirrors the proxy's
 * wire protocol (instrument/proxy/protocol.h, mirrored in
 * instrument/receiver/protocol.py).
 *
 * v0 scope (this file): geometry, transforms, render states (LIGHTING
 * enable, depth, alpha, fog enable), texture binding (white fallback when
 * no pixel payload), per-vertex DIFFUSE modulation, FVF 0x152 vertex layout
 * (XYZ+NORMAL+DIFFUSE+TEX1). Renders one captured frame; the loop re-renders
 * it each tick so the engine's screenshot/window path works unchanged.
 *
 * Convention bridge (D3D7 -> GL):
 *   - D3D row-vector matrices (pos * W*V*P) -> upload combined row-major MVP
 *     with transpose=GL_TRUE; the column-vector shader then evaluates
 *     (MVP)^T * pos which is the equivalent transform.
 *   - D3D Z-clip is [0,1], GL is [-1,1] -> glClipControl ZERO_TO_ONE.
 *   - Triangle winding: D3D default CW; we leave backface culling off so
 *     winding doesn't matter (proxy emits whatever order the game used).
 */

#include "replay.h"
#include "glad.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

/* GL constants not in the project's minimal glad.h (added locally). */
#ifndef GL_NEAREST
#define GL_NEAREST          0x2600
#endif
#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW      0x88E0
#endif
#ifndef GL_POINTS
#define GL_POINTS           0x0000
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP       0x0003
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP   0x0005
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN     0x0006
#endif

/* ---- protocol record types (must match instrument/proxy/protocol.h) ---- */
#define REC_FRAME_BEGIN          1
#define REC_FRAME_END            2
#define REC_SET_TRANSFORM        3
#define REC_VIEWPORT             4
#define REC_SET_TEXTURE          5
#define REC_TEXTURE_DEF          6
#define REC_SET_RENDERSTATE      7
#define REC_SET_TEXSTAGESTATE    8
#define REC_SET_LIGHT            9
#define REC_SET_MATERIAL        10
#define REC_DRAW_PRIMITIVE      11
#define REC_DRAW_INDEXED        12
#define REC_FRAME_MARK          13

#define XF_WORLD                 0
#define XF_VIEW                  1
#define XF_PROJ                  2

/* D3D render states we honor in v0. */
#define D3DRS_ZENABLE            7
#define D3DRS_ZWRITEENABLE      14
#define D3DRS_ALPHATESTENABLE   15
#define D3DRS_FOGENABLE         28
#define D3DRS_FOGCOLOR          34
#define D3DRS_ALPHABLENDENABLE  27
#define D3DRS_LIGHTING         137
#define D3DRS_AMBIENT          139

#define OMTC_MAGIC          0x434d544fu
#define MAX_TEXTURES        1024

/* ---- captured-stream cache ------------------------------------------- */
typedef struct {
    unsigned int tex_id;     /* D3D-side id from the capture */
    unsigned int gl_tex;     /* GL texture (white 1x1 in v0) */
    int          w, h;
} ReplayTex;

static int          g_active;
static char         g_path[512];
static unsigned char *g_buf;
static size_t       g_buf_len;
static size_t       g_record_start;     /* offset of first record (post-header) */

static ReplayTex    g_tex[MAX_TEXTURES];
static int          g_tex_count;
static unsigned int g_white_tex;        /* 1x1 white fallback */

static unsigned int g_prog;
static int          g_loc_mvp, g_loc_tex, g_loc_has_tex;
static unsigned int g_vao, g_vbo;
static int          g_viewport_w, g_viewport_h;

/* ---- GL shader: D3D7 fixed-function emulation (v0: textured + diffuse) - */
static const char *VS =
    "#version 330 core\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec2 aUV;\n"
    "layout(location=2) in vec4 aDiffuse;\n"
    "uniform mat4 uMVP;\n"
    "out vec2 vUV;\n"
    "out vec4 vDiff;\n"
    "void main(){\n"
    "  vec4 p = uMVP * vec4(aPos,1.0);\n"
    "  /* D3D clip-Z is [0,w] (NDC [0,1]); GL expects [-w,w] (NDC [-1,1]).\n"
    "     Remap before passing to the rasterizer so we don't depend on\n"
    "     glClipControl being loadable in every GL driver. */\n"
    "  p.z = p.z * 2.0 - p.w;\n"
    "  gl_Position = p;\n"
    "  vUV = aUV;\n"
    "  vDiff = aDiffuse;\n"
    "}\n";

static const char *FS =
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "in vec4 vDiff;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTex;\n"
    "uniform int  uHasTex;\n"
    "void main(){\n"
    "  vec4 tex = (uHasTex==1) ? texture(uTex, vUV) : vec4(1.0);\n"
    "  /* Modulate RGB by vertex diffuse, but ignore vertex alpha (D3D7 games\n"
    "     commonly leave the DIFFUSE alpha byte = 0; we don't want to discard\n"
    "     the whole scene). Texture alpha is honored. */\n"
    "  FragColor = vec4(tex.rgb * vDiff.rgb, tex.a);\n"
    "}\n";

/* ---- shader compile helper ------------------------------------------- */
static unsigned int compile(GLenum type, const char *src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(s, 1024, NULL, log);
               fprintf(stderr, "[replay] shader: %s\n", log); }
    return s;
}

/* ---- tiny little-endian readers -------------------------------------- */
static uint8_t  r_u8 (const unsigned char *p) { return p[0]; }
static uint16_t r_u16(const unsigned char *p) { return (uint16_t)(p[0] | (p[1]<<8)); }
static uint32_t r_u32(const unsigned char *p) {
    return (uint32_t)(p[0] | (p[1]<<8) | (p[2]<<16) | ((uint32_t)p[3]<<24));
}
static float    r_f32(const unsigned char *p) { float f; memcpy(&f, p, 4); return f; }

/* ---- texture lookup -------------------------------------------------- */
static ReplayTex *find_tex(unsigned int tex_id) {
    for (int i = 0; i < g_tex_count; i++)
        if (g_tex[i].tex_id == tex_id) return &g_tex[i];
    return NULL;
}

static void register_texture(unsigned int tex_id, int w, int h) {
    if (find_tex(tex_id)) return;
    if (g_tex_count >= MAX_TEXTURES) return;
    ReplayTex *t = &g_tex[g_tex_count++];
    t->tex_id = tex_id;
    t->gl_tex = g_white_tex;            /* v0: pixels not yet in stream */
    t->w = w; t->h = h;
}

/* Matrices on the wire are COLUMN-major (column-vector style: clip = M*pos),
   confirmed against diff.py's xform_point. Multiplication and upload follow
   that. (A*B)_flat[c*4+r] = sum_k A_flat[k*4+r] * B_flat[c*4+k]. */
static void mat4_mul_col(float out[16], const float a[16], const float b[16]) {
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) s += a[k*4+r] * b[c*4+k];
            out[c*4+r] = s;
        }
}

static void mat4_identity(float m[16]) {
    memset(m, 0, 64);
    m[0]=m[5]=m[10]=m[15]=1.0f;
}

/* ---- public api ------------------------------------------------------ */
int replay_active(void) {
    const char *p = getenv("JN_REPLAY");
    return (p && p[0]) ? 1 : 0;
}

int replay_init(int viewport_w, int viewport_h) {
    const char *path = getenv("JN_REPLAY");
    if (!path || !path[0]) return 0;
    snprintf(g_path, sizeof(g_path), "%s", path);
    g_viewport_w = viewport_w;
    g_viewport_h = viewport_h;

    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "[replay] cannot open %s\n", path); return 0; }
    fseek(f, 0, SEEK_END); g_buf_len = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    g_buf = (unsigned char *)malloc(g_buf_len);
    if (!g_buf) { fclose(f); return 0; }
    if (fread(g_buf, 1, g_buf_len, f) != g_buf_len) {
        fprintf(stderr, "[replay] short read\n"); fclose(f); return 0;
    }
    fclose(f);
    /* Validate header (14 bytes): magic u32 OMTC, version u16, pid u32,
       screen_w u16, screen_h u16. */
    if (g_buf_len < 14 || r_u32(g_buf) != OMTC_MAGIC) {
        fprintf(stderr, "[replay] bad header magic\n"); return 0;
    }
    g_record_start = 14;

    /* GL setup */
    unsigned int vs = compile(GL_VERTEX_SHADER, VS);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, FS);
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs); glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    int ok; glGetProgramiv(g_prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[1024]; glGetProgramInfoLog(g_prog, 1024, NULL, log);
               fprintf(stderr, "[replay] link: %s\n", log); return 0; }
    glDeleteShader(vs); glDeleteShader(fs);
    g_loc_mvp     = glGetUniformLocation(g_prog, "uMVP");
    g_loc_tex     = glGetUniformLocation(g_prog, "uTex");
    g_loc_has_tex = glGetUniformLocation(g_prog, "uHasTex");

    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);
    /* layout: pos vec3 (off 0), uv vec2 (off 24+4+? -- compact 9-float layout
       per vertex: x,y,z, u,v, dr,dg,db,da). We transcode FVF on CPU into
       this compact 9-float vertex (36 bytes), matching FVF 0x152's size. */
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 9*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 9*sizeof(float), (void*)(5*sizeof(float)));
    glBindVertexArray(0);

    /* 1x1 white fallback texture for untextured/no-pixel binds. */
    glGenTextures(1, &g_white_tex);
    glBindTexture(GL_TEXTURE_2D, g_white_tex);
    unsigned int white = 0xffffffffu;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    /* (Z remap handled in vertex shader; no glClipControl dependency.) */
    g_active = 1;
    fprintf(stderr, "[replay] loaded %s (%zu B); ready to render frame.\n",
            path, g_buf_len);
    return 1;
}

/* ---- per-frame replay ------------------------------------------------ */
void replay_render_frame(void) {
    if (!g_active) return;

    /* Sticky D3D state across the replay (we walk the entire buffer each
       frame -- one-frame capture, cheap). */
    float WORLD[16], VIEW[16], PROJ[16];
    mat4_identity(WORLD); mat4_identity(VIEW); mat4_identity(PROJ);
    int lighting_on = 0;
    unsigned int cur_tex_id = 0;

    glViewport(0, 0, g_viewport_w, g_viewport_h);
    glClearColor(0.45f, 0.70f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(g_prog);
    glUniform1i(g_loc_tex, 0);
    glActiveTexture(GL_TEXTURE0);

    size_t off = g_record_start;
    int    draws = 0;
    while (off + 4 <= g_buf_len) {
        unsigned int type   = r_u8 (g_buf + off);
        unsigned int len_hi = r_u8 (g_buf + off + 1);
        unsigned int len_lo = r_u16(g_buf + off + 2);
        unsigned int plen   = (len_hi << 16) | len_lo;
        off += 4;
        if (off + plen > g_buf_len) break;
        const unsigned char *p = g_buf + off;
        off += plen;

        switch (type) {
        case REC_FRAME_BEGIN:
        case REC_FRAME_END:
        case REC_FRAME_MARK:
        case REC_VIEWPORT:
        case REC_SET_TEXSTAGESTATE:
        case REC_SET_LIGHT:
        case REC_SET_MATERIAL:
        case REC_DRAW_INDEXED:
            /* v0: ignored. Frame markers don't gate (single-frame capture). */
            break;

        case REC_SET_TRANSFORM: {
            if (plen < 65) break;
            unsigned int which = p[0];
            const unsigned char *m = p + 1;
            float *dst = (which == XF_WORLD) ? WORLD :
                         (which == XF_VIEW)  ? VIEW  :
                         (which == XF_PROJ)  ? PROJ  : NULL;
            if (!dst) break;
            for (int i = 0; i < 16; i++) dst[i] = r_f32(m + i*4);
            break;
        }
        case REC_TEXTURE_DEF: {
            if (plen < 12) break;
            unsigned int tid = r_u32(p);
            int w = r_u16(p + 4);
            int h = r_u16(p + 6);
            register_texture(tid, w, h);
            break;
        }
        case REC_SET_TEXTURE: {
            if (plen < 5) break;
            unsigned int stage = p[0];
            if (stage != 0) break;
            cur_tex_id = r_u32(p + 1);
            break;
        }
        case REC_SET_RENDERSTATE: {
            if (plen < 8) break;
            unsigned int state = r_u32(p);
            unsigned int value = r_u32(p + 4);
            switch (state) {
            case D3DRS_LIGHTING:       lighting_on = (int)value; break;
            case D3DRS_ZENABLE:        if (value) glEnable(GL_DEPTH_TEST);
                                       else       glDisable(GL_DEPTH_TEST);
                                       break;
            case D3DRS_ZWRITEENABLE:   glDepthMask(value ? GL_TRUE : GL_FALSE); break;
            case D3DRS_ALPHABLENDENABLE: if (value) glEnable(GL_BLEND);
                                         else       glDisable(GL_BLEND);
                                         break;
            default: break;       /* others tracked-only in v0 */
            }
            break;
        }

        case REC_DRAW_PRIMITIVE: {
            if (plen < 9) break;
            unsigned int prim_type = p[0];
            unsigned int fvf       = r_u32(p + 1);
            unsigned int vtx_count = r_u32(p + 5);
            if (fvf != 0x152) break;          /* v0: only the known FVF */
            if (vtx_count == 0) break;
            const unsigned char *v = p + 9;
            if (9 + vtx_count * 36 > plen) break;

            /* Transcode FVF 0x152 -> compact 9-float vertex layout. */
            float *verts = (float *)malloc(vtx_count * 9 * sizeof(float));
            if (!verts) break;
            for (unsigned int i = 0; i < vtx_count; i++) {
                const unsigned char *vb = v + i * 36;
                verts[i*9+0] = r_f32(vb + 0);
                verts[i*9+1] = r_f32(vb + 4);
                verts[i*9+2] = r_f32(vb + 8);
                /* skip 12 bytes normal */
                unsigned int dargb = r_u32(vb + 24);
                /* D3DCOLOR is 0xAARRGGBB; per-vertex DIFFUSE in shader (RGBA). */
                verts[i*9+5] = ((dargb >> 16) & 0xff) / 255.0f;  /* R */
                verts[i*9+6] = ((dargb >>  8) & 0xff) / 255.0f;  /* G */
                verts[i*9+7] = ( dargb        & 0xff) / 255.0f;  /* B */
                verts[i*9+8] = ((dargb >> 24) & 0xff) / 255.0f;  /* A */
                verts[i*9+3] = r_f32(vb + 28);                    /* U */
                verts[i*9+4] = r_f32(vb + 32);                    /* V */
            }

            /* MVP = PROJ * VIEW * WORLD (column-vector); matrices are
               column-major on the wire. Upload with transpose=GL_FALSE. */
            float PV[16], MVP[16];
            mat4_mul_col(PV, PROJ, VIEW);
            mat4_mul_col(MVP, PV, WORLD);
            /* (matrix pipeline validated -- see docs/replay_v0_findings.md) */

            ReplayTex *tt = find_tex(cur_tex_id);
            glBindTexture(GL_TEXTURE_2D, tt ? tt->gl_tex : g_white_tex);
            glUniform1i(g_loc_has_tex, 1);
            glUniformMatrix4fv(g_loc_mvp, 1, GL_FALSE, MVP);

            glBindVertexArray(g_vao);
            glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
            glBufferData(GL_ARRAY_BUFFER, vtx_count * 9 * sizeof(float),
                         verts, GL_STREAM_DRAW);

            GLenum mode = GL_TRIANGLES;
            switch (prim_type) {
            case 1: mode = GL_POINTS; break;
            case 2: mode = GL_LINES; break;
            case 3: mode = GL_LINE_STRIP; break;
            case 4: mode = GL_TRIANGLES; break;
            case 5: mode = GL_TRIANGLE_STRIP; break;
            case 6: mode = GL_TRIANGLE_FAN; break;
            }
            glDrawArrays(mode, 0, (GLsizei)vtx_count);
            glBindVertexArray(0);
            free(verts);
            draws++;
            (void)lighting_on;       /* v0: unused; flat textured */
            break;
        }
        default: break;
        }
    }
    static int reported = 0;
    if (!reported) {
        fprintf(stderr, "[replay] frame: issued %d GL draws, "
                "registered %d textures\n", draws, g_tex_count);
        reported = 1;
    }
}

void replay_destroy(void) {
    if (g_prog) { glDeleteProgram(g_prog); g_prog = 0; }
    if (g_vao)  { glDeleteVertexArrays(1, &g_vao); g_vao = 0; }
    if (g_vbo)  { glDeleteBuffers(1, &g_vbo); g_vbo = 0; }
    if (g_white_tex) { glDeleteTextures(1, &g_white_tex); g_white_tex = 0; }
    if (g_buf) { free(g_buf); g_buf = NULL; }
    g_active = 0;
}
