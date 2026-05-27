#include "renderer.h"
#include "capture.h"
#include "canon_data.h"   /* Phase 12: measured lighting from build/canon.json */
#include "glad.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ---- mat4 helpers (column-major, OpenGL convention) ---- */
typedef float Mat4[16];

static void mat4_identity(Mat4 m) {
    memset(m, 0, 64);
    m[0]=m[5]=m[10]=m[15]=1.0f;
}

static void mat4_perspective(Mat4 m, float fov, float aspect, float near_z, float far_z) {
    memset(m, 0, 64);
    float f = 1.0f / tanf(fov * 0.5f);
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = (far_z + near_z) / (near_z - far_z);
    m[11] = -1.0f;
    m[14] = (2.0f * far_z * near_z) / (near_z - far_z);
}

static void mat4_lookat(Mat4 m, float ex, float ey, float ez,
                                float cx, float cy, float cz) {
    float fx=cx-ex, fy=cy-ey, fz=cz-ez;
    float len=sqrtf(fx*fx+fy*fy+fz*fz);
    if(len<1e-6f) { mat4_identity(m); return; }
    fx/=len; fy/=len; fz/=len;
    /* up = (0,1,0) */
    /* right = normalize(cross(forward, world_up=(0,1,0))) */
    float ux=0,uy=1,uz=0;
    float rx, ry, rz;
    rx=fy*uz-fz*uy; ry=fz*ux-fx*uz; rz=fx*uy-fy*ux;
    len=sqrtf(rx*rx+ry*ry+rz*rz);
    if(len<1e-6f){rx=1;ry=rz=0;}else{rx/=len;ry/=len;rz/=len;}
    /* recalc up */
    float ux2=ry*fz-rz*fy, uy2=rz*fx-rx*fz, uz2=rx*fy-ry*fx;
    m[0]=rx;  m[4]=ry;  m[8]=rz;   m[12]=-(rx*ex+ry*ey+rz*ez);
    m[1]=ux2; m[5]=uy2; m[9]=uz2;  m[13]=-(ux2*ex+uy2*ey+uz2*ez);
    m[2]=-fx; m[6]=-fy; m[10]=-fz; m[14]=fx*ex+fy*ey+fz*ez;
    m[3]=0;   m[7]=0;   m[11]=0;   m[15]=1;
}

static void mat4_mul(Mat4 out, const Mat4 a, const Mat4 b) {
    for(int r=0;r<4;r++) for(int c=0;c<4;c++) {
        float s=0; for(int k=0;k<4;k++) s+=a[r+k*4]*b[k+c*4];
        out[r+c*4]=s;
    }
}

/* ---- shaders ---- */

#ifdef __EMSCRIPTEN__
#define GLSL_VS "#version 300 es\n"
#define GLSL_FS "#version 300 es\nprecision highp float;\n"
#else
#define GLSL_VS "#version 330 core\n"
#define GLSL_FS "#version 330 core\n"
#endif

/* Flat-color shader — position only (used for placeholder boxes) */
static const char *VERT_SRC =
    GLSL_VS
    "layout(location=0) in vec3 aPos;\n"
    "uniform mat4 uMVP;\n"
    "void main() { gl_Position = uMVP * vec4(aPos, 1.0); }\n";

static const char *FRAG_SRC =
    GLSL_FS
    "out vec4 FragColor;\n"
    "uniform vec4 uColor;\n"
    "void main() { FragColor = uColor; }\n";

/* Billboard sprite shader: textured quad, alpha discard for transparency,
   optional RGB tint multiplier (uTint.a == 0 means "no tint", use raw texel). */
static const char *BB_VERT_SRC =
    GLSL_VS
    "layout(location=0) in vec2 aCorner;\n"  /* (-0.5..0.5) corner of quad */
    "layout(location=1) in vec2 aUV;\n"
    "uniform mat4 uVP;\n"
    "uniform vec3 uCenter;\n"
    "uniform vec3 uRight;\n"
    "uniform vec3 uUp;\n"
    "uniform vec2 uSize;\n"
    "out vec2 vUV;\n"
    "void main() {\n"
    "    vec3 world = uCenter + uRight * (aCorner.x * uSize.x)\n"
    "                         + uUp    * (aCorner.y * uSize.y);\n"
    "    gl_Position = uVP * vec4(world, 1.0);\n"
    "    vUV = aUV;\n"
    "}\n";

