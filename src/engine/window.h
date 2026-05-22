#ifndef WINDOW_H
#define WINDOW_H

#include <SDL.h>

typedef struct {
    SDL_Window   *sdl_win;
    SDL_GLContext gl_ctx;
    int           width;
    int           height;
    int           should_quit;
} Window;

int  window_init(Window *w, const char *title, int width, int height);
void window_poll_events(Window *w);
void window_swap(Window *w);
void window_destroy(Window *w);

#endif
