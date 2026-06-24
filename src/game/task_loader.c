#include "task_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* ---- .tsk binary parser (port of tools/tsk_parser.py) ------------------- */

static unsigned int read_be_u32(const unsigned char *p) {
    return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) |
           ((unsigned int)p[2] << 8)  |  (unsigned int)p[3];
}

static float read_be_f32(const unsigned char *p) {
    unsigned int u = read_be_u32(p);
    float f;
    memcpy(&f, &u, sizeof(f));
    return f;
}

/* Read a Pascal-style (u8 length + bytes) string into dst. Returns new pos. */
static size_t read_pstring(const unsigned char *data, size_t pos, size_t len,
                           char *dst, size_t dst_size) {
    if (pos >= len) { if (dst_size) dst[0] = '\0'; return pos; }
    unsigned int n = data[pos];
    size_t copy = n;
    if (copy >= dst_size) copy = dst_size - 1;
    if (pos + 1 + n <= len)
        memcpy(dst, data + pos + 1, copy);
    else
        copy = 0;
    dst[copy] = '\0';
    return pos + 1 + n;
}

/* Try to read a u8 count + count x (pstring, u32 BE) list at `pos`, requiring it
   to consume the file up to a short zero tail (the EOF anchor that tells a real
   count byte apart from stray bytes in the zero gap). Returns entity count, or
   -1 if the list is not well-formed here. */
static int try_entity_list(const unsigned char *data, size_t pos, size_t len,
                           TaskList *out) {
    unsigned int count = data[pos];
    if (count == 0 || count > 64) return -1;
    size_t p = pos + 1;
    TaskEntityState tmp[TASK_MAX_ENTITIES];
    for (unsigned int i = 0; i < count; i++) {
        if (p >= len) return -1;
        unsigned int nlen = data[p];
        if (nlen == 0 || nlen > 32 || p + 1 + nlen + 4 > len) return -1;
        const unsigned char *name = data + p + 1;
        for (unsigned int j = 0; j < nlen; j++)
            if (name[j] < 0x20 || name[j] >= 0x7f) return -1;
        size_t copy = nlen < TASK_TAG_MAX - 1 ? nlen : TASK_TAG_MAX - 1;
        memcpy(tmp[i].tag, name, copy);
        tmp[i].tag[copy] = '\0';
        tmp[i].state = read_be_u32(data + p + 1 + nlen);
        p += 1 + nlen + 4;
    }
    /* List must reach EOF; tolerate a short zero-padded tail only. */
    for (size_t q = p; q < len; q++)
        if (data[q] != 0) return -1;
    memcpy(out->entities, tmp, count * sizeof(TaskEntityState));
    out->entity_count = (int)count;
    return (int)count;
}

int task_parse_file(const char *path, TaskList *out) {
    if (!path || !out) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize < 6 || fsize > (8 << 20)) { fclose(f); return 0; }
    unsigned char *data = (unsigned char *)malloc((size_t)fsize);
    if (!data) { fclose(f); return 0; }
    size_t got = fread(data, 1, (size_t)fsize, f);
    fclose(f);
    if (got != (size_t)fsize) { free(data); return 0; }
    size_t len = (size_t)fsize;

    if (memcmp(data, "LV1B", 4) != 0) { free(data); return 0; }

    memset(out, 0, sizeof(*out));

    /* Locate the STARTEXP record (length-prefixed string "\x08STARTEXP"). */
    size_t after_spawn = 6;  /* fallback scan origin */
    const unsigned char marker[9] = { 0x08, 'S','T','A','R','T','E','X','P' };
    for (size_t i = 0; i + sizeof(marker) <= len; i++) {
        if (memcmp(data + i, marker, sizeof(marker)) == 0) {
            size_t pos = i;
            char param[64];
            pos = read_pstring(data, pos, len, out->task_name, sizeof(out->task_name));
            pos = read_pstring(data, pos, len, param, sizeof(param));        /* param1 */
            pos = read_pstring(data, pos, len, out->gam_file, sizeof(out->gam_file));
            pos = read_pstring(data, pos, len, param, sizeof(param));        /* param2 */
            if (pos + 12 <= len) {
                out->spawn[0] = read_be_f32(data + pos);
                out->spawn[1] = read_be_f32(data + pos + 4);
                out->spawn[2] = read_be_f32(data + pos + 8);
                out->has_start = 1;
            }
            after_spawn = pos + 12;
            break;
        }
    }

    /* Entity-state list: scan forward from past the spawn floats for the first
       non-zero byte that parses cleanly as the u8-counted table to EOF. */
    for (size_t pos = after_spawn; pos < len; pos++) {
        if (data[pos] == 0) continue;
        if (try_entity_list(data, pos, len, out) >= 0) break;
    }

    free(data);
    out->valid = 1;
    return 1;
}

