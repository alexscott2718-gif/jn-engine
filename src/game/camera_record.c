/* Original global camera/player-target record (DAT_00509a50), ported for
 * demo/RE purposes behind the V-key toggle. Recovered sources:
 *   - docs/decomp/evidence/camera_record_layout.md — record layout, the
 *     per-frame view build (FrameStepAndRender 0047e4f0), and the generic
 *     record transforms (00476e10 / 00476f10).
 *   - docs/decomp/C3DTriggerType.md — slot-242 retarget L1 (00447a70).
 *   - docs/decomp/CGameType.md — InitGame record seed (00474a10).
 *
 * Coordinate convention: native world space mirrors the original's Z. The
 * record keeps original 14-bit angles and native-space position; original
 * rotation math runs in original space and the resulting Z displacement is
 * negated on output, matching behavior_cutscene.c entity_transform_local.
 */

#include "camera_record.h"
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* 16384 angle units == 360 degrees; angles index the trig table directly. */
#define CAMREC_STEP ((float)(2.0 * M_PI / 16384.0))

static CameraRecord g_rec;
static int g_mode = CAMREC_OFF;

CameraRecord *camera_record(void) { return &g_rec; }
int camera_record_mode(void) { return g_mode; }

void camera_record_set_mode(int mode) {
    if (mode < CAMREC_OFF || mode > CAMREC_HOLD) mode = CAMREC_OFF;
    if (g_mode != mode) {
        static const char *names[] = { "OFF", "FOLLOW", "HOLD" };
        printf("[CAMREC] mode -> %s\n", names[mode]);
    }
    g_mode = mode;
}

void camera_record_cycle_mode(void) {
    camera_record_set_mode((g_mode + 1) % 3);
}

void camera_record_init_game(void) {
    /* CGameType::InitGame (00474a10): rec.pos = (0, 10000, 0). */
    g_rec.pos[0] = 0.0f;
    g_rec.pos[1] = 10000.0f;
    g_rec.pos[2] = 0.0f;
    g_rec.angle[0] = g_rec.angle[1] = g_rec.angle[2] = 0;
    g_rec.mode_flags = 0;
}

/* sin/cos of a 14-bit angle, quantized through the table index exactly like
   the original's global_exref lookup (and cutscene_trig14). */
static void camrec_sincos(short a14, float *s, float *c) {
    float t = (float)(a14 & 0x3fff) * CAMREC_STEP;
    *s = sinf(t);
    *c = cosf(t);
}

/* CameraRecordLocalToWorld (00476e10), verbatim structure. The original adds
   the rotated offset to rec.pos in original space; native negates the Z
   displacement. */
void camera_record_local_to_world(const float local[3], float out[3]) {
    float sA, cA, sB, cB, sC, cC;
    camrec_sincos(g_rec.angle[0], &sA, &cA);
    camrec_sincos(g_rec.angle[1], &sB, &cB);
    camrec_sincos(g_rec.angle[2], &sC, &cC);

    float xz = cC * local[1] - sC * local[0];
    float yz = sC * local[1] + cC * local[0];
    float t  = cA * local[2] - xz * sA;

    out[0] = g_rec.pos[0] + (yz * cB - t * sB);
    out[1] = g_rec.pos[1] + sA * local[2] + xz * cA;
    out[2] = g_rec.pos[2] - (yz * sB + t * cB);
}

/* CameraRecordLocalToWorldDir (00476f10): rotation only, no position add. */
void camera_record_local_to_world_dir(const float local[3], float out[3]) {
    float sA, cA, sB, cB, sC, cC;
    camrec_sincos(g_rec.angle[0], &sA, &cA);
    camrec_sincos(g_rec.angle[1], &sB, &cB);
    camrec_sincos(g_rec.angle[2], &sC, &cC);

    float xz = cC * local[1] - sC * local[0];
    float yz = sC * local[1] + cC * local[0];
    float t  = cA * local[2] - xz * sA;

    out[0] = yz * cB - t * sB;
    out[1] = sA * local[2] + xz * cA;
    out[2] = -(yz * sB + t * cB);
}

/* RunTriggerTypeNextTarget record write (00447a70). Constants 20/-20/-100 are
   the fixed camera-local offset from the original body (.rdata 00495324 /
   004bd7c4 / 0049fac4). The record keeps its orientation and swings its
   position to the offset from the target. */
