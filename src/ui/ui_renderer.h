#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <vao.h>
#include <vbo.h>
#include <shader.h>
#include <texture.h>
#include <cglm/cglm.h>

typedef struct {
    int digit_atlas_coords[10][2];
    int char_width;
    int char_height;
    int scale;
} ui_font_config;

typedef struct {
    int slot_atlas_coord[2];
    int selected_slot_atlas_coord[2];
    int slot_size;
    int scale;
    int padding;
} ui_hotbar_config;

typedef struct {
    int x;
    int y;
    int spacing;
} ui_fps_config;

typedef struct {
    VAO vao;
    VBO vbo;
    shader_program program;
    texture atlas;
    mat4 projection;

    ui_font_config font;
    ui_hotbar_config hotbar;
    ui_fps_config fps_config;
} ui_renderer;

void read_ui_settings(const char* filename);

ui_renderer create_ui_renderer();
void destroy_ui_renderer(ui_renderer* ui);

void render_ui_quad(ui_renderer* ui, float x, float y, float width, float height, int atlas_x, int atlas_y);
void render_fps(ui_renderer* ui, int fps);
void render_hotbar(ui_renderer* ui, char** hotbar, int hotbar_size, int selected_block);

#endif
