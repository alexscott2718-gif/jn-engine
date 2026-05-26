#include "capture_scene.h"
#include "glad.h"
#include "stb_image.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#define GLSL_VS "#version 300 es\n"
#define GLSL_FS "#version 300 es\nprecision highp float;\n"
#else
#define GLSL_VS "#version 330 core\n"
#define GLSL_FS "#version 330 core\n"
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T 0x2803
#endif
#ifndef GL_REPEAT
#define GL_REPEAT 0x2901
#endif
#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN 0x0006
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif
#ifndef GL_ZERO
#define GL_ZERO 0
#endif
#ifndef GL_ONE
#define GL_ONE 1
#endif
#ifndef GL_SRC_COLOR
#define GL_SRC_COLOR 0x0300
#endif
#ifndef GL_ONE_MINUS_SRC_COLOR
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#endif
#ifndef GL_DST_ALPHA
#define GL_DST_ALPHA 0x0304
#endif
#ifndef GL_ONE_MINUS_DST_ALPHA
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#endif
#ifndef GL_DST_COLOR
#define GL_DST_COLOR 0x0306
#endif
#ifndef GL_ONE_MINUS_DST_COLOR
#define GL_ONE_MINUS_DST_COLOR 0x0307
#endif
#ifndef GL_SRC_ALPHA_SATURATE
#define GL_SRC_ALPHA_SATURATE 0x0308
#endif

#define JNC1_MAGIC 0x31434E4Au
#define JNR1_MAGIC 0x31524E4Au
#define D3DBLEND_ZERO          1
#define D3DBLEND_ONE           2
#define D3DBLEND_SRCCOLOR      3
#define D3DBLEND_INVSRCCOLOR   4
#define D3DBLEND_SRCALPHA      5
#define D3DBLEND_INVSRCALPHA   6
#define D3DBLEND_DESTALPHA     7
#define D3DBLEND_INVDESTALPHA  8
#define D3DBLEND_DESTCOLOR     9
#define D3DBLEND_INVDESTCOLOR 10
#define D3DBLEND_SRCALPHASAT  11
#define D3DPT_TRIANGLELIST     4
#define D3DPT_TRIANGLESTRIP    5
#define D3DPT_TRIANGLEFAN      6

typedef struct {
    uint32_t tex_id;
    uint32_t gl_tex;
} CaptureSceneTexture;

typedef struct {
    uint32_t tex_id;
    uint32_t first;
    uint16_t count;
    uint8_t prim_type;
    uint8_t alpha_blend;
    uint8_t z_enable;
    uint8_t z_write;
    uint8_t alpha_discard;
    uint8_t group;
    uint32_t src_blend;
    uint32_t dst_blend;
} CaptureSceneDraw;

typedef struct {
    float clip[4];
    float world[3];
    float uv[2];
    float color[4];
} CaptureSceneVertex;

static int g_active;
static CaptureSceneTexture *g_textures;
static uint32_t g_texture_count;
static CaptureSceneDraw *g_draws;
static uint32_t g_draw_count;
static CaptureSceneVertex *g_vertices;
static uint32_t g_vertex_count;
static uint32_t g_prog, g_vao, g_vbo;
static int g_loc_tex, g_loc_alpha_discard, g_loc_ndc_offset;
static int g_loc_world_view_proj, g_loc_world_offset, g_loc_use_world;
static uint32_t g_group_mask = 0xFFFFFFFFu;
static float g_group_ndc_offset[32][2];
static float g_group_world_offset[32][3];
static int g_group_use_world[32];
static float g_world_view_proj[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};
static int g_reproject;

