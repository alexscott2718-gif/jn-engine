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
} EntityVisual;

/* Resolve an entity's visual. Returns 1 if a definite override applied
   (visible mesh OR explicit invisible). Returns 0 if no entry matched
   and caller should fall back to placeholder rendering. */
int entity_visual_resolve(const Entity *e, EntityVisual *out);

#endif
