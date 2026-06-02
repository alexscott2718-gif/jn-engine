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

typedef struct Entity {
    char  type[5];               /* FourCC null-terminated */
    char  tag[64];               /* ObjectTag property */
    char  target_level[64];      /* LOAD: LevelName destination (e.g. "level2.gam") */
    char  start_point[32];       /* LOAD: StartPoint name in the destination level */
    float x, y, z;               /* world position */
    float rx, ry, rz;            /* rotation */
    float vx, vy, vz;            /* velocity (units/sec) */
    float half_extents[3];       /* AABB half-size for collision */
    int   on_ground;
    int   alive;
    int   user_flag;             /* per-type: door open, item collected, ... */
    float user_float;            /* per-type: phase, timer, ... */
    const EntityVTable *vt;
    AseModel *model;
    struct Entity *next;
} Entity;

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
