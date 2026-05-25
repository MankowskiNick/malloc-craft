#include "metrics.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static float frame_time_buffer[FPS_BUFFER_SIZE];
static int frame_buffer_index = 0;
static int average_fps = -1;

static long long start_ms = -1;
static double last_tick_s = -1.0;
static double tick_s = -1.0;

static bool profile_enabled = false;
static bool profile_startup_enabled = true;
static bool profile_show_stats[PROFILE_STAT_COUNT] = { true, true, true };
static bool profile_section_enabled[PROFILE_SECTION_COUNT] = {
    true, true, true, true, true, true, true, true,
    true, true, true, true, true, true, true, true
};
static bool profile_log_to_file = false;
static char profile_log_path[256] = {0};
static FILE* profile_log_file = NULL;
static long long startup_origin_ms = 0;
static long long startup_checkpoint_ms = 0;
static long long frame_start_ms = 0;
static long long section_start_ms[PROFILE_SECTION_COUNT];
static double section_total_ms[PROFILE_SECTION_COUNT];
static double section_min_ms[PROFILE_SECTION_COUNT];
static double section_max_ms[PROFILE_SECTION_COUNT];
static double frame_elapsed_total_ms = 0.0;
static double frame_elapsed_min_ms = 0.0;
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

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static void close_profile_log_file(void) {
    if (profile_log_file != NULL) {
        fclose(profile_log_file);
        profile_log_file = NULL;
    }
}

static FILE* ensure_profile_log_file(void) {
    if (!profile_log_to_file || profile_log_path[0] == '\0') {
        return NULL;
    }
    if (profile_log_file == NULL) {
        profile_log_file = fopen(profile_log_path, "a");
        if (profile_log_file == NULL) {
            fprintf(stderr, "[profile] failed to open log file '%s'\n", profile_log_path);
            profile_log_to_file = false;
        }
    }
    return profile_log_file;
}

