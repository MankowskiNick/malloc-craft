#include <render.h>

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

renderer create_renderer(camera* camera) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    #ifdef WIREFRAME
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    #endif

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
    skybox sky = create_skybox(camera);
    sun s = create_sun(camera, 1.0f, 1.0f, 1.0f);
    shadow_map map = create_shadow_map(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

    renderer r = {
        .wr = wr,
        .lr = lr,
        .sky = sky,
        .s = s,
        .cam_cache = {
            .x = camera->position[0],
            .z = camera->position[2],
        },
        .cam = camera,
        .map = map,
    };

    return r;
}

void destroy_renderer(renderer* r) {
    m_cleanup();
    destroy_block_renderer(r->wr);
    destroy_block_renderer(r->lr);
    skybox_cleanup(&(r->sky));
    // sun_cleanup(&(r->s));
}

void render(render_args* args) {
    assert(args != NULL && "Render args are NULL\n");

    renderer* r = args->r;
    world_mesh* packet = args->packet;
    int num_packets = args->num_packets;

    if (r == NULL || packet == NULL) {
        assert(false && "Renderer or world mesh is NULL\n");
    }

    if (num_packets <= 0) {
        return; // No packets to render
    }


    shadow_map_render(&(r->map), &(r->s), packet);
    glActiveTexture(GL_TEXTURE0 + SHADOW_MAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, r->map.texture);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    render_skybox(&(r->sky));
    glEnable(GL_DEPTH_TEST);

    render_sun(&(r->s));

    glClear(GL_DEPTH_BUFFER_BIT);
    
    render_solids(&(r->wr), &(r->s), &(r->map), packet);

    render_liquids(&(r->lr), &(r->s), &(r->map), packet);

    render_transparent(&(r->wr), &(r->s), &(r->map), packet);
}
