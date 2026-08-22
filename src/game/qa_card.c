/* Native QA card editor + JSON export -- see qa_card.h for why this exists.
   Mirrors the web shell's dialog / tag stack / export triple (M2/M3 of
   docs/qa_annotate_plan.md) using the engine's own overlay + text renderer. */

#include "qa_card.h"
#include "ui_text.h"
#include "../engine/renderer.h"

#include <SDL.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

/* §6's fixed vocabulary, plus COL. The web list has no collision category
   because it was written for misplaced/misdrawn art; the surfaces you can walk
   through are the reason a native editor was asked for, so the vocabulary
   needs a word for them. Additive -- every existing web code still parses. */
typedef struct { const char *code; const char *label; } QaCategory;
static const QaCategory QA_CATEGORIES[] = {
    { "COL", "Collision missing or wrong" },
    { "PLC", "Placement wrong position"   },
    { "ORI", "Orientation or rotation"    },
    { "SCL", "Scale"                      },
    { "ANI", "Animation wrong or missing" },
    { "TEX", "Texture wrong or missing"   },
    { "MIS", "Model missing"              },
    { "GFX", "Other visual glitch"        },
    { "OTH", "Other"                      },
};
static const int QA_CATEGORY_COUNT =
    (int)(sizeof(QA_CATEGORIES) / sizeof(QA_CATEGORIES[0]));

#define QA_MSG_MAX   160
#define QA_CARD_MAX  256

typedef struct QaCard {
    char  kind[16];
    char  name[64];
    char  tag[64];
    char  asset[160];
    char  level[64];
    char  category[8];
    char  message[QA_MSG_MAX];
    char  ts[32];               /* ISO-8601 UTC */
    float pos[3];               /* drawn world position of the picked object */
    float authored[3];          /* authored OMT/.gam position */
    float player[3];            /* where the reporter was standing */
    float cam[3];
    float cam_yaw;
} QaCard;

static QaCard g_cards[QA_CARD_MAX];
static int    g_card_count = 0;

/* The card currently being edited, and whether the dialog is up. */
static QaCard g_edit;
static int    g_active   = 0;
static int    g_cat_idx  = 0;
static int    g_msg_len  = 0;

static float  g_player[3] = { 0.0f, 0.0f, 0.0f };

/* Transient banner ("saved", "exported 4 cards") shown under the tag stack. */
static char   g_status[96];

int qa_card_active(void) { return g_active; }
int qa_card_count(void)  { return g_card_count; }

void qa_card_note_player(float x, float y, float z) {
    g_player[0] = x; g_player[1] = y; g_player[2] = z;
}

static void iso_now(char *out, size_t cap) {
    time_t t = time(NULL);
    struct tm gm;
#ifdef _WIN32
    struct tm *p = gmtime(&t);
    if (p) gm = *p; else memset(&gm, 0, sizeof gm);
#else
    if (!gmtime_r(&t, &gm)) memset(&gm, 0, sizeof gm);
#endif
    if (strftime(out, cap, "%Y-%m-%dT%H:%M:%SZ", &gm) == 0)
        snprintf(out, cap, "unknown");
}

