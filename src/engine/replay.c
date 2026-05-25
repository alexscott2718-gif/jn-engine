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
#include "assets/tex_loader.h"   /* tex_load() — stb_image-backed PNG loader */
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
#ifndef GL_NEAREST_MIPMAP_NEAREST
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#endif
#ifndef GL_LINEAR_MIPMAP_NEAREST
#define GL_LINEAR_MIPMAP_NEAREST  0x2701
#endif
#ifndef GL_NEAREST_MIPMAP_LINEAR
#define GL_NEAREST_MIPMAP_LINEAR  0x2702
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
#ifndef GL_BGRA
#define GL_BGRA             0x80E1
#endif
#ifndef GL_BGR
#define GL_BGR              0x80E0
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S   0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T   0x2803
#endif
#ifndef GL_REPEAT
#define GL_REPEAT           0x2901
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE    0x812F
#endif
#ifndef GL_MIRRORED_REPEAT
#define GL_MIRRORED_REPEAT  0x8370
#endif
#ifndef GL_ZERO
#define GL_ZERO             0
#endif
#ifndef GL_ONE
#define GL_ONE              1
#endif
#ifndef GL_SRC_COLOR
#define GL_SRC_COLOR        0x0300
#endif
#ifndef GL_ONE_MINUS_SRC_COLOR
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#endif
#ifndef GL_DST_ALPHA
#define GL_DST_ALPHA        0x0304
#endif
#ifndef GL_ONE_MINUS_DST_ALPHA
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#endif
#ifndef GL_DST_COLOR
#define GL_DST_COLOR        0x0306
#endif
#ifndef GL_ONE_MINUS_DST_COLOR
#define GL_ONE_MINUS_DST_COLOR 0x0307
#endif
#ifndef GL_SRC_ALPHA_SATURATE
#define GL_SRC_ALPHA_SATURATE 0x0308
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
#define REC_TEXTURE_PIXELS      14   /* v3: raw locked-surface bytes */
#define REC_TEXTURE_FORMAT      15   /* v4: DDPIXELFORMAT masks */
#define REC_TEXTURE_COLORKEY    16   /* v4: DirectDraw color key */

#define XF_WORLD                 0
#define XF_VIEW                  1
#define XF_PROJ                  2

/* D3D render states we honor in v0. */
#define D3DRS_ZENABLE            7
#define D3DRS_ZWRITEENABLE      14
#define D3DRS_ALPHATESTENABLE   15
#define D3DRS_SRCBLEND          19
#define D3DRS_DESTBLEND         20
#define D3DRS_FOGENABLE         28
#define D3DRS_FOGCOLOR          34
#define D3DRS_ALPHABLENDENABLE  27
#define D3DRS_LIGHTING         137
#define D3DRS_AMBIENT          139

#define D3DBLEND_ZERO            1
#define D3DBLEND_ONE             2
#define D3DBLEND_SRCCOLOR        3
#define D3DBLEND_INVSRCCOLOR     4
#define D3DBLEND_SRCALPHA        5
#define D3DBLEND_INVSRCALPHA     6
#define D3DBLEND_DESTALPHA       7
#define D3DBLEND_INVDESTALPHA    8
#define D3DBLEND_DESTCOLOR       9
#define D3DBLEND_INVDESTCOLOR   10
#define D3DBLEND_SRCALPHASAT    11

#define D3DTSS_ADDRESS          12
#define D3DTSS_ADDRESSU         13
#define D3DTSS_ADDRESSV         14
#define D3DTSS_MAGFILTER        16
#define D3DTSS_MINFILTER        17
#define D3DTSS_MIPFILTER        18

#define D3DTADDRESS_WRAP         1
#define D3DTADDRESS_MIRROR       2
#define D3DTADDRESS_CLAMP        3
#define D3DTADDRESS_BORDER       4
#define D3DTADDRESS_MIRRORONCE   5

#define D3DTFG_POINT             1
#define D3DTFG_LINEAR            2
#define D3DTFN_POINT             1
#define D3DTFN_LINEAR            2
#define D3DTFP_NONE              1
#define D3DTFP_POINT             2
#define D3DTFP_LINEAR            3

#define DDPF_ALPHAPIXELS         0x00000001u
#define DDCKEY_SRCBLT            0x00000008u
#define DDCKEY_SRCOVERLAY        0x00000010u

