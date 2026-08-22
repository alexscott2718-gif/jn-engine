/* Headless probe for src/game/ui_text.c. Renderer/cache calls are captured so
   the test can verify glyph mapping, layout, and atlas UVs without OpenGL. */
#include <stdio.h>
#include "../src/game/ui_text.h"

static int draw_count;

unsigned int tex_cache_get(const char *path) {
    printf("T|%s\n", path ? path : "(null)");
    return 77;
}

void renderer_draw_mask_region_2d(unsigned int tex, int viewport_w, int viewport_h,
                                  float x, float y, float width, float height,
                                  float u0, float v_top, float u1, float v_bottom,
                                  float r, float g, float b, float a) {
    printf("D|%d|%u|%d|%d|%.3f|%.3f|%.3f|%.3f|%.9f|%.9f|%.9f|%.9f|%.2f|%.2f|%.2f|%.2f\n",
           draw_count++, tex, viewport_w, viewport_h, x, y, width, height,
           u0, v_top, u1, v_bottom, r, g, b, a);
}

int main(void) {
    /* Boundaries of every mapped run, plus two characters the atlas has
       no cell for, so both directions of a mapping mistake show up. */
    const char probes[] = { 'A', 'Z', 'a', 'z', '0', '9', ' ',
                            '(', '-', '.', ':', '@', '%', '~', '^' };
    for (unsigned int i = 0; i < sizeof(probes); i++)
        printf("G|%d|%d\n", (unsigned char)probes[i], ui_text_glyph_index(probes[i]));
    printf("M|%.3f|%.3f\n", ui_text_measure("New Game", 2.0f),
           ui_text_line_height(2.0f));
    ui_text_draw(640, 480, 10.0f, 20.0f, 2.0f, "New Game",
                 0.25f, 0.50f, 0.75f, 1.0f);
    return 0;
}
