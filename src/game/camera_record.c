/* Original global camera/player-target record (DAT_00509a50), ported for
 * demo/RE purposes behind the V-key toggle. Recovered sources:
 *   - docs/decomp/evidence/camera_record_layout.md — record layout, the
 *     per-frame view build (FrameStepAndRender 0047e4f0), and the generic
 *     record transforms (00476e10 / 00476f10).
 *   - docs/decomp/C3DTriggerType.md — slot-242 retarget L1 (00447a70).
 *   - docs/decomp/CGameType.md — InitGame record seed (00474a10).
 *   - docs/decomp/evidence/walking_camera_record_write.md — the walking-
 *     camera record write (UpdateWalkingCameraB 00439900, ProjectNoisy-
 *     CameraTarget 0043a5d0, ProbePlayerRayBlend 0043b820) and the OMedia
 *     angles()/atan2 table math (OMT2.dll import, LGPL OMT source).
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

/* ---- Walking-camera record write (UpdateWalkingCameraB 00439900) --------
   L1: docs/decomp/evidence/walking_camera_record_write.md. All original-space
   math below keeps the module convention: positions native, rotation math in
   original space, Z displacement negated on output. */

/* OMedia trig quantization: degrees * 45.511112 (0048e5e8, 16384/360),
   ftol-truncated, masked to the 14-bit table. */
#define OMEDIA_DEG2IDX 45.511112f

/* OMediaCosSinTable::atan2's acos table, built exactly per init_table
   (OMediaTrigo.cpp): acos_tab[l] = short(acos(l/0x2000 - 1) * 8192/pi / 2),
   acos_tab[0x4000] = 0. */
static short omedia_acos_tab(int t) {
    static short tab[0x4001];
    static int built = 0;
    if (!built) {
        for (int l = 0; l < 0x4000; l++) {
            double acc = acos((double)l / 8192.0 - 1.0) * 8192.0 / M_PI;
            tab[l] = (short)(acc / 2.0);
        }
        tab[0x4000] = 0;
        built = 1;
    }
    if (t < 0) t = 0;
    if (t > 0x4000) t = 0x4000;
    return tab[t];
}

/* OMediaCosSinTable::atan2 (OMediaTrigo.h), float arithmetic as compiled. */
static int omedia_atan2(float opp, float adj) {
    if (opp == 0.0f && adj == 0.0f) return 0;
    float adj2 = adj * adj;
    float hyp  = adj2 + opp * opp;
    int angle = omedia_acos_tab((int)(long)(adj2 * 16384.0f / hyp));
    if (adj < 0.0f) return (opp < 0.0f) ? 8192 + angle : 8192 - angle;
    return (opp < 0.0f) ? 16384 - angle : angle;
}

/* OMedia3DVector::angles (OMedia3DVector.cpp) — the record's angle
   extraction, called by both walking cameras through the OMT2.dll import
   (IAT 0048d108). Input is an ORIGINAL-space direction. */
static void omedia_angles(const float v[3], short *ax_out, short *ay_out) {
    int ay = (-omedia_atan2(v[0], v[2])) & 0x3fff;
    float s, c;
    camrec_sincos((short)(-ay), &s, &c);
    /* rotate(0, -ay, 0): OMT left-hand y rotation; only z' feeds the pitch. */
    float rz = v[0] * s + v[2] * c;
    int ax = ((-omedia_atan2(rz, v[1])) + 4096) & 0x3fff;
    if (ax > 8192) ax = -(16384 - ax);
    *ax_out = (short)ax;
    *ay_out = (short)ay;
}

/* The two walking-camera projections, original space in, native out.
   `noisy` selects ProjectNoisyCameraTarget (0043a5d0): pitch hard-indexed
   at 0, yaw led by the turn rate, negative yaw folded 360-|int(deg)|
   (integer-degree truncation — original quirk), >=360 folded once. Plain
   selects transform_local (00472980): straight ftol+mask on all axes
   (pitch stays a parameter-free 0 here: the walking element's pitch is
   zero in the recovered walking modes). */
static void walkcam_project(const float pos_native[3], float yaw_deg,
                            float roll_deg, float lead_deg, int noisy,
                            const float local[3], float out_native[3]) {
    float yaw = yaw_deg + (noisy ? lead_deg : 0.0f);
    int yaw_idx;
    if (noisy) {
        if (yaw < 0.0f) {
            long yl = (long)yaw;
            if (yl < 0) yl = -yl;
            yaw = (float)(360 - yl);
        } else if (yaw >= 360.0f) {
            yaw -= 360.0f;
        }
        yaw_idx = (int)(long)(yaw * OMEDIA_DEG2IDX) & 0x3fff;
    } else {
        yaw_idx = (int)(long)(yaw * OMEDIA_DEG2IDX) & 0x3fff;
    }
    int roll_idx = (int)(long)(roll_deg * OMEDIA_DEG2IDX) & 0x3fff;

    float sy, cy, sr, cr;
    camrec_sincos((short)yaw_idx, &sy, &cy);
    camrec_sincos((short)roll_idx, &sr, &cr);

    /* transform_local shape with pitch = 0 (u = local z). */
    float t1 = cr * local[1] - sr * local[0];
    float t2 = cr * local[0] + sr * local[1];
    float u  = local[2];

    out_native[0] = pos_native[0] + (t2 * cy - u * sy);
    out_native[1] = pos_native[1] + t1;
    out_native[2] = pos_native[2] - (u * cy + t2 * sy); /* native Z mirror */
}

