#ifndef INPUT_H
#define INPUT_H

#include <SDL.h>

int  input_init(void);
int  input_is_down(SDL_Scancode code);
int  input_just_pressed(SDL_Scancode code);
void input_update(void);
void input_destroy(void);

#endif
