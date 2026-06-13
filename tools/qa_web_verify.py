#!/usr/bin/env python3
"""Headless acceptance check for the in-game QA annotation mode (web build).

Serves web/ with Cache-Control: no-store on an ephemeral port (browsers
heuristically cache jnengine.{js,wasm,data} across rebuilds because the URLs
never change — stale mixed builds look like broken keybindings/buttons), then
drives the page in headless Chromium via Playwright:

  M1:  QA nav button toggles the mode (mobile path)
       hover over the canvas centre shows the #qaTip tooltip
       the B key toggles the mode off and back on
  M2:  click-pick opens the issue dialog
       category + message + OK appends to localStorage and the tag stack
       typing in the dialog does not leak keys into SDL ("b" must not toggle)
       Cancel discards; per-tag delete works; reports survive a page reload

Usage:  python3 tools/qa_web_verify.py        (after `make web`)
Needs:  pip install playwright && python3 -m playwright install chromium
"""
import os
import sys
import threading
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WEB = os.path.join(REPO, "web")


class NoStoreHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def log_message(self, *args):
        pass


def main():
    from playwright.sync_api import sync_playwright

    os.chdir(WEB)
    server = ThreadingHTTPServer(("127.0.0.1", 0), NoStoreHandler)
    port = server.server_address[1]
    threading.Thread(target=server.serve_forever, daemon=True).start()

    logs, failures = [], []

    def check(name, ok, detail=""):
        print(f"  {'PASS' if ok else 'FAIL'}  {name}  {detail}")
        if not ok:
            failures.append(name)

    def wait_for_engine(page):
        # Main loop is up once main.c starts rewriting the title with FPS.
        page.wait_for_function(
            "document.title.startsWith('JN Engine')", timeout=180_000)
        page.wait_for_timeout(2000)

    with sync_playwright() as p:
        browser = p.chromium.launch()
        page = browser.new_page(viewport={"width": 1440, "height": 1000})
        page.on("console", lambda m: logs.append(m.text))
        page.on("pageerror", lambda e: logs.append(f"PAGEERROR: {e}"))
        page.goto(f"http://127.0.0.1:{port}/jnengine.html?level=level1")
        wait_for_engine(page)
        page.evaluate("localStorage.removeItem('jnqa_reports_v1')")

        btn = page.locator("#qaToggle")
        dlg = page.locator("#qaDialog")
        check("initial label", btn.text_content() == "QA: Off",
              btn.text_content())

        # --- M1: button + hover -----------------------------------------
        btn.click()
        page.wait_for_timeout(500)
        check("button toggles on", btn.text_content() == "QA: On",
              btn.text_content())

        box = page.locator("#canvas").bounding_box()
        cx = box["x"] + box["width"] / 2
        cy = box["y"] + box["height"] * 0.72   # follow-cam centres the player
        page.mouse.move(cx, cy)
        page.wait_for_timeout(300)
        page.mouse.move(cx + 4, cy + 3)        # second move beats the throttle
        page.wait_for_timeout(500)
        tip = page.locator("#qaTip")
        tip_text = tip.text_content() or ""
        check("hover tooltip", tip.is_visible() and tip_text != "", tip_text)

        # --- M2: pick -> dialog -> OK -----------------------------------
        page.mouse.click(cx + 4, cy + 3)
        page.wait_for_timeout(500)
        check("pick opens dialog", dlg.is_visible(),
              page.locator("#qaDlgIdent").text_content() or "")

        page.select_option("#qaCategory", "ORI")
        # The message deliberately contains b/w/a/s/d: none may leak to SDL.
        page.locator("#qaMessage").fill("body rotated 90 degrees backwards")
        page.locator("#qaOk").click()
        page.wait_for_timeout(500)
        check("dialog closes on OK", not dlg.is_visible())
        check("QA still on after typing", btn.text_content() == "QA: On",
              btn.text_content())

        tags = page.locator(".qaTag")
        stored = page.evaluate(
            "JSON.parse(localStorage.getItem('jnqa_reports_v1') || '[]')")
        check("tag rendered", tags.count() == 1,
              (tags.first.text_content() or "") if tags.count() else "none")
        ok_store = (len(stored) == 1 and stored[0]["category"] == "ORI"
                    and stored[0]["message"].startswith("body rotated")
                    and stored[0]["level"] == "level1"
                    and isinstance(stored[0].get("pos"), list))
        check("report stored", ok_store, json_brief(stored))

        # --- M2: cancel path ---------------------------------------------
        page.mouse.click(cx + 4, cy + 3)
        page.wait_for_timeout(500)
        if dlg.is_visible():
            page.locator("#qaCancel").click()
            page.wait_for_timeout(300)
        stored = page.evaluate(
            "JSON.parse(localStorage.getItem('jnqa_reports_v1') || '[]')")
        check("cancel discards", len(stored) == 1, f"{len(stored)} stored")

        # --- M1: B key -----------------------------------------------------
        # Focus the canvas by clicking SKY (top centre): with QA on, any click
        # on an object would pick it and re-open the dialog, swallowing the
        # keystrokes below into the textarea (that isolation is by design).
        box = page.locator("#canvas").bounding_box()
        # Top-left sky: the 2026-06-12 QA#4 rotation-sign correction flipped
        # the authored 220-degree spawn yaw to its faithful orientation,
        # which rotated the boot view. Open sky now spans the top-LEFT
        # quarter (x<=0.25 at the top edge); x=0.3 lands on a building now.
        # The top-RIGHT corner is covered by the #qaTags overlay, so the
        # left band is the safe sky click. (A click on any object would pick
        # it and re-open the dialog, swallowing the keystrokes below.)
        page.locator("#canvas").click(
            position={"x": box["width"] * 0.12, "y": 10})
        page.wait_for_timeout(300)
        check("no dialog after sky click", not dlg.is_visible())
        def wait_label(expect):
            # The label flips on the engine's next frame; under headless
            # load that can exceed a fixed 500ms — poll instead of sleeping.
            try:
                page.wait_for_function(
                    "document.getElementById('qaToggle').textContent === "
                    + repr(expect), timeout=5000)
            except Exception:
                pass
        page.keyboard.press("b")
        wait_label("QA: Off")
        check("B key toggles off", btn.text_content() == "QA: Off",
              btn.text_content())
        page.keyboard.press("b")
        wait_label("QA: On")
        check("B key toggles on", btn.text_content() == "QA: On",
              btn.text_content())

        # --- M2: reload persistence ---------------------------------------
        page.reload()
        wait_for_engine(page)
        tags = page.locator(".qaTag")
        check("tags survive reload", tags.count() == 1,
              (tags.first.text_content() or "") if tags.count() else "none")

        # --- M3: clipboard export ------------------------------------------
        page.context.grant_permissions(
            ["clipboard-read", "clipboard-write"],
            origin=f"http://127.0.0.1:{port}")
        page.locator("#qaExport").click()
        page.wait_for_timeout(500)
        clip = page.evaluate("navigator.clipboard.readText()")
        has_row = "| level1 | 3JIM [JIM1] | ORI |" in clip
        json_ok = False
        if "```json" in clip:
            import json as _json
            try:
                payload = _json.loads(
                    clip.split("```json", 1)[1].split("```", 1)[0])
                json_ok = (len(payload) == 1
                           and payload[0]["category"] == "ORI"
                           and "cam" in payload[0])
            except ValueError:
                pass
        check("export markdown row", has_row, clip.splitlines()[0] if clip else "empty")
        check("export fenced JSON", json_ok)

        # --- M2: per-tag delete --------------------------------------------
        page.locator(".qaTagDel").first.click()
        page.wait_for_timeout(300)
        stored = page.evaluate(
            "JSON.parse(localStorage.getItem('jnqa_reports_v1') || '[]')")
        check("tag delete", page.locator(".qaTag").count() == 0
              and len(stored) == 0, f"{len(stored)} stored")

        browser.close()
    server.shutdown()

    if failures:
        print(f"\n{len(failures)} FAILED: {', '.join(failures)}")
        print("---- console tail ----")
        for line in logs[-20:]:
            print(line[:300])
        sys.exit(1)
    print("\nall QA web checks passed")


def json_brief(stored):
    if not stored:
        return "empty"
    r = stored[0]
    return f"cat={r.get('category')} msg={r.get('message', '')[:24]!r} level={r.get('level')}"


if __name__ == "__main__":
    main()
