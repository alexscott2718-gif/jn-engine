# `.gam` Property Schema — All Levels

> **Generated** by `tools/gam_schema.py` (idempotent; re-run after editing it).
> Source: every `assets/gam/*.gam`. This is the per-instance data the engine
> feeds each gameplay class — the same properties each class registers in its
> `InitObject` via the `vtable+0x3fc` registrar (see `ghidra_notes.md`).

- Levels parsed: **35**
- Distinct (printable) object FourCC types: **93**
- Total object instances: **3299**
- Property value types: `1=str 2=flag4 3=float 4=raw4 6=int`

**How to use (decomp):** for any placeable class, the *field map*, *constants*,
and *wiring* deliverables are mostly free here — RE only needs to recover the
*consuming logic*. Props marked ✓ are already read by `gam_loader.c`; ✗ are
dropped today (recoverable tuning/wiring).

## FourCC ↔ class map (from binary class-id registrars)

> Recovered by `tools/ghidra/Scan_ClassIds.java` (scan output committed at
> `docs/_gam_classids.tsv`). Each class registers its id as a little-endian
> immediate in its `InitObject`; `InitObject fn` is that function. The class
> string is the ctor's RTTI name where Ghidra captured it; blanks have the
> function pinned and get named in decomp Phase 0 (RTTI analyzer).

Named **71/93** placeable FourCCs; the rest have `InitObject fn` pinned.

**Note:** `3FLY` = `C3DFlyingObject` (`FUN_00419f70`) is the **movement base**
class — it registers MaxSpeed/AccelRate/DecelRate/MaxHeight/UpRate/DownRate/
MaxVertVelocity/NewGravity/AccelLean/DecelLean, which C3DPlayer/C3DJimmy inherit.

**Duplicate FourCC caveat:** `3YSH` has two registrars: `C3DYokianShield` (`FUN_0044b510`) is a runtime helper created by `C3DYokian`, while current `.gam` rows are ship-tagged AI objects and map to `C3DYokianShip` (`FUN_0044b7d0`).

| FourCC | Class | InitObject fn | id sites |
|---|---|---|---:|
| `3PAT` | C3DPatrolPoint | `FUN_00434b30` | 1 |
| `3PIC` | C3DPICKUPITEM | `FUN_004358b0` | 3 |
| `3NEU` | C3DSprite | `FUN_004329a0` | 1 |
| `3AIT` | C3DAITrigger | `FUN_0040ba70` | 1 |
| `3TRE` | C3DTree | `FUN_00446c50` | 1 |
| `3CAM` | C3DCutSceneCamera | `FUN_00415a00` | 1 |
| `3MCA` | C3DMultiCutSceneCamera | `FUN_004308d0` | 1 |
| `3SOU` | C3DSoundEffect | `FUN_00440e20` | 1 |
| `STRT` | C3DStartPoint | `FUN_004422d0` | 1 |
| `3ROK` | — (name pending Phase 0) | `FUN_0043cc40` | 1 |
| `LOAD` | C3DLoadLevel | `FUN_00457b10` | 2 |
| `3RED` | C3DREDNEUTRON | `FUN_0043c370` | 2 |
| `3BUT` | — (name pending Phase 0) | `FUN_004115c0` | 1 |
| `3CON` | C3DCone | `FUN_004152a0` | 1 |
| `3ARR` | C3DArrow | `FUN_0040f2d0` | 1 |
| `3LIO` | C3DLightObj | `FUN_0042dba0` | 1 |
| `3DOR` | C3DYokDoor | `FUN_00449cb0` | 1 |
| `3JIM` | C3DJimmy | `FUN_00422160` | 1 |
| `3ROC` | C3DRocketShip | `FUN_0043d840` | 1 |
| `3BAL` | C3DBalloon | `FUN_0040f710` | 1 |
| `3SOL` | C3DYokianSoldier | `FUN_0044bee0` | 1 |
| `3LEA` | C3DLeaves | `FUN_0042c6f0` | 1 |
| `3CHK` | C3DCheckPoint | `FUN_00414190` | 1 |
| `3TAR` | C3DShadow | `FUN_004453b0` | 2 |
| `3PHO` | C3DPHONEBOOTH | `FUN_004355a0` | 1 |
| `3OMT` | C3DOmtObj | `FUN_00434530` | 1 |
| `3TUR` | C3DYokTurret | `FUN_0044c6f0` | 1 |
| `3STE` | — (name pending Phase 0) | `FUN_00442920` | 1 |
| `3SPR` | C3DSprite | `FUN_00463f10` | 1 |
| `3EYE` | C3DEYE | `FUN_00418090` | 1 |
| `3FIS` | — (name pending Phase 0) | `FUN_00416ac0` | 1 |
| `3GUA` | C3DYokianGuard | `FUN_0044b220` | 1 |
| `3STA` | — (name pending Phase 0) | `FUN_00441bb0` | 1 |
| `3YSH` | C3DYokianShip | `FUN_0044b7d0` | 2 |
| `3YCA` | — (name pending Phase 0) | `FUN_00449360` | 1 |
| `3HUM` | C3DHumphrey | `FUN_00420730` | 1 |
| `3CAR` | C3DCarl | `FUN_00413af0` | 2 |
| `3RCK` | C3DRocket | `FUN_0043d090` | 1 |
| `3GEY` | — (name pending Phase 0) | `FUN_0041bfd0` | 1 |
| `3TRO` | C3DTrophy | `FUN_00448c60` | 1 |
| `3LAS` | C3DLaserTrigger | `FUN_0042c050` | 1 |
| `3MUS` | C3DMusicTrigger | `FUN_00431a20` | 1 |
| `3SWI` | — (name pending Phase 0) | `FUN_004449c0` | 1 |
| `3SWN` | — (name pending Phase 0) | `FUN_00444450` | 1 |
| `3AIO` | C3DAIOmtObj | `FUN_0040ae30` | 1 |
| `3FUE` | C3DROCKETFUEL | `FUN_0043d530` | 1 |
| `3ANI` | C3DAnimatedSprite | `FUN_0040e880` | 1 |
| `3SCD` | — (name pending Phase 0) | `FUN_0043f070` | 1 |
| `3TRC` | — (name pending Phase 0) | `FUN_004462c0` | 1 |
| `3CIN` | C3DCindy | `FUN_00414db0` | 1 |
| `3FAN` | — (name pending Phase 0) | `FUN_00418390` | 1 |
| `3GIR` | — (name pending Phase 0) | `FUN_0041c2d0` | 1 |
| `3SUV` | C3DAISuv | `FUN_0040b1b0` | 1 |
| `3TES` | C3DTesla | `FUN_00445870` | 1 |
| `3SHE` | C3DSheen | `FUN_0043f920` | 1 |
| `3DAI` | C3DAI | `FUN_00407a40` | 1 |
| `3FLE` | C3DFleetCommander | `FUN_00419760` | 1 |
| `3SPA` | — (name pending Phase 0) | `FUN_00441260` | 1 |
| `3SBU` | C3DBUS | `FUN_00411060` | 1 |
| `3NIC` | C3DNick | `FUN_00433490` | 1 |
| `3FOW` | C3DFowl | `FUN_0041b0a0` | 1 |
| `3LIB` | C3DLibby | `FUN_0042cd40` | 1 |
| `3BEN` | C3DBENNY | `FUN_00410340` | 1 |
| `3SPH` | C3DSphere | `FUN_00463b30` | 3 |
| `3TEL` | — (name pending Phase 0) | `FUN_00445600` | 1 |
| `3LIG` | C3DLight | `FUN_00461bb0` | 1 |
| `3FER` | C3DFERRIS | `FUN_00418cc0` | 1 |
| `3SUM` | C3DSumo | `FUN_00443dc0` | 1 |
| `3PEN` | C3DPENDULUM | `FUN_00434f60` | 1 |
| `3MOM` | C3DJUDY | `FUN_0042b2d0` | 1 |
| `3HYD` | — (name pending Phase 0) | `FUN_00420df0` | 1 |
| `3SPY` | C3DYokianSpy | `FUN_0044c2b0` | 1 |
| `3FLA` | — (name pending Phase 0) | `FUN_00419550` | 1 |
| `3DIN` | — (name pending Phase 0) | `FUN_00417100` | 1 |
| `3KIT` | C3DKitty | `FUN_0042b800` | 1 |
| `3ULT` | C3DUltraLord | `FUN_00448310` | 1 |
| `3CUB` | C3DCube | `FUN_004614e0` | 3 |
| `3HUG` | C3DHugh | `FUN_00420390` | 1 |
| `3DIG` | C3DDIGGER | `FUN_00416eb0` | 1 |
| `3SAI` | C3DSAILBOAT | `FUN_0043ecc0` | 1 |
| `3MER` | C3DMERRYGO | `FUN_0042e220` | 1 |
| `TRIG` | — (name pending Phase 0) | `FUN_0047dcf0` | 1 |
| `3OCT` | C3DOCTAPUKE | `FUN_00433e70` | 1 |
| `3PIR` | C3DPirate | `FUN_00436c40` | 1 |
| `3TRA` | — (name pending Phase 0) | `FUN_004466c0` | 1 |
| `3SM1` | C3DSmoke | `FUN_00440530` | 1 |
| `3SCR` | C3DLabScreen | `FUN_0042bc00` | 1 |
| `3DUD` | — (name pending Phase 0) | `FUN_004174f0` | 1 |
| `3TOL` | C3DToolChest | `FUN_00445f30` | 1 |
| `3CML` | C3DCamel | `FUN_00412150` | 1 |
| `3TRI` | C3DTrigger | `FUN_00446e50` | 1 |
| `3HOO` | C3DHOOK | `FUN_00420060` | 1 |
| `3SPW` | — (name pending Phase 0) | `FUN_004415a0` | 1 |

## Object types (by instance count)

| FourCC | Instances | #Props | #Unparsed | Most common ObjectTag |
|---|---:|---:|---:|---|
| `3PAT` | 742 | 21 | 11 | 1tree |
| `3PIC` | 383 | 36 | 26 | APPLEPIE |
| `3NEU` | 294 | 14 | 4 | C3DNEUTRON |
| `3AIT` | 174 | 44 | 34 | 2space1 |
| `3TRE` | 169 | 14 | 4 | C3DTREE |
| `3CAM` | 136 | 38 | 28 | 2space |
| `3MCA` | 114 | 63 | 53 | 2spacea |
| `3SOU` | 105 | 19 | 9 | C3DSOUNDEFFECT |
| `STRT` | 100 | 24 | 14 | BACKDOOR |
| `3ROK` | 99 | 19 | 11 | C3DROCK |
| `LOAD` | 97 | 24 | 12 | 2space2 |
| `3RED` | 70 | 24 | 14 | C3DREDNEUTRON |
| `3BUT` | 57 | 31 | 22 | C3DBUTTON |
| `3CON` | 56 | 14 | 4 | C3DCONE |
| `3ARR` | 47 | 17 | 7 | C3DARROW |
| `3LIO` | 37 | 24 | 14 | C3DLIGHTOBJ |
| `3DOR` | 36 | 26 | 18 | C3DYOKDOOR |
| `3JIM` | 35 | 21 | 12 | JIM1 |
| `3ROC` | 32 | 29 | 12 | C3DROCKETSHIP |
| `3BAL` | 30 | 17 | 7 | 1balloon |
| `3SOL` | 29 | 25 | 16 | C3DYOKIANSOLDIER |
| `3LEA` | 27 | 14 | 4 | C3DLEAVES |
| `3CHK` | 22 | 16 | 6 | C3DCHECKPOINT |
| `3TAR` | 22 | 25 | 15 | C3DMOVINGTARGET |
| `3PHO` | 21 | 19 | 11 | C3DPHONEBOOTH |
| `3OMT` | 20 | 22 | 14 | C3DOMTOBJ |
| `3TUR` | 18 | 25 | 16 | C3DYOKTURRET |
| `3STE` | 17 | 26 | 18 | C3DSTEAMVENT |
| `3SPR` | 15 | 11 | 4 | C3DSPRITE |
| `3EYE` | 15 | 25 | 16 | C3DEYE |
| `3FIS` | 12 | 27 | 18 | C3DDARWINFISH |
| `3GUA` | 12 | 25 | 16 | C3DYOKIANGUARD |
| `3STA` | 12 | 19 | 11 | STALAG05 |
| `3YSH` | 11 | 25 | 16 | C3DYOKIANSHIP |
| `3YCA` | 11 | 25 | 16 | C3DYOKCARGO |
| `3HUM` | 10 | 25 | 16 | C3DHUMPHREY |
| `3CAR` | 9 | 35 | 26 | C3DCARL |
| `3RCK` | 9 | 25 | 16 | rocket |
| `3GEY` | 9 | 20 | 12 | C3DGEYSER |
| `3TRO` | 9 | 19 | 11 | C3DTROPHY |
| `3LAS` | 8 | 22 | 14 | C3DLASERTRIGGER |
| `3MUS` | 8 | 30 | 20 | C3DMUSICTRIGGER |
| `3SWI` | 8 | 22 | 14 | C3DSWITCH |
| `3SWN` | 7 | 24 | 15 | C3DDOORSWING |
| `3AIO` | 7 | 29 | 20 | C3DAIOMTOBJ |
| `3FUE` | 7 | 14 | 4 | C3DROCKETFUEL |
| `3ANI` | 6 | 42 | 32 | C3DANIMATEDSPRITE |
| `3SCD` | 6 | 21 | 12 | C3DSCHOOLDOOR |
| `3TRC` | 6 | 19 | 11 | BEAM |
| `3CIN` | 6 | 35 | 26 | C3DCINDY |
| `3FAN` | 6 | 22 | 14 | FAN1 |
| `3GIR` | 5 | 27 | 18 | C3DGIRLEATINGPLANT |
| `3SUV` | 5 | 25 | 16 | C3DSUV |
| `3TES` | 5 | 20 | 12 | TESLA4 |
| `3SHE` | 4 | 35 | 26 | C3DSHEEN |
| `3DAI` | 4 | 0 | 0 |  |
| `3FLE` | 4 | 25 | 16 | FLEETC |
| `3SPA` | 4 | 20 | 12 | C3DSPARKWIRE |
| `3SBU` | 4 | 26 | 17 | C3DBUS |
| `3NIC` | 4 | 35 | 26 | C3DNICK |
| `3FOW` | 4 | 35 | 26 | C3DFOWL |
| `3LIB` | 3 | 35 | 26 | C3DLIBBY |
| `3BEN` | 3 | 35 | 26 | Benny1 |
| `3SPH` | 3 | 11 | 4 | C3DSPHERE |
| `3TEL` | 3 | 19 | 11 | C3DTELEPORTFX |
| `3LIG` | 3 | 23 | 16 | C3DLIGHT |
| `3FER` | 3 | 19 | 11 | C3DFERRIS |
| `3SUM` | 3 | 20 | 11 | C3DSUMO |
| `3PEN` | 3 | 20 | 12 | C3DPENDULUM |
| `3MOM` | 2 | 35 | 26 | C3DJUDY |
| `3HYD` | 2 | 19 | 11 | HYDRANT2 |
| `3SPY` | 2 | 25 | 16 | captain |
| `3FLA` | 2 | 19 | 11 | C3DFIRESTRATO |
| `3DIN` | 2 | 27 | 18 | C3DDINO |
| `3KIT` | 2 | 25 | 16 | C3DKITTY |
| `3ULT` | 2 | 35 | 26 | C3DULTRALORD |
| `3CUB` | 2 | 11 | 4 | C3DCUBE |
| `3HUG` | 2 | 35 | 26 | C3DHUGH |
| `3DIG` | 2 | 18 | 10 | C3DDIGGER |
| `3SAI` | 1 | 25 | 16 | SAILBOAT1 |
| `3MER` | 1 | 20 | 11 | C3DMERRYGO |
| `TRIG` | 1 | 23 | 16 | CTRIGGER |
| `3OCT` | 1 | 19 | 10 | C3DOCTAPUKE |
| `3PIR` | 1 | 19 | 10 | C3DPIRATE |
| `3TRA` | 1 | 21 | 12 | C3DTRANSLUCENT |
| `3SM1` | 1 | 14 | 4 | C3DSMOKE |
| `3SCR` | 1 | 19 | 11 | C3DLABSCREEN |
| `3DUD` | 1 | 27 | 18 | bars |
| `3TOL` | 1 | 19 | 11 | C3DTOOLCHEST |
| `3CML` | 1 | 25 | 16 | C3DCAMEL |
| `3TRI` | 1 | 32 | 22 | POWER3 |
| `3HOO` | 1 | 25 | 16 | HOOK1 |
| `3SPW` | 1 | 25 | 16 | vulta |

## Per-type property detail

### `3PAT`  — 742 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 742 | "1tree", "C3DPATROLPOINT", "CAM1", "CAM10", … |
| ✗ | `RotateToDest` | flag4 | 742 | 00010100, 01010100, 08010100 |
| ✗ | `ObjectID` | int | 742 | 860897620 … 860897620 |
| ✓ | `PositionX` | float | 742 | -6.86e+04 … 2.18e+04 |
| ✓ | `PositionY` | float | 742 | -7.33e+03 … 3.2e+04 |
| ✓ | `PositionZ` | float | 742 | -4.49e+04 … 6.94e+04 |
| ✓ | `RotationX` | float | 742 | 0 … 13.7 |
| ✓ | `RotationY` | float | 742 | 0 … 450 |
| ✓ | `RotationZ` | float | 742 | 0 … 0 |
| ✗ | `TaskName` | str | 742 | "none", "scene" |
| ✗ | `Debug` | int | 742 | 0 … 0 |
| ✓ | `SpriteSize` | int | 742 | 50 … 50 |
| ✓ | `SpriteDatabase` | str | 742 | "icons.omt", "permanenticons.omt" |
| ✓ | `SpriteIndex` | int | 742 | 6 … 6 |
| ✗ | `CallObjectTag` | str | 742 | "none" |
| ✗ | `ActivateAnim` | str | 742 | "none", "stop" |
| ✗ | `SoundDatabase` | str | 742 | "none", "soundeffects.omt" |
| ✗ | `SoundIndex` | int | 742 | -1 … 192 |
| ✗ | `NextPatrolPoint` | str | 742 | "CAM1", "CAM10", "CAM11", "CAM12", … |
| ✗ | `WaitAnim` | str | 742 | "1", "COUNT", "FIX", "STOP", … |
| ✗ | `WaitTime` | float | 742 | -1 … 1e+04 |

### `3PIC`  — 383 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 383 | "APPLEPIE", "BUBBLEPICKUP", "C3DPICKUPITEM", "NEST2", … |
| ✗ | `RotateToDest` | flag4 | 383 | 01010100 |
| ✗ | `ObjectID` | int | 383 | 860899651 … 860899651 |
| ✓ | `PositionX` | float | 383 | -6.36e+04 … 4.81e+04 |
| ✓ | `PositionY` | float | 383 | -5.76e+03 … 6.43e+03 |
| ✓ | `PositionZ` | float | 383 | -4.02e+04 … 3.88e+04 |
| ✓ | `RotationX` | float | 383 | 0 … 0 |
| ✓ | `RotationY` | float | 383 | 0 … 0 |
| ✓ | `RotationZ` | float | 383 | 0 … 0 |
| ✗ | `TaskName` | str | 383 | "SCENE", "none", "scene" |
| ✗ | `Debug` | int | 383 | 0 … 0 |
| ✓ | `SpriteSize` | int | 383 | 20 … 400 |
| ✓ | `SpriteDatabase` | str | 383 | "sprites.omt" |
| ✓ | `SpriteIndex` | int | 383 | 8 … 189 |
| ✗ | `Toggle` | int | 376 | -1 … 1 |
| ✗ | `ToggleObject` | str | 362 | "applepie", "applepie2", "applepie3", "c3dkitty", … |
| ✗ | `NextTrigger` | str | 383 | "2space1", "balloons", "dino1cam", "egg1", … |
| ✗ | `FadeType` | int | 362 | -1 … -1 |
| ✗ | `FadeTime` | float | 362 | 1 … 1 |
| ✗ | `PickupIndex` | int | 383 | 203 … 3812 |
| ✗ | `PIC_NUMBER` | int | 383 | -1 … 72 |
| ✗ | `RequiredLevel` | int | 383 | -1 … 470 |
| ✗ | `ExactLevel` | int | 383 | -1 … 170 |
| ✗ | `SoundIndex` | int | 383 | -1 … 248 |
| ✗ | `NeedMoreSound` | int | 376 | -1 … 178 |
| ✗ | `TimesToTrigger` | int | 383 | -1 … 151 |
| ✗ | `Radius` | float | 383 | 1 … 1e+03 |
| ✗ | `IsAmbient` | int | 383 | 0 … 0 |
| ✗ | `PointValue` | int | 383 | -1 … 1000 |
| ✗ | `PickedUpIndex` | int | 383 | -1 … 171 |
| ✗ | `RequiredPicNum` | int | 383 | -1 … 27 |
| ✗ | `ReqPicNumAmount` | int | 376 | -1 … 3 |
| ✗ | `InitallyActive` | int | 376 | 0 … 1 |
| ✗ | `ActivateObject` | str | 376 | "anewflurp", "cand", "cbar", "cjar", … |
| ✗ | `PassThru` | int | 362 | -1 … 1 |
| ✗ | `ShowArrow` | int | 353 | -1 … 1 |

### `3NEU`  — 294 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 294 | "C3DNEUTRON" |
| ✗ | `RotateToDest` | flag4 | 294 | 00010100, 01010100, ff010100 |
| ✗ | `ObjectID` | int | 294 | 860767573 … 860767573 |
| ✓ | `PositionX` | float | 294 | -6.36e+04 … 4.73e+04 |
| ✓ | `PositionY` | float | 294 | -5.69e+03 … 3.18e+03 |
| ✓ | `PositionZ` | float | 294 | -3.94e+04 … 3.87e+04 |
| ✓ | `RotationX` | float | 294 | 0 … 0 |
| ✓ | `RotationY` | float | 294 | 0 … 0 |
| ✓ | `RotationZ` | float | 294 | 0 … 0 |
| ✗ | `TaskName` | str | 294 | "none", "scene" |
| ✗ | `Debug` | int | 294 | 0 … 0 |
| ✓ | `SpriteSize` | int | 294 | 100 … 1300 |
| ✓ | `SpriteDatabase` | str | 294 | "icons.omt" |
| ✓ | `SpriteIndex` | int | 294 | 4 … 4 |

