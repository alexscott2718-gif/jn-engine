#include "gltf_loader.h"
#include "../glad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CGLTF_IMPLEMENTATION
#include "../cgltf.h"
/* stb_image implementation lives in tex_loader.c; we only need the decls. */
#include "../stb_image.h"

/* Same fixed-capacity scratch as ase_loader so we don't churn the heap during
   level load. Output is interleaved pos(3)+uv(2)+normal(3) = 8 floats/vertex,
   de-indexed (one emitted vertex per glTF index, sequential EBO). */
#define GLTF_MAX_VERTS (65536 * 3)
static float        s_vbuf[GLTF_MAX_VERTS * 8];
static unsigned int s_ibuf[GLTF_MAX_VERTS];

/* Per-material draw-group metadata gathered during the CPU parse. */
typedef struct {
    int                   index_offset;   /* in indices */
    int                   index_count;    /* in indices */
    float                 diffuse[3];
    const unsigned char  *png;            /* embedded PNG bytes, or NULL */
    int                   png_len;
    char                  tex_label[256]; /* image name (diagnostic) */
} GltfGroup;

/* CPU parse result (no GL). All vertex data is written into s_vbuf/s_ibuf. */
typedef struct {
    int       vertex_count;
    int       group_count;
    GltfGroup groups[ASE_MAX_MATERIALS];
    float     min[3], max[3], center[3];
} GltfCpu;

static const cgltf_attribute *find_attr(const cgltf_primitive *prim,
                                        cgltf_attribute_type type) {
    for (cgltf_size i = 0; i < prim->attributes_count; i++)
        if (prim->attributes[i].type == type)
            return &prim->attributes[i];
    return NULL;
}

/* Parse a .glb into s_vbuf/s_ibuf + group metadata. No OpenGL calls. Returns 1
   on success. *out is fully populated; cgltf_data ownership is returned via
   *data_out (caller must cgltf_free it once it's done reading embedded PNGs). */
static int parse_glb(const char *path, GltfCpu *out, cgltf_data **data_out) {
    memset(out, 0, sizeof(*out));
    *data_out = NULL;

    cgltf_options opts;
    memset(&opts, 0, sizeof(opts));
    cgltf_data *data = NULL;
    if (cgltf_parse_file(&opts, path, &data) != cgltf_result_success) {
        fprintf(stderr, "gltf_load: parse failed %s\n", path);
        return 0;
    }
    if (cgltf_load_buffers(&opts, data, path) != cgltf_result_success) {
        fprintf(stderr, "gltf_load: load_buffers failed %s\n", path);
        cgltf_free(data);
        return 0;
    }
    if (data->meshes_count == 0) {
        fprintf(stderr, "gltf_load: no meshes in %s\n", path);
        cgltf_free(data);
        return 0;
    }

    const cgltf_mesh *mesh = &data->meshes[0];
    float bmin[3] = { 1e30f,  1e30f,  1e30f};
    float bmax[3] = {-1e30f, -1e30f, -1e30f};
    int voff = 0;
    int gc = 0;

    for (cgltf_size pi = 0; pi < mesh->primitives_count; pi++) {
        if (gc >= ASE_MAX_MATERIALS) break;
        const cgltf_primitive *prim = &mesh->primitives[pi];
        const cgltf_attribute *pa = find_attr(prim, cgltf_attribute_type_position);
        if (!pa || !pa->data) continue;
        const cgltf_attribute *na = find_attr(prim, cgltf_attribute_type_normal);
        const cgltf_attribute *ta = find_attr(prim, cgltf_attribute_type_texcoord);

        cgltf_size n = prim->indices ? prim->indices->count
                                     : pa->data->count;
        if (n == 0) continue;

        GltfGroup *g = &out->groups[gc];
        g->index_offset = voff;

        for (cgltf_size i = 0; i < n; i++) {
            if (voff >= GLTF_MAX_VERTS) {
                fprintf(stderr, "gltf_load: %s exceeds vertex cap\n", path);
                cgltf_free(data);
                return 0;
            }
            cgltf_size vi = prim->indices ? cgltf_accessor_read_index(prim->indices, i)
                                          : i;
            float pos[3] = {0,0,0}, nrm[3] = {0,0,0}, uv[2] = {0,0};
            cgltf_accessor_read_float(pa->data, vi, pos, 3);
            if (na && na->data) cgltf_accessor_read_float(na->data, vi, nrm, 3);
            if (ta && ta->data) cgltf_accessor_read_float(ta->data, vi, uv, 2);

            float *vb = &s_vbuf[voff * 8];
            vb[0] = pos[0]; vb[1] = pos[1]; vb[2] = pos[2];
            vb[3] = uv[0];  vb[4] = uv[1];
            vb[5] = nrm[0]; vb[6] = nrm[1]; vb[7] = nrm[2];
            s_ibuf[voff] = (unsigned int)voff;

            for (int d = 0; d < 3; d++) {
                if (pos[d] < bmin[d]) bmin[d] = pos[d];
                if (pos[d] > bmax[d]) bmax[d] = pos[d];
            }
            voff++;
        }
        g->index_count = (int)n;

        /* Material: diffuse + embedded baseColorTexture PNG bytes. */
        g->diffuse[0] = g->diffuse[1] = g->diffuse[2] = 1.0f;
        g->png = NULL; g->png_len = 0; g->tex_label[0] = '\0';
        const cgltf_material *mat = prim->material;
        if (mat && mat->has_pbr_metallic_roughness) {
            const cgltf_pbr_metallic_roughness *pbr = &mat->pbr_metallic_roughness;
            g->diffuse[0] = pbr->base_color_factor[0];
            g->diffuse[1] = pbr->base_color_factor[1];
            g->diffuse[2] = pbr->base_color_factor[2];
            const cgltf_texture *tex = pbr->base_color_texture.texture;
            if (tex && tex->image && tex->image->buffer_view) {
                const cgltf_buffer_view *bv = tex->image->buffer_view;
                const unsigned char *base = bv->data
                    ? (const unsigned char *)bv->data
                    : (const unsigned char *)bv->buffer->data + bv->offset;
                g->png = base;
                g->png_len = (int)bv->size;
                if (tex->image->name)
                    snprintf(g->tex_label, sizeof(g->tex_label), "%s",
                             tex->image->name);
            }
        }
        gc++;
    }

    if (voff == 0 || gc == 0) {
        fprintf(stderr, "gltf_load: no drawable primitives in %s\n", path);
        cgltf_free(data);
        return 0;
    }

    out->vertex_count = voff;
    out->group_count = gc;
    for (int d = 0; d < 3; d++) {
        out->min[d] = bmin[d];
        out->max[d] = bmax[d];
        out->center[d] = (bmin[d] + bmax[d]) * 0.5f;
    }
    *data_out = data;
    return 1;
}

