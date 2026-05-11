#include "render/core/window.h"
#include "render/core/render.h"
#include "player/input/input.h"
#include "mesh/core/mesh.h"
#include "world/core/block.h"
#include "server/server.h"
#include "server/threads/client_recv.h"
#include "util/metrics.h"
#include "util/core.h"

int main(int argc, char** argv) {
    
    char env[16] = {0};
    bool server_mode = false;
    bool profile_client = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0) {
            server_mode = true;
        }
        if (strcmp(argv[i], "--profile-client") == 0) {
            profile_client = true;
        }
        if (strcmp(argv[i], "--localhost") == 0) {
            start_local_server();
        }
        if (strcmp(argv[i], "--env") == 0) {
            if (i + 1 >= argc) {
                printf("ERROR: Must provide environment when specifying \"--env\"");
                return -1;
            }
            strcpy(env, argv[i + 1]);
        }
    }

    enable_client_profiling(profile_client);
    profile_startup_reset();

    init_core(env);
    profile_startup_checkpoint("init_core");
    if (server_mode) {
        server_main();
        return 0;
    }
    
    start_broadcast_listener();
    profile_startup_checkpoint("broadcast_listener");

    GLFWwindow* window = create_window(TITLE, WIDTH, HEIGHT);
    profile_startup_checkpoint("create_window");

    game_data data = {
        .x = 0,
        .z = 0,
        .player = create_player(PLAYER_FILE),
        .is_running = true,
        .show_fps = false,
        .num_packets = NULL,
        .packet = NULL,
        .world_mesh = create_double_buffer(),
        .mesh_requires_update = true,
    };

    init_world_mesh(&data.player.cam);
    profile_startup_checkpoint("init_world_mesh");
    renderer r = create_renderer(&data);
    profile_startup_checkpoint("create_renderer");
    
    start_chunk_mesh_updater(&data);
    profile_startup_checkpoint("chunk_mesh_updater");
    start_world_mesh_updater(&data);
    profile_startup_checkpoint("world_mesh_updater");
    
    init_input(window, &data);
    profile_startup_checkpoint("init_input");
    init_metrics();

    while (!glfwWindowShouldClose(window)) {

        if (data.world_mesh == NULL || data.num_packets == NULL || *data.num_packets == 0)
            continue;

        profile_frame_begin();

        profile_begin_section(PROFILE_SECTION_UPDATE_GAME_DATA);
        update_game_data(&data);
        profile_end_section(PROFILE_SECTION_UPDATE_GAME_DATA);

        profile_begin_section(PROFILE_SECTION_UPDATE_FPS);
        update_fps();
        profile_end_section(PROFILE_SECTION_UPDATE_FPS);

        profile_begin_section(PROFILE_SECTION_UPDATE_CAMERA);
        update_camera(get_delta_ms());
        profile_end_section(PROFILE_SECTION_UPDATE_CAMERA);

        profile_begin_section(PROFILE_SECTION_APPLY_PHYSICS);
        apply_physics(&data.player, get_delta_ms());
        profile_end_section(PROFILE_SECTION_APPLY_PHYSICS);

        profile_begin_section(PROFILE_SECTION_UPDATE_SELECTED_BLOCK);
        update_selected_block(&data.player);
        profile_end_section(PROFILE_SECTION_UPDATE_SELECTED_BLOCK);

        profile_begin_section(PROFILE_SECTION_COPY_WORLD_MESH);
        world_mesh* render_mesh = copy_world_mesh(data.world_mesh);
        int render_packet_count = *(data.num_packets);
        profile_end_section(PROFILE_SECTION_COPY_WORLD_MESH);

        profile_begin_section(PROFILE_SECTION_RENDER_TOTAL);
        render(&data, &r, render_mesh, render_packet_count);
        profile_end_section(PROFILE_SECTION_RENDER_TOTAL);
        free_world_mesh(render_mesh);

        profile_begin_section(PROFILE_SECTION_SWAP_BUFFERS);
        glfwSwapBuffers(window);
        profile_end_section(PROFILE_SECTION_SWAP_BUFFERS);

        profile_begin_section(PROFILE_SECTION_POLL_EVENTS);
        glfwPollEvents();
        profile_end_section(PROFILE_SECTION_POLL_EVENTS);

        profile_frame_end();
    }

    kill_chunk_mesh_updater();
    kill_world_mesh_updater();

    destroy_renderer(&r);
    destroy_game_data(data);

    glfwDestroyWindow(window);
    glfwTerminate();

    mesh_cleanup();
    input_cleanup();

    core_cleanup();

    return 0;
}
