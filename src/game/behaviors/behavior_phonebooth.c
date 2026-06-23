/* behavior_phonebooth.c — C3DPhoneBooth (3PHO).
 *
 * A placeable C3DAnimated world prop (mesh phone.glb, already wired in
 * entity_visual.c). The decompiled leaf owns three things
 * (docs/decomp/C3DPhoneBooth.md):
 *
 *   - InitObject (vfunc_01_007): loads phone.omt and binds its shape — the
 *     booth visual. (Handled by entity_visual.c; nothing to do here.)
 *   - Per-frame (vfunc_01_010): runs the inherited CLocalGameObject update,
 *     then pushes the constant 40.0 through inherited slot 0x110 — a small
 *     contact/interaction radius the booth uses for its Jimmy check.
 *   - Touch (vfunc_01_016): the base CGameObject touch handler, then a guarded
 *     `if (toucher->IsA("C3DJIMMY")) toucher->method_0x1d4()` — i.e. when Jimmy
 *     (and only Jimmy) contacts the booth, it invokes a player-side method.
 *
 * Native model (the confirmed, faithful structure):
 *   - Solid placeable prop, honoring the authored InitiallyVisible / HasCollision
 *     gates (one Level1 booth is IV=0/HasCollision=0 — hidden + intangible).
 *   - Jimmy-only contact detection. Because the booth is SOLID the player can't
 *     enter its AABB to fire an overlap trigger (same constraint the rideable
 *     rocket hit), so contact is detected by proximity in on_update against a
 *     contact radius derived from the booth size + the decomp's 40.0 margin, and
 *     latched so it announces once per approach.
 *
 * Deferred: the player-side effect of the touch (vfunc_01_016's `method_0x1d4`)
 * is genuinely unresolved in the decomp — the spec's own open question flags it,
 * and the booth's tag ("C3DPHONEBOOTH") is not the player's StartPoint marker
 * ("PHONEBOOTH"), so it is not simply a spawn anchor. Rather than invent an
 * effect, this leaf ports the confirmed structure (visual + solidity + the
 * Jimmy-gated contact hook) and leaves the side effect deferred, the same way
 * behavior_escort.c / behavior_friend.c defer their unresolved reward paths.
 *
 * Scratch layout (Entity):
 *   user_flag = contact latch (1 while Jimmy is inside the contact radius)
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>
#include <math.h>
#include <stdio.h>

#define BOOTH_HALF_X   45.0f
#define BOOTH_HALF_Y  130.0f
#define BOOTH_HALF_Z   45.0f
#define BOOTH_TOUCH_MARGIN 40.0f   /* C3DPhoneBooth::vfunc_01_010 -> slot 0x110(40.0) */

static void phonebooth_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);   /* IV + HasCollision gates (may clear SOLID) */
    if (e->half_extents[0] == 0.0f && e->half_extents[1] == 0.0f &&
        e->half_extents[2] == 0.0f) {
        e->half_extents[0] = BOOTH_HALF_X;
        e->half_extents[1] = BOOTH_HALF_Y;
        e->half_extents[2] = BOOTH_HALF_Z;
    }
    e->user_flag = 0;
}

static void phonebooth_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return;

    /* Jimmy-only contact, modelled as proximity in the XZ plane (vfunc_01_016
       gates on IsA("C3DJIMMY")). Latched so the booth reacts once per approach. */
    Entity *p = g_player;
    if (!p || !p->alive) { e->user_flag = 0; return; }

    float rx = e->half_extents[0] + BOOTH_TOUCH_MARGIN;
    float rz = e->half_extents[2] + BOOTH_TOUCH_MARGIN;
    float dx = p->x - e->x;
    float dz = p->z - e->z;
    int inside = (fabsf(dx) <= rx && fabsf(dz) <= rz);

    if (inside && !e->user_flag) {
        e->user_flag = 1;
        /* Confirmed: Jimmy contacted the booth. The player-side effect
           (vfunc_01_016 -> player.method_0x1d4) is unresolved in the decomp and
           is deferred — see the file header. */
        printf("[PHONEBOOTH] '%s' contacted by Jimmy (effect deferred)\n", e->tag);
    } else if (!inside && e->user_flag) {
        e->user_flag = 0;   /* re-arm when Jimmy steps away */
    }
}

/* SOLID: the booth is a physical prop (HasCollision default/1). The animated
   spawn base clears SOLID for the IV=0 / HasCollision=0 instance. */
const EntityVTable vt_phonebooth = {
    .on_spawn   = phonebooth_on_spawn,
    .on_update  = phonebooth_on_update,
    .on_trigger = NULL,
    .flags      = ENTITY_FLAG_SOLID,
};