### `3AIT`  — 174 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 174 | "2space1", "AI1", "C3DAITRIGGER", "Carlwalk", … |
| ✗ | `RotateToDest` | flag4 | 174 | 01010100 |
| ✗ | `ObjectID` | int | 174 | 859916628 … 859916628 |
| ✓ | `PositionX` | float | 174 | -5.79e+04 … 4.72e+04 |
| ✓ | `PositionY` | float | 174 | -6.01e+03 … 1.23e+04 |
| ✓ | `PositionZ` | float | 174 | -4.49e+04 … 7.48e+04 |
| ✓ | `RotationX` | float | 174 | 0 … 0 |
| ✓ | `RotationY` | float | 174 | 0 … 180 |
| ✓ | `RotationZ` | float | 174 | 0 … 0 |
| ✗ | `TaskName` | str | 174 | "SCENE", "Scene", "none", "scene" |
| ✗ | `Debug` | int | 174 | 0 … 0 |
| ✓ | `SpriteSize` | int | 174 | 50 … 50 |
| ✓ | `SpriteDatabase` | str | 174 | "icons.omt", "permanenticons.omt" |
| ✓ | `SpriteIndex` | int | 174 | 9 … 9 |
| ✗ | `Toggle` | int | 174 | -1 … 1 |
| ✗ | `ToggleObject` | str | 174 | "LITE1", "NONE", "beam", "boatl", … |
| ✗ | `NextTrigger` | str | 174 | "2space", "AI2", "DEFAULT", "GOGODDARD", … |
| ✗ | `FadeType` | int | 174 | -1 … -1 |
| ✗ | `FadeTime` | float | 174 | 1 … 1 |
| ✗ | `Radius` | float | 174 | 1 … 3e+03 |
| ✗ | `ActivateBy` | str | 174 | "2space1", "C3DCARL", "C3DSUV", "GOCARL", … |
| ✗ | `IsA` | str | 174 | "C3DHUMPHREY", "none" |
| ✗ | `ActivateByAnim` | str | 174 | "none", "redneutron" |
| ✗ | `ActivateState0` | int | 174 | -1 … 500 |
| ✗ | `ActivateObject0` | str | 174 | "MAKEINVIS", "NONE", "crashpod", "crashsmoke", … |
| ✗ | `ActivateState1` | int | 174 | -1 … 490 |
| ✗ | `ActivateObject1` | str | 174 | "nextai", "none", "yokiancave2" |
| ✗ | `ActivateState2` | int | 174 | -1 … 125 |
| ✗ | `ActivateObject2` | str | 174 | "nextai", "none" |
| ✗ | `ActivateState3` | int | 174 | -1 … -1 |
| ✗ | `ActivateObject3` | str | 174 | "none" |
| ✗ | `ActivateState4` | int | 174 | -1 … -1 |
| ✗ | `ActivateObject4` | str | 174 | "none" |
| ✗ | `AITarget` | str | 174 | "C3DBENNY", "C3DCARL", "C3DGODDARD", "C3DSUV", … |
| ✗ | `AIAnim` | str | 174 | "BROKE", "SIT", "STOP", "TELE", … |
| ✗ | `AIState` | int | 174 | 1 … 8 |
| ✗ | `AISpeed` | int | 174 | -1 … 2900 |
| ✗ | `AIPatrol` | str | 174 | "BOAT1", "BOAT2", "GETOUT1", "GODDARDPAT1", … |
| ✗ | `AINewPos` | str | 174 | "CARLESC3", "STARTBOAT", "carl4", "fc2", … |
| ✗ | `AINewRotY` | float | 174 | -1 … 180 |
| ✗ | `AIHideObj` | int | 174 | -1 … 1 |
| ✗ | `PlayerControlled` | str | 174 | "NONE", "NULL", "jim1", "none", … |
| ✗ | `TimesToTrigger` | int | 174 | -1 … 99 |
| ✗ | `TouchActivated` | int | 174 | 0 … 1 |

### `3TRE`  — 169 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 169 | "C3DTREE", "c3dtree", "earth" |
| ✗ | `RotateToDest` | flag4 | 169 | 00010100, 01010100, 03010100 |
| ✗ | `ObjectID` | int | 169 | 861164101 … 861164101 |
| ✓ | `PositionX` | float | 169 | -1.84e+04 … 1.49e+04 |
| ✓ | `PositionY` | float | 169 | -539 … 3.18e+04 |
| ✓ | `PositionZ` | float | 169 | -1.7e+04 … 8.26e+04 |
| ✓ | `RotationX` | float | 169 | 0 … 0 |
| ✓ | `RotationY` | float | 169 | 0 … 0 |
| ✓ | `RotationZ` | float | 169 | 0 … 0 |
| ✗ | `TaskName` | str | 169 | "none", "scene" |
| ✗ | `Debug` | int | 169 | 0 … 0 |
| ✓ | `SpriteSize` | int | 169 | 1 … 4000 |
| ✓ | `SpriteDatabase` | str | 169 | "sprites.omt" |
| ✓ | `SpriteIndex` | int | 169 | 13 … 173 |

### `3CAM`  — 136 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 136 | "2space", "GOTKEY", "LABEXP1", "LABEXP2", … |
| ✗ | `RotateToDest` | flag4 | 136 | 01010100 |
| ✗ | `ObjectID` | int | 136 | 860045645 … 860045645 |
| ✓ | `PositionX` | float | 136 | -2.34e+04 … 1.06e+04 |
| ✓ | `PositionY` | float | 136 | -5.23e+03 … 2.58e+04 |
| ✓ | `PositionZ` | float | 136 | -3.92e+04 … 1.09e+04 |
| ✓ | `RotationX` | float | 136 | 0 … 0 |
| ✓ | `RotationY` | float | 136 | 0 … 45 |
| ✓ | `RotationZ` | float | 136 | 0 … 0 |
| ✗ | `TaskName` | str | 136 | "none", "scene" |
| ✗ | `Debug` | int | 136 | 0 … 0 |
| ✓ | `SpriteSize` | int | 136 | 50 … 50 |
| ✓ | `SpriteDatabase` | str | 136 | "icons.omt", "permanenticons.omt" |
| ✓ | `SpriteIndex` | int | 136 | 10 … 10 |
| ✗ | `Toggle` | int | 136 | -1 … 1 |
| ✗ | `ToggleObject` | str | 136 | "apple", "beam", "c3dkitty", "foil", … |
| ✗ | `NextTrigger` | str | 136 | "2spacea", "BRIDGETALK1", "GODDARDDIS", "LANDSHIP3", … |
| ✗ | `FadeType` | int | 136 | -1 … -1 |
| ✗ | `FadeTime` | float | 136 | 1 … 1 |
| ✗ | `CameraTarget` | str | 136 | "1tree", "C3DGODDARD", "C3DLIBBY", "JIM1", … |
| ✗ | `SoundDatabase` | str | 136 | "loadsfx.omt", "soundeffects.omt", "voicedemo.omt", "voicedowntown.omt", … |
| ✗ | `SoundIndex` | int | 136 | 0 … 102 |
| ✗ | `FaceObject` | str | 136 | "Jim1", "c3dcarl", "c3dgoddard", "c3dyokturret", … |
| ✗ | `ViewFromCamera` | int | 136 | 0 … 3 |
| ✗ | `TargOffsetX` | float | 136 | 0 … 300 |
| ✗ | `TargOffsetY` | float | 136 | 0 … 1e+03 |
| ✗ | `TargOffsetZ` | float | 136 | 0 … 1.5e+04 |
| ✗ | `TargetActAnim` | str | 136 | "Flip", "STOP", "TALK", "TELE", … |
| ✗ | `LoopActAnim` | int | 136 | -1 … 1 |
| ✗ | `TargetDeactAnim` | str | 136 | "BROKE", "STOP", "WAIT", "Walk", … |
| ✗ | `LookVoffset` | float | 136 | 0 … 5e+03 |
| ✗ | `CameraType` | int | 136 | 0 … 3 |
| ✗ | `ZoomSpeed` | float | 136 | -1.2e+03 … 450 |
| ✗ | `MaxDist` | float | 136 | -1e+04 … 8.7e+04 |
| ✗ | `MinDist` | float | 136 | -500 … 1e+04 |
| ✗ | `InitialDist` | float | 136 | -500 … 2.5e+04 |
| ✗ | `PlayerControlled` | str | 136 | "NULL", "none", "null" |
| ✗ | `DeactivateInv` | int | 133 | 1 … 1 |

### `3MCA`  — 114 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 114 | "2spacea", "C3DMULTICUTSCENE", "LABEXP3", "UTALK", … |
| ✗ | `RotateToDest` | flag4 | 114 | 00010100, 01010100, 3f010100 |
| ✗ | `ObjectID` | int | 114 | 860701505 … 860701505 |
| ✓ | `PositionX` | float | 114 | -8.55e+03 … 1.06e+04 |
| ✓ | `PositionY` | float | 114 | -4.98e+03 … 2.57e+03 |
| ✓ | `PositionZ` | float | 114 | -3.78e+04 … 1.17e+04 |
| ✓ | `RotationX` | float | 114 | 0 … 0 |
| ✓ | `RotationY` | float | 114 | 0 … 180 |
| ✓ | `RotationZ` | float | 114 | 0 … 0 |
| ✗ | `TaskName` | str | 114 | "Scene", "none", "scene" |
| ✗ | `Debug` | int | 114 | 0 … 0 |
| ✓ | `SpriteSize` | int | 114 | 50 … 50 |
| ✓ | `SpriteDatabase` | str | 114 | "icons.omt", "permanenticons.omt" |
| ✓ | `SpriteIndex` | int | 114 | 7 … 7 |
| ✗ | `Toggle` | int | 113 | -1 … 1 |
| ✗ | `ToggleObject` | str | 113 | "NONE", "beam", "book", "ending2", … |
| ✗ | `NextTrigger` | str | 114 | "2space2", "DEFAULT", "Default", "PUTGODDARD", … |
| ✗ | `FadeType` | int | 112 | -1 … -1 |
| ✗ | `FadeTime` | float | 112 | 1 … 1 |
| ✗ | `SoundDatabase` | str | 114 | "loadsfx.omt", "none", "voicedemo.omt", "voicedowntown.omt", … |
| ✗ | `TargetDeactAnim` | str | 114 | "STOP", "none", "stop" |
| ✗ | `CameraTarget0` | str | 114 | "C3DCARL", "JIM1", "Jim1", "b1", … |
| ✗ | `TargetAnim0` | str | 114 | "BROKE", "None", "TALK", "TELE", … |
| ✗ | `LookatVOffset0` | float | 114 | 0 … 280 |
| ✗ | `SoundIndex0` | int | 114 | -1 … 100 |
| ✗ | `CameraType0` | int | 114 | 0 … 4 |
| ✗ | `CameraTarget1` | str | 114 | "C3DBENNY", "C3DCARL", "C3DCINDY", "C3DFowl", … |
| ✗ | `TargetAnim1` | str | 114 | "EAT", "TALK", "Talk", "WIPE", … |
| ✗ | `LookatVOffset1` | float | 114 | 50 … 200 |
| ✗ | `SoundIndex1` | int | 114 | -1 … 99 |
| ✗ | `CameraType1` | int | 114 | 0 … 4 |
| ✗ | `CameraTarget2` | str | 114 | "C3DBENNY", "C3DCARL", "DOOR", "JIM1", … |
| ✗ | `TargetAnim2` | str | 114 | "TALK", "Talk", "attack", "cheer", … |
| ✗ | `LookatVOffset2` | float | 114 | 0 … 150 |
| ✗ | `SoundIndex2` | int | 114 | -1 … 96 |
| ✗ | `CameraType2` | int | 114 | 0 … 5 |
| ✗ | `CameraTarget3` | str | 114 | "Benny1", "C3DBENNY", "C3DCARL", "C3DCINDY", … |
| ✗ | `TargetAnim3` | str | 114 | "TALK", "Talk", "WIPE", "WPHONE", … |
| ✗ | `LookatVOffset3` | float | 114 | 100 … 150 |
| ✗ | `SoundIndex3` | int | 114 | -1 … 90 |
| ✗ | `CameraType3` | int | 114 | 0 … 4 |
| ✗ | `CameraTarget4` | str | 114 | "Benny1", "JIM1", "JIm1", "Jim1", … |
| ✗ | `TargetAnim4` | str | 114 | "TALK", "WPHONE", "none", "phone", … |
| ✗ | `LookatVOffset4` | float | 114 | 100 … 150 |
| ✗ | `SoundIndex4` | int | 114 | -1 … 85 |
| ✗ | `CameraType4` | int | 114 | 0 … 4 |
| ✗ | `CameraTarget5` | str | 114 | "Benny1", "C3DBENNY", "C3DJUDY", "C3DULTRALORD", … |
| ✗ | `TargetAnim5` | str | 114 | "NONE", "TALK", "Talk", "WIPE", … |
| ✗ | `LookatVOffset5` | float | 114 | 100 … 180 |
| ✗ | `SoundIndex5` | int | 114 | -1 … 61 |
| ✗ | `CameraType5` | int | 114 | 0 … 3 |
| ✗ | `CameraTarget6` | str | 114 | "C3DULTRALORD", "JIM1", "Jim1", "NONE", … |
| ✗ | `TargetAnim6` | str | 114 | "GIVE", "TALK", "none" |
| ✗ | `LookatVOffset6` | float | 114 | 100 … 100 |
| ✗ | `SoundIndex6` | int | 114 | -1 … 55 |
| ✗ | `CameraType6` | int | 114 | -1 … 4 |
| ✗ | `CameraTarget7` | str | 114 | "Benny1", "C3DJUDY", "C3DULTRALORD", "NONE", … |
| ✗ | `TargetAnim7` | str | 114 | "TALK", "none", "phone" |
| ✗ | `LookatVOffset7` | float | 114 | 100 … 180 |
| ✗ | `SoundIndex7` | int | 114 | -1 … 56 |
| ✗ | `CameraType7` | int | 114 | -1 … 3 |
| ✗ | `PlayerControlled` | str | 114 | "JIM1", "NULL", "none", "null" |
| ✗ | `DeactivateInv` | int | 108 | 0 … 1 |

### `3SOU`  — 105 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 105 | "C3DSOUNDEFFECT", "soundeffect" |
| ✗ | `RotateToDest` | flag4 | 105 | 01010100 |
| ✗ | `ObjectID` | int | 105 | 861097813 … 861097813 |
| ✓ | `PositionX` | float | 105 | -1.52e+04 … 4.72e+04 |
| ✓ | `PositionY` | float | 105 | -4.8e+03 … 4.33e+03 |
| ✓ | `PositionZ` | float | 105 | -3.01e+04 … 3.67e+04 |
| ✓ | `RotationX` | float | 105 | 0 … 0 |
| ✓ | `RotationY` | float | 105 | 0 … 0 |
| ✓ | `RotationZ` | float | 105 | 0 … 0 |
| ✗ | `TaskName` | str | 105 | "none", "scene" |
| ✗ | `Debug` | int | 105 | 0 … 0 |
| ✓ | `SpriteSize` | int | 105 | 50 … 50 |
| ✓ | `SpriteDatabase` | str | 105 | "icons.omt" |
| ✓ | `SpriteIndex` | int | 105 | 8 … 8 |
| ✗ | `SoundIndex` | int | 105 | -1 … 241 |
| ✗ | `TimesToTrigger` | int | 105 | -1 … 99 |
| ✗ | `Radius` | float | 105 | 50 … 2.5e+04 |
| ✗ | `IsAmbient` | int | 105 | -1 … 1 |
| ✗ | `RequiredLevel` | int | 105 | 0 … 70 |

### `STRT`  — 100 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 100 | "BACKDOOR", "FRONTDOOR", "FRONTDOOR1", "LEVEL5", … |
| ✗ | `RotateToDest` | flag4 | 100 | 00010100, 01000100, 01010100 |
| ✗ | `ObjectID` | int | 100 | 1398035028 … 1398035028 |
| ✓ | `PositionX` | float | 100 | -1.56e+04 … 1.18e+04 |
| ✓ | `PositionY` | float | 100 | -4.94e+03 … 1.13e+04 |
| ✓ | `PositionZ` | float | 100 | -3.8e+04 … 1.2e+04 |
| ✓ | `RotationX` | float | 100 | 0 … 180 |
| ✓ | `RotationY` | float | 100 | -160 … 320 |
| ✓ | `RotationZ` | float | 100 | 0 … 0 |
| ✗ | `TaskName` | str | 100 | "none", "scene" |
| ✗ | `Debug` | int | 100 | 0 … 0 |
| ✓ | `SpriteSize` | int | 100 | 100 … 100 |
| ✓ | `SpriteDatabase` | str | 100 | "Icons.omt", "icons.omt", "permanentIcons.omt" |
| ✓ | `SpriteIndex` | int | 100 | 5 … 5 |
| ✗ | `ViewportPx` | float | 100 | -1.57e+04 … 1.22e+04 |
| ✗ | `ViewportPy` | float | 100 | -4.99e+03 … 1.23e+04 |
| ✗ | `ViewportPz` | float | 100 | -3.78e+04 … 1.14e+04 |
| ✗ | `ViewportRx` | float | 100 | 0 … 0 |
| ✗ | `ViewportRy` | float | 100 | 0 … 0 |
| ✗ | `ViewportRz` | float | 100 | 0 … 0 |
| ✗ | `MusicDatabase` | str | 100 | "musicarea51.omt", "musiccandybar.omt", "musicdirtrace.omt", "musicdowntown.omt" |
| ✗ | `MusicIndex` | int | 100 | -1 … 2 |
| ✗ | `StartTrigger` | str | 98 | "LABEXP", "ZOOM1", "bb1", "bu", … |
| ✗ | `TaskState` | int | 2 | 0 … 0 |

### `3ROK`  — 99 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 99 | "C3DROCK" |
| ✗ | `RotateToDest` | flag4 | 99 | 01010100 |
| ✗ | `ObjectID` | int | 99 | 861032267 … 861032267 |
| ✓ | `PositionX` | float | 99 | 0 … 0 |
| ✓ | `PositionY` | float | 99 | 0 … 0 |
| ✓ | `PositionZ` | float | 99 | 0 … 0 |
| ✓ | `RotationX` | float | 99 | 0 … 0 |
| ✓ | `RotationY` | float | 99 | 0 … 0 |
| ✓ | `RotationZ` | float | 99 | 0 … 0 |
| ✗ | `TaskName` | str | 99 | "none" |
| ✗ | `Debug` | int | 99 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 99 | -1 … -1 |
| ✗ | `ExactLevel` | int | 99 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 99 | -1 … -1 |
| ✗ | `HasCollision` | int | 99 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 99 | -1 … -1 |
| ✗ | `CanMove` | int | 99 | 1 … 1 |
| ✗ | `SecondPass` | int | 99 | 0 … 0 |
| ✗ | `PickupLink` | str | 99 | "none" |

### `LOAD`  — 97 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 97 | "2space2", "C3DLOADLEVEL", "GOTOLAB", "LABTONEIGH", … |
| ✗ | `RotateToDest` | flag4 | 97 | 00010100, 01000100, 01010100 |
| ✗ | `ObjectID` | int | 97 | 1280262468 … 1280262468 |
| ✓ | `PositionX` | float | 97 | -3.14e+04 … 1.33e+04 |
| ✓ | `PositionY` | float | 97 | -4.09e+03 … 6.4e+03 |
| ✓ | `PositionZ` | float | 97 | -4.17e+04 … 1.35e+04 |
| ✓ | `RotationX` | float | 97 | 0 … 0 |
| ✓ | `RotationY` | float | 97 | 0 … 0 |
| ✓ | `RotationZ` | float | 97 | 0 … 0 |
| ✗ | `TaskName` | str | 97 | "SCENE", "Scene", "none", "scene" |
| ✗ | `Debug` | int | 97 | 0 … 0 |
| ✓ | `SpriteSize` | int | 97 | 100 … 200 |
| ✓ | `SpriteDatabase` | str | 97 | "icons.omt", "permanenticons.omt" |
| ✓ | `SpriteIndex` | int | 97 | 2 … 2 |
| ✓ | `LevelName` | str | 97 | "RETURN", "level1.gam", "level1a.gam", "level1b.gam", … |
| ✓ | `StartPoint` | str | 97 | "FRONTDOOR", "NONE", "PHONEBOOTH", "PPOUTSIDE", … |
| ✗ | `RequiredTask` | str | 97 | "SCENE", "Scene", "none", "scene", … |
| ✗ | `RequiredLevel` | int | 97 | -1 … 550 |
| ✗ | `ExactLevel` | int | 95 | -1 … 470 |
| ✗ | `Radius` | float | 97 | 0 … 3e+03 |
| ✗ | `SoundIndex` | int | 97 | -1 … 86 |
| ✗ | `FadeType` | int | 93 | -1 … 2 |
| ✗ | `FadeTime` | float | 93 | 0 … 5.5 |
| ✗ | `TaskState` | int | 2 | 0 … 0 |

