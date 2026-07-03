#ifndef GAME_CAMERA_RECORD_H
#define GAME_CAMERA_RECORD_H

#include "../engine/world.h"
#include "../engine/renderer.h"

/* Port of the original game's global camera/player-target record
   (DAT_00509a50) for demo/RE purposes, behind a runtime toggle (V key).
   Layout and math: docs/decomp/evidence/camera_record_layout.md.

   The record keeps the original representation: three 14-bit angle shorts
   (16384 == 360 deg, direct trig-table indexes) and a world position. The
   position is stored in native world space; original-space rotation math is
   applied in original space and the resulting Z displacement is negated on
   output — the same convention as the certified cutscene placement port
   (behavior_cutscene.c entity_transform_local). */

typedef struct CameraRecord {
    float pos[3];              /* +0x44/+0x48/+0x4c (native world space) */
    short angle[3];            /* +0x50/+0x52/+0x54: X, Y, Z 14-bit indexes */
    unsigned short mode_flags; /* +0x114 (editor free-fly sets bits 0+2) */
} CameraRecord;

/* Demo modes: OFF = native follow cam; FOLLOW = record tracks the player
   through the ported UpdateWalkingCameraB record write; HOLD = record keeps
   its pose (what a NextTrigger retarget swings). */
enum { CAMREC_OFF = 0, CAMREC_FOLLOW = 1, CAMREC_HOLD = 2 };

CameraRecord *camera_record(void);
int  camera_record_mode(void);
void camera_record_set_mode(int mode);
void camera_record_cycle_mode(void);

/* CGameType::InitGame (00474a10) seed: pos = (0, 10000, 0). */
void camera_record_init_game(void);

/* Port of CameraRecordLocalToWorld (00476e10): rotate an original-space local
   offset through the record's three angles and add the record position.
   `local[0..2]` bind to the original's (param_2, param_3, param_4). */
void camera_record_local_to_world(const float local[3], float out[3]);
/* Port of the direction-only variant (00476f10): no position add. */
void camera_record_local_to_world_dir(const float local[3], float out[3]);

/* Port of C3DTriggerType::RunTriggerTypeNextTarget's record write (00447a70):
   swing the record position to the fixed camera-local offset (20,-20,-100)
   from `target`, rotated through the record's current angles; orientation is
   kept. The original's outer gates (trigger-focus byte DAT_0050985a, active
   trigger pointer DAT_00509980+0xb4) are not ported — the focus byte's writer
   is unrecovered — so callers gate on the demo mode instead. */
void camera_record_retarget(const Entity *target);

/* Camera-collision hook for the walking-camera write, standing in for the
   original's world ray (ProbePlayerRayBlend 0043b820 -> FUN_0047c210).
   Given the ray source (the look point) and the eye target (native space),
   return nonzero and fill `hit` when world geometry blocks the ray. NULL
   means never blocked. main.c wires the native world query (solid-entity
   AABBs + the baked collider soup, mirroring the original's two-stage
   test); the blend/k mechanism is oracle-certified via a synthetic probe. */
typedef int (*CameraRecordRayProbe)(const float src[3], const float eye[3],
                                    float hit[3]);
void camera_record_set_ray_probe(CameraRecordRayProbe probe);

/* Port of the UpdateWalkingCameraB (00439900) record write, normal path
   (motion_submode != 2) — the plain walking camera. L1 in
   docs/decomp/evidence/walking_camera_record_write.md:

     snap = rec.pos
     eye  = ProjectNoisyCameraTarget(0, 200, -350)   (yaw + turn lead, 0043a5d0)
     look = transform_local(0, 80, 0)                (00472980 via slot +0x384)
     k    = ProbePlayerRayBlend(look, &eye)          (1.0 free / 1.5 blocked,
                                                      eye pulled 75% to the hit)
     rec.pos += (eye - snap) * (1.2, 2.0, 1.2) * k * dt
     rec.angle_x/y snap to OMedia3DVector::angles(look - snap)

   `pos_native` is the player position in native space; `yaw_deg`/`roll_deg`
   are the player transform angles in original-space degrees (native bridge:
   yaw_deg = degrees(ry) - 180, roll 0); `turn_lead_deg` is the original's
   turn-rate lead (0x6d4). Angle-write shape: pitch += (target - pitch),
   yaw += 14-bit-wrapped delta — both full snaps from the pre-update
   snapshot toward the look point. */
void camera_record_walkcam_write(const float pos_native[3], float yaw_deg,
                                 float roll_deg, float turn_lead_deg,
                                 float dt);

/* FOLLOW-mode demo wrapper: drives camera_record_walkcam_write from the
   native player entity. The original's turn lead (0x6d4) is player-input
   state native does not carry; the wrapper eases a lead angle toward
   +/-30 deg at the recovered 100 deg/s ramp from the observed yaw rate. */
void camera_record_follow_update(const Entity *player, float dt);

/* Record -> native Camera bridge, from the recovered per-frame view build
   (FrameStepAndRender, 0047e4f0): the original view rotation is
   RotY(-angle_y)*RotX(-angle_x)*RotZ(-angle_z) with zero translation, and the
   native renderer forward is (sin y cos p, sin p, -cos y cos p), so
   yaw = -angle_y, pitch = angle_x after the native Z mirror. angle_z (roll)
   has no native Camera slot; gameplay writers leave it 0 (roll shows up only
   in editor free-fly). */
void camera_record_apply(Camera *cam);

#endif
