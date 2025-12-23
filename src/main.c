#include <stdio.h>
#include <window.h>
#include <render.h>
#include <block.h>
#include <camera.h>
#include <input.h>
#include <settings.h>
#include <player_instance.h>
#include <skybox.h>
#include <chunk_mesh.h>
#include <mesh.h>
#include <world.h>
#include <biome.h>
#include <pthread.h>
#include <time.h>
#include <game_data.h>

int main() {
    read_settings("res/settings.json");
    read_biomes("res/biomes.json");

    GLFWwindow* window = create_window(TITLE, WIDTH, HEIGHT);
    if (!window || !load_gl()) {
        return -1;
    }

    block_init();

    player_instance player = player_init(PLAYER_FILE);

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

    long start = (long)(glfwGetTime() * 1000.0);
    int last_tick = 0;

    while (!glfwWindowShouldClose(window)) {

        data.x = player.cam.position[0];
        data.z = player.cam.position[2];
        data.tick = (int)(glfwGetTime() * 1000.0) - start; // 1 tick = 1 ms
        int delta_ms = data.tick - last_tick;
        last_tick = data.tick;

        update_camera(delta_ms);

        lock_mesh();
        render(&data, &r, data.world_mesh, *(data.num_packets));
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