### `3RED`  — 70 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 70 | "C3DREDNEUTRON", "c3dredneutron", "redneutron1", "redneutron2" |
| ✗ | `RotateToDest` | flag4 | 70 | 01010100, 1c010100, 5f010100 |
| ✗ | `ObjectID` | int | 70 | 861029700 … 861029700 |
| ✓ | `PositionX` | float | 70 | -6e+04 … 4.44e+04 |
| ✓ | `PositionY` | float | 70 | -4.85e+03 … 6.07e+03 |
| ✓ | `PositionZ` | float | 70 | -3.95e+04 … 3.89e+04 |
| ✓ | `RotationX` | float | 70 | 0 … 0 |
| ✓ | `RotationY` | float | 70 | 0 … 0 |
| ✓ | `RotationZ` | float | 70 | 0 … 0 |
| ✗ | `TaskName` | str | 70 | "none", "scene" |
| ✗ | `Debug` | int | 70 | 0 … 0 |
| ✓ | `SpriteSize` | int | 70 | 50 … 250 |
| ✓ | `SpriteDatabase` | str | 70 | "icons.omt" |
| ✓ | `SpriteIndex` | int | 70 | 4 … 4 |
| ✗ | `Toggle` | int | 70 | -1 … -1 |
| ✗ | `ToggleObject` | str | 70 | "none" |
| ✗ | `NextTrigger` | str | 70 | "downbeat", "none", "phonego", "shrink1" |
| ✗ | `FadeType` | int | 70 | -1 … -1 |
| ✗ | `FadeTime` | float | 70 | 1 … 1 |
| ✗ | `PickupIndex` | int | 70 | 201 … 2902 |
| ✗ | `PIC_NUMBER` | int | 70 | -1 … -1 |
| ✗ | `RequiredLevel` | int | 70 | 0 … 200 |
| ✗ | `ExactLevel` | int | 70 | -1 … -1 |
| ✗ | `Radius` | float | 70 | 75 … 400 |

### `3BUT`  — 57 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 57 | "C3DBUTTON", "C3DBUTTOND1", "C3DBUTTOND2", "REACTOR", … |
| ✗ | `RotateToDest` | flag4 | 57 | 01010100, c5010100 |
| ✗ | `ObjectID` | int | 57 | 859985236 … 859985236 |
| ✓ | `PositionX` | float | 57 | -1.61e+04 … 4.06e+03 |
| ✓ | `PositionY` | float | 57 | -1.42e+03 … 2.93e+03 |
| ✓ | `PositionZ` | float | 57 | -5.88e+03 … 1.18e+04 |
| ✓ | `RotationX` | float | 57 | 0 … 320 |
| ✓ | `RotationY` | float | 57 | 0 … 270 |
| ✓ | `RotationZ` | float | 57 | 0 … 0 |
| ✗ | `TaskName` | str | 57 | "POWER", "none", "scene" |
| ✗ | `Debug` | int | 57 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 57 | -1 … 0 |
| ✗ | `ExactLevel` | int | 57 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 57 | -1 … -1 |
| ✗ | `HasCollision` | int | 57 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 57 | -1 … -1 |
| ✗ | `CanMove` | int | 57 | 0 … 1 |
| ✗ | `SecondPass` | int | 57 | 0 … 0 |
| ✗ | `PickupLink` | str | 57 | "none" |
| ✗ | `Red` | float | 57 | 0 … 1 |
| ✗ | `Green` | float | 57 | 0 … 1 |
| ✗ | `Blue` | float | 57 | 0 … 1 |
| ✗ | `ButtonAvailable` | int | 57 | 0 … 1 |
| ✗ | `NASound` | int | 57 | -1 … -1 |
| ✗ | `AvailSound` | int | 57 | -1 … 134 |
| ✓ | `ActivateButton` | str | 57 | "DOORGRILL2", "DOORPP1", "anotherdoor", "bars", … |
| ✗ | `NewTaskState` | int | 57 | 0 … 1 |
| ✗ | `Toggle` | int | 57 | -1 … 1 |
| ✗ | `Down.ase` | str | 57 | "buttondown.ase", "buttondownship.ase", "secretbrick.ase" |
| ✗ | `Up.ase` | str | 57 | "buttonup.ase", "buttonupship.ase", "secretbrick.ase" |
| ✗ | `UpDown.Png` | str | 57 | "none", "secretbrick.png", "switch.png" |

### `3CON`  — 56 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 56 | "C3DCONE" |
| ✗ | `RotateToDest` | flag4 | 56 | 01010100 |
| ✗ | `ObjectID` | int | 56 | 860049230 … 860049230 |
| ✓ | `PositionX` | float | 56 | -1.67e+03 … 7.21e+03 |
| ✓ | `PositionY` | float | 56 | -225 … 390 |
| ✓ | `PositionZ` | float | 56 | -1.4e+04 … -7.15e+03 |
| ✓ | `RotationX` | float | 56 | 0 … 0 |
| ✓ | `RotationY` | float | 56 | 0 … 0 |
| ✓ | `RotationZ` | float | 56 | 0 … 0 |
| ✗ | `TaskName` | str | 56 | "none", "scene" |
| ✗ | `Debug` | int | 56 | 0 … 0 |
| ✓ | `SpriteSize` | int | 56 | 70 … 70 |
| ✓ | `SpriteDatabase` | str | 56 | "sprites.omt" |
| ✓ | `SpriteIndex` | int | 56 | 41 … 41 |

### `3ARR`  — 47 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 47 | "C3DARROW", "housearrow" |
| ✗ | `RotateToDest` | flag4 | 47 | 00010100, 01010100, 10010100 |
| ✗ | `ObjectID` | int | 47 | 859918930 … 859918930 |
| ✓ | `PositionX` | float | 47 | -6.51e+03 … 1.26e+04 |
| ✓ | `PositionY` | float | 47 | -3.95e+03 … 2.55e+03 |
| ✓ | `PositionZ` | float | 47 | -4.28e+04 … 1.31e+04 |
| ✓ | `RotationX` | float | 47 | 0 … 0 |
| ✓ | `RotationY` | float | 47 | 0 … 0 |
| ✓ | `RotationZ` | float | 47 | 0 … 0 |
| ✗ | `TaskName` | str | 47 | "none", "scene" |
| ✗ | `Debug` | int | 47 | 0 … 0 |
| ✓ | `SpriteSize` | int | 47 | 200 … 600 |
| ✓ | `SpriteDatabase` | str | 47 | "sprites.omt" |
| ✓ | `SpriteIndex` | int | 47 | 33 … 33 |
| ✗ | `RequiredTask` | str | 47 | "Scene", "none", "scene" |
| ✗ | `RequiredLevel` | int | 47 | 0 … 500 |
| ✗ | `ExactLevel` | int | 47 | -1 … -1 |

### `3LIO`  — 37 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 37 | "C3DLIGHTOBJ", "LITE1", "golight", "voxlight" |
| ✗ | `RotateToDest` | flag4 | 37 | 01010100 |
| ✗ | `ObjectID` | int | 37 | 860637519 … 860637519 |
| ✓ | `PositionX` | float | 37 | -1.21e+03 … 6.12e+03 |
| ✓ | `PositionY` | float | 37 | -898 … 1.59e+04 |
| ✓ | `PositionZ` | float | 37 | -2.57e+03 … 3.68e+04 |
| ✓ | `RotationX` | float | 37 | 0 … 0 |
| ✓ | `RotationY` | float | 37 | 0 … 0 |
| ✓ | `RotationZ` | float | 37 | 0 … 0 |
| ✗ | `TaskName` | str | 37 | "scene" |
| ✗ | `Debug` | int | 37 | 0 … 0 |
| ✓ | `SpriteSize` | int | 37 | 20 … 2500 |
| ✓ | `SpriteDatabase` | str | 37 | "sprites.omt" |
| ✓ | `SpriteIndex` | int | 37 | 36 … 36 |
| ✗ | `Alpha` | float | 37 | 0 … 1 |
| ✗ | `Red` | float | 37 | 0 … 1 |
| ✗ | `Green` | float | 37 | 0 … 1 |
| ✗ | `Blue` | float | 37 | 0 … 1 |
| ✗ | `Activated` | int | 37 | 1 … 1 |
| ✗ | `Pulsate` | int | 37 | 0 … 1 |
| ✗ | `Period` | float | 37 | 0.3 … 3 |
| ✗ | `OffsetTime` | float | 37 | 0 … 0.5 |
| ✗ | `OnSoundIndex` | int | 37 | -1 … -1 |
| ✗ | `OffSoundIndex` | int | 37 | -1 … -1 |

### `3DOR`  — 36 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 36 | "C3DYOKDOOR", "DOORGRILL2", "DOORPP1", "anotherdoor", … |
| ✗ | `RotateToDest` | flag4 | 36 | 01010100 |
| ✗ | `ObjectID` | int | 36 | 860114770 … 860114770 |
| ✓ | `PositionX` | float | 36 | -1.46e+04 … 3.31e+03 |
| ✓ | `PositionY` | float | 36 | -4.16e+03 … 2.9e+03 |
| ✓ | `PositionZ` | float | 36 | -5.23e+03 … 1.33e+04 |
| ✓ | `RotationX` | float | 36 | 0 … 0 |
| ✓ | `RotationY` | float | 36 | 0 … 340 |
| ✓ | `RotationZ` | float | 36 | 0 … 0 |
| ✗ | `TaskName` | str | 36 | "none", "scene" |
| ✗ | `Debug` | int | 36 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 36 | -1 … 400 |
| ✗ | `ExactLevel` | int | 36 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 36 | -1 … 0 |
| ✗ | `HasCollision` | int | 36 | -1 … 1 |
| ✓ | `InitiallyVisible` | int | 36 | -1 … -1 |
| ✗ | `CanMove` | int | 36 | 0 … 1 |
| ✗ | `SecondPass` | int | 36 | 0 … 0 |
| ✗ | `PickupLink` | str | 36 | "none" |
| ✗ | `ItemClosed` | int | 36 | 1 … 1 |
| ✗ | `Next` | str | 36 | "none" |
| ✗ | `DoorSpeed` | float | 36 | 0.5 … 5 |
| ✗ | `OpenTime` | float | 36 | 2 … 1e+05 |
| ✗ | `OpenAmount` | float | 36 | 0 … 600 |
| ✗ | `OmtDatabase` | str | 36 | "doors.omt", "objectslevel5a.omt" |
| ✗ | `OmtIndex` | int | 36 | 0 … 3 |

### `3JIM`  — 35 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 35 | "JIM1" |
| ✗ | `RotateToDest` | flag4 | 35 | 00000100, 00010100 |
| ✗ | `ObjectID` | int | 35 | 860506445 … 860506445 |
| ✓ | `PositionX` | float | 35 | -1.55e+04 … 9.87e+03 |
| ✓ | `PositionY` | float | 35 | -4.94e+03 … 1.05e+04 |
| ✓ | `PositionZ` | float | 35 | -3.59e+04 … 6.71e+03 |
| ✓ | `RotationX` | float | 35 | 0 … 0 |
| ✓ | `RotationY` | float | 35 | 0 … 290 |
| ✓ | `RotationZ` | float | 35 | 0 … 0 |
| ✗ | `TaskName` | str | 35 | "none" |
| ✗ | `Debug` | int | 35 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 32 | -1 … 0 |
| ✗ | `ExactLevel` | int | 32 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 32 | -1 … -1 |
| ✗ | `HasCollision` | int | 32 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 32 | -1 … -1 |
| ✗ | `CanMove` | int | 31 | 1 … 1 |
| ✗ | `SecondPass` | int | 31 | 0 … 0 |
| ✗ | `PickupLink` | str | 22 | "none" |
| ✓ | `StartPoint` | str | 35 | "FRONTDOOR", "PHONEBOOTH", "PPOUTSIDE", "STARTEXP", … |
| ✗ | `TaskState` | int | 2 | 0 … 0 |

### `3ROC`  — 32 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 32 | "C3DROCKETSHIP" |
| ✗ | `RotateToDest` | flag4 | 32 | 01010100 |
| ✗ | `ObjectID` | int | 32 | 861032259 … 861032259 |
| ✓ | `PositionX` | float | 32 | -1.55e+04 … 446 |
| ✓ | `PositionY` | float | 32 | -4.9e+03 … 6.47e+04 |
| ✓ | `PositionZ` | float | 32 | -7.41e+03 … 6.04e+03 |
| ✓ | `RotationX` | float | 32 | 0 … 360 |
| ✓ | `RotationY` | float | 32 | 0 … 68.6 |
| ✓ | `RotationZ` | float | 32 | 0 … 262 |
| ✗ | `TaskName` | str | 32 | "LIBBY", "scene" |
| ✗ | `Debug` | int | 32 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 32 | -1 … 0 |
| ✗ | `ExactLevel` | int | 32 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 32 | -1 … -1 |
| ✗ | `HasCollision` | int | 32 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 32 | -1 … -1 |
| ✗ | `CanMove` | int | 31 | 1 … 1 |
| ✗ | `SecondPass` | int | 31 | 0 … 0 |
| ✗ | `PickupLink` | str | 22 | "none" |
| ✓ | `MaxHeight` | float | 32 | 1.5e+03 … 4e+03 |
| ✓ | `MaxSpeed` | float | 32 | 1.4e+03 … 1.4e+03 |
| ✓ | `AccelRate` | float | 32 | 400 … 400 |
| ✓ | `DecelRate` | float | 32 | 1 … 1 |
| ✓ | `UpRate` | float | 32 | 650 … 650 |
| ✓ | `DownRate` | float | 32 | -650 … -650 |
| ✓ | `MaxVertVelocity` | float | 32 | 650 … 650 |
| ✗ | `NewGravity` | float | 32 | 0 … 0 |
| ✓ | `AccelLean` | float | 32 | 20 … 20 |
| ✓ | `DecelLean` | float | 32 | -20 … -20 |

### `3BAL`  — 30 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 30 | "1balloon", "C3DBALLOON" |
| ✗ | `RotateToDest` | flag4 | 30 | 01010100 |
| ✗ | `ObjectID` | int | 30 | 859980108 … 859980108 |
| ✓ | `PositionX` | float | 30 | -896 … 1.46e+04 |
| ✓ | `PositionY` | float | 30 | 155 … 1.74e+03 |
| ✓ | `PositionZ` | float | 30 | -6.29e+03 … 1.4e+03 |
| ✓ | `RotationX` | float | 30 | 0 … 0 |
| ✓ | `RotationY` | float | 30 | 0 … 0 |
| ✓ | `RotationZ` | float | 30 | 0 … 0 |
| ✗ | `TaskName` | str | 30 | "none", "scene" |
| ✗ | `Debug` | int | 30 | 0 … 0 |
| ✓ | `SpriteSize` | int | 30 | 200 … 200 |
| ✓ | `SpriteDatabase` | str | 30 | "sprites.omt" |
| ✓ | `SpriteIndex` | int | 30 | 50 … 50 |
| ✗ | `Red` | float | 30 | 0 … 4 |
| ✗ | `Green` | float | 30 | 0.1 … 5 |
| ✗ | `Blue` | float | 30 | 0 … 5 |

### `3SOL`  — 29 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 29 | "C3DYOKIANSOLDIER", "Second", "guard2", "second", … |
| ✗ | `RotateToDest` | flag4 | 29 | 01010101 |
| ✗ | `ObjectID` | int | 29 | 861097804 … 861097804 |
| ✓ | `PositionX` | float | 29 | -1.87e+04 … 3.97e+03 |
| ✓ | `PositionY` | float | 29 | -6.07e+03 … 6.42e+03 |
| ✓ | `PositionZ` | float | 29 | -3.86e+04 … 7.01e+03 |
| ✓ | `RotationX` | float | 29 | 0 … 0 |
| ✓ | `RotationY` | float | 29 | 0 … 270 |
| ✓ | `RotationZ` | float | 29 | 0 … 0.442 |
| ✗ | `TaskName` | str | 29 | "none", "scene" |
| ✗ | `Debug` | int | 29 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 29 | -1 … 400 |
| ✗ | `ExactLevel` | int | 29 | -1 … 420 |
| ✗ | `RemoveLevel` | int | 29 | -1 … 0 |
| ✗ | `HasCollision` | int | 29 | -1 … 1 |
| ✓ | `InitiallyVisible` | int | 29 | -1 … -1 |
| ✗ | `CanMove` | int | 21 | 0 … 1 |
| ✗ | `SecondPass` | int | 21 | 0 … 0 |
| ✗ | `PickupLink` | str | 20 | "none" |
| ✓ | `PatrolPoint` | str | 29 | "apple01", "ayok1", "bot01", "byok1", … |
| ✗ | `VisibleRange` | float | 29 | 0 … 2.5e+03 |
| ✗ | `FOV` | float | 29 | 1 … 359 |
| ✗ | `TargetName` | str | 29 | "JIM1", "Jim1", "none" |
| ✗ | `AIState` | int | 29 | 1 … 2 |
| ✗ | `WanderRange` | float | 29 | 800 … 1.5e+03 |

### `3LEA`  — 27 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 27 | "C3DLEAVES" |
| ✗ | `RotateToDest` | flag4 | 27 | 01010100 |
| ✗ | `ObjectID` | int | 27 | 860636481 … 860636481 |
| ✓ | `PositionX` | float | 27 | -2.72e+03 … 1.14e+04 |
| ✓ | `PositionY` | float | 27 | 372 … 835 |
| ✓ | `PositionZ` | float | 27 | -6.1e+03 … 7.43e+03 |
| ✓ | `RotationX` | float | 27 | 0 … 0 |
| ✓ | `RotationY` | float | 27 | 0 … 0 |
| ✓ | `RotationZ` | float | 27 | 0 … 0 |
| ✗ | `TaskName` | str | 27 | "none", "scene" |
| ✗ | `Debug` | int | 27 | 0 … 0 |
| ✓ | `SpriteSize` | int | 27 | 100 … 100 |
| ✓ | `SpriteDatabase` | str | 27 | "icons.omt" |
| ✓ | `SpriteIndex` | int | 27 | 4 … 45 |

### `3CHK`  — 22 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 22 | "C3DCHECKPOINT", "CHECK1", "CHECK2", "CHECK2_5", … |
| ✗ | `RotateToDest` | flag4 | 22 | 01010100 |
| ✗ | `ObjectID` | int | 22 | 860047435 … 860047435 |
| ✓ | `PositionX` | float | 22 | -4.76e+03 … 6.77e+03 |
| ✓ | `PositionY` | float | 22 | -60.8 … 515 |
| ✓ | `PositionZ` | float | 22 | -1.36e+04 … 2.46e+03 |
| ✓ | `RotationX` | float | 22 | 0 … 0 |
| ✓ | `RotationY` | float | 22 | 0 … 0 |
| ✓ | `RotationZ` | float | 22 | 0 … 0 |
| ✗ | `TaskName` | str | 22 | "none", "scene" |
| ✗ | `Debug` | int | 22 | 0 … 0 |
| ✓ | `SpriteSize` | int | 22 | 100 … 100 |
| ✓ | `SpriteDatabase` | str | 22 | "sprites.omt" |
| ✓ | `SpriteIndex` | int | 22 | 42 … 42 |
| ✗ | `CheckAvail` | int | 22 | 0 … 0 |
| ✗ | `Next` | str | 22 | "CHECK1", "CHECK2", "CHECK2_5", "CHECK3", … |

### `3TAR`  — 22 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 22 | "C3DMOVINGTARGET", "c3dmovingtarget" |
| ✗ | `RotateToDest` | flag4 | 22 | 01010100 |
| ✗ | `ObjectID` | int | 22 | 861159762 … 861159762 |
| ✓ | `PositionX` | float | 22 | -2.29e+03 … 2.77e+03 |
| ✓ | `PositionY` | float | 22 | 41.9 … 533 |
| ✓ | `PositionZ` | float | 22 | -557 … 1.6e+03 |
| ✓ | `RotationX` | float | 22 | 0 … 0 |
| ✓ | `RotationY` | float | 22 | 0 … 0 |
| ✓ | `RotationZ` | float | 22 | 0 … 0 |
| ✗ | `TaskName` | str | 22 | "SCENE", "none", "scene" |
| ✗ | `Debug` | int | 22 | 0 … 0 |
| ✓ | `SpriteSize` | int | 22 | 80 … 150 |
| ✓ | `SpriteDatabase` | str | 22 | "sprites.omt" |
| ✓ | `SpriteIndex` | int | 22 | 98 … 176 |
| ✗ | `StartPosX` | float | 22 | -2.3e+03 … 2.47e+03 |
| ✗ | `StartPosY` | float | 22 | 46 … 547 |
| ✗ | `StartPosZ` | float | 22 | -617 … 1.52e+03 |
| ✗ | `DestPosX` | float | 22 | -2.3e+03 … 2.47e+03 |
| ✗ | `DestPosY` | float | 22 | 46 … 547 |
| ✗ | `DestPosZ` | float | 22 | -486 … 1.52e+03 |
| ✗ | `Speed` | float | 22 | 20 … 115 |
| ✗ | `ActivateObject` | str | 22 | "martianticket", "none", "showerticket", "starticket", … |
| ✗ | `HitsRequired` | int | 22 | 3 … 5 |
| ✗ | `NumPoints` | int | 22 | 10 … 10 |
| ✗ | `RespawnTime` | float | 22 | 2 … 15 |

### `3PHO`  — 21 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 21 | "C3DPHONEBOOTH", "PHONE1", "PHONE2", "PHONEBOOTH" |
| ✗ | `RotateToDest` | flag4 | 21 | 01010100 |
| ✗ | `ObjectID` | int | 21 | 860899407 … 860899407 |
| ✓ | `PositionX` | float | 21 | -5.34e+03 … 1.42e+04 |
| ✓ | `PositionY` | float | 21 | -667 … 1.13e+04 |
| ✓ | `PositionZ` | float | 21 | -1.44e+04 … 6.12e+03 |
| ✓ | `RotationX` | float | 21 | 0 … 0 |
| ✓ | `RotationY` | float | 21 | 0 … 315 |
| ✓ | `RotationZ` | float | 21 | 0 … 0 |
| ✗ | `TaskName` | str | 21 | "none", "scene" |
| ✗ | `Debug` | int | 21 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 20 | -1 … 90 |
| ✗ | `ExactLevel` | int | 20 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 20 | -1 … 110 |
| ✗ | `HasCollision` | int | 20 | -1 … 0 |
| ✓ | `InitiallyVisible` | int | 20 | -1 … 0 |
| ✗ | `CanMove` | int | 19 | 1 … 1 |
| ✗ | `SecondPass` | int | 19 | 0 … 0 |
| ✗ | `PickupLink` | str | 13 | "none" |

