#include "ui_renderer.h"
#include <util/settings.h>
#include <render/core/window.h>
#include <block.h>
#include <block_models.h>
#include <cerialize/cerialize.h>
#include <util.h>
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ui_font_config g_font_config;
static ui_hotbar_config g_hotbar_config;
static ui_fps_config g_fps_config;

void read_ui_settings(const char* filename) {
    char* ui_json = read_file_to_string(filename);
    if (ui_json == NULL) {
        fprintf(stderr, "Error: Failed to read UI settings file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    json obj = deserialize_json(ui_json, strlen(ui_json));
    if (obj.failure) {
        fprintf(stderr, "Error: Failed to parse UI JSON '%s': %s\n", filename, obj.error_text);
        free(ui_json);
        exit(EXIT_FAILURE);
    }

    if (obj.root.type != JSON_OBJECT) {
        fprintf(stderr, "Error: Root of ui.json must be an object\n");
        free(ui_json);
        json_free(&obj);
        exit(EXIT_FAILURE);
    }

    // Parse font settings
    json_object font_obj = json_get_property(obj.root, "font");
    if (font_obj.type == JSON_OBJECT) {
        json_object digits_obj = json_get_property(font_obj, "digits");
        if (digits_obj.type == JSON_OBJECT) {
            for (int i = 0; i < 10; i++) {
                char digit_str[2] = { '0' + i, '\0' };
                json_object digit = json_get_property(digits_obj, digit_str);
                if (digit.type == JSON_LIST && digit.value.list.count >= 2) {
                    g_font_config.digit_atlas_coords[i][0] = (int)digit.value.list.items[0].value.number;
                    g_font_config.digit_atlas_coords[i][1] = (int)digit.value.list.items[1].value.number;
                }
            }
        }

        json_object char_width = json_get_property(font_obj, "char_width");
        if (char_width.type == JSON_NUMBER) {
            g_font_config.char_width = (int)char_width.value.number;
        }

        json_object char_height = json_get_property(font_obj, "char_height");
        if (char_height.type == JSON_NUMBER) {
            g_font_config.char_height = (int)char_height.value.number;
        }

        json_object scale = json_get_property(font_obj, "scale");
        if (scale.type == JSON_NUMBER) {
            g_font_config.scale = (int)scale.value.number;
        }
    }

    // Parse hotbar settings
    json_object hotbar_obj = json_get_property(obj.root, "hotbar");
    if (hotbar_obj.type == JSON_OBJECT) {
        json_object slot = json_get_property(hotbar_obj, "slot");
        if (slot.type == JSON_LIST && slot.value.list.count >= 2) {
            g_hotbar_config.slot_atlas_coord[0] = (int)slot.value.list.items[0].value.number;
            g_hotbar_config.slot_atlas_coord[1] = (int)slot.value.list.items[1].value.number;
        }

        json_object selected_slot = json_get_property(hotbar_obj, "selected_slot");
        if (selected_slot.type == JSON_LIST && selected_slot.value.list.count >= 2) {
            g_hotbar_config.selected_slot_atlas_coord[0] = (int)selected_slot.value.list.items[0].value.number;
            g_hotbar_config.selected_slot_atlas_coord[1] = (int)selected_slot.value.list.items[1].value.number;
        }

        json_object slot_size = json_get_property(hotbar_obj, "slot_size");
        if (slot_size.type == JSON_NUMBER) {
            g_hotbar_config.slot_size = (int)slot_size.value.number;
        }

        json_object scale = json_get_property(hotbar_obj, "scale");
        if (scale.type == JSON_NUMBER) {
            g_hotbar_config.scale = (int)scale.value.number;
        }

        json_object padding = json_get_property(hotbar_obj, "padding");
        if (padding.type == JSON_NUMBER) {
            g_hotbar_config.padding = (int)padding.value.number;
        }
    }

    // Parse FPS counter settings
    json_object fps_obj = json_get_property(obj.root, "fps_counter");
    if (fps_obj.type == JSON_OBJECT) {
        json_object x = json_get_property(fps_obj, "x");
        if (x.type == JSON_NUMBER) {
            g_fps_config.x = (int)x.value.number;
        }

        json_object y = json_get_property(fps_obj, "y");
        if (y.type == JSON_NUMBER) {
            g_fps_config.y = (int)y.value.number;
        }

        json_object spacing = json_get_property(fps_obj, "spacing");
        if (spacing.type == JSON_NUMBER) {
            g_fps_config.spacing = (int)spacing.value.number;
        }
    }

    free(ui_json);
    json_free(&obj);
}

ui_renderer create_ui_renderer() {
    // Read UI settings from JSON
    read_ui_settings("res/ui.json");

    // Create VAO and VBO for quad rendering
    VAO vao = create_vao();
    bind_vao(vao);
    VBO vbo = create_vbo(GL_DYNAMIC_DRAW);

    // Quad vertices: position (x, y) + texcoord (u, v)
    // We'll update this data dynamically for each quad
    float quad_vertices[] = {
        // pos      // tex
        0.0f, 1.0f, 0.0f, 0.0f,  // top-left
        0.0f, 0.0f, 0.0f, 1.0f,  // bottom-left
        1.0f, 0.0f, 1.0f, 1.0f,  // bottom-right

        0.0f, 1.0f, 0.0f, 0.0f,  // top-left
        1.0f, 0.0f, 1.0f, 1.0f,  // bottom-right
        1.0f, 1.0f, 1.0f, 0.0f   // top-right
    };

    buffer_data(vbo, GL_DYNAMIC_DRAW, quad_vertices, sizeof(quad_vertices));
    use_vbo(vbo);

    // Position attribute (location 0)
    f_add_attrib(&vbo, 0, 2, 0, 4 * sizeof(float));
    // Texture coordinate attribute (location 1)
    f_add_attrib(&vbo, 1, 2, 2 * sizeof(float), 4 * sizeof(float));

    // Create shaders
    shader vertex_shader = create_shader("res/shaders/ui.vert", GL_VERTEX_SHADER);
    shader fragment_shader = create_shader("res/shaders/ui.frag", GL_FRAGMENT_SHADER);
    shader_program program = create_program(vertex_shader, fragment_shader);
    delete_shader(vertex_shader);
    delete_shader(fragment_shader);

    // Load atlas texture (reuse the existing atlas)
    texture atlas = t_init(ATLAS_PATH, ATLAS_TEXTURE_INDEX);

    // Create orthographic projection matrix
    mat4 projection;
    glm_ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f, projection);

    // Apply scaling from settings
    g_hotbar_config.scale = (int)(g_hotbar_config.scale * UI_SCALE);
    g_fps_config.scale = FPS_COUNTER_SCALE;

    ui_renderer ui = {
        .vao = vao,
        .vbo = vbo,
        .program = program,
        .atlas = atlas,
        .font = g_font_config,
        .hotbar = g_hotbar_config,
        .fps_config = g_fps_config,
    };
    glm_mat4_copy(projection, ui.projection);

    return ui;
}

void destroy_ui_renderer(ui_renderer* ui) {
    delete_vao(ui->vao);
    delete_vbo(ui->vbo);
    delete_program(ui->program);
    t_cleanup(&ui->atlas);
}

void render_ui_quad_ex(ui_renderer* ui, float x, float y, float width, float height, int atlas_x, int atlas_y, bool flip_v) {
    use_program(ui->program);

    // Update projection matrix based on current screen size
    mat4 projection;
    glm_ortho(0.0f, (float)get_screen_width(), (float)get_screen_height(), 0.0f, -1.0f, 1.0f, projection);

    // Set uniforms
    uint proj_loc = glGetUniformLocation(ui->program.id, "projection");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)projection);

    uint atlas_coord_loc = glGetUniformLocation(ui->program.id, "atlasCoord");
    glUniform2f(atlas_coord_loc, (float)atlas_x, (float)atlas_y);

    uint atlas_size_loc = glGetUniformLocation(ui->program.id, "atlasSize");
    glUniform1f(atlas_size_loc, (float)ATLAS_SIZE);

    // Bind atlas texture
    glActiveTexture(GL_TEXTURE0 + ui->atlas.tex_index);
    glBindTexture(GL_TEXTURE_2D, ui->atlas.id);

    uint atlas_tex_loc = glGetUniformLocation(ui->program.id, "atlas");
    glUniform1i(atlas_tex_loc, ui->atlas.tex_index);

    // UV coordinates - flip if requested
    float v_top = flip_v ? 0.0f : 1.0f;
    float v_bottom = flip_v ? 1.0f : 0.0f;

    // Update vertex data for this quad
    float quad_vertices[] = {
        // pos                  // tex
        x, y,                   0.0f, v_top,     // top-left
        x, y + height,          0.0f, v_bottom,  // bottom-left
        x + width, y + height,  1.0f, v_bottom,  // bottom-right

        x, y,                   0.0f, v_top,     // top-left
        x + width, y + height,  1.0f, v_bottom,  // bottom-right
        x + width, y,           1.0f, v_top      // top-right
    };

    bind_vao(ui->vao);
    use_vbo(ui->vbo);
    buffer_data(ui->vbo, GL_DYNAMIC_DRAW, quad_vertices, sizeof(quad_vertices));

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render_ui_quad(ui_renderer* ui, float x, float y, float width, float height, int atlas_x, int atlas_y) {
    render_ui_quad_ex(ui, x, y, width, height, atlas_x, atlas_y, false);
}

