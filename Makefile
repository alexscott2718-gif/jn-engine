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

.PHONY: all clean web capture