### `3OMT`  — 20 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 20 | "C3DOMTOBJ", "beam", "bench01", "bench02", … |
| ✗ | `RotateToDest` | flag4 | 20 | 01010100 |
| ✗ | `ObjectID` | int | 20 | 860835156 … 860835156 |
| ✓ | `PositionX` | float | 20 | -2.32e+03 … 1.12e+04 |
| ✓ | `PositionY` | float | 20 | -585 … 1.48e+03 |
| ✓ | `PositionZ` | float | 20 | -2.88e+03 … 8.4e+03 |
| ✓ | `RotationX` | float | 20 | 0 … 30 |
| ✓ | `RotationY` | float | 20 | 0 … 300 |
| ✓ | `RotationZ` | float | 20 | 0 … 0 |
| ✗ | `TaskName` | str | 20 | "scene" |
| ✗ | `Debug` | int | 20 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 20 | -1 … 410 |
| ✗ | `ExactLevel` | int | 20 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 20 | -1 … 400 |
| ✗ | `HasCollision` | int | 20 | -1 … 0 |
| ✓ | `InitiallyVisible` | int | 20 | -1 … 0 |
| ✗ | `CanMove` | int | 20 | 0 … 1 |
| ✗ | `SecondPass` | int | 20 | 0 … 1 |
| ✗ | `PickupLink` | str | 19 | "none" |
| ✗ | `OmtDatabase` | str | 20 | "objects.omt" |
| ✗ | `OmtIndex` | int | 20 | 6 … 29 |
| ✗ | `Radius` | float | 20 | 117 … 713 |

### `3TUR`  — 18 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 18 | "C3DYOKTURRET", "yokturr", "yokturret" |
| ✗ | `RotateToDest` | flag4 | 18 | 01010101 |
| ✗ | `ObjectID` | int | 18 | 861164882 … 861164882 |
| ✓ | `PositionX` | float | 18 | -5.89e+04 … 4.61e+04 |
| ✓ | `PositionY` | float | 18 | -5.02e+03 … 3.6e+03 |
| ✓ | `PositionZ` | float | 18 | -3.97e+04 … 3.94e+04 |
| ✓ | `RotationX` | float | 18 | 0 … 0 |
| ✓ | `RotationY` | float | 18 | 0 … 270 |
| ✓ | `RotationZ` | float | 18 | 0 … 0 |
| ✗ | `TaskName` | str | 18 | "none", "scene" |
| ✗ | `Debug` | int | 18 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 18 | -1 … 0 |
| ✗ | `ExactLevel` | int | 18 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 18 | -1 … -1 |
| ✗ | `HasCollision` | int | 18 | 0 … 1 |
| ✓ | `InitiallyVisible` | int | 18 | -1 … 1 |
| ✗ | `CanMove` | int | 18 | 0 … 1 |
| ✗ | `SecondPass` | int | 18 | 0 … 0 |
| ✗ | `PickupLink` | str | 12 | "none" |
| ✓ | `PatrolPoint` | str | 18 | "none" |
| ✗ | `VisibleRange` | float | 18 | 7.5e+03 … 1.8e+04 |
| ✗ | `FOV` | float | 18 | 359 … 9e+03 |
| ✗ | `TargetName` | str | 18 | "JIM1" |
| ✗ | `AIState` | int | 18 | 6 … 6 |
| ✗ | `WanderRange` | float | 18 | 1.5e+03 … 1.5e+03 |

### `3STE`  — 17 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 17 | "C3DSTEAMVENT", "VENT1", "VENT10", "VENT11", … |
| ✗ | `RotateToDest` | flag4 | 17 | 00010100, 01010100, 69010100 |
| ✗ | `ObjectID` | int | 17 | 861099077 … 861099077 |
| ✓ | `PositionX` | float | 17 | -1.26e+03 … 6.18e+03 |
| ✓ | `PositionY` | float | 17 | -3.18e+03 … 2.93e+03 |
| ✓ | `PositionZ` | float | 17 | -5.8e+03 … 1.52e+04 |
| ✓ | `RotationX` | float | 17 | 0 … 0 |
| ✓ | `RotationY` | float | 17 | 0 … 270 |
| ✓ | `RotationZ` | float | 17 | 0 … 0 |
| ✗ | `TaskName` | str | 17 | "none", "scene" |
| ✗ | `Debug` | int | 17 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 17 | 0 … 0 |
| ✗ | `ExactLevel` | int | 17 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 17 | -1 … -1 |
| ✗ | `HasCollision` | int | 17 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 17 | -1 … -1 |
| ✗ | `CanMove` | int | 17 | 1 … 1 |
| ✗ | `SecondPass` | int | 17 | 0 … 0 |
| ✗ | `PickupLink` | str | 17 | "none" |
| ✗ | `Red` | float | 17 | 1 … 1 |
| ✗ | `Green` | float | 17 | 0.1 … 0.1 |
| ✗ | `Blue` | float | 17 | 0.1 … 0.1 |
| ✗ | `ButtonAvailable` | int | 17 | 0 … 1 |
| ✗ | `NASound` | int | 17 | -1 … -1 |
| ✗ | `AvailSound` | int | 17 | 81 … 81 |
| ✗ | `NextButton` | str | 17 | "VENT10", "VENT11", "VENT12", "VENT13", … |

### `3SPR`  — 15 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 15 | "C3DSPRITE" |
| ✗ | `RotateToDest` | flag4 | 15 | 01010100 |
| ✗ | `ObjectID` | int | 15 | 861098066 … 861098066 |
| ✓ | `PositionX` | float | 15 | -5.22e+03 … 9.67e+03 |
| ✓ | `PositionY` | float | 15 | 46.5 … 914 |
| ✓ | `PositionZ` | float | 15 | -1.32e+03 … 2.94e+03 |
| ✓ | `RotationX` | float | 15 | 0 … 0 |
| ✓ | `RotationY` | float | 15 | 0 … 0 |
| ✓ | `RotationZ` | float | 15 | 0 … 0 |
| ✗ | `TaskName` | str | 15 | "none", "scene" |
| ✗ | `Debug` | int | 15 | 0 … 0 |

### `3EYE`  — 15 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 15 | "C3DEYE" |
| ✗ | `RotateToDest` | flag4 | 15 | 00010101, 01010101, bf010101 |
| ✗ | `ObjectID` | int | 15 | 860182853 … 860182853 |
| ✓ | `PositionX` | float | 15 | -5.19e+03 … 4.87e+03 |
| ✓ | `PositionY` | float | 15 | 2.04e+03 … 2.93e+03 |
| ✓ | `PositionZ` | float | 15 | -4.7e+03 … 9.86e+03 |
| ✓ | `RotationX` | float | 15 | 0 … 0 |
| ✓ | `RotationY` | float | 15 | 0 … 270 |
| ✓ | `RotationZ` | float | 15 | 0 … 0 |
| ✗ | `TaskName` | str | 15 | "none" |
| ✗ | `Debug` | int | 15 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 15 | 0 … 0 |
| ✗ | `ExactLevel` | int | 15 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 15 | -1 … -1 |
| ✗ | `HasCollision` | int | 15 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 15 | -1 … -1 |
| ✗ | `CanMove` | int | 15 | 1 … 1 |
| ✗ | `SecondPass` | int | 15 | 0 … 0 |
| ✗ | `PickupLink` | str | 11 | "none" |
| ✓ | `PatrolPoint` | str | 15 | "EYE3", "EYE4", "eye01", "eye03", … |
| ✗ | `VisibleRange` | float | 15 | 2.5e+03 … 2.5e+03 |
| ✗ | `FOV` | float | 15 | 90 … 90 |
| ✗ | `TargetName` | str | 15 | "JIM1" |
| ✗ | `AIState` | int | 15 | 3 … 3 |
| ✗ | `WanderRange` | float | 15 | 1.5e+03 … 1.5e+03 |

### `3FIS`  — 12 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 12 | "C3DDARWINFISH" |
| ✗ | `RotateToDest` | flag4 | 12 | 01010101 |
| ✗ | `ObjectID` | int | 12 | 860244307 … 860244307 |
| ✓ | `PositionX` | float | 12 | -5.54e+03 … 9.07e+03 |
| ✓ | `PositionY` | float | 12 | 4.45 … 962 |
| ✓ | `PositionZ` | float | 12 | -843 … 7.03e+03 |
| ✓ | `RotationX` | float | 12 | 0 … 0 |
| ✓ | `RotationY` | float | 12 | 0 … 0 |
| ✓ | `RotationZ` | float | 12 | 0 … 0 |
| ✗ | `TaskName` | str | 12 | "clone", "none", "scene" |
| ✗ | `Debug` | int | 12 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 12 | 0 … 240 |
| ✗ | `ExactLevel` | int | 12 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 12 | -1 … 210 |
| ✗ | `HasCollision` | int | 12 | -1 … 1 |
| ✓ | `InitiallyVisible` | int | 12 | -1 … -1 |
| ✗ | `CanMove` | int | 12 | 1 … 1 |
| ✗ | `SecondPass` | int | 12 | 0 … 0 |
| ✗ | `PickupLink` | str | 11 | "none" |
| ✓ | `PatrolPoint` | str | 12 | "d1", "d2", "d3", "d4", … |
| ✗ | `VisibleRange` | float | 12 | 700 … 2e+03 |
| ✗ | `FOV` | float | 12 | 90 … 359 |
| ✗ | `TargetName` | str | 12 | "JIM1" |
| ✗ | `AIState` | int | 12 | 10 … 10 |
| ✗ | `WanderRange` | float | 12 | 700 … 1.5e+03 |
| ✗ | `PickupIndex` | int | 12 | 437 … 2405 |
| ✗ | `PIC_NUMBER` | int | 12 | 12 … 12 |

### `3GUA`  — 12 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 12 | "C3DYOKIANGUARD", "CARLGUARD1", "lastguard", "soldier2", … |
| ✗ | `RotateToDest` | flag4 | 12 | 01010101 |
| ✗ | `ObjectID` | int | 12 | 860312897 … 860312897 |
| ✓ | `PositionX` | float | 12 | -1.54e+04 … 1.6e+03 |
| ✓ | `PositionY` | float | 12 | -6.11e+03 … 1.49e+03 |
| ✓ | `PositionZ` | float | 12 | -4.02e+04 … 5.92e+03 |
| ✓ | `RotationX` | float | 12 | 0 … 0 |
| ✓ | `RotationY` | float | 12 | 0 … 270 |
| ✓ | `RotationZ` | float | 12 | 0 … 0 |
| ✗ | `TaskName` | str | 12 | "Scene", "none", "scene" |
| ✗ | `Debug` | int | 12 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 12 | -1 … 0 |
| ✗ | `ExactLevel` | int | 12 | -1 … 390 |
| ✗ | `RemoveLevel` | int | 12 | -1 … -1 |
| ✗ | `HasCollision` | int | 12 | -1 … 1 |
| ✓ | `InitiallyVisible` | int | 12 | -1 … -1 |
| ✗ | `CanMove` | int | 12 | 1 … 1 |
| ✗ | `SecondPass` | int | 12 | 0 … 0 |
| ✗ | `PickupLink` | str | 10 | "none" |
| ✓ | `PatrolPoint` | str | 12 | "Y1", "cell01", "lastg01", "none", … |
| ✗ | `VisibleRange` | float | 12 | 800 … 2.5e+03 |
| ✗ | `FOV` | float | 12 | 90 … 359 |
| ✗ | `TargetName` | str | 12 | "JIM1", "none" |
| ✗ | `AIState` | int | 12 | 1 … 6 |
| ✗ | `WanderRange` | float | 12 | 500 … 1.5e+03 |

### `3STA`  — 12 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 12 | "STALAG05", "stalag01", "stalag04", "stalag08", … |
| ✗ | `RotateToDest` | flag4 | 12 | 01010100 |
| ✗ | `ObjectID` | int | 12 | 861099073 … 861099073 |
| ✓ | `PositionX` | float | 12 | -5.87e+04 … 4.62e+04 |
| ✓ | `PositionY` | float | 12 | -2.33e+03 … 3.3e+03 |
| ✓ | `PositionZ` | float | 12 | -3.37e+04 … 3.79e+04 |
| ✓ | `RotationX` | float | 12 | 0 … 0 |
| ✓ | `RotationY` | float | 12 | 0 … 0 |
| ✓ | `RotationZ` | float | 12 | 0 … 0 |
| ✗ | `TaskName` | str | 12 | "none" |
| ✗ | `Debug` | int | 12 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 12 | -1 … -1 |
| ✗ | `ExactLevel` | int | 12 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 12 | -1 … -1 |
| ✗ | `HasCollision` | int | 12 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 12 | -1 … -1 |
| ✗ | `CanMove` | int | 12 | 1 … 1 |
| ✗ | `SecondPass` | int | 12 | 0 … 0 |
| ✗ | `PickupLink` | str | 6 | "none" |

### `3YSH`  — 11 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 11 | "C3DYOKIANSHIP", "SHIP1", "sh2", "yokship1", … |
| ✗ | `RotateToDest` | flag4 | 11 | 01010101 |
| ✗ | `ObjectID` | int | 11 | 861492040 … 861492040 |
| ✓ | `PositionX` | float | 11 | -3.34e+03 … 1.91e+04 |
| ✓ | `PositionY` | float | 11 | 2.03e+03 … 2.66e+04 |
| ✓ | `PositionZ` | float | 11 | -2.17e+04 … 8.05e+04 |
| ✓ | `RotationX` | float | 11 | 0 … 0 |
| ✓ | `RotationY` | float | 11 | 0 … 270 |
| ✓ | `RotationZ` | float | 11 | 0 … 0 |
| ✗ | `TaskName` | str | 11 | "none", "scene" |
| ✗ | `Debug` | int | 11 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 11 | -1 … 380 |
| ✗ | `ExactLevel` | int | 11 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 11 | -1 … -1 |
| ✗ | `HasCollision` | int | 11 | -1 … 1 |
| ✓ | `InitiallyVisible` | int | 11 | -1 … -1 |
| ✗ | `CanMove` | int | 11 | 1 … 1 |
| ✗ | `SecondPass` | int | 11 | 0 … 0 |
| ✗ | `PickupLink` | str | 11 | "none" |
| ✓ | `PatrolPoint` | str | 11 | "SHIP1PT", "SHIP2PT", "SHIP3PT", "SHIP6PT", … |
| ✗ | `VisibleRange` | float | 11 | 100 … 2.5e+03 |
| ✗ | `FOV` | float | 11 | 90 … 90 |
| ✗ | `TargetName` | str | 11 | "JIM1" |
| ✗ | `AIState` | int | 11 | 3 … 3 |
| ✗ | `WanderRange` | float | 11 | 1.5e+03 … 1.5e+03 |

### `3YCA`  — 11 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 11 | "C3DYOKCARGO", "CARGOSHIP1", "yoke", "yokecarl", … |
| ✗ | `RotateToDest` | flag4 | 11 | 01010101 |
| ✗ | `ObjectID` | int | 11 | 861487937 … 861487937 |
| ✓ | `PositionX` | float | 11 | -3.51e+03 … 4.31e+03 |
| ✓ | `PositionY` | float | 11 | -5.43e+03 … 2.45e+04 |
| ✓ | `PositionZ` | float | 11 | -4.51e+04 … 8.17e+04 |
| ✓ | `RotationX` | float | 11 | 0 … 0 |
| ✓ | `RotationY` | float | 11 | 0 … 270 |
| ✓ | `RotationZ` | float | 11 | 0 … 0 |
| ✗ | `TaskName` | str | 11 | "none", "scene" |
| ✗ | `Debug` | int | 11 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 11 | -1 … 480 |
| ✗ | `ExactLevel` | int | 11 | -1 … 410 |
| ✗ | `RemoveLevel` | int | 11 | -1 … 500 |
| ✗ | `HasCollision` | int | 11 | -1 … 1 |
| ✓ | `InitiallyVisible` | int | 11 | -1 … 1 |
| ✗ | `CanMove` | int | 11 | 0 … 1 |
| ✗ | `SecondPass` | int | 11 | 0 … 0 |
| ✗ | `PickupLink` | str | 9 | "none" |
| ✓ | `PatrolPoint` | str | 11 | "SHIP4PT", "SHIP5PT", "cargo1", "none", … |
| ✗ | `VisibleRange` | float | 11 | 100 … 1e+04 |
| ✗ | `FOV` | float | 11 | 0 … 359 |
| ✗ | `TargetName` | str | 11 | "JIM1", "none" |
| ✗ | `AIState` | int | 11 | 1 … 3 |
| ✗ | `WanderRange` | float | 11 | 1.5e+03 … 1.5e+03 |

### `3HUM`  — 10 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 10 | "C3DHUMPHREY", "clone1", "clone2", "clone4", … |
| ✗ | `RotateToDest` | flag4 | 10 | 01010101 |
| ✗ | `ObjectID` | int | 10 | 860378445 … 860378445 |
| ✓ | `PositionX` | float | 10 | -6.18e+03 … 8.06e+03 |
| ✓ | `PositionY` | float | 10 | -6.77 … 922 |
| ✓ | `PositionZ` | float | 10 | -5.27e+03 … 6.51e+03 |
| ✓ | `RotationX` | float | 10 | 0 … 0 |
| ✓ | `RotationY` | float | 10 | 0 … 220 |
| ✓ | `RotationZ` | float | 10 | 0 … 0 |
| ✗ | `TaskName` | str | 10 | "clone", "scene" |
| ✗ | `Debug` | int | 10 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 10 | 0 … 260 |
| ✗ | `ExactLevel` | int | 10 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 10 | -1 … 300 |
| ✗ | `HasCollision` | int | 10 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 10 | -1 … -1 |
| ✗ | `CanMove` | int | 10 | 1 … 1 |
| ✗ | `SecondPass` | int | 10 | 0 … 0 |
| ✗ | `PickupLink` | str | 7 | "none" |
| ✓ | `PatrolPoint` | str | 10 | "GETOUT1", "none" |
| ✗ | `VisibleRange` | float | 10 | 825 … 2.5e+03 |
| ✗ | `FOV` | float | 10 | 90 … 359 |
| ✗ | `TargetName` | str | 10 | "JIM1" |
| ✗ | `AIState` | int | 10 | 1 … 6 |
| ✗ | `WanderRange` | float | 10 | 1.5e+03 … 2e+03 |

### `3CAR`  — 9 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 9 | "C3DCARL", "Carl3", "Carl4" |
| ✗ | `RotateToDest` | flag4 | 9 | 01010101 |
| ✗ | `ObjectID` | int | 9 | 860045650 … 860045650 |
| ✓ | `PositionX` | float | 9 | -4.38e+03 … 6.9e+03 |
| ✓ | `PositionY` | float | 9 | -6.11e+03 … 1.79e+03 |
| ✓ | `PositionZ` | float | 9 | -3.91e+04 … 5.28e+03 |
| ✓ | `RotationX` | float | 9 | 0 … 0 |
| ✓ | `RotationY` | float | 9 | 0 … 300 |
| ✓ | `RotationZ` | float | 9 | 0 … 0 |
| ✗ | `TaskName` | str | 9 | "SCENE", "Scene", "scene" |
| ✗ | `Debug` | int | 9 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 9 | -1 … 380 |
| ✗ | `ExactLevel` | int | 9 | -1 … 390 |
| ✗ | `RemoveLevel` | int | 9 | -1 … 400 |
| ✗ | `HasCollision` | int | 9 | 0 … 1 |
| ✓ | `InitiallyVisible` | int | 9 | -1 … -1 |
| ✗ | `CanMove` | int | 9 | 1 … 1 |
| ✗ | `SecondPass` | int | 9 | 0 … 0 |
| ✗ | `PickupLink` | str | 8 | "none" |
| ✓ | `PatrolPoint` | str | 9 | "CARL1", "carl1", "crl1", "look1", … |
| ✗ | `VisibleRange` | float | 9 | 500 … 750 |
| ✗ | `FOV` | float | 9 | 90 … 359 |
| ✗ | `TargetName` | str | 9 | "JIM1", "Jim1" |
| ✗ | `AIState` | int | 9 | 1 … 9 |
| ✗ | `WanderRange` | float | 9 | -1 … 1.5e+03 |
| ✗ | `TalkState0` | int | 9 | 0 … 380 |
| ✗ | `TalkTrigger0` | str | 9 | "getstaken", "inhaler", "neutron1a", "neutron1b", … |
| ✗ | `TalkState1` | int | 9 | -1 … 70 |
| ✗ | `TalkTrigger1` | str | 9 | "neutron1c", "none" |
| ✗ | `TalkState2` | int | 9 | -1 … -1 |
| ✗ | `TalkTrigger2` | str | 9 | "none" |
| ✗ | `TalkState3` | int | 9 | -1 … -1 |
| ✗ | `TalkTrigger3` | str | 9 | "none" |
| ✗ | `TalkState4` | int | 9 | -1 … -1 |
| ✗ | `TalkTrigger4` | str | 9 | "none" |

