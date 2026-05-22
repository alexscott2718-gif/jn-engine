#ifndef PLACEMENT_LOADER_H
#define PLACEMENT_LOADER_H

#include "../world.h"

/* Parse a *_placements.txt sidecar (produced by tools/omt_mesh_export.py)
   and populate world->placements / placement_count. Returns the number of
   records loaded, or -1 on file-open error. Any pre-existing placement
   array on the World is freed first. */
int placements_load(World *w, const char *path);

#endif
