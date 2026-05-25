#ifndef CAPTURE_SCENE_H
#define CAPTURE_SCENE_H

int  capture_scene_init(const char *path);
int  capture_scene_active(void);
void capture_scene_render(int viewport_w, int viewport_h);
void capture_scene_destroy(void);

#endif
