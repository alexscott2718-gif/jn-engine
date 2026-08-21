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

/* The `StartTrigger` of the start point the last place_player() call used, or
   "" when it had none (or none matched). C3DStartPoint registers the property
   and the class doc calls it "Trigger fired once on spawn here", but the
   recovered PlacePlayer body does not show the firing, so *what* fires it is
   the caller's business and the mechanism is INFERRED -- see the fire site in
   main.c. Cleared on every placement, so nothing stays armed across a level. */
const char *spawn_start_trigger(void);

#endif /* JN_GAME_SPAWN_H */
