#ifndef TEXTURE_OVERRIDES_H
#define TEXTURE_OVERRIDES_H

/* Phase 2 ground/water texture overrides for native Level 1.
 * See docs/native_vs_capture_8881_plan.md and the schema documented in
 * assets/native/level1_texture_overrides.json. Each override is a measured
 * mesh+slot -> texture mapping derived from the captured D3D7 stream;
 * never from heuristic guesses. */

#include "ase_loader.h"  /* AseModel typedef */

/* Load the overrides table from the given TSV path. Idempotent; subsequent
 * calls replace the prior table. Returns 1 on success, 0 if the file is
 * missing or empty (which is fine — overrides are optional). */
int texture_overrides_load(const char *tsv_path);

/* If `mesh_name` has any registered overrides, look them up in the in-memory
 * table and patch matching slot indices of `m`. tex_cache_get is used to
 * acquire / cache the override texture id. No-op when the table is empty or
 * the mesh name is not present. */
void texture_overrides_apply(AseModel *m, const char *mesh_name);

/* For audit/logging: total override entries currently loaded. */
int texture_overrides_count(void);

#endif /* TEXTURE_OVERRIDES_H */
