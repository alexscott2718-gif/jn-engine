#include "asset_cache.h"
#include "tex_loader.h"
#include "ase_loader.h"
#include "gltf_loader.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

/* Dispatch by file extension: .glb -> self-contained glTF (textures embedded,
   texture_id populated by the loader); anything else -> legacy ASE text. */
static int load_model_by_ext(AseModel *m, const char *path) {
    size_t n = strlen(path);
    if (n >= 4 && strcasecmp(path + n - 4, ".glb") == 0)
        return gltf_load(m, path);
    return ase_load(m, path);
}

/* Phase 8: level1.omt contributes 194 static placements on top of ~15 entity
   models, so MAX_MODEL must be > ~210. Bump both caches together. */
#define MAX_TEX    256
#define MAX_MODEL  768
#define PATH_LEN   128

typedef struct {
    char         path[PATH_LEN];
    unsigned int id;
    int          gen;
    int          used;
} TexSlot;

typedef struct {
    char      path[PATH_LEN];
    AseModel  model;
    int       loaded;
    int       gen;
    int       used;
} ModelSlot;

static TexSlot   g_tex[MAX_TEX];
static ModelSlot g_model[MAX_MODEL];
static int       g_gen = 0;

void asset_cache_begin_level(void) {
    g_gen++;
}

/* Try assets/png/<stem>.png; return texture id (via tex_cache_get) or 0. */
static unsigned int try_png_stem(const char *stem) {
    if (!stem || !stem[0]) return 0;
    char path[256];
    snprintf(path, sizeof(path), "assets/png/%s.png", stem);
    if (access(path, R_OK) != 0) return 0;
    return tex_cache_get(path);
}

unsigned int tex_cache_resolve_bmp(const char *basename) {
    if (!basename || !basename[0]) return 0;

    /* Pull off the stem (filename minus the final .ext). */
    char stem[128];
    const char *dot = strrchr(basename, '.');
    size_t len = dot ? (size_t)(dot - basename) : strlen(basename);
    if (len == 0 || len >= sizeof(stem)) return 0;
    memcpy(stem, basename, len);
    stem[len] = '\0';

    unsigned int id;

    /* 1. Exact stem. */
    if ((id = try_png_stem(stem))) return id;

    /* 2. Lowercased stem. */
    char stem_lc[128];
    for (size_t i = 0; i <= len; i++) stem_lc[i] = (char)tolower((unsigned char)stem[i]);
    if (strcmp(stem_lc, stem) != 0 && (id = try_png_stem(stem_lc))) return id;

    /* 3. Strip trailing digits ("carl3" -> "carl"). */
    size_t end = len;
    while (end > 0 && isdigit((unsigned char)stem[end-1])) end--;
    if (end > 0 && end < len) {
        char stripped[128];
        memcpy(stripped, stem, end);
        stripped[end] = '\0';
        if ((id = try_png_stem(stripped))) return id;
        for (size_t i = 0; i <= end; i++) stripped[i] = (char)tolower((unsigned char)stripped[i]);
        if ((id = try_png_stem(stripped))) return id;
    }

    return 0;
}

unsigned int tex_cache_get(const char *path) {
    if (!path || !path[0]) return 0;
    for (int i = 0; i < MAX_TEX; i++) {
        if (g_tex[i].used && strncmp(g_tex[i].path, path, PATH_LEN) == 0) {
            g_tex[i].gen = g_gen;
            return g_tex[i].id;
        }
    }
    /* Load and insert. Failures are cached too (id = 0, same pattern as
       model_cache_get): per-frame callers retrying a missing file otherwise
       cost an fopen + a "tex_load: failed" log line every frame. */
    unsigned int id = tex_load(path);
    for (int i = 0; i < MAX_TEX; i++) {
        if (!g_tex[i].used) {
            g_tex[i].used = 1;
            g_tex[i].id   = id;
            g_tex[i].gen  = g_gen;
            snprintf(g_tex[i].path, PATH_LEN, "%s", path);
            return id;
        }
    }
    if (id) fprintf(stderr, "tex_cache: full, leaking %s\n", path);
    return id;
}

