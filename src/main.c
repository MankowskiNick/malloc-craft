#include <stdio.h>
#include <window.h>
#include <render.h>
#include <camera.h>
#include <input.h>
#include <settings.h>
#include <player_instance.h>
#include <skybox.h>
#include <chunk_mesh.h>
#include <mesh.h>
#include <world.h>
#include <pthread.h>

int main() {
    GLFWwindow* window = create_window(TITLE, WIDTH, HEIGHT);

    if (!window || !load_gl()) {
        return -1;
    }

    player_instance player = {
        .selected_block = 1,
        .cam = {
            .position = {1.0f, (float)WORLDGEN_BASE_TERRAIN_HEIGHT + 20.0f, 0.0f},
            .up = {0.0f, 1.0f, 0.0f},
            .front = {1.0f, 0.0f, 0.0f},
            .yaw = 0.0f,
            .pitch = 0.0f
        }
    };

    game_data data = {
        .x = (int)player.cam.position[0],
        .z = (int)player.cam.position[2],
        .player = &player,
        .is_running = TRUE
    };
    
    renderer r = create_renderer(&(player.cam));

    wm_init(&(player.cam));

    i_init(window, &data);

    int num_packets = 0;

    get_chunk_meshes(&data);
    get_world_mesh(&data);

    start_chunk_mesh_updater(&data);
    start_world_mesh_updater(&data);

    while (!glfwWindowShouldClose(window)) {

        data.x = player.cam.position[0];
        data.z = player.cam.position[2];

        update_camera();

        lock_mesh();
        render(&r, data.world_mesh, *(data.num_packets));
        unlock_mesh();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroy_renderer(&r);
    i_cleanup();
    m_cleanup();
    w_cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
