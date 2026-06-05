# jn-engine

A clean-room reimplementation of the **Open Media Toolkit 2.0** engine that ran
*Jimmy Neutron: Boy Genius* (THQ, 2002), plus the instrumentation that captures the
original game's **Direct3D 7** output on real Windows XP to use as ground truth.

One codebase builds a **native Linux** game, a **WebAssembly** browser demo, a
**D3D7-replay** renderer (renders the original's captured command stream verbatim),
and a **capture** build. The same engine also runs the sequel, *Jimmy Neutron vs.
Jimmy Negatron* (JNvsJN).

> **Status:** active research/reconstruction project. Level 1 of *Boy Genius* renders
> faithfully (measured against the original's capture); JNvsJN runs with 22 levels and
> a gameplay layer. See the history doc for where each piece stands.

**▶ Play in your browser** — [Boy Genius](https://exentt.com/jn-engine/) · [JNvsJN](https://exentt.com/jnvsjn/) · or the **[project hub →](https://exentt.com/jn)** (demos, assets & docs in one place).

## Start here

| If you want to… | Read |
|---|---|
| Understand **how the project got here and why** (and which approaches are dead ends) | [`docs/PROJECT_HISTORY.md`](docs/PROJECT_HISTORY.md) |
| Understand **the codebase as it is now** — subsystems, the three runtime modes, what's current vs historical | [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) |
| **Pick up a self-contained task** | [`docs/CONTRIBUTOR_object_capture_plan.md`](docs/CONTRIBUTOR_object_capture_plan.md) |
| **Format references** | [`docs/omt_3dsp_format.md`](docs/omt_3dsp_format.md), [`docs/omt_rendering_breakthrough.md`](docs/omt_rendering_breakthrough.md), [`docs/ghidra_notes.md`](docs/ghidra_notes.md) |

## Build

Native (Linux, SDL2 + OpenGL):

```bash
make              # -> ./jnengine
./jnengine --level level1
```

WebAssembly (Emscripten, WebGL2):

```bash
source ~/emsdk/emsdk_env.sh
make web          # -> web/jnengine.{html,js,wasm,data}
```

Other targets: `make capture` (D3D7-comparable capture build), `make web-jnvsjn`
(sequel browser bundle), plus a suite of native-vs-capture validators. See the
`Makefile` and [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) §9.

Controls: W/S move, A/D turn, Space jump, Shift run, R respawn, LMB free-look, Esc quit.

## Repo layout

```
src/engine/    platform + renderer + the replay/capture paths
src/engine/assets/   on-disk format loaders + asset cache
src/game/      entity model, game loop, gameplay (behaviors/)
instrument/    the capture pipeline: D3D7 proxy, receiver, diff/analysis tools
tools/         parsers, exporters, level builders, validators
assets/        game data (source formats + derived .glb/.png)
web/           WASM shell + built browser bundles
docs/          history, architecture, format references
```

## Legal

This is a **reverse-engineering project for preservation and interoperability**. The
engine code here is an independent clean-room reimplementation. *Jimmy Neutron* and
its assets are the property of their respective owners (Nickelodeon / THQ); this
repository does **not** redistribute the original game, its data files, or its
binaries. You need a legitimate copy of the original game to extract assets for
personal use.

🤖 Documentation generated with [Claude Code](https://claude.com/claude-code)
