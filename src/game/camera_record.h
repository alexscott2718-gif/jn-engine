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
   (provisional scaffolding, see camera_record_follow_update); HOLD = record
   keeps its pose (what a NextTrigger retarget swings). */
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

/* PROVISIONAL follow scaffolding — NOT an L1 port. Placeholder until
   UpdateWalkingCameraA/B (00438bc0/00439900) are ported 1:1. */
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
