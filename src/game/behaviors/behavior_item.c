#include "behaviors.h"
#include "../gamestate.h"
#include <stddef.h>
#include <string.h>
#include <math.h>

#define ITEM_BOB_AMP   12.0f
#define ITEM_BOB_FREQ  1.2f
#define ITEM_SPIN_RATE 2.4f

static void item_on_spawn(Entity *e, World *w) {
    (void)w;
    e->half_extents[0] = 30.0f;
    e->half_extents[1] = 30.0f;
    e->half_extents[2] = 30.0f;
    e->user_flag = 0;             /* 0 = uncollected, 1 = collected */
    e->user_float = e->y;         /* base y for bob */
    gamestate_item_added();
}

static void item_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!e->alive) return;
    static float t = 0.0f;
    t += dt;
    e->y  = e->user_float + ITEM_BOB_AMP * sinf(6.28318f * ITEM_BOB_FREQ * t + e->x * 0.01f);
    e->ry += ITEM_SPIN_RATE * dt;
}

/* Tool-granting pickups. Matched as a case-insensitive substring of the GAM
   ObjectTag so variants (pickupwatergun / WATERGUN1 / activatewatergun) all map
   to one inventory tool. Icons are authentic sprites pulled from sprites.omt. */
typedef struct { const char *tag; const char *tool; const char *icon; } ToolGrant;
static const ToolGrant TOOL_GRANTS[] = {
    { "watergun", "watergun", "assets/hud/tool_watergun.png" },
    { "glasses",  "glasses",  "assets/hud/tool_glasses.png"  },
    { "jetpack",  "jetpack",  "assets/hud/tool_jetpack.png"  },
    { "wrench",   "wrench",   "assets/hud/tool_wrench.png"   },
    { "megaburp", "burpgun",  "assets/hud/tool_burpgun.png"  },
    { "burpgun",  "burpgun",  "assets/hud/tool_burpgun.png"  },
    { "tools04",  "tools",    "assets/hud/tool_wrench.png"   },
    { "fowlkey",  "fowlkey",  "assets/hud/tool_key.png"      },
    { "key",      "key",      "assets/hud/tool_key.png"      },
};

/* Case-insensitive substring search (needle in haystack). */
static int tag_contains(const char *haystack, const char *needle) {
    if (!haystack[0] || !needle[0]) return 0;
    size_t nl = strlen(needle);
    for (const char *p = haystack; *p; p++) {
        size_t i = 0;
        while (i < nl && p[i] &&
               (p[i] | 0x20) == (needle[i] | 0x20)) i++;
        if (i == nl) return 1;
    }
    return 0;
}

static void item_grant_tool(const Entity *e) {
    for (size_t i = 0; i < sizeof(TOOL_GRANTS) / sizeof(TOOL_GRANTS[0]); i++) {
        if (tag_contains(e->tag, TOOL_GRANTS[i].tag)) {
            gamestate_grant_tool(TOOL_GRANTS[i].tool, TOOL_GRANTS[i].icon);
            return;  /* first match wins */
        }
    }
}

static void item_on_trigger(Entity *e, Entity *by) {
    (void)by;
    if (e->user_flag) return;
    e->user_flag = 1;
    e->alive = 0;
    /* Typed counters: gems tally separately; all pickups award their Points. */
    if (strncmp(e->type, "3GEM", 4) == 0)
        gamestate_gem_collected();
    if (e->points)
        gamestate_add_points(e->points);
    item_grant_tool(e);
    gamestate_item_collected();
}

const EntityVTable vt_item = {
    .on_spawn = item_on_spawn,
    .on_update = item_on_update,
    .on_trigger = item_on_trigger,
    .flags = ENTITY_FLAG_TRIGGER,
};
