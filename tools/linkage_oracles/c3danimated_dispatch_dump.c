/* Headless dumper for the C3DAnimated/event-animation-dispatch linkage oracle.
   Links the REAL, unmodified src/game/animated_dispatch.c and drives the
   recovered dispatch contract over stdin operations. Names contain no spaces.

     I                         init entity dispatch state
     K <base> <alt> <prefix>   key lead strings; "-" means empty
     M <mode>                  force shape_mode
     G <a> <b>                 force ready flags
     C <name> <frames> <ms>    create successful record with a persistent clip
     F <name>                  create failed-import record (clip NULL)
     S <name> <loop>           SetAnim3DByName
     X <id> <loop>             SelectAnim3DRecordIndex
     U <ms>                    update; converts ms -> float seconds
     P <0|1>                   pause/unpause
     H <name> <loop>           consuming hook: set_by_name(name, loop); H - clears
     L                         list records as R lines
     Q                         free and reset to a fresh entity

   After every operation, prints:

     ST <last_result> <alias|-> <current_index> <cur_rec_id|-1> <cur_rec_fc|-1>
        <cur_frame> <completed> <fires> <loop> <paused> <started>
        <tbbits> <clockbits>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/game/animated_dispatch.h"

static Entity g_entity;
static AnimatedClip g_clips[256];
static int g_clip_count;
static int g_last_result;
static char g_hook_name[128];
static int g_hook_loop;

static unsigned f32bits(float f) {
    unsigned u;
    memcpy(&u, &f, 4);
    return u;
}

static const char *undash(const char *s) {
    return (s && strcmp(s, "-") == 0) ? "" : s;
}

static void reset_entity(void) {
    animated_dispatch_free_entity(&g_entity);
    memset(&g_entity, 0, sizeof g_entity);
    g_clip_count = 0;
    g_hook_name[0] = '\0';
    g_hook_loop = 0;
}

static void consuming_hook(Entity *e) {
    if (g_hook_name[0])
        (void)animated_dispatch_set_by_name(e, g_hook_name, g_hook_loop);
}

static void print_records(void) {
    AnimatedDispatch *d = g_entity.anim_dispatch;
    if (!d) return;
    for (AnimatedRecord *r = d->records; r; r = r->next) {
        printf("R %d %d %d %s\n", r->seq_id, r->frame_count,
               r->clip ? 1 : 0, r->name[0] ? r->name : "-");
    }
}

static void print_state(void) {
    AnimatedDispatch *d = g_entity.anim_dispatch;
    const AnimatedRecord *cur = animated_dispatch_current_record(&g_entity);
    const char *alias = animated_dispatch_active_alias(&g_entity);
    if (!alias || !alias[0]) alias = "-";

    printf("ST %d %s %d %d %d %d %d %d %d %d %d %08x %08x\n",
           g_last_result,
           alias,
           d ? d->current_index : 0,
           cur ? cur->seq_id : -1,
           cur ? cur->frame_count : -1,
           d ? d->cur_frame : -1,
           animated_dispatch_completed(&g_entity),
           d ? d->anim_ended_fires : 0,
           d ? d->play_loop : 0,
           d ? d->paused : 0,
           d ? d->play_started : 0,
           f32bits(d ? d->tb_count_ms : 0.0f),
           f32bits(d ? d->clock : 0.0f));
}

int main(void) {
    char line[512];
    while (fgets(line, sizeof line, stdin)) {
        char op = 0;
        if (sscanf(line, " %c", &op) != 1) continue;

        if (op == 'I') {
            (void)animated_dispatch_init_entity(&g_entity);
        } else if (op == 'K') {
            char a[128], b[128], c[128];
            if (sscanf(line, " %c %127s %127s %127s", &op, a, b, c) == 4)
                animated_dispatch_set_key_strings(undash(a), undash(b), undash(c));
        } else if (op == 'M') {
            int mode;
            AnimatedDispatch *d = animated_dispatch_init_entity(&g_entity);
            if (d && sscanf(line, " %c %d", &op, &mode) == 2)
                d->shape_mode = mode;
        } else if (op == 'G') {
            int a, b;
            AnimatedDispatch *d = animated_dispatch_init_entity(&g_entity);
            if (d && sscanf(line, " %c %d %d", &op, &a, &b) == 3) {
                d->ready_a = !!a;
                d->ready_b = !!b;
            }
        } else if (op == 'C') {
            char name[128];
            int frames;
            float ms;
            if (sscanf(line, " %c %127s %d %f", &op, name, &frames, &ms) == 4 &&
                g_clip_count < (int)(sizeof g_clips / sizeof g_clips[0])) {
                g_clips[g_clip_count].frame_count = frames;
                g_clips[g_clip_count].ms_per_frame = ms;
                (void)animated_dispatch_create_record(&g_entity, name,
                                                      &g_clips[g_clip_count]);
                g_clip_count++;
            }
        } else if (op == 'F') {
            char name[128];
            if (sscanf(line, " %c %127s", &op, name) == 2)
                (void)animated_dispatch_create_record(&g_entity, name, NULL);
        } else if (op == 'S') {
            char name[128];
            int loop;
            if (sscanf(line, " %c %127s %d", &op, name, &loop) == 3)
                g_last_result = animated_dispatch_set_by_name(&g_entity, name, loop);
        } else if (op == 'X') {
            int id, loop;
            if (sscanf(line, " %c %d %d", &op, &id, &loop) == 3)
                g_last_result = animated_dispatch_select_index(&g_entity, id, loop);
        } else if (op == 'U') {
            float ms;
            if (sscanf(line, " %c %f", &op, &ms) == 2)
                animated_dispatch_update(&g_entity, ms / 1000.0f);
        } else if (op == 'P') {
            int p;
            if (sscanf(line, " %c %d", &op, &p) == 2)
                animated_dispatch_set_paused(&g_entity, p);
        } else if (op == 'H') {
            char name[128];
            int loop = 0;
            int n = sscanf(line, " %c %127s %d", &op, name, &loop);
            if (n >= 2 && strcmp(name, "-") == 0) {
                g_hook_name[0] = '\0';
                animated_dispatch_set_anim_ended_hook(&g_entity, NULL);
            } else if (n == 3) {
                snprintf(g_hook_name, sizeof g_hook_name, "%s", name);
                g_hook_loop = loop;
                animated_dispatch_set_anim_ended_hook(&g_entity, consuming_hook);
            }
        } else if (op == 'L') {
            print_records();
        } else if (op == 'Q') {
            reset_entity();
        }

        print_state();
    }

    reset_entity();
    return 0;
}