### `3RCK`  — 9 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 9 | "rocket", "rocket2", "rocket3" |
| ✗ | `RotateToDest` | flag4 | 9 | 01010101 |
| ✗ | `ObjectID` | int | 9 | 861029195 … 861029195 |
| ✓ | `PositionX` | float | 9 | -2.3e+03 … 1.08e+04 |
| ✓ | `PositionY` | float | 9 | -1.42e+03 … 2.31e+03 |
| ✓ | `PositionZ` | float | 9 | -5.21e+03 … 1.25e+04 |
| ✓ | `RotationX` | float | 9 | 0 … 20 |
| ✓ | `RotationY` | float | 9 | 0 … 300 |
| ✓ | `RotationZ` | float | 9 | 0 … 70 |
| ✗ | `TaskName` | str | 9 | "scene" |
| ✗ | `Debug` | int | 9 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 9 | -1 … -1 |
| ✗ | `ExactLevel` | int | 9 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 9 | -1 … -1 |
| ✗ | `HasCollision` | int | 9 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 9 | -1 … 0 |
| ✗ | `CanMove` | int | 9 | 1 … 1 |
| ✗ | `SecondPass` | int | 9 | 0 … 0 |
| ✗ | `PickupLink` | str | 8 | "none" |
| ✓ | `PatrolPoint` | str | 9 | "rc1", "rock1", "rock1b", "rock3a" |
| ✗ | `VisibleRange` | float | 9 | 2.5e+03 … 2.5e+03 |
| ✗ | `FOV` | float | 9 | 90 … 350 |
| ✗ | `TargetName` | str | 9 | "JIM1" |
| ✗ | `AIState` | int | 9 | 1 … 3 |
| ✗ | `WanderRange` | float | 9 | 1.5e+03 … 1.5e+03 |

### `3GEY`  — 9 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 9 | "C3DGEYSER" |
| ✗ | `RotateToDest` | flag4 | 9 | 01010100 |
| ✗ | `ObjectID` | int | 9 | 860308825 … 860308825 |
| ✓ | `PositionX` | float | 9 | -6.37e+04 … 4.44e+04 |
| ✓ | `PositionY` | float | 9 | -816 … 6.43e+03 |
| ✓ | `PositionZ` | float | 9 | -1.63e+04 … 3.38e+04 |
| ✓ | `RotationX` | float | 9 | 90 … 270 |
| ✓ | `RotationY` | float | 9 | 0 … 0 |
| ✓ | `RotationZ` | float | 9 | 0 … 0 |
| ✗ | `TaskName` | str | 9 | "none" |
| ✗ | `Debug` | int | 9 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 9 | -1 … -1 |
| ✗ | `ExactLevel` | int | 9 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 9 | -1 … -1 |
| ✗ | `HasCollision` | int | 9 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 9 | -1 … -1 |
| ✗ | `CanMove` | int | 9 | 1 … 1 |
| ✗ | `SecondPass` | int | 9 | 0 … 0 |
| ✗ | `PickupLink` | str | 9 | "none" |
| ✗ | `SteamPeriod` | float | 9 | 2 … 6 |

### `3TRO`  — 9 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 9 | "C3DTROPHY", "trophy" |
| ✗ | `RotateToDest` | flag4 | 9 | 01010100 |
| ✗ | `ObjectID` | int | 9 | 861164111 … 861164111 |
| ✓ | `PositionX` | float | 9 | -3.53e+03 … 1.24e+04 |
| ✓ | `PositionY` | float | 9 | 6.17 … 6.33e+03 |
| ✓ | `PositionZ` | float | 9 | -1.14e+04 … 813 |
| ✓ | `RotationX` | float | 9 | 0 … 0 |
| ✓ | `RotationY` | float | 9 | 0 … 0 |
| ✓ | `RotationZ` | float | 9 | 0 … 0 |
| ✗ | `TaskName` | str | 9 | "none", "scene" |
| ✗ | `Debug` | int | 9 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 8 | -1 … -1 |
| ✗ | `ExactLevel` | int | 8 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 8 | -1 … -1 |
| ✗ | `HasCollision` | int | 8 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 8 | -1 … 0 |
| ✗ | `CanMove` | int | 7 | 0 … 1 |
| ✗ | `SecondPass` | int | 7 | 0 … 0 |
| ✗ | `PickupLink` | str | 3 | "none" |

### `3LAS`  — 8 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 8 | "C3DLASERTRIGGER" |
| ✗ | `RotateToDest` | flag4 | 8 | 01010100 |
| ✗ | `ObjectID` | int | 8 | 860635475 … 860635475 |
| ✓ | `PositionX` | float | 8 | -1.57e+04 … -363 |
| ✓ | `PositionY` | float | 8 | -30.4 … 902 |
| ✓ | `PositionZ` | float | 8 | -4.32e+03 … 5.61e+03 |
| ✓ | `RotationX` | float | 8 | 0 … 0 |
| ✓ | `RotationY` | float | 8 | 0 … 270 |
| ✓ | `RotationZ` | float | 8 | 0 … 0 |
| ✗ | `TaskName` | str | 8 | "none", "scene" |
| ✗ | `Debug` | int | 8 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 8 | -1 … 0 |
| ✗ | `ExactLevel` | int | 8 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 8 | -1 … -1 |
| ✗ | `HasCollision` | int | 8 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 8 | -1 … -1 |
| ✗ | `CanMove` | int | 8 | 1 … 1 |
| ✗ | `SecondPass` | int | 8 | 0 … 0 |
| ✗ | `ItemActive` | int | 8 | 1 … 1 |
| ✗ | `Next` | str | 8 | "door005", "halldoor01", "none", "tesla2", … |
| ✗ | `Toggle` | int | 8 | -1 … 1 |
| ✗ | `PickupLink` | str | 7 | "none" |

### `3MUS`  — 8 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 8 | "C3DMUSICTRIGGER", "downbeat", "music", "racemusic" |
| ✗ | `RotateToDest` | flag4 | 8 | 01010100 |
| ✗ | `ObjectID` | int | 8 | 860706131 … 860706131 |
| ✓ | `PositionX` | float | 8 | -590 … 3.58e+04 |
| ✓ | `PositionY` | float | 8 | -3.93e+03 … 357 |
| ✓ | `PositionZ` | float | 8 | -3.78e+04 … 2.83e+04 |
| ✓ | `RotationX` | float | 8 | 0 … 0 |
| ✓ | `RotationY` | float | 8 | 0 … 0 |
| ✓ | `RotationZ` | float | 8 | 0 … 0 |
| ✗ | `TaskName` | str | 8 | "scene" |
| ✗ | `Debug` | int | 8 | 0 … 0 |
| ✓ | `SpriteSize` | int | 8 | 50 … 50 |
| ✓ | `SpriteDatabase` | str | 8 | "icons.omt" |
| ✓ | `SpriteIndex` | int | 8 | 11 … 11 |
| ✗ | `Toggle` | int | 8 | -1 … -1 |
| ✗ | `ToggleObject` | str | 8 | "none" |
| ✗ | `NextTrigger` | str | 8 | "default", "none" |
| ✗ | `FadeType` | int | 8 | -1 … -1 |
| ✗ | `FadeTime` | float | 8 | 1 … 1 |
| ✗ | `MusicDatabase` | str | 8 | "musicarea51.omt", "musicneighborhood.omt", "musicschoolrace.omt", "musicship.om |
| ✗ | `MusicIndex0` | int | 8 | -1 … 1 |
| ✗ | `MusicIndex1` | int | 8 | -1 … -1 |
| ✗ | `MusicIndex2` | int | 8 | -1 … 2 |
| ✗ | `MusicIndex3` | int | 8 | -1 … -1 |
| ✗ | `MusicIndex4` | int | 8 | -1 … 1 |
| ✗ | `TouchActivated` | int | 8 | 0 … 1 |
| ✗ | `Radius` | float | 8 | 1 … 500 |
| ✗ | `RequiredLevel` | int | 5 | 0 … 140 |
| ✗ | `ExactLevel` | int | 5 | 0 … 231 |
| ✗ | `RemoveLevel` | int | 5 | -1044667134 … 125 |

### `3SWI`  — 8 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 8 | "C3DSWITCH" |
| ✗ | `RotateToDest` | flag4 | 8 | 00010100, 01010100, 03010100 |
| ✗ | `ObjectID` | int | 8 | 861099849 … 861099849 |
| ✓ | `PositionX` | float | 8 | -1.32e+03 … 5.05e+03 |
| ✓ | `PositionY` | float | 8 | -2.98e+03 … 3.11e+03 |
| ✓ | `PositionZ` | float | 8 | -5.67e+03 … 1.53e+04 |
| ✓ | `RotationX` | float | 8 | 0 … 10 |
| ✓ | `RotationY` | float | 8 | 0 … 270 |
| ✓ | `RotationZ` | float | 8 | 0 … 0 |
| ✗ | `TaskName` | str | 8 | "none", "scene" |
| ✗ | `Debug` | int | 8 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 8 | 0 … 0 |
| ✗ | `ExactLevel` | int | 8 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 8 | -1 … -1 |
| ✗ | `HasCollision` | int | 8 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 8 | -1 … -1 |
| ✗ | `CanMove` | int | 8 | 1 … 1 |
| ✗ | `SecondPass` | int | 8 | 0 … 0 |
| ✗ | `PickupLink` | str | 8 | "none" |
| ✗ | `MyState` | int | 8 | 0 … 1 |
| ✗ | `SwitchObject` | str | 8 | "FAN1", "FAN2", "FAN4", "WIRE01", … |
| ✗ | `Toggle` | int | 8 | 0 … 0 |

### `3SWN`  — 7 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 7 | "C3DDOORSWING", "ldoor", "mummydoor" |
| ✗ | `RotateToDest` | flag4 | 7 | 01010100 |
| ✗ | `ObjectID` | int | 7 | 861099854 … 861099854 |
| ✓ | `PositionX` | float | 7 | -4.61e+03 … 4.8e+03 |
| ✓ | `PositionY` | float | 7 | -5.49 … 890 |
| ✓ | `PositionZ` | float | 7 | -4.12e+03 … 8.03e+03 |
| ✓ | `RotationX` | float | 7 | 0 … 0 |
| ✓ | `RotationY` | float | 7 | 0 … 270 |
| ✓ | `RotationZ` | float | 7 | 0 … 0 |
| ✗ | `TaskName` | str | 7 | "scene" |
| ✗ | `Debug` | int | 7 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 7 | -1 … 115 |
| ✗ | `ExactLevel` | int | 7 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 7 | -1 … -1 |
| ✗ | `HasCollision` | int | 7 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 7 | -1 … -1 |
| ✗ | `CanMove` | int | 7 | 1 … 1 |
| ✗ | `SecondPass` | int | 7 | 0 … 0 |
| ✗ | `PickupLink` | str | 5 | "none" |
| ✗ | `ASEFile` | str | 7 | "blocksdoor.ase", "door2a.ase", "doorretro.ase", "downdoor2a.ase", … |
| ✓ | `PNGFile` | str | 7 | "blocksdoor.png", "doorfowl.png", "pyramiddoor1.png", "showmedoor.png", … |
| ✗ | `TimeToOpen` | float | 7 | 1.8 … 1.8 |
| ✗ | `OpenSpeed` | float | 7 | 50 … 50 |
| ✗ | `TouchActivated` | int | 7 | 0 … 1 |

### `3AIO`  — 7 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 7 | "C3DAIOMTOBJ", "crashpod", "friedeggs", "pod", … |
| ✗ | `RotateToDest` | flag4 | 7 | 01010101 |
| ✗ | `ObjectID` | int | 7 | 859916623 … 859916623 |
| ✓ | `PositionX` | float | 7 | -1.55e+04 … 1.58e+04 |
| ✓ | `PositionY` | float | 7 | -12.8 … 2.63e+03 |
| ✓ | `PositionZ` | float | 7 | -4.58e+03 … 1.32e+04 |
| ✓ | `RotationX` | float | 7 | 0 … 359 |
| ✓ | `RotationY` | float | 7 | 0 … 270 |
| ✓ | `RotationZ` | float | 7 | 0 … 0.5 |
| ✗ | `TaskName` | str | 7 | "scene" |
| ✗ | `Debug` | int | 7 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 7 | -1 … 190 |
| ✗ | `ExactLevel` | int | 7 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 7 | -1 … 320 |
| ✗ | `HasCollision` | int | 7 | 0 … 1 |
| ✓ | `InitiallyVisible` | int | 7 | -1 … 0 |
| ✗ | `CanMove` | int | 7 | 0 … 1 |
| ✗ | `SecondPass` | int | 7 | 0 … 0 |
| ✗ | `PickupLink` | str | 6 | "none" |
| ✓ | `PatrolPoint` | str | 7 | "fallpod01", "none", "pod01", "wop04" |
| ✗ | `VisibleRange` | float | 7 | 2.5e+03 … 2.5e+03 |
| ✗ | `FOV` | float | 7 | 90 … 90 |
| ✗ | `TargetName` | str | 7 | "JIM1", "none" |
| ✗ | `AIState` | int | 7 | 1 … 3 |
| ✗ | `WanderRange` | float | 7 | 1.5e+03 … 1.5e+03 |
| ✗ | `OmtDatabase` | str | 7 | "objects.omt" |
| ✗ | `OmtIndex` | int | 7 | 6 … 30 |
| ✗ | `Radius` | float | 7 | 17.4 … 1.1e+03 |
| ✗ | `TerrainColl` | int | 7 | -1 … 0 |

### `3FUE`  — 7 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 7 | "C3DROCKETFUEL" |
| ✗ | `RotateToDest` | flag4 | 7 | 00010100, 04010100, 82010100 |
| ✗ | `ObjectID` | int | 7 | 860247365 … 860247365 |
| ✓ | `PositionX` | float | 7 | 2.22e+03 … 3.01e+03 |
| ✓ | `PositionY` | float | 7 | 2.94e+03 … 2.95e+03 |
| ✓ | `PositionZ` | float | 7 | 7.78e+03 … 8.62e+03 |
| ✓ | `RotationX` | float | 7 | 0 … 0 |
| ✓ | `RotationY` | float | 7 | 0 … 0 |
| ✓ | `RotationZ` | float | 7 | 0 … 0 |
| ✗ | `TaskName` | str | 7 | "none" |
| ✗ | `Debug` | int | 7 | 0 … 0 |
| ✓ | `SpriteSize` | int | 7 | 50 … 50 |
| ✓ | `SpriteDatabase` | str | 7 | "sprites.omt" |
| ✓ | `SpriteIndex` | int | 7 | 40 … 40 |

### `3ANI`  — 6 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 6 | "C3DANIMATEDSPRITE", "apple", "bottles01", "bottles02", … |
| ✗ | `RotateToDest` | flag4 | 6 | 01010100 |
| ✗ | `ObjectID` | int | 6 | 859917897 … 859917897 |
| ✓ | `PositionX` | float | 6 | -2.12e+03 … 5.46e+03 |
| ✓ | `PositionY` | float | 6 | -473 … 226 |
| ✓ | `PositionZ` | float | 6 | -393 … 2.34e+03 |
| ✓ | `RotationX` | float | 6 | 0 … 0 |
| ✓ | `RotationY` | float | 6 | 0 … 0 |
| ✓ | `RotationZ` | float | 6 | 0 … 0 |
| ✗ | `TaskName` | str | 6 | "scene" |
| ✗ | `Debug` | int | 6 | 0 … 0 |
| ✓ | `SpriteSize` | int | 6 | 75 … 150 |
| ✓ | `SpriteDatabase` | str | 6 | "sprites.omt" |
| ✓ | `SpriteIndex` | int | 6 | -1 … 177 |
| ✗ | `Toggle` | int | 6 | -1 … 1 |
| ✗ | `ToggleObject` | str | 6 | "bottleticket", "none" |
| ✗ | `NextTrigger` | str | 6 | "none" |
| ✗ | `FadeType` | int | 6 | -1 … -1 |
| ✗ | `FadeTime` | float | 6 | 1 … 1 |
| ✗ | `PickupIndex` | int | 6 | 306 … 1109 |
| ✗ | `PIC_NUMBER` | int | 6 | -1 … -1 |
| ✗ | `RequiredLevel` | int | 6 | 0 … 0 |
| ✗ | `ExactLevel` | int | 6 | -1 … 30 |
| ✗ | `Alpha` | float | 6 | 1 … 1 |
| ✗ | `Red` | float | 6 | 1 … 1 |
| ✗ | `Green` | float | 6 | 1 … 1 |
| ✗ | `Blue` | float | 6 | 1 … 1 |
| ✗ | `Activated` | int | 6 | 0 … 1 |
| ✗ | `OnSoundIndex` | int | 6 | -1 … 185 |
| ✗ | `OffSoundIndex` | int | 6 | -1 … 214 |
| ✗ | `FPS` | float | 6 | 3 … 10 |
| ✗ | `Loop` | int | 6 | 0 … 2 |
| ✗ | `Sprite1` | int | 6 | -1 … 177 |
| ✗ | `Sprite2` | int | 6 | -1 … 178 |
| ✗ | `Sprite3` | int | 6 | -1 … 179 |
| ✗ | `Sprite4` | int | 6 | -1 … 182 |
| ✗ | `Sprite5` | int | 6 | -1 … 180 |
| ✗ | `Sprite6` | int | 6 | -1 … 180 |
| ✗ | `Sprite7` | int | 6 | -1 … 180 |
| ✗ | `Sprite8` | int | 6 | -1 … 182 |
| ✗ | `Sprite9` | int | 6 | -1 … 177 |
| ✗ | `InitallyVisible` | int | 6 | -1 … 1 |

### `3SCD`  — 6 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 6 | "C3DSCHOOLDOOR", "fowlroom" |
| ✗ | `RotateToDest` | flag4 | 6 | 01010100 |
| ✗ | `ObjectID` | int | 6 | 861094724 … 861094724 |
| ✓ | `PositionX` | float | 6 | -1.2e+03 … 533 |
| ✓ | `PositionY` | float | 6 | 0 … 10.3 |
| ✓ | `PositionZ` | float | 6 | -1.31e+03 … 2.85e+03 |
| ✓ | `RotationX` | float | 6 | 0 … 0 |
| ✓ | `RotationY` | float | 6 | 0 … 180 |
| ✓ | `RotationZ` | float | 6 | 0 … 0 |
| ✗ | `TaskName` | str | 6 | "none", "scene" |
| ✗ | `Debug` | int | 6 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 6 | -1 … 0 |
| ✗ | `ExactLevel` | int | 6 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 6 | -1 … -1 |
| ✗ | `HasCollision` | int | 6 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 6 | -1 … -1 |
| ✗ | `CanMove` | int | 6 | 1 … 1 |
| ✗ | `SecondPass` | int | 6 | 0 … 0 |
| ✗ | `PickupLink` | str | 6 | "none" |
| ✗ | `ASEFile` | str | 6 | "doorfowl.ase", "doorretro.ase", "firedoor.ase" |
| ✓ | `PNGFile` | str | 6 | "doorfowl.png", "exit.png", "firedoor.png", "retrodoor.png" |

### `3TRC`  — 6 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 6 | "BEAM", "C3DTRACTORBEAM", "beam" |
| ✗ | `RotateToDest` | flag4 | 6 | 01010100 |
| ✗ | `ObjectID` | int | 6 | 861164099 … 861164099 |
| ✓ | `PositionX` | float | 6 | -3.15e+04 … 3.2e+03 |
| ✓ | `PositionY` | float | 6 | 3.39e+03 … 5.08e+03 |
| ✓ | `PositionZ` | float | 6 | -2.64e+04 … 1.14e+04 |
| ✓ | `RotationX` | float | 6 | 0 … 310 |
| ✓ | `RotationY` | float | 6 | 0 … 270 |
| ✓ | `RotationZ` | float | 6 | 0 … 110 |
| ✗ | `TaskName` | str | 6 | "scene" |
| ✗ | `Debug` | int | 6 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 6 | -1 … 475 |
| ✗ | `ExactLevel` | int | 6 | -1 … 0 |
| ✗ | `RemoveLevel` | int | 6 | -1 … 480 |
| ✗ | `HasCollision` | int | 6 | -1 … 1 |
| ✓ | `InitiallyVisible` | int | 6 | -1 … 0 |
| ✗ | `CanMove` | int | 6 | 0 … 1 |
| ✗ | `SecondPass` | int | 6 | 0 … 0 |
| ✗ | `PickupLink` | str | 5 | "none" |

### `3CIN`  — 6 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 6 | "C3DCINDY" |
| ✗ | `RotateToDest` | flag4 | 6 | 00010101, 01010101 |
| ✗ | `ObjectID` | int | 6 | 860047694 … 860047694 |
| ✓ | `PositionX` | float | 6 | -2.3e+03 … 2.14e+03 |
| ✓ | `PositionY` | float | 6 | -4.15e+03 … 1.8e+03 |
| ✓ | `PositionZ` | float | 6 | -417 … 4.94e+03 |
| ✓ | `RotationX` | float | 6 | 0 … 360 |
| ✓ | `RotationY` | float | 6 | 0 … 200 |
| ✓ | `RotationZ` | float | 6 | 0 … 0.422 |
| ✗ | `TaskName` | str | 6 | "Scene", "scene" |
| ✗ | `Debug` | int | 6 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 6 | -1 … 0 |
| ✗ | `ExactLevel` | int | 6 | -1 … 420 |
| ✗ | `RemoveLevel` | int | 6 | -1 … 420 |
| ✗ | `HasCollision` | int | 6 | 0 … 1 |
| ✓ | `InitiallyVisible` | int | 6 | -1 … -1 |
| ✗ | `CanMove` | int | 6 | 1 … 1 |
| ✗ | `SecondPass` | int | 6 | 0 … 0 |
| ✓ | `PatrolPoint` | str | 6 | "CINDY1", "c1", "cin2", "none" |
| ✗ | `VisibleRange` | float | 6 | 500 … 1e+03 |
| ✗ | `FOV` | float | 6 | 90 … 359 |
| ✗ | `TargetName` | str | 6 | "JIM1", "none" |
| ✗ | `AIState` | int | 6 | 1 … 6 |
| ✗ | `WanderRange` | float | 6 | 1.5e+03 … 1.5e+03 |
| ✗ | `TalkState0` | int | 6 | 0 … 410 |
| ✗ | `TalkTrigger0` | str | 6 | "getclaw", "needpasscard", "none" |
| ✗ | `TalkState1` | int | 6 | -1 … -1 |
| ✗ | `TalkTrigger1` | str | 6 | "none" |
| ✗ | `TalkState2` | int | 6 | -1 … -1 |
| ✗ | `TalkTrigger2` | str | 6 | "none" |
| ✗ | `TalkState3` | int | 6 | -1 … -1 |
| ✗ | `TalkTrigger3` | str | 6 | "none" |
| ✗ | `TalkState4` | int | 6 | -1 … -1 |
| ✗ | `TalkTrigger4` | str | 6 | "none" |
| ✗ | `PickupLink` | str | 5 | "none" |

