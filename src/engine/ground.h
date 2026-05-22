#ifndef GROUND_H
#define GROUND_H

/* Tiled terrain heightfield centered at (center_x, world.ground_y, center_z),
   created once with ground_init(); draw with ground_draw(ground_y). The XZ
   footprint is 2*half_x by 2*half_z; y_amplitude is the measured peak-to-trough
   topography (Phase 12, from canon). texture_id of 0 falls back to a flat tint. */

int  ground_init(unsigned int texture_id, float half_x, float half_z,
                 float center_x, float center_z, float tile_repeat,
                 float y_amplitude);
void ground_draw(float ground_y);
void ground_destroy(void);

#endif
