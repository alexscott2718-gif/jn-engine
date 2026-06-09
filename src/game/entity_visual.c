#include "entity_visual.h"
#include <string.h>
#include <strings.h>

/* Lookup tiers:
   1) per-(FourCC, tag) override
   2) per-FourCC default
   3) FourCC marked invisible
   No entry  -> resolver returns 0, caller draws the placeholder box. */

typedef struct {
    const char  *fourcc;
    const char  *tag;          /* case-insensitive match */
    EntityVisual v;
} TagEntry;

typedef struct {
    const char  *fourcc;
    EntityVisual v;
} TypeEntry;

typedef struct {
    const char *stem;       /* GRN filename stem, case-insensitive */
    const char *model_path;
    float       scale;
} GrnAssetEntry;

/* --- Tier 4: tag-driven overrides. Each row says "for FourCC X with tag Y,
   use this mesh." Tags are matched case-insensitively. */
static const TagEntry TAG_TABLE[] = {

    /* 3PIC — pickups whose visual is the tag's referent (Level1 tags). */
    { "3PIC", "BUBBLEPICKUP",  { "assets/ase/nest.ASE",         NULL, 1.0f, 0 } },
    { "3PIC", "NEST",          { "assets/ase/nest.ASE",         NULL, 1.0f, 0 } },
    { "3PIC", "NEST2",         { "assets/ase/nest.ASE",         NULL, 1.0f, 0 } },
    { "3PIC", "nest1",         { "assets/ase/nest.ASE",         NULL, 1.0f, 0 } },
    { "3PIC", "nest2",         { "assets/ase/nest.ASE",         NULL, 1.0f, 0 } },
    { "3PIC", "hydrant",       { "assets/ase/firehydrant.ASE",  NULL, 1.0f, 0 } },
    { "3PIC", "godphone",      { "assets/ase/phone.ASE",        NULL, 1.0f, 0 } },
    { "3PIC", "pad",           { "assets/ase/omt/RocketPad.ASE", NULL, 1.0f, 0 } },
    { "3PIC", "pad2",          { "assets/ase/omt/RocketPad.ASE", NULL, 1.0f, 0 } },
    { "3PIC", "egg2b",         { "assets/ase/omt/egg.ASE",      NULL, 0.5f, 0 } },
    { "3PIC", "boatl",         { "assets/ase/omt/SailBoat.ASE", NULL, 0.5f, 0 } },
    { "3PIC", "hsounds",       { NULL, NULL, 0.0f, 1 } },  /* sound trigger, no mesh */
    /* No wrench/water2 ASEs available; fall through to TYPE_TABLE default. */

    /* 3GRN — JNvsJN GRN-backed props. Prefer a textured OMT/ASE twin over the
       untextured GRN extraction: the mailbox already exists as a textured mesh
       from the OMT pipeline (assets/glb/omt/mailbox.glb), so reuse it. Other
       3GRN tags fall to the invisible TYPE default until a textured source
       exists. */
    { "3GRN", "mailbox", { "assets/glb/omt/mailbox.glb", NULL, 1.0f, 0 } },

    /* 3OMT — generic OMT-backed props by tag. */
    { "3OMT", "bench01", { "assets/ase/board.ASE",      NULL, 1.0f, 0 } },
    { "3OMT", "beam",    { "assets/ase/beams.ASE",      NULL, 1.0f, 0 } },
    { "3OMT", "beams",   { "assets/ase/beams.ASE",      NULL, 1.0f, 0 } },
    { "3OMT", "board",   { "assets/ase/board.ASE",      NULL, 1.0f, 0 } },
    { "3OMT", "ray",     { "assets/ase/ray.ASE",        NULL, 1.0f, 0 } },

    /* 3DOR — door variants by tag (else FourCC default below picks a door). */
    { "3DOR", "DOORPP1",     { "assets/ase/DoorPP1.ASE",     NULL, 1.0f, 0 } },
    /* Door family migrated off degenerate OMT->ASE stubs to the OMT->GLB
       meshes (real geometry; see docs/ase_stub_export_audit.md). The
       assets/glb/ase/ mirror carries the textured versions. */
    { "3DOR", "DOORPP2",     { "assets/glb/ase/DoorPP2.glb",    NULL, 1.0f, 0 } },
    { "3DOR", "DOORGRILL",   { "assets/ase/doorgrill.ASE",   NULL, 1.0f, 0 } },
    { "3DOR", "DOORGRILL2",  { "assets/glb/ase/DoorGrill2.glb", NULL, 1.0f, 0 } },
    { "3DOR", "DOORGRILL3",  { "assets/glb/ase/DoorGrill3.glb", NULL, 1.0f, 0 } },
    { "3DOR", "DOORCLOSET",  { "assets/glb/ase/DoorCloset.glb", NULL, 1.0f, 0 } },
    { "3DOR", "DOORFOWL",    { "assets/ase/doorfowl.ASE",    NULL, 1.0f, 0 } },
    { "3DOR", "DOORCAVE",    { "assets/glb/ase/doorcave.glb",   NULL, 1.0f, 0 } },
    { "3DOR", "DOORRETRO",   { "assets/glb/ase/doorretro.glb",  NULL, 1.0f, 0 } },
    { "3DOR", "FIREDOOR",    { "assets/glb/ase/firedoor.glb",   NULL, 1.0f, 0 } },
};
static const int TAG_TABLE_N = (int)(sizeof(TAG_TABLE) / sizeof(TAG_TABLE[0]));

