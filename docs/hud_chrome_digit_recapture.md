# HUD chrome digits — XP recapture for 3, 4, 6

The HUD counters use the original game's runtime-generated **chrome digit font**
(purple→cyan italic, drop-outline), not the on-disk `green_font`/`fontsmall`.
We harvested **0,1,2,5,7,8,9** from the full Level-1 capture
(`tools/harvest_hud_digits.py`) — but the counters never displayed **3, 4, 6**
during that session, so those three glyphs were never dumped.

The native HUD already renders the chrome counters live from `GameState`
(`src/game/hud.c` → `draw_counter`), and draws a dim placeholder for any digit
whose PNG is missing. **Dropping in `big_3.png`, `big_4.png`, `big_6.png`
completes the set with no code or rebuild changes** (textures load by path at
runtime). This note is the procedure to get those three glyphs.

## Why a recapture (not just more scanning)

The chrome digit bitmaps are generated per displayed value and dumped once each
(SHA-1 gated). 3/4/6 are simply absent from `build/level1_v4_hudfix.omtc`. We
need a capture in which a counter actually shows those digits. No FRAME_MARK or
single-frame extraction is needed — the harvester scans the whole stream for
digit-sized `TEXTURE_PIXELS`, so the glyphs only have to appear *somewhere*.

## Procedure (human-piloted — interactive game runs must be on the XP noVNC desktop)

1. **Proxy.** Use the dynamic-HUD-capable proxy (the v4 build that captured the
   digit pixels for `level1_v4_hudfix` on 2026-05-25). Confirm it is the
   deployed `ddraw.dll` in the XP game dir; redeploy from
   `instrument/proxy/ddraw.dll` if needed (verify by download-back SHA-1 — XP has
   no certutil).

2. **Receiver** (on Debian):
   ```sh
   python3 instrument/receiver/receive.py serve --out build/level1_digits346.omtc
   ```

3. **Play (XP noVNC).** Launch JNBG, reach Level 1, and make the counters cycle
   through 3, 4, and 6 — the simplest route is to just play a few minutes
   collecting items/points so the top-left and score counters pass through many
   values (anything containing 3, 4, 6 works). Then close the game / `stop` the
   receiver.

4. **Harvest** (on Debian):
   ```sh
   python3 tools/harvest_hud_digits.py \
     --capture build/level1_digits346.omtc \
     --out build/hud_digit_harvest346
   ```
   Open `build/hud_digit_harvest346/sheet_32x32.png`, find the chrome **3, 4, 6**
   among the candidates, and save each as:
   ```
   assets/hud/capture/font/big_3.png
   assets/hud/capture/font/big_4.png
   assets/hud/capture/font/big_6.png
   ```
   (32×32, the same chrome style as the existing `big_*.png`.)

5. **Verify.**
   ```sh
   env LD_LIBRARY_PATH=/home/scotty/sdl2/lib JN_CAPTURE_BACKED_LEVEL1=1 \
     JN_CAPTURE_BACKED_LIVE_HUD=1 JN_HUD_TEST="346,3460" JN_SCREENSHOT=1 \
     JN_SCREENSHOT_PATH=build/hud_digits_verify.png \
     xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
   ```
   The placeholders for 3/4/6 should now be real chrome glyphs.

## Notes
- The small (16×16) score counter reuses the big chrome glyphs scaled down, so
  no separate small 3/4/6 are required.
- `JN_HUD_TEST="items,score"` forces counter values for QA against the captured
  frame (the original showed `17` / `250`).