void qa_card_open(const char *kind, const char *name, const char *tag,
                  const char *asset, const char *level,
                  float px, float py, float pz,
                  float ax, float ay, float az,
                  float cam_x, float cam_y, float cam_z, float cam_yaw) {
    memset(&g_edit, 0, sizeof g_edit);
    snprintf(g_edit.kind,  sizeof g_edit.kind,  "%s", kind  ? kind  : "");
    snprintf(g_edit.name,  sizeof g_edit.name,  "%s", name  ? name  : "");
    snprintf(g_edit.tag,   sizeof g_edit.tag,   "%s", tag   ? tag   : "");
    snprintf(g_edit.asset, sizeof g_edit.asset, "%s", asset ? asset : "");
    snprintf(g_edit.level, sizeof g_edit.level, "%s", level ? level : "");
    g_edit.pos[0] = px; g_edit.pos[1] = py; g_edit.pos[2] = pz;
    g_edit.authored[0] = ax; g_edit.authored[1] = ay; g_edit.authored[2] = az;
    g_edit.player[0] = g_player[0];
    g_edit.player[1] = g_player[1];
    g_edit.player[2] = g_player[2];
    g_edit.cam[0] = cam_x; g_edit.cam[1] = cam_y; g_edit.cam[2] = cam_z;
    g_edit.cam_yaw = cam_yaw;

    g_msg_len = 0;
    g_edit.message[0] = '\0';
    g_active = 1;
    /* Keep the last category: a reporter logging a wall run files COL, COL,
       COL, and re-picking it every time is the kind of friction that stops
       people filing the fourth one. */
    SDL_StartTextInput();
    printf("[QACARD] open for %s '%s'\n", g_edit.kind, g_edit.name);
}

static void qa_card_close(void) {
    g_active = 0;
    SDL_StopTextInput();
}

static void qa_card_save(void) {
    if (g_card_count >= QA_CARD_MAX) {
        snprintf(g_status, sizeof g_status, "card list full at %d", QA_CARD_MAX);
        qa_card_close();
        return;
    }
    snprintf(g_edit.category, sizeof g_edit.category, "%s",
             QA_CATEGORIES[g_cat_idx].code);
    iso_now(g_edit.ts, sizeof g_edit.ts);
    g_cards[g_card_count++] = g_edit;
    snprintf(g_status, sizeof g_status, "saved card %d", g_card_count);
    printf("[QACARD] saved [%s] %s '%s' @ (%.0f %.0f %.0f) player (%.0f %.0f %.0f) %s\n",
           g_edit.category, g_edit.kind, g_edit.name,
           g_edit.pos[0], g_edit.pos[1], g_edit.pos[2],
           g_edit.player[0], g_edit.player[1], g_edit.player[2],
           g_edit.message);
    qa_card_close();
}

int qa_card_undo(void) {
    if (g_card_count <= 0) return 0;
    g_card_count--;
    snprintf(g_status, sizeof g_status, "removed last card %d left", g_card_count);
    printf("[QACARD] undo; %d card(s) left\n", g_card_count);
    return 1;
}

void qa_card_key(int sdl_keycode) {
    if (!g_active) return;
    switch (sdl_keycode) {
    case SDLK_ESCAPE:
        printf("[QACARD] cancelled\n");
        snprintf(g_status, sizeof g_status, "cancelled");
        qa_card_close();
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        qa_card_save();
        break;
    case SDLK_BACKSPACE:
        if (g_msg_len > 0) g_edit.message[--g_msg_len] = '\0';
        break;
    case SDLK_UP:
    case SDLK_LEFT:
        g_cat_idx = (g_cat_idx + QA_CATEGORY_COUNT - 1) % QA_CATEGORY_COUNT;
        break;
    case SDLK_DOWN:
    case SDLK_RIGHT:
    case SDLK_TAB:
        g_cat_idx = (g_cat_idx + 1) % QA_CATEGORY_COUNT;
        break;
    default:
        break;
    }
}

void qa_card_text(const char *utf8) {
    if (!g_active || !utf8) return;
    for (const char *p = utf8; *p; p++) {
        unsigned char c = (unsigned char)*p;
        /* Accept exactly what the atlas can draw. A character with no glyph
           would type as an invisible advance, which reads as a dropped
           keystroke -- so drop it here instead of showing a phantom gap. */
        int ok = (c == ' ') || (ui_text_glyph_index((char)c) >= 0);
        if (!ok) continue;
        if (g_msg_len >= QA_MSG_MAX - 1) return;
        g_edit.message[g_msg_len++] = (char)c;
        g_edit.message[g_msg_len] = '\0';
    }
}