### `3FAN`  — 6 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 6 | "FAN1", "FAN2", "FAN4", "labfan", … |
| ✗ | `RotateToDest` | flag4 | 6 | 00010100, 01010100, 29010100 |
| ✗ | `ObjectID` | int | 6 | 860242254 … 860242254 |
| ✓ | `PositionX` | float | 6 | -802 … 5.49e+03 |
| ✓ | `PositionY` | float | 6 | -2.35e+03 … 3.65e+03 |
| ✓ | `PositionZ` | float | 6 | 1.78e+03 … 1.48e+04 |
| ✓ | `RotationX` | float | 6 | 0 … 0 |
| ✓ | `RotationY` | float | 6 | 0 … 270 |
| ✓ | `RotationZ` | float | 6 | 0 … 0 |
| ✗ | `TaskName` | str | 6 | "none" |
| ✗ | `Debug` | int | 6 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 6 | 0 … 0 |
| ✗ | `ExactLevel` | int | 6 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 6 | -1 … -1 |
| ✗ | `HasCollision` | int | 6 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 6 | -1 … -1 |
| ✗ | `CanMove` | int | 6 | 1 … 1 |
| ✗ | `SecondPass` | int | 6 | 0 … 0 |
| ✗ | `PickupLink` | str | 6 | "none" |
| ✗ | `FanSpeed` | float | 6 | 800 … 2.7e+03 |
| ✗ | `FanRange` | float | 6 | 1e+03 … 3.5e+03 |
| ✗ | `FanOn` | int | 6 | 1 … 1 |

### `3GIR`  — 5 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 5 | "C3DGIRLEATINGPLANT", "cd", "df", "libbyplant" |
| ✗ | `RotateToDest` | flag4 | 5 | 01010101 |
| ✗ | `ObjectID` | int | 5 | 860309842 … 860309842 |
| ✓ | `PositionX` | float | 5 | 2.62e+03 … 1.1e+04 |
| ✓ | `PositionY` | float | 5 | -583 … 9.96 |
| ✓ | `PositionZ` | float | 5 | -328 … 1.15e+04 |
| ✓ | `RotationX` | float | 5 | 0 … 0 |
| ✓ | `RotationY` | float | 5 | 0 … 187 |
| ✓ | `RotationZ` | float | 5 | 0 … 0 |
| ✗ | `TaskName` | str | 5 | "none", "scene" |
| ✗ | `Debug` | int | 5 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 5 | 0 … 255 |
| ✗ | `ExactLevel` | int | 5 | -1 … 250 |
| ✗ | `RemoveLevel` | int | 5 | -1 … 280 |
| ✗ | `HasCollision` | int | 5 | 0 … 1 |
| ✓ | `InitiallyVisible` | int | 5 | -1 … -1 |
| ✗ | `CanMove` | int | 5 | 1 … 1 |
| ✗ | `SecondPass` | int | 5 | 0 … 0 |
| ✗ | `PickupLink` | str | 5 | "none" |
| ✓ | `PatrolPoint` | str | 5 | "gep1", "gep2", "gep3", "none" |
| ✗ | `VisibleRange` | float | 5 | 100 … 2e+03 |
| ✗ | `FOV` | float | 5 | 90 … 120 |
| ✗ | `TargetName` | str | 5 | "JIM1", "c3dlibby" |
| ✗ | `AIState` | int | 5 | 0 … 10 |
| ✗ | `WanderRange` | float | 5 | 1.5e+03 … 2e+03 |
| ✗ | `PickupIndex` | int | 5 | 302 … 816 |
| ✗ | `PIC_NUMBER` | int | 5 | 2 … 2 |

### `3SUV`  — 5 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 5 | "C3DSUV" |
| ✗ | `RotateToDest` | flag4 | 5 | 00010101, 01010101, 29010101 |
| ✗ | `ObjectID` | int | 5 | 861099350 … 861099350 |
| ✓ | `PositionX` | float | 5 | 828 … 1.62e+04 |
| ✓ | `PositionY` | float | 5 | -776 … 128 |
| ✓ | `PositionZ` | float | 5 | -1.19e+04 … 1.4e+04 |
| ✓ | `RotationX` | float | 5 | 0 … 0 |
| ✓ | `RotationY` | float | 5 | 0 … 120 |
| ✓ | `RotationZ` | float | 5 | 0 … 0 |
| ✗ | `TaskName` | str | 5 | "none", "scene" |
| ✗ | `Debug` | int | 5 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 5 | 0 … 1 |
| ✗ | `ExactLevel` | int | 5 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 5 | -1 … 320 |
| ✗ | `HasCollision` | int | 5 | -1 … 1 |
| ✓ | `InitiallyVisible` | int | 5 | -1 … -1 |
| ✗ | `CanMove` | int | 5 | 1 … 1 |
| ✗ | `SecondPass` | int | 5 | 0 … 0 |
| ✗ | `PickupLink` | str | 5 | "none" |
| ✓ | `PatrolPoint` | str | 5 | "MIB1", "PATA1", "none" |
| ✗ | `VisibleRange` | float | 5 | 2.5e+03 … 5e+03 |
| ✗ | `FOV` | float | 5 | 90 … 300 |
| ✗ | `TargetName` | str | 5 | "JIM1" |
| ✗ | `AIState` | int | 5 | 2 … 2 |
| ✗ | `WanderRange` | float | 5 | 1.5e+03 … 1.5e+03 |

### `3TES`  — 5 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 5 | "TESLA4", "tesla1", "tesla2", "tesla3", … |
| ✗ | `RotateToDest` | flag4 | 5 | 01010100 |
| ✗ | `ObjectID` | int | 5 | 861160787 … 861160787 |
| ✓ | `PositionX` | float | 5 | -1.34e+04 … -3e+03 |
| ✓ | `PositionY` | float | 5 | 740 … 750 |
| ✓ | `PositionZ` | float | 5 | -2.55e+03 … 6.41e+03 |
| ✓ | `RotationX` | float | 5 | 0 … 0 |
| ✓ | `RotationY` | float | 5 | 0 … 270 |
| ✓ | `RotationZ` | float | 5 | 0 … 0 |
| ✗ | `TaskName` | str | 5 | "none", "scene" |
| ✗ | `Debug` | int | 5 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 5 | -1 … -1 |
| ✗ | `ExactLevel` | int | 5 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 5 | -1 … -1 |
| ✗ | `HasCollision` | int | 5 | -1 … 0 |
| ✓ | `InitiallyVisible` | int | 5 | -1 … -1 |
| ✗ | `CanMove` | int | 5 | 0 … 1 |
| ✗ | `SecondPass` | int | 5 | 0 … 1 |
| ✗ | `PickupLink` | str | 5 | "none" |
| ✗ | `ItemActive` | int | 5 | 0 … 1 |

### `3SHE`  — 4 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 4 | "C3DSHEEN", "Sheen1", "Sheen2", "sheen3" |
| ✗ | `RotateToDest` | flag4 | 4 | 00010101, 01010101 |
| ✗ | `ObjectID` | int | 4 | 861096005 … 861096005 |
| ✓ | `PositionX` | float | 4 | -139 … 8.97e+03 |
| ✓ | `PositionY` | float | 4 | 2.54 … 89.7 |
| ✓ | `PositionZ` | float | 4 | -698 … 6.99e+03 |
| ✓ | `RotationX` | float | 4 | 0 … 0.0413 |
| ✓ | `RotationY` | float | 4 | 0 … 160 |
| ✓ | `RotationZ` | float | 4 | 0 … 0.411 |
| ✗ | `TaskName` | str | 4 | "Scene", "scene" |
| ✗ | `Debug` | int | 4 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 4 | 0 … 360 |
| ✗ | `ExactLevel` | int | 4 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 4 | -1 … 380 |
| ✗ | `HasCollision` | int | 4 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 4 | -1 … -1 |
| ✗ | `CanMove` | int | 4 | 1 … 1 |
| ✗ | `SecondPass` | int | 4 | 0 … 0 |
| ✗ | `PickupLink` | str | 3 | "none" |
| ✓ | `PatrolPoint` | str | 4 | "none", "shn1", "walks1" |
| ✗ | `VisibleRange` | float | 4 | 500 … 500 |
| ✗ | `FOV` | float | 4 | 90 … 359 |
| ✗ | `TargetName` | str | 4 | "JIM1" |
| ✗ | `AIState` | int | 4 | 1 … 2 |
| ✗ | `WanderRange` | float | 4 | 1.5e+03 … 1.5e+03 |
| ✗ | `TalkState0` | int | 4 | 0 … 360 |
| ✗ | `TalkTrigger0` | str | 4 | "exchange", "givetickets", "none", "sewerpart" |
| ✗ | `TalkState1` | int | 4 | -1 … -1 |
| ✗ | `TalkTrigger1` | str | 4 | "none" |
| ✗ | `TalkState2` | int | 4 | -1 … -1 |
| ✗ | `TalkTrigger2` | str | 4 | "none" |
| ✗ | `TalkState3` | int | 4 | -1 … -1 |
| ✗ | `TalkTrigger3` | str | 4 | "none" |
| ✗ | `TalkState4` | int | 4 | -1 … -1 |
| ✗ | `TalkTrigger4` | str | 4 | "none" |

### `3DAI`  — 4 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|

### `3FLE`  — 4 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 4 | "FLEETC", "goobar" |
| ✗ | `RotateToDest` | flag4 | 4 | 01010101 |
| ✗ | `ObjectID` | int | 4 | 860245061 … 860245061 |
| ✓ | `PositionX` | float | 4 | -1.67e+03 … 46.3 |
| ✓ | `PositionY` | float | 4 | -4.09e+03 … 1.47e+03 |
| ✓ | `PositionZ` | float | 4 | -3.36e+03 … 219 |
| ✓ | `RotationX` | float | 4 | 0 … 0 |
| ✓ | `RotationY` | float | 4 | 0 … 180 |
| ✓ | `RotationZ` | float | 4 | 0 … 0 |
| ✗ | `TaskName` | str | 4 | "scene" |
| ✗ | `Debug` | int | 4 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 4 | -1 … 0 |
| ✗ | `ExactLevel` | int | 4 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 4 | -1 … -1 |
| ✗ | `HasCollision` | int | 4 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 4 | -1 … -1 |
| ✗ | `CanMove` | int | 4 | 1 … 1 |
| ✗ | `SecondPass` | int | 4 | 0 … 0 |
| ✗ | `PickupLink` | str | 4 | "none" |
| ✓ | `PatrolPoint` | str | 4 | "fc1", "none" |
| ✗ | `VisibleRange` | float | 4 | 10 … 2.5e+03 |
| ✗ | `FOV` | float | 4 | 1 … 90 |
| ✗ | `TargetName` | str | 4 | "JIM1" |
| ✗ | `AIState` | int | 4 | 1 … 2 |
| ✗ | `WanderRange` | float | 4 | 1.5e+03 … 1.5e+03 |

### `3SPA`  — 4 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 4 | "C3DSPARKWIRE", "WIRE01", "wire01", "wire3" |
| ✗ | `RotateToDest` | flag4 | 4 | 01010100 |
| ✗ | `ObjectID` | int | 4 | 861098049 … 861098049 |
| ✓ | `PositionX` | float | 4 | -1.46e+03 … 7.41e+03 |
| ✓ | `PositionY` | float | 4 | -2.57e+03 … 2.26e+03 |
| ✓ | `PositionZ` | float | 4 | -1.3e+04 … 1.24e+04 |
| ✓ | `RotationX` | float | 4 | 0 … 0 |
| ✓ | `RotationY` | float | 4 | 0 … 270 |
| ✓ | `RotationZ` | float | 4 | 0 … 0 |
| ✗ | `TaskName` | str | 4 | "none", "scene" |
| ✗ | `Debug` | int | 4 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 4 | -1 … 0 |
| ✗ | `ExactLevel` | int | 4 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 4 | -1 … -1 |
| ✗ | `HasCollision` | int | 4 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 4 | -1 … -1 |
| ✗ | `CanMove` | int | 4 | 1 … 1 |
| ✗ | `SecondPass` | int | 4 | 0 … 0 |
| ✗ | `ItemActive` | int | 4 | 1 … 1 |
| ✗ | `PickupLink` | str | 3 | "none" |

### `3SBU`  — 4 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 4 | "C3DBUS" |
| ✗ | `RotateToDest` | flag4 | 4 | 01000101, 01010101 |
| ✗ | `ObjectID` | int | 4 | 861094485 … 861094485 |
| ✓ | `PositionX` | float | 4 | -1.27e+03 … -1.08e+03 |
| ✓ | `PositionY` | float | 4 | -1.22 … 80.5 |
| ✓ | `PositionZ` | float | 4 | -4.48e+03 … 2.73e+03 |
| ✓ | `RotationX` | float | 4 | 0 … 0 |
| ✓ | `RotationY` | float | 4 | 0 … 0 |
| ✓ | `RotationZ` | float | 4 | 0 … 0 |
| ✗ | `TaskName` | str | 4 | "DINO", "none" |
| ✗ | `Debug` | int | 4 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 2 | 0 … 0 |
| ✗ | `ExactLevel` | int | 2 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 2 | -1 … -1 |
| ✗ | `HasCollision` | int | 2 | -1 … 1 |
| ✓ | `InitiallyVisible` | int | 2 | -1 … -1 |
| ✗ | `CanMove` | int | 2 | 1 … 1 |
| ✗ | `SecondPass` | int | 2 | 0 … 0 |
| ✓ | `PatrolPoint` | str | 4 | "bus01", "none" |
| ✗ | `VisibleRange` | float | 4 | 2.5e+03 … 8e+03 |
| ✗ | `FOV` | float | 4 | 90 … 90 |
| ✗ | `TargetName` | str | 4 | "JIM1" |
| ✗ | `AIState` | int | 4 | 3 … 3 |
| ✗ | `WanderRange` | float | 2 | 1.5e+03 … 1.5e+03 |
| ✗ | `PickupLink` | str | 1 | "none" |
| ✗ | `TaskState` | int | 2 | 0 … 0 |

### `3NIC`  — 4 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 4 | "C3DNICK", "Nick1", "Nick2" |
| ✗ | `RotateToDest` | flag4 | 4 | 01010101, 18010101 |
| ✗ | `ObjectID` | int | 4 | 860768579 … 860768579 |
| ✓ | `PositionX` | float | 4 | -1.48e+03 … -184 |
| ✓ | `PositionY` | float | 4 | 1.8 … 337 |
| ✓ | `PositionZ` | float | 4 | -9.31e+03 … 970 |
| ✓ | `RotationX` | float | 4 | 0 … 0 |
| ✓ | `RotationY` | float | 4 | 0 … 290 |
| ✓ | `RotationZ` | float | 4 | 0 … 0.00228 |
| ✗ | `TaskName` | str | 4 | "SCENE", "Scene", "scene" |
| ✗ | `Debug` | int | 4 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 4 | 0 … 140 |
| ✗ | `ExactLevel` | int | 4 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 4 | -1 … 162 |
| ✗ | `HasCollision` | int | 4 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 4 | -1 … -1 |
| ✗ | `CanMove` | int | 4 | 1 … 1 |
| ✗ | `SecondPass` | int | 4 | 0 … 0 |
| ✓ | `PatrolPoint` | str | 4 | "NICPAT1", "none" |
| ✗ | `VisibleRange` | float | 4 | 550 … 1e+03 |
| ✗ | `FOV` | float | 4 | 90 … 359 |
| ✗ | `TargetName` | str | 4 | "JIM1" |
| ✗ | `AIState` | int | 4 | 1 … 6 |
| ✗ | `WanderRange` | float | 4 | 1.5e+03 … 1.5e+03 |
| ✗ | `TalkState0` | int | 4 | 0 … 140 |
| ✗ | `TalkTrigger0` | str | 4 | "none", "race1", "race2" |
| ✗ | `TalkState1` | int | 4 | -1 … 150 |
| ✗ | `TalkTrigger1` | str | 4 | "loserace1", "none", "race2again" |
| ✗ | `TalkState2` | int | 4 | -1 … 160 |
| ✗ | `TalkTrigger2` | str | 4 | "none", "winrace1", "winrace2" |
| ✗ | `TalkState3` | int | 4 | -1 … 145 |
| ✗ | `TalkTrigger3` | str | 4 | "loserace1", "none", "race2again" |
| ✗ | `TalkState4` | int | 4 | -1 … 141 |
| ✗ | `TalkTrigger4` | str | 4 | "dirtrace", "none", "race2" |
| ✗ | `PickupLink` | str | 3 | "none" |

### `3FOW`  — 4 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 4 | "C3DFOWL", "fowl" |
| ✗ | `RotateToDest` | flag4 | 4 | 01010101 |
| ✗ | `ObjectID` | int | 4 | 860245847 … 860245847 |
| ✓ | `PositionX` | float | 4 | -2.4e+03 … 619 |
| ✓ | `PositionY` | float | 4 | 0.383 … 1.8e+03 |
| ✓ | `PositionZ` | float | 4 | -564 … 6.81e+03 |
| ✓ | `RotationX` | float | 4 | 0 … 0 |
| ✓ | `RotationY` | float | 4 | 140 … 220 |
| ✓ | `RotationZ` | float | 4 | 0 … 0 |
| ✗ | `TaskName` | str | 4 | "scene" |
| ✗ | `Debug` | int | 4 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 4 | 0 … 460 |
| ✗ | `ExactLevel` | int | 4 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 4 | -1 … 470 |
| ✗ | `HasCollision` | int | 4 | 0 … 1 |
| ✓ | `InitiallyVisible` | int | 4 | -1 … -1 |
| ✗ | `CanMove` | int | 4 | 1 … 1 |
| ✗ | `SecondPass` | int | 4 | 0 … 0 |
| ✗ | `PickupLink` | str | 4 | "none" |
| ✓ | `PatrolPoint` | str | 4 | "fowl1", "none" |
| ✗ | `VisibleRange` | float | 4 | 500 … 500 |
| ✗ | `FOV` | float | 4 | 90 … 359 |
| ✗ | `TargetName` | str | 4 | "JIM1" |
| ✗ | `AIState` | int | 4 | 1 … 6 |
| ✗ | `WanderRange` | float | 4 | 1.5e+03 … 1.5e+03 |
| ✗ | `TalkState0` | int | 4 | 0 … 460 |
| ✗ | `TalkTrigger0` | str | 4 | "none", "powerplant" |
| ✗ | `TalkState1` | int | 4 | -1 … -1 |
| ✗ | `TalkTrigger1` | str | 4 | "none" |
| ✗ | `TalkState2` | int | 4 | -1 … -1 |
| ✗ | `TalkTrigger2` | str | 4 | "none" |
| ✗ | `TalkState3` | int | 4 | -1 … -1 |
| ✗ | `TalkTrigger3` | str | 4 | "none" |
| ✗ | `TalkState4` | int | 4 | -1 … -1 |
| ✗ | `TalkTrigger4` | str | 4 | "none" |

### `3LIB`  — 3 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 3 | "C3DLIBBY", "libby2" |
| ✗ | `RotateToDest` | flag4 | 3 | 00010101, 01010101 |
| ✗ | `ObjectID` | int | 3 | 860637506 … 860637506 |
| ✓ | `PositionX` | float | 3 | -101 … 1.01e+04 |
| ✓ | `PositionY` | float | 3 | 4.71 … 36.5 |
| ✓ | `PositionZ` | float | 3 | -915 … 1.04e+03 |
| ✓ | `RotationX` | float | 3 | 0 … 0 |
| ✓ | `RotationY` | float | 3 | 0 … 187 |
| ✓ | `RotationZ` | float | 3 | 0 … 0 |
| ✗ | `TaskName` | str | 3 | "Scene", "scene" |
| ✗ | `Debug` | int | 3 | 0 … 1 |
| ✗ | `RequiredLevel` | int | 3 | 0 … 400 |
| ✗ | `ExactLevel` | int | 3 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 3 | -1 … 420 |
| ✗ | `HasCollision` | int | 3 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 3 | -1 … -1 |
| ✗ | `CanMove` | int | 3 | 1 … 1 |
| ✗ | `SecondPass` | int | 3 | 0 … 0 |
| ✗ | `PickupLink` | str | 3 | "none" |
| ✓ | `PatrolPoint` | str | 3 | "Lib1", "lib1", "none" |
| ✗ | `VisibleRange` | float | 3 | 500 … 700 |
| ✗ | `FOV` | float | 3 | 90 … 359 |
| ✗ | `TargetName` | str | 3 | "JIM1" |
| ✗ | `AIState` | int | 3 | 1 … 2 |
| ✗ | `WanderRange` | float | 3 | -1 … 1.5e+03 |
| ✗ | `TalkState0` | int | 3 | 0 … 400 |
| ✗ | `TalkTrigger0` | str | 3 | "needcindy", "none", "seecindy" |
| ✗ | `TalkState1` | int | 3 | -1 … 300 |
| ✗ | `TalkTrigger1` | str | 3 | "none", "seesheen" |
| ✗ | `TalkState2` | int | 3 | -1 … 350 |
| ✗ | `TalkTrigger2` | str | 3 | "none", "sheencandybar" |
| ✗ | `TalkState3` | int | 3 | -1 … -1 |
| ✗ | `TalkTrigger3` | str | 3 | "none" |
| ✗ | `TalkState4` | int | 3 | -1 … -1 |
| ✗ | `TalkTrigger4` | str | 3 | "none" |

