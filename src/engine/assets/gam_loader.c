#include "gam_loader.h"
#include "../player_physics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Big-endian read helpers */
static int read_u32be(FILE *f, unsigned int *out) {
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) return 0;
    *out = ((unsigned int)b[0]<<24)|((unsigned int)b[1]<<16)|
           ((unsigned int)b[2]<<8)|(unsigned int)b[3];
    return 1;
}

static int read_f32be(FILE *f, float *out) {
    unsigned int u;
    if (!read_u32be(f, &u)) return 0;
    memcpy(out, &u, 4);
    return 1;
}


int gam_load(World *w, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "gam_load: cannot open %s\n", path); return -1; }

    /* Header: 4-byte magic + 4-byte object count (BE) */
    char magic[5] = {0};
    if (fread(magic, 1, 4, f) != 4) { fclose(f); return -1; }
    unsigned int obj_count = 0;
    if (!read_u32be(f, &obj_count)) { fclose(f); return -1; }

    printf("gam_load: %s  magic='%s'  objects=%u\n", path, magic, obj_count);

    player_physics_reset_defaults();   /* C3DPlayer props below overwrite these */

    int loaded = 0;
    char prop_name[256];
    char str_val[256];

    for (unsigned int oi = 0; oi < obj_count; oi++) {
        /* Object header: 4-byte FourCC + 4-byte prop count */
        char fcc[5] = {0};
        if (fread(fcc, 1, 4, f) != 4) break;
        unsigned int prop_count = 0;
        if (!read_u32be(f, &prop_count)) break;

        Entity *e = world_add(w);
        if (!e) break;
        memcpy(e->type, fcc, 4);
        e->type[4] = '\0';

        for (unsigned int pi = 0; pi < prop_count; pi++) {
            /* Property name */
            unsigned char name_len = 0;
            if (fread(&name_len, 1, 1, f) != 1) goto done;
            if (name_len >= (unsigned char)(sizeof(prop_name) - 1)) {
                fseek(f, name_len, SEEK_CUR);
                name_len = 0;
            } else {
                if (fread(prop_name, 1, name_len, f) != name_len) goto done;
            }
            prop_name[name_len] = '\0';

            /* Type and value length */
            unsigned int type_id = 0, val_len = 0;
            if (!read_u32be(f, &type_id)) goto done;
            if (!read_u32be(f, &val_len)) goto done;

            if (type_id == 1) {
                /* String: 1-byte redundant length prefix, then val_len chars */
                fseek(f, 1, SEEK_CUR);
                unsigned int slen = val_len < sizeof(str_val)-1 ? val_len : sizeof(str_val)-2;
                if (fread(str_val, 1, slen, f) != slen) goto done;
                str_val[slen] = '\0';
                /* Skip remainder if truncated */
                if (slen < val_len) fseek(f, val_len - slen, SEEK_CUR);

                if (strcmp(prop_name, "ObjectTag") == 0)
                    snprintf(e->tag, sizeof(e->tag), "%s", str_val);
                else if (strcmp(prop_name, "LevelName") == 0)
                    snprintf(e->target_level, sizeof(e->target_level), "%s", str_val);
                else if (strcmp(prop_name, "StartPoint") == 0)
                    snprintf(e->start_point, sizeof(e->start_point), "%s", str_val);

            } else if (type_id == 3) {
                /* float32 BE */
                float fval = 0;
                read_f32be(f, &fval);
                if (val_len > 4) fseek(f, val_len - 4, SEEK_CUR);

                if      (strcmp(prop_name, "PositionX") == 0) e->x = fval;
                else if (strcmp(prop_name, "PositionY") == 0) e->y = fval;
                else if (strcmp(prop_name, "PositionZ") == 0) e->z = fval;
                else if (strcmp(prop_name, "RotationX") == 0) e->rx = fval;
                else if (strcmp(prop_name, "RotationY") == 0) e->ry = fval;
                else if (strcmp(prop_name, "RotationZ") == 0) e->rz = fval;
                else {
                    /* C3DPlayer movement/physics constants -> PlayerPhysics. */
                    PlayerPhysics *pp = player_physics_mutable();
                    if      (strcmp(prop_name, "MaxSpeed")==0)        { pp->max_speed=fval;         pp->loaded=1; }
                    else if (strcmp(prop_name, "AccelRate")==0)       { pp->accel_rate=fval;        pp->loaded=1; }
                    else if (strcmp(prop_name, "DecelRate")==0)       { pp->decel_rate=fval;        pp->loaded=1; }
                    else if (strcmp(prop_name, "MaxHeight")==0)       { pp->max_height=fval;        pp->loaded=1; }
                    else if (strcmp(prop_name, "UpRate")==0)          { pp->up_rate=fval;           pp->loaded=1; }
                    else if (strcmp(prop_name, "DownRate")==0)        { pp->down_rate=fval;         pp->loaded=1; }
                    else if (strcmp(prop_name, "MaxVertVelocity")==0) { pp->max_vert_velocity=fval; pp->loaded=1; }
                    else if (strcmp(prop_name, "AccelLean")==0)       { pp->accel_lean=fval;        pp->loaded=1; }
                    else if (strcmp(prop_name, "DecelLean")==0)       { pp->decel_lean=fval;        pp->loaded=1; }
                }

            } else if (type_id == 6) {
                /* int32 BE — skip */
                fseek(f, val_len, SEEK_CUR);
            } else {
                /* raw / unknown — skip */
                fseek(f, val_len, SEEK_CUR);
            }

            /* For strings we consumed 1 extra byte for the prefix, nothing else needed */
        }

        /* Per-object 4-byte checksum (skip) */
        fseek(f, 4, SEEK_CUR);
        loaded++;
    }

done:
    fclose(f);
    printf("gam_load: loaded %d entities\n", loaded);
    {
        const PlayerPhysics *pp = player_physics_get();
        printf("gam_load: player physics %s: MaxSpeed=%.0f Accel=%.0f Decel=%.0f "
               "Up=%.0f Down=%.0f MaxVert=%.0f Lean=%.0f/%.0f\n",
               pp->loaded ? "from .gam" : "DEFAULT", pp->max_speed, pp->accel_rate,
               pp->decel_rate, pp->up_rate, pp->down_rate, pp->max_vert_velocity,
               pp->accel_lean, pp->decel_lean);
    }
    return loaded;
}
