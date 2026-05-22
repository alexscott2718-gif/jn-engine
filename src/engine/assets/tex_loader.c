#include "tex_loader.h"
#include "../glad.h"
#include "../capture.h"
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

unsigned int tex_load(const char *path) {
    int w, h, ch;
    stbi_set_flip_vertically_on_load(1);
    unsigned char *data = stbi_load(path, &w, &h, &ch, 0);
    if (!data) {
        fprintf(stderr, "tex_load: failed to load %s: %s\n", path, stbi_failure_reason());
        return 0;
    }

    GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;

    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    printf("tex_load: %s  %dx%d ch=%d\n", path, w, h, ch);
    capture_note_texture(id, path, w, h);
    return id;
}

void tex_free(unsigned int id) {
    if (id) glDeleteTextures(1, &id);
}
