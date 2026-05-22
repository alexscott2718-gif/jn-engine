#ifndef ENTITIES_H
#define ENTITIES_H

#include "../engine/world.h"

const char         *entity_get_type_name (const char *fourcc);
const char         *entity_get_description(const char *fourcc);
const EntityVTable *entity_resolve_vtable(const char *fourcc);

/* Resolve vtables for every entity in the world and call on_spawn. Call once
   after gam_load, before the first update tick. */
void entity_bind_vtables(World *w);

void entity_update(Entity *e, World *w, float dt);

#endif
