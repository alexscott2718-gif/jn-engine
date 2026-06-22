#include "entities.h"
#include "behaviors/behaviors.h"
#include <string.h>

typedef struct {
    const char *type;
    const char *description;
    const EntityVTable *vt;
} EntityTypeInfo;

/* NOTE: vtables are external (defined per behavior_*.c). */
static const EntityTypeInfo entity_types[] = {
    { "3JIM", "Player start position", &vt_player  },
    { "3ROC", "Rocket",                &vt_static  },
    { "3TRE", "Tree",                  &vt_static  },
    { "LOAD", "Level loader trigger",  &vt_load    },
    { "STRT", "Start marker",          &vt_default },
    { "PLAT", "Platform",              &vt_plat    },
    { "DOOR", "Door",                  &vt_door    },
    { "TRIG", "Trigger volume",        &vt_trig    },
    { "CRAT", "Crate",                 &vt_static  },
    { "ITEM", "Collectible item",      &vt_item    },
    { "3GEM", "Gem pickup",            &vt_item    },
    { "3PIC", "Pickup item",           &vt_item    },
    { "3CHK", "Checkpoint",            &vt_checkpoint },
    { "3MOP", "Moving platform",       &vt_movplat },
    { "3BUT", "Button",                &vt_button  },
    { "3WAB", "Water button",          &vt_button  },
    /* Doors openable by buttons (and by direct touch); non-solid so they never
       trap the player at a doorway. */
    { "3DOR", "Door",                  &vt_leveldoor },
    { "3DUD", "Door up/down",          &vt_leveldoor },
    { "3SCD", "School door",           &vt_leveldoor },
    { "3FAN", "Fan",                   &vt_fan       },
    { "3SWI", "Switch",                &vt_switch    },
    { "3GEY", "Geyser",                &vt_geyser    },
    { "3PEN", "Pendulum",              &vt_pendulum  },
    { "3GAT", "Gate",                  &vt_leveldoor },  /* C3DGate1: door-like */
    { "3STE", "Steam vent",            &vt_steamvent },
    { "3FER", "Ferris wheel",          &vt_ferris    },
    { "3TRC", "Tractor beam",          &vt_tractor   },
    { "3SOU", "Sound emitter",         &vt_soundfx   },
    { "3MUS", "Music trigger",         &vt_music     },
    { "3PAT", "Patrol point",          &vt_patrolpoint },
    { "3CAR", "Carl (patrol walker)",  &vt_walker    },
    { NULL, NULL, NULL }
};

const char *entity_get_type_name(const char *fourcc) {
    for (int i = 0; entity_types[i].type != NULL; i++) {
        if (strncmp(entity_types[i].type, fourcc, 4) == 0) {
            return entity_types[i].type;
        }
    }
    return "UNKN";
}

const char *entity_get_description(const char *fourcc) {
    for (int i = 0; entity_types[i].type != NULL; i++) {
        if (strncmp(entity_types[i].type, fourcc, 4) == 0) {
            return entity_types[i].description;
        }
    }
    return "Unknown entity type";
}

const EntityVTable *entity_resolve_vtable(const char *fourcc) {
    for (int i = 0; entity_types[i].type != NULL; i++) {
        if (strncmp(entity_types[i].type, fourcc, 4) == 0) {
            return entity_types[i].vt;
        }
    }
    return &vt_default;
}

/* Default AABB half-extents per FourCC, used when a behavior doesn't set its own.
   Tuned for Level1 visual scales (trees scale=100, crates=50, etc.). */
static void apply_default_extents(Entity *e) {
    if (e->half_extents[0] != 0.0f || e->half_extents[1] != 0.0f || e->half_extents[2] != 0.0f)
        return;
    if      (strncmp(e->type, "3TRE", 4) == 0) { e->half_extents[0]=50.f;  e->half_extents[1]=200.f; e->half_extents[2]=50.f;  }
    else if (strncmp(e->type, "3ROC", 4) == 0) { e->half_extents[0]=80.f;  e->half_extents[1]=120.f; e->half_extents[2]=80.f;  }
    else if (strncmp(e->type, "CRAT", 4) == 0) { e->half_extents[0]=40.f;  e->half_extents[1]=40.f;  e->half_extents[2]=40.f;  }
    else if (strncmp(e->type, "STRT", 4) == 0) { e->half_extents[0]=20.f;  e->half_extents[1]=20.f;  e->half_extents[2]=20.f;  }
    else                                       { e->half_extents[0]=40.f;  e->half_extents[1]=40.f;  e->half_extents[2]=40.f;  }
}

void entity_bind_vtables(World *w) {
    for (Entity *e = w->head; e; e = e->next) {
        e->vt = entity_resolve_vtable(e->type);
        e->runtime_flags = e->vt ? e->vt->flags : 0;
        if (e->vt && e->vt->on_spawn) e->vt->on_spawn(e, w);
        apply_default_extents(e);
    }
}

void entity_update(Entity *e, World *w, float dt) {
    if (!e || !e->vt) return;
    if (!e->alive) return;
    if (e->vt->on_update) e->vt->on_update(e, w, dt);
}