static const GrnAssetEntry GRN_ASSET_TABLE[] = {
    /* Prefer textured OMT twins where they exist; otherwise use converted
       high-confidence static GRN meshes. */
    { "mailboxbase",       "assets/glb/omt/mailbox.glb",       1.0f },
    { "jetpack",           "assets/glb/omt/jetpack.glb",       1.0f },
    { "watergun2",         "assets/glb/grn/watergun2.glb",     1.0f },
    { "flurpbottlebase",   "assets/glb/grn/flurpbottlebase.glb", 1.0f },
    { "steamer",           "assets/glb/grn/steamer.glb",       1.0f },
    { "springbase",        "assets/glb/grn_capture/springbase.glb", 1.0f }, /* M2d textured capture */
    { "jonnyshootbase",    "assets/glb/grn/Jonnyshootbase.glb", 1.0f },
    { "gemred",            "assets/glb/grn_capture/gemred.glb",    1.0f }, /* M2d textured capture */
    { "gemblue",           "assets/glb/grn_capture/gemblue.glb",   1.0f }, /* M2d textured capture */
    { "gemyellow",         "assets/glb/grn_capture/gemyellow.glb", 1.0f }, /* M2d textured capture */
    { "gembase",           "assets/glb/grn/gembase.glb",       1.0f },
    { "watertowerbase",    "assets/glb/grn/watertowerbase.glb", 1.0f },
    { "megaburpgun",       "assets/glb/grn/megaburpgun.glb",   1.0f },
    { "megaburpgun1",      "assets/glb/grn/megaburpgun.glb",   1.0f },
    { "megaburpgun2",      "assets/glb/grn/megaburpgun.glb",   1.0f },
    { "megaburpgun3",      "assets/glb/grn/megaburpgun.glb",   1.0f },
    { "nummeyscooterbase", "assets/glb/grn/nummeyscooterbase.glb", 1.0f },
    { "cagedoorbase",      "assets/glb/grn/cagedoorstop.glb",  1.0f },
    { "tombstonebase",     "assets/glb/grn/tombstonestop.glb", 1.0f },
    { "tombstonebase2",    "assets/glb/grn/tombstonestop.glb", 1.0f },

    /* M2d proxy-captured, source-named, *textured* GRN meshes. Each stem here is
       referenced by a GAM *Animation property in >=1 JNvsJN level (verified via
       `strings` over jnvsjn-original/gam), so the resolver's GRN stem path reaches
       them. Strict upgrade over the untextured static-converter outputs above. */
    { "chutebase",         "assets/glb/grn_capture/chutebase.glb",      1.0f },
    { "speedbase",         "assets/glb/grn_capture/speedbase.glb",      1.0f },
    { "hand",              "assets/glb/grn_capture/hand.glb",           1.0f },
    { "doom",              "assets/glb/grn_capture/doom.glb",           1.0f },
    { "momybase",          "assets/glb/grn_capture/momybase.glb",       1.0f },
    { "dadpiramidbase",    "assets/glb/grn_capture/dadpiramidbase.glb", 1.0f },
    { "fowlmummybase",     "assets/glb/grn_capture/fowlmummybase.glb",  1.0f },
};
static const int GRN_ASSET_TABLE_N = (int)(sizeof(GRN_ASSET_TABLE) / sizeof(GRN_ASSET_TABLE[0]));

