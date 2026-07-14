#include "window.h"
#include "glad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

int window_init(Window *w, const char *title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    /* Under Emscripten, SDL_WINDOW_RESIZABLE makes the runtime auto-sync the
       canvas drawing buffer to the canvas CSS size every frame. If the page
       layout transiently narrows the canvas (initial flexbox layout, a
       devicePixelRatio quirk, or any DOM event during preload), the drawing
       buffer can collapse to 1px and never recover. Drop RESIZABLE for the
       web build and pin the drawing buffer explicitly below. The native
       build keeps RESIZABLE because we genuinely want window resize there. */
    Uint32 win_flags = SDL_WINDOW_OPENGL;
#ifndef __EMSCRIPTEN__
    win_flags |= SDL_WINDOW_RESIZABLE;
    const char *headless = getenv("JN_HEADLESS");
    if (headless && headless[0] && strcmp(headless, "0") != 0)
        win_flags |= SDL_WINDOW_HIDDEN;
#endif
    w->sdl_win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        win_flags
    );
    if (!w->sdl_win) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    w->gl_ctx = SDL_GL_CreateContext(w->sdl_win);
    if (!w->gl_ctx) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(w->sdl_win);
        SDL_Quit();
        return 0;
    }

    const char *headless_swap = getenv("JN_HEADLESS");
    SDL_GL_SetSwapInterval(headless_swap && headless_swap[0] && strcmp(headless_swap, "0") != 0
                           ? 0 : 1);

    if (!glad_load_gl()) {
        fprintf(stderr, "Failed to load GL functions\n");
        SDL_GL_DeleteContext(w->gl_ctx);
        SDL_DestroyWindow(w->sdl_win);
        SDL_Quit();
        return 0;
    }

    w->width      = width;
    w->height     = height;
    w->should_quit = 0;

#ifdef __EMSCRIPTEN__
    /* Pin the canvas drawing buffer to the requested SDL window size so the
       initial render targets the full 1280x720 (or whatever caller asked
       for). CSS can still scale the visible canvas to fit the viewport. */
    emscripten_set_canvas_element_size("#canvas", width, height);
#endif

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printf("OpenGL: %s\n", (const char*)SDL_GL_GetProcAddress == 0 ? "n/a" : "ok");
    printf("Window: %dx%d\n", width, height);
    return 1;
}

void window_poll_events(Window *w) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT)
            w->should_quit = 1;
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
            w->should_quit = 1;
        if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_RESIZED) {
            w->width  = ev.window.data1;
            w->height = ev.window.data2;
            glViewport(0, 0, w->width, w->height);
        }
    }
}

void window_swap(Window *w) {
    SDL_GL_SwapWindow(w->sdl_win);
}

void window_destroy(Window *w) {
    if (w->gl_ctx)  SDL_GL_DeleteContext(w->gl_ctx);
    if (w->sdl_win) SDL_DestroyWindow(w->sdl_win);
    SDL_Quit();
}