#define OMTC_MAGIC          0x434d544fu
#define MAX_TEXTURES        1024

/* ---- captured-stream cache ------------------------------------------- */
typedef struct {
    int          valid;
    unsigned int flags;
    unsigned int rgb_bit_count;
    unsigned int r_mask;
    unsigned int g_mask;
    unsigned int b_mask;
    unsigned int a_mask;
} ReplayPixelFormat;

typedef struct {
    int          known;
    int          active;
    unsigned int flags;
    unsigned int low;
    unsigned int high;
} ReplayColorKey;

#define MAX_COLORKEYS_PER_TEX 4

typedef struct {
    unsigned int tex_id;     /* D3D-side id from the capture */
    unsigned int gl_tex;     /* GL texture (white 1x1 in v0) */
    int          w, h;
    int          has_zero_alpha;
    ReplayPixelFormat fmt;
    ReplayColorKey colorkey[MAX_COLORKEYS_PER_TEX];
} ReplayTex;

typedef struct {
    unsigned int address_u;
    unsigned int address_v;
    unsigned int mag_filter;
    unsigned int min_filter;
    unsigned int mip_filter;
} ReplayTexStage;

typedef struct {
    float diffuse[4];
    float ambient[4];
    float specular[4];
    float emissive[4];
    float power;
} ReplayMaterial;

static int          g_active;
static char         g_path[512];
static unsigned char *g_buf;
static size_t       g_buf_len;
static size_t       g_record_start;     /* offset of first record (post-header) */

static ReplayTex    g_tex[MAX_TEXTURES];
static int          g_tex_count;
static unsigned int g_white_tex;        /* 1x1 white fallback */

/* Optional sidecar: tex_id (hex) -> PNG path. Loaded if JN_REPLAY_TEX_MAP is
   set. Built by instrument/diff/build_replay_texmap.py (heuristic dim-based
   pairing -- partial fidelity until the proxy carries pixel payloads). */
#define MAX_TEXMAP_ENTRIES 512
typedef struct { unsigned int tex_id; char path[256]; } TexMapEntry;
static TexMapEntry g_texmap[MAX_TEXMAP_ENTRIES];
static int         g_texmap_count;

static const char *texmap_lookup(unsigned int tex_id) {
    for (int i = 0; i < g_texmap_count; i++)
        if (g_texmap[i].tex_id == tex_id) return g_texmap[i].path;
    return NULL;
}

static void load_texmap(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[replay] tex-map %s not found -- white fallbacks only\n", path);
        return;
    }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned int tid;
        char p[256];
        /* Format: <hex>\t<path>\n */
        if (sscanf(line, "%x\t%255[^\n]", &tid, p) != 2) continue;
        if (g_texmap_count >= MAX_TEXMAP_ENTRIES) break;
        g_texmap[g_texmap_count].tex_id = tid;
        snprintf(g_texmap[g_texmap_count].path, sizeof(g_texmap[0].path), "%s", p);
        g_texmap_count++;
    }
    fclose(f);
    fprintf(stderr, "[replay] loaded %d entries from %s\n", g_texmap_count, path);
}

static unsigned int g_prog;
static int          g_loc_mvp, g_loc_tex, g_loc_has_tex, g_loc_use_alpha;
static int          g_loc_alpha_discard, g_loc_debug_mode, g_loc_debug_color;
static int          g_loc_lighting, g_loc_lit_color;
static unsigned int g_vao, g_vbo;
static int          g_viewport_w, g_viewport_h;
static int          g_dbg_draw_start = 1;
static int          g_dbg_draw_end = 0x7fffffff;
static int          g_dbg_only_tex_enabled;
static unsigned int g_dbg_only_tex;
static int          g_dbg_highlight_tex_enabled;
static unsigned int g_dbg_highlight_tex;
static int          g_dbg_disable_blend;
static int          g_dbg_flat_groups;

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
    "uniform int  uUseAlpha;\n"
    "uniform int  uAlphaDiscard;\n"
    "uniform int  uLighting;\n"
    "uniform vec4 uLitColor;\n"
    "uniform int  uDebugMode;\n"
    "uniform vec4 uDebugColor;\n"
    "void main(){\n"
    "  if (uDebugMode==1) { FragColor = uDebugColor; return; }\n"
    "  vec4 tex = (uHasTex==1) ? texture(uTex, vUV) : vec4(1.0);\n"
    "  if (uAlphaDiscard==1 && tex.a <= 0.001) discard;\n"
    "  vec4 factor = (uLighting==1) ? uLitColor : vDiff;\n"
    "  vec4 color = tex * factor;\n"
    "  FragColor = vec4(color.rgb, (uUseAlpha==1) ? color.a : 1.0);\n"
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
    memset(t, 0, sizeof(*t));
    t->tex_id = tex_id;
    t->w = w; t->h = h;
    /* If the sidecar maps this tex_id to a local PNG, load real pixels via
       the engine's stb_image-backed tex_load(); otherwise fall back to
       white. Failures (bad path / decode error) also fall back. */
    const char *png = texmap_lookup(tex_id);
    if (png) {
        unsigned int gl = tex_load(png);
        t->gl_tex = gl ? gl : g_white_tex;
    } else {
        t->gl_tex = g_white_tex;
    }
}