### `3BEN`  — 3 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 3 | "Benny1", "C3DBENNY" |
| ✗ | `RotateToDest` | flag4 | 3 | 01010101 |
| ✗ | `ObjectID` | int | 3 | 859981134 … 859981134 |
| ✓ | `PositionX` | float | 3 | -69.5 … 1.42e+04 |
| ✓ | `PositionY` | float | 3 | 9.46 … 23.8 |
| ✓ | `PositionZ` | float | 3 | -1.44e+04 … 190 |
| ✓ | `RotationX` | float | 3 | 0 … 0 |
| ✓ | `RotationY` | float | 3 | 60 … 180 |
| ✓ | `RotationZ` | float | 3 | 0 … 0 |
| ✗ | `TaskName` | str | 3 | "scene" |
| ✗ | `Debug` | int | 3 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 3 | 0 … 90 |
| ✗ | `ExactLevel` | int | 3 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 3 | -1 … 100 |
| ✗ | `HasCollision` | int | 3 | 0 … 1 |
| ✓ | `InitiallyVisible` | int | 3 | -1 … -1 |
| ✗ | `CanMove` | int | 3 | 1 … 1 |
| ✗ | `SecondPass` | int | 3 | 0 … 0 |
| ✗ | `PickupLink` | str | 2 | "none" |
| ✓ | `PatrolPoint` | str | 3 | "ben1", "none" |
| ✗ | `VisibleRange` | float | 3 | 10 … 700 |
| ✗ | `FOV` | float | 3 | 90 … 359 |
| ✗ | `TargetName` | str | 3 | "JIM1" |
| ✗ | `AIState` | int | 3 | 1 … 6 |
| ✗ | `WanderRange` | float | 3 | 1.5e+03 … 1.5e+03 |
| ✗ | `TalkState0` | int | 3 | 0 … 110 |
| ✗ | `TalkTrigger0` | str | 3 | "goinside", "none" |
| ✗ | `TalkState1` | int | 3 | -1 … 120 |
| ✗ | `TalkTrigger1` | str | 3 | "benwalk", "none" |
| ✗ | `TalkState2` | int | 3 | -1 … 140 |
| ✗ | `TalkTrigger2` | str | 3 | "gototrack", "none" |
| ✗ | `TalkState3` | int | 3 | -1 … 162 |
| ✗ | `TalkTrigger3` | str | 3 | "gohome", "none" |
| ✗ | `TalkState4` | int | 3 | -1 … 340 |
| ✗ | `TalkTrigger4` | str | 3 | "libbysheen", "none" |

### `3SPH`  — 3 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 3 | "C3DSPHERE" |
| ✗ | `RotateToDest` | flag4 | 3 | 01010100 |
| ✗ | `ObjectID` | int | 3 | 861098056 … 861098056 |
| ✓ | `PositionX` | float | 3 | 2.05e+03 … 5.56e+03 |
| ✓ | `PositionY` | float | 3 | 238 … 381 |
| ✓ | `PositionZ` | float | 3 | -3.63e+03 … 3.53e+03 |
| ✓ | `RotationX` | float | 3 | 0 … 0 |
| ✓ | `RotationY` | float | 3 | 0 … 0 |
| ✓ | `RotationZ` | float | 3 | 0 … 0 |
| ✗ | `TaskName` | str | 3 | "scene" |
| ✗ | `Debug` | int | 3 | 0 … 0 |

### `3TEL`  — 3 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 3 | "C3DTELEPORTFX", "telfx1", "telfx2" |
| ✗ | `RotateToDest` | flag4 | 3 | 01010100 |
| ✗ | `ObjectID` | int | 3 | 861160780 … 861160780 |
| ✓ | `PositionX` | float | 3 | 183 … 5.25e+03 |
| ✓ | `PositionY` | float | 3 | 16.8 … 1.13e+04 |
| ✓ | `PositionZ` | float | 3 | -2.43e+03 … -198 |
| ✓ | `RotationX` | float | 3 | 0 … 0 |
| ✓ | `RotationY` | float | 3 | 0 … 180 |
| ✓ | `RotationZ` | float | 3 | 0 … 0 |
| ✗ | `TaskName` | str | 3 | "scene" |
| ✗ | `Debug` | int | 3 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 3 | -1 … -1 |
| ✗ | `ExactLevel` | int | 3 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 3 | -1 … -1 |
| ✗ | `HasCollision` | int | 3 | 0 … 0 |
| ✓ | `InitiallyVisible` | int | 3 | 0 … 1 |
| ✗ | `CanMove` | int | 3 | 0 … 1 |
| ✗ | `SecondPass` | int | 3 | 0 … 0 |
| ✗ | `PickupLink` | str | 3 | "none" |

### `3LIG`  — 3 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 3 | "C3DLIGHT" |
| ✗ | `RotateToDest` | flag4 | 3 | 01010100 |
| ✗ | `ObjectID` | int | 3 | 860637511 … 860637511 |
| ✓ | `PositionX` | float | 3 | -751 … 272 |
| ✓ | `PositionY` | float | 3 | 134 … 1.23e+04 |
| ✓ | `PositionZ` | float | 3 | 4.86e+03 … 1.2e+04 |
| ✓ | `RotationX` | float | 3 | 0 … 0 |
| ✓ | `RotationY` | float | 3 | 0 … 0 |
| ✓ | `RotationZ` | float | 3 | 0 … 0 |
| ✗ | `TaskName` | str | 3 | "scene" |
| ✗ | `Debug` | int | 3 | 0 … 0 |
| ✗ | `LightType` | raw4 | 3 | 00000000 |
| ✗ | `LightRange` | float | 3 | 1e+04 … 1e+04 |
| ✗ | `CAttenuation` | float | 3 | 1 … 1 |
| ✗ | `DifRed` | float | 3 | 1 … 1 |
| ✗ | `DifGreen` | float | 3 | 1 … 1 |
| ✗ | `DifBlue` | float | 3 | 1 … 1 |
| ✗ | `SpecRed` | float | 3 | 1 … 1 |
| ✗ | `SpecGreen` | float | 3 | 1 … 1 |
| ✗ | `SpecBlue` | float | 3 | 1 … 1 |
| ✗ | `AmbRed` | float | 3 | 0 … 0 |
| ✗ | `AmbGreen` | float | 3 | 0 … 0 |
| ✗ | `AmbBlue` | float | 3 | 0 … 0 |

### `3FER`  — 3 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 3 | "C3DFERRIS" |
| ✗ | `RotateToDest` | flag4 | 3 | 00010100, 01010100 |
| ✗ | `ObjectID` | int | 3 | 860243282 … 860243282 |
| ✓ | `PositionX` | float | 3 | -1.15e+04 … 1.89e+03 |
| ✓ | `PositionY` | float | 3 | 566 … 929 |
| ✓ | `PositionZ` | float | 3 | -5.29e+03 … 8.31e+03 |
| ✓ | `RotationX` | float | 3 | 0 … 0 |
| ✓ | `RotationY` | float | 3 | 0 … 135 |
| ✓ | `RotationZ` | float | 3 | 0 … 110 |
| ✗ | `TaskName` | str | 3 | "none" |
| ✗ | `Debug` | int | 3 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 3 | 0 … 0 |
| ✗ | `ExactLevel` | int | 3 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 3 | -1 … -1 |
| ✗ | `HasCollision` | int | 3 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 3 | -1 … -1 |
| ✗ | `CanMove` | int | 3 | 1 … 1 |
| ✗ | `SecondPass` | int | 3 | 0 … 0 |
| ✗ | `PickupLink` | str | 2 | "none" |

### `3SUM`  — 3 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 3 | "C3DSUMO" |
| ✗ | `RotateToDest` | flag4 | 3 | 00010100, 01010100 |
| ✗ | `ObjectID` | int | 3 | 861099341 … 861099341 |
| ✓ | `PositionX` | float | 3 | -9.52e+03 … -1.72e+03 |
| ✓ | `PositionY` | float | 3 | -108 … 57.9 |
| ✓ | `PositionZ` | float | 3 | -4.21e+03 … 4.86e+03 |
| ✓ | `RotationX` | float | 3 | 0 … 0 |
| ✓ | `RotationY` | float | 3 | 0 … 66.5 |
| ✓ | `RotationZ` | float | 3 | 0 … 0 |
| ✗ | `TaskName` | str | 3 | "none", "scene" |
| ✗ | `Debug` | int | 3 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 3 | -1 … 0 |
| ✗ | `ExactLevel` | int | 3 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 3 | -1 … -1 |
| ✗ | `HasCollision` | int | 3 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 3 | -1 … -1 |
| ✗ | `CanMove` | int | 3 | 1 … 1 |
| ✗ | `SecondPass` | int | 3 | 0 … 0 |
| ✗ | `PickupLink` | str | 2 | "none" |
| ✓ | `StartPoint` | str | 3 | "none", "sumoexit" |

### `3PEN`  — 3 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 3 | "C3DPENDULUM" |
| ✗ | `RotateToDest` | flag4 | 3 | 00010100, 3f010100, 6f010100 |
| ✗ | `ObjectID` | int | 3 | 860898638 … 860898638 |
| ✓ | `PositionX` | float | 3 | 1.52e+03 … 2.96e+03 |
| ✓ | `PositionY` | float | 3 | -651 … 4.7e+03 |
| ✓ | `PositionZ` | float | 3 | -2.29e+03 … 6.67e+03 |
| ✓ | `RotationX` | float | 3 | 0 … 0 |
| ✓ | `RotationY` | float | 3 | 0 … 90 |
| ✓ | `RotationZ` | float | 3 | 0 … 0 |
| ✗ | `TaskName` | str | 3 | "none" |
| ✗ | `Debug` | int | 3 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 3 | 0 … 0 |
| ✗ | `ExactLevel` | int | 3 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 3 | -1 … -1 |
| ✗ | `HasCollision` | int | 3 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 3 | -1 … -1 |
| ✗ | `CanMove` | int | 3 | 1 … 1 |
| ✗ | `SecondPass` | int | 3 | 0 … 0 |
| ✗ | `PickupLink` | str | 3 | "none" |
| ✗ | `SwingHeight` | float | 3 | 15 … 25 |

### `3MOM`  — 2 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 2 | "C3DJUDY" |
| ✗ | `RotateToDest` | flag4 | 2 | 00010101, 01010101 |
| ✗ | `ObjectID` | int | 2 | 860704589 … 860704589 |
| ✓ | `PositionX` | float | 2 | 437 … 8.16e+03 |
| ✓ | `PositionY` | float | 2 | 11.8 … 25.6 |
| ✓ | `PositionZ` | float | 2 | -1.13e+03 … 1.38e+03 |
| ✓ | `RotationX` | float | 2 | 0 … 0 |
| ✓ | `RotationY` | float | 2 | 140 … 233 |
| ✓ | `RotationZ` | float | 2 | 0 … 0 |
| ✗ | `TaskName` | str | 2 | "Scene", "scene" |
| ✗ | `Debug` | int | 2 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 2 | 0 … 200 |
| ✗ | `ExactLevel` | int | 2 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 2 | -1 … 250 |
| ✗ | `HasCollision` | int | 2 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 2 | -1 … -1 |
| ✗ | `CanMove` | int | 2 | 1 … 1 |
| ✗ | `SecondPass` | int | 2 | 0 … 0 |
| ✗ | `PickupLink` | str | 2 | "none" |
| ✓ | `PatrolPoint` | str | 2 | "JUDY1A", "none" |
| ✗ | `VisibleRange` | float | 2 | 500 … 500 |
| ✗ | `FOV` | float | 2 | 90 … 359 |
| ✗ | `TargetName` | str | 2 | "JIM1" |
| ✗ | `AIState` | int | 2 | 1 … 2 |
| ✗ | `WanderRange` | float | 2 | 1.5e+03 … 1.5e+03 |
| ✗ | `TalkState0` | int | 2 | 0 … 200 |
| ✗ | `TalkTrigger0` | str | 2 | "none", "requestkey" |
| ✗ | `TalkState1` | int | 2 | -1 … 210 |
| ✗ | `TalkTrigger1` | str | 2 | "gotkey", "none" |
| ✗ | `TalkState2` | int | 2 | -1 … 230 |
| ✗ | `TalkTrigger2` | str | 2 | "none", "withwrench" |
| ✗ | `TalkState3` | int | 2 | -1 … -1 |
| ✗ | `TalkTrigger3` | str | 2 | "none" |
| ✗ | `TalkState4` | int | 2 | -1 … -1 |
| ✗ | `TalkTrigger4` | str | 2 | "none" |

### `3HYD`  — 2 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 2 | "HYDRANT2", "water1" |
| ✗ | `RotateToDest` | flag4 | 2 | 01010100 |
| ✗ | `ObjectID` | int | 2 | 860379460 … 860379460 |
| ✓ | `PositionX` | float | 2 | 3.36e+03 … 9.77e+03 |
| ✓ | `PositionY` | float | 2 | -22.6 … -7.35 |
| ✓ | `PositionZ` | float | 2 | -815 … 2.35e+03 |
| ✓ | `RotationX` | float | 2 | 0 … 0 |
| ✓ | `RotationY` | float | 2 | 200 … 220 |
| ✓ | `RotationZ` | float | 2 | 0 … 0 |
| ✗ | `TaskName` | str | 2 | "scene" |
| ✗ | `Debug` | int | 2 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 2 | -1 … -1 |
| ✗ | `ExactLevel` | int | 2 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 2 | -1 … -1 |
| ✗ | `HasCollision` | int | 2 | -1 … 1 |
| ✓ | `InitiallyVisible` | int | 2 | 0 … 0 |
| ✗ | `CanMove` | int | 2 | 0 … 0 |
| ✗ | `SecondPass` | int | 2 | 1 … 1 |
| ✗ | `PickupLink` | str | 2 | "hydrant", "water2" |

### `3SPY`  — 2 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 2 | "captain", "lackie" |
| ✗ | `RotateToDest` | flag4 | 2 | 01010101 |
| ✗ | `ObjectID` | int | 2 | 861098073 … 861098073 |
| ✓ | `PositionX` | float | 2 | -486 … -380 |
| ✓ | `PositionY` | float | 2 | -4.09e+03 … -4.08e+03 |
| ✓ | `PositionZ` | float | 2 | -897 … -767 |
| ✓ | `RotationX` | float | 2 | 0 … 0 |
| ✓ | `RotationY` | float | 2 | 220 … 230 |
| ✓ | `RotationZ` | float | 2 | 0 … 0 |
| ✗ | `TaskName` | str | 2 | "scene" |
| ✗ | `Debug` | int | 2 | 0 … 1 |
| ✗ | `RequiredLevel` | int | 2 | -1 … 0 |
| ✗ | `ExactLevel` | int | 2 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 2 | -1 … 410 |
| ✗ | `HasCollision` | int | 2 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 2 | -1 … -1 |
| ✗ | `CanMove` | int | 2 | 1 … 1 |
| ✗ | `SecondPass` | int | 2 | 0 … 0 |
| ✗ | `PickupLink` | str | 2 | "none" |
| ✓ | `PatrolPoint` | str | 2 | "none" |
| ✗ | `VisibleRange` | float | 2 | 100 … 2.5e+03 |
| ✗ | `FOV` | float | 2 | 90 … 90 |
| ✗ | `TargetName` | str | 2 | "JIM1" |
| ✗ | `AIState` | int | 2 | 1 … 2 |
| ✗ | `WanderRange` | float | 2 | 1.5e+03 … 1.5e+03 |

### `3FLA`  — 2 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 2 | "C3DFIRESTRATO" |
| ✗ | `RotateToDest` | flag4 | 2 | 01010100 |
| ✗ | `ObjectID` | int | 2 | 860245057 … 860245057 |
| ✓ | `PositionX` | float | 2 | 431 … 431 |
| ✓ | `PositionY` | float | 2 | 992 … 992 |
| ✓ | `PositionZ` | float | 2 | 411 … 411 |
| ✓ | `RotationX` | float | 2 | 0 … 0 |
| ✓ | `RotationY` | float | 2 | 180 … 180 |
| ✓ | `RotationZ` | float | 2 | 0 … 0 |
| ✗ | `TaskName` | str | 2 | "none" |
| ✗ | `Debug` | int | 2 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 2 | 0 … 0 |
| ✗ | `ExactLevel` | int | 2 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 2 | -1 … -1 |
| ✗ | `HasCollision` | int | 2 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 2 | -1 … -1 |
| ✗ | `CanMove` | int | 2 | 1 … 1 |
| ✗ | `SecondPass` | int | 2 | 0 … 0 |
| ✗ | `PickupLink` | str | 1 | "none" |

### `3DIN`  — 2 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 2 | "C3DDINO" |
| ✗ | `RotateToDest` | flag4 | 2 | 01010101 |
| ✗ | `ObjectID` | int | 2 | 860113230 … 860113230 |
| ✓ | `PositionX` | float | 2 | -1.3e+03 … -1.3e+03 |
| ✓ | `PositionY` | float | 2 | 30.2 … 30.2 |
| ✓ | `PositionZ` | float | 2 | 1.06e+03 … 1.06e+03 |
| ✓ | `RotationX` | float | 2 | 0 … 0 |
| ✓ | `RotationY` | float | 2 | 0 … 0 |
| ✓ | `RotationZ` | float | 2 | 0 … 0 |
| ✗ | `TaskName` | str | 2 | "scene" |
| ✗ | `Debug` | int | 2 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 2 | 260 … 260 |
| ✗ | `ExactLevel` | int | 2 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 2 | 460 … 460 |
| ✗ | `HasCollision` | int | 2 | -1 … 1 |
| ✓ | `InitiallyVisible` | int | 2 | -1 … -1 |
| ✗ | `CanMove` | int | 2 | 1 … 1 |
| ✗ | `SecondPass` | int | 2 | 0 … 0 |
| ✓ | `PatrolPoint` | str | 2 | "none" |
| ✗ | `VisibleRange` | float | 2 | 2e+03 … 2e+03 |
| ✗ | `FOV` | float | 2 | 90 … 90 |
| ✗ | `TargetName` | str | 2 | "C3DBUS" |
| ✗ | `AIState` | int | 2 | 4 … 4 |
| ✗ | `WanderRange` | float | 2 | 2e+03 … 2e+03 |
| ✗ | `PickupIndex` | int | 2 | 622 … 3803 |
| ✗ | `PIC_NUMBER` | int | 2 | 14 … 14 |
| ✗ | `PickupLink` | str | 1 | "none" |

### `3KIT`  — 2 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 2 | "C3DKITTY" |
| ✗ | `RotateToDest` | flag4 | 2 | 01010101 |
| ✗ | `ObjectID` | int | 2 | 860571988 … 860571988 |
| ✓ | `PositionX` | float | 2 | 3.62e+03 … 7.01e+03 |
| ✓ | `PositionY` | float | 2 | 791 … 818 |
| ✓ | `PositionZ` | float | 2 | 1.66e+03 … 5.09e+03 |
| ✓ | `RotationX` | float | 2 | 0 … 0 |
| ✓ | `RotationY` | float | 2 | 0 … 90 |
| ✓ | `RotationZ` | float | 2 | 0 … 0 |
| ✗ | `TaskName` | str | 2 | "kitty1", "kitty2" |
| ✗ | `Debug` | int | 2 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 2 | -1 … 0 |
| ✗ | `ExactLevel` | int | 2 | 0 … 0 |
| ✗ | `RemoveLevel` | int | 2 | -1 … -1 |
| ✗ | `HasCollision` | int | 2 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 2 | 0 … 1 |
| ✗ | `CanMove` | int | 2 | 1 … 1 |
| ✗ | `SecondPass` | int | 2 | 0 … 0 |
| ✓ | `PatrolPoint` | str | 2 | "cat1", "runpuss1" |
| ✗ | `VisibleRange` | float | 2 | 2.5e+03 … 4e+03 |
| ✗ | `FOV` | float | 2 | 90 … 359 |
| ✗ | `TargetName` | str | 2 | "JIM1", "none" |
| ✗ | `AIState` | int | 2 | 1 … 1 |
| ✗ | `WanderRange` | float | 2 | 1.5e+03 … 1.5e+03 |
| ✗ | `PickupLink` | str | 1 | "none" |

### `3ULT`  — 2 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 2 | "C3DULTRALORD" |
| ✗ | `RotateToDest` | flag4 | 2 | 01010101, 50010101 |
| ✗ | `ObjectID` | int | 2 | 861228116 … 861228116 |
| ✓ | `PositionX` | float | 2 | -2.17e+03 … 273 |
| ✓ | `PositionY` | float | 2 | 8.25 … 99.1 |
| ✓ | `PositionZ` | float | 2 | -627 … 7.85e+03 |
| ✓ | `RotationX` | float | 2 | 0 … 0.116 |
| ✓ | `RotationY` | float | 2 | 65.7 … 180 |
| ✓ | `RotationZ` | float | 2 | 0 … 358 |
| ✗ | `TaskName` | str | 2 | "Scene", "scene" |
| ✗ | `Debug` | int | 2 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 2 | 0 … 0 |
| ✗ | `ExactLevel` | int | 2 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 2 | -1 … -1 |
| ✗ | `HasCollision` | int | 2 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 2 | -1 … -1 |
| ✗ | `CanMove` | int | 2 | 1 … 1 |
| ✗ | `SecondPass` | int | 2 | 0 … 0 |
| ✗ | `PickupLink` | str | 2 | "none" |
| ✓ | `PatrolPoint` | str | 2 | "none", "ultra1" |
| ✗ | `VisibleRange` | float | 2 | 500 … 500 |
| ✗ | `FOV` | float | 2 | 90 … 359 |
| ✗ | `TargetName` | str | 2 | "JIM1" |
| ✗ | `AIState` | int | 2 | 1 … 2 |
| ✗ | `WanderRange` | float | 2 | 1.5e+03 … 1.5e+03 |
| ✗ | `TalkState0` | int | 2 | 0 … 330 |
| ✗ | `TalkTrigger0` | str | 2 | "none", "ultralord" |
| ✗ | `TalkState1` | int | 2 | -1 … 390 |
| ✗ | `TalkTrigger1` | str | 2 | "getfuel", "none" |
| ✗ | `TalkState2` | int | 2 | -1 … -1 |
| ✗ | `TalkTrigger2` | str | 2 | "none" |
| ✗ | `TalkState3` | int | 2 | -1 … -1 |
| ✗ | `TalkTrigger3` | str | 2 | "none" |
| ✗ | `TalkState4` | int | 2 | -1 … -1 |
| ✗ | `TalkTrigger4` | str | 2 | "none" |

