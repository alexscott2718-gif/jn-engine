/* behavior_soundfx.c — native port of C3DSoundEffect (FourCC 3SOU).
 *
 * From docs/decomp/C3DSoundEffect.md + shipped .gam data (9 instances in
 * Level 1): a placed positional sound emitter. While the player is within
 * `Radius` it either keeps a looping ambient bed alive (`IsAmbient`) or fires a
 * one-shot up to `TimesToTrigger` times on entry. `RequiredLevel` is a progress
 * gate.
 *
 * Native realisation: faithful proximity/state control. The shipped level sound
 * bank is not yet wired (SoundIndex -> bank mapping is an open decomp question,
 * see the spec), so SoundIndex resolves into the small placeholder WAV bank via
 * `SoundIndex % audio_sound_count()`. Gain falls off linearly across Radius.
 * RequiredLevel is read but not enforced (no global level-progress counter yet).
 *
 * State packing (Entity scratch):
 *   ambient   -> user_flag = active mixer channel + 1 (0 = not playing)
 *   one-shot  -> user_flag = triggers remaining; user_float = inside-edge (0/1)
 */
#include "behaviors.h"
#include "../../engine/audio.h"
#include <stddef.h>
#include <math.h>

static void soundfx_on_spawn(Entity *e, World *w) {
    (void)w;
    if (gam_prop_i(e, "IsAmbient", 0)) {
        e->user_flag = 0;          /* no ambient channel playing yet */
    } else {
        e->user_flag = gam_prop_i(e, "TimesToTrigger", 1); /* triggers remaining */
    }
    e->user_float = 0.0f;          /* outside */
}

static void soundfx_on_update(Entity *e, World *w, float dt) {
    (void)w; (void)dt;
    if (!g_player) return;
    int nsnd = audio_sound_count();
    if (nsnd <= 0) return;         /* audio unavailable */

    float radius = gam_prop_f(e, "Radius", 200.0f);
    if (radius <= 0.0f) return;
    int snd = gam_prop_i(e, "SoundIndex", 0) % nsnd;
    if (snd < 0) snd += nsnd;

    float dx = g_player->x - e->x;
    float dy = g_player->y - e->y;
    float dz = g_player->z - e->z;
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
    int inside = dist < radius;
    /* Linear proximity falloff to full volume at the emitter. */
    int gain = inside ? (int)(128.0f * (1.0f - dist / radius)) : 0;

    if (gam_prop_i(e, "IsAmbient", 0)) {
        if (inside) {
            if (e->user_flag == 0) {
                int ch = audio_play_ex(snd, -1, gain);
                e->user_flag = (ch >= 0) ? ch + 1 : 0;
            } else {
                audio_channel_gain(e->user_flag - 1, gain);
            }
        } else if (e->user_flag != 0) {
            audio_channel_halt(e->user_flag - 1);
            e->user_flag = 0;
        }
    } else {
        int was_inside = e->user_float > 0.5f;
        if (inside && !was_inside && e->user_flag > 0) {
            audio_play_ex(snd, 0, gain);
            e->user_flag--;        /* consume one of TimesToTrigger */
        }
        e->user_float = inside ? 1.0f : 0.0f;
    }
}

const EntityVTable vt_soundfx = {
    .on_spawn   = soundfx_on_spawn,
    .on_update  = soundfx_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
