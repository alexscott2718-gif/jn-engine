/* gadget_menu.c -- the in-game action menu (AMI) and action-mode dispatch.
   See gadget_menu.h for what is ported from evidence and what deliberately is
   not. */
#include "gadget_menu.h"
#include "gamestate.h"
#include "ui_text.h"
#include "entity_visual.h"
#include "behaviors/behaviors.h"
#include "../engine/renderer.h"
#include "../engine/input.h"
#include "../engine/assets/asset_cache.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

/* --- The globals the recovered bodies read and write ----------------------
   Named for what they are, with the original's symbol alongside so the bodies
   in docs/decomp/evidence/c3djimmy_target6.md stay greppable against this. */
static int   g_available   = 1;    /* DAT_004ec494  -- set by the ctor */
static int   g_open        = 0;    /* DAT_004f8181  -- the open latch */
static int   g_action_mode = ACTION_MODE_NONE;   /* DAT_004f0588 */
static int   g_input_gate  = 1;    /* DAT_004f8434  -- 0 while open, 1 after */
static int   g_input_latch = 0;    /* DAT_004f8182  -- cleared on exit */
static float g_menu_timer  = 0.0f; /* DAT_004f8188  -- seeded 20.0 on enter */

static int g_sel = 0;              /* selected gadget ordinal */

/* --- AMI tables ----------------------------------------------------------
   SelectJimmyGadgetOrVRMode (00428d50) is one switch over the request id.
   Every arm does the same two things: write DAT_004f0588, and -- when the
   game-type probe returns 2 -- issue controller command 0x4b8(0x9b) and route
   to a VR level through (vrNN.gam, "PHONEBOOTH", ...).

   The routes are a clean id -> vr(id+1) ladder, which is worth noting because
   it is an independent cross-check: CMainMenu's already-certified level table
   carries the same eight VR levels, and the two agree.

   Two arms write one of two modes depending on a branch whose condition was
   not recovered. The table carries the primary; the alternate is in the
   comment rather than silently dropped. */
typedef struct {
    int         mode;       /* written to DAT_004f0588; INT_MIN = no write */
    const char *vr_level;   /* VR route, NULL if the arm has none */
} AmiEntry;

#define AMI_NO_MODE (-32768)

static const AmiEntry AMI[AMI_ID_MAX + 1] = {
    /* 0 */ { ACTION_MODE_IDLE,   "vr01" },  /* also writes -1 on the other arm */
    /* 1 */ { ACTION_MODE_ROCKET, "vr02" },
    /* 2 */ { ACTION_MODE_2,      "vr03" },  /* also writes -1 on the other arm */
    /* 3 */ { AMI_NO_MODE,        "vr04" },  /* the default arm, shared with -1 */
    /* 4 */ { ACTION_MODE_4,      "vr05" },
    /* 5 */ { ACTION_MODE_5,      "vr06" },
    /* 6 */ { ACTION_MODE_AIM,    "vr07" },
    /* 7 */ { ACTION_MODE_7,      "vr08" },
    /* 8 */ { AMI_NO_MODE,        NULL   },  /* no mode write, no route */
};

const char *ami_vr_level(int ami_id) {
    if (ami_id < 0 || ami_id > AMI_ID_MAX) return NULL;
    return AMI[ami_id].vr_level;
}

int ami_dispatch(int ami_id) {
    /* The original opens with the trace "CAll in AMI %d" and closes with
       "Exiting AMI"; keeping them makes a native run diffable against a
       trace capture from the real executable. */
    printf("[AMI] CAll in AMI %d\n", ami_id);
    if (ami_id >= 0 && ami_id <= AMI_ID_MAX && AMI[ami_id].mode != AMI_NO_MODE)
        g_action_mode = AMI[ami_id].mode;
    printf("[AMI] Exiting AMI (mode %d)\n", g_action_mode);
    return g_action_mode;
}

int  action_mode(void)         { return g_action_mode; }
void action_mode_set(int mode) { g_action_mode = mode; }

void gadget_menu_set_available(int available) { g_available = !!available; }
int  gadget_menu_is_open(void)                { return g_open; }

/* JimmyEnterActionMenuLock (00425ef0). The order below is the decompiled
   order. Most of the body is controller-slot traffic that native has no
   counterpart for (0x4c4, 0x488, 0x45c, 0x4ac, 0x4b4, 0x460, 0x4ec and the
   direct +0x4c0/+0x4c4 writes); what survives the translation is the guard,
   the pause, the cursor, the cooldown clear, and the flag state. */
int gadget_menu_open(int param) {
    if (!g_available || g_open) return 0;     /* DAT_004ec494 && !DAT_004f8181 */

    /* Pause. In the original this is global game slot 0x168(1); here it is the
       same freeze the front-end menu already uses -- main's update gate skips
       the whole simulation while a menu is open. */
    g_open = 1;                               /* DAT_004f8181 = 1 */

    /* The cursor is shown unless the enter argument is 2. What 2 means was not
       recovered, so it stays a parameter rather than being folded away. */
    if (param != 2) SDL_ShowCursor(SDL_ENABLE);

    g_input_gate = 0;                         /* DAT_004f8434 = 0 */
    g_menu_timer = 20.0f;                     /* DAT_004f8188 = 20.0 */
    g_sel = 0;

    printf("[AMI] menu open (%d gadget%s)\n", gamestate_gadget_count(),
           gamestate_gadget_count() == 1 ? "" : "s");
    return 1;
}

