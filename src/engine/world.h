#ifndef WORLD_H
#define WORLD_H

#include "assets/ase_loader.h"

struct Entity;
struct World;

typedef struct EntityVTable {
    void (*on_spawn)  (struct Entity *e, struct World *w);
    void (*on_update) (struct Entity *e, struct World *w, float dt);
    void (*on_trigger)(struct Entity *e, struct Entity *by);
    unsigned int flags; /* see ENTITY_FLAG_* */
} EntityVTable;

/* vtable flags */
#define ENTITY_FLAG_PHYSICS   0x01u  /* affected by gravity + integrates velocity */
#define ENTITY_FLAG_SOLID     0x02u  /* participates in AABB collision */
#define ENTITY_FLAG_TRIGGER   0x04u  /* fires on_trigger on player overlap */
#define ENTITY_FLAG_PLAYER    0x08u  /* the controllable entity */

/* Generic .gam property bag. The loader maps well-known properties onto named
   Entity fields; every other authored float/int property is captured here so a
   ported behavior can read its class's parameters (e.g. FanSpeed, SteamPeriod,
   MusicIndex0) by name. This is the porting surface for the Neutron.exe gameplay
   classes — see docs/decomp/<Class>.md for each class's validated property set. */
#define ENTITY_MAX_PROPS 24
typedef struct GamProp {
    char  name[24];
    float f;     /* float value (type-3 props) */
    int   i;     /* int value   (type-6 props) */
} GamProp;

typedef struct Entity {
    char  type[5];               /* FourCC null-terminated */
    char  tag[64];               /* ObjectTag property */
    char  target_level[64];      /* LOAD: LevelName destination (e.g. "level2.gam") */
    char  start_point[32];       /* LOAD: StartPoint name in the destination level */
    char  grn_base[64];          /* BASEAnimation GRN filename, when present */
    char  grn_walk[64];          /* WALKAnimation GRN filename, when present */
    char  grn_talk[64];          /* TALKAnimation GRN filename, when present */
    char  grn_stop[64];          /* STOPAnimation GRN filename, when present */
    char  grn_run[64];           /* RUNAnimation GRN filename, when present */
    char  grn_fly[64];           /* FLYAnimation GRN filename, when present */
    char  grn_anim[4][64];       /* ANIM1Animation..ANIM4Animation filenames */
    char  sprite_database[64];   /* SpriteDatabase property, when present */
    char  sound_database[64];    /* SoundDatabase property, when present */
    char  music_database[64];    /* MusicDatabase property, when present */
    char  omt_database[64];      /* 3OMT: OmtDatabase container filename */
    char  ase_file[64];          /* 3ASE: ASEStop/ASEWalk mesh filename */
    char  png_file[64];          /* 3ASE: PNGFile texture filename */
    char  patrol_point[32];      /* 3MOP/AI: PatrolPoint target marker tag */
    char  next_patrol[32];       /* 3PAT: NextPatrolPoint — next waypoint tag (graph edge) */
    char  activate_target[32];   /* 3BUT/3WAB: ActivateButton -> target ObjectTag */
    float home[3];               /* behavior scratch: captured spawn position */
    float patrol_to[3];          /* behavior scratch: resolved patrol target */
    struct Entity *link_target;  /* behavior scratch: resolved activate target */
    float x, y, z;               /* world position */
    float rx, ry, rz;            /* rotation */
    float vx, vy, vz;            /* velocity (units/sec) */
    float half_extents[3];       /* AABB half-size for collision */
    int   has_initially_visible; /* 1 when InitiallyVisible was authored */
    int   initially_visible;     /* GAM InitiallyVisible value */
    int   sprite_index;          /* SpriteIndex property (sprites.omt chunk id) */
    int   omt_index;             /* 3OMT: OmtIndex shape chunk id; -1 = unset */
    float sprite_size;           /* SpriteSize property (world units), 0 = unset */
    int   effect_type;           /* EffectType property, when present */
    int   points;                /* Points property (pickup score value) */
    int   on_ground;
    int   alive;
    int   visible;               /* runtime draw/update gate; default true */
    unsigned int runtime_flags;   /* mutable copy of EntityVTable.flags */
    float anim_time;              /* shared animated-object clock */
    float move_speed;             /* shared movement-base horizontal speed */
    float move_vert;              /* shared movement-base vertical velocity */
    float move_lean;              /* shared movement-base lean angle */
    int   user_flag;             /* per-type: door open, item collected, ... */
    float user_float;            /* per-type: phase, timer, ... */
    GamProp props[ENTITY_MAX_PROPS]; /* generic .gam property bag (see above) */
    int    nprops;
    const EntityVTable *vt;
    AseModel *model;
    struct Entity *next;
} Entity;

/* Read a captured .gam property by name from the generic bag, with a default
   if the object didn't author it. Defined in assets/gam_loader.c. */
float gam_prop_f(const Entity *e, const char *name, float def);
int   gam_prop_i(const Entity *e, const char *name, int def);

/* Static-geometry placement extracted from an OMT (e.g. level1.omt).
   Each entry refers to a localized ASE on disk plus the original OMT center.
   Native Level 1 draws OMT ASEs at (x, 0, -z) because ase_load maps Max Y to
   GL -Z; legacy validation paths still use the historical +Z placement. Y is
   informational because the exporter bakes height into vertices. */
typedef struct WorldPlacement {
    char  name[64];              /* mesh name (e.g. "labshak") — for debug */
    char  ase_path[160];         /* path to the localized ASE on disk */
    float x, y, z;               /* AABB-center world coords from the OMT */
} WorldPlacement;

typedef struct World {
    Entity *head;
    int     count;
    float   ground_y;            /* ground plane elevation */
    int     safety_floor_enabled;
    float   safety_floor_y;
    float   safety_floor_cx;
    float   safety_floor_cz;
    float   safety_floor_half_x;
    float   safety_floor_half_z;

    WorldPlacement *placements;  /* static-geometry array (NULL when none) */
    int             placement_count;
} World;

void    world_init(World *w);
Entity *world_add(World *w);
Entity *world_find_type(const World *w, const char *type);
void    world_destroy(World *w);

/* Cast a segment from (px,py,pz) toward (qx,qy,qz) through every alive
   ENTITY_FLAG_SOLID AABB except `ignore`. Returns the smallest t in [0,1]
   at which the segment enters a box, or 1.0 if nothing was hit. */
float   world_query_segment(const World *w, const struct Entity *ignore,
                            float px, float py, float pz,
                            float qx, float qy, float qz);

/* Placeholder box VAO (shared across all unloaded entities) */
int          world_box_init(void);
unsigned int world_box_vao(void);
int          world_box_index_count(void);
void         world_box_destroy(void);

#endif
