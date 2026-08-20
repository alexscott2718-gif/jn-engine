#ifndef ENTITY_VISUAL_H
#define ENTITY_VISUAL_H

#include "../engine/world.h"

typedef struct {
    const char *model_path;     /* NULL = no mesh (fall back to placeholder box) */
    const char *texture_path;   /* NULL = use model's bound BMP if any */
    float       scale;          /* uniform mesh scale (1.0 = ASE intrinsic units) */
    int         invisible;      /* 1 = render nothing (markers/triggers) */
    /* Phase 10 Step 2: billboard sprite. When sprite_path != NULL the entity
       renders as a camera-facing alpha-tested quad; model_path is ignored.
       sprite_size is the world-space quad size (width and height); if 0 the
       renderer falls back to 64.0. tint_a==0 means "no tint" (use raw texel). */
    const char *sprite_path;
    float       sprite_size;
    float       tint_r, tint_g, tint_b, tint_a;
    /* Re-center an absolute-positioned mesh onto the entity. The OMT->GLB
       placement meshes under assets/glb/omt/<level>/ bake their absolute world
       Y (and sometimes X/Z) into the vertices — correct for the static
       placement layer, but as an *entity* mesh they would render thousands of
       units from the object. When recenter=1 the draw path offsets the mesh by
       its own bbox center so the mesh sits at the entity's (x,y,z). Leave 0 for
       origin-centered meshes (the assets/glb/ase mirror, top-level glb/omt). */
    int         recenter;
} EntityVisual;

/* Resolve an entity's visual. Returns 1 if a definite override applied
   (visible mesh OR explicit invisible). Returns 0 if no entry matched
   and caller should fall back to placeholder rendering. */
int entity_visual_resolve(const Entity *e, EntityVisual *out);

/* Tag-table check only (tier 1). Lets draw paths that shortcut on
   SpriteIndex (the 3PIC "hidden" ?-placeholder) yield to a curated
   per-tag visual first. out may be NULL when only the test is needed. */
int entity_visual_tag_override(const Entity *e, EntityVisual *out);

/* Game selector for the per-instance sprite tier. The C3DSprite family reads
   SpriteDatabase ("sprites.omt") + SpriteIndex from the .gam, but each game
   ships its *own* sprites.omt — JNBG indices resolve through the generated
   chunk map, JNvsJN through assets/parsed/sprites/jnvsjn/spr_<id>.png.
   main.c calls this once at boot from JN_GAM_ROOT (default root = JNBG). */
void entity_visual_set_jnbg(int is_jnbg);
int  entity_visual_is_jnbg(void);

/* Sandbox/verification mode: reveal normally-hidden rideable objects (the
   C3DRocketShip 3ROC) so they can be located and driven. Default OFF, so the
   audit + matched-camera validators are unaffected. */
void entity_visual_set_sandbox(int on);
int  entity_visual_sandbox_enabled(void);

/* JNBG sprite-database lookups: (.gam SpriteDatabase, SpriteIndex=chunk id)
   -> extracted PNG path, NULL if unknown. sprite_chunk_path is the
   sprites.omt shorthand used by the pickup draw path. */
const char *sprite_db_path(const char *db, int chunk_id);
const char *sprite_chunk_path(int chunk_id);

/* 1 when the entity's authored sprite ref is JNBG sprites.omt chunk 106 —
   the "hidden" canvas: an invisible pickup-trigger stand-in. Draw nothing. */
int sprite_ref_hidden(const Entity *e);
/* Same test without an Entity: is this sprites.omt chunk the "hidden"
   canvas? Used by the HUD, which only has a chunk id. */
int sprite_chunk_is_hidden(int chunk_id);

#endif