/* --- Per-FourCC defaults. */
static const TypeEntry TYPE_TABLE[] = {
    /* Tier 1 — invisible markers/triggers. */
    { "3PAT", { NULL, NULL, 0.0f, 1 } },
    { "3AIT", { NULL, NULL, 0.0f, 1 } },
    { "3CAM", { NULL, NULL, 0.0f, 1 } },
    { "3MCA", { NULL, NULL, 0.0f, 1 } },
    { "3SOU", { NULL, NULL, 0.0f, 1 } },
    { "3LIO", { NULL, NULL, 0.0f, 1 } },
    { "STRT", { NULL, NULL, 0.0f, 1 } },
    { "TRIG", { NULL, NULL, 0.0f, 1 } },
    /* Phase 9 invisibles: pure billboard/trigger/dummy entities. */
    { "3SPR", { NULL, NULL, 0.0f, 1 } },  /* C3DSPRITE — billboard, no 3D mesh */
    { "3ANI", { NULL, NULL, 0.0f, 1 } },  /* C3DANIMATEDSPRITE — billboard */
    { "3LAS", { NULL, NULL, 0.0f, 1 } },  /* C3DLASERTRIGGER — trigger volume */
    { "3DAI", { NULL, NULL, 0.0f, 1 } },  /* always at (0,0,0), dummy */
    { "3RCK", { NULL, NULL, 0.0f, 1 } },  /* rocket — HIDDEN flag on all instances */
    { "LOAD", { NULL, NULL, 0.0f, 1 } },  /* level-load trigger */
    { "3TSP", { NULL, NULL, 0.0f, 1 } },  /* JNvsJN transport effect; sprite/effect later */
    /* 3CHK checkpoint: visible camera-facing marker (sprites.omt "checkpoint").
       Behavior (vt_checkpoint) sets the respawn point on overlap. */
    { "3CHK", { NULL, NULL, 0, 0,
                "assets/parsed/sprites/jnvsjn/spr_checkpoint.png",
                160.0f, 0,0,0,0 } },
    { "3FAD", { NULL, NULL, 0.0f, 1 } },  /* fade/camera transition trigger */
    { "3FOG", { NULL, NULL, 0.0f, 1 } },  /* fog volume/controller */
    { "3MUS", { NULL, NULL, 0.0f, 1 } },  /* music trigger */
    { "3HEL", { NULL, NULL, 0.0f, 1 } },  /* help/sound trigger */
    { "3AIJ", { NULL, NULL, 0.0f, 1 } },  /* AI Jimmy controller */
    { "3FLY", { NULL, NULL, 0.0f, 1 } },  /* fly-through camera/control path */

    /* JNvsJN GRN-backed actors/props. The .gam loader preserves their GRN
       filenames; render them once a GRN path exists instead of drawing boxes. */
    { "3GRN", { NULL, NULL, 0.0f, 1 } },
    { "3GRO", { NULL, NULL, 0.0f, 1 } },
    { "3GEM", { NULL, NULL, 0.0f, 1 } },
    /* Herman's GRN is still unconverted, but the original JNvsJN install also
       ships a matching textured bigbot ASE using the same bigbot texture. */
    { "3HER", { "assets/ase/bigbotstop.ASE", NULL, 1.0f, 0 } },

    /* Synthetic test items (no game FourCC; visual is same as a pickup). */
    { "ITEM", { "assets/ase/jimpickup.ASE",     NULL, 1.0f, 0 } },

    /* Generic fallbacks for tag-driven FourCCs when no tag matches. */
    { "3PIC", { "assets/ase/jimpickup.ASE",     NULL, 0.5f, 0 } },
    { "3OMT", { "assets/ase/omt/Sphere01.ASE",  NULL, 1.0f, 0 } },

    /* Tier 2 — environment singletons. */
    { "3ROC", { "assets/ase/rocket.ASE",   NULL, 1.0f, 0 } },
    { "3ARR", { "assets/ase/3Darrow.ASE",  NULL, 1.0f, 0 } },
    { "3BUT", { "assets/ase/buttonup.ASE", NULL, 1.0f, 0 } },
    /* No same-named door.glb exists (the generic "door" mesh isn't in the
       OMT or ase->glb pipelines); substitute a textured Retroville door from
       the OMT->GLB pipeline instead of the 8-vert door.ASE stub. */
    { "3DOR", { "assets/glb/omt/level1d/DOOR.glb", NULL, 1.0f, 0 } },

    /* Tier 3 — characters (single stop/idle pose; no per-entity anim yet). */
    { "3CAR", { "assets/ase/carlstop.ASE",    NULL, 1.0f, 0 } },
    { "3BEN", { "assets/ase/bennystop.ASE",   NULL, 1.0f, 0 } },
    { "3HUM", { "assets/ase/humpstop.ASE",    NULL, 1.0f, 0 } },
    { "3FIS", { "assets/ase/fish2.ASE",       NULL, 1.0f, 0 } },
    { "3PHO", { "assets/ase/phone.ASE",       NULL, 1.0f, 0 } },
    { "3HYD", { "assets/ase/firehydrant.ASE", NULL, 1.0f, 0 } },
    /* Phase 9 characters (Stage A confident mappings). */
    { "3JIM", { "assets/ase/jimstop.ase",     NULL, 1.0f, 0 } },
    { "3SHE", { "assets/glb/grn_capture/sheenbase.glb", NULL, 1.0f, 0 } }, /* Sheen — M2d capture (RTTI C3DSHEEN) */
    { "3LIB", { "assets/ase/libystop.ASE",    NULL, 1.0f, 0 } },
    { "3MOM", { "assets/ase/judystop.ASE",    NULL, 1.0f, 0 } },
    { "3GIR", { "assets/ase/plantstop.ASE",   NULL, 1.0f, 0 } },
    { "3KIT", { "assets/ase/catsit.ASE",      NULL, 1.0f, 0 } },
    { "3NIC", { "assets/ase/nickstop.ASE",    NULL, 1.0f, 0 } },
    { "3GUA", { "assets/ase/guardwalk.ASE",   NULL, 1.0f, 0 } },
    { "3SOL", { "assets/ase/soldwalk.ASE",    NULL, 1.0f, 0 } },
    { "3FLA", { "assets/ase/firestrato.ASE",  NULL, 1.0f, 0 } },
    { "3SBU", { "assets/ase/retrobus.ASE",    NULL, 1.0f, 0 } },

    /* Tier 4 — environment defaults sourced from OMT extraction (Phase 7). */
    { "3TRE", { "assets/glb/omt/tree01.glb",    NULL, 1.0f, 0 } },  /* GLB (ASE was 8-vert stub) */
    { "3DIN", { "assets/ase/omt/dino.ASE",      NULL, 1.0f, 0 } },
    /* 3FAN: the old OMT->ASE export produced an 8-vert stub (fan.ASE) textured
       off a metal atlas — it rendered as a "wood" board. Use the OMT->GLB
       pipeline's real fan mesh (228 verts, embedded textures) instead. See the
       degenerate-ASE-stub note in docs/decomp / PROJECT_HISTORY. */
    /* level5a/fan.glb is an absolute-positioned placement mesh (baked level5a
       world Y ~= -3500); recenter it onto the 3FAN entity so it renders at the
       authored fan position and spins there. */
    { "3FAN", { "assets/glb/omt/level5a/fan.glb", NULL, 1.0f, 0, NULL, 0.0f, 0,0,0,0, 1 } },
    { "3SAI", { "assets/ase/omt/SailBoat.ASE",  NULL, 1.0f, 0 } },
    { "3SPH", { "assets/ase/omt/Sphere01.ASE",  NULL, 1.0f, 0 } },

    /* Phase 9 — exact 3D meshes (Stage B Ghidra confirmed). */
    { "3MER", { "assets/ase/omt/Rocket.ASE",     NULL, 1.0f, 0 } },  /* objects.omt id-16 */
    { "3SUV", { "assets/ase/omt/lawnmower.ASE",  NULL, 1.0f, 0 } },  /* jeep.omt id-2 */
    { "3AIO", { "assets/glb/omt/Box03.glb",      NULL, 1.0f, 0 } },  /* objects.omt id-4 default (GLB; ASE was 11-vert stub) */
    { "3SWN", { "assets/ase/doorfowl.ASE",       NULL, 1.0f, 0 } },  /* ASE-direct */

    /* Phase 10 Step 2 — sprite billboards. These FourCCs back to sprites.omt
       Canvas chunks (Ghidra-verified Phase 9, Stage B). No 3D mesh exists.
       3NEU/3RED share the same atlas (canvas 0–12); 3RED gets a red tint.
       sprite_size is in world units; tuned to match the Phase 9 placeholder
       sphere scale so existing placement heights still read correctly. */
    { "3NEU", { NULL, NULL, 0, 0,
                "assets/parsed/sprites/sprites_images/0000_100x100d32.png",
                90.0f, 0,0,0,0 } },
    { "3RED", { NULL, NULL, 0, 0,
                "assets/parsed/sprites/sprites_images/0000_100x100d32.png",
                90.0f, 1.0f, 0.25f, 0.25f, 1.0f } },
    { "3BAL", { NULL, NULL, 0, 0,
                "assets/parsed/sprites/sprites_images/0050_32x32d32.png",
                70.0f, 0,0,0,0 } },
    { "3CON", { NULL, NULL, 0, 0,
                "assets/parsed/sprites/sprites_images/0041_128x128d16.png",
                90.0f, 0,0,0,0 } },
    { "3LEA", { NULL, NULL, 0, 0,
                "assets/parsed/sprites/sprites_images/0045_129x100d16.png",
                90.0f, 0,0,0,0 } },

    /* ---- JNvsJN Step 5: widen entity coverage across all 22 levels ----
       FourCC->class from Neutron2.exe RTTI strings. Visible classes point at
       the best name-matched ASE/GRN; pure trigger/effect/data classes are
       marked invisible so they stop drawing placeholder boxes. */

    /* Characters / enemies (textured ASEs ship in assets/ase). */
    { "3CIN", { "assets/ase/cindycheer.ASE",   NULL, 1.0f, 0 } },  /* Cindy */
    { "3HUG", { "assets/ase/hughstop.ASE",     NULL, 1.0f, 0 } },  /* Hugh */
    { "3ULT", { "assets/ase/ultrastop.ASE",    NULL, 1.0f, 0 } },  /* UltraLord */
    { "3FOW", { "assets/glb/grn_capture/fowlbase.glb", NULL, 1.0f, 0 } },  /* Fowl — M2d capture (RTTI C3DFOWL) */
    { "3SPK", { "assets/ase/sporkystop.ASE",   NULL, 1.0f, 0 } },  /* Sporky */
    { "3BOT", { "assets/ase/bigbotstop.ASE",   NULL, 1.0f, 0 } },  /* Bot */
    { "3ENE", { "assets/ase/bigbotstop.ASE",   NULL, 1.0f, 0 } },  /* generic Enemy */
    { "3NUM", { "assets/glb/grn/nummeyscooterbase.glb", NULL, 1.0f, 0 } }, /* Numey */

    /* Props (name-matched OMT/ASE meshes). */
    /* Tesla coil. level6/tesla.glb is an absolute-positioned placement mesh
       (baked level6 world Y ~= 838); recenter onto the entity. */
    { "3TES", { "assets/glb/omt/level6/tesla.glb", NULL, 1.0f, 0, NULL, 0.0f, 0,0,0,0, 1 } }, /* GLB; ASE was 4-vert stub */
    { "3MOP", { "assets/ase/omt/block.ASE",      NULL, 1.0f, 0 } },  /* MovingPlatform */
    { "3MOR", { "assets/glb/omt/Box01.glb",      NULL, 1.0f, 0 } },  /* MovableRock (GLB; ASE was 4-vert stub) */
    { "3SHU", { "assets/glb/omt/BUSH01.glb",     NULL, 1.0f, 0 } },  /* Shrubbery (GLB; ASE was 6-vert stub) */
    { "3DUD", { "assets/glb/ase/downdoor2a.glb", NULL, 1.0f, 0 } },  /* DoorUpDown (GLB; ASE was 4-vert stub) */
    { "3WAB", { "assets/ase/buttondown.ASE",     NULL, 1.0f, 0 } },  /* WaterButton */
    { "3SCR", { "assets/ase/omt/blockscreen.ASE", NULL, 1.0f, 0 } }, /* Screen */
    { "3SCD", { "assets/glb/omt/level1d/DOOR.glb", NULL, 1.0f, 0 } }, /* SchoolDoor (GLB; ASE door was 8-vert stub) */
    { "3CRB", { "assets/ase/omt/block.ASE",      NULL, 1.0f, 0 } },  /* CraneBoom */

    /* Sprite-backed objects (sprites.omt). Camera-facing billboards. */
    { "3GRT", { NULL, NULL, 0, 0,
                "assets/parsed/sprites/jnvsjn/spr_greenneutron.png",
                80.0f, 0,0,0,0 } },                                /* GreenThingie */
    { "3TAR", { NULL, NULL, 0, 0,
                "assets/parsed/sprites/jnvsjn/spr_target.png",
                90.0f, 0,0,0,0 } },                                /* MovingTarget */

    /* ---- Step 6: vehicles. Hardcoded meshes (no GAM mesh ref); imported ASEs
       under assets/ase/jnvsjn/. Visual-only for now; ridability is separate. */
    { "3JEE", { "assets/ase/jnvsjn/car.ase",            NULL, 1.0f, 0 } },  /* Jeep */
    { "3NCA", { "assets/ase/jnvsjn/car.ase",            NULL, 1.0f, 0 } },  /* NeuCar */
    { "3NC2", { "assets/ase/jnvsjn/car.ase",            NULL, 1.0f, 0 } },  /* NeuCar2 */
    { "3HOV", { "assets/ase/jnvsjn/godflycycle.ase",    NULL, 1.0f, 0 } },  /* HoverCycle */
    { "3SUB", { "assets/ase/jnvsjn/substop.ase",        NULL, 1.0f, 0 } },  /* MiniSub */
    { "3VRT", { "assets/ase/jnvsjn/tankstop.ase",       NULL, 1.0f, 0 } },  /* VRTank */
    { "3LUN", { "assets/ase/jnvsjn/lunerlanderstop.ase", NULL, 1.0f, 0 } }, /* LunarLander */

    /* Pure triggers / effects / not-data-drivable -> invisible (no box). */
    { "3TEX", { NULL, NULL, 0.0f, 1 } },  /* text trigger */
    { "3COR", { NULL, NULL, 0.0f, 1 } },  /* corona glow fx */
    { "3VOR", { NULL, NULL, 0.0f, 1 } },  /* vortex fx */
    { "3TSU", { NULL, NULL, 0.0f, 1 } },  /* tsunami fx */
    { "3SRO", { NULL, NULL, 0.0f, 1 } },  /* sprite object (no index map yet) */
    { "3ELE", { NULL, NULL, 0.0f, 1 } },  /* electron fx */
    { "3BUG", { NULL, NULL, 0.0f, 1 } },  /* flying bug (no mesh) */
    { "3GWA", { NULL, NULL, 0.0f, 1 } },  /* giant wasp (no mesh) */
    { "3WAT", { NULL, NULL, 0.0f, 1 } },  /* water plane (not data-drivable, M5) */
    { "3ROP", { NULL, NULL, 0.0f, 1 } },  /* rope */
    { "3PEN", { NULL, NULL, 0.0f, 1 } },  /* pendulum (no mesh) */
    { "3TRA", { NULL, NULL, 0.0f, 1 } },  /* trampoline (no mesh) */
};
static const int TYPE_TABLE_N = (int)(sizeof(TYPE_TABLE) / sizeof(TYPE_TABLE[0]));

