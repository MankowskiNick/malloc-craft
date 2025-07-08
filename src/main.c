#include <stdio.h>
#include <window.h>
#include <render.h>
#include <camera.h>
#include <input.h>
#include <settings.h>
#include <player_instance.h>
#include <skybox.h>
#include <chunk_mesh.h>
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
    
    renderer r = create_renderer(&(player.cam));

    wm_init(&(player.cam));

    i_init(window, &player);

    int num_packets = 0;
    mesh_args args = {
        .x = (int)player.cam.position[0],
        .z = (int)player.cam.position[2]
    };

    get_chunk_meshes(&args);
    get_world_mesh(&args);

    start_chunk_mesh_updater(&args);
    start_world_mesh_updater(&args);


    while (!glfwWindowShouldClose(window)) {

        args.x = player.cam.position[0];
        args.z = player.cam.position[2];

        update_camera();

        lock_chunk_mesh();
        lock_world_mesh();
        render_args r_args = {
            .r = &r,
            .packet = args.world_mesh,
            .num_packets = *(args.num_packets)
        };
        unlock_chunk_mesh();

        render(&r_args);
        unlock_world_mesh();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroy_renderer(&r);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
