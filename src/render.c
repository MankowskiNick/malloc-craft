#include <render.h>

#include <block_renderer.h>
#include <blockbench_renderer.h>
#include <liquid_renderer.h>
#include <foliage_renderer.h>
#include <skybox.h>
#include <glad/glad.h>
#include <chunk_mesh.h>
#include <mesh.h>
#include <sort.h>
#include <world.h>
#include <util.h>
#include <settings.h>
#include <world_mesh.h>
#include <fbo.h>
#include <shadow_map.h>
#include <reflection_map.h>
#include <assert.h>

renderer create_renderer(camera* camera) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (WIREFRAME) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    w_init();
    m_init(camera);

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = (int)(camera->position[0]) - CHUNK_RENDER_DISTANCE + i;
            int z = (int)(camera->position[2]) - CHUNK_RENDER_DISTANCE + j;
            get_chunk_mesh(x, z);
            load_chunk();
        }
    }

    block_renderer wr = create_block_renderer(camera, ATLAS_PATH, BUMP_PATH, CAUSTIC_PATH);
    block_renderer lr = create_liquid_renderer(camera, ATLAS_PATH, BUMP_PATH, CAUSTIC_PATH);
    block_renderer fr = create_foliage_renderer(camera, ATLAS_PATH, BUMP_PATH, CAUSTIC_PATH);
    blockbench_renderer br = create_blockbench_renderer(camera, ATLAS_PATH, BUMP_PATH);

    skybox sky = create_skybox(camera);
    sun s = create_sun(camera, 1.0f, 1.0f, 1.0f);
    FBO shadow_map = create_shadow_map(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
    FBO reflection_map = create_reflection_map(WIDTH, HEIGHT);

    renderer r = {
        .wr = wr,
        .lr = lr,
        .fr = fr,
        .br = br,
        .sky = sky,
        .s = s,
        .cam_cache = {
            .x = camera->position[0],
            .z = camera->position[2],
        },
        .cam = camera,
        .shadow_map = shadow_map,
        .reflection_map = reflection_map,
    };

    return r;
}

void destroy_renderer(renderer* r) {
    destroy_block_renderer(r->wr);
    destroy_block_renderer(r->lr);
    destroy_blockbench_renderer(r->br);
    skybox_cleanup(&(r->sky));
}

void render(game_data* args, renderer* r, world_mesh* packet, int num_packets) {
    assert(r != NULL && "Renderer is NULL\n");
    assert(packet != NULL && "World mesh is NULL\n");

    if (num_packets <= 0) {
        return; // No packets to render
    }

    render_shadow_map(&(r->shadow_map), &(r->s), packet);
    render_reflection_map(&(r->reflection_map), r->cam, (float)WORLDGEN_WATER_LEVEL, packet);
    
    glActiveTexture(GL_TEXTURE0 + SHADOW_MAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, r->shadow_map.texture);
    
    glActiveTexture(GL_TEXTURE0 + REFLECTION_MAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, r->reflection_map.texture);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    render_skybox(&(r->sky));
    glEnable(GL_DEPTH_TEST);

    render_sun(&(r->s), args->tick);

    glClear(GL_DEPTH_BUFFER_BIT);
    
    render_solids(&(r->wr), &(r->s), &(r->shadow_map), packet);
    render_liquids(&(r->lr), &(r->s), &(r->shadow_map), &(r->reflection_map), packet);
    render_foliage(&(r->fr), &(r->s), &(r->shadow_map), packet);
    render_transparent(&(r->wr), &(r->s), &(r->shadow_map), packet);
    render_blockbench_models(&(r->br), &(r->s), &(r->shadow_map), packet);
}
