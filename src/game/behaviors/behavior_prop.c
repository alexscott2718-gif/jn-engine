/* behavior_prop.c — shared "gated static prop" vtable (vt_prop).
 *
 * The base/resolver + effect long tail collapses a spread of placeable FourCCs
 * into one faithful minimal shape: a placed, mostly-static prop or effect that
 * runs the inherited C3DObject/C3DAnimated/C3DSprite visibility + level/progress
 * gate, takes its collision from authored data, and leaves the visual to
 * entity_visual.c. Each class below owns *some* gameplay method, but in every
 * case that method depends on a subsystem this native port has not landed, so it
 * is faithfully DEFERRED (decomp bodies read first; see docs/decomp/<Class>.md):
 *
 *   3FLA  C3DFlag       — world prop; owns only a dtor. Static flag mesh (flag.ASE).
 *   3HYD  C3DHydrant    — world prop; InitObject binds waterhydrant.ase + a 4-frame
 *                         water animation. Static hydrant mesh (frame loop deferred).
 *   3SCR  C3DLabScreen  — controller prop; InitObject binds screen.ase + 5 screen
 *                         frames. Static display mesh (frame cycling deferred).
 *   3TEL  C3DTeleportFX — effect; owns only a dtor. Static teleport-ray mesh.
 *   3SPH  C3DSphere     — world prop; builds a procedural OMedia3DShape sphere
 *                         (Sphere01.ASE proxy). HasCollision=0 in-corpus → non-solid.
 *   3CUB  C3DCube       — world prop; builds a procedural OMedia3DShape cube. We do
 *                         not render procedural primitives, so it draws nothing
 *                         (resolver gap, not a behavior gap); HasCollision=1 in-corpus,
 *                         so it stays a solid invisible block, faithful to its data.
 *   3TOL  C3DToolChest  — world prop; InitObject binds open/notools/closed anims,
 *                         default CLOSED. Static closed-chest mesh.
 *   3OCT  C3DOctapuke   — set-dressing hazard; the Jimmy-touch handler teleports the
 *                         player gated on pickup counter 0x1b — deferred (no counter
 *                         system / unresolved player slot). Static octo mesh.
 *   3MER  C3DMerryGo    — mechanism; the Jimmy-touch handler attaches the player to
 *                         the ride — deferred (unresolved player attach slots). Static
 *                         RideSpin mesh (no owned per-frame spin method).
 *   3TRA  C3DTransRepl  — set-dressing; registers per-instance ASEFile/PNGFile
 *                         (lavalevel5a.ase). The per-instance mesh path is only wired
 *                         for 3SWN doors today, so the visual is deferred (resolver
 *                         gap); the vtable lands the gate.
 *   3SM1  C3DSmoke      — effect sprite; owns only a dtor. Static smoke billboard.
 *   3TRI  C3DTrigger    — base trigger leaf; its activate-object cascade / NextTrigger
 *                         dispatch is the project-wide deferred scripted-trigger system.
 *                         The single in-corpus row is fully "none" (inert). Marker sprite.
 *
 * Solidity is taken strictly from authored HasCollision (1 → solid, 0 → non-solid),
 * defaulting non-solid when unset so the port never invents an obstacle. No invented
 * per-frame side effects: this is the faithful minimal port of a placed prop whose
 * concrete gameplay needs an unported subsystem.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>

static void prop_apply_solidity(Entity *e) {
    int hc = gam_prop_i(e, "HasCollision", -1);
    if (hc == 1)      e->runtime_flags |= ENTITY_FLAG_SOLID;
    else if (hc == 0) e->runtime_flags &= ~ENTITY_FLAG_SOLID;
    /* unset (-1): leave the vtable default (non-solid). */
}

static void prop_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    if (e->visible) prop_apply_solidity(e);
}

static void prop_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return;
    prop_apply_solidity(e);
}

const EntityVTable vt_prop = {
    .on_spawn   = prop_on_spawn,
    .on_update  = prop_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
