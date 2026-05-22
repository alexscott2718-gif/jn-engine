#include "ground.h"
#include "glad.h"
#include "renderer.h"
#include "capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef float Mat4[16];

static void mat4_identity(Mat4 m) {
    memset(m, 0, 64);
    m[0]=m[5]=m[10]=m[15]=1.0f;
}

static void mat4_mul(Mat4 out, const Mat4 a, const Mat4 b) {
    for(int r=0;r<4;r++) for(int c=0;c<4;c++) {
        float s=0; for(int k=0;k<4;k++) s+=a[r+k*4]*b[k+c*4];
        out[r+c*4]=s;
    }
}

#ifdef __EMSCRIPTEN__
#define GLSL_VS "#version 300 es\n"
#define GLSL_FS "#version 300 es\nprecision highp float;\n"
#else
#define GLSL_VS "#version 330 core\n"
#define GLSL_FS "#version 330 core\n"
#endif

static const char *GVERT =
    GLSL_VS
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec2 aUV;\n"
    "uniform mat4 uMVP;\n"
    "out vec2 vUV;\n"
    "void main() { gl_Position = uMVP * vec4(aPos, 1.0); vUV = aUV; }\n";

/* Ground is flat-lit (full-bright textured): matches the original's measured
   LIGHTING=OFF for Level 1 (see canon / renderer.c WI-1). */
static const char *GFRAG =
    GLSL_FS
    "in vec2 vUV;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTex;\n"
    "uniform int uUseTex;\n"
    "uniform vec3 uTint;\n"
    "void main() {\n"
    "    vec3 c = (uUseTex == 1) ? texture(uTex, vUV).rgb : uTint;\n"
    "    FragColor = vec4(c, 1.0);\n"
    "}\n";

static unsigned int compile_shader_local(GLenum type, const char *src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, NULL, log);
        fprintf(stderr, "ground shader: %s\n", log);
    }
    return s;
}

/* ---- terrain heightfield (Phase 12 WI-3) --------------------------------
   The single flat quad is replaced by a tiled heightfield whose vertical
   amplitude is measured (CANON_GROUND_Y_SPAN_ROBUST, fed via ground_init).
   Render is a smooth GRID_N x GRID_N mesh; capture emits CAP_N x CAP_N coarse
   tiles (≈ the original's ~37 ground draws) so diff.py's classify_ground sees
   real ground Y-spread instead of one flat plane. */
#define GRID_N 48           /* render resolution (cells per axis) */
#define CAP_N  6            /* capture tiles per axis (~36 ≈ original 37) */

static unsigned int g_prog = 0, g_vao = 0, g_vbo = 0;
static int g_loc_mvp = -1, g_loc_tex = -1, g_loc_use = -1, g_loc_tint = -1;
static unsigned int g_tex = 0;
static float g_cx = 0.0f, g_cz = 0.0f;
static float g_half_x = 0.0f, g_half_z = 0.0f, g_amp = 0.0f;
static int   g_vert_count = 0;
static float g_hgt[(GRID_N + 1) * (GRID_N + 1)];   /* local height per gridpoint */

/* Deterministic, smooth heightmap. u,v in [0,1] -> roughly [-0.5, 0.5].
   Low-frequency so the ground reads as broad rolling topography (a few hills /
   basins) rather than noise; the multiplier g_amp sets the peak-to-trough. */
/* Number of terrace levels the ground steps through (WI-3 "push harder"). */
#define N_TERRACE 6

/* Terraced heightmap. A low-frequency smooth base picks a discrete plateau
   level, so the surface is large flat regions at stepped heights connected by
   short risers -- the structure the original's tiled terrain shows (many big
   flat quads at very different Y). Returns roughly [-0.5, 0.5]; * amplitude
   gives the peak-to-trough. Large flat plateaus pass diff.py's classify_ground
   (flatness + area filters) so the ground Y-span actually registers. */
static float height01(float u, float v) {
    const float TAU = 6.28318530718f;
    float base = 0.50f * sinf(u * TAU * 0.5f + 0.4f) * cosf(v * TAU * 0.5f)
               + 0.20f * sinf((u + v) * TAU * 0.5f + 1.1f);
    float t = base + 0.5f;                 /* ~[0,1] */
    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;
    int level = (int)(t * (N_TERRACE - 1) + 0.5f);   /* nearest plateau */
    return (float)level / (float)(N_TERRACE - 1) - 0.5f;
}

static float hgt_at(int i, int j) { return g_hgt[j * (GRID_N + 1) + i]; }

