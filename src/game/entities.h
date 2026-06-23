#ifndef ENTITIES_H
#define ENTITIES_H

#include "../engine/world.h"

const char         *entity_get_type_name (const char *fourcc);
const char         *entity_get_description(const char *fourcc);
const EntityVTable *entity_resolve_vtable(const char *fourcc);

/* Resolve vtables for every entity in the world and call on_spawn. Call once
   after gam_load, before the first update tick. */
void entity_bind_vtables(World *w);

/* Spawn one entity at runtime: allocates it, sets the FourCC type + position +
   tag, resolves its vtable, fires on_spawn, and applies default AABB extents —
   the single-entity equivalent of what entity_bind_vtables does for the whole
   world at load. Returns the new entity (or NULL on alloc failure). */
Entity *entity_spawn(World *w, const char *type, const char *tag,
                     float x, float y, float z);

void entity_update(Entity *e, World *w, float dt);

#endif
