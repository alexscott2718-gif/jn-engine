/* Headless dumper for the C3DPlayer walking-camera linkage oracle
   (walking-camera-record, UpdateWalkingCameraB 00439900 record write).
   Links the REAL, unmodified src/game/camera_record.c and drives
   camera_record_walkcam_write over scenarios fed on stdin, one per line:

     ax ay az px py pz plx ply plz yaw roll lead dt probe hx hy hz

   (record angles as 14-bit ints, record position, player position in
   native space, player yaw/roll in original-space degrees, turn lead in
   degrees, frame dt, probe flag + synthetic ray hit point for the
   ProbePlayerRayBlend path). Prints one result line per scenario:

     P <hex f32 pos.x> <hex f32 pos.y> <hex f32 pos.z> A <ax> <ay> <az>

   camera_record_walkcam_write itself logs nothing; the oracle parses only
   lines starting with "P ". */
#include <stdio.h>
#include <string.h>

#include "../../src/game/camera_record.h"

static unsigned f32bits(float f) {
    unsigned u;
    memcpy(&u, &f, 4);
    return u;
}

static int   g_probe_on;
static float g_hit[3];

static int test_probe(const float src[3], const float eye[3], float hit[3]) {
    (void)src;
    (void)eye;
    if (!g_probe_on) return 0;
    hit[0] = g_hit[0];
    hit[1] = g_hit[1];
    hit[2] = g_hit[2];
    return 1;
}

int main(void) {
    int ax, ay, az, probe;
    float px, py, pz, plx, ply, plz, yaw, roll, lead, dt;

    camera_record_set_ray_probe(test_probe);

    while (scanf("%d %d %d %f %f %f %f %f %f %f %f %f %f %d %f %f %f",
                 &ax, &ay, &az, &px, &py, &pz, &plx, &ply, &plz,
                 &yaw, &roll, &lead, &dt, &probe,
                 &g_hit[0], &g_hit[1], &g_hit[2]) == 17) {
        CameraRecord *rec = camera_record();
        rec->angle[0] = (short)ax;
        rec->angle[1] = (short)ay;
        rec->angle[2] = (short)az;
        rec->pos[0] = px;
        rec->pos[1] = py;
        rec->pos[2] = pz;
        g_probe_on = probe;

        float pos[3] = { plx, ply, plz };
        camera_record_walkcam_write(pos, yaw, roll, lead, dt);

        printf("P %08x %08x %08x A %d %d %d\n",
               f32bits(rec->pos[0]), f32bits(rec->pos[1]), f32bits(rec->pos[2]),
               (int)rec->angle[0], (int)rec->angle[1], (int)rec->angle[2]);
    }
    return 0;
}
