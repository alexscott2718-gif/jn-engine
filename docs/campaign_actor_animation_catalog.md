# Campaign Actor Placement + Animation Catalog

Generated: 2026-07-02T20:33:19+00:00

Non-player campaign actor placements from `assets/gam/*.gam`, excluding VR levels.
Positions include native Z, which is the loader-mirrored value used by the engine.

## Totals

- Placements: **123**
- Levels: **23**
- Actor FourCC types: **24**
- With patrol point: **77**
- With talk trigger: **19**
- With visibility/progression gate: **106**
- With native cutscene animation gap: **66**
- Animation aliases tracked: **115**
- ASE aliases with parseable source: **113 / 113**
- Static GLB snapshots present: **9**
- Actor loop previews present: **0**
- Animated sprite previews present: **82**

## Native cutscene animation gaps

Aliases below are present in the placement/decomp actor map but are not yet in `behavior_cutscene.c`'s non-player actor animation switcher.

- `3DIN`: SHRINK, STOP, WALK
- `3FIS`: SHRINK, STOP, WALK
- `3FLE`: SHRINK, STOP, TALK, WALK
- `3GIR`: SHRINK, STOP, WALK
- `3HUM`: ATTACK, GROW, RUNSHRUNK, SHRINK, STOP, STOP2, WALK
- `3KIT`: ATTACK, WALK
- `3NIC`: GLIDE, WAIT
- `3SHE`: PICK, WAVE
- `3SOL`: ATTACK, SHRINK, TALK
- `3SPW`: STOP, WALK
- `3SPY`: ATTACK, BROKE, HELMET, SHRINK, STOP, TALK, WALK
- `3ULT`: FLEX1, FLEX2, FLEX3

## Animation asset readiness

Each row is a unique actor animation alias from the decomp/placement map. `Loop preview` is intentionally `missing` until a real committed preview artifact exists.

| Type | Alias | Asset | Readiness | Textures | Static GLB | Loop preview | Status |
|---|---|---|---|---|---|---|---|
| `3BEN` | `PHONE` | `bennyphone.ASE` | 7 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3BEN` | `STOP` | `bennystop.ASE` | 2 frames @ 5fps; 1 mesh(es), 467 faces | benny.bmp:thumb | `assets/glb/ase/bennystop.glb` | missing | ready_static_no_loop |
| `3BEN` | `TALK` | `bennytalk.ASE` | 7 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3BEN` | `WALK` | `bennywalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3BEN` | `WIPE` | `bennywipe.ASE` | 5 frames @ 4fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3BEN` | `WPHONE` | `bennywipephone.ASE` | 5 frames @ 4fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3CAR` | `CHEER` | `carlcheer.ASE` | 4 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3CAR` | `INHALE` | `carlinhale.ASE` | 5 frames @ 3fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3CAR` | `STOP` | `carlstop.ASE` | 2 frames @ 5fps; 1 mesh(es), 387 faces | carl3.bmp:missing | `assets/glb/ase/carlstop.glb` | missing | ready_static_no_loop |
| `3CAR` | `TALK` | `carltalk.ASE` | 6 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3CAR` | `TELE` | `carlteleport.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3CAR` | `WALK` | `carlwalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3CIN` | `CHEER` | `cindycheer.ASE` | 4 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3CIN` | `STOP` | `cindstop.ASE` | 2 frames @ 5fps; 1 mesh(es), 494 faces | cindy.bmp:thumb | `-` | missing | source_only_no_loop |
| `3CIN` | `TALK` | `cindtalk.ASE` | 7 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3CIN` | `TELE` | `cindteleport.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3CIN` | `WALK` | `cindwalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3CIN` | `WAVE` | `cindwave.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3CML` | `BASE` | `camel.omt` | OMT runtime visual | - | `-` | missing | omt_runtime_visual |
| `3DIN` | `SHRINK` | `dinoshrink.ASE` | 2 frames @ 3fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3DIN` | `STOP` | `dinostop.ASE` | 3 frames @ 10fps; 1 mesh(es), 233 faces | dino.bmp:thumb | `-` | missing | source_only_no_loop |
| `3DIN` | `WALK` | `dinowalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3FIS` | `SHRINK` | `darwinshrink.ASE` | 2 frames @ 3fps; 24 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3FIS` | `STOP` | `darwinstop.ASE` | 2 frames @ 3fps; 1 mesh(es), 224 faces | darwin.bmp:thumb | `-` | missing | source_only_no_loop |
| `3FIS` | `WALK` | `darwinwalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3FLE` | `SHRINK` | `commandershrink.ASE` | 4 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3FLE` | `STOP` | `commanderstop.ASE` | 3 frames @ 3fps; 1 mesh(es), 389 faces | comander.bmp:thumb | `-` | missing | source_only_no_loop |
| `3FLE` | `TALK` | `commandertalk.ASE` | 3 frames @ 3fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3FLE` | `WALK` | `commanderwalk.ASE` | 5 frames @ 3fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3FOW` | `CHEER` | `fowlcheer.ASE` | 4 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3FOW` | `STOP` | `fowlstop.ASE` | 2 frames @ 5fps; 1 mesh(es), 340 faces | fowl.bmp:thumb | `-` | missing | source_only_no_loop |
| `3FOW` | `TALK` | `fowltalk.ASE` | 6 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3FOW` | `TELE` | `fowlteleport.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3FOW` | `WALK` | `fowlwalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GIR` | `SHRINK` | `plantshrink.ASE` | 3 frames @ 3fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GIR` | `STOP` | `plantwait.ASE` | 3 frames @ 3fps; 1 mesh(es), 168 faces | plant.bmp:thumb | `-` | missing | source_only_no_loop |
| `3GIR` | `WALK` | `plantwalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GOD` | `BARK` | `godbark.ASE` | 2 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GOD` | `DEAD` | `goddead.ASE` | 2 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GOD` | `EAT` | `godeat.ASE` | 2 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GOD` | `FLY` | `godfly.ASE` | 3 frames @ 10fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GOD` | `GROWL` | `godgrowl.ASE` | 2 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GOD` | `POINT` | `godpoint.ASE` | 2 frames @ 5fps; 1 mesh(es), 407 faces | goddard02.bmp:thumb | `-` | missing | source_only_no_loop |
| `3GOD` | `ROCKET` | `godrocket.ASE` | 3 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GOD` | `RUN` | `godrun.ASE` | 5 frames @ 10fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GOD` | `SCOOT` | `godscooter.ASE` | 2 frames @ 30fps; 1 mesh(es), 366 faces | goddard02.png:thumb | `-` | missing | source_only_no_loop |
| `3GOD` | `SIT` | `godsit.ASE` | 2 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GOD` | `WAG` | `godwag.ASE` | 3 frames @ 5fps; 1 mesh(es), 407 faces | goddard02.bmp:thumb | `-` | missing | source_only_no_loop |
| `3GUA` | `ATTACK` | `guardatak.ASE` | 2 frames @ 5fps; 1 mesh(es), 156 faces | yokguard.png:thumb | `-` | missing | source_only_no_loop |
| `3GUA` | `SHRINK` | `guardshrink.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3GUA` | `STOP` | `guardatak.ASE` | 2 frames @ 5fps; 1 mesh(es), 156 faces | yokguard.png:thumb | `-` | missing | source_only_no_loop |
| `3GUA` | `WALK` | `guardwalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3HUG` | `COUNT` | `hughcount.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3HUG` | `STOP` | `hughstop.ASE` | 2 frames @ 5fps; 1 mesh(es), 470 faces | hugh.bmp:thumb | `assets/glb/ase/hughstop.glb` | missing | ready_static_no_loop |
| `3HUG` | `TALK` | `hughtalk.ASE` | 6 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3HUG` | `WALK` | `hughwalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3HUM` | `ATTACK` | `humprun.ASE` | 5 frames @ 10fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3HUM` | `GROW` | `humpgrow.ASE` | 6 frames @ 3fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3HUM` | `RUNSHRUNK` | `humprunshrunk.ASE` | 5 frames @ 10fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3HUM` | `SHRINK` | `humpshrink.ASE` | 4 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3HUM` | `STOP` | `humpsleep.ASE` | 3 frames @ 3fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3HUM` | `STOP2` | `humpstop.ASE` | 2 frames @ 5fps; 1 mesh(es), 334 faces | humphrey.bmp:thumb | `assets/glb/ase/humpstop.glb` | missing | ready_static_no_loop |
| `3HUM` | `WALK` | `humpwalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3KIT` | `ATTACK` | `catrun.ASE` | 4 frames @ 10fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3KIT` | `STOP` | `catsit.ASE` | 4 frames @ 5fps; 1 mesh(es), 242 faces | cat.bmp:thumb | `assets/glb/ase/catsit.glb` | missing | ready_static_no_loop |
| `3KIT` | `TALK` | `cattalk.ASE` | 4 frames @ 3fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3KIT` | `WALK` | `catrun.ASE` | 4 frames @ 10fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3LIB` | `PHONE` | `libbyphone.ASE` | 7 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3LIB` | `RUN` | `libyrun.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3LIB` | `STOP` | `libystop.ASE` | 2 frames @ 5fps; 1 mesh(es), 473 faces | libby2.bmp:missing | `assets/glb/ase/libystop.glb` | missing | ready_static_no_loop |
| `3LIB` | `TALK` | `libytalk.ASE` | 7 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3LIB` | `WALK` | `libywalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3LIB` | `WAVE` | `libywave.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3MOM` | `FIX` | `judyfix.ASE` | 4 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3MOM` | `STOP` | `judystop.ASE` | 2 frames @ 5fps; 1 mesh(es), 470 faces | - | `assets/glb/ase/judystop.glb` | missing | ready_static_no_loop |
| `3MOM` | `TALK` | `judytalk.ASE` | 7 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3MOM` | `WALK` | `judywalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3NIC` | `COIN` | `nickcoin.ASE` | 6 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3NIC` | `GLIDE` | `nickskate2.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3NIC` | `SKATE` | `nickskate1.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3NIC` | `STOP` | `nickcoin.ASE` | 6 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3NIC` | `TALK` | `nicktalkboard.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3NIC` | `WAIT` | `nickstop.ASE` | 2 frames @ 5fps; 1 mesh(es), 544 faces | nick.png:thumb | `assets/glb/ase/nickstop.glb` | missing | ready_static_no_loop |
| `3NIC` | `WALK` | `nickwalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3NIC` | `WAVE` | `nickwave.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3PIR` | `DEFAULT` | `viking.ASE` | 101 frames @ 30fps; 1 mesh(es), 322 faces | - | `-` | missing | source_only_no_loop |
| `3SHE` | `PICK` | `shenpick.ASE` | 6 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3SHE` | `STOP` | `shenstop.ASE` | 2 frames @ 5fps; 1 mesh(es), 514 faces | sheen.bmp:thumb | `-` | missing | source_only_no_loop |
| `3SHE` | `TALK` | `shentalk.ASE` | 7 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3SHE` | `WALK` | `shenwalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3SHE` | `WAVE` | `shenwave.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3SOL` | `ATTACK` | `soldatak.ASE` | 2 frames @ 10fps; 1 mesh(es), 339 faces | yoksold.png:thumb | `-` | missing | source_only_no_loop |
| `3SOL` | `SHRINK` | `soldshrink.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3SOL` | `STOP` | `soldatak.ASE` | 2 frames @ 10fps; 1 mesh(es), 339 faces | yoksold.png:thumb | `-` | missing | source_only_no_loop |
| `3SOL` | `TALK` | `soldatak.ASE` | 2 frames @ 10fps; 1 mesh(es), 339 faces | yoksold.png:thumb | `-` | missing | source_only_no_loop |
| `3SOL` | `WALK` | `soldwalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3SPW` | `STOP` | `vulture01.ASE` | 3 frames @ 5fps; 1 mesh(es), 24 faces | vulture.bmp:thumb | `-` | missing | source_only_no_loop |
| `3SPW` | `WALK` | `vulture02.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3SPY` | `ATTACK` | `yokcapatak.ASE` | 2 frames @ 5fps; 1 mesh(es), 450 faces | yokcaptn.bmp:thumb | `-` | missing | source_only_no_loop |
| `3SPY` | `BROKE` | `yokcaptnbroken.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3SPY` | `HELMET` | `yokcaphelm.ASE` | 5 frames @ 5fps; 1 mesh(es), 46 faces | yokcaptn.bmp:thumb | `-` | missing | source_only_no_loop |
| `3SPY` | `SHRINK` | `yokcapshrink.ASE` | 3 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3SPY` | `STOP` | `yokcaptnstop.ASE` | 5 frames @ 5fps; 1 mesh(es), 450 faces | yokcaptn.bmp:thumb | `-` | missing | source_only_no_loop |
| `3SPY` | `TALK` | `yokcaplook.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3SPY` | `WALK` | `yokcaplook.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3SUM` | `DEFAULT` | `sumo.ASE` | 5 frames @ 5fps; 1 mesh(es), 121 faces | sumo.bmp:thumb | `-` | missing | source_only_no_loop |
| `3ULT` | `FLEX1` | `ultraflex.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3ULT` | `FLEX2` | `ultraflex2.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3ULT` | `FLEX3` | `ultraflex3.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3ULT` | `GIVE` | `ultragive.ASE` | 6 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3ULT` | `STOP` | `ultrastop.ASE` | 2 frames @ 5fps; 1 mesh(es), 438 faces | ultralord.bmp:thumb | `assets/glb/ase/ultrastop.glb` | missing | ready_static_no_loop |
| `3ULT` | `TALK` | `ultratalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3ULT` | `WALK` | `ultrawalk.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3ULT` | `WHISPER` | `ultrawhisper.ASE` | 5 frames @ 5fps; 1 mesh(es), 0 faces | - | `-` | missing | source_only_no_loop |
| `3YSH` | `BASE` | `yokianship.omt` | OMT runtime visual | - | `-` | missing | omt_runtime_visual |