/* ---- Baked NewGame default (from docs/tsk_data/NewGame.tsk.json) --------- */

static void task_fill_newgame_default(TaskList *out) {
    static const struct { const char *tag; unsigned int state; } baked[] = {
        { "MOM", 0 }, { "LIBBY", 0 }, { "BENNY", 0 }, { "SCENE", 30 },
        { "DINO", 0 }, { "CLONE", 0 }, { "KITTY1", 0 }, { "KITTY2", 0 },
        { "KITTY3", 0 }, { "HYDRANT1", 0 }, { "HYDRANT2", 0 }, { "REACTOR", 0 },
    };
    memset(out, 0, sizeof(*out));
    snprintf(out->task_name, sizeof(out->task_name), "STARTEXP");
    snprintf(out->gam_file, sizeof(out->gam_file), "level1b.gam");
    out->spawn[0] = 470.5965f;
    out->spawn[1] = 609.2417f;
    out->spawn[2] = -87.7631f;
    out->has_start = 1;
    int n = (int)(sizeof(baked) / sizeof(baked[0]));
    for (int i = 0; i < n; i++) {
        snprintf(out->entities[i].tag, TASK_TAG_MAX, "%s", baked[i].tag);
        out->entities[i].state = baked[i].state;
    }
    out->entity_count = n;
    out->valid = 1;
}

static int try_load_from(const char *root, const char *name, TaskList *out) {
    if (!root || !root[0]) return 0;
    char path[256];
    snprintf(path, sizeof(path), "%s/%s.tsk", root, name);
    if (task_parse_file(path, out)) return 1;
    return 0;
}

int task_load(const char *name, TaskList *out) {
    if (!name || !out) return 0;

    if (try_load_from(getenv("JN_TSK_ROOT"), name, out)) return 1;
    if (try_load_from(getenv("JN_GAM_ROOT"), name, out)) return 1;
    if (try_load_from("assets/gam", name, out)) return 1;
    if (try_load_from("/home/scotty/xp-jnbg-original", name, out)) return 1;

    /* No on-disk .tsk (proprietary, not committed). Bake the NewGame default. */
    if (strcasecmp(name, "NewGame") == 0) {
        task_fill_newgame_default(out);
        return 1;
    }
    return 0;
}

long task_entity_state(const TaskList *t, const char *tag) {
    if (!t || !tag || !tag[0]) return -1;
    for (int i = 0; i < t->entity_count; i++)
        if (strcasecmp(t->entities[i].tag, tag) == 0)
            return (long)t->entities[i].state;
    return -1;
}

int task_set_entity_state(TaskList *t, const char *tag, long value) {
    if (!t || !tag || !tag[0]) return 0;
    /* Faithful to FUN_0045f990: write an EXISTING entry; never append. SCENE and
       the KITTY1..3 mission tags are all in the seeded NewGame table, so the
       story-progress writers always find their target. */
    for (int i = 0; i < t->entity_count; i++)
        if (strcasecmp(t->entities[i].tag, tag) == 0) {
            t->entities[i].state = (unsigned int)value;
            return 1;
        }
    return 0;
}
