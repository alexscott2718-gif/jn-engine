# Reporting Visual Bugs In-Game (QA Annotate Mode)

The playable demos at https://exentt.com/jn-engine/ and https://exentt.com/jnvsjn/
have a built-in bug-reporting mode. When you spot a misplaced, misoriented,
glitched, or broken-looking model, you can click it in-game and file a report —
the game records exactly which asset it is, where it sits in the level, and
where you were standing. No guessing about which "weird brown thing" you meant.

## How to use it

1. **Play normally** until you see something wrong (walk with WASD, `N` for
   noclip flying if you want to get closer).
2. **Press `B`** (think "bug") or tap the **`QA: Off`** button in the top bar —
   it flips to `QA: On`. Your mouse cursor now inspects instead of steering
   the camera.
3. **Hover** over any model: it lights up yellow and a small label shows its
   name (e.g. `labshak · placement` or `3JIM [JIM1] · entity`).
4. **Click** the broken model. It turns orange and a dialog opens.
5. In the dialog, pick a **category**:
   | Code | Meaning |
   |---|---|
   | PLC | wrong position |
   | ORI | wrong orientation / rotation |
   | SCL | wrong size |
   | ANI | animation wrong or missing |
   | TEX | texture wrong or missing |
   | MIS | model missing entirely |
   | GFX | other visual glitch |
   | OTH | anything else |
6. **Describe the issue** in a sentence ("rotated 90° vs original", "floats
   above the ground", "texture is the fence instead of bark") and hit **OK**.
   (Cancel or Esc discards.)
7. A tag appears in the top-right corner — your reports **stack up there as
   you keep playing**, and they survive switching levels. Hover a tag's ✕ to
   re-read its message, or click ✕ to delete it. Press `B` again to go back
   to normal play anytime.
8. When you're done with a session, hit **export** at the top of the tag
   stack — it copies the full report (a readable table plus machine-readable
   data) to your clipboard.
9. **Paste the clipboard contents into a Discord DM to `scotty`** (or the
   engine-troubleshooting channel). That paste contains everything needed to
   find and fix each issue — asset names, coordinates, camera position, your
   notes — so it goes straight into the engine's fix queue.