static const char *first_grn_name(const Entity *e) {
    const char *names[] = {
        e->grn_base, e->grn_stop, e->grn_walk, e->grn_run, e->grn_talk, e->grn_fly,
        e->grn_anim[0], e->grn_anim[1], e->grn_anim[2], e->grn_anim[3],
    };
    const int n = (int)(sizeof(names) / sizeof(names[0]));
    for (int i = 0; i < n; i++) {
        if (!names[i][0]) continue;
        if (strcasecmp(names[i], "none") == 0) continue;
        return names[i];
    }
    return NULL;
}

static void grn_stem(const char *name, char *out, size_t out_size) {
    if (!out_size) return;
    out[0] = '\0';
    if (!name) return;
    const char *base = strrchr(name, '/');
    base = base ? base + 1 : name;
    const char *dot = strrchr(base, '.');
    size_t n = dot && dot > base ? (size_t)(dot - base) : strlen(base);
    if (n >= out_size) n = out_size - 1;
    memcpy(out, base, n);
    out[n] = '\0';
}

static int resolve_grn_asset(const Entity *e, EntityVisual *out) {
    /* Fire for ANY entity that names a GRN animation (BASEAnimation/STOP/WALK/...)
       whose stem we have a mesh for. The match is gated by GRN_ASSET_TABLE
       membership (props only), so this can only redirect an entity to an explicit
       captured/converted mesh -- never to a wrong default. */
    const char *grn = first_grn_name(e);
    if (!grn) return 0;
    char stem[80];
    grn_stem(grn, stem, sizeof(stem));
    if (!stem[0]) return 0;
    for (int i = 0; i < GRN_ASSET_TABLE_N; i++) {
        if (strcasecmp(stem, GRN_ASSET_TABLE[i].stem) != 0) continue;
        out->model_path = GRN_ASSET_TABLE[i].model_path;
        out->texture_path = NULL;
        out->scale = GRN_ASSET_TABLE[i].scale;
        out->invisible = 0;
        out->sprite_path = NULL;
        out->sprite_size = 0.0f;
        out->tint_r = out->tint_g = out->tint_b = out->tint_a = 0.0f;
        return 1;
    }
    return 0;
}

int entity_visual_resolve(const Entity *e, EntityVisual *out) {
    if (!e || !out) return 0;

    for (int i = 0; i < TAG_TABLE_N; i++) {
        if (strncmp(e->type, TAG_TABLE[i].fourcc, 4) != 0) continue;
        if (strcasecmp(e->tag, TAG_TABLE[i].tag) != 0) continue;
        *out = TAG_TABLE[i].v;
        return 1;
    }
    if (resolve_grn_asset(e, out)) return 1;
    for (int i = 0; i < TYPE_TABLE_N; i++) {
        if (strncmp(e->type, TYPE_TABLE[i].fourcc, 4) != 0) continue;
        *out = TYPE_TABLE[i].v;
        return 1;
    }
    return 0;
}
