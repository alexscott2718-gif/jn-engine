#ifndef CAPTURE_SCENE_H
#define CAPTURE_SCENE_H

enum {
    CAPTURE_SCENE_GROUP_STATIC_WORLD = 0,
    CAPTURE_SCENE_GROUP_HUD = 1,
    CAPTURE_SCENE_GROUP_PLAYER_JIMMY = 2,
};

int  capture_scene_init(const char *path);
int  capture_scene_active(void);
int  capture_scene_is_world_mode(void);
void capture_scene_set_group_visible(int group, int visible);
void capture_scene_set_group_ndc_offset(int group, float x, float y);
void capture_scene_set_group_world_offset(int group, float x, float y, float z);
void capture_scene_set_group_use_world(int group, int use);
void capture_scene_set_world_view_proj(const float view[16], const float proj[16]);
void capture_scene_render(int viewport_w, int viewport_h);
void capture_scene_destroy(void);

#endif