static const float IDENTITY_MAT4[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

static const char *VS =
    GLSL_VS
    "layout(location=0) in vec4 aClip;\n"
    "layout(location=1) in vec3 aWorld;\n"
    "layout(location=2) in vec2 aUV;\n"
    "layout(location=3) in vec4 aColor;\n"
    "out vec2 vUV;\n"
    "out vec4 vColor;\n"
    "uniform vec2 uNdcOffset;\n"
    "uniform mat4 uWorldViewProj;\n"
    "uniform vec3 uWorldOffset;\n"
    "uniform int uUseWorld;\n"
    "void main(){ vec4 p; if(uUseWorld==1){ p=uWorldViewProj*vec4(aWorld+uWorldOffset,1.0); } else { p=aClip; p.xy+=uNdcOffset*p.w; p.z=p.z*2.0-p.w; } gl_Position=p; vUV=aUV; vColor=aColor; }\n";

static const char *FS =
    GLSL_FS
    "in vec2 vUV;\n"
    "in vec4 vColor;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTex;\n"
    "uniform int uAlphaDiscard;\n"
    "void main(){ vec4 tex=texture(uTex,vUV); if(uAlphaDiscard==1 && tex.a<=0.001) discard; FragColor=tex*vColor; }\n";

static int read_exact(FILE *f, void *dst, size_t size) {
    return fread(dst, 1, size, f) == size;
}

static uint32_t read_u32(FILE *f, int *ok) {
    uint32_t v = 0;
    if (*ok && !read_exact(f, &v, sizeof(v))) *ok = 0;
    return v;
}

static uint16_t read_u16(FILE *f, int *ok) {
    uint16_t v = 0;
    if (*ok && !read_exact(f, &v, sizeof(v))) *ok = 0;
    return v;
}

static uint8_t read_u8(FILE *f, int *ok) {
    uint8_t v = 0;
    if (*ok && !read_exact(f, &v, sizeof(v))) *ok = 0;
    return v;
}

static void reset_group_state(void) {
    g_group_mask = 0xFFFFFFFFu;
    for (int i = 0; i < 32; i++) {
        g_group_ndc_offset[i][0] = 0.0f;
        g_group_ndc_offset[i][1] = 0.0f;
        g_group_world_offset[i][0] = 0.0f;
        g_group_world_offset[i][1] = 0.0f;
        g_group_world_offset[i][2] = 0.0f;
        g_group_use_world[i] = 0;
    }
}

static void reset_world_view_proj(void) {
    for (int i = 0; i < 16; i++)
        g_world_view_proj[i] = IDENTITY_MAT4[i];
}

static uint32_t compile_shader(GLenum type, const char *src) {
    uint32_t s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    int ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "[capture_scene] shader: %s\n", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static int init_program(void) {
    uint32_t vs = compile_shader(GL_VERTEX_SHADER, VS);
    uint32_t fs = compile_shader(GL_FRAGMENT_SHADER, FS);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs);
    glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    int ok = 0;
    glGetProgramiv(g_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(g_prog, sizeof(log), NULL, log);
        fprintf(stderr, "[capture_scene] link: %s\n", log);
        glDeleteProgram(g_prog);
        g_prog = 0;
        return 0;
    }
    g_loc_tex = glGetUniformLocation(g_prog, "uTex");
    g_loc_alpha_discard = glGetUniformLocation(g_prog, "uAlphaDiscard");
    g_loc_ndc_offset = glGetUniformLocation(g_prog, "uNdcOffset");
    g_loc_world_view_proj = glGetUniformLocation(g_prog, "uWorldViewProj");
    g_loc_world_offset = glGetUniformLocation(g_prog, "uWorldOffset");
    g_loc_use_world = glGetUniformLocation(g_prog, "uUseWorld");

    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(CaptureSceneVertex), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CaptureSceneVertex), (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CaptureSceneVertex), (void *)(7 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(CaptureSceneVertex), (void *)(9 * sizeof(float)));
    glBindVertexArray(0);
    return 1;
}

static uint32_t load_png_no_flip(const char *path) {
    int w, h, ch;
    stbi_set_flip_vertically_on_load(0);
    unsigned char *data = stbi_load(path, &w, &h, &ch, 4);
    if (!data) {
        fprintf(stderr, "[capture_scene] texture load failed %s: %s\n", path, stbi_failure_reason());
        return 0;
    }
    uint32_t tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return tex;
}

