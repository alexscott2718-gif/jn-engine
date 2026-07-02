/* Headless dumper for the C3DTriggerType linkage oracle
   (nexttrigger-camera-retarget, slot 242 @ 00447a70). Links the REAL,
   unmodified src/game/camera_record.c and drives camera_record_retarget over
   scenarios fed on stdin, one per line:

     ax ay az px py pz tx ty tz

   (record angles as 14-bit ints, record position, native target position).
   Prints one result line per scenario:

     P <hex f32 pos.x> <hex f32 pos.y> <hex f32 pos.z>

   camera_record_retarget also logs a [CAMREC] line to stdout; the oracle
   parses only lines starting with "P ". */
#include <stdio.h>
#include <string.h>

#include "../../src/game/camera_record.h"

static unsigned f32bits(float f) {
    unsigned u;
    memcpy(&u, &f, 4);
    return u;
}

int main(void) {
    int ax, ay, az;
    float px, py, pz, tx, ty, tz;
    while (scanf("%d %d %d %f %f %f %f %f %f",
                 &ax, &ay, &az, &px, &py, &pz, &tx, &ty, &tz) == 9) {
        CameraRecord *rec = camera_record();
        rec->angle[0] = (short)ax;
        rec->angle[1] = (short)ay;
        rec->angle[2] = (short)az;
        rec->pos[0] = px;
        rec->pos[1] = py;
        rec->pos[2] = pz;

        Entity target;
        memset(&target, 0, sizeof target);
        target.x = tx;
        target.y = ty;
        target.z = tz;

        camera_record_retarget(&target);
        printf("P %08x %08x %08x\n",
               f32bits(rec->pos[0]), f32bits(rec->pos[1]), f32bits(rec->pos[2]));
    }
    return 0;
}
