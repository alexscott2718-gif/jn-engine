# Linked-Parity Scoreboard

Generated 2026-07-02 by `tools/check_linkage_certificates.py`.

Source of truth: `docs/linkage_certificates.csv`. A `linked` row is counted
here only after its oracle ran green in this pass (see
`docs/linked_parity_plan.md` for the Linkage Certificate L1-L5 contract).

- **linked (oracle-verified):** 7
- **linked-blocked (returns to native-port):** 8

## linked

| class | aspect | domain | oracle | proof |
|---|---|---|---|---|
| `CTaskList` | tsk-deserialization | progression / objectives | `tools/linkage_oracles/CTaskList.py` | PASS CTaskList: task_parse_file == tsk_parser.py byte-exact on 5 synthesized .tsk streams; NewGame table matches CTaskList.md (SCENE=30) |
| `CLoadLevel` | gam-deserialization | progression / objectives | `tools/linkage_oracles/CLoadLevel.py` | PASS CLoadLevel: gam_load == gam_parser.parse_gam byte-exact on 35 shipped .gam files (3299 objects, 97 LOAD rows -- all 9 docs/decomp/CLoadLevel.md properties) |
| `CTaskList` | set-task-state | progression / objectives | `tools/linkage_oracles/CTaskList_set_task_state.py` | PASS CTaskList/set-task-state: task_set_entity_state == FUN_0045f990's table-write loop (case-insensitive match, write-existing-only, never append) across 4 write sequences over the documented NewGame table |
| `C3DCutSceneCamera` | 3cam-camera-math | camera / cutscene | `tools/linkage_oracles/C3DCutSceneCamera.py` | PASS C3DCutSceneCamera/3cam-camera-math: cutscene_3cam_dist byte-exact on 544 (row x t) samples across all 136 shipped 3CAM rows; cutscene_3cam_place's CameraType==2 static-branch precedence over ViewFromCamera byte-exact on all 16 real CT==2 rows; ViewFromCamera/CameraType distributions match docs/decomp/C3DCutSceneCamera.md |
| `C3DMultiCutSceneCamera` | 3mca-offset-table | camera / cutscene | `tools/linkage_oracles/C3DMultiCutSceneCamera.py` | PASS C3DMultiCutSceneCamera/3mca-offset-table: cutscene_mca_local_offset byte-exact on 5436 (entry x t) samples across 906 real CameraTypeN entries in the documented 0..4 range (all 114 shipped 3MCA rows x up to 8 steps); 6 out-of-table entries excluded per the doc's scope note |
| `CJimmyGame` | initgame-seed | progression / objectives | `tools/linkage_oracles/CJimmyGame.py` | PASS CJimmyGame/initgame-seed: game_flow_init_game reproduces the decompiled CJimmyGame::InitGame (0044d3d0) mission seed exactly (lives=5, mission_value=100), confirmed against a zero pre-seed baseline and idempotent re-seed |
| `C3DPatrolPoint` | on-arrive | AI / pathing | `tools/linkage_oracles/C3DPatrolPoint.py` | PASS C3DPatrolPoint/on-arrive: behavior_ai_find_patrol_point + gam_prop_f reproduce NextPatrolPoint resolution and WaitTime byte-exact across all 742 shipped 3PAT waypoints in 35 levels (581 edges resolve to a real neighbor) |

## linked-blocked (needs gameplay / by-eye / by-ear evidence)

| class | aspect | domain | why it cannot be linked here |
|---|---|---|---|
| `C3DPlayer` | free-roam-feel | player movement | Free-roam movement FEEL (accel curve and turn response as experienced) can only be confirmed by-eye or against a capture-with-input trace. The state-machine LOGIC is separately linkable via a headless input-trace oracle; feel confirmation returns to native-port. |
| `C3DGoddard` | texture-uv | animation / actor pose | Goddard texture/UV correctness is an art-fidelity issue with no headless oracle; needs capture/original comparison on native-port. |
| `C3DCindy` | location-pathing | AI / pathing | Cindy's correct location/patrol state is unresolved and requires original-game or capture evidence; do not guess. Returns to native-port. |
| `C3DSoundEffect` | by-ear-mix | triggers / story sequencing | Audio timbre/mix/by-ear correctness needs desktop/noVNC listening; the dispatch/trigger LOGIC is separately linkable. Returns to native-port. |
| `C3DCarl` | vehicle-rider-pose | vehicles / special movement | Carl's vehicle insertion pose/offset needs decomp proof or capture evidence; the vehicle integrator MATH is separately linkable. Returns to native-port. |
| `C3DAnimated` | ase-deserialization | animation / actor pose | ase_loader.c parses this project's own OMT->ASE exporter output (PROJECT_HISTORY.md Era 2 -- OMT->ASE exporter), not a Neutron.exe binary format -- no decompiled body exists to certify against (L1 unsatisfiable). The original binary mesh format is OMT/3DSP, already decoded and settled (docs/omt_3dsp_format.md); diffing ase_loader.c against tools/ase_parser.py would only prove two readers of this project's own re-export format agree with each other, which the plan excludes as self-comparing. Mesh/pose/texture visual correctness is the real faithfulness question and needs by-eye comparison. Returns to native-port. |
| `C3DStartPoint` | spawn | progression / objectives | Native place_player (src/game/main.c) is a genuine, faithful partial port of PlacePlayer (00442740) -- STRT tag match, position+rotation teleport, MusicDatabase/MusicIndex selection -- not inert, not a deliberate divergence. Blocked on harness cost, not on a missing body: place_player is static inside the 2,480-line main.c (full game loop: window/GL/audio init, physics, render loop), and reaching it headless would need stubbing that whole init path or a production-code extraction refactor, both out of scope for one linkage row. ViewportP*/ViewportR* camera pose and StartTrigger remain unported gaps, independent of this question. Returns to native-port or a future linkage pass. |
| `C3DCheckPoint` | progress | progression / objectives | Native vt_checkpoint (behavior_checkpoint.c) is a deliberate simplification, not a port of the decompiled UpdateCheckPoint (00414410): its own comment says it matches the original's checkpoint-progression FEEL via last-touched-wins respawn relocation, with no FINISHLINE check, race-timer gate (DAT_004eefc8), finish call (FUN_004073b0), or HUD time draw. Same shape as CJimmyGame's win-bridge exclusion: a working native behavior the project chose not to make 1:1 with the recovered body, so there is no fidelity claim to certify. Porting the actual FINISHLINE/race-timer mechanism is real behavior-porting work, out of scope for a linkage-certification pass. Returns to native-port. |
