#ifndef PLAYER_ANIM_H
#define PLAYER_ANIM_H

#include "../engine/assets/ase_loader.h"

struct Entity;

typedef enum {
    PA_IDLE      = 0,
    PA_RUN       = 1,
    PA_LEFT      = 2,
    PA_RIGHT     = 3,
    PA_BACKPEDAL = 4,
    PA_JUMP      = 5,
    PA_FALL      = 6,
    PA_PICKUP    = 7,
    PA_SWING     = 8,   /* contextual: near a playground swing (3SWN) */
    PA_LADDER    = 9,   /* contextual: on a ladder (dormant — no Level-1 ladder) */
    PA_TALK      = 10,  /* cutscene/dialogue target pose */
    PA_BUTTONS   = 11,  /* cutscene control-panel pose */
    PA_SHRINK    = 12,  /* shrink-ray target pose */
    PA_DRIVE     = 13,  /* seated/riding pose for vehicles */
    PA_FENCE     = 14,  /* recovered player special-state animation */
    PA_SPLAT     = 15,  /* recovered Jimmy impact/splat reaction */
    PA_HIT       = 16,  /* recovered Jimmy hit reaction */
    PA_SCOOT     = 17,  /* riding the scooter -- Jimmy alias HISCOOT */
    PA_SCOOTSTOP = 18,  /* stopped on the scooter -- HISCOOTSTOP */
    PA_FLY       = 19,  /* jetpack flight -- Jimmy alias HIFLY */
    PA_SHOOT     = 20,  /* shrink-ray fire -- HISHOOT */
    PA_COUNT     = 21
} PlayerAnim;

typedef struct {
    const AseModel *model;
    int frame_a;
    int frame_b;
    float lerp;
} PlayerAnimSample;

/* Load all player locomotion/action clips and assign them the shared texture.
   Returns the number of poses successfully loaded (0 on total failure). */
int player_anim_init(unsigned int shared_texture_id);
void player_anim_destroy(void);
void player_anim_advance(PlayerAnim a, float dt);
void player_anim_bind_entity(struct Entity *e);
void player_anim_advance_entity(struct Entity *e, PlayerAnim a, float dt);
int  player_anim_start_entity_state(struct Entity *e, PlayerAnim a);
int  player_anim_is_special(PlayerAnim a);
int  player_anim_entity_special_active(const struct Entity *e);
void player_anim_react_to_damage(struct Entity *e, int is_down);

/* Returns the loaded model for the given anim, or the IDLE model if the
   requested one didn't load. NULL if nothing loaded. */
const AseModel *player_anim_model(PlayerAnim a);
PlayerAnimSample player_anim_sample(PlayerAnim a);
PlayerAnimSample player_anim_sample_entity(const struct Entity *e,
                                           PlayerAnim a);

#endif
