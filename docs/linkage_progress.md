# Linked-Parity Scoreboard

Generated 2026-07-01 by `tools/check_linkage_certificates.py`.

Source of truth: `docs/linkage_certificates.csv`. A `linked` row is counted
here only after its oracle ran green in this pass (see
`docs/linked_parity_plan.md` for the Linkage Certificate L1-L5 contract).

- **linked (oracle-verified):** 1
- **linked-blocked (returns to native-port):** 5

## linked

| class | aspect | domain | oracle | proof |
|---|---|---|---|---|
| `CTaskList` | tsk-deserialization | progression / objectives | `tools/linkage_oracles/CTaskList.py` | PASS CTaskList: task_parse_file == tsk_parser.py byte-exact on 5 synthesized .tsk streams; NewGame table matches CTaskList.md (SCENE=30) |

## linked-blocked (needs gameplay / by-eye / by-ear evidence)

| class | aspect | domain | why it cannot be linked here |
|---|---|---|---|
| `C3DPlayer` | free-roam-feel | player movement | Free-roam movement FEEL (accel curve and turn response as experienced) can only be confirmed by-eye or against a capture-with-input trace. The state-machine LOGIC is separately linkable via a headless input-trace oracle; feel confirmation returns to native-port. |
| `C3DGoddard` | texture-uv | animation / actor pose | Goddard texture/UV correctness is an art-fidelity issue with no headless oracle; needs capture/original comparison on native-port. |
| `C3DCindy` | location-pathing | AI / pathing | Cindy's correct location/patrol state is unresolved and requires original-game or capture evidence; do not guess. Returns to native-port. |
| `C3DSoundEffect` | by-ear-mix | triggers / story sequencing | Audio timbre/mix/by-ear correctness needs desktop/noVNC listening; the dispatch/trigger LOGIC is separately linkable. Returns to native-port. |
| `C3DCarl` | vehicle-rider-pose | vehicles / special movement | Carl's vehicle insertion pose/offset needs decomp proof or capture evidence; the vehicle integrator MATH is separately linkable. Returns to native-port. |
