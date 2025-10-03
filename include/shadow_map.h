#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include <fbo.h>

FBO create_shadow_map(uint width, uint height);
void render_shadow_map(FBO* map, sun* s, world_mesh* packet);

#endif
