# Phase 2: Ghidra Static Analysis — Jimmy Neutron PC Port (JNBG Decomp)

**Date:** 2026-05-12  
**Status:** ✅ Complete — Both binaries fully imported and analyzed

---

## Key Findings Summary

| Question | Answer |
|----------|--------|
| DirectX version | DirectX 8 (game ships DX8 installer) |
| 3D renderer | **DirectDraw** (DDRAW.DLL in OMT2.dll) — NOT D3D8/D3D9 |
| Audio backend | **DirectSound** (DSOUND.DLL in OMT2.dll; also in Neutron.exe) |
| Input backend | **DirectInput 8** (DINPUT8.DLL in OMT2.dll) |
| Game architecture | Thin shell: Neutron.exe delegates ALL rendering/input to OMT2.dll |
| Engine API surface | **1,358 exports** across **110 C++ classes** |
| Phase 3 renderer target | SDL2 + SDL_Surface/OpenGL 1.x (matches DirectDraw model) |

---

## Binary Overview

### Neutron.exe — Game Executable
- **Type:** PE32 x86 (0x014c), Windows subsystem
- **Imports:** Only 4 DLLs — `omt2.dll`, `dsound.dll`, `kernel32.dll`, `user32.dll`
- **Exports:** None
- **Architecture:** Thin shell — all graphics, input, asset loading delegated to OMT2
- **Ghidra:** ✅ Imported + analyzed (191 sec)

### OMT2.dll — Engine Library
- **Type:** PE32 x86 DLL (0x014c)
- **Imports:** `DDRAW.DLL`, `DSOUND.DLL`, `DINPUT8.DLL`, `WINMM.DLL`, `USER32.DLL`, `GDI32.DLL`, `COMDLG32.DLL`, `KERNEL32.DLL`
- **Exports:** 1,358 functions across 110 C++ classes (see `omt2_exports.csv`)
- **Ghidra:** ✅ Imported + analyzed (308 sec)

---

## Renderer Design (Phase 3 Implication)

OMT2.dll imports **DirectDraw** (`DDRAW.DLL`) — not `d3d8.dll` or `d3d9.dll`. This means:
- Rendering is surface-based (DirectDraw surfaces, not programmable pipeline)
- No vertex/pixel shaders — fixed-function only
- **Phase 3 target: SDL2 + SDL_Surface for 2D, or OpenGL 1.x fixed-function for 3D**
- DirectX 8 is shipped by the installer primarily for **DirectInput 8** (not D3D8)

The game's rendering model is DX7-era despite the DX8 installer.

---

## OMT2 Engine Class Taxonomy (110 classes)

### Rendering
| Class | Role |
|-------|------|
| `OMediaRenderer` | Renderer base class |
| `OMediaDXRenderer` | DirectX (DirectDraw) renderer |
| `OMediaOMTRenderer` | OMT-format renderer |
| `OMediaRenderPort` | Render port / viewport base |
| `OMediaDXRenderPort` | DirectX render port |
| `OMediaOMTRenderPort` | OMT render port |
| `OMediaCanvas` | 2D sprite/image canvas |
| `OMediaDXCanvas` | DirectX canvas (surface-backed) |
| `OMediaOMTCanvas` | OMT canvas |
| `OMediaOffscreenBuffer` | Offscreen render target base |
| `OMediaDXOffscreenBuffer` | DirectX offscreen buffer |
| `OMediaWinOffscreenBuffer` | Windows offscreen buffer |
| `OMediaLayer` | Rendering layer |
| `OMediaViewPort` | Viewport |
| `OMediaPipeline` | Rendering pipeline |

### Scene Graph / World
| Class | Role |
|-------|------|
| `OMediaWorld` | Root scene/world object |
| `OMediaElement` | Scene element base |
| `OMediaElementContainer` | Element collection |
| `OMediaPrimitiveElement` | Renderable 3D primitive |
| `OMediaSurfaceElement` | Surface-based 2D element |
| `OMediaLight` | Scene lighting |
| `OMediaParticleEmitter` | Particle system |
| `OMediaLayer` | Scene layer |

### 3D Geometry
| Class | Role |
|-------|------|
| `OMedia3DShape` | 3D mesh |
| `OMedia3DPolygon` | Polygon |
| `OMedia3DMaterial` | Material definition |
| `OMedia3DShapeElement` | Shape scene element |

