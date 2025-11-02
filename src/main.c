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

    // For invisible window on Linux, keep it visible but minimized/iconified
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Show window to capture input, then iconify to keep it out of the way
    glfwShowWindow(window);
    glfwFocusWindow(window);
    glfwPollEvents();
    // Use iconify instead of hide - still captures input on Linux
    glfwIconifyWindow(window);

    while (!glfwWindowShouldClose(window)) {

        data.x = player.cam.position[0];
        data.z = player.cam.position[2];

        update_camera();

        lock_mesh();
        render(&r, data.world_mesh, *(data.num_packets));
        char* terminal_output = buffer_screen_to_char(&r);
        if (terminal_output) {
            // reset cursor to top-left
            printf("\033[H");
            printf("%s", terminal_output);
            free(terminal_output);
        }
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
