#ifndef AUDIO_H
#define AUDIO_H

int  audio_init(void);
int  audio_play(int sound_id);
void audio_stop_all(void);
void audio_destroy(void);

/* --- Extended mixer surface used by the ported gameplay audio classes
   (C3DSoundEffect / C3DMusicTrigger). The shipped level sound/music banks are
   not yet wired (SoundIndex -> bank mapping is an open decomp question), so
   callers index the small placeholder WAV bank below modulo audio_sound_count().
   See src/game/behaviors/behavior_soundfx.c + behavior_music.c. */

/* Number of loaded placeholder sounds (0 if audio is unavailable). */
int  audio_sound_count(void);

/* Play sound_id with `loops` repeats (-1 = loop forever, 0 = play once) at
   `gain` (0..128). Returns the mixer channel, or -1 on failure. */
int  audio_play_ex(int sound_id, int loops, int gain);

/* Adjust / stop a previously returned channel. No-ops on invalid channels. */
void audio_channel_gain(int channel, int gain);
void audio_channel_halt(int channel);

/* Switch the background-music selection to `track`. The music bank is not yet
   loaded, so this records the active track and logs the switch (no playback);
   it exists so C3DMusicTrigger's control logic is faithfully wired. Returns the
   previously-active track. */
int  audio_set_music(int track);

#endif
