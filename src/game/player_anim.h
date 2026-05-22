#ifndef PLAYER_ANIM_H
#define PLAYER_ANIM_H

#include "../engine/assets/ase_loader.h"

typedef enum {
    PA_IDLE   = 0,
    PA_RUN    = 1,
    PA_JUMP   = 2,
    PA_FALL   = 3,
    PA_PICKUP = 4,
    PA_COUNT  = 5
} PlayerAnim;

/* Load all five player poses and assign them the shared player texture.
   Returns the number of poses successfully loaded (0 on total failure). */
int player_anim_init(unsigned int shared_texture_id);
void player_anim_destroy(void);

/* Returns the loaded model for the given anim, or the IDLE model if the
   requested one didn't load. NULL if nothing loaded. */
const AseModel *player_anim_model(PlayerAnim a);

#endif
