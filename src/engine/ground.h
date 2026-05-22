#ifndef GROUND_H
#define GROUND_H

/* Large textured quad at world.ground_y. Created once with ground_init();
   draw with ground_draw(ground_y). texture_id of 0 falls back to a flat
   neutral color. */

int  ground_init(unsigned int texture_id, float half_size, float center_x, float center_z, float tile_repeat);
void ground_draw(float ground_y);
void ground_destroy(void);

#endif
