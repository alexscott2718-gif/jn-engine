#ifndef GAME_FIXTURE_H
#define GAME_FIXTURE_H

#include "../engine/world.h"

int  fixture_level_build(World *world);
int  fixture_level_start_runtime(World *world);
void fixture_level_configure_floor(World *world);
void fixture_level_destroy(void);

#endif