static ReplayColorKey *find_or_add_colorkey(ReplayTex *t, unsigned int flags) {
    for (int i = 0; i < MAX_COLORKEYS_PER_TEX; i++)
        if (t->colorkey[i].known && t->colorkey[i].flags == flags)
            return &t->colorkey[i];
    for (int i = 0; i < MAX_COLORKEYS_PER_TEX; i++)
        if (!t->colorkey[i].known) {
            t->colorkey[i].known = 1;
            t->colorkey[i].flags = flags;
            return &t->colorkey[i];
        }
    return NULL;
}

static int mask_shift(unsigned int mask) {
    int s = 0;
    if (!mask) return 0;
    while ((mask & 1u) == 0u) {
        mask >>= 1;
        s++;
    }
    return s;
}

static int mask_bits(unsigned int mask) {
    int n = 0;
    if (!mask) return 0;
    mask >>= mask_shift(mask);
    while (mask & 1u) {
        n++;
        mask >>= 1;
    }
    return n;
}

static unsigned char expand_masked_8(unsigned int value, unsigned int mask) {
    int bits = mask_bits(mask);
    if (bits <= 0) return 0;
    unsigned int raw = (value & mask) >> mask_shift(mask);
    unsigned int maxv = (1u << bits) - 1u;
    return (unsigned char)((raw * 255u + maxv / 2u) / maxv);
}

static unsigned int read_packed_pixel(const unsigned char *p, int bytes) {
    unsigned int v = 0;
    for (int i = 0; i < bytes && i < 4; i++)
        v |= ((unsigned int)p[i]) << (i * 8);
    return v;
}

static int colorkey_applies(const ReplayColorKey *ck) {
    if (!ck->known || !ck->active)
        return 0;
    return ((ck->flags & (DDCKEY_SRCBLT | DDCKEY_SRCOVERLAY)) != 0u) ||
           ck->flags == 0u;
}

static int texel_is_keyed(const ReplayTex *t, unsigned int encoded) {
    unsigned int rgb_mask = t->fmt.r_mask | t->fmt.g_mask | t->fmt.b_mask;
    unsigned int value = rgb_mask ? (encoded & rgb_mask) : encoded;
    for (int i = 0; i < MAX_COLORKEYS_PER_TEX; i++) {
        const ReplayColorKey *ck = &t->colorkey[i];
        unsigned int low = rgb_mask ? (ck->low & rgb_mask) : ck->low;
        unsigned int high = rgb_mask ? (ck->high & rgb_mask) : ck->high;
        if (colorkey_applies(ck) && value >= low && value <= high)
            return 1;
    }
    return 0;
}