/* ---- export ------------------------------------------------------------ */

static void json_escape(const char *in, char *out, size_t cap) {
    size_t o = 0;
    for (; in && *in && o + 2 < cap; in++) {
        unsigned char c = (unsigned char)*in;
        if (c == '"' || c == '\\') { out[o++] = '\\'; out[o++] = (char)c; }
        else if (c < 0x20)         { out[o++] = ' '; }
        else                       { out[o++] = (char)c; }
    }
    if (cap) out[o] = '\0';
}

/* Builds §7's payload: a markdown table humans skim, then the fenced JSON
   agents parse. Returns the length written (excluding the terminator). */
static size_t build_payload(char *buf, size_t cap) {
    size_t n = 0;
    char date[32];
    iso_now(date, sizeof date);
    date[10] = '\0';   /* YYYY-MM-DD */

    n += (size_t)snprintf(buf + n, cap > n ? cap - n : 0,
        "## JN QA session - %s (%d report%s)\n\n"
        "| level | object | category | pos | player pos | issue |\n"
        "|---|---|---|---|---|---|\n",
        date, g_card_count, g_card_count == 1 ? "" : "s");

    for (int i = 0; i < g_card_count && n < cap; i++) {
        const QaCard *c = &g_cards[i];
        n += (size_t)snprintf(buf + n, cap > n ? cap - n : 0,
            "| %s | %s | %s | %.0f, %.0f, %.0f | %.0f, %.0f, %.0f | %s |\n",
            c->level, c->name[0] ? c->name : c->tag, c->category,
            c->pos[0], c->pos[1], c->pos[2],
            c->player[0], c->player[1], c->player[2],
            c->message);
    }

    n += (size_t)snprintf(buf + n, cap > n ? cap - n : 0, "\n```json\n[\n");
    for (int i = 0; i < g_card_count && n < cap; i++) {
        const QaCard *c = &g_cards[i];
        char name[136], tag[136], asset[328], level[136], msg[344];
        json_escape(c->name,    name,  sizeof name);
        json_escape(c->tag,     tag,   sizeof tag);
        json_escape(c->asset,   asset, sizeof asset);
        json_escape(c->level,   level, sizeof level);
        json_escape(c->message, msg,   sizeof msg);
        n += (size_t)snprintf(buf + n, cap > n ? cap - n : 0,
            "  { \"level\": \"%s\", \"kind\": \"%s\", \"name\": \"%s\", "
            "\"tag\": \"%s\", \"asset\": \"%s\", \"category\": \"%s\", "
            "\"pos\": [%.1f, %.1f, %.1f], \"omt_pos\": [%.1f, %.1f, %.1f], "
            "\"player_pos\": [%.1f, %.1f, %.1f], "
            "\"cam\": { \"pos\": [%.1f, %.1f, %.1f], \"yaw\": %.4f }, "
            "\"message\": \"%s\", \"ts\": \"%s\" }%s\n",
            level, c->kind, name, tag, asset, c->category,
            c->pos[0], c->pos[1], c->pos[2],
            c->authored[0], c->authored[1], c->authored[2],
            c->player[0], c->player[1], c->player[2],
            c->cam[0], c->cam[1], c->cam[2], c->cam_yaw,
            msg, c->ts, i + 1 < g_card_count ? "," : "");
    }
    n += (size_t)snprintf(buf + n, cap > n ? cap - n : 0, "]\n```\n");
    return n < cap ? n : cap - 1;
}

/* One static buffer rather than a stack array: 256 cards of JSON is well past
   what a default 1MB Windows thread stack wants to carry. */
static char g_payload[QA_CARD_MAX * 900 + 4096];

