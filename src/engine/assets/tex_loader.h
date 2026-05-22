#ifndef TEX_LOADER_H
#define TEX_LOADER_H

/* Load a PNG/JPG/BMP from disk into an OpenGL texture.
   Returns GL texture ID, or 0 on failure. */
unsigned int tex_load(const char *path);
void         tex_free(unsigned int tex_id);

#endif