static uint32_t lookup_texture(uint32_t tex_id) {
    for (uint32_t i = 0; i < g_texture_count; i++)
        if (g_textures[i].tex_id == tex_id)
            return g_textures[i].gl_tex;
    return 0;
}

static GLenum d3d_blend_to_gl(uint32_t value) {
    switch (value) {
    case D3DBLEND_ZERO: return GL_ZERO;
    case D3DBLEND_ONE: return GL_ONE;
    case D3DBLEND_SRCCOLOR: return GL_SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR: return GL_ONE_MINUS_SRC_COLOR;
    case D3DBLEND_SRCALPHA: return GL_SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA: return GL_ONE_MINUS_SRC_ALPHA;
    case D3DBLEND_DESTALPHA: return GL_DST_ALPHA;
    case D3DBLEND_INVDESTALPHA: return GL_ONE_MINUS_DST_ALPHA;
    case D3DBLEND_DESTCOLOR: return GL_DST_COLOR;
    case D3DBLEND_INVDESTCOLOR: return GL_ONE_MINUS_DST_COLOR;
    case D3DBLEND_SRCALPHASAT: return GL_SRC_ALPHA_SATURATE;
    default: return GL_ONE;
    }
}

static GLenum d3d_prim_to_gl(uint8_t value) {
    switch (value) {
    case D3DPT_TRIANGLELIST: return GL_TRIANGLES;
    case D3DPT_TRIANGLESTRIP: return GL_TRIANGLE_STRIP;
    case D3DPT_TRIANGLEFAN: return GL_TRIANGLE_FAN;
    default: return 0;
    }
}

static int valid_prim_count(uint8_t prim_type, uint16_t count) {
    switch (prim_type) {
    case D3DPT_TRIANGLELIST: return count >= 3 && (count % 3) == 0;
    case D3DPT_TRIANGLESTRIP:
    case D3DPT_TRIANGLEFAN:
        return count >= 3;
    default:
        return 0;
    }
}