static const char *BB_FRAG_SRC =
    GLSL_FS
    "in vec2 vUV;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTex;\n"
    "uniform vec4 uTint;\n"
    "void main() {\n"
    "    vec4 c = texture(uTex, vUV);\n"
    "    if (c.a < 0.5) discard;\n"
    "    if (uTint.a > 0.0) c.rgb *= uTint.rgb;\n"
    "    FragColor = vec4(c.rgb, 1.0);\n"
    "}\n";

/* Sky-gradient shader. Two fullscreen triangles in NDC; alpha attribute carries
   the per-vertex sky color blend factor (0 = bottom, 1 = top). */
static const char *SKY_VERT_SRC =
    GLSL_VS
    "layout(location=0) in vec2 aPos;\n"
    "layout(location=1) in float aT;\n"
    "out float vT;\n"
    "void main() { gl_Position = vec4(aPos, 0.0, 1.0); vT = aT; }\n";

static const char *SKY_FRAG_SRC =
    GLSL_FS
    "in float vT;\n"
    "out vec4 FragColor;\n"
    "uniform vec3 uTop;\n"
    "uniform vec3 uBot;\n"
    "void main() { FragColor = vec4(mix(uBot, uTop, vT), 1.0); }\n";

/* Screen-space solid color rectangle shader. Coordinates are already NDC. */
static const char *RECT_VERT_SRC =
    GLSL_VS
    "layout(location=0) in vec2 aPos;\n"
    "void main() { gl_Position = vec4(aPos, 0.0, 1.0); }\n";

static const char *RECT_FRAG_SRC =
    GLSL_FS
    "out vec4 FragColor;\n"
    "uniform vec4 uColor;\n"
    "void main() { FragColor = uColor; }\n";

/* Lit + textured shader — pos(0) uv(1) normal(2) */
static const char *LIT_VERT_SRC =
    GLSL_VS
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec2 aUV;\n"
    "layout(location=2) in vec3 aNorm;\n"
    "uniform mat4 uMVP;\n"
    "uniform mat4 uModel;\n"
    "out vec2 vUV;\n"
    "out vec3 vNorm;\n"
    "void main() {\n"
    "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "    vUV   = aUV;\n"
    "    vNorm = mat3(uModel) * aNorm;\n"
    "}\n";

static const char *LIT_FRAG_SRC =
    GLSL_FS
    "in vec2 vUV;\n"
    "in vec3 vNorm;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTex;\n"
    "uniform vec3 uLightDir;\n"
    "uniform vec3 uTint;\n"          /* per-material diffuse multiplier */
    "uniform int  uHasTex;\n"        /* 0 = ignore uTex, use uTint only */
    "uniform int  uLightingOn;\n"    /* Phase 12: D3D LIGHTING render-state */
    "uniform vec3 uAmbient;\n"       /* measured D3DRS_AMBIENT (0..1) */
    "uniform vec3 uLightDiffuse;\n"  /* measured DIR-light diffuse */
    "uniform vec3 uSceneTint;\n"     /* Phase 1: per-channel ambient/scene multiplier */
    "uniform int  uAlphaCutout;\n"   /* Phase 4: 1 = discard fragments with tex.a < threshold */
    "uniform float uAlphaThreshold;\n"
    "void main() {\n"
    "    vec3 n = normalize(vNorm);\n"
    "    /* Match the original's measured lighting. With LIGHTING OFF (Phase 12\n"
    "       canon for Level 1) the scene is flat, full-bright texture-lit -- no\n"
    "       diffuse term -- which fixes the demo's over-dark directional shading.\n"
    "       When enabled, ambient floor + measured directional diffuse. */\n"
    "    vec3 lightTerm;\n"
    "    if (uLightingOn == 0) {\n"
    "        lightTerm = vec3(1.0);\n"
    "    } else {\n"
    "        float diff = max(dot(n, uLightDir), 0.0);\n"
    "        lightTerm = uAmbient + uLightDiffuse * diff;\n"
    "    }\n"
    "    vec3 base = uTint;\n"
    "    float alpha = 1.0;\n"
    "    if (uHasTex != 0) {\n"
    "        vec4 c = texture(uTex, vUV);\n"
    "        if (uAlphaCutout != 0 && c.a < uAlphaThreshold) discard;\n"
    "        base *= c.rgb;\n"
    "        alpha = c.a;\n"
    "    }\n"
    "    /* Phase 1: scene-wide tint applied AFTER the lighting term so it acts\n"
    "       as a measured ambient/sky cast on both lit and unlit paths.\n"
    "       Phase 4: alpha forced to 1.0 (live-window X compositor safety; the\n"
    "       alpha cutout above already handles transparency via discard). */\n"
    "    FragColor = vec4(base * lightTerm * uSceneTint, 1.0);\n"
    "}\n";

