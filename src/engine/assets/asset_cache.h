#ifndef ASSET_CACHE_H
#define ASSET_CACHE_H

#include "ase_loader.h"

/* Path-keyed cache for textures + ASE models. Lifetime is generation-based:
   bump the generation at the start of a level; entries not touched during
   that level become stale and can be purged. Mirrors the per-type purge
   shape of the original engine's OMediaDataBase, sans LRU/factory/async. */

unsigned int tex_cache_get(const char *path);     /* 0 on failure */
AseModel    *model_cache_get(const char *path);   /* NULL on failure */

/* Resolve a Windows-style BMP basename (e.g. "carl3.bmp") to a PNG under
   assets/png/. Tries the exact stem first, then lowercased stem, then the
   stem with trailing digits stripped (so "carl3.bmp" finds "carl.png").
   Returns the loaded texture id, or 0 if nothing maps. Used to make the
   legacy Jimmy ASEs render with their authored textures. */
unsigned int tex_cache_resolve_bmp(const char *bmp_basename);

void asset_cache_begin_level(void);   /* bump generation */
void asset_cache_purge_stale(void);   /* free entries not touched this gen */
void asset_cache_destroy_all(void);   /* shutdown */

/* Diagnostics */
int  asset_cache_tex_count(void);
int  asset_cache_model_count(void);

#endif