void camera_record_retarget(const Entity *target) {
    if (!target) return;

    float sA, cA, sB, cB, sC, cC;
    camrec_sincos(g_rec.angle[0], &sA, &cA);
    camrec_sincos(g_rec.angle[1], &sB, &cB);
    camrec_sincos(g_rec.angle[2], &sC, &cC);

    /* (X1,Y1) = Rz(C) . (20,-20), then R(A), then R(B) — per the L1 doc. */
    float x1 = 20.0f * cC + 20.0f * sC;
    float y1 = 20.0f * sC - 20.0f * cC;
    float t  = -100.0f * cA - x1 * sA;
    float oy = x1 * cA - 100.0f * sA;
    float ox = y1 * cB - t * sB;
    float oz = y1 * sB + t * cB;

    g_rec.pos[0] = target->x + ox;
    g_rec.pos[1] = target->y + oy;
    g_rec.pos[2] = target->z - oz; /* native Z mirror of the original += */

    printf("[CAMREC] retarget -> '%s' at (%.1f, %.1f, %.1f)\n",
           target->tag[0] ? target->tag : target->type,
           g_rec.pos[0], g_rec.pos[1], g_rec.pos[2]);
}

/* PROVISIONAL follow scaffolding — explicitly not an L1 port. The original
   walking camera is UpdateWalkingCameraA (00438bc0: angle_x smoothing +
   wrap-relative angle_y turn toward the player) and UpdateWalkingCameraB
   (00439900: position smoothing toward an offset target with 1.2 scaling);
   raw bodies are dumped in docs/decomp/evidence/c3dplayer_movement_target3.md
   and still need the interpretation pass. This placeholder keeps the record
   camera usable live: it eases angle_y toward "behind the player" with the
   original's 14-bit wrap arithmetic and eases position toward a fixed frame
   offset, so a NextTrigger retarget visibly swings away and back. */
void camera_record_follow_update(const Entity *player, float dt) {
    if (!player) return;

    /* Native behind-the-player yaw is (PI - ry); record yaw bridges as
       yaw = -angle_y, so angle_y = (ry - PI) in 14-bit units. */
    float want_yaw = (float)(player->ry - M_PI);
    short want_y = (short)((int)lrintf(want_yaw / CAMREC_STEP) & 0x3fff);
    short want_x = (short)((int)lrintf(-0.12f / CAMREC_STEP) & 0x3fff);

    /* 14-bit wrap-relative deltas — the (delta & 0x3fff) trick from the
       original walking camera (00438bc0). */
    int dy = ((want_y - g_rec.angle[1]) & 0x3fff);
    if (dy > 0x2000) dy -= 0x4000;
    int dx = ((want_x - g_rec.angle[0]) & 0x3fff);
    if (dx > 0x2000) dx -= 0x4000;

    float k = 1.0f - expf(-6.0f * dt);
    g_rec.angle[1] = (short)((g_rec.angle[1] + (int)(dy * k)) & 0x3fff);
    g_rec.angle[0] = (short)((g_rec.angle[0] + (int)(dx * k)) & 0x3fff);

    /* Desired eye, computed in native terms (this is scaffolding, not a port):
       back the camera out along its own current view forward, exactly how the
       record pose will be rendered (see camera_record_apply). */
    float yaw   = -(float)((short)(((g_rec.angle[1] & 0x3fff) > 0x2000)
                                   ? (g_rec.angle[1] & 0x3fff) - 0x4000
                                   : (g_rec.angle[1] & 0x3fff))) * CAMREC_STEP;
    float pitch = -0.12f;
    float fx = sinf(yaw) * cosf(pitch);
    float fy = sinf(pitch);
    float fz = -cosf(yaw) * cosf(pitch);
    float eye[3] = { player->x - fx * 240.0f,
                     player->y + 60.0f - fy * 240.0f,
                     player->z - fz * 240.0f };
    g_rec.pos[0] += (eye[0] - g_rec.pos[0]) * k;
    g_rec.pos[1] += (eye[1] - g_rec.pos[1]) * k;
    g_rec.pos[2] += (eye[2] - g_rec.pos[2]) * k;
}

/* Record -> native Camera. Derivation (camera_record_layout.md): the original
   view rotation is RotY(-angle_y)*RotX(-angle_x)*RotZ(-angle_z) with zero
   translation; the record world forward is (-cosA sinB, sinA, cosA cosB) in
   original space, which after the native Z mirror inverts against the native
   renderer forward (sin y cos p, sin p, -cos y cos p) to yaw = -angle_y,
   pitch = angle_x. */
void camera_record_apply(Camera *cam) {
    cam->pos[0] = g_rec.pos[0];
    cam->pos[1] = g_rec.pos[1];
    cam->pos[2] = g_rec.pos[2];

    short ay = (short)(g_rec.angle[1] & 0x3fff);
    short ax = (short)(g_rec.angle[0] & 0x3fff);
    if (ay > 0x2000) ay -= 0x4000; /* signed range for clean radians */
    if (ax > 0x2000) ax -= 0x4000;
    cam->yaw   = -(float)ay * CAMREC_STEP;
    cam->pitch =  (float)ax * CAMREC_STEP;
}
