#include "entities.h"
#include "behaviors/behaviors.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *type;
    const char *description;
    const EntityVTable *vt;
} EntityTypeInfo;

/* NOTE: vtables are external (defined per behavior_*.c). */
static const EntityTypeInfo entity_types[] = {
    { "3JIM", "Player start position", &vt_player  },
    { "3ROC", "Rocketship (rideable)", &vt_rocket  },
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
    { "3SOL", "Yokian soldier",        &vt_yokian    },
    { "3GUA", "Yokian guard",          &vt_yokian    },
    { "3SPY", "Yokian spy",            &vt_yokian    },
    { "3TUR", "Yokian turret",         &vt_yok_turret },
    { "3YSH", "Yokian ship",           &vt_yokian_ship },
    { "3FLE", "Yokian fleet commander", &vt_yokian      },
    { "3EYE", "Eye patrol actor",      &vt_eye         },
    { "3DIG", "Digger animated prop",  &vt_digger      },
    { "3HOO", "Hook AI target",        &vt_hook        },
    { "3CIN", "Cindy friend NPC",      &vt_cindy       },
    /* C3DFriends / C3DAI idle NPC cast (shared vt_friend). */
    { "3NIC", "Nick (friend NPC)",     &vt_friend      },
    { "3SHE", "Sheen (friend NPC)",    &vt_friend      },
    { "3ULT", "UltraLord (friend NPC)",&vt_friend      },
    { "3LIB", "Libby (friend NPC)",    &vt_friend      },
    { "3HUG", "Hugh (friend NPC)",     &vt_friend      },
    { "3BEN", "Benny (friend NPC)",    &vt_friend      },
    { "3MOM", "Judy (friend NPC)",     &vt_friend      },
    { "3KIT", "Kitty (C3DAI NPC)",     &vt_friend      },
    /* C3DAnimated exit-escort actors (touch -> StartPoint teleport). */
    { "3SUM", "Sumo (exit escort)",    &vt_escort      },
    { "3PIR", "Pirate (exit escort)",  &vt_escort      },
    { "3TES", "Tesla hazard",          &vt_tesla     },
    { "3LAS", "Laser trigger",         &vt_laser_trigger },
    { "3AIT", "AI/script trigger",     &vt_ai_trigger },
    /* Wave N3: player combat + pickups family. */
    { "3BAL", "Balloon (pop for score)", &vt_balloon  },
    { "3BPU", "Baseball pickup",       &vt_baseball_pickup },
    { "3BUP", "Bubble pickup",         &vt_bubble_pickup },
    { "3HEL", "Helmet pickup",         &vt_helmet    },
    { "3MEP", "Metal-can pickup",      &vt_metal_pickup },
    /* Wave N4: self-driving AI vehicles (patrol via the C3DAI base). */
    { "3SUV", "AI SUV",                &vt_ai_vehicle },
    { "3SBU", "AI school bus",         &vt_ai_vehicle },
    { "3SAI", "AI sailboat",           &vt_ai_vehicle },
    /* Wave N5: scripted cutscene cameras (invisible directors). */
    { "3CAM", "Cutscene camera",       &vt_cutscene_camera },
    { "3MCA", "Multi-cutscene camera", &vt_multi_cutscene },
    /* Actor / gameplay focus rows (post-N5). */
    { "3PHO", "Phone booth (contact prop)", &vt_phonebooth },
    { "3RCK", "AI patrol rocket",      &vt_rocket_ai },
    { "3HUM", "Humphrey (clone controller)", &vt_humphrey },
    { "3NEU", "Neutron pickup",        &vt_neutron },
    { "3RED", "Red neutron pickup",    &vt_red_neutron },
    { "3ARR", "Navigation arrow",      &vt_arrow },
    { "3LIO", "Light object",          &vt_lightobj },
    { "3OMT", "OMT shape prop",        &vt_omtobj },
    { "3CON", "Cone sprite decor",     &vt_cone },
    { "3TRO", "VR trophy (objective)", &vt_trophy },
    { "3LEA", "Leaves sprite decor",   &vt_leaves },
    { "3TAR", "Shadow sprite decor",   &vt_shadow },
    { "3AIO", "AI OMT shape prop",     &vt_ai_omtobj },
    /* Base/effect tail pass 4. */
    { "3LIG", "Scene light (data row)",              &vt_light },
    { "3FIS", "Darwin fish (set-dressing creature)", &vt_creature },
    { "3GIR", "Girl-eating plant (set-dressing creature)", &vt_creature },
    { "3SPA", "Spark wire (Tesla contact hazard)",   &vt_tesla },
    { "3STA", "Stalactite (static terrain prop)",    &vt_stalactite },
    /* Base/resolver long tail. */
    { "3ANI", "Animated sprite (frame animator)",    &vt_animsprite },
    { "3SWN", "Swing door (timed yaw swing)",        &vt_swingdoor },
    /* C3DAI set-dressing creatures (shared vt_creature; shrink-targets, see
       behavior_creature.c — active shrink->pickup mechanic deferred). */
    { "3DIN", "Dino (set-dressing creature)",        &vt_creature },
    { "3CML", "Camel (set-dressing creature)",       &vt_creature },
    { "3SPW", "Sparrow/vulture (set-dressing creature)", &vt_creature },
    /* Gated static prop / effect long tail (shared vt_prop; owned gameplay
       deferred to unported subsystems — see behavior_prop.c). */
    { "3FLA", "Flag (static prop)",                  &vt_prop },
    { "3HYD", "Fire hydrant (static prop)",          &vt_prop },
    { "3SCR", "Lab screen (display prop)",           &vt_prop },
    { "3TEL", "Teleport FX (static effect)",         &vt_prop },
    { "3SPH", "Sphere (world prop)",                 &vt_prop },
    { "3CUB", "Cube (solid invisible block)",        &vt_prop },
    { "3TOL", "Tool chest (static prop)",            &vt_prop },
    { "3OCT", "Octapuke (set-dressing hazard)",      &vt_prop },
    { "3MER", "Merry-go-round (static prop)",        &vt_prop },
    { "3TRA", "Trans-replicator (set-dressing prop)", &vt_prop },
    { "3SM1", "Smoke (effect sprite)",               &vt_prop },
    { "3FUE", "Rocket fuel (pickup sprite)",         &vt_prop },
    { "3TRI", "Trigger (base marker, inert)",        &vt_prop },
    /* SCENE-sequencer consumers (unblocked by the task-state story progression
       — visibility windows in behavior_fowl.c / behavior_cargo.c). */
    { "3FOW", "Fowl (SCENE-window creature)",        &vt_fowl },
    { "3YCA", "Yokian cargo (LEV5 SCENE-gated)",     &vt_cargo },
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

Entity *entity_spawn(World *w, const char *type, const char *tag,
                     float x, float y, float z) {
    Entity *e = world_add(w);
    if (!e) return NULL;
    snprintf(e->type, sizeof(e->type), "%s", type);
    if (tag) snprintf(e->tag, sizeof(e->tag), "%s", tag);
    e->x = x; e->y = y; e->z = z;
    e->vt = entity_resolve_vtable(e->type);
    e->runtime_flags = e->vt ? e->vt->flags : 0;
    if (e->vt && e->vt->on_spawn) e->vt->on_spawn(e, w);
    apply_default_extents(e);
    return e;
}

void entity_update(Entity *e, World *w, float dt) {
    if (!e || !e->vt) return;
    if (!e->alive) return;
    if (e->vt->on_update) e->vt->on_update(e, w, dt);
}