### `3CUB`  — 2 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 2 | "C3DCUBE" |
| ✗ | `RotateToDest` | flag4 | 2 | 01010100 |
| ✗ | `ObjectID` | int | 2 | 860050754 … 860050754 |
| ✓ | `PositionX` | float | 2 | 0 … 0 |
| ✓ | `PositionY` | float | 2 | 0 … 0 |
| ✓ | `PositionZ` | float | 2 | 0 … 0 |
| ✓ | `RotationX` | float | 2 | 0 … 0 |
| ✓ | `RotationY` | float | 2 | 0 … 0 |
| ✓ | `RotationZ` | float | 2 | 0 … 0 |
| ✗ | `TaskName` | str | 2 | "none" |
| ✗ | `Debug` | int | 2 | 0 … 0 |

### `3HUG`  — 2 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 2 | "C3DHUGH" |
| ✗ | `RotateToDest` | flag4 | 2 | 00010101, 01010101 |
| ✗ | `ObjectID` | int | 2 | 860378439 … 860378439 |
| ✓ | `PositionX` | float | 2 | -313 … 872 |
| ✓ | `PositionY` | float | 2 | 21 … 31.6 |
| ✓ | `PositionZ` | float | 2 | -1.14e+03 … -843 |
| ✓ | `RotationX` | float | 2 | 0 … 0 |
| ✓ | `RotationY` | float | 2 | 0 … 210 |
| ✓ | `RotationZ` | float | 2 | 0 … 0 |
| ✗ | `TaskName` | str | 2 | "Scene", "scene" |
| ✗ | `Debug` | int | 2 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 2 | 0 … 100 |
| ✗ | `ExactLevel` | int | 2 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 2 | -1 … -1 |
| ✗ | `HasCollision` | int | 2 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 2 | -1 … -1 |
| ✗ | `CanMove` | int | 2 | 1 … 1 |
| ✗ | `SecondPass` | int | 2 | 0 … 0 |
| ✓ | `PatrolPoint` | str | 2 | "HUGH1A", "none" |
| ✗ | `VisibleRange` | float | 2 | 500 … 500 |
| ✗ | `FOV` | float | 2 | 90 … 359 |
| ✗ | `TargetName` | str | 2 | "JIM1" |
| ✗ | `AIState` | int | 2 | 1 … 2 |
| ✗ | `WanderRange` | float | 2 | 1.5e+03 … 1.5e+03 |
| ✗ | `TalkState0` | int | 2 | 0 … 205 |
| ✗ | `TalkTrigger0` | str | 2 | "gotkey", "none" |
| ✗ | `TalkState1` | int | 2 | -1 … -1 |
| ✗ | `TalkTrigger1` | str | 2 | "none" |
| ✗ | `TalkState2` | int | 2 | -1 … -1 |
| ✗ | `TalkTrigger2` | str | 2 | "none" |
| ✗ | `TalkState3` | int | 2 | -1 … -1 |
| ✗ | `TalkTrigger3` | str | 2 | "none" |
| ✗ | `TalkState4` | int | 2 | -1 … -1 |
| ✗ | `TalkTrigger4` | str | 2 | "none" |
| ✗ | `PickupLink` | str | 1 | "none" |

### `3DIG`  — 2 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 2 | "C3DDIGGER" |
| ✗ | `RotateToDest` | flag4 | 2 | 01010100 |
| ✗ | `ObjectID` | int | 2 | 860113223 … 860113223 |
| ✓ | `PositionX` | float | 2 | -1.48e+04 … 2.98e+04 |
| ✓ | `PositionY` | float | 2 | -5.22e+03 … -4.45e+03 |
| ✓ | `PositionZ` | float | 2 | -3.15e+04 … 1.53e+04 |
| ✓ | `RotationX` | float | 2 | 0 … 15 |
| ✓ | `RotationY` | float | 2 | 15 … 100 |
| ✓ | `RotationZ` | float | 2 | 0 … 30 |
| ✗ | `TaskName` | str | 2 | "none" |
| ✗ | `Debug` | int | 2 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 2 | -1 … -1 |
| ✗ | `ExactLevel` | int | 2 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 2 | -1 … -1 |
| ✗ | `HasCollision` | int | 2 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 2 | -1 … -1 |
| ✗ | `CanMove` | int | 2 | 1 … 1 |
| ✗ | `SecondPass` | int | 2 | 0 … 0 |

### `3SAI`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "SAILBOAT1" |
| ✗ | `RotateToDest` | flag4 | 1 | 01010101 |
| ✗ | `ObjectID` | int | 1 | 861094217 … 861094217 |
| ✓ | `PositionX` | float | 1 | 458 … 458 |
| ✓ | `PositionY` | float | 1 | 17 … 17 |
| ✓ | `PositionZ` | float | 1 | 4.84e+03 … 4.84e+03 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 180 … 180 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "scene" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 1 | 100 … 100 |
| ✗ | `ExactLevel` | int | 1 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 1 | -1 … -1 |
| ✗ | `HasCollision` | int | 1 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 1 | -1 … -1 |
| ✗ | `CanMove` | int | 1 | 1 … 1 |
| ✗ | `SecondPass` | int | 1 | 0 … 0 |
| ✗ | `PickupLink` | str | 1 | "none" |
| ✓ | `PatrolPoint` | str | 1 | "BOAT2" |
| ✗ | `VisibleRange` | float | 1 | 2.5e+03 … 2.5e+03 |
| ✗ | `FOV` | float | 1 | 90 … 90 |
| ✗ | `TargetName` | str | 1 | "JIM1" |
| ✗ | `AIState` | int | 1 | 1 … 1 |
| ✗ | `WanderRange` | float | 1 | 1.5e+03 … 1.5e+03 |

### `3MER`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "C3DMERRYGO" |
| ✗ | `RotateToDest` | flag4 | 1 | 01010100 |
| ✗ | `ObjectID` | int | 1 | 860702034 … 860702034 |
| ✓ | `PositionX` | float | 1 | 3.44e+03 … 3.44e+03 |
| ✓ | `PositionY` | float | 1 | -2.09 … -2.09 |
| ✓ | `PositionZ` | float | 1 | 7.97e+03 … 7.97e+03 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 0 … 0 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "scene" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 1 | 0 … 0 |
| ✗ | `ExactLevel` | int | 1 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 1 | -1 … -1 |
| ✗ | `HasCollision` | int | 1 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 1 | -1 … -1 |
| ✗ | `CanMove` | int | 1 | 1 … 1 |
| ✗ | `SecondPass` | int | 1 | 0 … 0 |
| ✗ | `PickupLink` | str | 1 | "none" |
| ✓ | `StartPoint` | str | 1 | "MERRYSTART" |

### `TRIG`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "CTRIGGER" |
| ✗ | `RotateToDest` | flag4 | 1 | 01000100 |
| ✗ | `ObjectID` | int | 1 | 1414678855 … 1414678855 |
| ✓ | `PositionX` | float | 1 | -1.14e+03 … -1.14e+03 |
| ✓ | `PositionY` | float | 1 | 136 … 136 |
| ✓ | `PositionZ` | float | 1 | 3.97e+03 … 3.97e+03 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 0 … 0 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "none" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `LightType` | raw4 | 1 | 00000000 |
| ✗ | `LightRange` | float | 1 | 1e+04 … 1e+04 |
| ✗ | `CAttenuation` | float | 1 | 1 … 1 |
| ✗ | `DifRed` | float | 1 | 1 … 1 |
| ✗ | `DifGreen` | float | 1 | 1 … 1 |
| ✗ | `DifBlue` | float | 1 | 1 … 1 |
| ✗ | `SpecRed` | float | 1 | 1 … 1 |
| ✗ | `SpecGreen` | float | 1 | 1 … 1 |
| ✗ | `SpecBlue` | float | 1 | 1 … 1 |
| ✗ | `AmbRed` | float | 1 | 0 … 0 |
| ✗ | `AmbGreen` | float | 1 | 0 … 0 |
| ✗ | `AmbBlue` | float | 1 | 0 … 0 |

### `3OCT`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "C3DOCTAPUKE" |
| ✗ | `RotateToDest` | flag4 | 1 | 01010100 |
| ✗ | `ObjectID` | int | 1 | 860832596 … 860832596 |
| ✓ | `PositionX` | float | 1 | 1.81e+03 … 1.81e+03 |
| ✓ | `PositionY` | float | 1 | 7.2 … 7.2 |
| ✓ | `PositionZ` | float | 1 | -966 … -966 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 0 … 0 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "none" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 1 | 0 … 0 |
| ✗ | `ExactLevel` | int | 1 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 1 | -1 … -1 |
| ✗ | `HasCollision` | int | 1 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 1 | -1 … -1 |
| ✗ | `CanMove` | int | 1 | 1 … 1 |
| ✗ | `SecondPass` | int | 1 | 0 … 0 |
| ✓ | `StartPoint` | str | 1 | "octaexit" |

### `3PIR`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "C3DPIRATE" |
| ✗ | `RotateToDest` | flag4 | 1 | 01010100 |
| ✗ | `ObjectID` | int | 1 | 860899666 … 860899666 |
| ✓ | `PositionX` | float | 1 | 448 … 448 |
| ✓ | `PositionY` | float | 1 | 1.16e+03 … 1.16e+03 |
| ✓ | `PositionZ` | float | 1 | 3.03e+03 … 3.03e+03 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 303 … 303 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "none" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 1 | 0 … 0 |
| ✗ | `ExactLevel` | int | 1 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 1 | -1 … -1 |
| ✗ | `HasCollision` | int | 1 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 1 | -1 … -1 |
| ✗ | `CanMove` | int | 1 | 1 … 1 |
| ✗ | `SecondPass` | int | 1 | 0 … 0 |
| ✓ | `StartPoint` | str | 1 | "shipexit" |

### `3TRA`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "C3DTRANSLUCENT" |
| ✗ | `RotateToDest` | flag4 | 1 | 01010100 |
| ✗ | `ObjectID` | int | 1 | 861164097 … 861164097 |
| ✓ | `PositionX` | float | 1 | -0.648 … -0.648 |
| ✓ | `PositionY` | float | 1 | 8.28 … 8.28 |
| ✓ | `PositionZ` | float | 1 | -3.42 … -3.42 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 0 … 0 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "scene" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 1 | -1 … -1 |
| ✗ | `ExactLevel` | int | 1 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 1 | -1 … -1 |
| ✗ | `HasCollision` | int | 1 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 1 | -1 … -1 |
| ✗ | `CanMove` | int | 1 | 1 … 1 |
| ✗ | `SecondPass` | int | 1 | 0 … 0 |
| ✗ | `PickupLink` | str | 1 | "none" |
| ✗ | `ASEFile` | str | 1 | "lavalevel5a.ase" |
| ✓ | `PNGFile` | str | 1 | "lava2.png" |

### `3SM1`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "C3DSMOKE" |
| ✗ | `RotateToDest` | flag4 | 1 | 01010100 |
| ✗ | `ObjectID` | int | 1 | 861097265 … 861097265 |
| ✓ | `PositionX` | float | 1 | -3.12e+03 … -3.12e+03 |
| ✓ | `PositionY` | float | 1 | 6.44e+03 … 6.44e+03 |
| ✓ | `PositionZ` | float | 1 | -1.21e+03 … -1.21e+03 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 0 … 0 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "none" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✓ | `SpriteSize` | int | 1 | 40 … 40 |
| ✓ | `SpriteDatabase` | str | 1 | "sprites.omt" |
| ✓ | `SpriteIndex` | int | 1 | 37 … 37 |

### `3SCR`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "C3DLABSCREEN" |
| ✗ | `RotateToDest` | flag4 | 1 | 01010100 |
| ✗ | `ObjectID` | int | 1 | 861094738 … 861094738 |
| ✓ | `PositionX` | float | 1 | 2.4e+03 … 2.4e+03 |
| ✓ | `PositionY` | float | 1 | 277 … 277 |
| ✓ | `PositionZ` | float | 1 | -1.08e+03 … -1.08e+03 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 0 … 0 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "none" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 1 | 0 … 0 |
| ✗ | `ExactLevel` | int | 1 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 1 | -1 … -1 |
| ✗ | `HasCollision` | int | 1 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 1 | -1 … -1 |
| ✗ | `CanMove` | int | 1 | 1 … 1 |
| ✗ | `SecondPass` | int | 1 | 0 … 0 |
| ✗ | `PickupLink` | str | 1 | "none" |

### `3DUD`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "bars" |
| ✗ | `RotateToDest` | flag4 | 1 | 01010100 |
| ✗ | `ObjectID` | int | 1 | 860116292 … 860116292 |
| ✓ | `PositionX` | float | 1 | 3.81e+03 … 3.81e+03 |
| ✓ | `PositionY` | float | 1 | -1.08e+03 … -1.08e+03 |
| ✓ | `PositionZ` | float | 1 | -1.18e+03 … -1.18e+03 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 0 … 0 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "scene" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 1 | -1 … -1 |
| ✗ | `ExactLevel` | int | 1 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 1 | -1 … -1 |
| ✗ | `HasCollision` | int | 1 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 1 | -1 … -1 |
| ✗ | `CanMove` | int | 1 | 1 … 1 |
| ✗ | `SecondPass` | int | 1 | 0 … 0 |
| ✗ | `PickupLink` | str | 1 | "none" |
| ✗ | `ItemClosed` | int | 1 | 1 … 1 |
| ✗ | `Next` | str | 1 | "none" |
| ✗ | `DoorSpeed` | float | 1 | 5 … 5 |
| ✗ | `OpenTime` | float | 1 | 3 … 3 |
| ✗ | `OpenAmount` | float | 1 | 500 … 500 |
| ✗ | `ASEFile` | str | 1 | "bars.ase" |
| ✓ | `PNGFile` | str | 1 | "chain.png" |
| ✗ | `TouchActivated` | int | 1 | 0 … 0 |

### `3TOL`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "C3DTOOLCHEST" |
| ✗ | `RotateToDest` | flag4 | 1 | 01010100 |
| ✗ | `ObjectID` | int | 1 | 861163340 … 861163340 |
| ✓ | `PositionX` | float | 1 | -1.8e+03 … -1.8e+03 |
| ✓ | `PositionY` | float | 1 | 109 … 109 |
| ✓ | `PositionZ` | float | 1 | 4.44e+03 … 4.44e+03 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 180 … 180 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "none" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 1 | -1 … -1 |
| ✗ | `ExactLevel` | int | 1 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 1 | -1 … -1 |
| ✗ | `HasCollision` | int | 1 | 1 … 1 |
| ✓ | `InitiallyVisible` | int | 1 | -1 … -1 |
| ✗ | `CanMove` | int | 1 | 0 … 0 |
| ✗ | `SecondPass` | int | 1 | 0 … 0 |
| ✗ | `PickupLink` | str | 1 | "none" |

### `3CML`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "C3DCAMEL" |
| ✗ | `RotateToDest` | flag4 | 1 | 01010101 |
| ✗ | `ObjectID` | int | 1 | 860048716 … 860048716 |
| ✓ | `PositionX` | float | 1 | 44.5 … 44.5 |
| ✓ | `PositionY` | float | 1 | 2.54e+03 … 2.54e+03 |
| ✓ | `PositionZ` | float | 1 | 3.96e+03 … 3.96e+03 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 0 … 0 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "none" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 1 | 0 … 0 |
| ✗ | `ExactLevel` | int | 1 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 1 | -1 … -1 |
| ✗ | `HasCollision` | int | 1 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 1 | -1 … -1 |
| ✗ | `CanMove` | int | 1 | 1 … 1 |
| ✗ | `SecondPass` | int | 1 | 0 … 0 |
| ✗ | `PickupLink` | str | 1 | "none" |
| ✓ | `PatrolPoint` | str | 1 | "CAM1" |
| ✗ | `VisibleRange` | float | 1 | 500 … 500 |
| ✗ | `FOV` | float | 1 | 90 … 90 |
| ✗ | `TargetName` | str | 1 | "JIM1" |
| ✗ | `AIState` | int | 1 | 1 … 1 |
| ✗ | `WanderRange` | float | 1 | 1.5e+03 … 1.5e+03 |

### `3TRI`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "POWER3" |
| ✗ | `RotateToDest` | flag4 | 1 | ff010100 |
| ✗ | `ObjectID` | int | 1 | 861164105 … 861164105 |
| ✓ | `PositionX` | float | 1 | 1.31e+03 … 1.31e+03 |
| ✓ | `PositionY` | float | 1 | -668 … -668 |
| ✓ | `PositionZ` | float | 1 | -5.51e+03 … -5.51e+03 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 0 … 0 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "none" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✓ | `SpriteSize` | int | 1 | 50 … 50 |
| ✓ | `SpriteDatabase` | str | 1 | "icons.omt" |
| ✓ | `SpriteIndex` | int | 1 | 0 … 0 |
| ✗ | `ActivateState0` | int | 1 | 0 … 0 |
| ✗ | `ActivateObject0` | str | 1 | "none" |
| ✗ | `ActivateState1` | int | 1 | -1 … -1 |
| ✗ | `ActivateObject1` | str | 1 | "none" |
| ✗ | `ActivateState2` | int | 1 | -1 … -1 |
| ✗ | `ActivateObject2` | str | 1 | "none" |
| ✗ | `ActivateState3` | int | 1 | -1 … -1 |
| ✗ | `ActivateObject3` | str | 1 | "none" |
| ✗ | `ActivateState4` | int | 1 | -1 … -1 |
| ✗ | `ActivateObject4` | str | 1 | "none" |
| ✗ | `ActivateAnim` | str | 1 | "none" |
| ✗ | `ActivateBy` | str | 1 | "none" |
| ✗ | `SoundDatabase` | str | 1 | "none" |
| ✗ | `SoundIndex` | int | 1 | -1 … -1 |
| ✗ | `NextTrigger` | str | 1 | "none" |
| ✗ | `PlayerControlled` | str | 1 | "JIM1" |
| ✗ | `TimesToTrigger` | int | 1 | -1 … -1 |
| ✗ | `TouchActivated` | int | 1 | 0 … 0 |

### `3HOO`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "HOOK1" |
| ✗ | `RotateToDest` | flag4 | 1 | 00010101 |
| ✗ | `ObjectID` | int | 1 | 860376911 … 860376911 |
| ✓ | `PositionX` | float | 1 | 4.86e+03 … 4.86e+03 |
| ✓ | `PositionY` | float | 1 | 3.1e+03 … 3.1e+03 |
| ✓ | `PositionZ` | float | 1 | 795 … 795 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 0 … 0 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "none" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 1 | 0 … 0 |
| ✗ | `ExactLevel` | int | 1 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 1 | -1 … -1 |
| ✗ | `HasCollision` | int | 1 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 1 | -1 … -1 |
| ✗ | `CanMove` | int | 1 | 1 … 1 |
| ✗ | `SecondPass` | int | 1 | 0 … 0 |
| ✗ | `PickupLink` | str | 1 | "none" |
| ✓ | `PatrolPoint` | str | 1 | "HOOK1A" |
| ✗ | `VisibleRange` | float | 1 | 2.5e+03 … 2.5e+03 |
| ✗ | `FOV` | float | 1 | 90 … 90 |
| ✗ | `TargetName` | str | 1 | "JIM1" |
| ✗ | `AIState` | int | 1 | 3 … 3 |
| ✗ | `WanderRange` | float | 1 | 1.5e+03 … 1.5e+03 |

### `3SPW`  — 1 instances

| ✓ | prop | type | count | value range / samples |
|---|---|---|---:|---|
| ✓ | `ObjectTag` | str | 1 | "vulta" |
| ✗ | `RotateToDest` | flag4 | 1 | 01010101 |
| ✗ | `ObjectID` | int | 1 | 861098071 … 861098071 |
| ✓ | `PositionX` | float | 1 | 3.84e+03 … 3.84e+03 |
| ✓ | `PositionY` | float | 1 | -37.8 … -37.8 |
| ✓ | `PositionZ` | float | 1 | 4.44e+03 … 4.44e+03 |
| ✓ | `RotationX` | float | 1 | 0 … 0 |
| ✓ | `RotationY` | float | 1 | 0 … 0 |
| ✓ | `RotationZ` | float | 1 | 0 … 0 |
| ✗ | `TaskName` | str | 1 | "scene" |
| ✗ | `Debug` | int | 1 | 0 … 0 |
| ✗ | `RequiredLevel` | int | 1 | -1 … -1 |
| ✗ | `ExactLevel` | int | 1 | -1 … -1 |
| ✗ | `RemoveLevel` | int | 1 | -1 … -1 |
| ✗ | `HasCollision` | int | 1 | -1 … -1 |
| ✓ | `InitiallyVisible` | int | 1 | -1 … -1 |
| ✗ | `CanMove` | int | 1 | 1 … 1 |
| ✗ | `SecondPass` | int | 1 | 0 … 0 |
| ✗ | `PickupLink` | str | 1 | "none" |
| ✓ | `PatrolPoint` | str | 1 | "vulta02" |
| ✗ | `VisibleRange` | float | 1 | 2.5e+03 … 2.5e+03 |
| ✗ | `FOV` | float | 1 | 90 … 90 |
| ✗ | `TargetName` | str | 1 | "JIM1" |
| ✗ | `AIState` | int | 1 | 3 … 3 |
| ✗ | `WanderRange` | float | 1 | 1.5e+03 … 1.5e+03 |
