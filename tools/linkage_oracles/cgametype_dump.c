/* Headless dumper for CGameType::InitGame's camera-record seed linkage
   certificate.  Links the real, unmodified src/game/camera_record.c and
   proves address 00474a10 writes only DAT_00509a50 +0x44/+0x48/+0x4c.

   The pre-seed uses non-zero sentinels for every adjacent field so the oracle
   catches the old native over-reset of the record angles/mode flags. */
#include <stdio.h>
#include <string.h>

#include "../../src/game/camera_record.h"

static unsigned f32bits(float f) {
    unsigned u;
    memcpy(&u, &f, sizeof u);
    return u;
}

static void dump(const char *phase) {
    const CameraRecord *r = camera_record();
    printf("STATE|%s|%08x|%08x|%08x|%d|%d|%d|%u|%d\n",
           phase,
           f32bits(r->pos[0]), f32bits(r->pos[1]), f32bits(r->pos[2]),
           (int)r->angle[0], (int)r->angle[1], (int)r->angle[2],
           (unsigned)r->mode_flags, camera_record_mode());
}

int main(void) {
    CameraRecord *r = camera_record();
    r->pos[0] = 12.5f;
    r->pos[1] = -34.25f;
    r->pos[2] = 56.75f;
    r->angle[0] = 111;
    r->angle[1] = -222;
    r->angle[2] = 333;
    r->mode_flags = 5;
    camera_record_set_mode(CAMREC_HOLD);

    dump("PRE");
    camera_record_init_game();
    dump("POST1");
    camera_record_init_game();
    dump("POST2");
    return 0;
}