/* Decode an embedded PNG (top-down — glTF UV origin is top-left, so NO
   vertical flip, unlike tex_loader's ASE path) and upload a GL texture. */
static unsigned int upload_embedded_png(const unsigned char *png, int len) {
    int w, h, ch;
    stbi_set_flip_vertically_on_load(0);
    unsigned char *data = stbi_load_from_memory(png, len, &w, &h, &ch, 0);
    if (!data) {
        fprintf(stderr, "gltf_load: embedded PNG decode failed: %s\n",
                stbi_failure_reason());
        return 0;
    }
    GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return id;
}

int gltf_load(AseModel *m, const char *path) {
    memset(m, 0, sizeof(*m));

    GltfCpu cpu;
    cgltf_data *data = NULL;
    if (!parse_glb(path, &cpu, &data)) return 0;

    for (int d = 0; d < 3; d++) {
        m->min[d] = cpu.min[d];
        m->max[d] = cpu.max[d];
        m->center[d] = cpu.center[d];
    }

    glGenVertexArrays(1, &m->vao);
    glGenBuffers(1, &m->vbo);
    glGenBuffers(1, &m->ebo);
    glBindVertexArray(m->vao);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glBufferData(GL_ARRAY_BUFFER, cpu.vertex_count * 8 * sizeof(float),
                 s_vbuf, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cpu.vertex_count * sizeof(unsigned int),
                 s_ibuf, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
    glBindVertexArray(0);
    m->index_count = cpu.vertex_count;

    m->material_count = cpu.group_count;
    for (int k = 0; k < cpu.group_count; k++) {
        AseMaterial *am = &m->materials[k];
        const GltfGroup *g = &cpu.groups[k];
        am->index_offset = g->index_offset;
        am->index_count  = g->index_count;
        am->diffuse[0] = g->diffuse[0];
        am->diffuse[1] = g->diffuse[1];
        am->diffuse[2] = g->diffuse[2];
        am->texture_id = g->png ? upload_embedded_png(g->png, g->png_len) : 0;
        snprintf(am->tex_file, sizeof(am->tex_file), "%s", g->tex_label);
    }
    /* Legacy single-bitmap mirror for callers reading m->tex_file/texture_id. */
    for (int k = 0; k < cpu.group_count; k++) {
        if (m->materials[k].texture_id) {
            m->texture_id = m->materials[k].texture_id;
            snprintf(m->tex_file, sizeof(m->tex_file), "%s",
                     m->materials[k].tex_file);
            break;
        }
    }

    cgltf_free(data);
    printf("gltf_load: %s  verts=%d  mats=%d\n", path,
           cpu.vertex_count, cpu.group_count);
    return 1;
}

int gltf_inspect(const char *path, GltfInfo *info) {
    GltfCpu cpu;
    cgltf_data *data = NULL;
    if (!parse_glb(path, &cpu, &data)) return 0;
    if (info) {
        memset(info, 0, sizeof(*info));
        info->vertex_count = cpu.vertex_count;
        info->material_count = cpu.group_count;
        info->first_index_count = cpu.groups[0].index_count;
        info->first_has_texture = cpu.groups[0].png != NULL;
        for (int d = 0; d < 3; d++) {
            info->first_diffuse[d] = cpu.groups[0].diffuse[d];
            info->first_vertex[d] = s_vbuf[d];
            info->min[d] = cpu.min[d];
            info->max[d] = cpu.max[d];
            info->center[d] = cpu.center[d];
        }
    }
    cgltf_free(data);
    return 1;
}