## Sprite and item preview readiness

- Sprite canvases indexed: **200**
- Still sprite thumbnails available: **200**
- Animated sprite previews available: **82**
- Item/sprite candidates: **104**

| Database | Chunk | Name | Still thumbnail | Animated preview | Kind |
|---|---:|---|---|---|---|
| `sprites.omt` | 0 | `bneu0000` | `assets/parsed/sprites/sprites_images/0000_100x100d32.png` | `previews/anim/sprites/bneu.webp` | animated_sprite_preview |
| `sprites.omt` | 29 | `ball0000` | `assets/parsed/sprites/sprites_images/0003_32x32d32.png` | `previews/anim/RetainedSprites/ball.webp` | animated_sprite_preview |
| `sprites.omt` | 117 | `wrench` | `assets/parsed/sprites/sprites_images/0004_64x64d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 12 | `neut0013` | `assets/parsed/sprites/sprites_images/0006_100x100d32.png` | `previews/anim/sprites/neut.webp` | animated_sprite_preview |
| `sprites.omt` | 2 | `bneu0002` | `assets/parsed/sprites/sprites_images/0007_100x100d32.png` | `previews/anim/sprites/bneu.webp` | animated_sprite_preview |
| `sprites.omt` | 45 | `leave0000` | `assets/parsed/sprites/sprites_images/0008_64x64d32.png` | `previews/anim/sprites/leave.webp` | animated_sprite_preview |
| `sprites.omt` | 4 | `bneu0004` | `assets/parsed/sprites/sprites_images/0009_100x100d32.png` | `previews/anim/sprites/bneu.webp` | animated_sprite_preview |
| `sprites.omt` | 5 | `bneu0005` | `assets/parsed/sprites/sprites_images/0010_100x100d32.png` | `previews/anim/sprites/bneu.webp` | animated_sprite_preview |
| `sprites.omt` | 6 | `bneu0006` | `assets/parsed/sprites/sprites_images/0011_100x100d32.png` | `previews/anim/sprites/bneu.webp` | animated_sprite_preview |
| `sprites.omt` | 7 | `bneu0007` | `assets/parsed/sprites/sprites_images/0012_100x100d32.png` | `previews/anim/sprites/bneu.webp` | animated_sprite_preview |
| `sprites.omt` | 8 | `neut0009` | `assets/parsed/sprites/sprites_images/0013_100x100d32.png` | `previews/anim/sprites/neut.webp` | animated_sprite_preview |
| `sprites.omt` | 9 | `neut0010` | `assets/parsed/sprites/sprites_images/0014_100x100d32.png` | `previews/anim/sprites/neut.webp` | animated_sprite_preview |
| `sprites.omt` | 10 | `neut0011` | `assets/parsed/sprites/sprites_images/0015_100x100d32.png` | `previews/anim/sprites/neut.webp` | animated_sprite_preview |
| `sprites.omt` | 11 | `neut0012` | `assets/parsed/sprites/sprites_images/0016_100x100d32.png` | `previews/anim/sprites/neut.webp` | animated_sprite_preview |
| `sprites.omt` | 1 | `bneu0001` | `assets/parsed/sprites/sprites_images/0017_100x100d32.png` | `previews/anim/sprites/bneu.webp` | animated_sprite_preview |
| `sprites.omt` | 58 | `coin0002` | `assets/parsed/sprites/sprites_images/0019_64x64d32.png` | `previews/anim/sprites/coin.webp` | animated_sprite_preview |
| `sprites.omt` | 3 | `bneu0003` | `assets/parsed/sprites/sprites_images/0020_100x100d32.png` | `previews/anim/sprites/bneu.webp` | animated_sprite_preview |
| `sprites.omt` | 48 | `leave0003` | `assets/parsed/sprites/sprites_images/0029_64x64d32.png` | `previews/anim/sprites/leave.webp` | animated_sprite_preview |
| `sprites.omt` | 156 | `DogBowl2` | `assets/parsed/sprites/sprites_images/0032_129x100d16.png` | `previews/anim/sprites/DogBowl.webp` | animated_sprite_preview |
| `sprites.omt` | 116 | `retrotickets` | `assets/parsed/sprites/sprites_images/0033_64x64d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 170 | `cookiejarfull` | `assets/parsed/sprites/sprites_images/0036_128x128d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 168 | `candyjaremty` | `assets/parsed/sprites/sprites_images/0037_128x128d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 167 | `candyjarfull` | `assets/parsed/sprites/sprites_images/0038_128x128d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 161 | `cookiejaremty` | `assets/parsed/sprites/sprites_images/0039_128x128d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 154 | `cookies` | `assets/parsed/sprites/sprites_images/0040_128x128d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 158 | `Fruitbowlfull` | `assets/parsed/sprites/sprites_images/0042_128x128d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 157 | `FruitbowlEmty` | `assets/parsed/sprites/sprites_images/0043_128x128d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 169 | `cookiejaremty` | `assets/parsed/sprites/sprites_images/0044_128x128d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 25 | `cans128` | `assets/parsed/sprites/sprites_images/0045_128x128d32.png` | `previews/anim/sprites/cans.webp` | animated_sprite_preview |
| `sprites.omt` | 155 | `DogBowl1` | `assets/parsed/sprites/sprites_images/0046_129x100d16.png` | `previews/anim/sprites/DogBowl.webp` | animated_sprite_preview |
| `sprites.omt` | 162 | `coin` | `assets/parsed/sprites/sprites_images/0050_64x64d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 32 | `ball0003` | `assets/parsed/sprites/sprites_images/0051_32x32d32.png` | `previews/anim/RetainedSprites/ball.webp` | animated_sprite_preview |
| `sprites.omt` | 47 | `leave0002` | `assets/parsed/sprites/sprites_images/0052_64x64d32.png` | `previews/anim/sprites/leave.webp` | animated_sprite_preview |
| `sprites.omt` | 18 | `cans64` | `assets/parsed/sprites/sprites_images/0053_64x64d32.png` | `previews/anim/sprites/cans.webp` | animated_sprite_preview |
| `sprites.omt` | 30 | `ball0001` | `assets/parsed/sprites/sprites_images/0055_32x32d32.png` | `previews/anim/RetainedSprites/ball.webp` | animated_sprite_preview |
| `sprites.omt` | 31 | `ball0002` | `assets/parsed/sprites/sprites_images/0056_32x32d32.png` | `previews/anim/RetainedSprites/ball.webp` | animated_sprite_preview |
| `sprites.omt` | 146 | `pie1` | `assets/parsed/sprites/sprites_images/0060_128x128d16.png` | `previews/anim/sprites/pie.webp` | animated_sprite_preview |
| `sprites.omt` | 38 | `pass` | `assets/parsed/sprites/sprites_images/0063_128x128d32.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 39 | `helmet` | `assets/parsed/sprites/sprites_images/0064_128x128d32.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 86 | `gopher0015` | `assets/parsed/sprites/sprites_images/0066_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 85 | `gopher0014` | `assets/parsed/sprites/sprites_images/0069_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 87 | `candybar` | `assets/parsed/sprites/sprites_images/0070_64x64d32.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 50 | `balloon` | `assets/parsed/sprites/sprites_images/0071_128x128d32.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 51 | `apple` | `assets/parsed/sprites/sprites_images/0072_64x64d32.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 49 | `leave0004` | `assets/parsed/sprites/sprites_images/0073_64x64d32.png` | `previews/anim/sprites/leave.webp` | animated_sprite_preview |
| `sprites.omt` | 133 | `apple0009` | `assets/parsed/sprites/sprites_images/0077_100x100d32.png` | `previews/anim/sprites/apple.webp` | animated_sprite_preview |
| `sprites.omt` | 57 | `coin0001` | `assets/parsed/sprites/sprites_images/0078_64x64d32.png` | `previews/anim/sprites/coin.webp` | animated_sprite_preview |
| `sprites.omt` | 59 | `coin0003` | `assets/parsed/sprites/sprites_images/0079_64x64d32.png` | `previews/anim/sprites/coin.webp` | animated_sprite_preview |
| `sprites.omt` | 52 | `2coin0000` | `assets/parsed/sprites/sprites_images/0080_64x64d32.png` | `previews/anim/sprites/2coin.webp` | animated_sprite_preview |
| `sprites.omt` | 53 | `2coin0001` | `assets/parsed/sprites/sprites_images/0081_64x64d32.png` | `previews/anim/sprites/2coin.webp` | animated_sprite_preview |
| `sprites.omt` | 54 | `2coin0002` | `assets/parsed/sprites/sprites_images/0082_64x64d32.png` | `previews/anim/sprites/2coin.webp` | animated_sprite_preview |
| `sprites.omt` | 55 | `2coin0003` | `assets/parsed/sprites/sprites_images/0083_64x64d32.png` | `previews/anim/sprites/2coin.webp` | animated_sprite_preview |
| `sprites.omt` | 56 | `coin0000` | `assets/parsed/sprites/sprites_images/0084_64x64d32.png` | `previews/anim/sprites/coin.webp` | animated_sprite_preview |
| `sprites.omt` | 60 | `fount1` | `assets/parsed/sprites/sprites_images/0085_128x128d32.png` | `previews/anim/sprites/fount.webp` | animated_sprite_preview |
| `sprites.omt` | 61 | `fount2` | `assets/parsed/sprites/sprites_images/0086_128x128d32.png` | `previews/anim/sprites/fount.webp` | animated_sprite_preview |
| `sprites.omt` | 62 | `fount3` | `assets/parsed/sprites/sprites_images/0087_128x128d32.png` | `previews/anim/sprites/fount.webp` | animated_sprite_preview |
| `sprites.omt` | 64 | `trees4` | `assets/parsed/sprites/sprites_images/0089_257x256d32.png` | `previews/anim/sprites/trees.webp` | animated_sprite_preview |
| `sprites.omt` | 65 | `trees5` | `assets/parsed/sprites/sprites_images/0090_256x256d32.png` | `previews/anim/sprites/trees.webp` | animated_sprite_preview |
| `sprites.omt` | 66 | `trees6` | `assets/parsed/sprites/sprites_images/0091_256x256d32.png` | `previews/anim/sprites/trees.webp` | animated_sprite_preview |
| `sprites.omt` | 46 | `leave0001` | `assets/parsed/sprites/sprites_images/0094_64x64d32.png` | `previews/anim/sprites/leave.webp` | animated_sprite_preview |
| `sprites.omt` | 71 | `candy01` | `assets/parsed/sprites/sprites_images/0098_64x64d32.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 75 | `gopher0000` | `assets/parsed/sprites/sprites_images/0102_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 76 | `gopher0001` | `assets/parsed/sprites/sprites_images/0103_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 77 | `gopher0002` | `assets/parsed/sprites/sprites_images/0104_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 78 | `gopher0003` | `assets/parsed/sprites/sprites_images/0105_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 79 | `gopher0004` | `assets/parsed/sprites/sprites_images/0106_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 80 | `gopher0005` | `assets/parsed/sprites/sprites_images/0107_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 81 | `gopher0006` | `assets/parsed/sprites/sprites_images/0108_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 82 | `gopher0007` | `assets/parsed/sprites/sprites_images/0109_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 83 | `gopher0009` | `assets/parsed/sprites/sprites_images/0110_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 84 | `gopher0011` | `assets/parsed/sprites/sprites_images/0111_64x64d32.png` | `previews/anim/sprites/gopher.webp` | animated_sprite_preview |
| `sprites.omt` | 88 | `minexp0000` | `assets/parsed/sprites/sprites_images/0112_128x128d32.png` | `previews/anim/sprites/minexp.webp` | animated_sprite_preview |
| `sprites.omt` | 89 | `minexp0001` | `assets/parsed/sprites/sprites_images/0116_128x128d32.png` | `previews/anim/sprites/minexp.webp` | animated_sprite_preview |
| `sprites.omt` | 90 | `minexp0002` | `assets/parsed/sprites/sprites_images/0117_128x128d32.png` | `previews/anim/sprites/minexp.webp` | animated_sprite_preview |
| `sprites.omt` | 91 | `minexp0003` | `assets/parsed/sprites/sprites_images/0118_128x128d32.png` | `previews/anim/sprites/minexp.webp` | animated_sprite_preview |
| `sprites.omt` | 92 | `minexp0004` | `assets/parsed/sprites/sprites_images/0119_128x128d32.png` | `previews/anim/sprites/minexp.webp` | animated_sprite_preview |
| `sprites.omt` | 94 | `minexp0006` | `assets/parsed/sprites/sprites_images/0120_128x128d32.png` | `previews/anim/sprites/minexp.webp` | animated_sprite_preview |
| `sprites.omt` | 95 | `minexp0007` | `assets/parsed/sprites/sprites_images/0121_128x128d32.png` | `previews/anim/sprites/minexp.webp` | animated_sprite_preview |
| `sprites.omt` | 96 | `minexp0008` | `assets/parsed/sprites/sprites_images/0122_128x128d32.png` | `previews/anim/sprites/minexp.webp` | animated_sprite_preview |
| `sprites.omt` | 93 | `minexp0005` | `assets/parsed/sprites/sprites_images/0123_128x128d32.png` | `previews/anim/sprites/minexp.webp` | animated_sprite_preview |
| `sprites.omt` | 103 | `trees7` | `assets/parsed/sprites/sprites_images/0127_128x128d32.png` | `previews/anim/sprites/trees.webp` | animated_sprite_preview |
| `sprites.omt` | 187 | `Gem3` | `assets/parsed/sprites/sprites_images/0136_128x127d16.png` | `previews/anim/sprites/Gem.webp` | animated_sprite_preview |
| `sprites.omt` | 188 | `Gem2` | `assets/parsed/sprites/sprites_images/0137_128x127d16.png` | `previews/anim/sprites/Gem.webp` | animated_sprite_preview |
| `sprites.omt` | 189 | `Gem1` | `assets/parsed/sprites/sprites_images/0138_128x127d16.png` | `previews/anim/sprites/Gem.webp` | animated_sprite_preview |
| `sprites.omt` | 122 | `tumbleweed3` | `assets/parsed/sprites/sprites_images/0146_100x100d32.png` | `previews/anim/sprites/tumbleweed.webp` | animated_sprite_preview |
| `sprites.omt` | 123 | `tumbleweed2` | `assets/parsed/sprites/sprites_images/0147_100x100d32.png` | `previews/anim/sprites/tumbleweed.webp` | animated_sprite_preview |
| `sprites.omt` | 124 | `apple0000` | `assets/parsed/sprites/sprites_images/0148_100x100d32.png` | `previews/anim/sprites/apple.webp` | animated_sprite_preview |
| `sprites.omt` | 125 | `apple0001` | `assets/parsed/sprites/sprites_images/0149_100x100d32.png` | `previews/anim/sprites/apple.webp` | animated_sprite_preview |
| `sprites.omt` | 126 | `apple0002` | `assets/parsed/sprites/sprites_images/0150_100x100d32.png` | `previews/anim/sprites/apple.webp` | animated_sprite_preview |
| `sprites.omt` | 127 | `apple0003` | `assets/parsed/sprites/sprites_images/0151_100x100d32.png` | `previews/anim/sprites/apple.webp` | animated_sprite_preview |
| `sprites.omt` | 128 | `apple0004` | `assets/parsed/sprites/sprites_images/0152_100x100d32.png` | `previews/anim/sprites/apple.webp` | animated_sprite_preview |
| `sprites.omt` | 129 | `apple0005` | `assets/parsed/sprites/sprites_images/0153_100x100d32.png` | `previews/anim/sprites/apple.webp` | animated_sprite_preview |
| `sprites.omt` | 130 | `apple0005` | `assets/parsed/sprites/sprites_images/0154_100x100d32.png` | `previews/anim/sprites/apple.webp` | animated_sprite_preview |
| `sprites.omt` | 131 | `apple0006` | `assets/parsed/sprites/sprites_images/0155_100x100d32.png` | `previews/anim/sprites/apple.webp` | animated_sprite_preview |
| `sprites.omt` | 132 | `apple0007` | `assets/parsed/sprites/sprites_images/0156_100x100d32.png` | `previews/anim/sprites/apple.webp` | animated_sprite_preview |
| `sprites.omt` | 22 | `pie2` | `assets/parsed/sprites/sprites_images/0169_128x128d16.png` | `previews/anim/sprites/pie.webp` | animated_sprite_preview |
| `sprites.omt` | 150 | `fruitbowl2` | `assets/parsed/sprites/sprites_images/0170_128x128d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 147 | `Fruitbowl1` | `assets/parsed/sprites/sprites_images/0171_128x128d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 143 | `cookiejarclosed` | `assets/parsed/sprites/sprites_images/0179_128x128d16.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 172 | `fishbowl` | `assets/parsed/sprites/sprites_images/0183_128x128d32.png` | `-` | still_sprite_thumbnail |
| `sprites.omt` | 171 | `fishbowl2` | `assets/parsed/sprites/sprites_images/0184_128x128d32.png` | `previews/anim/sprites/fishbowl.webp` | animated_sprite_preview |
| `sprites.omt` | 181 | `fishbowl3` | `assets/parsed/sprites/sprites_images/0185_128x128d32.png` | `previews/anim/sprites/fishbowl.webp` | animated_sprite_preview |
| `sprites.omt` | 182 | `fishbowl4` | `assets/parsed/sprites/sprites_images/0186_128x128d32.png` | `previews/anim/sprites/fishbowl.webp` | animated_sprite_preview |
| `icons.omt` | 6 | `apple` | `assets/parsed/icons/icons_images/0006_32x32d32.png` | `-` | still_sprite_thumbnail |

## Preview pipeline decision

Selected: **engine-driven actor clip capture**

Status: decision-recorded; preview artifact generation not implemented in this catalog patch

Rationale:

- The native engine already resolves ASE clips, textures, frame timing, and actor orientation.
- Extending ASE-to-GLB with animation timelines would duplicate runtime animation logic.
- A browser ASE previewer would add a second renderer/parsing path for the same question.

Next step: Add a small headless engine capture mode that loads one actor clip, loops it for a fixed duration, and emits a WebP/PNG strip under a committed preview path.

## Placements

| Level | Type | Tag | Native position | Patrol | Talk triggers | Gates | Native gap | Animations |
|---|---|---|---:|---|---|---|---|---|
| `level1` | `3SHE` C3DSheen | `Sheen1` | 8967.2, 35.8, -6992.0 | `shn1` | sewerpart | RequiredLevel=0, TaskName=Scene | WAVE, PICK | STOP:shenstop.ASE, WALK:shenwalk.ASE, TALK:shentalk.ASE, WAVE:shenwave.ASE, PICK:shenpick.ASE |
| `level1` | `3LIB` C3DLibby | `C3DLIBBY` | 10092.9, 36.5, 914.9 | `Lib1` | seecindy, seesheen, sheencandybar | RequiredLevel=0, TaskName=Scene | - | STOP:libystop.ASE, WALK:libywalk.ASE, RUN:libyrun.ASE, TALK:libytalk.ASE, PHONE:libbyphone.ASE, WAVE:libywave.ASE |
| `level1` | `3MOM` C3DJudy | `C3DJUDY` | 8159.1, 25.6, -1384.3 | `JUDY1A` | requestkey, gotkey, withwrench | RequiredLevel=200, RemoveLevel=250, TaskName=Scene | - | STOP:judystop.ASE, WALK:judywalk.ASE, TALK:judytalk.ASE, FIX:judyfix.ASE |
| `level1` | `3CAR` C3DCarl | `C3DCARL` | 6317.0, 4.3, -4414.9 | `carl1` | neutron1b, neutron1c | RequiredLevel=0, RemoveLevel=90, TaskName=scene | - | STOP:carlstop.ASE, WALK:carlwalk.ASE, TALK:carltalk.ASE, TELE:carlteleport.ASE, CHEER:carlcheer.ASE, INHALE:carlinhale.ASE |
| `level1` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | 8187.6, 11.5, -5952.6 | `d1` | - | RequiredLevel=10, TaskName=clone | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level1` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | 8696.3, 17.4, -7032.4 | `d2` | - | RequiredLevel=10, TaskName=clone | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level1` | `3GIR` C3DGirlEatingPlant | `libbyplant` | 10121.2, 10.0, 328.2 | `-` | - | RequiredLevel=0, ExactLevel=250, RemoveLevel=255, TaskName=scene | STOP, WALK, SHRINK | STOP:plantwait.ASE, WALK:plantwalk.ASE, SHRINK:plantshrink.ASE |
| `level1` | `3GIR` C3DGirlEatingPlant | `C3DGIRLEATINGPLANT` | 6859.5, -8.4, -11506.4 | `gep1` | - | RequiredLevel=240, RemoveLevel=280, TaskName=scene | STOP, WALK, SHRINK | STOP:plantwait.ASE, WALK:plantwalk.ASE, SHRINK:plantshrink.ASE |
| `level1` | `3GIR` C3DGirlEatingPlant | `df` | 10987.5, 8.1, -6846.9 | `gep2` | - | RequiredLevel=255, RemoveLevel=280, TaskName=scene | STOP, WALK, SHRINK | STOP:plantwait.ASE, WALK:plantwalk.ASE, SHRINK:plantshrink.ASE |
| `level1` | `3GIR` C3DGirlEatingPlant | `cd` | 4956.0, 5.8, -711.3 | `gep3` | - | RequiredLevel=255, RemoveLevel=280, TaskName=scene | STOP, WALK, SHRINK | STOP:plantwait.ASE, WALK:plantwalk.ASE, SHRINK:plantshrink.ASE |
| `level1` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | 9074.3, 8.2, -3444.8 | `d3` | - | RequiredLevel=120, RemoveLevel=210, TaskName=scene | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level1` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | 2652.4, 4.5, -2154.0 | `d4` | - | RequiredLevel=10, TaskName=clone | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level1` | `3BEN` C3DBenny | `Benny1` | 14222.8, 23.8, 14389.8 | `-` | - | RequiredLevel=90, RemoveLevel=100, TaskName=scene | - | STOP:bennystop.ASE, WALK:bennywalk.ASE, TALK:bennytalk.ASE, PHONE:bennyphone.ASE, WIPE:bennywipe.ASE, WPHONE:bennywipephone.ASE |
| `level1` | `3HUM` C3DHumphrey | `preclone` | 6140.4, 39.5, 3625.4 | `-` | - | RequiredLevel=0, TaskName=scene | STOP, STOP2, WALK, ATTACK, SHRINK, RUNSHRUNK, GROW | STOP:humpsleep.ASE, WALK:humpwalk.ASE, ATTACK:humprun.ASE, SHRINK:humpshrink.ASE, STOP2:humpstop.ASE, RUNSHRUNK:humprunshrunk.ASE, GROW:humpgrow.ASE |
| `level1` | `3HUM` C3DHumphrey | `clone1` | 6907.2, 32.4, 2638.1 | `GETOUT1` | - | RequiredLevel=10, TaskName=clone | STOP, STOP2, WALK, ATTACK, SHRINK, RUNSHRUNK, GROW | STOP:humpsleep.ASE, WALK:humpwalk.ASE, ATTACK:humprun.ASE, SHRINK:humpshrink.ASE, STOP2:humpstop.ASE, RUNSHRUNK:humprunshrunk.ASE, GROW:humpgrow.ASE |
| `level1` | `3HUM` C3DHumphrey | `clone2` | 8057.5, 37.2, 2829.0 | `GETOUT1` | - | RequiredLevel=10, TaskName=clone | STOP, STOP2, WALK, ATTACK, SHRINK, RUNSHRUNK, GROW | STOP:humpsleep.ASE, WALK:humpwalk.ASE, ATTACK:humprun.ASE, SHRINK:humpshrink.ASE, STOP2:humpstop.ASE, RUNSHRUNK:humprunshrunk.ASE, GROW:humpgrow.ASE |
| `level1` | `3HUM` C3DHumphrey | `clone4` | 7398.5, 35.6, 3560.5 | `GETOUT1` | - | RequiredLevel=10, TaskName=clone | STOP, STOP2, WALK, ATTACK, SHRINK, RUNSHRUNK, GROW | STOP:humpsleep.ASE, WALK:humpwalk.ASE, ATTACK:humprun.ASE, SHRINK:humpshrink.ASE, STOP2:humpstop.ASE, RUNSHRUNK:humprunshrunk.ASE, GROW:humpgrow.ASE |
| `level1b` | `3CAR` C3DCarl | `C3DCARL` | 4567.5, 37.0, -589.8 | `CARL1` | neutron1a | RequiredLevel=0, TaskName=SCENE | - | STOP:carlstop.ASE, WALK:carlwalk.ASE, TALK:carltalk.ASE, TELE:carlteleport.ASE, CHEER:carlcheer.ASE, INHALE:carlinhale.ASE |
| `level1b` | `3GIR` C3DGirlEatingPlant | `C3DGIRLEATINGPLANT` | 2623.7, -582.8, -2345.1 | `-` | - | RequiredLevel=0 | STOP, WALK, SHRINK | STOP:plantwait.ASE, WALK:plantwalk.ASE, SHRINK:plantshrink.ASE |
| `level1c` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | -1053.0, 8.0, 843.3 | `-` | - | RequiredLevel=220, TaskName=scene | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level1c` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | -1992.9, 638.8, -1196.0 | `-` | - | RequiredLevel=240, TaskName=scene | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level1d` | `3YSH` C3DYokianShip | `C3DYOKIANSHIP` | -3252.4, 26586.3, 21743.1 | `SHIP6PT` | - | - | - | BASE:yokianship.omt |
| `level1d` | `3YSH` C3DYokianShip | `SHIP1` | -330.7, 25193.7, 15879.9 | `SHIP1PT` | - | - | - | BASE:yokianship.omt |
| `level1d` | `3YSH` C3DYokianShip | `sh2` | 2338.7, 26236.7, 21742.1 | `SHIP2PT` | - | - | - | BASE:yokianship.omt |
| `level1d` | `3YSH` C3DYokianShip | `C3DYOKIANSHIP` | -1691.5, 23644.1, 20868.9 | `SHIP3PT` | - | - | - | BASE:yokianship.omt |
| `level1d` | `3FLE` C3DFleetCommander | `goobar` | 16.6, -4068.0, 3364.8 | `fc1` | - | RequiredLevel=0, TaskName=scene | STOP, WALK, TALK, SHRINK | STOP:commanderstop.ASE, WALK:commanderwalk.ASE, TALK:commandertalk.ASE, SHRINK:commandershrink.ASE |
| `level1d` | `3SPY` C3DYokianSpy | `lackie` | -485.8, -4086.8, 897.5 | `-` | - | RequiredLevel=0, TaskName=scene | ATTACK, WALK, TALK, SHRINK, BROKE, STOP, HELMET | STOP:yokcaptnstop.ASE, WALK:yokcaplook.ASE, TALK:yokcaplook.ASE, ATTACK:yokcapatak.ASE, SHRINK:yokcapshrink.ASE, BROKE:yokcaptnbroken.ASE, HELMET:yokcaphelm.ASE |
| `level1d` | `3SOL` C3DYokianSoldier | `soldier1` | -3002.1, -4157.4, -370.9 | `s11` | - | RequiredLevel=400, RemoveLevel=0, TaskName=scene | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level1e` | `3FLE` C3DFleetCommander | `goobar` | 46.3, -4090.6, 1284.1 | `fc1` | - | RequiredLevel=0, TaskName=scene | STOP, WALK, TALK, SHRINK | STOP:commanderstop.ASE, WALK:commanderwalk.ASE, TALK:commandertalk.ASE, SHRINK:commandershrink.ASE |
| `level1e` | `3CAR` C3DCarl | `C3DCARL` | -1576.3, -4088.7, -385.8 | `crl1` | - | RequiredLevel=0, ExactLevel=390, TaskName=scene | - | STOP:carlstop.ASE, WALK:carlwalk.ASE, TALK:carltalk.ASE, TELE:carlteleport.ASE, CHEER:carlcheer.ASE, INHALE:carlinhale.ASE |
| `level1e` | `3SOL` C3DYokianSoldier | `soldier1` | -1103.7, -4096.8, -324.2 | `s1` | - | ExactLevel=390, TaskName=scene | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level1e` | `3GUA` C3DYokianGuard | `soldier2` | -1988.6, -4125.8, -407.2 | `sol1` | - | ExactLevel=390, TaskName=scene | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level1e` | `3CIN` C3DCindy | `C3DCINDY` | 524.4, -4150.4, -209.6 | `cin2` | - | ExactLevel=420, TaskName=scene | - | STOP:cindstop.ASE, WALK:cindwalk.ASE, TALK:cindtalk.ASE, TELE:cindteleport.ASE, CHEER:cindycheer.ASE, WAVE:cindwave.ASE |
| `level1e` | `3SOL` C3DYokianSoldier | `yokcin` | 275.5, -4066.8, -250.7 | `-` | - | ExactLevel=420, TaskName=scene | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level1e` | `3SPY` C3DYokianSpy | `captain` | -379.9, -4076.6, 766.8 | `-` | - | RemoveLevel=410, TaskName=scene | ATTACK, WALK, TALK, SHRINK, BROKE, STOP, HELMET | STOP:yokcaptnstop.ASE, WALK:yokcaplook.ASE, TALK:yokcaplook.ASE, ATTACK:yokcapatak.ASE, SHRINK:yokcapshrink.ASE, BROKE:yokcaptnbroken.ASE, HELMET:yokcaphelm.ASE |
| `level1e` | `3SOL` C3DYokianSoldier | `second` | 334.5, -4052.8, 746.6 | `-` | - | ExactLevel=420, TaskName=scene | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level1f` | `3YSH` C3DYokianShip | `yokship6` | 1254.1, 12300.7, -79879.3 | `ship6end` | - | - | - | BASE:yokianship.omt |
| `level1f` | `3YSH` C3DYokianShip | `yokship1` | -1909.2, 11738.0, -80506.0 | `ship1end` | - | - | - | BASE:yokianship.omt |
| `level1f` | `3YSH` C3DYokianShip | `yokship2` | -1452.6, 16755.5, -80238.2 | `ship2end` | - | - | - | BASE:yokianship.omt |
| `level1f` | `3YSH` C3DYokianShip | `yokship3` | 3569.9, 16836.7, -80365.5 | `ship3end` | - | - | - | BASE:yokianship.omt |
| `level2` | `3BEN` C3DBenny | `C3DBENNY` | 635.3, 12.2, 1124.8 | `ben1` | goinside, benwalk, gototrack, gohome, libbysheen | RequiredLevel=0, TaskName=scene | - | STOP:bennystop.ASE, WALK:bennywalk.ASE, TALK:bennytalk.ASE, PHONE:bennyphone.ASE, WIPE:bennywipe.ASE, WPHONE:bennywipephone.ASE |
| `level2` | `3NIC` C3DNick | `Nick2` | -1214.0, 303.7, 8252.7 | `NICPAT1` | race2, race2again, winrace2 | RequiredLevel=140, RemoveLevel=162, TaskName=Scene | WAIT, GLIDE | STOP:nickcoin.ASE, WALK:nickwalk.ASE, TALK:nicktalkboard.ASE, WAIT:nickstop.ASE, SKATE:nickskate1.ASE, GLIDE:nickskate2.ASE, WAVE:nickwave.ASE, COIN:nickcoin.ASE |
| `level2` | `3CAR` C3DCarl | `Carl3` | 6897.1, 2.3, -2207.3 | `look1` | inhaler | RequiredLevel=270, RemoveLevel=290, TaskName=Scene | - | STOP:carlstop.ASE, WALK:carlwalk.ASE, TALK:carltalk.ASE, TELE:carlteleport.ASE, CHEER:carlcheer.ASE, INHALE:carlinhale.ASE |
| `level2` | `3SHE` C3DSheen | `Sheen2` | -138.9, 18.5, -2206.3 | `walks1` | exchange | RequiredLevel=0, TaskName=Scene | WAVE, PICK | STOP:shenstop.ASE, WALK:shenwalk.ASE, TALK:shentalk.ASE, WAVE:shenwave.ASE, PICK:shenpick.ASE |
| `level2` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | 5874.0, 11.9, -364.3 | `-` | - | RequiredLevel=0, TaskName=scene | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level2` | `3HUM` C3DHumphrey | `C3DHUMPHREY` | 660.8, 2.7, 5266.7 | `-` | - | RequiredLevel=140, RemoveLevel=180, TaskName=clone | STOP, STOP2, WALK, ATTACK, SHRINK, RUNSHRUNK, GROW | STOP:humpsleep.ASE, WALK:humpwalk.ASE, ATTACK:humprun.ASE, SHRINK:humpshrink.ASE, STOP2:humpstop.ASE, RUNSHRUNK:humprunshrunk.ASE, GROW:humpgrow.ASE |
| `level2` | `3DIN` C3DDino | `C3DDINO` | -1298.8, 30.2, -1061.4 | `-` | - | RequiredLevel=260, RemoveLevel=460, TaskName=scene | STOP, WALK, SHRINK | STOP:dinostop.ASE, WALK:dinowalk.ASE, SHRINK:dinoshrink.ASE |
| `level2` | `3HUM` C3DHumphrey | `C3DHUMPHREY` | 3754.3, 14.7, -6513.1 | `-` | - | RequiredLevel=200, RemoveLevel=300, TaskName=clone | STOP, STOP2, WALK, ATTACK, SHRINK, RUNSHRUNK, GROW | STOP:humpsleep.ASE, WALK:humpwalk.ASE, ATTACK:humprun.ASE, SHRINK:humpshrink.ASE, STOP2:humpstop.ASE, RUNSHRUNK:humprunshrunk.ASE, GROW:humpgrow.ASE |
| `level2` | `3KIT` C3DKitty | `C3DKITTY` | 3615.9, 817.7, -1662.6 | `runpuss1` | - | InitiallyVisible=0, RequiredLevel=0, ExactLevel=0, TaskName=kitty2 | WALK, ATTACK | STOP:catsit.ASE, WALK:catrun.ASE, TALK:cattalk.ASE, ATTACK:catrun.ASE |
| `level2` | `3HUM` C3DHumphrey | `C3DHUMPHREY` | 4196.0, 51.1, -2159.7 | `-` | - | RequiredLevel=260, RemoveLevel=290, TaskName=scene | STOP, STOP2, WALK, ATTACK, SHRINK, RUNSHRUNK, GROW | STOP:humpsleep.ASE, WALK:humpwalk.ASE, ATTACK:humprun.ASE, SHRINK:humpshrink.ASE, STOP2:humpstop.ASE, RUNSHRUNK:humprunshrunk.ASE, GROW:humpgrow.ASE |
| `level2a` | `3NIC` C3DNick | `Nick1` | -184.4, 2.5, -969.9 | `NICPAT1` | race1, loserace1, winrace1, loserace1, dirtrace | RequiredLevel=0, RemoveLevel=140, TaskName=SCENE | WAIT, GLIDE | STOP:nickcoin.ASE, WALK:nickwalk.ASE, TALK:nicktalkboard.ASE, WAIT:nickstop.ASE, SKATE:nickskate1.ASE, GLIDE:nickskate2.ASE, WAVE:nickwave.ASE, COIN:nickcoin.ASE |
| `level2a` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | 5530.1, 962.4, 572.1 | `-` | - | RequiredLevel=140, TaskName=scene | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level2a` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | 2573.4, 936.3, -2373.4 | `d3` | - | RequiredLevel=140, TaskName=scene | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level2a` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | -5537.0, 896.8, 472.8 | `d1` | - | RequiredLevel=140, TaskName=scene | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level2a` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | -4839.8, 4.5, -2359.1 | `d2` | - | RequiredLevel=140, TaskName=scene | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level2a` | `3HUM` C3DHumphrey | `C3DHUMPHREY` | 5300.7, 31.5, -3057.1 | `-` | - | RequiredLevel=140, RemoveLevel=180, TaskName=scene | STOP, STOP2, WALK, ATTACK, SHRINK, RUNSHRUNK, GROW | STOP:humpsleep.ASE, WALK:humpwalk.ASE, ATTACK:humprun.ASE, SHRINK:humpshrink.ASE, STOP2:humpstop.ASE, RUNSHRUNK:humprunshrunk.ASE, GROW:humpgrow.ASE |
| `level2a` | `3HUM` C3DHumphrey | `C3DHUMPHREY` | -6175.2, 921.8, -2752.3 | `-` | - | RequiredLevel=180, RemoveLevel=220, TaskName=scene | STOP, STOP2, WALK, ATTACK, SHRINK, RUNSHRUNK, GROW | STOP:humpsleep.ASE, WALK:humpwalk.ASE, ATTACK:humprun.ASE, SHRINK:humpshrink.ASE, STOP2:humpstop.ASE, RUNSHRUNK:humprunshrunk.ASE, GROW:humpgrow.ASE |
| `level2b` | `3NIC` C3DNick | `Nick2` | -1483.8, 337.4, 9307.4 | `NICPAT1` | race2, race2again, winrace2, race2again, race2 | RequiredLevel=0, TaskName=Scene | WAIT, GLIDE | STOP:nickcoin.ASE, WALK:nickwalk.ASE, TALK:nicktalkboard.ASE, WAIT:nickstop.ASE, SKATE:nickskate1.ASE, GLIDE:nickskate2.ASE, WAVE:nickwave.ASE, COIN:nickcoin.ASE |
| `level2b` | `3DIN` C3DDino | `C3DDINO` | -1298.8, 30.2, -1061.4 | `-` | - | RequiredLevel=260, RemoveLevel=460, TaskName=scene | STOP, WALK, SHRINK | STOP:dinostop.ASE, WALK:dinowalk.ASE, SHRINK:dinoshrink.ASE |
| `level3` | `3SUM` C3DSumo | `C3DSUMO` | -2828.7, 57.9, -4855.8 | `-` | - | RequiredLevel=0 | - | DEFAULT:sumo.ASE |
| `level3` | `3ULT` C3DUltraLord | `C3DULTRALORD` | -2172.3, 99.1, -7854.6 | `ultra1` | ultralord, getfuel | RequiredLevel=0, TaskName=Scene | FLEX1, FLEX2, FLEX3 | STOP:ultrastop.ASE, WALK:ultrawalk.ASE, TALK:ultratalk.ASE, GIVE:ultragive.ASE, FLEX1:ultraflex.ASE, FLEX2:ultraflex2.ASE, FLEX3:ultraflex3.ASE, WHISPER:ultrawhisper.ASE |
| `level3` | `3CAR` C3DCarl | `Carl4` | -1392.3, 4.9, -5277.7 | `crl1` | getstaken | RequiredLevel=380, RemoveLevel=400, TaskName=scene | - | STOP:carlstop.ASE, WALK:carlwalk.ASE, TALK:carltalk.ASE, TELE:carlteleport.ASE, CHEER:carlcheer.ASE, INHALE:carlinhale.ASE |
| `level3a` | `3CML` C3DCamel | `C3DCAMEL` | 44.5, 2536.1, -3959.1 | `CAM1` | - | RequiredLevel=0 | - | BASE:camel.omt |
| `level3c` | `3LIB` C3DLibby | `libby2` | 59.3, 4.7, -1039.7 | `lib1` | needcindy | RequiredLevel=400, RemoveLevel=420, TaskName=Scene | - | STOP:libystop.ASE, WALK:libywalk.ASE, RUN:libyrun.ASE, TALK:libytalk.ASE, PHONE:libbyphone.ASE, WAVE:libywave.ASE |
| `level3c` | `3SUM` C3DSumo | `C3DSUMO` | -1723.7, -68.7, 4214.3 | `-` | - | TaskName=scene | - | DEFAULT:sumo.ASE |
| `level3d` | `3SUM` C3DSumo | `C3DSUMO` | -9520.9, -108.0, -1647.8 | `-` | - | RequiredLevel=0 | - | DEFAULT:sumo.ASE |
| `level3d` | `3PIR` C3DPirate | `C3DPIRATE` | 448.3, 1163.5, -3032.2 | `-` | - | RequiredLevel=0 | - | DEFAULT:viking.ASE |
| `level3d` | `3CIN` C3DCindy | `C3DCINDY` | 858.6, 4.7, -388.9 | `c1` | needpasscard | RequiredLevel=0, ExactLevel=410, RemoveLevel=420, TaskName=scene | - | STOP:cindstop.ASE, WALK:cindwalk.ASE, TALK:cindtalk.ASE, TELE:cindteleport.ASE, CHEER:cindycheer.ASE, WAVE:cindwave.ASE |
| `level4` | `3CIN` C3DCindy | `C3DCINDY` | 2137.4, 70.4, -4944.7 | `CINDY1` | getclaw | RequiredLevel=0, TaskName=Scene | - | STOP:cindstop.ASE, WALK:cindwalk.ASE, TALK:cindtalk.ASE, TELE:cindteleport.ASE, CHEER:cindycheer.ASE, WAVE:cindwave.ASE |
| `level4` | `3YSH` C3DYokianShip | `C3DYOKIANSHIP` | 19137.0, 3519.0, -3885.2 | `YOKSHIP1` | - | RequiredLevel=380, TaskName=scene | - | BASE:yokianship.omt |
| `level4` | `3SHE` C3DSheen | `sheen3` | 5751.2, 89.7, -3039.0 | `shn1` | givetickets | RequiredLevel=360, RemoveLevel=380, TaskName=scene | WAVE, PICK | STOP:shenstop.ASE, WALK:shenwalk.ASE, TALK:shentalk.ASE, WAVE:shenwave.ASE, PICK:shenpick.ASE |
| `level4` | `3FIS` C3DDarwinFish | `C3DDARWINFISH` | 1609.3, 13.8, -6322.6 | `-` | - | RequiredLevel=0 | STOP, WALK, SHRINK | STOP:darwinstop.ASE, WALK:darwinwalk.ASE, SHRINK:darwinshrink.ASE |
| `level4` | `3KIT` C3DKitty | `C3DKITTY` | 7008.3, 791.1, -5093.0 | `cat1` | - | InitiallyVisible=1, ExactLevel=0, TaskName=kitty1 | WALK, ATTACK | STOP:catsit.ASE, WALK:catrun.ASE, TALK:cattalk.ASE, ATTACK:catrun.ASE |
| `level4a` | `3HUG` C3DHugh | `C3DHUGH` | 871.8, 21.0, 1144.8 | `HUGH1A` | gotkey | RequiredLevel=100, TaskName=Scene | - | STOP:hughstop.ASE, WALK:hughwalk.ASE, TALK:hughtalk.ASE, COUNT:hughcount.ASE |
| `level4c` | `3FOW` C3DFowl | `C3DFOWL` | -457.3, 13.4, -6813.3 | `fowl1` | powerplant | RequiredLevel=460, RemoveLevel=470, TaskName=scene | - | STOP:fowlstop.ASE, WALK:fowlwalk.ASE, TALK:fowltalk.ASE, TELE:fowlteleport.ASE, CHEER:fowlcheer.ASE |
| `level4c` | `3YSH` C3DYokianShip | `C3DYOKIANSHIP` | -3335.4, 5247.8, -11438.0 | `-` | - | TaskName=scene | - | BASE:yokianship.omt |
| `level4d` | `3MOM` C3DJudy | `C3DJUDY` | 437.0, 11.8, 1128.7 | `-` | - | RequiredLevel=0, TaskName=scene | - | STOP:judystop.ASE, WALK:judywalk.ASE, TALK:judytalk.ASE, FIX:judyfix.ASE |
| `level4d` | `3CIN` C3DCindy | `C3DCINDY` | -791.6, 1.1, 86.1 | `-` | - | RequiredLevel=0, TaskName=scene | - | STOP:cindstop.ASE, WALK:cindwalk.ASE, TALK:cindtalk.ASE, TELE:cindteleport.ASE, CHEER:cindycheer.ASE, WAVE:cindwave.ASE |
| `level4d` | `3HUM` C3DHumphrey | `C3DHUMPHREY` | -830.7, -6.8, 232.9 | `-` | - | RequiredLevel=0, TaskName=scene | STOP, STOP2, WALK, ATTACK, SHRINK, RUNSHRUNK, GROW | STOP:humpsleep.ASE, WALK:humpwalk.ASE, ATTACK:humprun.ASE, SHRINK:humpshrink.ASE, STOP2:humpstop.ASE, RUNSHRUNK:humprunshrunk.ASE, GROW:humpgrow.ASE |
| `level4d` | `3CAR` C3DCarl | `C3DCARL` | -283.7, -2.6, 1084.2 | `-` | - | RequiredLevel=0, TaskName=scene | - | STOP:carlstop.ASE, WALK:carlwalk.ASE, TALK:carltalk.ASE, TELE:carlteleport.ASE, CHEER:carlcheer.ASE, INHALE:carlinhale.ASE |
| `level4d` | `3HUG` C3DHugh | `C3DHUGH` | -312.7, 31.6, 842.7 | `-` | - | RequiredLevel=0, TaskName=scene | - | STOP:hughstop.ASE, WALK:hughwalk.ASE, TALK:hughtalk.ASE, COUNT:hughcount.ASE |
| `level4d` | `3NIC` C3DNick | `C3DNICK` | -346.9, 1.8, 66.0 | `-` | - | RequiredLevel=0, TaskName=scene | WAIT, GLIDE | STOP:nickcoin.ASE, WALK:nickwalk.ASE, TALK:nicktalkboard.ASE, WAIT:nickstop.ASE, SKATE:nickskate1.ASE, GLIDE:nickskate2.ASE, WAVE:nickwave.ASE, COIN:nickcoin.ASE |
| `level4d` | `3LIB` C3DLibby | `C3DLIBBY` | -101.5, 21.6, 802.7 | `-` | - | RequiredLevel=0, TaskName=scene | - | STOP:libystop.ASE, WALK:libywalk.ASE, RUN:libyrun.ASE, TALK:libytalk.ASE, PHONE:libbyphone.ASE, WAVE:libywave.ASE |
| `level4d` | `3SHE` C3DSheen | `C3DSHEEN` | 323.1, 2.5, 697.7 | `-` | - | RequiredLevel=0, TaskName=scene | WAVE, PICK | STOP:shenstop.ASE, WALK:shenwalk.ASE, TALK:shentalk.ASE, WAVE:shenwave.ASE, PICK:shenpick.ASE |
| `level4d` | `3BEN` C3DBenny | `C3DBENNY` | -69.5, 9.5, -190.0 | `-` | - | RequiredLevel=0, TaskName=scene | - | STOP:bennystop.ASE, WALK:bennywalk.ASE, TALK:bennytalk.ASE, PHONE:bennyphone.ASE, WIPE:bennywipe.ASE, WPHONE:bennywipephone.ASE |
| `level4d` | `3FOW` C3DFowl | `fowl` | 619.2, 0.4, 563.8 | `-` | - | RequiredLevel=0, TaskName=scene | - | STOP:fowlstop.ASE, WALK:fowlwalk.ASE, TALK:fowltalk.ASE, TELE:fowlteleport.ASE, CHEER:fowlcheer.ASE |
| `level4d` | `3ULT` C3DUltraLord | `C3DULTRALORD` | 272.7, 8.2, 627.4 | `-` | - | RequiredLevel=0, TaskName=scene | FLEX1, FLEX2, FLEX3 | STOP:ultrastop.ASE, WALK:ultrawalk.ASE, TALK:ultratalk.ASE, GIVE:ultragive.ASE, FLEX1:ultraflex.ASE, FLEX2:ultraflex2.ASE, FLEX3:ultraflex3.ASE, WHISPER:ultrawhisper.ASE |
| `level5` | `3SPW` C3DVulture | `vulta` | 3838.1, -37.8, -4438.5 | `vulta02` | - | TaskName=scene | STOP, WALK | STOP:vulture01.ASE, WALK:vulture02.ASE |
| `level5a` | `3GUA` C3DYokianGuard | `C3DYOKIANGUARD` | 1601.0, -4839.9, 37737.1 | `Y1` | - | TaskName=scene | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level5a` | `3SOL` C3DYokianSoldier | `C3DYOKIANSOLDIER` | 1124.9, -4170.9, 37567.7 | `s1` | - | TaskName=scene | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level5a` | `3GUA` C3DYokianGuard | `C3DYOKIANGUARD` | -447.2, -4187.7, 40206.3 | `yg1` | - | TaskName=scene | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level5b` | `3GUA` C3DYokianGuard | `CARLGUARD1` | -4311.6, -6114.2, 39507.3 | `yok1` | - | - | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level5b` | `3CAR` C3DCarl | `C3DCARL` | -4384.2, -6112.2, 39087.6 | `carl1` | - | TaskName=scene | - | STOP:carlstop.ASE, WALK:carlwalk.ASE, TALK:carltalk.ASE, TELE:carlteleport.ASE, CHEER:carlcheer.ASE, INHALE:carlinhale.ASE |
| `level5b` | `3SOL` C3DYokianSoldier | `guard2` | -4073.3, -6066.8, 38578.6 | `yok` | - | TaskName=scene | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level6` | `3SOL` C3DYokianSoldier | `C3DYOKIANSOLDIER` | -1831.3, 893.9, -7006.7 | `hall1` | - | - | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level6` | `3GUA` C3DYokianGuard | `C3DYOKIANGUARD` | -7238.2, 1027.2, -5916.4 | `rooma1` | - | - | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level6` | `3SOL` C3DYokianSoldier | `C3DYOKIANSOLDIER` | -3387.9, 855.2, -6982.5 | `roomb1` | - | - | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level6` | `3SOL` C3DYokianSoldier | `C3DYOKIANSOLDIER` | -9028.9, 785.3, -5875.3 | `pat01` | - | - | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level6` | `3SOL` C3DYokianSoldier | `C3DYOKIANSOLDIER` | -8630.3, 172.0, -6373.5 | `apple01` | - | - | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level6` | `3SOL` C3DYokianSoldier | `C3DYOKIANSOLDIER` | -9587.5, 875.5, -2317.1 | `patb01` | - | - | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level6` | `3FLE` C3DFleetCommander | `FLEETC` | -1667.3, 1474.2, -219.1 | `-` | - | TaskName=scene | STOP, WALK, TALK, SHRINK | STOP:commanderstop.ASE, WALK:commanderwalk.ASE, TALK:commandertalk.ASE, SHRINK:commandershrink.ASE |
| `level6` | `3SOL` C3DYokianSoldier | `Second` | -737.8, 1485.5, 1301.0 | `line` | - | TaskName=scene | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level6` | `3CAR` C3DCarl | `C3DCARL` | -2469.0, 1792.3, 458.3 | `-` | - | TaskName=scene | - | STOP:carlstop.ASE, WALK:carlwalk.ASE, TALK:carltalk.ASE, TELE:carlteleport.ASE, CHEER:carlcheer.ASE, INHALE:carlinhale.ASE |
| `level6` | `3CIN` C3DCindy | `C3DCINDY` | -2303.4, 1799.6, 417.3 | `-` | - | RequiredLevel=0, TaskName=scene | - | STOP:cindstop.ASE, WALK:cindwalk.ASE, TALK:cindtalk.ASE, TELE:cindteleport.ASE, CHEER:cindycheer.ASE, WAVE:cindwave.ASE |
| `level6` | `3FOW` C3DFowl | `C3DFOWL` | -2401.8, 1798.0, 309.3 | `-` | - | RequiredLevel=0, TaskName=scene | - | STOP:fowlstop.ASE, WALK:fowlwalk.ASE, TALK:fowltalk.ASE, TELE:fowlteleport.ASE, CHEER:fowlcheer.ASE |
| `level6` | `3SOL` C3DYokianSoldier | `yoksol` | 3.7, 32.0, -3475.6 | `yok1` | - | TaskName=scene | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level6a` | `3GUA` C3DYokianGuard | `C3DYOKIANGUARD` | -15411.3, 895.3, 2463.5 | `wlk01` | - | - | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level6a` | `3SOL` C3DYokianSoldier | `C3DYOKIANSOLDIER` | -18653.7, 809.4, 2015.8 | `bot01` | - | - | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level6a` | `3FLE` C3DFleetCommander | `FLEETC` | -1667.3, 1474.2, -219.1 | `-` | - | TaskName=scene | STOP, WALK, TALK, SHRINK | STOP:commanderstop.ASE, WALK:commanderwalk.ASE, TALK:commandertalk.ASE, SHRINK:commandershrink.ASE |
| `level6a` | `3SOL` C3DYokianSoldier | `Second` | -737.8, 1485.5, 1301.0 | `line` | - | TaskName=scene | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level6a` | `3GUA` C3DYokianGuard | `lastguard` | -4254.1, 1488.4, 2887.0 | `lastg01` | - | TaskName=scene | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level6a` | `3SOL` C3DYokianSoldier | `yokunknownsold` | -4567.8, 866.6, 4233.3 | `no01` | - | TaskName=scene | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level6a` | `3CAR` C3DCarl | `C3DCARL` | -2469.0, 1792.3, 458.3 | `-` | - | TaskName=scene | - | STOP:carlstop.ASE, WALK:carlwalk.ASE, TALK:carltalk.ASE, TELE:carlteleport.ASE, CHEER:carlcheer.ASE, INHALE:carlinhale.ASE |
| `level6a` | `3CIN` C3DCindy | `C3DCINDY` | -2303.4, 1799.6, 417.3 | `-` | - | RequiredLevel=0, TaskName=scene | - | STOP:cindstop.ASE, WALK:cindwalk.ASE, TALK:cindtalk.ASE, TELE:cindteleport.ASE, CHEER:cindycheer.ASE, WAVE:cindwave.ASE |
| `level6a` | `3FOW` C3DFowl | `C3DFOWL` | -2401.8, 1798.0, 309.3 | `-` | - | RequiredLevel=0, TaskName=scene | - | STOP:fowlstop.ASE, WALK:fowlwalk.ASE, TALK:fowltalk.ASE, TELE:fowlteleport.ASE, CHEER:fowlcheer.ASE |
| `level7` | `3GUA` C3DYokianGuard | `C3DYOKIANGUARD` | -7114.3, 1481.5, -2251.1 | `cell01` | - | RequiredLevel=0 | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level7` | `3SOL` C3DYokianSoldier | `C3DYOKIANSOLDIER` | -3285.0, 1431.4, 1038.2 | `ys1` | - | RequiredLevel=0 | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level7` | `3GUA` C3DYokianGuard | `C3DYOKIANGUARD` | -7002.9, 1465.5, 1770.4 | `yg1` | - | RequiredLevel=0 | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level7` | `3GUA` C3DYokianGuard | `C3DYOKIANGUARD` | -4824.1, 819.7, 1557.9 | `-` | - | RequiredLevel=0 | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level7` | `3SOL` C3DYokianSoldier | `C3DYOKIANSOLDIER` | -6007.7, 888.7, -2424.4 | `-` | - | RequiredLevel=0 | SHRINK, TALK, ATTACK | STOP:soldatak.ASE, WALK:soldwalk.ASE, TALK:soldatak.ASE, ATTACK:soldatak.ASE, SHRINK:soldshrink.ASE |
| `level7` | `3GUA` C3DYokianGuard | `yokguard` | -9674.1, 865.8, 4494.5 | `p1` | - | RequiredLevel=0, TaskName=Scene | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level7` | `3GUA` C3DYokianGuard | `C3DYOKIANGUARD` | -3987.0, 907.9, 4473.7 | `q1` | - | RequiredLevel=0 | - | STOP:guardatak.ASE, WALK:guardwalk.ASE, ATTACK:guardatak.ASE, SHRINK:guardshrink.ASE |
| `level7` | `3YSH` C3DYokianShip | `C3DYOKIANSHIP` | 2926.5, 2026.8, -1039.8 | `ship01` | - | RequiredLevel=0, TaskName=scene | - | BASE:yokianship.omt |
