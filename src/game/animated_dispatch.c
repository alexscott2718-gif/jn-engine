#include "animated_dispatch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static char g_key_base[80] = "";
static char g_key_alt[80] = "";
static char g_key_prefix[80] = "";

static void copy_bounded(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) src = "";
    snprintf(dst, dst_size, "%s", src);
}

static const char *mode_lead(const AnimatedDispatch *d) {
    if (!d) return g_key_prefix;
    if (d->shape_mode == 0) return g_key_base;
    if (d->shape_mode == 1) return g_key_alt;
    return g_key_prefix;
}

void animated_dispatch_set_key_strings(const char *suffix_base,
                                       const char *suffix_alt,
                                       const char *prefix) {
    copy_bounded(g_key_base, sizeof g_key_base, suffix_base);
    copy_bounded(g_key_alt, sizeof g_key_alt, suffix_alt);
    copy_bounded(g_key_prefix, sizeof g_key_prefix, prefix);
}

AnimatedDispatch *animated_dispatch_init_entity(Entity *e) {
    if (!e) return NULL;
    if (!e->anim_dispatch) {
        e->anim_dispatch = calloc(1, sizeof(*e->anim_dispatch));
        if (!e->anim_dispatch) return NULL;
        e->anim_dispatch->cur_frame = -1;
    }

    AnimatedDispatch *d = e->anim_dispatch;
    if (d->ready_a && d->ready_b) return d;

    d->shape_mode = 0;
    d->ready_a = 1;
    d->ready_b = 1;
    return d;
}

void animated_dispatch_free_entity(Entity *e) {
    if (!e || !e->anim_dispatch) return;
    AnimatedDispatch *d = e->anim_dispatch;
    AnimatedRecord *r = d->records;
    while (r) {
        AnimatedRecord *n = r->next;
        free(r);
        r = n;
    }
    free(d);
    e->anim_dispatch = NULL;
}

AnimatedRecord *animated_dispatch_create_record(Entity *e, const char *name,
                                                const AnimatedClip *clip) {
    AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    if (!d) d = animated_dispatch_init_entity(e);
    if (!d) return NULL;

    AnimatedRecord *r = calloc(1, sizeof(*r));
    if (!r) return NULL;

    copy_bounded(r->name, sizeof r->name, g_key_prefix);
    r->seq_id = d->record_count;
    r->frame_count = clip ? clip->frame_count : 0;
    r->clip = clip;

    if (clip) {
        copy_bounded(r->name, sizeof r->name, name);
        d->record_count++;
    }

    if (!d->records) {
        d->records = r;
    } else {
        AnimatedRecord *tail = d->records;
        while (tail->next) tail = tail->next;
        tail->next = r;
    }
    return r;
}

const AnimatedRecord *animated_dispatch_find_record(const Entity *e,
                                                    const char *name) {
    const AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    if (!d || !d->ready_a || !d->ready_b) return NULL;
    if (!name) name = "";
    for (const AnimatedRecord *r = d->records; r; r = r->next) {
        if (strcasecmp(r->name, name) == 0) return r;
    }
    return NULL;
}

const AnimatedRecord *animated_dispatch_current_record(const Entity *e) {
    const AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    if (!d || !d->ready_a || !d->ready_b) return NULL;
    for (const AnimatedRecord *r = d->records; r; r = r->next) {
        if (r->seq_id == d->current_index) return r;
    }
    return NULL;
}

const AnimatedRecord *animated_dispatch_selected_record(const Entity *e) {
    const AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    return d ? d->current : NULL;
}

const AnimatedClip *animated_dispatch_current_clip(const Entity *e) {
    const AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    return d ? d->current_clip : NULL;
}

AnimatedDispatchResult animated_dispatch_select_index(Entity *e, int seq_id,
                                                      int loop) {
    AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    if (!d) d = animated_dispatch_init_entity(e);
    if (!d || !d->ready_b || seq_id < 0)
        return ANIMATED_DISPATCH_UNCHANGED;

    d->current_index = seq_id;
    d->play_loop = !!loop;
    d->play_started = 0;

    if (d->current && d->current->clip)
        d->current->frame_count = d->current->clip->frame_count;

    return ANIMATED_DISPATCH_SELECTED;
}

