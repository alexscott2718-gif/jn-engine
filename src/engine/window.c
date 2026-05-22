#include "window.h"
#include "glad.h"
#include <stdio.h>

int window_init(Window *w, const char *title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    w->sdl_win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
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

    SDL_GL_SetSwapInterval(1);

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
