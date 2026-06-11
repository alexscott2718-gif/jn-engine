#ifndef QA_H
#define QA_H

/* In-game QA annotation mode (docs/qa_annotate_plan.md, M1).
   Q toggles the mode; while active the follow-cam mouse drag is suspended and
   the cursor picks scene objects via the renderer's color-ID pass. Object
   identity events are pushed to the web shell (window.jnQA.onEvent) or stdout
   on native builds; the dialog/tag-stack UI lives in web/shell.html. */

#include "../engine/world.h"

int  qa_active(void);
void qa_toggle(void);

/* Scene enumeration. Call begin_scene before draw_scene in BOTH the main
   render pass (pick_pass=0: applies hover/selection highlight) and the pick
   pass (pick_pass=1: installs per-object pick IDs); call end_scene after the
   main pass to drop any lingering highlight tint. Registration order must be
   identical across the two passes — guaranteed by sharing draw_scene. */
void qa_begin_scene(int pick_pass);
void qa_register_entity(const Entity *e);
void qa_register_placement(const WorldPlacement *pl,
                           float draw_x, float draw_y, float draw_z);
void qa_end_scene(void);

/* Input + pick driving (main.c). Cursor coords are SDL window coords. */
void qa_on_mouse_motion(int x, int y);
void qa_on_mouse_click(int x, int y);
int  qa_want_pick(unsigned int now_ms);   /* throttled, immediate on click */
void qa_get_cursor(int *x, int *y);

/* Resolve the ID read back by renderer_pick_end: updates hover/selection and
   emits the hover/pick event. Level + camera context ride along so the pick
   payload is self-contained for the reporter/agent. */
void qa_pick_resolve(unsigned int id, const char *level_name,
                     float cam_x, float cam_y, float cam_z, float cam_yaw);

/* Exported to JS (dialog Cancel/OK clears the selection tint). */
void qa_clear_selection(void);

#endif
