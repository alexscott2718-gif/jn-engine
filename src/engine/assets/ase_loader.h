#ifndef ASE_LOADER_H
#define ASE_LOADER_H

/* Loads a 3DS Max ASCII Scene Export (.ASE) file into GPU buffers.
   Vertex layout: pos(3) + uv(2) + normal(3) = 8 floats per vertex.
   Coordinate conversion: ASE (X,Y,Z-up) -> OpenGL (X,Z,-Y). */

#define ASE_MAX_MATERIALS 32

typedef struct {
    /* texture filename extracted from *BITMAP (basename or repo-relative) */
    char         tex_file[256];
    /* GL texture ID (0 = none / not yet loaded) */
    unsigned int texture_id;
    /* Per-material slice of the index buffer (in indices, not bytes). */
    int          index_offset;
    int          index_count;
    /* Phase 10 Step 3: diffuse color from *MATERIAL_DIFFUSE (default white).
       Multiplies the texture sample, or is the standalone fragment color when
       texture_id == 0. */
    float        diffuse[3];
} AseMaterial;

typedef struct {
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    int          index_count;        /* total */
    int          vertex_count;       /* interleaved vertices in vbo */
    int          frame_count;        /* 1 for static meshes / glTF */
    float        framespeed;         /* ASE *SCENE_FRAMESPEED, fps */
    float       *frames;             /* optional: frame_count * vertex_count * 8 floats */
    float       *anim_scratch;       /* renderer-owned CPU lerp buffer */
    /* bounding box in OpenGL space */
    float        min[3], max[3], center[3];
    /* Legacy single-bitmap fields — populated from materials[0] for callers
       that still read m->tex_file / m->texture_id directly. */
    char         tex_file[256];
    unsigned int texture_id;
    /* Multi-material draw groups (Phase 10). materials[0..material_count) is
       always populated after a successful load (material_count >= 1). */
    int          material_count;
    AseMaterial  materials[ASE_MAX_MATERIALS];
    /* OMT-sourced meshes (assets/glb/omt/) bake two-sided surfaces as
       explicit reversed-winding twin polys with their own UVs, and the
       original OMediaPipeline back-face culls every poly before submitting
       (om3pf_TwoSided is unused in the corpus). Without culling the later
       twin overdraws the front face under GL_LEQUAL — blanking co-planar
       decals like the SIGN/Constsign text quads (2026-06-11 QA). Set by
       asset_cache for assets/glb/omt/ models; renderer enables
       GL_CULL_FACE while drawing them. */
    int          cull_backfaces;
    /* Source ASE omitted texture coordinates; loader supplied object-space
       fallback UVs so class-attached texture pages do not sample (0,0)
       everywhere. */
    int          generated_uvs;
} AseModel;

/* Load ASE from path. Returns 1 on success, 0 on failure. */
int  ase_load(AseModel *m, const char *path);
void ase_free(AseModel *m);

#endif
