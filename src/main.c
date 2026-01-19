#include <stdio.h>
#include "render/core/window.h"
#include "render.h"
#include "world/core/block.h"
#include <player/core/camera.h>
#include <player/input/input.h>
#include "util/settings.h"
#include <player/core/player.h>
#include "skybox.h"
#include <chunk_mesh.h>
#include <mesh.h>
#include "world/core/world.h"
#include "world/generation/biome.h"
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

    player player = player_init(PLAYER_FILE);

    game_data data = {
        .x = (int)player.cam.position[0],
        .z = (int)player.cam.position[2],
        .player = &player,
        .is_running = TRUE,
        .show_fps = false,
        .fps = 0,
        .fps_average_frames = FPS_AVERAGE_FRAMES,
        .frame_buffer_index = 0,
        .average_fps = 0
    };
    
    // Initialize frame time buffer for rolling average
    data.frame_time_buffer = (float*)malloc(sizeof(float) * data.fps_average_frames);
    for (int i = 0; i < data.fps_average_frames; i++) {
        data.frame_time_buffer[i] = 16.67f; // Default ~60 FPS
    }
    
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

        // Calculate rolling average FPS
        if (delta_ms > 0) {
            // Add current frame time to circular buffer
            data.frame_time_buffer[data.frame_buffer_index] = (float)delta_ms;
            data.frame_buffer_index = (data.frame_buffer_index + 1) % data.fps_average_frames;
            
            // Calculate average frame time
            float avg_delta_ms = 0.0f;
            for (int i = 0; i < data.fps_average_frames; i++) {
                avg_delta_ms += data.frame_time_buffer[i];
            }
            avg_delta_ms /= (float)data.fps_average_frames;
            
            // Convert to FPS
            data.average_fps = (int)(1000.0f / avg_delta_ms);
            data.fps = 1000 / delta_ms; // Keep for backwards compatibility
        }

        update_camera(delta_ms);
        apply_physics(&player, delta_ms);
        update_selected_block(&player);

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
    
    free_game_data(data);
    // // Clean up frame time buffer
    // free(data.frame_time_buffer);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