static void profile_write_line(const char* fmt, ...) {
    char buffer[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    fputs(buffer, stdout);

    FILE* log_file = ensure_profile_log_file();
    if (log_file != NULL) {
        fputs(buffer, log_file);
        fflush(log_file);
    }
}

static bool section_is_enabled(profile_section section) {
    return section >= 0
        && section < PROFILE_SECTION_COUNT
        && profile_section_enabled[section];
}

static void print_profile_group_header(const char* label, int frame_count) {
    profile_write_line("[profile] %s over %d frames\n", label, frame_count);
}

static void print_profile_stat_group(const char* label, const double* values, double frame_total_value) {
    profile_write_line("[profile]   %s\n", label);
    for (int i = 0; i < PROFILE_SECTION_COUNT; i++) {
        if (!profile_section_enabled[i]) {
            continue;
        }
        profile_write_line("[profile]     %-24s %8.2f ms\n", profile_section_names[i], values[i]);
    }
    profile_write_line("[profile]     %-24s %8.2f ms\n", "frame_total", frame_total_value);
}

static void ensure_metrics_initialized(void) {
    if (start_ms >= 0) {
        return;
    }

    start_ms = now_ms();
    last_tick_s = now_seconds();
    tick_s = last_tick_s;

    for (int i = 0; i < FPS_BUFFER_SIZE; i++) {
        frame_time_buffer[i] = 0.0f;
    }
}

static void reset_profile_aggregates(void) {
    for (int i = 0; i < PROFILE_SECTION_COUNT; i++) {
        section_start_ms[i] = 0;
        section_total_ms[i] = 0.0;
        section_min_ms[i] = 0.0;
        section_max_ms[i] = 0.0;
    }
    frame_elapsed_total_ms = 0.0;
    frame_elapsed_min_ms = 0.0;
    frame_elapsed_max_ms = 0.0;
    profiled_frame_count = 0;
}

void init_metrics(void) {
    start_ms = -1;
    last_tick_s = -1.0;
    tick_s = -1.0;
    ensure_metrics_initialized();
}

float get_fps(void) {
    return average_fps;
}

int get_delta_ms() {
    ensure_metrics_initialized();
    return (int)(get_delta_seconds() * 1000.0f);
}

float get_delta_seconds(void) {
    ensure_metrics_initialized();
    return (float)(tick_s - last_tick_s);
}

void update_fps(void) {
    ensure_metrics_initialized();

    last_tick_s = tick_s;
    tick_s = now_seconds();

    float delta_seconds = (float)(tick_s - last_tick_s);

    // Calculate rolling average FPS
    if (delta_seconds > 0.0f) {
        float delta_ms = delta_seconds * 1000.0f;

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
}

void enable_client_profiling(bool enabled) {
    profile_enabled = enabled;
    if (enabled) {
        reset_profile_aggregates();
    } else {
        close_profile_log_file();
    }
}

bool client_profiling_enabled(void) {
    return profile_enabled;
}

void profile_configure_startup(bool enabled) {
    profile_startup_enabled = enabled;
}

void profile_configure_output(bool show_average, bool show_min, bool show_max) {
    profile_show_stats[PROFILE_STAT_AVERAGE] = show_average;
    profile_show_stats[PROFILE_STAT_MIN] = show_min;
    profile_show_stats[PROFILE_STAT_MAX] = show_max;
}

void profile_configure_log_file(bool enabled, const char* filename) {
    profile_log_to_file = enabled;
    if (filename != NULL) {
        snprintf(profile_log_path, sizeof(profile_log_path), "%s", filename);
    } else {
        profile_log_path[0] = '\0';
    }
    close_profile_log_file();
}

void profile_set_all_sections_enabled(bool enabled) {
    for (int i = 0; i < PROFILE_SECTION_COUNT; i++) {
        profile_section_enabled[i] = enabled;
    }
}

void profile_set_section_enabled(profile_section section, bool enabled) {
    if (section < 0 || section >= PROFILE_SECTION_COUNT) {
        return;
    }
    profile_section_enabled[section] = enabled;
}

bool profile_section_from_name(const char* name, profile_section* section) {
    if (name == NULL || section == NULL) {
        return false;
    }

    for (int i = 0; i < PROFILE_SECTION_COUNT; i++) {
        if (strcmp(name, profile_section_names[i]) == 0) {
            *section = (profile_section)i;
            return true;
        }
    }

    return false;
}

void profile_startup_reset(void) {
    if (!profile_enabled || !profile_startup_enabled) {
        return;
    }

    startup_origin_ms = now_ms();
    startup_checkpoint_ms = startup_origin_ms;
    profile_write_line("[profile] startup profiling enabled\n");
}

void profile_startup_checkpoint(const char* label) {
    if (!profile_enabled || !profile_startup_enabled) {
        return;
    }

    long long current_ms = now_ms();
    long long phase_ms = current_ms - startup_checkpoint_ms;
    long long total_ms = current_ms - startup_origin_ms;
    profile_write_line("[profile] startup %-24s phase=%4lld ms total=%4lld ms\n",
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
    if (frame_elapsed_min_ms == 0.0 || frame_elapsed_ms < frame_elapsed_min_ms) {
        frame_elapsed_min_ms = frame_elapsed_ms;
    }
    if (frame_elapsed_ms > frame_elapsed_max_ms) {
        frame_elapsed_max_ms = frame_elapsed_ms;
    }

    profiled_frame_count++;

    if (profiled_frame_count >= FPS_BUFFER_SIZE) {
        double avg_values[PROFILE_SECTION_COUNT];
        double min_values[PROFILE_SECTION_COUNT];
        double max_values[PROFILE_SECTION_COUNT];

        for (int i = 0; i < PROFILE_SECTION_COUNT; i++) {
            avg_values[i] = section_total_ms[i] / (double)profiled_frame_count;
            min_values[i] = section_min_ms[i];
            max_values[i] = section_max_ms[i];
        }

        print_profile_group_header("frame summary", profiled_frame_count);
        if (profile_show_stats[PROFILE_STAT_AVERAGE]) {
            print_profile_stat_group("average", avg_values, frame_elapsed_total_ms / (double)profiled_frame_count);
        }
        if (profile_show_stats[PROFILE_STAT_MIN]) {
            print_profile_stat_group("minimum", min_values, frame_elapsed_min_ms);
        }
        if (profile_show_stats[PROFILE_STAT_MAX]) {
            print_profile_stat_group("maximum", max_values, frame_elapsed_max_ms);
        }

        reset_profile_aggregates();
    }

    frame_start_ms = 0;
}

void profile_begin_section(profile_section section) {
    if (!profile_enabled || !section_is_enabled(section)) {
        return;
    }

    section_start_ms[section] = now_ms();
}

void profile_end_section(profile_section section) {
    if (!profile_enabled || !section_is_enabled(section)) {
        return;
    }

    long long start = section_start_ms[section];
    if (start == 0) {
        return;
    }

    double elapsed_ms = (double)(now_ms() - start);
    section_total_ms[section] += elapsed_ms;
    if (section_min_ms[section] == 0.0 || elapsed_ms < section_min_ms[section]) {
        section_min_ms[section] = elapsed_ms;
    }
    if (elapsed_ms > section_max_ms[section]) {
        section_max_ms[section] = elapsed_ms;
    }
    section_start_ms[section] = 0;
}