static unsigned int compile_shader(GLenum type, const char *src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, NULL, log);
        fprintf(stderr, "shader error: %s\n", log);
    }
    return s;
}

/* ---- module state ---- */
static unsigned int g_prog = 0;
static int g_loc_mvp = -1, g_loc_color = -1;

static unsigned int g_lit_prog = 0;
static int g_lit_loc_mvp = -1, g_lit_loc_model = -1;
static int g_lit_loc_tex = -1, g_lit_loc_light = -1;
static int g_lit_loc_tint = -1, g_lit_loc_hastex = -1;
static int g_lit_loc_lighton = -1, g_lit_loc_ambient = -1, g_lit_loc_ldiff = -1;
static int g_lit_loc_scene_tint = -1;
static int g_lit_loc_alpha_cutout = -1, g_lit_loc_alpha_threshold = -1;
static float g_scene_tint[3] = { 1.0f, 1.0f, 1.0f };
/* Phase 4: alpha-cutout state. When enabled, the lit fragment shader
 * discards fragments whose sampled texture alpha is below the threshold.
 * Used for capture-derived foliage / playground billboards whose alpha
 * channel is the original D3D7 color-key, decoded by the proxy. */
static int   g_alpha_cutout = 0;
static float g_alpha_threshold = 0.5f;
/* When non-zero, untextured material groups inside multi-material meshes are
   skipped instead of rendered as flat-tinted slabs. Set by native Level 1 so
   collision-only volumes and unresolved canvas slots (BLOCKING_*, SCHOOL
   cross-level slots, etc.) don't dominate the view. */
static int g_hide_untextured_groups = 0;

static unsigned int g_sky_prog = 0, g_sky_vao = 0, g_sky_vbo = 0;
static int g_sky_loc_top = -1, g_sky_loc_bot = -1;

static unsigned int g_rect_prog = 0, g_rect_vao = 0, g_rect_vbo = 0;
static int g_rect_loc_color = -1;

/* Billboard program state. */
static unsigned int g_bb_prog = 0, g_bb_vao = 0, g_bb_vbo = 0;
static int g_bb_loc_vp = -1, g_bb_loc_center = -1, g_bb_loc_right = -1;
static int g_bb_loc_up = -1, g_bb_loc_size = -1, g_bb_loc_tex = -1, g_bb_loc_tint = -1;
/* Cached camera basis vectors recomputed in begin_frame. */
static float g_cam_right[3] = { 1.0f, 0.0f, 0.0f };
static float g_cam_up[3]    = { 0.0f, 1.0f, 0.0f };
static float g_sky_top[3] = { 0.45f, 0.70f, 0.95f };
static float g_sky_bot[3] = { 0.85f, 0.92f, 0.98f };

static Camera g_cam;
static Mat4 g_proj, g_view, g_view_proj;

/* M7a camera override: when active, begin_frame ignores the follow camera and
   uses these GL column-major matrices directly (matched-camera capture). */
static int  g_cam_override = 0;
static Mat4 g_override_view, g_override_proj;

void renderer_set_camera_override(const float view[16], const float proj[16]) {
    if (view && proj) {
        memcpy(g_override_view, view, 64);
        memcpy(g_override_proj, proj, 64);
        g_cam_override = 1;
    } else {
        g_cam_override = 0;
    }
}

int renderer_camera_override_active(void) { return g_cam_override; }