AseModel *model_cache_get(const char *path) {
    if (!path || !path[0]) return NULL;
    for (int i = 0; i < MAX_MODEL; i++) {
        if (g_model[i].used && strncmp(g_model[i].path, path, PATH_LEN) == 0) {
            g_model[i].gen = g_gen;
            return g_model[i].loaded ? &g_model[i].model : NULL;
        }
    }
    /* Find free slot, load. */
    for (int i = 0; i < MAX_MODEL; i++) {
        if (!g_model[i].used) {
            g_model[i].used = 1;
            g_model[i].gen  = g_gen;
            snprintf(g_model[i].path, PATH_LEN, "%s", path);
            g_model[i].loaded = load_model_by_ext(&g_model[i].model, path);
            if (!g_model[i].loaded) {
                fprintf(stderr, "model_cache: failed to load %s\n", path);
                return NULL;
            }
            /* Resolve textures for every material. Two sources:
                 - repo-relative paths (Phase 10 OMT exports → assets/parsed/...)
                 - legacy Windows BMP basenames (Jimmy + NPC ASEs) which we
                   remap to assets/png/<stem>.png via tex_cache_resolve_bmp. */
            AseModel *mm = &g_model[i].model;
            for (int k = 0; k < mm->material_count; k++) {
                AseMaterial *am = &mm->materials[k];
                if (am->texture_id) continue;
                if (strncmp(am->tex_file, "assets/", 7) == 0)
                    am->texture_id = tex_cache_get(am->tex_file);
                else if (am->tex_file[0])
                    am->texture_id = tex_cache_resolve_bmp(am->tex_file);
            }
            if (!mm->texture_id && mm->material_count > 0)
                mm->texture_id = mm->materials[0].texture_id;
            if (!mm->texture_id && mm->tex_file[0]) {
                if (strncmp(mm->tex_file, "assets/", 7) == 0)
                    mm->texture_id = tex_cache_get(mm->tex_file);
                else
                    mm->texture_id = tex_cache_resolve_bmp(mm->tex_file);
            }
            return mm;
        }
    }
    fprintf(stderr, "model_cache: full, dropping %s\n", path);
    return NULL;
}

void asset_cache_purge_stale(void) {
    for (int i = 0; i < MAX_TEX; i++) {
        if (g_tex[i].used && g_tex[i].gen != g_gen) {
            tex_free(g_tex[i].id);
            g_tex[i].used = 0;
            g_tex[i].id   = 0;
            g_tex[i].path[0] = '\0';
        }
    }
    for (int i = 0; i < MAX_MODEL; i++) {
        if (g_model[i].used && g_model[i].gen != g_gen) {
            if (g_model[i].loaded) ase_free(&g_model[i].model);
            g_model[i].used   = 0;
            g_model[i].loaded = 0;
            g_model[i].path[0] = '\0';
        }
    }
}

void asset_cache_destroy_all(void) {
    for (int i = 0; i < MAX_TEX; i++) {
        if (g_tex[i].used) {
            tex_free(g_tex[i].id);
            g_tex[i].used = 0;
            g_tex[i].id   = 0;
        }
    }
    for (int i = 0; i < MAX_MODEL; i++) {
        if (g_model[i].used) {
            if (g_model[i].loaded) ase_free(&g_model[i].model);
            g_model[i].used   = 0;
            g_model[i].loaded = 0;
        }
    }
}

int asset_cache_tex_count(void) {
    int n = 0;
    for (int i = 0; i < MAX_TEX; i++) if (g_tex[i].used) n++;
    return n;
}
int asset_cache_model_count(void) {
    int n = 0;
    for (int i = 0; i < MAX_MODEL; i++) if (g_model[i].used) n++;
    return n;
}