void render_fps(ui_renderer* ui, int fps) {
    // Clamp FPS to reasonable range
    if (fps < 0) fps = 0;
    if (fps > 9999) fps = 9999;

    // Convert FPS to digits
    char fps_str[8];
    snprintf(fps_str, sizeof(fps_str), "%d", fps);

    float x = (float)ui->fps_config.x;
    float y = (float)ui->fps_config.y;
    float char_width = (float)(ui->font.char_width * ui->font.scale) * ui->fps_config.scale;
    float char_height = (float)(ui->font.char_height * ui->font.scale) * ui->fps_config.scale;
    float spacing = (float)ui->fps_config.spacing * ui->fps_config.scale;

    // Render each digit
    for (int i = 0; fps_str[i] != '\0'; i++) {
        int digit = fps_str[i] - '0';
        if (digit >= 0 && digit <= 9) {
            render_ui_quad(ui, x, y, char_width, char_height,
                          ui->font.digit_atlas_coords[digit][0],
                          ui->font.digit_atlas_coords[digit][1]);
        }
        x += char_width + spacing;
    }
}

void render_hotbar(ui_renderer* ui, char** hotbar, int hotbar_size, int selected_block) {
    int screen_width = get_screen_width();
    int screen_height = get_screen_height();
    float slot_size = (float)(ui->hotbar.slot_size * ui->hotbar.scale);
    float padding = (float)(ui->hotbar.padding * ui->hotbar.scale);
    float total_width = hotbar_size * slot_size + (hotbar_size - 1) * padding;
    float start_x = ((float)screen_width - total_width) / 2.0f;
    float y = (float)screen_height - slot_size - (20.0f * ui->hotbar.scale);  // 20 pixels from bottom

    for (int i = 0; i < hotbar_size; i++) {
        // Map visual position to slot: display order is 1,2,3,4,5,6,7,8,9,0
        // Visual position i=0 shows slot 1, i=8 shows slot 9, i=9 shows slot 0
        int slot_index = (i + 1) % hotbar_size;
        float x = start_x + i * (slot_size + padding);
        int* slot_coord;

        // Use selected or normal slot texture
        if (slot_index == selected_block) {
            slot_coord = ui->hotbar.selected_slot_atlas_coord;
        } else {
            slot_coord = ui->hotbar.slot_atlas_coord;
        }

        // Render slot background
        render_ui_quad(ui, x, y, slot_size, slot_size, slot_coord[0], slot_coord[1]);

        // Render block top face inside the slot
        if (hotbar[slot_index] != NULL) {
            short block_id = get_block_id(hotbar[slot_index]);
            if (block_id >= 0) {
                block_type* bt = get_block_type(block_id);
                if (bt != NULL && !bt->is_custom_model) {
                    // Get atlas coords - use face 0 for foliage, UP face (4) for others
                    int face_index = bt->is_foliage ? 0 : 4;
                    int atlas_x = bt->face_atlas_coords[face_index][0];
                    int atlas_y = bt->face_atlas_coords[face_index][1];

                    // Only render if valid coords
                    if (atlas_x >= 0 && atlas_y >= 0) {
                        // Render block slightly smaller and centered within slot
                        float block_padding = slot_size * 0.1f;
                        float block_size = slot_size - 2 * block_padding;
                        render_ui_quad_ex(ui, x + block_padding, y + block_padding,
                                      block_size, block_size, atlas_x, atlas_y, false);
                    }
                }
            }
        }
    }
}
