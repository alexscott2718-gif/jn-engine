#ifndef AUDIO_H
#define AUDIO_H

int  audio_init(void);
int  audio_play(int sound_id);
void audio_stop_all(void);
void audio_destroy(void);

/* --- Extended mixer surface used by the ported gameplay audio classes.
   The original OMT audio containers store WAVs in RIFF byte order plus a
   trailing Wave handle table. GAM SoundIndex/MusicIndex values address that
   handle table, not the RIFF extraction order. Runtime resolution therefore
   goes through assets/parsed/<db>/<db>_audio_handles.tsv, generated from the
   shipped .omt files. */

/* Number of loaded placeholder sounds (0 if audio is unavailable). */
int  audio_sound_count(void);

/* Play sound_id with `loops` repeats (-1 = loop forever, 0 = play once) at
   `gain` (0..128). Returns the mixer channel, or -1 on failure. */
int  audio_play_ex(int sound_id, int loops, int gain);

/* Play an authored OMT audio handle from a named database. `db` may include
   ".omt"; NULL/"none" defaults to the global SoundEffects.omt bank for
   gameplay SFX. */
int  audio_play_db(const char *db, int handle, int loops, int gain);

/* Adjust / stop a previously returned channel. No-ops on invalid channels. */
void audio_channel_gain(int channel, int gain);
void audio_channel_halt(int channel);

/* Switch background music by authored OMT handle. For music banks with a
   single table entry, a missing authored handle falls back to that sole entry;
   this matches Level5a's musicrocketmaze start point data. */
int  audio_set_music_db(const char *db, int handle);

/* Legacy keyboard/demo helper: plays from assets/parsed/sounds. */
int  audio_set_music(int track);

#endif
