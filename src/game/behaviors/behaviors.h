#ifndef GAME_BEHAVIORS_H
#define GAME_BEHAVIORS_H

#include "../../engine/world.h"
#include "../../engine/renderer.h"

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
extern const EntityVTable vt_yok_turret;  /* 3TUR — C3DYokTurret ranged enemy */
extern const EntityVTable vt_yokian_ship; /* 3YSH — C3DYokianShip patrol actor */
extern const EntityVTable vt_eye;         /* 3EYE — C3DEye C3DAICar patrol actor */
extern const EntityVTable vt_digger;      /* 3DIG — C3DDigger animated prop */
extern const EntityVTable vt_hook;        /* 3HOO — C3DHook AI hook target */
extern const EntityVTable vt_cindy;       /* 3CIN — C3DCindy friend NPC */
extern const EntityVTable vt_friend;      /* 3NIC/3SHE/3ULT/3LIB/3HUG/3BEN/3MOM/3KIT — C3DFriends/C3DAI idle cast */
Entity *behavior_friend_talk_nearest(World *w);          /* T-key: talk nearest friend */
Entity *behavior_friend_talk_tag(World *w, const char *tag); /* JN_TEST_TALK headless hook */
extern const EntityVTable vt_goddard;     /* 3GOD — C3DGoddard runtime companion/controller */
void behavior_goddard_reset(void);
void behavior_player_reset(void);   /* clear g_player on world teardown */
void behavior_vehicle_reset(void);  /* clear the ridden-vehicle cache */
Entity *behavior_goddard_ensure(World *w, const char *level_name);
Entity *behavior_goddard_get(void);
int behavior_goddard_request_mode(Entity *target, int mode);
int behavior_goddard_release_if_target(Entity *target, int mode);
int behavior_goddard_target_is(Entity *target);
int behavior_goddard_mode(void);
int behavior_goddard_apply_ai_state(Entity *g, int ai_state, int ai_speed); /* 3AIT C3DAI-target branch -> Goddard scripted mode */
extern const EntityVTable vt_escort;      /* 3SUM/3PIR — C3DSumo/C3DPirate exit-escort actors */
extern const EntityVTable vt_ai_trigger;  /* 3AIT — C3DAITrigger AI/script mission-wiring volume */
/* Shared C3DTriggerType tag dispatch (defined in behavior_ai_trigger.c, which
   owns the certified dispatch-graph aspect -- see the note there). Both
   C3DAITrigger and C3DPickupItem author ActivateObject / ToggleObject /
   NextTrigger and must dispatch them identically.
     find_by_tag     ObjectTag -> entity, or NULL.
     fire_tag        forward the target's on_trigger (the trigger chain).
     set_state_tag   call the target's state slot with `state` (the original's
                     vtable offset 0x428, fed by the authored Toggle).
   Both dispatchers resolve first and are depth-capped against cycles. */
Entity *behavior_trigger_find_by_tag(World *w, const char *tag);
Entity *behavior_trigger_fire_tag(World *w, const char *tag, Entity *by);
Entity *behavior_trigger_set_state_tag(World *w, const char *tag, int state);
Entity *behavior_ai_trigger_test_fire(World *w);  /* JN_TEST_AITRIG headless hook */
Entity *behavior_ai_trigger_fire_tag(World *w, const char *tag); /* JN_TEST_SCENE headless hook */
extern const EntityVTable vt_tesla;       /* 3TES — C3DTesla electric hazard */
extern const EntityVTable vt_laser_trigger; /* 3LAS — C3DLaserTrigger tripwire */
extern const EntityVTable vt_projectile;  /* PROJ — shared projectile */
/* Wave N3 — player combat + pickups family. */
extern const EntityVTable vt_balloon;        /* 3BAL — C3DBalloon (release/pop) */
extern const EntityVTable vt_baseball_pickup; /* 3BPU — C3DBaseballPickup */
extern const EntityVTable vt_bubble_pickup;   /* 3BUP — C3DBubblePickup */
extern const EntityVTable vt_helmet;          /* 3HEL — C3DHelmet */
extern const EntityVTable vt_metal_pickup;    /* 3MEP — C3DMetalPickup */
/* Wave N4 — vehicles. */
extern const EntityVTable vt_rocket;          /* 3ROC — C3DRocketShip (player-rideable) */
extern const EntityVTable vt_scooter;         /* 3JEE — C3DJeep, the scooter gadget */
extern const EntityVTable vt_shrinkray;       /* 3SHR — C3DShrinkRay, the fired ray */
extern const EntityVTable vt_bubble;          /* 3BUB — C3DBubble */
extern const EntityVTable vt_grapple_rope;    /* 3GRA — C3DGraplingHook rope */
extern const EntityVTable vt_rocket_fuel;     /* 3FUE — C3DRocketFuel (plutonium rods) */
extern const EntityVTable vt_moving_target;   /* 3TAR — C3DMovingTarget (shooting range) */
/* The baseball's hit. Returns 1 when the entity consumed it. */
int  behavior_moving_target_take_hit(Entity *e, World *w);
void behavior_moving_target_reset(void);

