#include <stdio.h>
#include <window.h>
#include <render.h>
#include <camera.h>
#include <input.h>
#include <settings.h>
#include <player_instance.h>

int main() {
    GLFWwindow* window = create_window("malloc-craft", WIDTH, HEIGHT);

    if (!window || !load_gl()) {
        return -1;
    }

    // camera cam = {
    //     .position = {1.0f, (float)WORLDGEN_BASE_TERRAIN_HEIGHT + 20.0f, 0.0f},
    //     .up = {0.0f, 1.0f, 0.0f},
    //     .front = {1.0f, 0.0f, 0.0f},
    //     .yaw = 0.0f,
    //     .pitch = 0.0f
    // };

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

    shader_program program;
    r_init(&program, &(player.cam));

    i_init(window, &player);

    while (!glfwWindowShouldClose(window)) {
        update_camera();

        // pre optimization ~ 9-10 ms
        render(player.cam, program);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    r_cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}