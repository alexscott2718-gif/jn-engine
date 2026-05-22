#include "input.h"
#include <stdio.h>
#include <string.h>

static const Uint8 *keys_current = NULL;
static Uint8 keys_previous[SDL_NUM_SCANCODES];
static int num_keys = 0;
static int input_initialized = 0;

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
