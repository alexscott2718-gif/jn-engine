#include "behaviors.h"
#include <stddef.h>

const EntityVTable vt_default = {
    .on_spawn = NULL,
    .on_update = NULL,
    .on_trigger = NULL,
    .flags = 0,
};

/* Static decor — solid for AABB collision but no physics integration. */
const EntityVTable vt_static = {
    .on_spawn = NULL,
    .on_update = NULL,
    .on_trigger = NULL,
    .flags = ENTITY_FLAG_SOLID,
};
