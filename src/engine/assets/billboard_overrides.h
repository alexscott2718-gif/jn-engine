#ifndef BILLBOARD_OVERRIDES_H
#define BILLBOARD_OVERRIDES_H

/* Phase 5 billboard overrides for native Level 1.
 * Maps a mesh name (e.g. "tree01") to a captured 4-vertex quad size + texture.
 * Provenance: assets/native/level1_billboard_overrides.json. Each entry is
 * measured directly from a 4-vertex DRAW_PRIMITIVE record in the captured
 * frame's .omtc; never guessed. See docs/native_vs_capture_8881_plan.md. */

int billboard_overrides_load(const char *tsv_path);

/* If `mesh_name` has a billboard override, fill *size_out + *tex_path_out
 * (pointer into the in-memory table, valid until the next load) and return 1.
 * Returns 0 when the mesh is not in the table. */
int billboard_overrides_lookup(const char *mesh_name,
                               float *size_out,
                               const char **tex_path_out);

int billboard_overrides_count(void);

#endif /* BILLBOARD_OVERRIDES_H */