/* The eight gadgets (docs/picture_flag_wiring_plan.md section 18). */
enum {
    GADGET_NONE = 0, GADGET_JETPACK, GADGET_SHRINKRAY, GADGET_BUBBLE,
    GADGET_GRAPPLER, GADGET_GODDARD, GADGET_ROCKET, GADGET_SCOOTER,
    GADGET_INVISIBILITY
};
int  behavior_gadget_activate(const char *tag, World *w);
void behavior_gadgets_update(World *w, float dt);
int  behavior_gadget_fire(World *w);   /* the trigger, for input and tests */
void behavior_gadgets_reset(void);
int  behavior_gadget_active(void);
int  behavior_gadget_invisible(void);
const char *behavior_gadget_name(int gadget);
/* Bubble draw size: width holds, height carries the sin(t*10)*30 wave, both
   scaled by the grow/fade transition. */
void behavior_bubble_size(const Entity *e, float *w, float *h, float *alpha);
/* The scooter is never placed in level data: JimmySetupOrReset code-spawns it
   hidden at Jimmy 0x970 and reveals it on demand. ensure() is that spawn. */
Entity *behavior_scooter_ensure(World *w);
Entity *behavior_scooter_get(void);
int     behavior_scooter_riding(void);
int     behavior_scooter_activate(void);   /* action-menu selection; toggles */
void    behavior_scooter_reset(void);
extern const EntityVTable vt_ai_vehicle;      /* 3SUV/3SBU/3SAI — self-driving C3DAI vehicles */
/* Wave N5 — scripted cutscene cameras. */
extern const EntityVTable vt_cutscene_camera; /* 3CAM — C3DCutSceneCamera (shot) */
extern const EntityVTable vt_multi_cutscene;  /* 3MCA — C3DMultiCutSceneCamera (sequencer) */
/* Actor / gameplay focus rows (post-N5). */
extern const EntityVTable vt_phonebooth;      /* 3PHO — C3DPhoneBooth (solid contact prop) */
extern const EntityVTable vt_rocket_ai;       /* 3RCK — C3DRocket (placed C3DAI patrol rocket) */
extern const EntityVTable vt_humphrey;        /* 3HUM — C3DHumphrey (hidden clone-controller) */
extern const EntityVTable vt_neutron;         /* 3NEU — C3DNeutron animated pickup sprite */
extern const EntityVTable vt_red_neutron;     /* 3RED — C3DRedNeutron trigger pickup sprite */
extern const EntityVTable vt_arrow;           /* 3ARR — C3DArrow gated nav sprite */
extern const EntityVTable vt_lightobj;        /* 3LIO — C3DLightObj invisible light-data row */
extern const EntityVTable vt_omtobj;          /* 3OMT — C3DOmtObj authored OMT-shape prop */
extern const EntityVTable vt_cone;            /* 3CON — C3DCone sprite decor */
extern const EntityVTable vt_trophy;          /* 3TRO — C3DVRTrophy win-condition pickup */
extern const EntityVTable vt_leaves;          /* 3LEA — C3DLeaves sprite decor */
extern const EntityVTable vt_shadow;          /* 3TAR — C3DShadow sprite decor */
extern const EntityVTable vt_ai_omtobj;       /* 3AIO — C3DAIOmtObj OMT-shape prop */
/* Base/effect tail pass 4. */
extern const EntityVTable vt_light;           /* 3LIG — C3DLight invisible scene light-data row */
extern const EntityVTable vt_creature;        /* 3FIS/3GIR/3DIN/3CML/3SPW — C3DAI idle set-dressing creatures (shrink-targets; active shrink mechanic deferred) */
extern const EntityVTable vt_stalactite;      /* 3STA — C3DStalagtite static terrain prop */
/* Base/resolver long tail. */
extern const EntityVTable vt_animsprite;      /* 3ANI — C3DAnimatedSprite sprites.omt frame animator */
extern const EntityVTable vt_swingdoor;       /* 3SWN — C3DSwingDoor timed yaw-swing door */
extern const EntityVTable vt_prop;            /* 3FLA/3HYD/3SCR/3TEL/3SPH/3CUB/3TOL/3OCT/3MER/3TRA/3SM1/3FUE/3TRI — gated static prop/effect leaves (owned gameplay deferred) */
extern const EntityVTable vt_resolver_inert;  /* 3ROK/3SPR/3DAI — native inert rows whose unresolved visual/positioning stays hidden */
/* SCENE-sequencer consumers (freed by the task-state story progression). */
extern const EntityVTable vt_fowl;            /* 3FOW — C3DFowl per-level SCENE-window creature */
extern const EntityVTable vt_cargo;           /* 3YCA — C3DYokCargo LEV5 SCENE-gated cargo prop */

/* Cutscene runtime (behavior_cutscene.c). 3CAM shots register on spawn; the
   sequence plays through renderer_set_camera_override when requested. Off by
   default so the audit / matched-camera validators are unaffected. */
void cutscene_reset(void);                    /* clear registered shots (per level) */
void cutscene_request_intro(void);            /* begin playing the level's shots */
int  cutscene_request_index(int index);       /* play a registered 3MCA sequence */
int  cutscene_request_shot_index(int index);  /* play one registered standalone 3CAM */
int  cutscene_sequence_count(void);
int  cutscene_active(void);
void cutscene_stop(void);
void cutscene_update(Camera *cam, World *w, float dt);
int  cutscene_player_control_locked(void);   /* active shot suppresses player input */
int  cutscene_player_anim_override(void);    /* PlayerAnim id, or -1 when none */

/* The currently-controlled player; resolved at spawn. NULL until first 3JIM resolved. */
extern Entity *g_player;

#endif
