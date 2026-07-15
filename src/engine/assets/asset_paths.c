#include "asset_paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSET_PATH_MAX 1024

static int initialized;
static char roots[4][ASSET_PATH_MAX];

static const char *nonempty_env(const char *name) {
    const char *value = getenv(name);
    return (value && value[0]) ? value : NULL;
}

int asset_path_join(char *out, size_t out_size, const char *root,
                    const char *leaf) {
    if (!out || out_size == 0 || !root || !root[0]) return 0;
    if (!leaf || !leaf[0]) {
        int written = snprintf(out, out_size, "%s", root);
        return written >= 0 && (size_t)written < out_size;
    }
    size_t root_len = strlen(root);
    int written = snprintf(out, out_size, "%s%s%s", root,
                           root[root_len - 1] == '/' ? "" : "/", leaf);
    return written >= 0 && (size_t)written < out_size;
}

static void initialize_roots(void) {
    if (initialized) return;
    const char *base = nonempty_env("JN_ASSET_ROOT");
    snprintf(roots[JN_ASSET_BASE], ASSET_PATH_MAX, "%s", base ? base : "assets");

    const char *gam = nonempty_env("JN_GAM_ROOT");
    if (gam)
        snprintf(roots[JN_ASSET_GAM], ASSET_PATH_MAX, "%s", gam);
    else
        asset_path_join(roots[JN_ASSET_GAM], ASSET_PATH_MAX,
                        roots[JN_ASSET_BASE], "gam");

    const char *placements = nonempty_env("JN_PLACEMENTS_ROOT");
    if (!placements) placements = nonempty_env("JN_PLB_ROOT");
    if (placements)
        snprintf(roots[JN_ASSET_PLACEMENTS], ASSET_PATH_MAX, "%s", placements);
    else
        asset_path_join(roots[JN_ASSET_PLACEMENTS], ASSET_PATH_MAX,
                        roots[JN_ASSET_BASE], "glb/omt");

    const char *native = nonempty_env("JN_NATIVE_ROOT");
    if (native)
        snprintf(roots[JN_ASSET_NATIVE], ASSET_PATH_MAX, "%s", native);
    else
        asset_path_join(roots[JN_ASSET_NATIVE], ASSET_PATH_MAX,
                        roots[JN_ASSET_BASE], "native");
    initialized = 1;
}

const char *asset_root(JnAssetRoot root) {
    initialize_roots();
    if (root < JN_ASSET_BASE || root > JN_ASSET_NATIVE)
        root = JN_ASSET_BASE;
    return roots[root];
}

int asset_path_resolve(char *out, size_t out_size, const char *path) {
    if (!out || out_size == 0 || !path || !path[0]) return 0;
    const char *relative = NULL;
    if (strncmp(path, "assets/", 7) == 0)
        relative = path + 7;
    else if (strncmp(path, "./assets/", 9) == 0)
        relative = path + 9;
    else if (strcmp(path, "assets") == 0 || strcmp(path, "./assets") == 0)
        relative = "";
    if (relative)
        return asset_path_join(out, out_size, asset_root(JN_ASSET_BASE), relative);
    int written = snprintf(out, out_size, "%s", path);
    return written >= 0 && (size_t)written < out_size;
}
