#ifndef REPLAY_H
#define REPLAY_H

/* Faithful replay path (Phase-12 pivot proof). Reads a single-frame self-
   contained .omtc capture (extract_frame_capture.py) and renders it as GL,
   translating D3D7 semantics directly -- no asset re-parse, no guessed
   lighting/ground/terrain. Activated when JN_REPLAY=<path.omtc> is set;
   bypasses the imitation game loop. */

int  replay_active(void);                 /* JN_REPLAY env present? */
int  replay_init(int viewport_w, int viewport_h);  /* load + GL setup */
void replay_render_frame(void);           /* re-render the captured frame */
void replay_destroy(void);

#endif
