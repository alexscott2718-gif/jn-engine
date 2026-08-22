/* In-game QA annotation mode — engine side (docs/qa_annotate_plan.md, M1).
   Owns the per-frame object table built during scene enumeration, hover and
   selection state, and the JSON event bridge to the web shell. Everything
   human-facing (tooltip, dialog, tag stack, clipboard) is JS in shell.html. */

#include "qa.h"

#include <stdio.h>
#include <string.h>

#include "../engine/renderer.h"
#include "qa_card.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#include <SDL.h>   /* native fallback: pick JSON -> SDL_SetClipboardText */
#endif

/* One row per registered scene object; the row index + 1 is the pick ID
   (renderer reserves 0 for background). Rebuilt every enumerated frame. */
typedef struct QaObject {
    int   kind;                 /* 0 = entity, 1 = placement */
    char  name[64];             /* placement mesh name / entity FourCC */
    char  tag[64];              /* entity ObjectTag ("" for placements) */
    char  asset[160];           /* source mesh/sprite when known */
    float x, y, z;              /* drawn world position */
    float ax, ay, az;           /* authored position (OMT center / .gam) */
} QaObject;

#define QA_MAX_OBJECTS 2048
static QaObject     g_objects[QA_MAX_OBJECTS];
static int          g_count = 0;

static int          g_active = 0;
static int          g_pick_pass = 0;
static unsigned int g_hover_id = 0;     /* 0 = none; otherwise table idx + 1 */
static unsigned int g_selected_id = 0;
static int          g_cursor_x = -1, g_cursor_y = -1;
static int          g_click_pending = 0;
static unsigned int g_last_pick_ms = 0;

#define QA_PICK_INTERVAL_MS 100u

/* Hover = yellow wash, selection = orange (matches the plan's dialog flow:
   selection stays lit while the JS dialog is open). */
static const float QA_HOVER_TINT[4]  = { 1.0f, 1.0f, 0.25f, 0.40f };
static const float QA_SELECT_TINT[4] = { 1.0f, 0.55f, 0.10f, 0.60f };

int qa_active(void) { return g_active; }

/* ---- event bridge ------------------------------------------------------ */

/* Escape for JSON string content. Asset/tag names are filesystem-safe ASCII
   in practice; this guards the odd quote/backslash rather than full UTF-8. */
static void json_escape(const char *in, char *out, size_t cap) {
    size_t o = 0;
    for (; *in && o + 2 < cap; in++) {
        unsigned char c = (unsigned char)*in;
        if (c == '"' || c == '\\') { out[o++] = '\\'; out[o++] = (char)c; }
        else if (c < 0x20)         { out[o++] = ' '; }
        else                       { out[o++] = (char)c; }
    }
    out[o] = '\0';
}

static void qa_emit(const char *json) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (window.jnQA && window.jnQA.onEvent)
            window.jnQA.onEvent(UTF8ToString($0));
    }, json);
#else
    printf("[QA] %s\n", json);
#endif
}

static void qa_emit_mode(void) {
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"ev\":\"mode\",\"active\":%d}", g_active);
    qa_emit(buf);
}

static void qa_emit_object(const char *ev, unsigned int id,
                           const char *level_name,
                           float cx, float cy, float cz, float yaw) {
    if (id == 0 || id > (unsigned int)g_count) {
        char buf[96];
        snprintf(buf, sizeof(buf), "{\"ev\":\"%s\",\"id\":0}", ev);
        qa_emit(buf);
        return;
    }
    const QaObject *o = &g_objects[id - 1];
    char name[136], tag[136], asset[328], level[136];
    json_escape(o->name, name, sizeof(name));
    json_escape(o->tag, tag, sizeof(tag));
    json_escape(o->asset, asset, sizeof(asset));
    json_escape(level_name ? level_name : "", level, sizeof(level));
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "{\"ev\":\"%s\",\"id\":%u,\"kind\":\"%s\",\"name\":\"%s\","
        "\"tag\":\"%s\",\"asset\":\"%s\","
        "\"pos\":[%.1f,%.1f,%.1f],\"authored_pos\":[%.1f,%.1f,%.1f],"
        "\"level\":\"%s\","
        "\"cam\":{\"pos\":[%.1f,%.1f,%.1f],\"yaw\":%.4f}}",
        ev, id, o->kind == 1 ? "placement" : "entity",
        name, tag, asset,
        o->x, o->y, o->z, o->ax, o->ay, o->az,
        level, cx, cy, cz, yaw);
    qa_emit(buf);
#ifndef __EMSCRIPTEN__
    /* Native degraded mode (plan M5): no dialog/tag UI exists, so every pick
       lands its identity JSON on the system clipboard, ready to paste into a
       report. Web builds handle clipboard in shell.html instead. */
    if (strcmp(ev, "pick") == 0) {
        SDL_SetClipboardText(buf);
        /* ...and open the in-engine card, which is the half M5 left out: the
           clipboard carries *what* you clicked, the card carries *what is
           wrong with it*. Web builds skip this -- shell.html owns their UI. */
        qa_card_open(o->kind == 1 ? "placement" : "entity",
                     o->name, o->tag, o->asset, level_name ? level_name : "",
                     o->x, o->y, o->z, o->ax, o->ay, o->az,
                     cx, cy, cz, yaw);
    }
