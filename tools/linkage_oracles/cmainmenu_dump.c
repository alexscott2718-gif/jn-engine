/* Headless dumper for the CMainMenu linkage oracle
   (docs/linked_parity_plan.md L3). Compiles the real, unmodified
   src/game/menu.c and walks its routing table through the real public API:
   for each index i, menu_open() + i synthetic Down presses (menu_input) +
   a synthetic confirm (menu_take_confirm), dumping the routed
   (level, is_newgame) pair. A final pass presses Down once per item from a
   fresh open and confirms, proving the selection wraps back to item 0 --
   which pins the item count without reaching into menu.c's statics.

   Stubs input_just_pressed (scripted key queue; menu.c's only input read)
   and the two renderer overlay calls (menu_draw is never invoked, but the
   linker needs the symbols). Everything else in menu.c runs as shipped.

   Build: cc -I src/engine -I <SDL2 include> tools/linkage_oracles/cmainmenu_dump.c \
              src/game/menu.c -o <out>
   Run:   <out>   -> M|<idx>|<level-or-(quit)>|<is_newgame> rows + W|... wrap row */
#include <stdio.h>
#include <SDL.h>

#include "../../src/game/menu.h"
#include "../../src/engine/renderer.h"

/* ---- input stub: scripted Down presses + a one-shot confirm ------------- */
static int g_down_queue = 0;
static int g_confirm = 0;

int input_just_pressed(SDL_Scancode code) {
    if (code == SDL_SCANCODE_DOWN && g_down_queue > 0) {
        g_down_queue--;
        return 1;
    }
    if (code == SDL_SCANCODE_RETURN && g_confirm) {
        g_confirm = 0;
        return 1;
    }
    return 0;
}

/* ---- renderer stubs (menu_draw is not exercised headless) --------------- */
void renderer_begin_overlay(int viewport_w, int viewport_h) {
    (void)viewport_w; (void)viewport_h;
}
void renderer_draw_screen_rect(int viewport_w, int viewport_h,
                               float x, float y, float w, float h,
                               float r, float g, float b, float a) {
    (void)viewport_w; (void)viewport_h; (void)x; (void)y; (void)w; (void)h;
    (void)r; (void)g; (void)b; (void)a;
}

static int route_at(int idx, const char **level, int *is_newgame) {
    menu_open();
    for (int s = 0; s < idx; s++) {
        g_down_queue = 1;
        menu_input();
    }
    g_confirm = 1;
    return menu_take_confirm(level, is_newgame);
}

int main(void) {
    const char *level;
    int is_newgame;

    /* Probe generously past the expected count; wrapped indices repeat the
       head of the table, which the oracle detects to pin the count. */
    for (int i = 0; i < 16; i++) {
        if (!route_at(i, &level, &is_newgame)) {
            printf("M|%d|CONFIRM_FAILED|0\n", i);
            continue;
        }
        printf("M|%d|%s|%d\n", i, level ? level : "(quit)", is_newgame);
    }
    return 0;
}
