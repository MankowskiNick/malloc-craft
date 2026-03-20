#include "render/core/window.h"
#include "render/core/render.h"
#include "player/input/input.h"
#include "mesh/core/mesh.h"
#include "world/core/world.h"
#include "world/core/block.h"
#include "server/server.h"
#include "server/threads/client_recv.h"
#include "util/metrics.h"
#include "util/core.h"

int main(int argc, char** argv) {
    

    init_core();
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0) {
            server_main();
            return 0;
        }
        if (strcmp(argv[i], "--localhost") == 0) {
            start_local_server();
        }
    }
    
    start_broadcast_listener();

    GLFWwindow* window = create_window(TITLE, WIDTH, HEIGHT);

    game_data data = {
        .x = 0,
        .z = 0,
        .player = create_player(PLAYER_FILE),
        .is_running = true,
        .show_fps = false,
        .num_packets = NULL,
        .packet = NULL,
        .world_mesh = NULL,
        .mesh_requires_update = true,
    };

    init_world_mesh(&data.player.cam);
    renderer r = create_renderer(&data);
    
    start_chunk_mesh_updater(&data);
    start_world_mesh_updater(&data);
    
    init_input(window, &data);

    while (!glfwWindowShouldClose(window)) {

        if (data.world_mesh == NULL || *data.num_packets == 0)
            continue;

        update_game_data(&data);
        update_fps();

        update_camera(get_delta_ms());

        apply_physics(&data.player, get_delta_ms());
        update_selected_block(&data.player);

        lock_mesh();
            world_mesh* render_mesh = copy_world_mesh(data.world_mesh);
            int render_packet_count = *(data.num_packets);
        unlock_mesh();

        render(&data, &r, render_mesh, render_packet_count);
        free_world_mesh(render_mesh);

        glfwSwapBuffers(window);
        glfwPollEvents();
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
