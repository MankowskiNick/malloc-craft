#include <stdio.h>
#include <window.h>
#include <render.h>
#include <camera.h>
#include <input.h>
#include <settings.h>
#include <player_instance.h>
#include <skybox.h>

int main() {
    GLFWwindow* window = create_window("malloc-craft", WIDTH, HEIGHT);

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

    skybox* s = create_skybox();

    shader_program program;
    r_init(&program, &(player.cam));

    i_init(window, &player);

    while (!glfwWindowShouldClose(window)) {
        update_camera();

        draw_skybox(s, &(player.cam));
        render(player.cam, program);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    r_cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}