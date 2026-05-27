#include "texture_overrides.h"
#include "ase_loader.h"
#include "asset_cache.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Tiny TSV-backed table. Phase 2 only needs a handful of entries (GROUND +
 * a few water meshes at most), so a flat array is the right tool. If this
 * grows past ~64 entries, swap to a hash. */
#define TX_OV_MAX 64
typedef struct {
    char mesh[64];
    int  slot;
    char texture[256];
} OvEntry;

static OvEntry g_ov[TX_OV_MAX];
static int     g_ov_n = 0;

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

int texture_overrides_load(const char *tsv_path) {
    g_ov_n = 0;
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
        const char *mesh = trim(p);
        const char *slot_s = trim(t1 + 1);
        const char *tex = trim(t2 + 1);
        if (!*mesh || !*tex) continue;
        if (g_ov_n >= TX_OV_MAX) {
            fprintf(stderr, "[texture_overrides] table full at %d, "
                            "dropping later entries (raise TX_OV_MAX)\n",
                    TX_OV_MAX);
            break;
        }
        OvEntry *e = &g_ov[g_ov_n++];
        snprintf(e->mesh, sizeof(e->mesh), "%s", mesh);
        e->slot = atoi(slot_s);
        snprintf(e->texture, sizeof(e->texture), "%s", tex);
        loaded++;
    }
    fclose(f);
    if (loaded) {
        fprintf(stderr, "[texture_overrides] loaded %d entries from %s\n",
                loaded, tsv_path);
    }
    return loaded > 0;
}

void texture_overrides_apply(AseModel *m, const char *mesh_name) {
    if (!m || !mesh_name || !*mesh_name || g_ov_n == 0) return;
    /* Each override is explicitly authored against measured capture
       evidence -- always replace, even if the OMT canvas chain resolved
       something. Phase 3 (mesh-by-mesh recovery) specifically swaps
       OMT-resolved bitmaps for the texture the capture binds. */
    for (int i = 0; i < g_ov_n; i++) {
        if (strcmp(g_ov[i].mesh, mesh_name) != 0) continue;
        int slot = g_ov[i].slot;
        if (slot < 0 || slot >= m->material_count) continue;
        AseMaterial *am = &m->materials[slot];
        unsigned int tid = tex_cache_get(g_ov[i].texture);
        if (!tid) continue;
        am->texture_id = tid;
        snprintf(am->tex_file, sizeof(am->tex_file), "%s", g_ov[i].texture);
        if (!m->texture_id) m->texture_id = tid;
    }
}

int texture_overrides_count(void) { return g_ov_n; }
