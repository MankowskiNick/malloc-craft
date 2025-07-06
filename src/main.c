#include <stdio.h>
#include <window.h>
#include <render.h>
#include <camera.h>
#include <input.h>
#include <settings.h>
#include <player_instance.h>
#include <skybox.h>
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

    chunk_args args = {
        .x = (int)player.cam.position[0],
        .z = (int)player.cam.position[2],
        .num_packets = &num_packets
    };

    chunk_mesh_init(&(player.cam));
    chunk_mesh** packet = get_chunk_meshes(&args);

    args.packet = packet;

    world_mesh* cur_mesh = get_world_mesh(&args);

    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    while (!glfwWindowShouldClose(window)) {

        update_camera();

        pthread_t mesh_thread;
        void* mesh_result;

        chunk_args args = {
            .x = (int)player.cam.position[0],
            .z = (int)player.cam.position[2],
            .num_packets = &num_packets
        };
        args.packet = get_chunk_meshes(&args);

        pthread_create(&mesh_thread, NULL, (void* (*)(void*))get_world_mesh, &args);


        render_args r_args = {
            .r = &r,
            .packet = cur_mesh,
            .num_packets = num_packets
        };


        render(&r_args);

        pthread_join(mesh_thread, &mesh_result);
        cur_mesh = (world_mesh*)mesh_result;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroy_renderer(&r);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
