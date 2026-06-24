#ifndef COLLISION_H
#define COLLISION_H

/* Mesh collision world (native-port collision overhaul, 2026-06-24).

   The original game's collision world is a set of invisible OMT collider
   meshes (BLOCKING_* / BLOCK_*) plus the visible walkable floor (level1's is
   literally GROUND.glb, both drawn and collided). Buildings/fences are blocked
   by their BLOCKING_* twins, not by the visible building meshes. This module
   bakes those collider meshes into a single world-space triangle soup behind a
   uniform XZ grid so the player and physics entities can ground-follow, be
   blocked by walls, and step up curbs against real geometry instead of the old
   floor-only height sample + safety plane.

   Triangles are baked in the same basis the renderer/physics use for OMT
   placements: world = (placement.x + vx, vy, vz - placement.z). */

struct World;

typedef struct CollisionWorld CollisionWorld;

/* Naming predicates — the single source of truth for "what is a collider",
   shared by the physics build and the renderer's invisible-collider skip.

   collision_is_collider: contributes geometry to the CollisionWorld.
     = GROUND* (visible walkable terrain)  OR
       BLOCK* / BLOCKING* (invisible collision proxies),
     excluding the visible playground toys Blocks_Out / Blocks_In (those are
     real geometry with their own BLOCKING12_monkeybars collider).

   collision_is_invisible: the renderer skips drawing it (it is an invisible
     collision proxy, never a visible surface).
     = BLOCK* / BLOCKING* excluding Blocks_Out / Blocks_In. */
int collision_is_collider(const char *name);
int collision_is_invisible(const char *name);

/* Build the collision world from w->placements. Collider meshes are pulled via
   model_cache_get, so a live GL context is required (same as the render path).
   Returns NULL when the level has no collider geometry. */
CollisionWorld *collision_build(const struct World *w);
void            collision_free(CollisionWorld *cw);

/* Highest collision surface at (x,z) that is at or below ref_y + the step cap,
   so the player lands on terrain/floors/curbs and is never snapped up onto a
   roof above them. Writes the surface Y to *out_y; if n != NULL, writes the
   winning triangle's upward unit normal. Returns 1 on hit, 0 if nothing is
   under (x,z). */
int collision_ground_height(const CollisionWorld *cw, float x, float z,
                            float ref_y, float *out_y, float n[3]);

/* Push an axis-aligned box (center pos[3], half-extents half[3]) out of any
   wall-like collider triangles, sliding along surfaces and zeroing the
   into-wall component of vel[3]. pos and vel are updated in place; the Y axis
   is left to collision_ground_height (this only moves XZ). Curbs/steps whose
   top is within the step cap of the feet are NOT treated as walls (the player
   steps up onto them via ground-follow). */
void collision_resolve_horizontal(const CollisionWorld *cw,
                                   float pos[3], const float half[3],
                                   float vel[3]);

/* Cast a segment p->q against the collider triangles. Returns the smallest t in
   [0,1] of the first hit (1.0 = no hit). If n != NULL, writes the hit triangle
   normal (oriented against the segment direction). */
float collision_segment(const CollisionWorld *cw,
                        const float p[3], const float q[3], float n[3]);

/* Test/diagnostic: find the TALL wall-like collider triangle (|normal.y| < the
   wall slope threshold and taller than the step cap) whose surface is closest
   to p within `search` units. On success writes the closest surface point to
   out_pt[3], the triangle's outward horizontal unit normal (oriented toward p)
   to out_n[3], the triangle's Y range to out_ymin and out_ymax, and returns 1;
   returns 0 if no qualifying wall is near. Used by the JN_TEST_COLLIDE seam to
   exercise wall clamping level-agnostically (seating the player's feet at the
   wall base). */
int collision_nearest_wall(const CollisionWorld *cw, const float p[3],
                           float search, float out_pt[3], float out_n[3],
                           float *out_ymin, float *out_ymax);

#endif