### Animation
| Class | Role |
|-------|------|
| `OMediaAnim` | Animation base |
| `OMediaAnimDef` | Animation definition |
| `OMediaAnimFrame` | Animation frame |
| `OMediaAnimPeriodical` | Time-driven animation |
| `OMedia3DMorphAnim` | Morph/blend-shape animation |
| `OMedia3DShapeAnim` | 3D shape animation |
| `OMediaCanvasAnim` | Sprite animation |

### Audio
| Class | Role |
|-------|------|
| `OMediaSoundEngine` | Audio engine base |
| `OMediaDXSoundEngine` | DirectSound engine |
| `OMediaWinSoundEngine` | Windows audio engine |
| `OMediaSound` | Sound asset |
| `OMediaDXSound` | DirectX sound asset |
| `OMediaWinRtgSound` | Windows sound (retarget) |
| `OMediaSoundChannel` | Playback channel |
| `OMediaDXSoundChannel` | DirectX playback channel |
| `OMediaWinSoundChannel` | Windows playback channel |

### Input
| Class | Role |
|-------|------|
| `OMediaInputEngine` | Input engine base |
| `OMediaDXInputEngine` | DirectInput 8 engine |
| `OMediaWinInputEngine` | Windows input engine |
| `OMediaHumanInterfaceDevice` | HID device |
| `OMediaDXHID` | DirectX HID |
| `OMediaDXHIDElement` | DX HID input element |
| `OMediaWinHID_Keyboard` | Keyboard input |
| `OMediaFFEffect` | Force feedback effect |
| `OMediaDXFFEffect` | DirectX force feedback |

### Window / Video
| Class | Role |
|-------|------|
| `OMediaWindow` | Window base |
| `OMediaSingleWindow` | Single-window app |
| `OMediaFullscreenWindow` | Fullscreen window |
| `OMediaVideoEngine` | Video/display engine base |
| `OMediaDXVideoEngine` | DirectX video engine |
| `OMediaWinVideoEngine` | Windows video engine |

### Asset Converters / I/O
| Class | Role |
|-------|------|
| `OMediaOMTConverter` | OMT container format |
| `OMediaASEConverter` | ASE 3D mesh format |
| `OMediaDXFConverter` | DXF (AutoCAD) format |
| `OMediaXSIConverter` | XSI format |
| `OMedia3DShapeConverter` | Generic 3D shape I/O |
| `OMediaCanvasConverter` | Image/canvas I/O |
| `OMediaGIFConverter` | GIF image |
| `OMediaPNGConverter` | PNG image |
| `OMediaStream` | Stream base |
| `OMediaFileStream` | File I/O stream |
| `OMediaMemStream` | In-memory stream |
| `OMediaClassStreamer` | Object serialization |
| `OMediaFilePath` | File path abstraction |
| `OMediaWinRtgFilePath` | Windows file path |

### UI Widgets
| Class | Role |
|-------|------|
| `OMediaAbstractButton` | Button base |
| `OMediaCanvasButton` | Canvas/sprite button |
| `OMediaStdButton` | Standard button |
| `OMediaCaption` | Text label |
| `OMediaCanvasFont` | Bitmap font |
| `OMediaScroller` | Scrollable area |
| `OMediaSlider` | Slider widget |
| `OMediaStringField` | Text input |
| `OMediaRadioGroup` | Radio button group |

### System / Framework
| Class | Role |
|-------|------|
| `OMediaApplication` | Application entry point |
| `OMediaEngineFactory` | Engine subsystem factory |
| `OMediaEventManager` | Event dispatch |
| `OMediaBroadcaster` | Event broadcaster |
| `OMediaListener` | Event listener |
| `OMediaPeriodical` | Per-frame tick base |
| `OMediaSupervisor` | System supervisor |
| `OMediaRetarget` | Platform abstraction layer |
| `OMediaDBObject` | Database object base |
| `OMediaDataBase` | Object database |
| `OMediaCollisionCache` | Collision detection |
| `OMediaCosSinTable` | Trig lookup table |
| `OMediaMoveableMem` | Relocatable memory |

---

## RTTI Type Found During Analysis

Ghidra found during RTTI analysis:
- `enum omt_WinSerialError` — serial/USB error codes (suggests serial port or hardware key support)
- `enum omt_InitAppHints` — application init flags (controls startup behavior)
- `enum omt_FFEffectType` / `omt_FFEffectSubType` — force feedback joystick effects

---

## Ghidra Project