static unsigned char *convert_masked_pixels(const ReplayTex *t,
                                            const unsigned char *pix,
                                            int w, int h,
                                            unsigned int bpp,
                                            unsigned int nbytes,
                                            int *has_zero_alpha) {
    if (!t->fmt.valid)
        return NULL;
    int bytes = (int)((bpp + 7u) / 8u);
    if (bytes <= 0 || bytes > 4)
        return NULL;
    unsigned int need = (unsigned int)(w * h * bytes);
    if (need > nbytes)
        return NULL;

    unsigned char *rgba = (unsigned char *)malloc((size_t)w * (size_t)h * 4u);
    if (!rgba)
        return NULL;
    int has_alpha = (t->fmt.flags & DDPF_ALPHAPIXELS) && t->fmt.a_mask;
    if (has_zero_alpha)
        *has_zero_alpha = 0;
    for (int i = 0; i < w * h; i++) {
        unsigned int encoded = read_packed_pixel(pix + i * bytes, bytes);
        rgba[i*4+0] = expand_masked_8(encoded, t->fmt.r_mask);
        rgba[i*4+1] = expand_masked_8(encoded, t->fmt.g_mask);
        rgba[i*4+2] = expand_masked_8(encoded, t->fmt.b_mask);
        rgba[i*4+3] = has_alpha ?
            expand_masked_8(encoded, t->fmt.a_mask) : 255u;
        if (has_zero_alpha && rgba[i*4+3] == 0)
            *has_zero_alpha = 1;
        if (texel_is_keyed(t, encoded)) {
            rgba[i*4+3] = 0;
            if (has_zero_alpha)
                *has_zero_alpha = 1;
        }
    }
    return rgba;
}

static void texstage_init(ReplayTexStage *ts) {
    ts->address_u = D3DTADDRESS_WRAP;
    ts->address_v = D3DTADDRESS_WRAP;
    ts->mag_filter = D3DTFG_LINEAR;
    ts->min_filter = D3DTFN_LINEAR;
    ts->mip_filter = D3DTFP_NONE;
}

static GLenum d3d_address_to_gl(unsigned int value) {
    switch (value) {
    case D3DTADDRESS_WRAP:   return GL_REPEAT;
    case D3DTADDRESS_MIRROR: return GL_MIRRORED_REPEAT;
    case D3DTADDRESS_CLAMP:  return GL_CLAMP_TO_EDGE;
    case D3DTADDRESS_BORDER: return GL_CLAMP_TO_EDGE;
    case D3DTADDRESS_MIRRORONCE:
        return GL_CLAMP_TO_EDGE;
    default: return GL_REPEAT;
    }
}

static GLenum d3d_mag_filter_to_gl(unsigned int value) {
    switch (value) {
    case D3DTFG_POINT:  return GL_NEAREST;
    case D3DTFG_LINEAR: return GL_LINEAR;
    default: return GL_LINEAR;
    }
}

static GLenum d3d_min_filter_to_gl(const ReplayTexStage *ts) {
    int linear_min = (ts->min_filter == D3DTFN_LINEAR);
    switch (ts->mip_filter) {
    case D3DTFP_POINT:
        return linear_min ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
    case D3DTFP_LINEAR:
        return linear_min ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
    default:
        return linear_min ? GL_LINEAR : GL_NEAREST;
    }
}

static void apply_texstage(const ReplayTexStage *ts, unsigned int gl_tex) {
    glBindTexture(GL_TEXTURE_2D, gl_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    d3d_address_to_gl(ts->address_u));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    d3d_address_to_gl(ts->address_v));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    d3d_mag_filter_to_gl(ts->mag_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    d3d_min_filter_to_gl(ts));
}

static GLenum d3d_blend_to_gl(unsigned int value) {
    switch (value) {
    case D3DBLEND_ZERO:         return GL_ZERO;
    case D3DBLEND_ONE:          return GL_ONE;
    case D3DBLEND_SRCCOLOR:     return GL_SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR:  return GL_ONE_MINUS_SRC_COLOR;
    case D3DBLEND_SRCALPHA:     return GL_SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA:  return GL_ONE_MINUS_SRC_ALPHA;
    case D3DBLEND_DESTALPHA:    return GL_DST_ALPHA;
    case D3DBLEND_INVDESTALPHA: return GL_ONE_MINUS_DST_ALPHA;
    case D3DBLEND_DESTCOLOR:    return GL_DST_COLOR;
    case D3DBLEND_INVDESTCOLOR: return GL_ONE_MINUS_DST_COLOR;
    case D3DBLEND_SRCALPHASAT:  return GL_SRC_ALPHA_SATURATE;
    default:                    return GL_ONE;
    }
}

