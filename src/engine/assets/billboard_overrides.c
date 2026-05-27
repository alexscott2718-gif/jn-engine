#include "billboard_overrides.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BB_OV_MAX 32
typedef struct {
    char  mesh[64];
    float size;
    char  texture[256];
} BbEntry;

static BbEntry g_bb[BB_OV_MAX];
static int     g_bb_n = 0;

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t'
                     || e[-1] == '\r' || e[-1] == '\n')) {
        e--;
    }
    *e = '\0';
    return s;
}

int billboard_overrides_load(const char *tsv_path) {
    g_bb_n = 0;
    if (!tsv_path || !tsv_path[0]) return 0;
    FILE *f = fopen(tsv_path, "r");
    if (!f) return 0;
    char line[512];
    int loaded = 0;
    while (fgets(line, sizeof(line), f)) {
        char *p = trim(line);
        if (!*p || *p == '#') continue;
        char *t1 = strchr(p, '\t');
        if (!t1) continue;
        *t1 = '\0';
        char *t2 = strchr(t1 + 1, '\t');
        if (!t2) continue;
        *t2 = '\0';
        const char *mesh    = trim(p);
        const char *size_s  = trim(t1 + 1);
        const char *tex     = trim(t2 + 1);
        if (!*mesh || !*tex) continue;
        float sz = (float)atof(size_s);
        if (sz <= 0.0f) continue;
        if (g_bb_n >= BB_OV_MAX) {
            fprintf(stderr, "[billboard_overrides] table full at %d, "
                            "dropping later entries (raise BB_OV_MAX)\n",
                    BB_OV_MAX);
            break;
        }
        BbEntry *e = &g_bb[g_bb_n++];
        snprintf(e->mesh, sizeof(e->mesh), "%s", mesh);
        e->size = sz;
        snprintf(e->texture, sizeof(e->texture), "%s", tex);
        loaded++;
    }
    fclose(f);
    if (loaded) {
        fprintf(stderr, "[billboard_overrides] loaded %d entries from %s\n",
                loaded, tsv_path);
    }
    return loaded > 0;
}

int billboard_overrides_lookup(const char *mesh_name,
                               float *size_out,
                               const char **tex_path_out) {
    if (!mesh_name || !*mesh_name || g_bb_n == 0) return 0;
    for (int i = 0; i < g_bb_n; i++) {
        if (strcmp(g_bb[i].mesh, mesh_name) != 0) continue;
        if (size_out)     *size_out     = g_bb[i].size;
        if (tex_path_out) *tex_path_out = g_bb[i].texture;
        return 1;
    }
    return 0;
}

int billboard_overrides_count(void) { return g_bb_n; }
