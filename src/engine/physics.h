#ifndef PHYSICS_H
#define PHYSICS_H

#include "world.h"

/* Tunables (Level1 units — JIM spawns near y=31, world extents ~10000). */
#define PHYSICS_GRAVITY      1600.0f   /* units / sec^2, downward */
#define PHYSICS_MAX_FALL     2400.0f   /* terminal velocity */

/* Canonical entity AABB overlap test used by trigger/pickup-style behaviors. */
int physics_aabb_overlap(const Entity *a, const Entity *b);

/* Integrate one fixed step:
   - apply gravity + integrate velocity for entities flagged ENTITY_FLAG_PHYSICS
   - resolve AABB collisions against entities flagged ENTITY_FLAG_SOLID
   - fire on_trigger for entities flagged ENTITY_FLAG_TRIGGER that the player overlaps
*/
void physics_step(World *w, float dt);

#endif
