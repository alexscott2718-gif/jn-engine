#include "behavior_base.h"
#include <stdlib.h>
#include <string.h>

static int progress_gate_enabled(void) {
    static int cached = -1;
    if (cached < 0) {
        const char *s = getenv("JN_PROGRESS_LEVEL");
        cached = (s && s[0]) ? 1 : 0;
    }
    return cached;
}

static int current_progress_level(void) {
    const char *s = getenv("JN_PROGRESS_LEVEL");
    return (s && s[0]) ? atoi(s) : 0;
}

static int animated_level_gate_allows(const Entity *e) {
    if (!progress_gate_enabled()) return 1;

    int level = current_progress_level();
    int exact = gam_prop_i(e, "ExactLevel", -1);
    int required = gam_prop_i(e, "RequiredLevel", -1);
    int remove = gam_prop_i(e, "RemoveLevel", -1);

    if (exact != -1) return level == exact;
    if (required == -1) return 0;
    return required <= level && (remove == -1 || level < remove);
}

static int animated_initially_visible_allows(const Entity *e) {
    return !e->has_initially_visible || e->initially_visible != 0;
}

static void animated_apply_collision_flag(Entity *e) {
    int has_collision = gam_prop_i(e, "HasCollision", -1);
    if (has_collision == 0)
        e->runtime_flags &= ~ENTITY_FLAG_SOLID;
}

void behavior_animated_spawn_base(Entity *e) {
    if (!e) return;
    e->visible = animated_initially_visible_allows(e) &&
                 animated_level_gate_allows(e);
    animated_apply_collision_flag(e);
    if (!e->visible)
        e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
}

int behavior_animated_update_base(Entity *e, World *w, float dt) {
    (void)w;
    if (!e || !e->alive) return 0;
    if (!animated_level_gate_allows(e) ||
        !animated_initially_visible_allows(e)) {
        e->visible = 0;
        e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
        return 0;
    }

    e->visible = 1;
    animated_apply_collision_flag(e);
    e->anim_time += dt;
    return 1;
}

void behavior_trigger_spawn_base(Entity *e, float hx, float hy, float hz) {
    if (!e) return;
    behavior_animated_spawn_base(e);
    e->half_extents[0] = hx;
    e->half_extents[1] = hy;
    e->half_extents[2] = hz;

    /* CPickupType descendants use the misspelled authored flag. */
    if (gam_prop_i(e, "InitallyActive", -1) == 0) {
        e->visible = 0;
        e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
    }
}
