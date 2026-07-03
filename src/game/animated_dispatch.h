#ifndef GAME_ANIMATED_DISPATCH_H
#define GAME_ANIMATED_DISPATCH_H

#include "../engine/world.h"

/* Native stand-in for one imported OMedia A3dm definition. `ms_per_frame`
   mirrors OMediaAnimFrame::getmillisecperframe() and stays float. The current
   runtime imports Jimmy/cutscene animation metadata from the original ASE
   streams, which become one OMedia sequence after import. */
typedef struct AnimatedClip {
    int frame_count;
    float ms_per_frame;
    int sequence_count;
    int sequence_index;
    float source_framespeed;
} AnimatedClip;

void animated_dispatch_clip_from_ase(AnimatedClip *out, const AseModel *model);

typedef struct AnimatedRecord {
    char name[64];
    struct AnimatedRecord *next;   /* original record +0x40 */
    int seq_id;                    /* original record +0x44 */
    int frame_count;               /* original record +0x48 */
    const AnimatedClip *clip;      /* native stand-in for DB object/def +0x4c */
} AnimatedRecord;

typedef enum AnimatedDispatchResult {
    ANIMATED_DISPATCH_NOT_FOUND = 0,
    ANIMATED_DISPATCH_SELECTED = 1,
    ANIMATED_DISPATCH_UNCHANGED = 2
} AnimatedDispatchResult;

typedef struct AnimatedDispatch {
    AnimatedRecord *records;
    int record_count;

    int ready_a;                   /* adjusted +0x634 */
    int ready_b;                   /* adjusted +0x635 */
    int shape_mode;                /* adjusted +0x580 */
    float clock;                   /* adjusted +0x584 */
    int current_index;             /* adjusted +0x588 */
    const AnimatedClip *current_clip; /* adjusted +0x58c */
    AnimatedRecord *current;       /* adjusted +0x62c */
    char current_name[64];         /* +0x17a region */

    int play_loop;                 /* OMediaAnim adjusted +0xad */
    int paused;                    /* SetAnim3DPaused mirror / pause_count */
    int play_started;              /* OMediaAnim adjusted +0xaf */
    float tb_count_ms;             /* OMediaAnim adjusted +0xb4 */
    int cur_frame;                 /* -1 means no current OMedia frame */

    void (*on_anim_ended)(Entity *e); /* vtable-4 slot 65; NULL = base no-op */
    int anim_ended_fires;          /* oracle-visible hook fire count */
} AnimatedDispatch;

typedef struct AnimatedDispatchSample {
    int frame_a;
    int frame_b;
    float lerp;
} AnimatedDispatchSample;

void animated_dispatch_set_key_strings(const char *suffix_base,
                                       const char *suffix_alt,
                                       const char *prefix);

/* Lazy per-entity state. No runtime path allocates this until future wiring
   explicitly calls init/create/set; world_destroy only frees it if present. */
AnimatedDispatch *animated_dispatch_init_entity(Entity *e);
void animated_dispatch_free_entity(Entity *e);

AnimatedRecord *animated_dispatch_create_record(Entity *e, const char *name,
                                                const AnimatedClip *clip);
const AnimatedRecord *animated_dispatch_find_record(const Entity *e,
                                                    const char *name);
const AnimatedRecord *animated_dispatch_current_record(const Entity *e);
const AnimatedRecord *animated_dispatch_selected_record(const Entity *e);
const AnimatedClip *animated_dispatch_current_clip(const Entity *e);

AnimatedDispatchResult animated_dispatch_select_index(Entity *e, int seq_id,
                                                      int loop);
AnimatedDispatchResult animated_dispatch_set_by_name(Entity *e,
                                                     const char *name,
                                                     int loop);
void animated_dispatch_update(Entity *e, float dt_seconds);
void animated_dispatch_set_paused(Entity *e, int paused);
void animated_dispatch_set_anim_ended_hook(Entity *e,
                                           void (*hook)(Entity *e));

const char *animated_dispatch_active_alias(const Entity *e);
int animated_dispatch_completed(const Entity *e);
int animated_dispatch_sample(const Entity *e, AnimatedDispatchSample *out);

#endif
