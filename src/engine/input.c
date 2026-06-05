#include "input.h"
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

static const Uint8 *keys_current = NULL;
static Uint8 keys_previous[SDL_NUM_SCANCODES];
static int num_keys = 0;
static int input_initialized = 0;

/* Virtual touch input state. */
static float g_vmove_x = 0.0f, g_vmove_y = 0.0f;
static float g_vfly_y = 0.0f;
static int   g_vjump_pending = 0;
static int   g_noclip_enabled = 0;
static int   g_turbo_enabled  = 0;

int input_init(void) {
    keys_current = SDL_GetKeyboardState(&num_keys);
    if (!keys_current) {
        fprintf(stderr, "SDL_GetKeyboardState failed\n");
        return 0;
    }
    memset(keys_previous, 0, sizeof(keys_previous));
    input_initialized = 1;
    printf("Input subsystem initialized\n");
    return 1;
}

int input_is_down(SDL_Scancode code) {
    if (!input_initialized || code >= SDL_NUM_SCANCODES) return 0;
    return keys_current[code];
}

int input_just_pressed(SDL_Scancode code) {
    if (!input_initialized || code >= SDL_NUM_SCANCODES) return 0;
    return keys_current[code] && !keys_previous[code];
}

void input_update(void) {
    if (!input_initialized) return;
    memcpy(keys_previous, keys_current, num_keys);
    keys_current = SDL_GetKeyboardState(NULL);
}

void input_destroy(void) {
    if (!input_initialized) return;
    input_initialized = 0;
    printf("Input subsystem destroyed\n");
}

/* --- Virtual touch input (exported to JS via EMSCRIPTEN_KEEPALIVE) --- */

EMSCRIPTEN_KEEPALIVE
void input_set_virtual_move(float x, float y) {
    /* Clamp to the unit disc so diagonal stick throws don't exceed full speed. */
    float m2 = x * x + y * y;
    if (m2 > 1.0f) { float inv = 1.0f / SDL_sqrtf(m2); x *= inv; y *= inv; }
    g_vmove_x = x;
    g_vmove_y = y;
}

void input_get_virtual_move(float *x, float *y) {
    if (x) *x = g_vmove_x;
    if (y) *y = g_vmove_y;
}

EMSCRIPTEN_KEEPALIVE
void input_set_virtual_fly(float y) {
    if (y > 1.0f) y = 1.0f;
    if (y < -1.0f) y = -1.0f;
    g_vfly_y = y;
}

float input_get_virtual_fly(void) {
    return g_vfly_y;
}

EMSCRIPTEN_KEEPALIVE
void input_press_virtual_jump(void) {
    g_vjump_pending = 1;
}

int input_virtual_take_jump(void) {
    int j = g_vjump_pending;
    g_vjump_pending = 0;
    return j;
}

EMSCRIPTEN_KEEPALIVE
void input_set_noclip(int enabled) {
    g_noclip_enabled = enabled ? 1 : 0;
    printf("[noclip] %s\n", g_noclip_enabled ? "enabled" : "disabled");
}

EMSCRIPTEN_KEEPALIVE
int input_toggle_noclip(void) {
    input_set_noclip(!g_noclip_enabled);
    return g_noclip_enabled;
}

EMSCRIPTEN_KEEPALIVE
int input_noclip_enabled(void) {
    return g_noclip_enabled;
}

/* Turbo: a sticky speed boost for fast level exploration (separate from the
   held SHIFT run, with which it stacks). Exported so the web UI can toggle it. */
EMSCRIPTEN_KEEPALIVE
void input_set_turbo(int enabled) {
    g_turbo_enabled = enabled ? 1 : 0;
    printf("[turbo] %s\n", g_turbo_enabled ? "enabled" : "disabled");
}

EMSCRIPTEN_KEEPALIVE
int input_toggle_turbo(void) {
    input_set_turbo(!g_turbo_enabled);
    return g_turbo_enabled;
}

EMSCRIPTEN_KEEPALIVE
int input_turbo_enabled(void) {
    return g_turbo_enabled;
}