int renderer_init(int w, int h) {
    /* Flat-color program */
    unsigned int vs = compile_shader(GL_VERTEX_SHADER, VERT_SRC);
    unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, FRAG_SRC);
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs); glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    int ok; glGetProgramiv(g_prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetProgramInfoLog(g_prog, 512, NULL, log); fprintf(stderr, "flat link: %s\n", log); return 0; }
    glDeleteShader(vs); glDeleteShader(fs);
    g_loc_mvp   = glGetUniformLocation(g_prog, "uMVP");
    g_loc_color = glGetUniformLocation(g_prog, "uColor");

    /* Lit+textured program */
    unsigned int lvs = compile_shader(GL_VERTEX_SHADER, LIT_VERT_SRC);
    unsigned int lfs = compile_shader(GL_FRAGMENT_SHADER, LIT_FRAG_SRC);
    g_lit_prog = glCreateProgram();
    glAttachShader(g_lit_prog, lvs); glAttachShader(g_lit_prog, lfs);
    glLinkProgram(g_lit_prog);
    glGetProgramiv(g_lit_prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetProgramInfoLog(g_lit_prog, 512, NULL, log); fprintf(stderr, "lit link: %s\n", log); return 0; }
    glDeleteShader(lvs); glDeleteShader(lfs);
    g_lit_loc_mvp    = glGetUniformLocation(g_lit_prog, "uMVP");
    g_lit_loc_model  = glGetUniformLocation(g_lit_prog, "uModel");
    g_lit_loc_tex    = glGetUniformLocation(g_lit_prog, "uTex");
    g_lit_loc_light  = glGetUniformLocation(g_lit_prog, "uLightDir");
    g_lit_loc_tint   = glGetUniformLocation(g_lit_prog, "uTint");
    g_lit_loc_hastex = glGetUniformLocation(g_lit_prog, "uHasTex");
    g_lit_loc_lighton = glGetUniformLocation(g_lit_prog, "uLightingOn");
    g_lit_loc_ambient = glGetUniformLocation(g_lit_prog, "uAmbient");
    g_lit_loc_ldiff   = glGetUniformLocation(g_lit_prog, "uLightDiffuse");
    g_lit_loc_scene_tint = glGetUniformLocation(g_lit_prog, "uSceneTint");
    g_lit_loc_alpha_cutout    = glGetUniformLocation(g_lit_prog, "uAlphaCutout");
    g_lit_loc_alpha_threshold = glGetUniformLocation(g_lit_prog, "uAlphaThreshold");

    /* Sky-gradient program + fullscreen quad. Each vertex carries an NDC
       position and a t value (1 at top, 0 at bottom) used to lerp top/bot. */
    unsigned int svs = compile_shader(GL_VERTEX_SHADER, SKY_VERT_SRC);
    unsigned int sfs = compile_shader(GL_FRAGMENT_SHADER, SKY_FRAG_SRC);
    g_sky_prog = glCreateProgram();
    glAttachShader(g_sky_prog, svs); glAttachShader(g_sky_prog, sfs);
    glLinkProgram(g_sky_prog);
    glGetProgramiv(g_sky_prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetProgramInfoLog(g_sky_prog, 512, NULL, log); fprintf(stderr, "sky link: %s\n", log); return 0; }
    glDeleteShader(svs); glDeleteShader(sfs);
    g_sky_loc_top = glGetUniformLocation(g_sky_prog, "uTop");
    g_sky_loc_bot = glGetUniformLocation(g_sky_prog, "uBot");

    {
        /* (x, y, t) — two triangles covering NDC, t=1 at top, t=0 at bottom. */
        const float verts[] = {
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
             1.0f,  1.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,
             1.0f,  1.0f, 1.0f,
            -1.0f,  1.0f, 1.0f,
        };
        glGenVertexArrays(1, &g_sky_vao);
        glGenBuffers(1, &g_sky_vbo);
        glBindVertexArray(g_sky_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_sky_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    /* Billboard program + unit quad. Vertex layout: (corner.xy, uv.xy). */
    {
        unsigned int bvs = compile_shader(GL_VERTEX_SHADER, BB_VERT_SRC);
        unsigned int bfs = compile_shader(GL_FRAGMENT_SHADER, BB_FRAG_SRC);
        g_bb_prog = glCreateProgram();
        glAttachShader(g_bb_prog, bvs); glAttachShader(g_bb_prog, bfs);
        glLinkProgram(g_bb_prog);
        glGetProgramiv(g_bb_prog, GL_LINK_STATUS, &ok);
        if (!ok) { char log[512]; glGetProgramInfoLog(g_bb_prog, 512, NULL, log); fprintf(stderr, "bb link: %s\n", log); return 0; }
        glDeleteShader(bvs); glDeleteShader(bfs);
        g_bb_loc_vp     = glGetUniformLocation(g_bb_prog, "uVP");
        g_bb_loc_center = glGetUniformLocation(g_bb_prog, "uCenter");
        g_bb_loc_right  = glGetUniformLocation(g_bb_prog, "uRight");
        g_bb_loc_up     = glGetUniformLocation(g_bb_prog, "uUp");
        g_bb_loc_size   = glGetUniformLocation(g_bb_prog, "uSize");
        g_bb_loc_tex    = glGetUniformLocation(g_bb_prog, "uTex");
        g_bb_loc_tint   = glGetUniformLocation(g_bb_prog, "uTint");

        /* Unit quad (-0.5..0.5) with UV (0..1). V is flipped because
           tex_loader uses stbi_set_flip_vertically_on_load(1). */
        const float verts[] = {
            /* corner.x  corner.y    u    v */
            -0.5f, -0.5f,             0.0f, 0.0f,
             0.5f, -0.5f,             1.0f, 0.0f,
             0.5f,  0.5f,             1.0f, 1.0f,
            -0.5f, -0.5f,             0.0f, 0.0f,
             0.5f,  0.5f,             1.0f, 1.0f,
            -0.5f,  0.5f,             0.0f, 1.0f,
        };
        glGenVertexArrays(1, &g_bb_vao);
        glGenBuffers(1, &g_bb_vbo);
        glBindVertexArray(g_bb_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_bb_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    {
        unsigned int rvs = compile_shader(GL_VERTEX_SHADER, RECT_VERT_SRC);
        unsigned int rfs = compile_shader(GL_FRAGMENT_SHADER, RECT_FRAG_SRC);
        g_rect_prog = glCreateProgram();
        glAttachShader(g_rect_prog, rvs); glAttachShader(g_rect_prog, rfs);
        glLinkProgram(g_rect_prog);
        glGetProgramiv(g_rect_prog, GL_LINK_STATUS, &ok);
        if (!ok) { char log[512]; glGetProgramInfoLog(g_rect_prog, 512, NULL, log); fprintf(stderr, "rect link: %s\n", log); return 0; }
        glDeleteShader(rvs); glDeleteShader(rfs);
        g_rect_loc_color = glGetUniformLocation(g_rect_prog, "uColor");
        glGenVertexArrays(1, &g_rect_vao);
        glGenBuffers(1, &g_rect_vbo);
        glBindVertexArray(g_rect_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_rect_vbo);
        glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), NULL, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    /* Default camera */
    g_cam.pos[0] = 0; g_cam.pos[1] = 200; g_cam.pos[2] = 400;
    g_cam.yaw    = 0;
    g_cam.pitch  = -0.3f;
    g_cam.fov    = 1.0472f; /* 60 deg */
    g_cam.near_z = 1.0f;
    g_cam.far_z  = 50000.0f;

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, w, h);
    return 1;
}

void renderer_destroy(void) {
    if (g_prog)     { glDeleteProgram(g_prog);     g_prog = 0; }
    if (g_lit_prog) { glDeleteProgram(g_lit_prog); g_lit_prog = 0; }
    if (g_sky_prog) { glDeleteProgram(g_sky_prog); g_sky_prog = 0; }
    if (g_sky_vao)  { glDeleteVertexArrays(1, &g_sky_vao); g_sky_vao = 0; }
    if (g_sky_vbo)  { glDeleteBuffers(1, &g_sky_vbo);      g_sky_vbo = 0; }
    if (g_bb_prog)  { glDeleteProgram(g_bb_prog);  g_bb_prog = 0; }
    if (g_bb_vao)   { glDeleteVertexArrays(1, &g_bb_vao); g_bb_vao = 0; }
    if (g_bb_vbo)   { glDeleteBuffers(1, &g_bb_vbo);      g_bb_vbo = 0; }
    if (g_rect_prog){ glDeleteProgram(g_rect_prog); g_rect_prog = 0; }
    if (g_rect_vao) { glDeleteVertexArrays(1, &g_rect_vao); g_rect_vao = 0; }
    if (g_rect_vbo) { glDeleteBuffers(1, &g_rect_vbo);      g_rect_vbo = 0; }
}

void renderer_set_sky(float tr, float tg, float tb,
                      float br, float bg, float bb) {
    g_sky_top[0] = tr; g_sky_top[1] = tg; g_sky_top[2] = tb;
    g_sky_bot[0] = br; g_sky_bot[1] = bg; g_sky_bot[2] = bb;
}

void renderer_set_scene_tint(float r, float g, float b) {
    g_scene_tint[0] = r;
    g_scene_tint[1] = g;
    g_scene_tint[2] = b;
}

void renderer_set_hide_untextured_groups(int enable) {
    g_hide_untextured_groups = enable ? 1 : 0;
}

void renderer_set_alpha_cutout(int enable, float threshold) {
    g_alpha_cutout = enable ? 1 : 0;
    if (threshold > 0.0f && threshold < 1.0f) g_alpha_threshold = threshold;
}

static void renderer_prepare_camera(int w, int h) {
    glViewport(0, 0, w, h);
    if (g_cam_override) {
        /* Matched-camera capture: use the supplied matrices verbatim. */
        memcpy(g_proj, g_override_proj, 64);
        memcpy(g_view, g_override_view, 64);
    } else {
        mat4_perspective(g_proj, g_cam.fov, (float)w / (float)h,
                         g_cam.near_z, g_cam.far_z);

        /* Compute look-at from yaw/pitch */
        float cy = cosf(g_cam.yaw),   sy = sinf(g_cam.yaw);
        float cp = cosf(g_cam.pitch), sp = sinf(g_cam.pitch);
        float dx = sy * cp, dy = sp, dz = -cy * cp;
        float lx = g_cam.pos[0] + dx;
        float ly = g_cam.pos[1] + dy;
        float lz = g_cam.pos[2] + dz;
        mat4_lookat(g_view, g_cam.pos[0], g_cam.pos[1], g_cam.pos[2],
                    lx, ly, lz);
    }
    mat4_mul(g_view_proj, g_proj, g_view);

    /* Cache camera right/up for billboard math. Right comes straight from the
       view matrix row 0; up is fixed world-up so sprites stay vertical. */
    g_cam_right[0] = g_view[0]; g_cam_right[1] = g_view[4]; g_cam_right[2] = g_view[8];
    g_cam_up[0] = 0.0f; g_cam_up[1] = 1.0f; g_cam_up[2] = 0.0f;

    capture_set_camera(g_view, g_proj);
}

void renderer_begin_frame(int w, int h) {
    glViewport(0, 0, w, h);
    glClearColor(g_sky_bot[0], g_sky_bot[1], g_sky_bot[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Sky gradient — covers the screen behind everything. Depth writes off so
       subsequent depth-tested geometry always wins. */
    if (g_sky_prog && g_sky_vao) {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glUseProgram(g_sky_prog);
        glUniform3f(g_sky_loc_top, g_sky_top[0], g_sky_top[1], g_sky_top[2]);
        glUniform3f(g_sky_loc_bot, g_sky_bot[0], g_sky_bot[1], g_sky_bot[2]);
        glBindVertexArray(g_sky_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    }

    renderer_prepare_camera(w, h);
}

void renderer_begin_overlay(int w, int h) {
    renderer_prepare_camera(w, h);
}

void renderer_draw_billboard(unsigned int tex,
                             float tx, float ty, float tz,
                             float width, float height,
                             float tint_r, float tint_g, float tint_b, float tint_a) {
    if (!tex || !g_bb_prog) return;
    if (capture_active()) {
        const float bmodel[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, tx,ty,tz,1 };
        float hw = width * 0.5f, hh = height * 0.5f;
        const float bmin[3] = { -hw, -hh, -hw };
        const float bmax[3] = {  hw,  hh,  hw };
        capture_draw(bmodel, tex, bmin, bmax);
    }
    glUseProgram(g_bb_prog);
    glUniformMatrix4fv(g_bb_loc_vp, 1, GL_FALSE, g_view_proj);
    glUniform3f(g_bb_loc_center, tx, ty, tz);
    glUniform3f(g_bb_loc_right, g_cam_right[0], g_cam_right[1], g_cam_right[2]);
    glUniform3f(g_bb_loc_up,    g_cam_up[0],    g_cam_up[1],    g_cam_up[2]);
    glUniform2f(g_bb_loc_size, width, height);
    glUniform4f(g_bb_loc_tint, tint_r, tint_g, tint_b, tint_a);
    glUniform1i(g_bb_loc_tex, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindVertexArray(g_bb_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderer_draw_screen_rect(int viewport_w, int viewport_h,
                               float x, float y, float width, float height,
                               float r, float g, float b, float a) {
    if (!g_rect_prog || !g_rect_vao || viewport_w <= 0 || viewport_h <= 0)
        return;
    if (width <= 0.0f || height <= 0.0f) return;

    float x0 = (x / (float)viewport_w) * 2.0f - 1.0f;
    float x1 = ((x + width) / (float)viewport_w) * 2.0f - 1.0f;
    float y0 = 1.0f - (y / (float)viewport_h) * 2.0f;
    float y1 = 1.0f - ((y + height) / (float)viewport_h) * 2.0f;
    const float verts[12] = {
        x0, y0, x1, y0, x1, y1,
        x0, y0, x1, y1, x0, y1,
    };

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    if (a < 1.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
    glUseProgram(g_rect_prog);
    glUniform4f(g_rect_loc_color, r, g, b, a);
    glBindVertexArray(g_rect_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_rect_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void renderer_get_view_proj(float out[16]) {
    memcpy(out, g_view_proj, 64);
}

void renderer_draw_model_matrix(const AseModel *m, unsigned int texture_id_override,
                                const float model[16]) {
    Mat4 mvp, vp;
    mat4_mul(vp, g_proj, g_view);
    mat4_mul(mvp, vp, model);

    if (capture_active()) {
        unsigned int cap_tex = texture_id_override ? texture_id_override
            : (m->texture_id ? m->texture_id
               : (m->material_count > 0 ? m->materials[0].texture_id : 0));
        capture_draw(model, cap_tex, m->min, m->max);
    }

    /* Multi-material path (Phase 10). Every group uses the lit shader with a
       per-material diffuse tint; the uHasTex uniform toggles texture sampling
       vs. flat color so untextured materials still receive lighting. */
    glBindVertexArray(m->vao);
    glUseProgram(g_lit_prog);
    glUniformMatrix4fv(g_lit_loc_mvp,   1, GL_FALSE, mvp);
    glUniformMatrix4fv(g_lit_loc_model, 1, GL_FALSE, model);
    glUniform1i(g_lit_loc_tex, 0);
    /* Phase 12: lighting driven by measured canon (build/canon.json). For
       Level 1 the original ran LIGHTING OFF, so uLightingOn=0 -> flat. */
    glUniform1i(g_lit_loc_lighton, CANON_LIGHTING_ENABLED);
    glUniform3f(g_lit_loc_light, CANON_LIGHT_DIR_X, CANON_LIGHT_DIR_Y, CANON_LIGHT_DIR_Z);
    glUniform3f(g_lit_loc_ambient, CANON_AMBIENT_R, CANON_AMBIENT_G, CANON_AMBIENT_B);
    glUniform3f(g_lit_loc_ldiff, CANON_LIGHT_DIFF_R, CANON_LIGHT_DIFF_G, CANON_LIGHT_DIFF_B);
    glUniform3f(g_lit_loc_scene_tint, g_scene_tint[0], g_scene_tint[1], g_scene_tint[2]);
    glUniform1i(g_lit_loc_alpha_cutout, g_alpha_cutout);
    glUniform1f(g_lit_loc_alpha_threshold, g_alpha_threshold);
    glActiveTexture(GL_TEXTURE0);

    int groups = m->material_count > 0 ? m->material_count : 1;
    for (int g = 0; g < groups; g++) {
        int offset_idx, count_idx;
        unsigned int group_tex;
        float tr = 1.0f, tg = 1.0f, tb = 1.0f;
        if (m->material_count > 0) {
            const AseMaterial *am = &m->materials[g];
            offset_idx = am->index_offset;
            count_idx  = am->index_count;
            tr = am->diffuse[0]; tg = am->diffuse[1]; tb = am->diffuse[2];
            /* Texture resolution priority:
               1. caller override (single texture for everything),
               2. per-material texture (multi-material auth from OMT),
               3. model-level texture (legacy ASEs: Jimmy, NPCs all share
                  one BMP across many materials with unresolvable paths),
               4. untextured (use per-material diffuse as flat color).
               When falling back to (1) or (3) we neutralize the tint — the
               diffuse was authored to multiply a *different* texture and
               applying it to the substitute would mistint everything. */
            if (texture_id_override) {
                group_tex = texture_id_override;
                tr = tg = tb = 1.0f;
            } else if (am->texture_id) {
                group_tex = am->texture_id;
            } else if (m->texture_id) {
                group_tex = m->texture_id;
                tr = tg = tb = 1.0f;
            } else {
                group_tex = 0;
            }
        } else {
            offset_idx = 0;
            count_idx  = m->index_count;
            group_tex  = texture_id_override ? texture_id_override : m->texture_id;
        }
        if (count_idx <= 0) continue;

        /* Skip groups that are fully transparent (untextured + black tint).
           OMT untextured materials sometimes carry (0,0,0) for invisible
           helper geometry; rendering them produces ugly black slabs. */
        if (!group_tex && tr == 0.0f && tg == 0.0f && tb == 0.0f) continue;
        if (!group_tex && g_hide_untextured_groups) continue;

        glUniform3f(g_lit_loc_tint, tr, tg, tb);
        if (group_tex) {
            glUniform1i(g_lit_loc_hastex, 1);
            glBindTexture(GL_TEXTURE_2D, group_tex);
        } else {
            glUniform1i(g_lit_loc_hastex, 0);
        }

        glDrawElements(GL_TRIANGLES, count_idx, GL_UNSIGNED_INT,
                       (void*)(size_t)(offset_idx * sizeof(unsigned int)));
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderer_draw_model(const AseModel *m, unsigned int texture_id_override,
                         float tx, float ty, float tz, float yaw, float scale) {
    Mat4 model;
    /* Y-axis rotation by yaw, with uniform scale, then translation. Column-major. */
    float c = cosf(yaw) * scale, s = sinf(yaw) * scale;
    model[0]= c;  model[1]=0;     model[2]=-s;     model[3]=0;
    model[4]= 0;  model[5]=scale; model[6]= 0;     model[7]=0;
    model[8]= s;  model[9]=0;     model[10]=c;     model[11]=0;
    model[12]=tx; model[13]=ty;   model[14]=tz;    model[15]=1;
    renderer_draw_model_matrix(m, texture_id_override, model);
}

void renderer_draw_box(unsigned int vao, int index_count,
                       float tx, float ty, float tz, float scale,
                       float r, float g, float b) {
    Mat4 model, mvp, vp;
    mat4_identity(model);
    model[0]=scale; model[5]=scale; model[10]=scale;
    model[12]=tx; model[13]=ty; model[14]=tz;
    mat4_mul(vp, g_proj, g_view);
    mat4_mul(mvp, vp, model);
    if (capture_active()) {
        const float bmin[3] = { -1.0f, -1.0f, -1.0f };
        const float bmax[3] = {  1.0f,  1.0f,  1.0f };
        capture_draw(model, 0, bmin, bmax);
    }
    glUseProgram(g_prog);
    glUniformMatrix4fv(g_loc_mvp, 1, GL_FALSE, mvp);
    glUniform4f(g_loc_color, r, g, b, 1.0f);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void renderer_end_frame(void) {
    glUseProgram(0);
}


Camera *renderer_camera(void) { return &g_cam; }