static void build_lookup_key(char key[80], const AnimatedDispatch *d,
                             const char *name) {
    snprintf(key, 80, "%s%s", mode_lead(d), name ? name : "");
}

AnimatedDispatchResult animated_dispatch_set_by_name(Entity *e,
                                                     const char *name,
                                                     int loop) {
    AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    if (!d) d = animated_dispatch_init_entity(e);
    if (!d || !d->ready_a || !d->ready_b)
        return ANIMATED_DISPATCH_UNCHANGED;

    if (!name) name = "";
    d->clock = 0.0f;

    if (d->shape_mode == 0 || d->shape_mode == 1)
        copy_bounded(d->current_name, sizeof d->current_name, name);

    char key[80];
    build_lookup_key(key, d, name);
    AnimatedRecord *rec = (AnimatedRecord *)animated_dispatch_find_record(e, key);
    if (!rec) return ANIMATED_DISPATCH_NOT_FOUND;

    d->current = rec;
    copy_bounded(d->current_name, sizeof d->current_name, name);
    (void)animated_dispatch_select_index(e, rec->seq_id, loop);

    if (d->current_clip != rec->clip) {
        d->cur_frame = -1;
        d->play_started = 0;
    }
    d->current_clip = rec->clip;

    return ANIMATED_DISPATCH_SELECTED;
}

static int advance_frame(AnimatedDispatch *d) {
    const AnimatedClip *clip = d ? d->current_clip : NULL;
    int frames = clip ? clip->frame_count : 0;
    if (!d || !clip || frames <= 0) return 1;

    if (d->cur_frame < 0)
        d->cur_frame = 0;

    if (frames == 1) return 1;
    if (d->cur_frame < frames - 1) {
        d->cur_frame++;
        return 0;
    }
    if (d->play_loop) {
        d->cur_frame = 0;
        return 0;
    }
    return 1;
}

static void update_logic(AnimatedDispatch *d, float elapsed_ms) {
    if (!d || d->paused || !d->current_clip) return;
    float ms_per_frame = d->current_clip->ms_per_frame;
    if (ms_per_frame == 0.0f) return;

    if (!d->play_started) {
        d->play_started = 1;
        d->tb_count_ms = ms_per_frame;
        return;
    }

    float mel = elapsed_ms;
    for (;;) {
        if (mel >= d->tb_count_ms) {
            mel -= d->tb_count_ms;
            if (advance_frame(d)) {
                d->tb_count_ms = ms_per_frame;
                break;
            }
            d->tb_count_ms = ms_per_frame;
            if (d->tb_count_ms == 0.0f) break;
        } else {
            d->tb_count_ms -= mel;
            break;
        }
    }
}

int animated_dispatch_completed(const Entity *e) {
    const AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    if (!d || !d->ready_a || !d->ready_b || d->play_loop || !d->current)
        return 0;
    int frame_pos = d->cur_frame < 0 ? 0 : d->cur_frame;
    return frame_pos >= d->current->frame_count - 1;
}

void animated_dispatch_update(Entity *e, float dt_seconds) {
    AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    if (!d || !d->ready_a || !d->ready_b) return;

    float elapsed_ms = dt_seconds * 1000.0f;
    update_logic(d, elapsed_ms);
    d->clock += dt_seconds;

    if (animated_dispatch_completed(e)) {
        d->anim_ended_fires++;
        if (d->on_anim_ended)
            d->on_anim_ended(e);
    }
}

void animated_dispatch_set_paused(Entity *e, int paused) {
    AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    if (!d) d = animated_dispatch_init_entity(e);
    if (!d) return;

    paused = !!paused;
    if (d->paused == paused) return;
    d->paused = paused;
    if (!paused)
        d->play_started = 0;
}

void animated_dispatch_set_anim_ended_hook(Entity *e,
                                           void (*hook)(Entity *e)) {
    AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    if (!d) d = animated_dispatch_init_entity(e);
    if (d) d->on_anim_ended = hook;
}

const char *animated_dispatch_active_alias(const Entity *e) {
    const AnimatedDispatch *d = e ? e->anim_dispatch : NULL;
    if (!d || !d->current_name[0]) return NULL;
    return d->current_name;
}
