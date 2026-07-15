#ifndef JN_ASSET_PATHS_H
#define JN_ASSET_PATHS_H

#include <stddef.h>

typedef enum {
    JN_ASSET_BASE = 0,
    JN_ASSET_GAM,
    JN_ASSET_PLACEMENTS,
    JN_ASSET_NATIVE
} JnAssetRoot;

/* Central runtime asset resolution. Environment variables are read once on
   first use and remain stable for the process lifetime. */
const char *asset_root(JnAssetRoot root);
int asset_path_join(char *out, size_t out_size, const char *root,
                    const char *leaf);
int asset_path_resolve(char *out, size_t out_size, const char *path);

#endif