int capture_scene_init(const char *path) {
    capture_scene_destroy();
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[capture_scene] cannot open %s\n", path);
        return 0;
    }
    int ok = 1;
    uint32_t magic = read_u32(f, &ok);
    if (!ok) {
        fprintf(stderr, "[capture_scene] short scene header in %s\n", path);
        fclose(f);
        return 0;
    }
    if (magic != JNC1_MAGIC && magic != JNR1_MAGIC) {
        fprintf(stderr, "[capture_scene] bad scene magic in %s\n", path);
        fclose(f);
        return 0;
    }
    g_reproject = magic == JNR1_MAGIC;
    g_texture_count = read_u32(f, &ok);
    g_draw_count = read_u32(f, &ok);
    g_vertex_count = read_u32(f, &ok);
    if (!ok) {
        fprintf(stderr, "[capture_scene] short scene counts in %s\n", path);
        fclose(f);
        capture_scene_destroy();
        return 0;
    }
    g_textures = calloc(g_texture_count, sizeof(*g_textures));
    g_draws = calloc(g_draw_count, sizeof(*g_draws));
    g_vertices = calloc(g_vertex_count, sizeof(*g_vertices));
    if (!g_textures || !g_draws || !g_vertices) {
        fclose(f);
        capture_scene_destroy();
        return 0;
    }
    for (uint32_t i = 0; i < g_texture_count; i++) {
        uint32_t tex_id = read_u32(f, &ok);
        (void)read_u16(f, &ok);
        (void)read_u16(f, &ok);
        (void)read_u8(f, &ok);
        uint8_t path_len = read_u8(f, &ok);
        if (!ok) {
            fprintf(stderr, "[capture_scene] short texture table in %s\n", path);
            fclose(f);
            capture_scene_destroy();
            return 0;
        }
        char rel[256];
        if (!read_exact(f, rel, path_len)) {
            fclose(f);
            capture_scene_destroy();
            return 0;
        }
        rel[path_len] = '\0';
        char full[512];
        snprintf(full, sizeof(full), "assets/capture/level1_hudfix/%s", rel);
        g_textures[i].tex_id = tex_id;
        g_textures[i].gl_tex = load_png_no_flip(full);
    }
    for (uint32_t i = 0; i < g_draw_count; i++) {
        CaptureSceneDraw *d = &g_draws[i];
        d->tex_id = read_u32(f, &ok);
        d->first = read_u32(f, &ok);
        d->count = read_u16(f, &ok);
        d->prim_type = read_u8(f, &ok);
        d->alpha_blend = read_u8(f, &ok);
        d->z_enable = read_u8(f, &ok);
        d->z_write = read_u8(f, &ok);
        d->alpha_discard = read_u8(f, &ok);
        d->group = read_u8(f, &ok);
        d->src_blend = read_u32(f, &ok);
        d->dst_blend = read_u32(f, &ok);
        if (!ok) {
            fprintf(stderr, "[capture_scene] short draw table in %s\n", path);
            fclose(f);
            capture_scene_destroy();
            return 0;
        }
        if (!valid_prim_count(d->prim_type, d->count) ||
            d->first > g_vertex_count ||
            d->count > g_vertex_count - d->first) {
            fprintf(stderr,
                    "[capture_scene] invalid draw %u prim=%u first=%u count=%u vertices=%u\n",
                    i, d->prim_type, d->first, d->count, g_vertex_count);
            fclose(f);
            capture_scene_destroy();
            return 0;
        }
    }
    for (uint32_t i = 0; i < g_vertex_count; i++) {
        if (g_reproject) {
            if (!read_exact(f, &g_vertices[i], sizeof(*g_vertices))) {
                fclose(f);
                capture_scene_destroy();
                return 0;
            }
        } else {
            float old_vertex[10];
            if (!read_exact(f, old_vertex, sizeof(old_vertex))) {
                fclose(f);
                capture_scene_destroy();
                return 0;
            }
            g_vertices[i].clip[0] = old_vertex[0];
            g_vertices[i].clip[1] = old_vertex[1];
            g_vertices[i].clip[2] = old_vertex[2];
            g_vertices[i].clip[3] = old_vertex[3];
            g_vertices[i].world[0] = 0.0f;
            g_vertices[i].world[1] = 0.0f;
            g_vertices[i].world[2] = 0.0f;
            g_vertices[i].uv[0] = old_vertex[4];
            g_vertices[i].uv[1] = old_vertex[5];
            g_vertices[i].color[0] = old_vertex[6];
            g_vertices[i].color[1] = old_vertex[7];
            g_vertices[i].color[2] = old_vertex[8];
            g_vertices[i].color[3] = old_vertex[9];
        }
    }
    fclose(f);
    if (!init_program()) {
        capture_scene_destroy();
        return 0;
    }
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, (size_t)g_vertex_count * sizeof(*g_vertices), g_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    reset_group_state();
    reset_world_view_proj();
    g_active = 1;
    fprintf(stderr, "[capture_scene] loaded %s: %u textures, %u draws, %u vertices%s\n",
            path, g_texture_count, g_draw_count, g_vertex_count,
            g_reproject ? ", reproject" : "");
    return 1;
}

int capture_scene_active(void) {
    return g_active;
}

void capture_scene_set_group_visible(int group, int visible) {
    if (group < 0 || group >= 32) return;
    if (visible)
        g_group_mask |= (1u << group);
    else
        g_group_mask &= ~(1u << group);
}

void capture_scene_set_group_ndc_offset(int group, float x, float y) {
    if (group < 0 || group >= 32) return;
    g_group_ndc_offset[group][0] = x;
    g_group_ndc_offset[group][1] = y;
}