static CameraRecordRayProbe g_ray_probe = 0;

void camera_record_set_ray_probe(CameraRecordRayProbe probe) {
    g_ray_probe = probe;
}

/* SetPlayerDefaultConstants (00437740) camera offsets: eye (0x7c8/7cc/7d0),
   look (0x7d8/7dc/7e0). Axis convention (x side, y up, z forward). */
static const float WALKCAM_EYE_LOCAL[3]  = { 0.0f, 200.0f, -350.0f };
static const float WALKCAM_LOOK_LOCAL[3] = { 0.0f,  80.0f,    0.0f };

void camera_record_walkcam_write(const float pos_native[3], float yaw_deg,
                                 float roll_deg, float turn_lead_deg,
                                 float dt) {
    float snap[3] = { g_rec.pos[0], g_rec.pos[1], g_rec.pos[2] };

    float eye[3], look[3];
    walkcam_project(pos_native, yaw_deg, roll_deg, turn_lead_deg, 1,
                    WALKCAM_EYE_LOCAL, eye);
    walkcam_project(pos_native, yaw_deg, roll_deg, 0.0f, 0,
                    WALKCAM_LOOK_LOCAL, look);

    /* ProbePlayerRayBlend (0043b820): ray look -> eye; on hit the eye is
       pulled 75% of the way to the obstruction and smoothing runs 1.5x. */
    float k = 1.0f;
    if (g_ray_probe) {
        float hit[3];
        if (g_ray_probe(look, eye, hit)) {
            eye[0] = look[0] - (look[0] - hit[0]) * 0.75f;
            eye[1] = look[1] - (look[1] - hit[1]) * 0.75f;
            eye[2] = look[2] - (look[2] - hit[2]) * 0.75f;
            k = 1.5f;
        }
    }

    /* Normal-path axis scales (motion_submode != 2): x 1.2, y 2.0, z 1.2. */
    float dyy = (eye[1] - snap[1]) * k * dt;
    g_rec.pos[0] += (eye[0] - snap[0]) * 1.2f * k * dt;
    g_rec.pos[1] += dyy + dyy;
    g_rec.pos[2] += (eye[2] - snap[2]) * 1.2f * k * dt;

    /* Angle snap from the PRE-update snapshot toward the look point; the
       direction is un-mirrored back to original space for angles(). */
    float dir[3] = { look[0] - snap[0], look[1] - snap[1],
                     -(look[2] - snap[2]) };
    short ax, ay;
    omedia_angles(dir, &ax, &ay);

    unsigned short dy = (unsigned short)((ay - g_rec.angle[1]) & 0x3fff);
    int dyi = dy;
    if (dyi > 0x2000) dyi -= 0x4000;
    g_rec.angle[0] = (short)(g_rec.angle[0] + (short)(ax - g_rec.angle[0]));
    g_rec.angle[1] = (short)(g_rec.angle[1] + dyi);
}

/* FOLLOW-mode demo wrapper. Native bridge: original yaw_deg =
   degrees(ry) - 180 (native player forward (sin ry, 0, cos ry) against the
   original transform forward (-sin B, 0, cos B) after the Z mirror); roll 0.
   The turn lead stands in for the original 0x6d4 input ramp: ease toward
   +/-30 deg at 100 deg/s while the observed yaw rate says the player is
   turning, decay at 100 deg/s when not (UpdateWalkingCameraB's ramp). */
void camera_record_follow_update(const Entity *player, float dt) {
    static float prev_ry = 0.0f;
    static float lead_deg = 0.0f;
    static int   have_prev = 0;

    if (!player) return;

    float turn_dir = 0.0f;
    if (have_prev && dt > 0.0f) {
        float d = player->ry - prev_ry;
        while (d >  (float)M_PI) d -= (float)(2.0 * M_PI);
        while (d < -(float)M_PI) d += (float)(2.0 * M_PI);
        if (d >  1e-4f) turn_dir = 1.0f;
        if (d < -1e-4f) turn_dir = -1.0f;
    }
    prev_ry = player->ry;
    have_prev = 1;

    /* Recovered sign relation: B writes -0x6d4 into the transform yaw while
       projecting at yaw + 0x6d4, so the lead opposes the observed yaw rate. */
    float target = -turn_dir * 30.0f;
    float step = dt * 100.0f;
    if (lead_deg < target) {
        lead_deg += step;
        if (lead_deg > target) lead_deg = target;
    } else if (lead_deg > target) {
        lead_deg -= step;
        if (lead_deg < target) lead_deg = target;
    }

    float pos[3] = { player->x, player->y, player->z };
    float yaw_deg = player->ry * (float)(180.0 / M_PI) - 180.0f;
    camera_record_walkcam_write(pos, yaw_deg, 0.0f, lead_deg, dt);
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
