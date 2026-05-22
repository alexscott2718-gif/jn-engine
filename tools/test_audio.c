/* Verify audio files can be loaded and analyzed */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    char riff[4];
    uint32_t size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} WAVHeader;

int analyze_wav(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "  ERROR: Cannot open file\n");
        return 0;
    }

    WAVHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
        fprintf(stderr, "  ERROR: Cannot read WAV header\n");
        fclose(f);
        return 0;
    }

    /* Verify RIFF magic */
    if (strncmp(hdr.riff, "RIFF", 4) != 0) {
        fprintf(stderr, "  ERROR: Invalid RIFF magic\n");
        fclose(f);
        return 0;
    }

    /* Verify WAVE format */
    if (strncmp(hdr.wave, "WAVE", 4) != 0) {
        fprintf(stderr, "  ERROR: Invalid WAVE format\n");
        fclose(f);
        return 0;
    }

    /* Verify format chunk */
    if (strncmp(hdr.fmt, "fmt ", 4) != 0) {
        fprintf(stderr, "  ERROR: Invalid fmt chunk\n");
        fclose(f);
        return 0;
    }

    printf("  Format: PCM, %d-bit, %d channel(s), %d Hz\n",
           hdr.bits_per_sample, hdr.num_channels, hdr.sample_rate);
    printf("  Byte rate: %d bytes/sec\n", hdr.byte_rate);

    /* Find data chunk */
    char chunk[4];
    uint32_t chunk_size;
    int found_data = 0;
    while (fread(chunk, 4, 1, f) == 1 && fread(&chunk_size, 4, 1, f) == 1) {
        if (strncmp(chunk, "data", 4) == 0) {
            printf("  Audio data size: %u bytes\n", chunk_size);
            if (hdr.byte_rate > 0) {
                double duration = (double)chunk_size / hdr.byte_rate;
                printf("  Duration: %.2f seconds\n", duration);
            }
            found_data = 1;
            break;
        }
        if (fseek(f, chunk_size, SEEK_CUR) != 0) break;
    }

    fclose(f);
    return found_data ? 1 : 0;
}

int main(void) {
    const char *sound_files[] = {
        "assets/parsed/sounds/sounds_audio/0000_Windows_Logon_Sound.wav",
        "assets/parsed/sounds/sounds_audio/0001_save.wav",
        "assets/parsed/sounds/sounds_audio/0002_bark1.wav",
        "assets/parsed/sounds/sounds_audio/0003_detect.wav"
    };
    const char *sound_names[] = {
        "0: Windows Logon",
        "1: save",
        "2: bark1",
        "3: detect"
    };

    printf("Audio File Analysis\n");
    printf("===================\n\n");

    int all_ok = 1;
    for (int i = 0; i < 4; i++) {
        printf("%s\n", sound_names[i]);
        printf("  Path: %s\n", sound_files[i]);
        if (!analyze_wav(sound_files[i])) {
            all_ok = 0;
            printf("  Status: FAILED\n");
        } else {
            printf("  Status: OK\n");
        }
        printf("\n");
    }

    if (all_ok) {
        printf("All audio files verified successfully.\n");
        return 0;
    } else {
        printf("Some audio files could not be verified.\n");
        return 1;
    }
}
