#ifndef GAME_BEHAVIOR_TESLA_H
#define GAME_BEHAVIOR_TESLA_H

#include "behaviors.h"

/* C3DTesla activation receiver convention: 0=off, 1=on, other=toggle. */
void behavior_tesla_set_active(Entity *e, int mode);

#endif