static void apply_blend_state(int alpha_blend, unsigned int src, unsigned int dst) {
    if (!alpha_blend) {
        glDisable(GL_BLEND);
        return;
    }
    glEnable(GL_BLEND);
    glBlendFuncSeparate(d3d_blend_to_gl(src), d3d_blend_to_gl(dst),
                        GL_ZERO, GL_ONE);
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

static void material_default(ReplayMaterial *m) {
    memset(m, 0, sizeof(*m));
    m->diffuse[0] = m->diffuse[1] = m->diffuse[2] = m->diffuse[3] = 1.0f;
}

static void material_read(ReplayMaterial *m, const unsigned char *p) {
    for (int i = 0; i < 4; i++) m->diffuse[i] = r_f32(p + i*4);
    for (int i = 0; i < 4; i++) m->ambient[i] = r_f32(p + 16 + i*4);
    for (int i = 0; i < 4; i++) m->specular[i] = r_f32(p + 32 + i*4);
    for (int i = 0; i < 4; i++) m->emissive[i] = r_f32(p + 48 + i*4);
    m->power = r_f32(p + 64);
}

static int parse_u32_env(const char *name, unsigned int *out) {
    const char *s = getenv(name);
    if (!s || !s[0]) return 0;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 0);
    if (end == s) return 0;
    *out = (unsigned int)v;
    return 1;
}

static int parse_i32_env(const char *name, int fallback) {
    const char *s = getenv(name);
    if (!s || !s[0]) return fallback;
    char *end = NULL;
    long v = strtol(s, &end, 0);
    if (end == s) return fallback;
    if (v < 1) v = 1;
    if (v > 0x7fffffffL) v = 0x7fffffffL;
    return (int)v;
}

static int env_enabled(const char *name) {
    const char *s = getenv(name);
    return (s && s[0] && strcmp(s, "0") != 0) ? 1 : 0;
}

