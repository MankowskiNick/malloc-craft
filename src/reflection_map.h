#ifndef REFLECTION_MAP_H
#define REFLECTION_MAP_H

#include <fbo.h>

FBO create_reflection_map(uint width, uint height);
void render_reflection_map(FBO* map, camera* cam, float water_level, world_mesh* packet);

#endif