#endif
}

/* ---- mode + input ------------------------------------------------------ */

/* KEEPALIVE: also called from the shell's "QA" button (mobile has no Q key).
   Button label syncs off the resulting "mode" event, never local JS state. */
EMSCRIPTEN_KEEPALIVE
void qa_toggle(void) {
    g_active = !g_active;
    if (!g_active) {
        g_hover_id = 0;
        g_selected_id = 0;
        g_click_pending = 0;
        renderer_set_highlight(0, 0, 0, 0);
    }
    printf("[QA] annotation mode %s\n", g_active ? "ON" : "OFF");
    qa_emit_mode();
}

void qa_on_mouse_motion(int x, int y) { g_cursor_x = x; g_cursor_y = y; }

void qa_on_mouse_click(int x, int y) {
    g_cursor_x = x; g_cursor_y = y;
    g_click_pending = 1;
}

int qa_want_pick(unsigned int now_ms) {
    if (!g_active) return 0;
    if (g_click_pending) return 1;
    if (g_cursor_x < 0) return 0;
    if (now_ms - g_last_pick_ms < QA_PICK_INTERVAL_MS) return 0;
    g_last_pick_ms = now_ms;
    return 1;
}

void qa_get_cursor(int *x, int *y) { *x = g_cursor_x; *y = g_cursor_y; }

/* ---- scene enumeration ------------------------------------------------- */

void qa_begin_scene(int pick_pass) {
    g_pick_pass = pick_pass;
    g_count = 0;
    if (g_active && pick_pass)
        renderer_pick_set_id(0);
}

/* Shared tail: assign the ID and either install it (pick pass) or light the
   object up (main pass) for the draws that follow until the next register. */
static void qa_register_common(void) {
    unsigned int id = (unsigned int)g_count;   /* row idx + 1, set by caller */
    if (g_pick_pass) {
        renderer_pick_set_id(id);
    } else if (id == g_selected_id) {
        renderer_set_highlight(QA_SELECT_TINT[0], QA_SELECT_TINT[1],
                               QA_SELECT_TINT[2], QA_SELECT_TINT[3]);
    } else if (id == g_hover_id) {
        renderer_set_highlight(QA_HOVER_TINT[0], QA_HOVER_TINT[1],
                               QA_HOVER_TINT[2], QA_HOVER_TINT[3]);
    } else {
        renderer_set_highlight(0, 0, 0, 0);
    }
}

void qa_register_entity(const Entity *e) {
    if (!g_active) return;
    if (g_count >= QA_MAX_OBJECTS) { if (g_pick_pass) renderer_pick_set_id(0); return; }
    QaObject *o = &g_objects[g_count++];
    o->kind = 0;
    snprintf(o->name, sizeof(o->name), "%s", e->type);
    snprintf(o->tag, sizeof(o->tag), "%s", e->tag);
    if (e->ase_file[0])
        snprintf(o->asset, sizeof(o->asset), "%s", e->ase_file);
    else if (e->sprite_index > 0)
        snprintf(o->asset, sizeof(o->asset), "sprite_index:%d", e->sprite_index);
    else
        o->asset[0] = '\0';
    o->x = e->x; o->y = e->y; o->z = e->z;
    o->ax = e->x; o->ay = e->y; o->az = e->z;
    qa_register_common();
}

void qa_register_placement(const WorldPlacement *pl,
                           float draw_x, float draw_y, float draw_z) {
    if (!g_active) return;
    if (g_count >= QA_MAX_OBJECTS) { if (g_pick_pass) renderer_pick_set_id(0); return; }
    QaObject *o = &g_objects[g_count++];
    o->kind = 1;
    snprintf(o->name, sizeof(o->name), "%s", pl->name);
    o->tag[0] = '\0';
    snprintf(o->asset, sizeof(o->asset), "%s", pl->ase_path);
    o->x = draw_x; o->y = draw_y; o->z = draw_z;
    o->ax = pl->x; o->ay = pl->y; o->az = pl->z;
    qa_register_common();
}

void qa_end_scene(void) {
    if (g_active && !g_pick_pass)
        renderer_set_highlight(0, 0, 0, 0);
}

/* ---- pick resolution --------------------------------------------------- */

void qa_pick_resolve(unsigned int id, const char *level_name,
                     float cam_x, float cam_y, float cam_z, float cam_yaw) {
    if (id > (unsigned int)g_count) id = 0;   /* stale read; treat as none */
    if (g_click_pending) {
        g_click_pending = 0;
        g_selected_id = id;
        qa_emit_object("pick", id, level_name, cam_x, cam_y, cam_z, cam_yaw);
    } else if (id != g_hover_id) {
        g_hover_id = id;
        qa_emit_object("hover", id, level_name, cam_x, cam_y, cam_z, cam_yaw);
    }
}

EMSCRIPTEN_KEEPALIVE
void qa_clear_selection(void) {
    g_selected_id = 0;
}
