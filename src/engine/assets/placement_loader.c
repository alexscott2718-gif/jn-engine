#include "placement_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int placements_load(World *w, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "placements_load: cannot open %s\n", path); return -1; }

    free(w->placements);
    w->placements = NULL;
    w->placement_count = 0;

    int cap = 0, n = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') continue;

        /* Format: NAME\tASE_PATH\tCX\tCY\tCZ */
        char *p = line;
        char *name = p;
        char *t1 = strchr(p, '\t'); if (!t1) continue;
        *t1 = '\0';
        char *ase = t1 + 1;
        char *t2 = strchr(ase, '\t'); if (!t2) continue;
        *t2 = '\0';
        float cx = 0, cy = 0, cz = 0;
        if (sscanf(t2 + 1, "%f\t%f\t%f", &cx, &cy, &cz) != 3) continue;

        if (n >= cap) {
            int new_cap = cap ? cap * 2 : 64;
            WorldPlacement *grown = realloc(w->placements,
                                             (size_t)new_cap * sizeof(WorldPlacement));
            if (!grown) { fclose(f); return n; }
            w->placements = grown;
            cap = new_cap;
        }
        WorldPlacement *pl = &w->placements[n++];
        snprintf(pl->name,     sizeof(pl->name),     "%s", name);
        snprintf(pl->ase_path, sizeof(pl->ase_path), "%s", ase);
        pl->x = cx; pl->y = cy; pl->z = cz;
    }
    fclose(f);
    w->placement_count = n;
    printf("placements_load: %s  %d placements\n", path, n);
    return n;
}