int qa_card_export(void) {
    if (g_card_count == 0) {
        snprintf(g_status, sizeof g_status, "nothing to export");
        printf("[QACARD] export: no cards\n");
        return 0;
    }
    build_payload(g_payload, sizeof g_payload);

    /* Written to the working directory, which PLAY.bat pins to the bundle
       folder -- so the files land next to jnengine.exe where a reporter will
       actually find them. */
    const char *md_path   = "jn-qa-report.md";
    const char *json_path = "jn-qa-report.json";

    FILE *f = fopen(md_path, "w");
    if (!f) {
        snprintf(g_status, sizeof g_status, "export failed cannot write file");
        fprintf(stderr, "[QACARD] export: cannot open %s\n", md_path);
        return -1;
    }
    fputs(g_payload, f);
    fclose(f);

    /* The JSON half again on its own, so tooling does not have to strip the
       markdown fence. */
    f = fopen(json_path, "w");
    if (f) {
        const char *start = strstr(g_payload, "```json\n");
        if (start) {
            start += 8;
            const char *end = strstr(start, "```");
            if (end) fwrite(start, 1, (size_t)(end - start), f);
        }
        fclose(f);
    }

    SDL_SetClipboardText(g_payload);
    snprintf(g_status, sizeof g_status, "exported %d cards to jn qa report",
             g_card_count);
    printf("[QACARD] exported %d card(s) -> %s + %s (and the clipboard)\n",
           g_card_count, md_path, json_path);
    return g_card_count;
}

/* ---- drawing ----------------------------------------------------------- */

static void draw_line(int vw, int vh, float x, float y, float scale,
                      const char *s, float r, float g, float b) {
    ui_text_draw(vw, vh, x, y, scale, s, r, g, b, 1.0f);
}

/* Tag stack, top-right, mirroring the web shell's. Always drawn while cards
   exist so a reporter can see the session growing without opening anything. */
static void draw_tag_stack(int vw, int vh) {
    const float scale = 1.0f;
    const float lh    = ui_text_line_height(scale);
    const float pad   = 8.0f;
    int shown = g_card_count < 8 ? g_card_count : 8;
    if (shown == 0 && g_status[0] == '\0') return;

    float bw = vw * 0.30f;
    float bh = (shown + 2) * (lh + 4.0f) + pad * 2.0f;
    float x  = (float)vw - bw - 16.0f;
    float y  = 16.0f;

    renderer_draw_screen_rect(vw, vh, x, y, bw, bh, 0.04f, 0.06f, 0.12f, 0.80f);

    char hdr[64];
    snprintf(hdr, sizeof hdr, "QA CARDS %d", g_card_count);
    draw_line(vw, vh, x + pad, y + pad, scale, hdr, 0.95f, 0.80f, 0.25f);

    float ly = y + pad + lh + 4.0f;
    for (int i = g_card_count - shown; i < g_card_count; i++) {
        const QaCard *c = &g_cards[i];
        char row[96];
        snprintf(row, sizeof row, "%s %s %s", c->category,
                 c->name[0] ? c->name : c->tag, c->level);
        draw_line(vw, vh, x + pad, ly, scale, row, 0.85f, 0.90f, 0.98f);
        ly += lh + 4.0f;
    }
    if (g_status[0])
        draw_line(vw, vh, x + pad, ly, scale, g_status, 0.55f, 0.85f, 0.55f);
}

