#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include "ase_loader.h"

/* Loads a glTF 2.0 binary (.glb) produced by omt_asset_toolkit's gltf_export
   into the same AseModel contract used by ase_loader (interleaved
   pos(3)+uv(2)+normal(3), indexed, multi-material draw groups). The .glb is
   self-contained: canvas textures are embedded and decoded/uploaded here, so
   AseModel.materials[k].texture_id is populated directly (unlike the ASE path,
   where the asset cache resolves textures by filename).

   Conventions are pre-baked by the exporter (see docs/gltf_export_plan.md):
   positions/normals are already in engine GL space, UVs are raw with REPEAT
   wrap and no V-flip. The loader therefore applies no coordinate or UV
   transforms — it decodes embedded PNGs without vertical flip (glTF UV origin
   is top-left), matching the raw UVs. */

/* Load .glb from path. Returns 1 on success, 0 on failure. */
int gltf_load(AseModel *m, const char *path);

/* No-GL inspection of a .glb: fills the counts/first-vertex/first-material
   fields so the parse path can be unit-tested without an OpenGL context.
   Any out-pointer may be NULL. Returns 1 on success, 0 on failure. */
typedef struct {
    int   vertex_count;            /* total de-indexed vertices */
    int   material_count;          /* = primitive count */
    int   first_index_count;       /* indices in material 0 */
    int   first_has_texture;       /* 1 if material 0 has a baseColorTexture */
    float first_diffuse[3];        /* material 0 baseColorFactor rgb */
    float first_vertex[3];         /* position of the first emitted vertex */
    float min[3], max[3], center[3];
} GltfInfo;

int gltf_inspect(const char *path, GltfInfo *info);

#endif
