#ifndef JN_GAME_SPAWN_H
#define JN_GAME_SPAWN_H

#include "../engine/world.h"

/* C3DStartPoint::PlacePlayer (Neutron.exe @ 00442740) counterpart: resolves
   `start_point` (or, when empty, the player's own authored StartPoint
   request) against the placed STRT entities case-insensitively, teleports
   the player to the matched start's transform, and selects its
   MusicDatabase/MusicIndex. Returns the player entity, or NULL if the world
   has no 3JIM. See docs/decomp/C3DStartPoint.md. */
Entity *place_player(World *world, const char *start_point);

#endif /* JN_GAME_SPAWN_H */
