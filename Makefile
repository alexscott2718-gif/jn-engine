HOME_DIR     = $(HOME)
TOOLCHAIN    = $(HOME_DIR)/toolchain/usr/bin
CC           = $(HOME_DIR)/zig/zig cc -target x86_64-linux-gnu
SDL2_INC     = $(HOME_DIR)/sdl2/include
SDL2_LIB     = $(HOME_DIR)/sdl2/lib
X11_INC      = $(HOME_DIR)/toolchain/usr/include
X11_LIB      = $(HOME_DIR)/toolchain/usr/lib/x86_64-linux-gnu

CFLAGS  = -Wall -O2 \
          -I$(SDL2_INC) -I$(SDL2_INC)/SDL2 \
          -I$(X11_INC) \
          -Isrc/engine

LIBS    = $(SDL2_LIB)/libSDL2.a \
          $(SDL2_LIB)/libSDL2_mixer.a \
          $(SDL2_LIB)/libGL.so \
          -L$(X11_LIB) -lX11 -lXext \
          -lm -ldl -lpthread -lz

SRC     = $(wildcard src/engine/*.c src/engine/assets/*.c src/game/*.c src/game/behaviors/*.c)
OBJ     = $(SRC:.c=.o)
TARGET  = jnengine

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LIBS) -Wl,-rpath,$(X11_LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
	# Remove web build outputs only -- keep web/shell.html (tracked source),
	# otherwise `make capture` (which runs clean) wipes the WASM shell.
	rm -f web/jnengine.html web/jnengine.js web/jnengine.wasm web/jnengine.data

# --- M6 instrumentation build ----------------------------------------------
# `make capture` builds jnengine with the demo-side render-stream capture
# (src/engine/capture.c) compiled in. Run it with JN_CAPTURE=<path> set to
# record an .omtc session for instrument/diff/diff.py. Clean rebuild so no
# stale objects compiled without -DJN_CAPTURE linger.
capture: CFLAGS += -DJN_CAPTURE
capture: clean $(TARGET)
	@echo "built jnengine with -DJN_CAPTURE -- run with JN_CAPTURE=<out.omtc>"

# Validate the accepted Level 1 v4 HUD-fix replay fixture. The large .omtc and
# generated inspect output live under build/; only the manifest is tracked.
REPLAY_HUDFIX_MANIFEST = assets/capture/level1_hudfix/frame_meta.json

replay-hudfix: $(TARGET)
	python3 tools/validate_replay_fixture.py $(REPLAY_HUDFIX_MANIFEST)

capture-static: $(TARGET)
	python3 tools/validate_capture_backed_static.py

capture-live-jimmy: $(TARGET)
	python3 tools/validate_capture_backed_live_jimmy.py

capture-live-hud: $(TARGET)
	python3 tools/validate_capture_backed_live_hud.py

capture-multiframe: $(TARGET)
	python3 tools/validate_capture_backed_multiframe.py

hybrid-level1: $(TARGET)
	python3 tools/validate_hybrid_level1.py

hybrid-level1-manifest:
	python3 tools/build_hybrid_level1_manifest.py

phase1-sky-tint:
	python3 tools/sample_phase1_sky_tint.py

native-level1-map:
	python3 tools/build_native_level1_map.py
	python3 tools/build_native_keyframe_cameras.py

native-level1: $(TARGET) native-level1-map
	python3 tools/validate_native_level1_map.py
	python3 tools/validate_native_keyframe_alignment.py --keyframe 8881 --write-report

# Sweep alignment check across every solved keyframe descriptor. Useful when
# adjusting the native basis or keyframe camera generator — any descriptor
# whose camera now points at empty space will fail.
native-level1-keyframes:
	@for f in assets/native/keyframe_cameras/*.txt; do \
		kf=$$(basename $$f .txt); \
		echo "=== keyframe $$kf ==="; \
		python3 tools/validate_native_keyframe_alignment.py --keyframe $$kf || exit 1; \
	done

diff-native-capture:
	@test -f build/native_keyframe_alignment_8881.json || $(MAKE) native-level1
	python3 tools/diff_native_capture_keyframe.py

native-vs-capture-8881-review: native-level1 diff-native-capture
	python3 tools/build_native_capture_side_by_side.py

capture-fixture:
	python3 tools/build_level1_hudfix_fixture.py

# Build the multi-frame world fixture from the full Level 1 capture. Picks
# keyframes by camera-spread, then unions their static-world draws into
# scene_world.bin. Source captures live in build/ and are not committed.
capture-world-fixture:
	python3 tools/extract_world_keyframes.py
	python3 tools/solve_keyframe_views.py
	python3 tools/build_multiframe_world_fixture.py

# Just resolve VIEW per keyframe (slow: re-reads the full capture). Useful
# when keyframes.json changed but extract_world_keyframes.py output is current.
solve-keyframe-views:
	python3 tools/solve_keyframe_views.py

# --- WebAssembly (Emscripten) build ---------------------------------------
# Run `source ~/emsdk/emsdk_env.sh` once per shell before `make web`.
EMCC        = emcc
WEB_SRC     = $(filter-out src/engine/glad.c,$(SRC))
WEB_OUT_DIR = web
WEB_TARGET  = $(WEB_OUT_DIR)/jnengine.html

WEB_CFLAGS  = -O2 -Isrc/engine \
              -sUSE_SDL=2 -sUSE_SDL_MIXER=2 -sUSE_ZLIB=1

WEB_LDFLAGS = -sUSE_SDL=2 -sUSE_SDL_MIXER=2 -sUSE_ZLIB=1 \
              -sFULL_ES3=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
              -sALLOW_MEMORY_GROWTH=1 -sASYNCIFY \
              -sEXIT_RUNTIME=0 \
              --preload-file assets \
              --shell-file web/shell.html

web:
	mkdir -p $(WEB_OUT_DIR)
	$(EMCC) $(WEB_CFLAGS) $(WEB_SRC) $(WEB_LDFLAGS) -o $(WEB_TARGET)

.PHONY: all clean web capture replay-hudfix capture-static capture-live-jimmy capture-live-hud capture-multiframe hybrid-level1 hybrid-level1-manifest native-level1-map native-level1 native-level1-keyframes diff-native-capture native-vs-capture-8881-review phase1-sky-tint capture-fixture capture-world-fixture solve-keyframe-views
