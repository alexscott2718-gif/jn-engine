/* behavior_animsprite.c — C3DAnimatedSprite (3ANI).
 *
 * A sprites.omt canvas-frame animator: CPickupType -> C3DTriggerType ->
 * C3DSprite. The class cycles the authored Sprite1..Sprite9 frame list at FPS
 * while Activated, swapping the billboard canvas each step and honoring the
 * Loop end behavior; when inactive it shows Sprite1 (or stays hidden) per the
 * misspelled InitallyVisible flag. docs/decomp/C3DAnimatedSprite.md.
 *
 * Faithful scope: the original's SetAnimatedSpriteState(0/1/-1) is invoked by
 * the scripted trigger chain (ToggleObject/Toggle) and the pickup-state system,
 * neither of which is ported yet (same posture as 3NEU's NextTrigger). So the
 * runtime animation/visibility state is seeded from the authored Activated /
 * InitallyVisible and toggle activation is deferred. Rows authoring Activated=1
 * animate on load (the Level3C 'bottles' break sequence; the level1b 171/182
 * blink); Activated=0 rows show Sprite1 statically until a trigger fires (the
 * level1b 'apple'). The lone Level1 row authors an all-(-1) frame list
 * (frame_count 0, SpriteIndex -1) and draws nothing — faithful to
 * LoadAnimatedSpriteCanvas, which seeds SpriteIndex from Sprite1.
 *
 * Scratch layout: user_flag=current_frame, user_float=frame_elapsed,
 * home[0]=frame_count, home[1]=animation_running.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ANIMSPRITE_DEFAULT_FPS  8.0f
#define ANIMSPRITE_DEFAULT_SIZE 75.0f

/* Sprite1..Sprite9 (1-based in the .gam); returns -1 for the end sentinel. */
static int anim_frame_at(const Entity *e, int slot) {
    char name[12];
    snprintf(name, sizeof(name), "Sprite%d", slot + 1);
    return gam_prop_i(e, name, -1);
}

/* frame_count = index of the first Sprite==-1, else the full 9. */
static int anim_frame_count(const Entity *e) {
    for (int i = 0; i < 9; i++)
        if (anim_frame_at(e, i) < 0) return i;
    return 9;
}

static float anim_fps(const Entity *e) {
    float fps = gam_prop_f(e, "FPS", ANIMSPRITE_DEFAULT_FPS);
    return fps > 0.0f ? fps : ANIMSPRITE_DEFAULT_FPS;
}

/* InitallyVisible (misspelled in the source + .gam): only ==0 hides. */
static int anim_initially_visible(const Entity *e) {
    return gam_prop_i(e, "InitallyVisible", 1) != 0;
}

static void animsprite_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);

    if (!e->sprite_database[0])
        snprintf(e->sprite_database, sizeof(e->sprite_database), "%s", "sprites.omt");
    if (e->sprite_size <= 1.0f)
        e->sprite_size = ANIMSPRITE_DEFAULT_SIZE;

    int frame_count = anim_frame_count(e);
    int first = anim_frame_at(e, 0);            /* Sprite1 */
    int activated = gam_prop_i(e, "Activated", 0);

    e->home[0] = (float)frame_count;
    e->home[1] = (activated && frame_count > 0) ? 1.0f : 0.0f;
    e->user_flag = 0;                            /* current_frame */
    e->user_float = 0.0f;                        /* frame_elapsed */

    /* LoadAnimatedSpriteCanvas: SpriteIndex = Sprite1 (when authored). A
       degenerate all-(-1) list leaves the authored SpriteIndex (-1 -> hidden). */
    if (first >= 0) e->sprite_index = first;

    /* Active rows keep the level-gate visibility (visible by default);
       inactive rows additionally honor InitallyVisible. */
    if (!activated)
        e->visible = e->visible && anim_initially_visible(e);

    e->runtime_flags = 0;                        /* non-solid, non-trigger sprite */
}

static void animsprite_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return;
    e->runtime_flags = 0;

    int activated = gam_prop_i(e, "Activated", 0);
    if (!activated) {
        /* Static: show Sprite1 only when initially visible; never advance. */
        e->visible = anim_initially_visible(e);
        return;
    }

    e->visible = 1;
    if (e->home[1] == 0.0f) return;              /* finished a Loop==0 pass */

    int frame_count = (int)e->home[0];
    if (frame_count <= 0) return;                /* degenerate (Level1) */

    e->user_float += dt;
    float step = 1.0f / anim_fps(e);
    if (e->user_float < step) return;
    e->user_float = 0.0f;

    int cur = e->user_flag;
    int frame = anim_frame_at(e, cur);
    if (frame >= 0) e->sprite_index = frame;
    cur++;
    if (cur >= frame_count) {
        cur = 0;
        int loop = gam_prop_i(e, "Loop", 1);
        if (loop == 0) e->home[1] = 0.0f;        /* stop, hold the last frame */
        /* loop==2 re-asserts visibility (already visible); loop==1 wraps. */
    }
    e->user_flag = cur;
}

const EntityVTable vt_animsprite = {
    .on_spawn  = animsprite_spawn,
    .on_update = animsprite_update,
    .flags     = 0,
};
