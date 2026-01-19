#ifndef foliage_renderer_h
#define foliage_renderer_h

#include <block_renderer.h>
#include <liquid_renderer.h>
#include <skybox.h>
#include <glad/glad.h>
#include <chunk_mesh.h>
#include <mesh.h>
#include <sort.h>
#include <world.h>
#include <util.h>
#include <settings.h>
#include <world_mesh.h>
#include <assert.h>

block_renderer create_foliage_renderer(camera* cam, char* atlas_path, char* bump_path, char* caustic_path);
void destroy_foliage_renderer(block_renderer br);

void render_foliage(block_renderer* br, sun* sun, FBO* map, world_mesh* packet);
#endif