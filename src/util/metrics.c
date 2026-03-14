#include "metrics.h"

#include "settings.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static float frame_time_buffer[FPS_BUFFER_SIZE];
static int frame_buffer_index = 0;
static int average_fps = -1;

static int start_ms = -1;
static int last_tick = -1;
static int tick = -1;

void init_metrics(void) {
    start_ms = (long)(glfwGetTime() * 1000.0);
    

    for (int i = 0; i < FPS_BUFFER_SIZE; i++) {
        frame_time_buffer[i] = 0.0f;
    }
}

float get_fps(void) {
    return average_fps;
}

int get_delta_ms() {
    return tick - last_tick;
}

void update_fps(void) {
    // Calculate rolling average FPS
    if (tick - last_tick > 0) {
        int delta_ms = tick - last_tick;

        // Add current frame time to circular buffer
        frame_time_buffer[frame_buffer_index] = delta_ms;
        frame_buffer_index = (frame_buffer_index + 1) % FPS_BUFFER_SIZE;
        
        // Calculate average frame time
        float avg_delta_ms = 0.0f;
        for (int i = 0; i < FPS_BUFFER_SIZE; i++) {
            avg_delta_ms += frame_time_buffer[i];
        }
        avg_delta_ms /= (float)FPS_BUFFER_SIZE;
        
        // Convert to FPS
        average_fps = (int)(1000.0f / avg_delta_ms);
    }

    last_tick = tick;
    tick = (int)(glfwGetTime() * 1000) - start_ms;
}