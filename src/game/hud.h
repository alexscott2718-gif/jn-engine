#ifndef HUD_H
#define HUD_H

#include "gamestate.h"

/* 2D screen-space HUD overlay. Drawn AFTER the 3D scene each frame via the
   renderer's orthographic sprite/rect passes. Art is extracted from the
   original game's icon OMTs + png/ panels (see assets/hud/). */

/* Load HUD textures into the asset cache. Safe to call once after the GL
   context + asset cache are ready. */
void hud_init(void);

/* Draw the full HUD for the current frame. viewport_w/h are the window pixels. */
void hud_draw(int viewport_w, int viewport_h, const GameState *gs);

#endif
