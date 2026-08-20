#ifndef GADGET_MENU_H
#define GADGET_MENU_H

/* The in-game action menu -- "AMI" in the original's own trace strings -- and
   the action mode it selects.
   Evidence: docs/decomp/C3DJimmy.md, docs/decomp/C2DInGameMenu.md, and the
   recovered bodies in docs/decomp/evidence/c3djimmy_target6.md.

   In the original this is Jimmy field 0xa18, a code-created C2DInGameMenu that
   is simultaneously the HUD overlay and the gadget/menu command endpoint, and
   Jimmy talks to it through eighteen vtable slots. None of that protocol is
   ported here and none of it should be: the C3DJimmy/gadget-mode-dispatch
   certificate is linked-blocked precisely because native has no such
   controller, and building a fake one would certify a different design.

   What IS ported is the part that is observable without the controller:

     * the enter/exit lock (JimmyEnterActionMenuLock 00425ef0 and its reverse
       00425b20) -- the guard, the open latch, the pause, the cursor, the
       cooldown clear, and the flag flips, in the decompiled order;
     * the AMI request-id tables from SelectJimmyGadgetOrVRMode (00428d50) --
       which action mode each id writes to DAT_004f0588, and which VR level
       each id routes to.

   What is deliberately NOT ported, because it was never recovered: which AMI
   id a given gadget corresponds to. That mapping lives in the C2DInGameMenu
   canvas records the hud-draw certificate is still blocked on. So the menu
   selects a gadget (native chrome, our layout) and the AMI table is exposed
   separately for callers that hold a real id. The two are not joined by a
   guess. */

/* --- Action modes -----------------------------------------------------------
   The value set written to DAT_004f0588 by AMI dispatch. Only two are named:
   those are the two whose meaning is pinned by a recovered body. The rest keep
   their numbers, with the evidence for what they probably are in the comment,
   because naming them would be interpretation dressed as fact. */
#define ACTION_MODE_NONE    (-1)  /* sentinel; helper 110 keys on it */
#define ACTION_MODE_IDLE      0   /* input tail: "mode 0 returns" */
#define ACTION_MODE_ROCKET    1   /* "Activating Rocket"/"ACT 2 Rocket", DRIVE */
#define ACTION_MODE_2         2   /* beep 0x75 + latch; helper 114 runs a
                                     charge/progress meter; helper 80 holds a
                                     looping sound only in this mode */
#define ACTION_MODE_4         4   /* input tail returns; controller update has
                                     dedicated ==4 and !=4 branches */
#define ACTION_MODE_5         5   /* C3DMetalPickup points the companion at a
                                     can with mode 5, releases back to 2 */
#define ACTION_MODE_AIM       6   /* aim/shoot: clamps pitch to [0,45], builds
                                     (0, aim+80, 45) through slot 0x384, SHOOT */
#define ACTION_MODE_7         7   /* helpers 98 and 112 gate on != 7;
                                     "mode-7 trajectory state" */

/* AMI request ids are 0..8. dispatch() writes the mode for the id and, when
   vr_routing is set, returns the VR level the id routes to. */
#define AMI_ID_MAX 8

/* The .gam the given AMI id routes to in VR mode ("vr01".."vr08"), or NULL if
   the id has no route. Entry point is always PHONEBOOTH. */
const char *ami_vr_level(int ami_id);

/* Apply an AMI request: writes the action mode for that id. Returns the mode
   now in force. Ids with no mode write leave it unchanged. */
int ami_dispatch(int ami_id);

/* The current action mode (the DAT_004f0588 mirror). */
int  action_mode(void);
void action_mode_set(int mode);

/* --- The menu ------------------------------------------------------------ */

/* DAT_004ec494 -- set by the C2DInGameMenu constructor in the original, so the
   menu becomes available once the overlay exists. Enter is a no-op until then. */
void gadget_menu_set_available(int available);

/* JimmyEnterActionMenuLock (00425ef0). Guarded on available && !open, so a
   second call while open does nothing, exactly as the original's
   DAT_004f8181 test does. `param` is the enter argument: the original skips
   showing the cursor when it is 2. Returns 1 if the menu opened. */
int  gadget_menu_open(int param);

/* JimmyExitActionMenuLock (00425b20). Guarded on open. */
void gadget_menu_close(void);

int  gadget_menu_is_open(void);

/* Per-frame while open: Up/Down move, Enter selects, Tab/Esc closes. */
void gadget_menu_input(void);

/* Draw the overlay. Call after the 3D scene, like menu_draw. */
void gadget_menu_draw(int viewport_w, int viewport_h);

#endif
