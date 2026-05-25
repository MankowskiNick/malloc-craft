#ifndef METRICS_H
#define METRICS_H

#include <stdbool.h>

#define FPS_BUFFER_SIZE 60

typedef enum {
    PROFILE_SECTION_UPDATE_GAME_DATA = 0,
    PROFILE_SECTION_UPDATE_FPS,
    PROFILE_SECTION_UPDATE_CAMERA,
    PROFILE_SECTION_APPLY_PHYSICS,
    PROFILE_SECTION_UPDATE_SELECTED_BLOCK,
    PROFILE_SECTION_COPY_WORLD_MESH,
    PROFILE_SECTION_RENDER_TOTAL,
    PROFILE_SECTION_RENDER_CLEAR,
    PROFILE_SECTION_RENDER_SKYBOX,
    PROFILE_SECTION_RENDER_SUN,
    PROFILE_SECTION_RENDER_SHADOW_MAP,
    PROFILE_SECTION_RENDER_REFLECTION_MAP,
    PROFILE_SECTION_RENDER_WORLD,
    PROFILE_SECTION_RENDER_UI,
    PROFILE_SECTION_SWAP_BUFFERS,
    PROFILE_SECTION_POLL_EVENTS,
    PROFILE_SECTION_COUNT
} profile_section;

typedef enum {
    PROFILE_STAT_AVERAGE = 0,
    PROFILE_STAT_MIN,
    PROFILE_STAT_MAX,
    PROFILE_STAT_COUNT
} profile_stat;

void init_metrics(void);
float get_fps(void);
void update_fps(void);
int get_delta_ms(void);
void enable_client_profiling(bool enabled);
bool client_profiling_enabled(void);
void profile_configure_startup(bool enabled);
void profile_configure_output(bool show_average, bool show_min, bool show_max);
void profile_configure_log_file(bool enabled, const char* filename);
void profile_set_all_sections_enabled(bool enabled);
void profile_set_section_enabled(profile_section section, bool enabled);
bool profile_section_from_name(const char* name, profile_section* section);
void profile_startup_reset(void);
void profile_startup_checkpoint(const char* label);
void profile_frame_begin(void);
void profile_frame_end(void);
void profile_begin_section(profile_section section);
void profile_end_section(profile_section section);

#endif
