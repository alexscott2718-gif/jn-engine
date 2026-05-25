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
    uint32_t src_blend;
    uint32_t dst_blend;
} CaptureSceneDraw;

typedef struct {
    float clip[4];
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
static int g_loc_tex, g_loc_alpha_discard;

static const char *VS =
    GLSL_VS
    "layout(location=0) in vec4 aClip;\n"
    "layout(location=1) in vec2 aUV;\n"
    "layout(location=2) in vec4 aColor;\n"
    "out vec2 vUV;\n"
    "out vec4 vColor;\n"
    "void main(){ vec4 p=aClip; p.z=p.z*2.0-p.w; gl_Position=p; vUV=aUV; vColor=aColor; }\n";

static const char *FS =
    GLSL_FS
    "in vec2 vUV;\n"
    "in vec4 vColor;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTex;\n"
    "uniform int uAlphaDiscard;\n"
    "void main(){ vec4 tex=texture(uTex,vUV); if(uAlphaDiscard==1 && tex.a<=0.001) discard; FragColor=tex*vColor; }\n";

static uint32_t read_u32(FILE *f) { uint32_t v = 0; fread(&v, 1, sizeof(v), f); return v; }
static uint16_t read_u16(FILE *f) { uint16_t v = 0; fread(&v, 1, sizeof(v), f); return v; }
static uint8_t read_u8(FILE *f) { uint8_t v = 0; fread(&v, 1, sizeof(v), f); return v; }

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
    }
    return s;
}

static int init_program(void) {
    uint32_t vs = compile_shader(GL_VERTEX_SHADER, VS);
    uint32_t fs = compile_shader(GL_FRAGMENT_SHADER, FS);
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs);
    glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    int ok = 0;
    glGetProgramiv(g_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(g_prog, sizeof(log), NULL, log);
        fprintf(stderr, "[capture_scene] link: %s\n", log);
        return 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    g_loc_tex = glGetUniformLocation(g_prog, "uTex");
    g_loc_alpha_discard = glGetUniformLocation(g_prog, "uAlphaDiscard");

    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(CaptureSceneVertex), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CaptureSceneVertex), (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(CaptureSceneVertex), (void *)(6 * sizeof(float)));
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

int capture_scene_init(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[capture_scene] cannot open %s\n", path);
        return 0;
    }
    if (read_u32(f) != JNC1_MAGIC) {
        fprintf(stderr, "[capture_scene] bad scene magic in %s\n", path);
        fclose(f);
        return 0;
    }
    g_texture_count = read_u32(f);
    g_draw_count = read_u32(f);
    g_vertex_count = read_u32(f);
    g_textures = calloc(g_texture_count, sizeof(*g_textures));
    g_draws = calloc(g_draw_count, sizeof(*g_draws));
    g_vertices = calloc(g_vertex_count, sizeof(*g_vertices));
    if (!g_textures || !g_draws || !g_vertices) {
        fclose(f);
        return 0;
    }
    for (uint32_t i = 0; i < g_texture_count; i++) {
        uint32_t tex_id = read_u32(f);
        (void)read_u16(f);
        (void)read_u16(f);
        (void)read_u8(f);
        uint8_t path_len = read_u8(f);
        char rel[256];
        if (fread(rel, 1, path_len, f) != path_len) {
            fclose(f);
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
        d->tex_id = read_u32(f);
        d->first = read_u32(f);
        d->count = read_u16(f);
        d->prim_type = read_u8(f);
        d->alpha_blend = read_u8(f);
        d->z_enable = read_u8(f);
        d->z_write = read_u8(f);
        d->alpha_discard = read_u8(f);
        (void)read_u8(f);
        d->src_blend = read_u32(f);
        d->dst_blend = read_u32(f);
    }
    if (fread(g_vertices, sizeof(*g_vertices), g_vertex_count, f) != g_vertex_count) {
        fclose(f);
        return 0;
    }
    fclose(f);
    if (!init_program()) return 0;
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, (size_t)g_vertex_count * sizeof(*g_vertices), g_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    g_active = 1;
    fprintf(stderr, "[capture_scene] loaded %s: %u textures, %u draws, %u vertices\n",
            path, g_texture_count, g_draw_count, g_vertex_count);
    return 1;
}

int capture_scene_active(void) {
    return g_active;
}

void capture_scene_render(int viewport_w, int viewport_h) {
    if (!g_active) return;
    glViewport(0, 0, viewport_w, viewport_h);
    glClearColor(0.45f, 0.70f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glUseProgram(g_prog);
    glUniform1i(g_loc_tex, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(g_vao);
    for (uint32_t i = 0; i < g_draw_count; i++) {
        const CaptureSceneDraw *d = &g_draws[i];
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
        glBindTexture(GL_TEXTURE_2D, tex);
        glDrawArrays(GL_TRIANGLE_FAN, (GLint)d->first, (GLsizei)d->count);
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
    g_active = 0;
}
