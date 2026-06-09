#ifndef GAME_BEHAVIORS_H
#define GAME_BEHAVIORS_H

#include "../../engine/world.h"

extern const EntityVTable vt_default;
extern const EntityVTable vt_player;
extern const EntityVTable vt_static;   /* trees, rocks, start marker — solid props */
extern const EntityVTable vt_load;
extern const EntityVTable vt_trig;
extern const EntityVTable vt_door;
extern const EntityVTable vt_plat;
extern const EntityVTable vt_item;
extern const EntityVTable vt_checkpoint;  /* 3CHK */
extern const EntityVTable vt_movplat;     /* 3MOP */
extern const EntityVTable vt_button;      /* 3BUT / 3WAB */
extern const EntityVTable vt_leveldoor;   /* 3DOR / 3DUD / 3SCD */
extern const EntityVTable vt_fan;         /* 3FAN — C3DFan */
extern const EntityVTable vt_switch;      /* 3SWI — C3DSwitch */
extern const EntityVTable vt_geyser;      /* 3GEY — C3DGeyser */
extern const EntityVTable vt_pendulum;    /* 3PEN — C3DPendulum */

/* The currently-controlled player; resolved at spawn. NULL until first 3JIM resolved. */
extern Entity *g_player;

#endif
