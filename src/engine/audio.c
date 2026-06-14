#include "audio.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define NUM_SOUNDS 4

static Mix_Chunk *sounds[NUM_SOUNDS];
static int audio_initialized = 0;

#define AUDIO_CACHE_MAX 128
typedef struct {
    char path[256];
    Mix_Chunk *chunk;
} AudioCacheEntry;

static AudioCacheEntry audio_cache[AUDIO_CACHE_MAX];
static int audio_cache_count = 0;

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

static void normalize_db_name(const char *db, char *out, size_t out_size) {
    if (!out || out_size == 0) return;
    const char *src = db;
    if (!src || !src[0] || strcasecmp(src, "none") == 0)
        src = "soundeffects.omt";
    const char *base = src;
    for (const char *p = src; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    snprintf(out, out_size, "%s", base);
    char *dot = strrchr(out, '.');
    if (dot && strcasecmp(dot, ".omt") == 0) *dot = '\0';
    for (char *p = out; *p; p++) *p = (char)tolower((unsigned char)*p);
}

static Mix_Chunk *load_chunk_cached(const char *path) {
    for (int i = 0; i < audio_cache_count; i++) {
        if (strcmp(audio_cache[i].path, path) == 0)
            return audio_cache[i].chunk;
    }
    if (audio_cache_count >= AUDIO_CACHE_MAX) return NULL;
    Mix_Chunk *chunk = Mix_LoadWAV(path);
    if (!chunk) {
        fprintf(stderr, "Failed to load audio %s: %s\n", path, Mix_GetError());
        return NULL;
    }
    AudioCacheEntry *e = &audio_cache[audio_cache_count++];
    snprintf(e->path, sizeof(e->path), "%s", path);
    e->chunk = chunk;
    return chunk;
}

static int resolve_audio_handle(const char *db, int handle, int allow_single_fallback,
                                char *out_path, size_t out_size) {
    if (!out_path || out_size == 0) return 0;
    out_path[0] = '\0';

    char stem[96];
    normalize_db_name(db, stem, sizeof(stem));

    char map_path[256];
    snprintf(map_path, sizeof(map_path),
             "assets/parsed/%s/%s_audio_handles.tsv", stem, stem);
    FILE *f = fopen(map_path, "r");
    if (!f) {
        fprintf(stderr, "[AUDIO] missing handle map %s\n", map_path);
        return 0;
    }

    char line[512];
    char single_path[256] = {0};
    int single_count = 0;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        char *save = NULL;
        char *h_s = strtok_r(line, "\t\r\n", &save);
        char *path_s = strtok_r(NULL, "\t\r\n", &save);
        if (!h_s || !path_s) continue;
        int h = atoi(h_s);
        single_count++;
        if (!single_path[0])
            snprintf(single_path, sizeof(single_path), "%s", path_s);
        if (h == handle) {
            snprintf(out_path, out_size, "%s", path_s);
            fclose(f);
            return 1;
        }
    }
    fclose(f);

    if (allow_single_fallback && single_count == 1 && single_path[0]) {
        snprintf(out_path, out_size, "%s", single_path);
        fprintf(stderr, "[AUDIO] %s handle %d not present; using sole bank entry %s\n",
                stem, handle, single_path);
        return 1;
    }

    fprintf(stderr, "[AUDIO] %s handle %d not found\n", stem, handle);
    return 0;
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

int audio_sound_count(void) {
    return audio_initialized ? NUM_SOUNDS : 0;
}

int audio_play_ex(int sound_id, int loops, int gain) {
    if (!audio_initialized) return -1;
    if (sound_id < 0 || sound_id >= NUM_SOUNDS || !sounds[sound_id]) return -1;
    if (gain < 0)   gain = 0;
    if (gain > 128) gain = 128;
    int channel = Mix_PlayChannel(-1, sounds[sound_id], loops);
    if (channel < 0) return -1;
    Mix_Volume(channel, gain);
    return channel;
}

int audio_play_db(const char *db, int handle, int loops, int gain) {
    if (!audio_initialized) return -1;
    if (handle < 0) return -1;
    if (gain < 0)   gain = 0;
    if (gain > 128) gain = 128;

    char path[256];
    if (!resolve_audio_handle(db, handle, 0, path, sizeof(path)))
        return -1;
    Mix_Chunk *chunk = load_chunk_cached(path);
    if (!chunk) return -1;

    int channel = Mix_PlayChannel(-1, chunk, loops);
    if (channel < 0) {
        fprintf(stderr, "Mix_PlayChannel failed for %s: %s\n", path, Mix_GetError());
        return -1;
    }
    Mix_Volume(channel, gain);
    printf("[AUDIO] play %s[%d] -> %s on channel %d\n",
           (db && db[0]) ? db : "soundeffects.omt", handle, path, channel);
    return channel;
}

void audio_channel_gain(int channel, int gain) {
    if (!audio_initialized || channel < 0) return;
    if (gain < 0)   gain = 0;
    if (gain > 128) gain = 128;
    Mix_Volume(channel, gain);
}

void audio_channel_halt(int channel) {
    if (!audio_initialized || channel < 0) return;
    Mix_HaltChannel(channel);
}

static int active_music_track = -1;
static int active_music_channel = -1;
static char active_music_db[96] = "";

int audio_set_music_db(const char *db, int handle) {
    if (!audio_initialized) return -1;
    if (handle < 0) handle = 0;

    char stem[96];
    normalize_db_name(db, stem, sizeof(stem));
    if (active_music_channel >= 0 &&
        active_music_track == handle &&
        strcmp(active_music_db, stem) == 0)
        return active_music_track;

    char path[256];
    if (!resolve_audio_handle(db, handle, 1, path, sizeof(path)))
        return active_music_track;
    Mix_Chunk *chunk = load_chunk_cached(path);
    if (!chunk) return active_music_track;

    int prev = active_music_track;
    if (active_music_channel >= 0)
        Mix_HaltChannel(active_music_channel);
    int channel = Mix_PlayChannel(-1, chunk, -1);
    if (channel < 0) {
        fprintf(stderr, "Mix_PlayChannel music failed for %s: %s\n", path, Mix_GetError());
        return prev;
    }
    Mix_Volume(channel, 96);
    active_music_channel = channel;
    active_music_track = handle;
    snprintf(active_music_db, sizeof(active_music_db), "%s", stem);
    printf("[MUSIC] switch %s[%d] -> %s on channel %d\n", stem, handle, path, channel);
    return prev;
}

int audio_set_music(int track) {
    return audio_set_music_db("sounds.omt", track);
}

void audio_stop_all(void) {
    if (!audio_initialized) return;
    Mix_HaltChannel(-1);
    active_music_channel = -1;
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
    for (int i = 0; i < audio_cache_count; i++) {
        if (audio_cache[i].chunk) {
            Mix_FreeChunk(audio_cache[i].chunk);
            audio_cache[i].chunk = NULL;
        }
        audio_cache[i].path[0] = '\0';
    }
    audio_cache_count = 0;
    active_music_channel = -1;
    active_music_track = -1;
    active_music_db[0] = '\0';
    Mix_CloseAudio();
    audio_initialized = 0;
    printf("Audio subsystem destroyed\n");
}