static void debug_color_for_tex(unsigned int tex_id, float out[4]) {
    unsigned int x = tex_id ? tex_id : 0x9e3779b9u;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    out[0] = 0.25f + ((x >>  0) & 0xff) / 340.0f;
    out[1] = 0.25f + ((x >>  8) & 0xff) / 340.0f;
    out[2] = 0.25f + ((x >> 16) & 0xff) / 340.0f;
    out[3] = 1.0f;
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
    g_loc_use_alpha = glGetUniformLocation(g_prog, "uUseAlpha");
    g_loc_alpha_discard = glGetUniformLocation(g_prog, "uAlphaDiscard");
    g_loc_debug_mode = glGetUniformLocation(g_prog, "uDebugMode");
    g_loc_debug_color = glGetUniformLocation(g_prog, "uDebugColor");
    g_loc_lighting = glGetUniformLocation(g_prog, "uLighting");
    g_loc_lit_color = glGetUniformLocation(g_prog, "uLitColor");

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
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    /* (Z remap handled in vertex shader; no glClipControl dependency.) */

    /* Optional texture sidecar (JN_REPLAY_TEX_MAP=<path.txt>) maps captured
       tex_ids to local PNGs so replay renders with real textures instead of
       white silhouettes. Heuristic dim-based pairing; partial fidelity until
       the proxy carries pixel payloads. */
    const char *tm = getenv("JN_REPLAY_TEX_MAP");
    if (tm && tm[0]) load_texmap(tm);

    g_dbg_draw_start = parse_i32_env("JN_REPLAY_DRAW_START", 1);
    g_dbg_draw_end = parse_i32_env("JN_REPLAY_DRAW_END", 0x7fffffff);
    g_dbg_only_tex_enabled =
        parse_u32_env("JN_REPLAY_ONLY_TEX", &g_dbg_only_tex);
    g_dbg_highlight_tex_enabled =
        parse_u32_env("JN_REPLAY_HIGHLIGHT_TEX", &g_dbg_highlight_tex);
    g_dbg_disable_blend = env_enabled("JN_REPLAY_DISABLE_BLEND");
    g_dbg_flat_groups = env_enabled("JN_REPLAY_FLAT_GROUPS");
    if (g_dbg_draw_start > g_dbg_draw_end) {
        int tmp = g_dbg_draw_start;
        g_dbg_draw_start = g_dbg_draw_end;
        g_dbg_draw_end = tmp;
    }
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
    int alpha_blend = 0;
    unsigned int src_blend = D3DBLEND_ONE;
    unsigned int dst_blend = D3DBLEND_ZERO;
    ReplayTexStage texstage;
    ReplayMaterial material;
    texstage_init(&texstage);
    material_default(&material);

    glViewport(0, 0, g_viewport_w, g_viewport_h);
    glClearColor(0.45f, 0.70f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    apply_blend_state(g_dbg_disable_blend ? 0 : alpha_blend,
                      src_blend, dst_blend);

    glUseProgram(g_prog);
    glUniform1i(g_loc_tex, 0);
    glUniform1i(g_loc_debug_mode, 0);
    glUniform1i(g_loc_lighting, 0);
    glActiveTexture(GL_TEXTURE0);

    size_t off = g_record_start;
    int    draws = 0;
    int    issued_draws = 0;
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
        case REC_SET_LIGHT:
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
        case REC_TEXTURE_FORMAT: {
            if (plen < 28) break;
            unsigned int tid = r_u32(p);
            ReplayTex *tt = find_tex(tid);
            if (!tt) { register_texture(tid, 0, 0); tt = find_tex(tid); }
            if (!tt) break;
            tt->fmt.valid = 1;
            tt->fmt.flags = r_u32(p + 4);
            tt->fmt.rgb_bit_count = r_u32(p + 8);
            tt->fmt.r_mask = r_u32(p + 12);
            tt->fmt.g_mask = r_u32(p + 16);
            tt->fmt.b_mask = r_u32(p + 20);
            tt->fmt.a_mask = r_u32(p + 24);
            break;
        }
        case REC_TEXTURE_COLORKEY: {
            if (plen < 20) break;
            unsigned int tid = r_u32(p);
            ReplayTex *tt = find_tex(tid);
            if (!tt) { register_texture(tid, 0, 0); tt = find_tex(tid); }
            if (!tt) break;
            ReplayColorKey *ck = find_or_add_colorkey(tt, r_u32(p + 4));
            if (!ck) break;
            ck->low = r_u32(p + 8);
            ck->high = r_u32(p + 12);
            ck->active = r_u32(p + 16) ? 1 : 0;
            break;
        }
        case REC_TEXTURE_PIXELS: {
            /* v3 payload: tex_id u32, w u16, h u16, bpp u32, pixel_bytes u32,
               then `pixel_bytes` of packed raw surface bytes (no padding).
               Upload as a GL texture, overriding whatever register_texture
               assigned (white fallback / sidecar PNG). */
            if (plen < 16) break;
            unsigned int tid  = r_u32(p);
            int          w    = r_u16(p + 4);
            int          h    = r_u16(p + 6);
            unsigned int bpp  = r_u32(p + 8);
            unsigned int nbytes = r_u32(p + 12);
            if (16 + nbytes > plen || w <= 0 || h <= 0) break;
            const unsigned char *pix = p + 16;
            ReplayTex *tt = find_tex(tid);
            if (!tt) { register_texture(tid, w, h); tt = find_tex(tid); }
            if (!tt) break;
            /* Free any prior GL texture we attached (white or sidecar PNG). */
            if (tt->gl_tex && tt->gl_tex != g_white_tex) {
                glDeleteTextures(1, &tt->gl_tex);
            }
            GLuint gl;
            glGenTextures(1, &gl);
            glBindTexture(GL_TEXTURE_2D, gl);
            int has_zero_alpha = 0;
            unsigned char *rgba = convert_masked_pixels(tt, pix, w, h, bpp,
                                                        nbytes,
                                                        &has_zero_alpha);
            if (rgba) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, rgba);
                free(rgba);
                tt->has_zero_alpha = has_zero_alpha;
            } else if (bpp == 32) {
                /* v1-v3 compatibility: old captures lack DDPIXELFORMAT, so
                   retain the known A8R8G8B8 little-endian BGRA assumption. */
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                             GL_BGRA, GL_UNSIGNED_BYTE, pix);
            } else if (bpp == 24) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
                             GL_BGR, GL_UNSIGNED_BYTE, pix);
            } else {
                /* Unknown bpp -- bind white fallback. */
                glDeleteTextures(1, &gl);
                tt->gl_tex = g_white_tex;
                break;
            }
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);
            tt->gl_tex = gl;
            tt->w = w; tt->h = h;
            break;
        }
        case REC_SET_TEXTURE: {
            if (plen < 5) break;
            unsigned int stage = p[0];
            if (stage != 0) break;
            cur_tex_id = r_u32(p + 1);
            break;
        }
        case REC_SET_TEXSTAGESTATE: {
            if (plen < 9) break;
            unsigned int stage = p[0];
            unsigned int state = r_u32(p + 1);
            unsigned int value = r_u32(p + 5);
            if (stage != 0) break;
            switch (state) {
            case D3DTSS_ADDRESS:
                texstage.address_u = value;
                texstage.address_v = value;
                break;
            case D3DTSS_ADDRESSU:
                texstage.address_u = value;
                break;
            case D3DTSS_ADDRESSV:
                texstage.address_v = value;
                break;
            case D3DTSS_MAGFILTER:
                texstage.mag_filter = value;
                break;
            case D3DTSS_MINFILTER:
                texstage.min_filter = value;
                break;
            case D3DTSS_MIPFILTER:
                texstage.mip_filter = value;
                break;
            default:
                break;
            }
            break;
        }
        case REC_SET_MATERIAL: {
            if (plen >= 68)
                material_read(&material, p);
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
            case D3DRS_ALPHABLENDENABLE: alpha_blend = value ? 1 : 0;
                                         apply_blend_state(g_dbg_disable_blend ? 0 : alpha_blend,
                                                           src_blend, dst_blend);
                                         break;
            case D3DRS_SRCBLEND:         src_blend = value;
                                         apply_blend_state(g_dbg_disable_blend ? 0 : alpha_blend,
                                                           src_blend, dst_blend);
                                         break;
            case D3DRS_DESTBLEND:        dst_blend = value;
                                         apply_blend_state(g_dbg_disable_blend ? 0 : alpha_blend,
                                                           src_blend, dst_blend);
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
            int draw_index = draws + 1;
            draws++;
            if (draw_index < g_dbg_draw_start || draw_index > g_dbg_draw_end)
                break;
            if (g_dbg_only_tex_enabled && cur_tex_id != g_dbg_only_tex)
                break;

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
            apply_texstage(&texstage, tt ? tt->gl_tex : g_white_tex);
            glUniform1i(g_loc_has_tex, 1);
            glUniform1i(g_loc_use_alpha,
                        (alpha_blend && !g_dbg_disable_blend) ? 1 : 0);
            glUniform1i(g_loc_alpha_discard,
                        (tt && tt->has_zero_alpha) ? 1 : 0);
            if (lighting_on) {
                float lr = material.emissive[0] + material.diffuse[0];
                float lg = material.emissive[1] + material.diffuse[1];
                float lb = material.emissive[2] + material.diffuse[2];
                float la = material.emissive[3] > 0.0f ?
                           material.emissive[3] : material.diffuse[3];
                if (lr > 1.0f) lr = 1.0f;
                if (lg > 1.0f) lg = 1.0f;
                if (lb > 1.0f) lb = 1.0f;
                if (la > 1.0f) la = 1.0f;
                glUniform1i(g_loc_lighting, 1);
                glUniform4f(g_loc_lit_color, lr, lg, lb, la);
            } else {
                glUniform1i(g_loc_lighting, 0);
            }
            if (g_dbg_highlight_tex_enabled &&
                cur_tex_id == g_dbg_highlight_tex) {
                glUniform1i(g_loc_debug_mode, 1);
                glUniform4f(g_loc_debug_color, 1.0f, 0.0f, 1.0f, 1.0f);
            } else if (g_dbg_flat_groups) {
                float color[4];
                debug_color_for_tex(cur_tex_id, color);
                glUniform1i(g_loc_debug_mode, 1);
                glUniform4f(g_loc_debug_color, color[0], color[1],
                            color[2], color[3]);
            } else {
                glUniform1i(g_loc_debug_mode, 0);
            }
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
            issued_draws++;
            glBindVertexArray(0);
            glUniform1i(g_loc_debug_mode, 0);
            free(verts);
            (void)lighting_on;       /* v0: unused; flat textured */
            break;
        }
        default: break;
        }
    }
    static int reported = 0;
    if (!reported) {
        fprintf(stderr, "[replay] frame: issued %d GL draws, "
                "registered %d textures\n", issued_draws, g_tex_count);
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
