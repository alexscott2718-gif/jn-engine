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
extern const EntityVTable vt_steamvent;   /* 3STE — C3DSteamVent */
extern const EntityVTable vt_ferris;      /* 3FER — C3DFerris */
extern const EntityVTable vt_tractor;     /* 3TRC — C3DTractorBeam */
extern const EntityVTable vt_soundfx;     /* 3SOU — C3DSoundEffect */
extern const EntityVTable vt_music;       /* 3MUS — C3DMusicTrigger */
extern const EntityVTable vt_walker;      /* 3CAR — C3DAI patrol walker (Carl) */
extern const EntityVTable vt_patrolpoint; /* 3PAT — C3DPatrolPoint (nav node) */
extern const EntityVTable vt_yokian;      /* 3SOL/3GUA/3SPY — C3DYokian family */
extern const EntityVTable vt_projectile;  /* PROJ — shared projectile */
/* Wave N3 — player combat + pickups family. */
extern const EntityVTable vt_balloon;        /* 3BAL — C3DBalloon (release/pop) */
extern const EntityVTable vt_baseball_pickup; /* 3BPU — C3DBaseballPickup */
extern const EntityVTable vt_bubble_pickup;   /* 3BUP — C3DBubblePickup */
extern const EntityVTable vt_helmet;          /* 3HEL — C3DHelmet */
extern const EntityVTable vt_metal_pickup;    /* 3MEP — C3DMetalPickup */

/* The currently-controlled player; resolved at spawn. NULL until first 3JIM resolved. */
extern Entity *g_player;

#endif
