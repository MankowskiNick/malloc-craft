#include "render.h"

#include "block_renderer.h"
#include "blockbench_renderer.h"
#include "liquid_renderer.h"
#include "foliage_renderer.h"
#include "skybox.h"
#include <glad/glad.h>
#include <chunk_mesh.h>
#include <mesh.h>
#include "util/sort.h"
#include "../../world/core/world.h"
#include <util.h>
#include "util/settings.h"
#include <world_mesh.h>
#include "fbo.h"
#include "shadow_map.h"
#include "reflection_map.h"
#include <assert.h>

renderer create_renderer(game_data* data) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    player* player = data->player;
    camera* camera = &(player->cam);

    if (WIREFRAME) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    w_init();
    m_init(&(player->cam));

    // Preload initial chunks around the player
    preload_initial_chunks(player->position[0], player->position[2]);

    block_renderer wr = create_block_renderer(camera, ATLAS_PATH, BUMP_PATH, CAUSTIC_PATH);
    block_renderer lr = create_liquid_renderer(camera, ATLAS_PATH, BUMP_PATH, CAUSTIC_PATH);
    block_renderer fr = create_foliage_renderer(camera, ATLAS_PATH, BUMP_PATH, CAUSTIC_PATH);
    blockbench_renderer br = create_blockbench_renderer(camera, ATLAS_PATH, BUMP_PATH);
    outline_renderer or = create_outline_renderer(camera);
    ui_renderer ui = create_ui_renderer();

    skybox sky = create_skybox(camera);
    sun s = create_sun(camera, 1.0f, 1.0f, 1.0f);
    FBO shadow_map = create_shadow_map(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
    FBO reflection_map = create_reflection_map(WIDTH, HEIGHT);

    renderer r = {
        .wr = wr,
        .lr = lr,
        .fr = fr,
        .br = br,
        .or = or,
        .ui = ui,
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
    destroy_outline_renderer(r->or);
    destroy_ui_renderer(&r->ui);
    skybox_cleanup(&(r->sky));
}

void render(game_data* args, renderer* r, world_mesh* packet, int num_packets) {
    assert(r != NULL && "Renderer is NULL\n");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    render_skybox(&(r->sky));
    glEnable(GL_DEPTH_TEST);

    render_sun(&(r->s), args->tick);

    if (packet != NULL && num_packets > 0) {
        render_shadow_map(&(r->shadow_map), &(r->s), packet);
        render_reflection_map(&(r->reflection_map), r->cam, (float)WORLDGEN_WATER_LEVEL, packet);

        glActiveTexture(GL_TEXTURE0 + SHADOW_MAP_TEXTURE_INDEX);
        glBindTexture(GL_TEXTURE_2D, r->shadow_map.texture);

        glActiveTexture(GL_TEXTURE0 + REFLECTION_MAP_TEXTURE_INDEX);
        glBindTexture(GL_TEXTURE_2D, r->reflection_map.texture);

        glClear(GL_DEPTH_BUFFER_BIT);

        render_solids(&(r->wr), &(r->s), &(r->shadow_map), packet);
        render_blockbench_models(&(r->br), &(r->s), &(r->shadow_map), packet);
        if (args->player->is_underwater) {
            render_foliage(&(r->fr), &(r->s), &(r->shadow_map), packet);
            render_transparent(&(r->wr), &(r->s), &(r->shadow_map), packet);
            render_liquids(&(r->lr), &(r->s), &(r->shadow_map), &(r->reflection_map), packet);
        } else {
            render_liquids(&(r->lr), &(r->s), &(r->shadow_map), &(r->reflection_map), packet);
            render_foliage(&(r->fr), &(r->s), &(r->shadow_map), packet);
            render_transparent(&(r->wr), &(r->s), &(r->shadow_map), packet);
        }

        if (args->player->has_selected_block) {
            render_outline(&(r->or), args->player->selected_block_pos[0], args->player->selected_block_pos[1], args->player->selected_block_pos[2]);
        }
    }

    glDisable(GL_DEPTH_TEST);
    render_hotbar(&(r->ui), args->player->hotbar, args->player->hotbar_size, args->player->selected_block);
    if (args->show_fps) {
        render_fps(&(r->ui), args->average_fps);
    }
    glEnable(GL_DEPTH_TEST);
}
