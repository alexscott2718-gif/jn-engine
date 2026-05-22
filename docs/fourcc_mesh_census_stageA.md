# FourCC Mesh Census — Stage A Findings

Stage A result: filesystem enumeration of game install on XP (no runtime monitoring needed).
All files confirmed present; pull-list complete.

---

## OMT pull-list (Step 1)

| OMT | Pulled | Contents |
|-----|--------|----------|
| level1.omt | already local | Environment geometry (196 meshes) |
| level1a.omt | ✅ 2026-05-15 | Zone A geometry (30 meshes) |
| level1b.omt | ✅ 2026-05-15 | Zone B geometry (121 meshes) |
| level1c.omt | ✅ 2026-05-15 | Zone C geometry (124 meshes) |
| level1d.omt | ✅ 2026-05-15 | Zone D geometry (5 meshes) |
| level1e.omt | ✅ 2026-05-15 | Zone E geometry (5 meshes) |
| level1f.omt | ✅ 2026-05-15 | Zone F geometry (14 meshes) |

**Key finding**: None of the Level1 OMTs contain character or entity meshes.
All 442 meshes are environment/building/blocking geometry.

Character meshes are stored as raw ASE files in the game's `ASE/` directory
(`C:\Program Files\THQ\Jimmy Neutron\Jimmy Neutron Boy Genius\ASE\`).
All 210 ASE files are already present locally at `assets/ase/`.

---

## Confident FourCC → ASE mappings (deduced from tag names)

| FourCC | Level(s) | Tag | ASE | Confidence |
|--------|----------|-----|-----|------------|
| 3JIM | 1,2+ | JIM1 | jimstop.ase | HIGH — direct name match |
| 3SHE | 1,2 | Sheen1/Sheen2 | shenstop.ASE | HIGH — Sheen Estevez |
| 3LIB | 1 | C3DLIBBY | libystop.ASE | HIGH — Libby |
| 3MOM | 1 | C3DJUDY | judystop.ASE | HIGH — Judy Neutron |
| 3GIR | 1 | C3DGIRLEATINGPLANT | plantstop.ASE | MED — girl-eating plant enemy; plantwait/plantwalk also available |
| 3KIT | 2+ | C3DKITTY | catsit.ASE | HIGH — kitty/cat |
| 3NIC | 2+ | Nick2 | nickstop.ASE | HIGH — Nick Dean |
| 3GUA | 2+ | (guards) | guardwalk.ASE | HIGH — guard enemy |
| 3SOL | 2+ | (soldiers) | soldwalk.ASE | HIGH — soldier enemy |
| 3SBU | 2+ | C3DBUS | retrobus.ASE | MED — bus; retrobus or bus.ASE |
| 3FLA | 2+ | C3DFIRESTRATO | firestrato.ASE | HIGH — fire strato |
| 3RCK | 1 | rocket (HIDDEN) | rocket.ASE | LOW — HIDDEN flag; treat as invisible |

## Invisible/trigger FourCCs (add to entity_visual.c as invisible)

| FourCC | Level(s) | Tag | Reason |
|--------|----------|-----|--------|
| 3SPR | 1 | C3DSPRITE | Sprite-rendered billboard, no ASE mesh |
| 3ANI | 1 | C3DANIMATEDSPRITE | Animated sprite, no ASE mesh |
| 3LAS | 2+ | C3DLASERTRIGGER | Trigger volume, invisible |
| 3DAI | 1 | 3DAI | pos=(0,0,0) in all instances; likely dummy/disabled |
| 3RCK | 1 | rocket (HIDDEN) | HIDDEN flag in GAM |

---

## Unknown — need Stage B (Ghidra) to resolve

No ASE file found in `assets/ase/` matching these classes.
These entities likely load a mesh by name via the OMT2.dll loader.

| FourCC | Level(s) | Tag(s) | Notes |
|--------|----------|--------|-------|
| 3NEU | 1,2+ | C3DNEUTRON | Neutron enemy (23 in L1, 49 in L2). No neutron.ASE. Might be OMT-loaded sphere. |
| 3LEA | 1,2+ | C3DLEAVES | Leaf collectible (13 in L1). No leaf.ASE. Billboard quad likely. |
| 3RED | 1,2+ | C3DREDNEUTRON, redneutron1, redneutron2 | Red neutron enemy (5 in L1). No redneutron.ASE. |
| 3BAL | 2+ | C3DBALLOON, 1balloon | Balloon prop (17 in L2). No balloon.ASE. |
| 3CON | 2+ | C3DCONE | Cone prop (10 in L2). No cone.ASE. |
| 3MER | 1 | C3DMERRYGO | Merry-go-round (1 in L1). No merrygoround.ASE. Maybe RideSpin (objects.omt[17]). |
| 3AIO | 1 | C3DAIOMTOBJ | AI-animated OMT object (3 in L1). Loads an OMT mesh by reference. |
| 3SUV | 1 | C3DSUV | SUV vehicle (3 in L1). No suv.ASE. Maybe car.ASE proxy? |
| 3SWN | 1 | C3DDOORSWING | Door swing (1 in L1). Possibly swing.ASE or a door variant. |

---

## Level1.gam entity coverage summary

Total entities in Level1.gam: ~322

| Status | Count (approx) | FourCCs |
|--------|---------------|---------|
| Already wired | ~200 | 3PAT, 3AIT, 3CAM, 3MCA, 3SOU, 3LIO, STRT, 3ROC, 3ARR, 3BUT, 3DOR, 3CAR, 3BEN, 3HUM, 3FIS, 3PHO, 3HYD, 3TRE, 3DIN, 3FAN, 3SAI, 3SPH, 3OMT, 3PIC, LOAD |
| Can wire now (confident) | ~35 | 3JIM, 3SHE, 3LIB, 3MOM, 3GIR, 3RCK |
| Should mark invisible | ~10 | 3SPR, 3ANI, 3DAI, 3LAS |
| Need Stage B to wire | ~77 | 3NEU, 3LEA, 3RED, 3MER, 3AIO, 3SUV, 3SWN |

Wiring "confident" set reduces placeholder boxes by ~35 entities.
"Need Stage B" set covers 3NEU (23 entities, the biggest win) + 3LEA/3RED.

---

## Next step

⛔ Checkpoint A: Stage B (Ghidra RE of C3DNeutron::InitObject, C3DLeaves, C3DRedNeutron)
will resolve the 9 unknowns above. Suggest HIGH effort.
