# Web (WebAssembly) Build

The engine compiles to WebAssembly via Emscripten and runs in any browser
supporting WebGL 2. Output lands in `web/` and is served by the host nginx
(see `~/CLAUDE.md` § _jn-engine Web Demo_ for hosting details).

## Prerequisites

Emscripten SDK at `~/emsdk/`. Activate it once per shell:

```bash
source ~/emsdk/emsdk_env.sh
```

System libs (SDL2, SDL2_mixer, zlib, ogg, vorbis) are fetched on first build
and cached under `~/emsdk/upstream/emscripten/cache/`.

## Building

```bash
cd ~/jn-engine && make web
```

Produces in `web/`:

| File | Purpose |
|---|---|
| `jnengine.html` | Emscripten launcher page |
| `jnengine.js`   | Glue: loads wasm, wires SDL ↔ DOM, drives main loop |
| `jnengine.wasm` | Compiled engine |
| `jnengine.data` | Preloaded `assets/` tree (~83 MB) |

Native and web builds share all sources; the differences are guarded by
`#ifdef __EMSCRIPTEN__`.

## Platform differences (what `__EMSCRIPTEN__` switches)

- **`src/engine/glad.h`** — on web, includes `<GLES3/gl3.h>` directly and stubs
  `glad_load_gl()` to return 1. WebGL contexts don't need a proc-address loader.
- **`src/engine/glad.c`** — entire body skipped on web.
- **`src/engine/window.c`** — requests `SDL_GL_CONTEXT_PROFILE_ES` major=3 minor=0
  to get a WebGL 2 context (native still requests Core 3.3).
- **`src/engine/renderer.c`** — `GLSL_VS` / `GLSL_FS` macros emit
  `#version 300 es` (plus `precision highp float;` for fragment shaders)
  instead of `#version 330 core`.

## Makefile flags

```
-sUSE_SDL=2 -sUSE_SDL_MIXER=2 -sUSE_ZLIB=1
-sFULL_ES3=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
-sALLOW_MEMORY_GROWTH=1 -sASYNCIFY -sEXIT_RUNTIME=0
--preload-file assets
```

- `-sASYNCIFY` lets the native `while(running)` main loop work unmodified by
  yielding to the browser on SDL/audio calls. Costs ~30% in size and some
  runtime overhead — a future cleanup is to refactor `main.c` to
  `emscripten_set_main_loop_arg(...)` and drop `-sASYNCIFY`.
- `--preload-file assets` bundles the whole tree into `jnengine.data`,
  fetched once on page load. Splitting this into a small boot pack plus
  lazy `emscripten_async_wget` of level/audio data is a future optimization.

## Demo scene

`src/game/main.c` loads `assets/gam/Level1.gam` and frames the camera on the
`3JIM` entity. Jimmy is drawn with the `assets/ase/jimstop.ase` idle mesh
textured from `assets/png/vrjimmy.png` (the ASE's referenced `jimmylast.bmp`
isn't shipped, so the player texture is hardcoded). `STRT` start-marker
entities are skipped at render time — they sat as a magenta debug box around
Jimmy. Other entities without a model still render as their colored debug box.

## Deployment

`web/` is served live by nginx at `https://<DEBIAN_HOST>:4300/` (LAN) or
`https://<EXTERNAL_HOST>:8500/` (external). `make web` overwrites in place; no
nginx reload needed.

Access via the landing page at `https://<EXTERNAL_HOST>:8420/` (LAN: `https://<DEBIAN_HOST>:8420/`),
which links all gateway services.

## Known issues / TODO

- 84 MB initial download — trim assets or split into lazy packs.
- ASYNCIFY overhead — switch to `emscripten_set_main_loop_arg`.
- TLS cert is a real Let's Encrypt cert for `<EXTERNAL_HOST>` (shared with ttyd, auto-renewed by acme.sh via Cloudflare DNS-01).

## See also

- `docs/gateway.md` — Complete gateway infrastructure & service guide