/* JimmyExitActionMenuLock (00425b20). */
void gadget_menu_close(void) {
    if (!g_open) return;                      /* DAT_004f8181 != 0 */
    g_open = 0;

    /* The original hides the cursor unless controller short +0x4d4 is 2 -- the
       same unrecovered "2" as on the way in. Native has no controller to ask,
       so the cursor always goes back; noted so the asymmetry is not mistaken
       for an oversight. */
    SDL_ShowCursor(SDL_DISABLE);

    g_input_gate  = 1;                        /* DAT_004f8434 = 1 */
    g_input_latch = 0;                        /* DAT_004f8182 = 0 */
    printf("[AMI] menu close (mode %d)\n", g_action_mode);
}

void gadget_menu_input(void) {
    if (!g_open) return;
    int n = gamestate_gadget_count();

    if (input_just_pressed(SDL_SCANCODE_TAB) ||
        input_just_pressed(SDL_SCANCODE_ESCAPE)) { gadget_menu_close(); return; }
    if (n <= 0) return;

    if (input_just_pressed(SDL_SCANCODE_UP))   g_sel = (g_sel + n - 1) % n;
    if (input_just_pressed(SDL_SCANCODE_DOWN)) g_sel = (g_sel + 1) % n;

    if (input_just_pressed(SDL_SCANCODE_RETURN) ||
        input_just_pressed(SDL_SCANCODE_KP_ENTER)) {
        gamestate_set_active_gadget(g_sel);
        const InventorySlot *s = gamestate_gadget_at(g_sel);
        if (s) printf("[AMI] Activating Item %d ('%s')\n", g_sel, s->tag);
        /* One gadget is live so far. The original routes a selection through
           the C2DInGameMenu controller and an AMI id; that mapping is not
           recovered (see the header), so the selection dispatches by identity
           until it is. */
        if (s && strcmp(s->tag, "scooter") == 0)
            behavior_scooter_activate();
        gadget_menu_close();
    }
}

void gadget_menu_draw(int vw, int vh) {
    if (!g_open || vw <= 0 || vh <= 0) return;

    /* Dim the frozen scene, same treatment the front-end menu gives it. */
    renderer_draw_screen_rect(vw, vh, 0, 0, (float)vw, (float)vh,
                              0.02f, 0.04f, 0.10f, 0.55f);

    const float s = (float)vh / 480.0f;
    const float cell = 72.0f * s;
    const float gap  = 10.0f * s;
    const float ts   = 1.5f;
    const float th   = ui_text_line_height(ts);

    int n = gamestate_gadget_count();
    float total = n > 0 ? n * (cell + gap) - gap : cell;
    float x = ((float)vw - cell) * 0.5f;
    float y = ((float)vh - total) * 0.5f;

    ui_text_draw_centered(vw, vh, (float)vw * 0.5f, y - th - gap * 2.0f,
                          ts, "GADGETS", 0.95f, 0.85f, 0.35f, 1.0f);

    if (n <= 0) {
        ui_text_draw_centered(vw, vh, (float)vw * 0.5f, y + cell * 0.5f,
                              ts, "NONE", 0.70f, 0.70f, 0.75f, 1.0f);
        return;
    }

    for (int i = 0; i < n; i++) {
        const InventorySlot *g = gamestate_gadget_at(i);
        if (!g) continue;
        int on = (i == g_sel);
        float cy = y + i * (cell + gap);

        renderer_draw_screen_rect(vw, vh, x, cy, cell, cell,
                                  on ? 0.95f : 0.20f,
                                  on ? 0.80f : 0.24f,
                                  on ? 0.25f : 0.34f,
                                  on ? 0.95f : 0.70f);

        const char *icon = g->icon_path;
        if (!icon && g->sprite > 0 && !sprite_chunk_is_hidden(g->sprite))
            icon = sprite_chunk_path(g->sprite);
        if (icon) {
            unsigned int tex = tex_cache_get(icon);
            if (tex) {
                float pad = cell * 0.12f;
                renderer_draw_sprite_2d(tex, vw, vh, x + pad, cy + pad,
                                        cell - pad * 2.0f, cell - pad * 2.0f,
                                        1, 1, 1, 1);
            }
        }

        /* Label from the artist's canvas name, not the .gam ObjectTag. The
           two disagree for every gadget in the table -- level1b's `shrinkray`
           row draws the canvas named "Jetpack 1", and the owner collected it
           and quite reasonably called it a jetpack. The player is looking at
           the art, so the art names it. The tag stays the corpus key. */
        const char *label = g->sprite > 0 ? sprite_chunk_name(g->sprite) : "";
        if (!label[0]) label = g->tag;

        ui_text_draw(vw, vh, x + cell + gap, cy + (cell - th) * 0.5f, ts,
                     label,
                     on ? 1.00f : 0.85f,
                     on ? 0.95f : 0.85f,
                     on ? 0.45f : 0.90f, 1.0f);
    }
}
