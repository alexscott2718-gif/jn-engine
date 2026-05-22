#include "ground.h"
#include "glad.h"
#include "renderer.h"
#include "capture.h"
#include <stdio.h>
#include <string.h>

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

static unsigned int g_prog = 0, g_vao = 0, g_vbo = 0;
static int g_loc_mvp = -1, g_loc_tex = -1, g_loc_use = -1, g_loc_tint = -1;
static unsigned int g_tex = 0;
static float g_cx = 0.0f, g_cz = 0.0f;
static float g_half = 0.0f;

int ground_init(unsigned int texture_id, float half_size,
                float center_x, float center_z, float tile_repeat) {
    g_tex  = texture_id;
    g_cx   = center_x;
    g_cz   = center_z;
    g_half = half_size;

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

    float r = tile_repeat;
    float h = half_size;
    const float verts[] = {
        -h, 0.0f, -h,   0.0f, 0.0f,
         h, 0.0f, -h,      r, 0.0f,
         h, 0.0f,  h,      r,    r,
        -h, 0.0f, -h,   0.0f, 0.0f,
         h, 0.0f,  h,      r,    r,
        -h, 0.0f,  h,   0.0f,    r,
    };
    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    return 1;
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

    if (capture_active()) {
        /* The ground quad spans [-half,0,-half] .. [half,0,half] in local space. */
        const float gmin[3] = { -g_half, 0.0f, -g_half };
        const float gmax[3] = {  g_half, 0.0f,  g_half };
        capture_draw(model, g_tex, gmin, gmax);
    }

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
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    if (g_tex) glBindTexture(GL_TEXTURE_2D, 0);
}

void ground_destroy(void) {
    if (g_prog) { glDeleteProgram(g_prog); g_prog = 0; }
    if (g_vao)  { glDeleteVertexArrays(1, &g_vao); g_vao = 0; }
    if (g_vbo)  { glDeleteBuffers(1, &g_vbo);      g_vbo = 0; }
}
