#ifndef TEX_LOADER_H
#define TEX_LOADER_H

/* Load a PNG/JPG/BMP from disk into an OpenGL texture.
   Returns GL texture ID, or 0 on failure. */
unsigned int tex_load(const char *path);
/* Same, but loaded with the OPPOSITE vertical orientation (for applying a
   standalone PNG to a mesh whose UVs use the DX/glb-baked convention). */
unsigned int tex_load_vflip(const char *path);
void         tex_free(unsigned int tex_id);

#endif
