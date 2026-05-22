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

    /* 3OMT — generic OMT-backed props by tag. */
    { "3OMT", "bench01", { "assets/ase/board.ASE",      NULL, 1.0f, 0 } },
    { "3OMT", "beam",    { "assets/ase/beams.ASE",      NULL, 1.0f, 0 } },
    { "3OMT", "beams",   { "assets/ase/beams.ASE",      NULL, 1.0f, 0 } },
    { "3OMT", "board",   { "assets/ase/board.ASE",      NULL, 1.0f, 0 } },
    { "3OMT", "ray",     { "assets/ase/ray.ASE",        NULL, 1.0f, 0 } },

    /* 3DOR — door variants by tag (else FourCC default below picks a door). */
    { "3DOR", "DOORPP1",     { "assets/ase/DoorPP1.ASE",     NULL, 1.0f, 0 } },
    { "3DOR", "DOORPP2",     { "assets/ase/DoorPP2.ASE",     NULL, 1.0f, 0 } },
    { "3DOR", "DOORGRILL",   { "assets/ase/doorgrill.ASE",   NULL, 1.0f, 0 } },
    { "3DOR", "DOORGRILL2",  { "assets/ase/DoorGrill2.ASE",  NULL, 1.0f, 0 } },
    { "3DOR", "DOORGRILL3",  { "assets/ase/DoorGrill3.ASE",  NULL, 1.0f, 0 } },
    { "3DOR", "DOORCLOSET",  { "assets/ase/DoorCloset.ASE",  NULL, 1.0f, 0 } },
    { "3DOR", "DOORFOWL",    { "assets/ase/doorfowl.ASE",    NULL, 1.0f, 0 } },
    { "3DOR", "DOORCAVE",    { "assets/ase/doorcave.ASE",    NULL, 1.0f, 0 } },
    { "3DOR", "DOORRETRO",   { "assets/ase/doorretro.ASE",   NULL, 1.0f, 0 } },
    { "3DOR", "FIREDOOR",    { "assets/ase/firedoor.ASE",    NULL, 1.0f, 0 } },
};
static const int TAG_TABLE_N = (int)(sizeof(TAG_TABLE) / sizeof(TAG_TABLE[0]));

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

    /* Synthetic test items (no game FourCC; visual is same as a pickup). */
    { "ITEM", { "assets/ase/jimpickup.ASE",     NULL, 1.0f, 0 } },

    /* Generic fallbacks for tag-driven FourCCs when no tag matches. */
    { "3PIC", { "assets/ase/jimpickup.ASE",     NULL, 0.5f, 0 } },
    { "3OMT", { "assets/ase/omt/Sphere01.ASE",  NULL, 1.0f, 0 } },

    /* Tier 2 — environment singletons. */
    { "3ROC", { "assets/ase/rocket.ASE",   NULL, 1.0f, 0 } },
    { "3ARR", { "assets/ase/3Darrow.ASE",  NULL, 1.0f, 0 } },
    { "3BUT", { "assets/ase/buttonup.ASE", NULL, 1.0f, 0 } },
    { "3DOR", { "assets/ase/door.ASE",     NULL, 1.0f, 0 } },

    /* Tier 3 — characters (single stop/idle pose; no per-entity anim yet). */
    { "3CAR", { "assets/ase/carlstop.ASE",    NULL, 1.0f, 0 } },
    { "3BEN", { "assets/ase/bennystop.ASE",   NULL, 1.0f, 0 } },
    { "3HUM", { "assets/ase/humpstop.ASE",    NULL, 1.0f, 0 } },
    { "3FIS", { "assets/ase/fish2.ASE",       NULL, 1.0f, 0 } },
    { "3PHO", { "assets/ase/phone.ASE",       NULL, 1.0f, 0 } },
    { "3HYD", { "assets/ase/firehydrant.ASE", NULL, 1.0f, 0 } },
    /* Phase 9 characters (Stage A confident mappings). */
    { "3JIM", { "assets/ase/jimstop.ase",     NULL, 1.0f, 0 } },
    { "3SHE", { "assets/ase/shenstop.ASE",    NULL, 1.0f, 0 } },
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
    { "3TRE", { "assets/ase/omt/tree01.ASE",    NULL, 1.0f, 0 } },
    { "3DIN", { "assets/ase/omt/dino.ASE",      NULL, 1.0f, 0 } },
    { "3FAN", { "assets/ase/omt/fan.ASE",       NULL, 1.0f, 0 } },
    { "3SAI", { "assets/ase/omt/SailBoat.ASE",  NULL, 1.0f, 0 } },
    { "3SPH", { "assets/ase/omt/Sphere01.ASE",  NULL, 1.0f, 0 } },

    /* Phase 9 — exact 3D meshes (Stage B Ghidra confirmed). */
    { "3MER", { "assets/ase/omt/Rocket.ASE",     NULL, 1.0f, 0 } },  /* objects.omt id-16 */
    { "3SUV", { "assets/ase/omt/lawnmower.ASE",  NULL, 1.0f, 0 } },  /* jeep.omt id-2 */
    { "3AIO", { "assets/ase/omt/Box03.ASE",      NULL, 1.0f, 0 } },  /* objects.omt id-4 default */
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
};
static const int TYPE_TABLE_N = (int)(sizeof(TYPE_TABLE) / sizeof(TYPE_TABLE[0]));

int entity_visual_resolve(const Entity *e, EntityVisual *out) {
    if (!e || !out) return 0;

    for (int i = 0; i < TAG_TABLE_N; i++) {
        if (strncmp(e->type, TAG_TABLE[i].fourcc, 4) != 0) continue;
        if (strcasecmp(e->tag, TAG_TABLE[i].tag) != 0) continue;
        *out = TAG_TABLE[i].v;
        return 1;
    }
    for (int i = 0; i < TYPE_TABLE_N; i++) {
        if (strncmp(e->type, TYPE_TABLE[i].fourcc, 4) != 0) continue;
        *out = TYPE_TABLE[i].v;
        return 1;
    }
    return 0;
}
