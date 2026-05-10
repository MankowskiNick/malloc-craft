#include "metrics.h"

#include <stdio.h>
#include <time.h>

static float frame_time_buffer[FPS_BUFFER_SIZE];
static int frame_buffer_index = 0;
static int average_fps = -1;

static long long start_ms = -1;
static long long last_tick = -1;
static long long tick = -1;

static bool profile_enabled = false;
static long long startup_origin_ms = 0;
static long long startup_checkpoint_ms = 0;
static long long frame_start_ms = 0;
static long long section_start_ms[PROFILE_SECTION_COUNT];
static double section_total_ms[PROFILE_SECTION_COUNT];
static double section_max_ms[PROFILE_SECTION_COUNT];
static double frame_elapsed_total_ms = 0.0;
static double frame_elapsed_max_ms = 0.0;
static int profiled_frame_count = 0;

static const char* profile_section_names[PROFILE_SECTION_COUNT] = {
    "update_game_data",
    "update_fps",
    "update_camera",
    "apply_physics",
    "update_selected_block",
    "copy_world_mesh",
    "render_total",
    "render_clear",
    "render_skybox",
    "render_sun",
    "render_shadow_map",
    "render_reflection_map",
    "render_world",
    "render_ui",
    "swap_buffers",
    "poll_events"
};

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + (long long)(ts.tv_nsec / 1000000LL);
}

static void ensure_metrics_initialized(void) {
    if (start_ms >= 0) {
        return;
    }

    start_ms = now_ms();
    last_tick = 0;
    tick = 0;

    for (int i = 0; i < FPS_BUFFER_SIZE; i++) {
        frame_time_buffer[i] = 0.0f;
    }
}

static void reset_profile_aggregates(void) {
    for (int i = 0; i < PROFILE_SECTION_COUNT; i++) {
        section_start_ms[i] = 0;
        section_total_ms[i] = 0.0;
        section_max_ms[i] = 0.0;
    }
    frame_elapsed_total_ms = 0.0;
    frame_elapsed_max_ms = 0.0;
    profiled_frame_count = 0;
}

void init_metrics(void) {
    start_ms = -1;
    last_tick = -1;
    tick = -1;
    ensure_metrics_initialized();
}

float get_fps(void) {
    return average_fps;
}

int get_delta_ms() {
    ensure_metrics_initialized();
    return (int)(tick - last_tick);
}

void update_fps(void) {
    ensure_metrics_initialized();

    // Calculate rolling average FPS
    if (tick - last_tick > 0) {
        int delta_ms = (int)(tick - last_tick);

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
    tick = now_ms() - start_ms;
}

void enable_client_profiling(bool enabled) {
    profile_enabled = enabled;
    if (enabled) {
        reset_profile_aggregates();
    }
}

bool client_profiling_enabled(void) {
    return profile_enabled;
}

void profile_startup_reset(void) {
    if (!profile_enabled) {
        return;
    }

    startup_origin_ms = now_ms();
    startup_checkpoint_ms = startup_origin_ms;
    printf("[profile] client startup profiling enabled\n");
}

void profile_startup_checkpoint(const char* label) {
    if (!profile_enabled) {
        return;
    }

    long long current_ms = now_ms();
    long long phase_ms = current_ms - startup_checkpoint_ms;
    long long total_ms = current_ms - startup_origin_ms;
    printf("[profile] startup %-24s phase=%4lld ms total=%4lld ms\n",
           label, phase_ms, total_ms);
    startup_checkpoint_ms = current_ms;
}

void profile_frame_begin(void) {
    if (!profile_enabled) {
        return;
    }

    frame_start_ms = now_ms();
}

void profile_frame_end(void) {
    if (!profile_enabled || frame_start_ms == 0) {
        return;
    }

    double frame_elapsed_ms = (double)(now_ms() - frame_start_ms);
    frame_elapsed_total_ms += frame_elapsed_ms;
    if (frame_elapsed_ms > frame_elapsed_max_ms) {
        frame_elapsed_max_ms = frame_elapsed_ms;
    }

    profiled_frame_count++;

    if (profiled_frame_count >= FPS_BUFFER_SIZE) {
        printf("[profile] frame avg over %d frames:", profiled_frame_count);
        for (int i = 0; i < PROFILE_SECTION_COUNT; i++) {
            double avg_ms = section_total_ms[i] / (double)profiled_frame_count;
            printf(" %s=%.2fms", profile_section_names[i], avg_ms);
        }
        printf(" frame_total=%.2fms\n", frame_elapsed_total_ms / (double)profiled_frame_count);

        printf("[profile] frame max over %d frames:", profiled_frame_count);
        for (int i = 0; i < PROFILE_SECTION_COUNT; i++) {
            printf(" %s=%.2fms", profile_section_names[i], section_max_ms[i]);
        }
        printf(" frame_total=%.2fms\n", frame_elapsed_max_ms);

        reset_profile_aggregates();
    }

    frame_start_ms = 0;
}

void profile_begin_section(profile_section section) {
    if (!profile_enabled || section < 0 || section >= PROFILE_SECTION_COUNT) {
        return;
    }

    section_start_ms[section] = now_ms();
}

void profile_end_section(profile_section section) {
    if (!profile_enabled || section < 0 || section >= PROFILE_SECTION_COUNT) {
        return;
    }

    long long start = section_start_ms[section];
    if (start == 0) {
        return;
    }

    double elapsed_ms = (double)(now_ms() - start);
    section_total_ms[section] += elapsed_ms;
    if (elapsed_ms > section_max_ms[section]) {
        section_max_ms[section] = elapsed_ms;
    }
    section_start_ms[section] = 0;
}
