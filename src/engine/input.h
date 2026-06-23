#ifndef INPUT_H
#define INPUT_H

#include <SDL.h>

int  input_init(void);
int  input_is_down(SDL_Scancode code);
int  input_just_pressed(SDL_Scancode code);
void input_update(void);
void input_destroy(void);

/* Virtual analog input, driven by on-screen touch controls (web). The move
   axis is camera-relative screen space: x = +right, y = +forward (up on the
   joystick), each in [-1, 1]; magnitude scales speed. jump is edge-triggered
   and consumed by input_virtual_take_jump(). On native these stay zero unless
   set, so keyboard remains the only input. */
void  input_set_virtual_move(float x, float y);  /* JS: each frame from stick */
void  input_get_virtual_move(float *x, float *y);
void  input_set_virtual_fly(float y);            /* JS: held noclip up/down */
float input_get_virtual_fly(void);
void  input_press_virtual_jump(void);            /* JS: jump button tap */
int   input_virtual_take_jump(void);             /* behavior: consume the tap */
void  input_press_virtual_use(void);             /* JS: use-active-tool button */
int   input_virtual_take_use(void);              /* behavior: consume the tap */
void  input_press_virtual_board(void);           /* JS: enter/leave vehicle button */
int   input_virtual_board_peek(void);            /* behavior: read without consuming */
void  input_virtual_board_consume(void);         /* behavior/main: clear the tap */

/* Noclip/fly mode. Exported for web UI; native toggles it from player input. */
void  input_set_noclip(int enabled);
int   input_toggle_noclip(void);
int   input_noclip_enabled(void);

/* Turbo: sticky speed boost for fast level exploration (stacks with SHIFT). */
void  input_set_turbo(int enabled);
int   input_toggle_turbo(void);
int   input_turbo_enabled(void);

#endif
