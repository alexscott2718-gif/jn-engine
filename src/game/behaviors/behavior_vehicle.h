#ifndef GAME_BEHAVIOR_VEHICLE_H
#define GAME_BEHAVIOR_VEHICLE_H

#include "behaviors.h"

/* Wave N4 — vehicles. The player can board the rocket (vt_rocket); AI vehicles
   (vt_ai_vehicle) drive themselves. While the player is riding, behavior_player
   suppresses on-foot control (the vehicle drives and positions the player). */
int     behavior_vehicle_riding(void);   /* 1 while the player rides a vehicle */
Entity *behavior_vehicle_current(void);  /* the ridden vehicle, or NULL */

/* Force the player onto a vehicle (headless test hook; JN_TEST_RIDE). */
void    behavior_vehicle_force_board(Entity *vehicle);

#endif