void qa_card_draw(int vw, int vh) {
    /* Nothing to say -> touch nothing. Beginning the overlay pass on an empty
       frame still mutates renderer state, and doing it unconditionally moved
       every golden frame in the suite. */
    if (!g_active && g_card_count == 0 && g_status[0] == '\0') return;

    renderer_begin_overlay(vw, vh);
    draw_tag_stack(vw, vh);
    if (!g_active) return;

    const float scale = 1.0f;
    const float big   = 2.0f;
    const float lh    = ui_text_line_height(scale);
    const float pad   = 14.0f;

    /* Height from the content, not a guess: header, the identity lines (the
       asset line only exists when the pick had one), the category row, the
       message field and the key hint. A fixed box left a third of the panel
       empty on entities, which reads as a half-loaded dialog. */
    int   info_lines = 3 + (g_edit.asset[0] ? 1 : 0);
    float bh = 34.0f + pad
             + info_lines * (lh + 3.0f) + 7.0f      /* identity block */
             + (lh + 8.0f) + 16.0f                  /* category row   */
             + (lh + 4.0f)                          /* "issue" label  */
             + (lh * 2.0f + 12.0f) + 18.0f          /* message field  */
             + lh + pad;                            /* hint + bottom  */
    float bw = vw * 0.56f;
    float x  = (float)(int)((vw - bw) * 0.5f);
    float y  = (float)(int)((vh - bh) * 0.5f);

    renderer_draw_screen_rect(vw, vh, x, y, bw, bh, 0.03f, 0.05f, 0.11f, 0.94f);
    renderer_draw_screen_rect(vw, vh, x, y, bw, 34.0f, 0.95f, 0.80f, 0.25f, 0.96f);
    ui_text_draw_centered(vw, vh, x + bw * 0.5f, y + 8.0f, big,
                          "QA CARD", 0.08f, 0.05f, 0.02f, 1.0f);

    float ly = y + 34.0f + pad;
    char line[256];

    snprintf(line, sizeof line, "%s %s", g_edit.kind,
             g_edit.name[0] ? g_edit.name : g_edit.tag);
    draw_line(vw, vh, x + pad, ly, scale, line, 0.95f, 0.95f, 1.0f); ly += lh + 3.0f;

    if (g_edit.asset[0]) {
        snprintf(line, sizeof line, "asset %s", g_edit.asset);
        draw_line(vw, vh, x + pad, ly, scale, line, 0.62f, 0.70f, 0.82f); ly += lh + 3.0f;
    }
    snprintf(line, sizeof line, "level %s   pos %.0f %.0f %.0f",
             g_edit.level, g_edit.pos[0], g_edit.pos[1], g_edit.pos[2]);
    draw_line(vw, vh, x + pad, ly, scale, line, 0.62f, 0.70f, 0.82f); ly += lh + 3.0f;

    snprintf(line, sizeof line, "player %.0f %.0f %.0f",
             g_edit.player[0], g_edit.player[1], g_edit.player[2]);
    draw_line(vw, vh, x + pad, ly, scale, line, 0.62f, 0.70f, 0.82f); ly += lh + 10.0f;

    /* Category row. */
    snprintf(line, sizeof line, "category   %s   %s",
             QA_CATEGORIES[g_cat_idx].code, QA_CATEGORIES[g_cat_idx].label);
    renderer_draw_screen_rect(vw, vh, x + pad - 4.0f, ly - 3.0f,
                              bw - pad * 2.0f + 8.0f, lh + 8.0f,
                              0.16f, 0.20f, 0.30f, 0.9f);
    draw_line(vw, vh, x + pad, ly, scale, line, 0.98f, 0.86f, 0.40f);
    ly += lh + 16.0f;

    draw_line(vw, vh, x + pad, ly, scale, "issue", 0.62f, 0.70f, 0.82f);
    ly += lh + 4.0f;

    /* Message field, with a blinking-free caret (a steady bar reads fine and
       keeps this deterministic for the headless harness). */
    renderer_draw_screen_rect(vw, vh, x + pad - 4.0f, ly - 4.0f,
                              bw - pad * 2.0f + 8.0f, lh * 2.0f + 12.0f,
                              0.10f, 0.13f, 0.20f, 0.95f);
    snprintf(line, sizeof line, "%s_", g_edit.message);
    draw_line(vw, vh, x + pad, ly, scale, line, 0.95f, 0.98f, 1.0f);
    ly += lh * 2.0f + 18.0f;

    draw_line(vw, vh, x + pad, ly, scale,
              "type to describe    Up Down category    Enter save    Esc cancel",
              0.52f, 0.58f, 0.68f);
}