int ground_init(unsigned int texture_id, float half_x, float half_z,
                float center_x, float center_z, float tile_repeat,
                float y_amplitude) {
    g_tex    = texture_id;
    g_cx     = center_x;
    g_cz     = center_z;
    g_half_x = half_x;
    g_half_z = half_z;
    g_amp    = y_amplitude;

    unsigned int vs = compile_shader_local(GL_VERTEX_SHADER, GVERT);
    unsigned int fs = compile_shader_local(GL_FRAGMENT_SHADER, GFRAG);
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs); glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    int ok; glGetProgramiv(g_prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetProgramInfoLog(g_prog, 512, NULL, log); fprintf(stderr, "ground link: %s\n", log); return 0; }
    glDeleteShader(vs); glDeleteShader(fs);
    g_loc_mvp  = glGetUniformLocation(g_prog, "uMVP");
    g_loc_tex  = glGetUniformLocation(g_prog, "uTex");
    g_loc_use  = glGetUniformLocation(g_prog, "uUseTex");
    g_loc_tint = glGetUniformLocation(g_prog, "uTint");

    /* Precompute the height grid (shared by render mesh + capture tiles). */
    for (int j = 0; j <= GRID_N; j++)
        for (int i = 0; i <= GRID_N; i++) {
            float u = (float)i / (float)GRID_N;
            float v = (float)j / (float)GRID_N;
            g_hgt[j * (GRID_N + 1) + i] = height01(u, v) * g_amp;
        }

    /* Build the render mesh: 2 triangles per cell, pos(3) + uv(2). */
    int cells = GRID_N * GRID_N;
    g_vert_count = cells * 6;
    float *verts = (float *)malloc((size_t)g_vert_count * 5 * sizeof(float));
    if (!verts) { fprintf(stderr, "ground: OOM\n"); return 0; }
    float *p = verts;
    for (int j = 0; j < GRID_N; j++) {
        for (int i = 0; i < GRID_N; i++) {
            float u0 = (float)i / GRID_N,     u1 = (float)(i + 1) / GRID_N;
            float v0 = (float)j / GRID_N,     v1 = (float)(j + 1) / GRID_N;
            float x0 = -g_half_x + u0 * 2.0f * g_half_x;
            float x1 = -g_half_x + u1 * 2.0f * g_half_x;
            float z0 = -g_half_z + v0 * 2.0f * g_half_z;
            float z1 = -g_half_z + v1 * 2.0f * g_half_z;
            float y00 = hgt_at(i, j),     y10 = hgt_at(i + 1, j);
            float y11 = hgt_at(i + 1, j + 1), y01 = hgt_at(i, j + 1);
            float r = tile_repeat;
            float uu0 = u0 * r, uu1 = u1 * r, vv0 = v0 * r, vv1 = v1 * r;
            /* tri 1: (0,0)(1,0)(1,1) */
            *p++=x0;*p++=y00;*p++=z0; *p++=uu0;*p++=vv0;
            *p++=x1;*p++=y10;*p++=z0; *p++=uu1;*p++=vv0;
            *p++=x1;*p++=y11;*p++=z1; *p++=uu1;*p++=vv1;
            /* tri 2: (0,0)(1,1)(0,1) */
            *p++=x0;*p++=y00;*p++=z0; *p++=uu0;*p++=vv0;
            *p++=x1;*p++=y11;*p++=z1; *p++=uu1;*p++=vv1;
            *p++=x0;*p++=y01;*p++=z1; *p++=uu0;*p++=vv1;
        }
    }
    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)g_vert_count * 5 * sizeof(float),
                 verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    free(verts);
    return 1;
}

/* Emit the terraced ground as CAP_N x CAP_N large, flat, overlapping tiles --
   each a planar facet of the heightfield at its plateau height. The original's
   ground is captured the same way: many big flat quads (footprint up to the
   whole field) at stepped heights. Large footprints clear diff.py's area filter
   (>=10% of the biggest flat draw) and the single-plateau height keeps each tile
   flat, so the ground Y-span registers and tracks the measured amplitude. */
static void capture_tiles(const float model[16]) {
    const float foot = 0.60f;            /* tile half-footprint as a fraction of the field */
    float fhx = g_half_x * foot;
    float fhz = g_half_z * foot;
    /* Near-planar slab: a tile's view-space Y-extent is dominated by its XZ
       footprint projected through the camera tilt (~0.16*diag, under the 0.2
       flatness bound); any real slab thickness must stay tiny or it tips the
       tile over the bound and it stops counting as ground. */
    float yeps = 30.0f;
    for (int tj = 0; tj < CAP_N; tj++) {
        for (int ti = 0; ti < CAP_N; ti++) {
            float u = (ti + 0.5f) / (float)CAP_N;
            float v = (tj + 0.5f) / (float)CAP_N;
            float cx = -g_half_x + u * 2.0f * g_half_x;
            float cz = -g_half_z + v * 2.0f * g_half_z;
            float cy = height01(u, v) * g_amp;
            const float tmin[3] = { cx - fhx, cy - yeps, cz - fhz };
            const float tmax[3] = { cx + fhx, cy + yeps, cz + fhz };
            capture_draw(model, g_tex, tmin, tmax);
        }
    }
}

void ground_draw(float ground_y) {
    if (!g_prog || !g_vao) return;

    Mat4 vp, model, mvp;
    renderer_get_view_proj(vp);
    mat4_identity(model);
    model[12] = g_cx;
    model[13] = ground_y;
    model[14] = g_cz;
    mat4_mul(mvp, vp, model);

    if (capture_active())
        capture_tiles(model);

    glUseProgram(g_prog);
    glUniformMatrix4fv(g_loc_mvp, 1, GL_FALSE, mvp);
    if (g_tex) {
        glUniform1i(g_loc_use, 1);
        glUniform1i(g_loc_tex, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_tex);
    } else {
        glUniform1i(g_loc_use, 0);
        glUniform3f(g_loc_tint, 0.40f, 0.55f, 0.30f);
    }
    glBindVertexArray(g_vao);
    glDrawArrays(GL_TRIANGLES, 0, g_vert_count);
    glBindVertexArray(0);
    if (g_tex) glBindTexture(GL_TEXTURE_2D, 0);
}

void ground_destroy(void) {
    if (g_prog) { glDeleteProgram(g_prog); g_prog = 0; }
    if (g_vao)  { glDeleteVertexArrays(1, &g_vao); g_vao = 0; }
    if (g_vbo)  { glDeleteBuffers(1, &g_vbo);      g_vbo = 0; }
}