void capture_scene_set_group_world_offset(int group, float x, float y, float z) {
    if (group < 0 || group >= 32) return;
    g_group_world_offset[group][0] = x;
    g_group_world_offset[group][1] = y;
    g_group_world_offset[group][2] = z;
    g_group_use_world[group] =
        (x * x + y * y + z * z) > 0.000001f ? 1 : 0;
}

static void mat4_mul_local(float out[16], const float a[16], const float b[16]) {
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            out[c * 4 + r] =
                a[0 * 4 + r] * b[c * 4 + 0] +
                a[1 * 4 + r] * b[c * 4 + 1] +
                a[2 * 4 + r] * b[c * 4 + 2] +
                a[3 * 4 + r] * b[c * 4 + 3];
        }
    }
}

void capture_scene_set_world_view_proj(const float view[16], const float proj[16]) {
    if (!view || !proj) return;
    mat4_mul_local(g_world_view_proj, proj, view);
}

void capture_scene_render(int viewport_w, int viewport_h) {
    if (!g_active) return;
    glViewport(0, 0, viewport_w, viewport_h);
    glClearColor(0.45f, 0.70f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glUseProgram(g_prog);
    glUniform1i(g_loc_tex, 0);
    glUniformMatrix4fv(g_loc_world_view_proj, 1, GL_FALSE, g_world_view_proj);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(g_vao);
    for (uint32_t i = 0; i < g_draw_count; i++) {
        const CaptureSceneDraw *d = &g_draws[i];
        if (d->group < 32 && ((g_group_mask & (1u << d->group)) == 0))
            continue;
        uint32_t tex = lookup_texture(d->tex_id);
        if (!tex) continue;
        if (d->z_enable) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        glDepthMask(d->z_write ? GL_TRUE : GL_FALSE);
        if (d->alpha_blend) {
            glEnable(GL_BLEND);
            glBlendFuncSeparate(d3d_blend_to_gl(d->src_blend),
                                d3d_blend_to_gl(d->dst_blend),
                                GL_ZERO, GL_ONE);
        } else {
            glDisable(GL_BLEND);
        }
        glUniform1i(g_loc_alpha_discard, d->alpha_discard ? 1 : 0);
        if (d->group < 32)
            glUniform2f(g_loc_ndc_offset,
                        g_group_ndc_offset[d->group][0],
                        g_group_ndc_offset[d->group][1]);
        else
            glUniform2f(g_loc_ndc_offset, 0.0f, 0.0f);
        if (g_reproject && d->group < 32 && g_group_use_world[d->group]) {
            glUniform1i(g_loc_use_world, 1);
            glUniform3f(g_loc_world_offset,
                        g_group_world_offset[d->group][0],
                        g_group_world_offset[d->group][1],
                        g_group_world_offset[d->group][2]);
        } else {
            glUniform1i(g_loc_use_world, 0);
            glUniform3f(g_loc_world_offset, 0.0f, 0.0f, 0.0f);
        }
        glBindTexture(GL_TEXTURE_2D, tex);
        GLenum prim = d3d_prim_to_gl(d->prim_type);
        if (!prim) continue;
        glDrawArrays(prim, (GLint)d->first, (GLsizei)d->count);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void capture_scene_destroy(void) {
    if (g_textures) {
        for (uint32_t i = 0; i < g_texture_count; i++)
            if (g_textures[i].gl_tex)
                glDeleteTextures(1, &g_textures[i].gl_tex);
    }
    if (g_prog) { glDeleteProgram(g_prog); g_prog = 0; }
    if (g_vao) { glDeleteVertexArrays(1, &g_vao); g_vao = 0; }
    if (g_vbo) { glDeleteBuffers(1, &g_vbo); g_vbo = 0; }
    free(g_textures); g_textures = NULL;
    free(g_draws); g_draws = NULL;
    free(g_vertices); g_vertices = NULL;
    g_texture_count = g_draw_count = g_vertex_count = 0;
    reset_group_state();
    reset_world_view_proj();
    g_reproject = 0;
    g_active = 0;
}
