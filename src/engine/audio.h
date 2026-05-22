#ifndef AUDIO_H
#define AUDIO_H

int  audio_init(void);
int  audio_play(int sound_id);
void audio_stop_all(void);
void audio_destroy(void);

#endif
