#ifndef GAM_LOADER_H
#define GAM_LOADER_H

#include "../world.h"

/* Parse a .gam level file and populate the World with entities.
   Returns number of entities loaded, or -1 on failure. */
int gam_load(World *w, const char *path);

#endif
