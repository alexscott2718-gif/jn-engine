#include "audio.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_SOUNDS 4

static Mix_Chunk *sounds[NUM_SOUNDS];
static int audio_initialized = 0;

const char *sound_paths[NUM_SOUNDS] = {
    "assets/parsed/sounds/sounds_audio/0000_Windows_Logon_Sound.wav",
    "assets/parsed/sounds/sounds_audio/0001_save.wav",
    "assets/parsed/sounds/sounds_audio/0002_bark1.wav",
    "assets/parsed/sounds/sounds_audio/0003_detect.wav"
};

const char *sound_names[NUM_SOUNDS] = {
    "Windows Logon",
    "save",
    "bark1",
    "detect"
};

int audio_init(void) {
    /* Audio is optional - if it fails, continue without it */
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Audio subsystem unavailable: %s (continuing without audio)\n", SDL_GetError());
        audio_initialized = 0;
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "Mix_OpenAudio failed: %s (continuing without audio)\n", Mix_GetError());
        audio_initialized = 0;
        return 1;
    }

    for (int i = 0; i < NUM_SOUNDS; i++) {
        sounds[i] = Mix_LoadWAV(sound_paths[i]);
        if (!sounds[i]) {
            fprintf(stderr, "Failed to load sound %d (%s): %s\n",
                    i, sound_names[i], Mix_GetError());
        } else {
            printf("Loaded sound %d: %s (%d bytes)\n",
                   i, sound_names[i], sounds[i]->alen);
        }
    }

    audio_initialized = 1;
    printf("Audio subsystem initialized (44100 Hz, stereo, %d sounds)\n", NUM_SOUNDS);
    return 1;
}

int audio_play(int sound_id) {
    if (!audio_initialized) {
        fprintf(stderr, "Audio not initialized\n");
        return -1;
    }
    if (sound_id < 0 || sound_id >= NUM_SOUNDS) {
        fprintf(stderr, "Invalid sound ID: %d\n", sound_id);
        return -1;
    }
    if (!sounds[sound_id]) {
        fprintf(stderr, "Sound %d not loaded\n", sound_id);
        return -1;
    }

    int channel = Mix_PlayChannel(-1, sounds[sound_id], 0);
    if (channel < 0) {
        fprintf(stderr, "Mix_PlayChannel failed: %s\n", Mix_GetError());
        return -1;
    }
    printf("Playing sound %d (%s) on channel %d\n", sound_id, sound_names[sound_id], channel);
    return channel;
}

void audio_stop_all(void) {
    if (!audio_initialized) return;
    Mix_HaltChannel(-1);
    printf("Stopped all audio\n");
}

void audio_destroy(void) {
    if (!audio_initialized) return;

    Mix_HaltChannel(-1);
    for (int i = 0; i < NUM_SOUNDS; i++) {
        if (sounds[i]) {
            Mix_FreeChunk(sounds[i]);
            sounds[i] = NULL;
        }
    }
    Mix_CloseAudio();
    audio_initialized = 0;
    printf("Audio subsystem destroyed\n");
}