```
Project:   /home/scotty/ghidra-projects/JN_decomp.rep
Neutron:   Analyzed ✅ (191 sec)
OMT2.dll:  Analyzed ✅ (308 sec)

Open with:
  export JAVA_HOME=/home/scotty/jdk21
  /home/scotty/ghidra/ghidraRun
  File → Open Project → /home/scotty/ghidra-projects/JN_decomp.rep
```

---

## Output Files

| File | Contents |
|------|----------|
| `docs/ghidra_notes.md` | This file — full analysis summary |
| `docs/omt2_exports.csv` | All 1,358 OMT2.dll exports (ordinal + mangled name) |

---

## What Phase 3 Must Implement

Based on this analysis, the C/SDL2 engine needs to replace the following OMT2 subsystems:

1. **Renderer** — SDL2_Surface or OpenGL 1.x fixed-function (replaces `OMediaDXRenderer` / DirectDraw)
2. **Canvas / Sprite** — 2D image blitting (replaces `OMediaCanvas`, `OMediaDXCanvas`)
3. **World / Scene Graph** — scene hierarchy (replaces `OMediaWorld`, `OMediaElement`, `OMediaElementContainer`)
4. **3D Geometry** — mesh loading + rendering (replaces `OMedia3DShape`, `OMedia3DPolygon`, `OMedia3DMaterial`)
5. **Animation** — keyframe + morph (replaces `OMediaAnim`, `OMedia3DMorphAnim`)
6. **Audio** — SDL_mixer or OpenAL (replaces `OMediaDXSoundEngine`, `OMediaDXSound`)
7. **Input** — SDL2 events (replaces `OMediaDXInputEngine`, `OMediaDXHID`)
8. **Asset Loading** — OMT container, ASE, PNG (replaces `OMediaOMTConverter`, `OMediaASEConverter`, etc.)
9. **Window** — SDL2 window (replaces `OMediaWindow`, `OMediaFullscreenWindow`)
10. **Event / Tick** — SDL2 event loop + fixed timestep (replaces `OMediaEventManager`, `OMediaPeriodical`)

---

# Phase 11: OMT2.dll Render-Pipeline Deep Dive (Stage B)

**Date:** 2026-05-15
**Scripts:** `~/ghidra-scripts/Phase11_OMT2_Inventory.java`, `Phase11_OMT2_Decompile.java`
**Outputs:** `/tmp/phase11_OMT2_inventory.txt`, `/tmp/phase11_OMT2_decompile.txt`

All 3D rendering lives in **OMT2.dll** (not Neutron.exe). OMT2.dll exposes
named symbols — full class/method visibility. It targets **DirectDraw7 /
Direct3D7**, not Direct3D8.

## Render entry chain

`OMedia3DShapeElement::render_geometry` → `OMediaDXRenderPort::draw_shape`
(`0x100371c0`) → `IDirect3DDevice7::DrawPrimitive` (device vtable slot
`0x64`).

- FVF is `0x152` = `D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE |
  D3DFVF_TEX1` → 9 DWORDs / 36-byte vertex.
- The engine calls **DrawPrimitive** (triangles pre-expanded into a vertex
  buffer), not DrawIndexedPrimitive.
- `draw_shape` copies each corner's UV straight into the vertex buffer
  (`vb[7]=u; vb[8]=v`) — **no UV transform, no V-flip**.

## Material / texture

- Each face holds a **direct material pointer** at `face+0x4`; the
  material's texture (canvas) pointer is at `material+0x80`. MTLID is
  resolved to a pointer at load time — there is no render-time MTLID
  table lookup.
- Texture wrap: `OMediaDXRenderPort::set_texture` sets
  `D3DTSS_ADDRESS = WRAP` (UVs tile). Min/mag/mip filter come from
  canvas fields `+0x60` / `+0x64`.
- `OMediaDXRenderTarget::check_texture_size` clamps texture dimensions
  to **256×256 max** and rounds up to a power of two.

## Bitmap loading

There is **no runtime BMP loader**. Textures are decoded from OMT
canvases via `OMediaOMTConverter` / `OMediaPNGConverter` /
`OMediaGIFConverter`. `OMediaOMTConverter::create_canvas` copies bitmap
rows in source order (no Y-flip). ASE `*BITMAP` paths are authoring-time
references only.

These findings drove the Phase 11 fixes: the UV double-flip removal and
the canvas-table material resolution (see `omt_3dsp_format.md`).
