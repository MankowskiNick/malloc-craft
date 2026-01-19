#include "render/core/window.h"
#include "input.h"
#include <block_models.h>
#include "world/core/block.h"
#include "world/core/world.h"
#include <player/core/camera.h>
#include <chunk_mesh.h>
#include "util/settings.h"
#include <cglm/cglm.h>

#ifndef PI
    #define PI 3.141592653
#endif

typedef struct key_entry {
    int key;
    struct key_entry* next;
} key_entry;

key_entry* key_stack;
game_data* g_data;
GLFWwindow* g_window;

void update_pos(int key, vec3 front, vec3 right, int is_underwater) {
    // Calculate desired direction (not velocity, not acceleration directly)
    // This will be applied as acceleration in apply_physics()
    float dx = 0.0f, dy = 0.0f, dz = 0.0f;
    
    switch(key) {
        case GLFW_KEY_W:
            if (is_underwater) {
                // When underwater, W moves in full 3D camera direction (includes vertical)
                dx = front[0];
                dy = front[1];
                dz = front[2];
            } else {
                // On ground, W moves only horizontally (camera front's horizontal component)
                dx = front[0];
                dz = front[2];
            }
            break;
        case GLFW_KEY_S:
            if (is_underwater) {
                // Reverse full 3D direction when underwater
                dx = -front[0];
                dy = -front[1];
                dz = -front[2];
            } else {
                // Reverse horizontal direction
                dx = -front[0];
                dz = -front[2];
            }
            break;
        case GLFW_KEY_A:
            dx = -right[0];
            dz = -right[2];
            break;
        case GLFW_KEY_D:
            dx = right[0];
            dz = right[2];
            break;
        default:
            break;
    }
    
    // Accumulate desired direction (will be normalized later)
    player* p = g_data->player;
    p->acceleration[0] += dx;
    p->acceleration[1] += dy;  // Vertical movement when swimming
    p->acceleration[2] += dz;
}

void update_position() {
    // Reset desired direction accumulation
    player* p = g_data->player;
    p->acceleration[0] = 0.0f;
    p->acceleration[1] = 0.0f;  // Reset vertical acceleration
    p->acceleration[2] = 0.0f;
    
    // Iterate through key stack to accumulate desired direction
    key_entry* cur = key_stack;
    while(cur != NULL) {
        if (cur->key == GLFW_KEY_SPACE) {
            // Space: move up when flying or swimming
            if (p->fly_mode || p->is_underwater) {
                p->acceleration[1] += SWIM_VERTICAL_ACCEL;  // Strong upward acceleration
            }
        } else if (cur->key == GLFW_KEY_LEFT_SHIFT) {
            // Shift: move down when flying or swimming
            if (p->fly_mode || p->is_underwater) {
                p->acceleration[1] -= SWIM_VERTICAL_ACCEL;  // Strong downward acceleration
            }
        } else {
            // Movement keys (W, A, S, D)
            vec3 front, up, right;
            camera* cam = &(g_data->player->cam);
            glm_normalize_to(cam->front, front);
            glm_normalize_to(cam->up, up);
            glm_vec3_cross(front, up, right);
            glm_normalize_to(right, right);
            
            update_pos(cur->key, front, right, p->is_underwater || p->fly_mode);
        }
        cur = cur->next;
    }
}

void update_camera(int delta_ms) {
    update_position();
    update_orientation(&(g_data->player->cam));
}



void handle_keypress(int key) {
    /* quick handles for non-movement keys */
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
        g_data->is_running = FALSE;
        return;
    }

    /* Toggle FPS counter with F1 key */
    if (key == GLFW_KEY_F1) {
        g_data->show_fps = !g_data->show_fps;
        return;
    }

    /* Toggle fullscreen with F11 key */
    if (key == GLFW_KEY_F11) {
        toggle_fullscreen(g_window);
        return;
    }

    /* Toggle fly mode with V key */
    if (key == GLFW_KEY_V) {
        g_data->player->fly_mode = !g_data->player->fly_mode;
        // Reset velocity when entering/exiting fly mode to prevent momentum carry-over
        g_data->player->velocity[0] = 0.0f;
        g_data->player->velocity[1] = 0.0f;
        g_data->player->velocity[2] = 0.0f;
        return;
    }

    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
        int index = key - GLFW_KEY_0;
        if (index < g_data->player->hotbar_size) {
            g_data->player->selected_block = index;
        }
        return;
    }

    /* Handle jump input */
    if (key == GLFW_KEY_SPACE) {
        g_data->player->jump_requested = 1;
        // Also add to key stack for swimming control
        for (key_entry* cur = key_stack; cur != NULL; cur = cur->next) {
            if (cur->key == key) return;  // Already in stack
        }
        key_entry* new_entry = malloc(sizeof(*new_entry));
        if (!new_entry) return;
        new_entry->key = key;
        new_entry->next = key_stack;
        key_stack = new_entry;
        return;
    }

    /* avoid duplicate entries in the key stack */
    for (key_entry* cur = key_stack; cur != NULL; cur = cur->next) {
        if (cur->key == key) return;
    }

    key_entry* new_entry = malloc(sizeof(*new_entry));
    if (!new_entry) return; /* allocation failed, ignore key */
    new_entry->key = key;
    new_entry->next = key_stack;
    key_stack = new_entry;
}

void handle_keyrelease(int key) {
    key_entry* cur = key_stack;
    key_entry* prev = NULL;

    // search for key in stack
    while (cur != NULL && cur->key != key) {
        prev = cur;
        cur = cur->next;
    }

    // not found - return
    if (cur == NULL) {
        return;
    }

    // remove key from stack
    if (prev == NULL) {
        key_stack = cur->next;
    }
    else {
        prev->next = cur->next;
    }
    free(cur);
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch(action) {
        case GLFW_PRESS:
            handle_keypress(key);
            break;
        case GLFW_RELEASE:
            handle_keyrelease(key);
            break;
        default:
            break;
    }
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos) {
    double dx = xpos - WIDTH / 2;
    double dy = ypos - HEIGHT / 2;

    camera* cam = &(g_data->player->cam);
    cam->yaw += dx * SENSITIVITY;
    cam->pitch -= dy * SENSITIVITY;


    glfwSetCursorPos(window, WIDTH / 2, HEIGHT / 2);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        lock_mesh();
        switch(button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                g_data->mesh_requires_update = TRUE;
                break_block(g_data);
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                g_data->mesh_requires_update = TRUE;
                place_block(g_data);
                break;
            default:
                break;
        }
        unlock_mesh();
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (g_data->player->hotbar_size <= 0) return;
    
    // yoffset > 0 means scroll up (next item), < 0 means scroll down (previous item)
    if (yoffset < 0) {
        // Scroll up - cycle to next item
        g_data->player->selected_block = (g_data->player->selected_block + 1) % g_data->player->hotbar_size;
    } else if (yoffset > 0) {
        // Scroll down - cycle to previous item
        g_data->player->selected_block = (g_data->player->selected_block - 1 + g_data->player->hotbar_size) % g_data->player->hotbar_size;
    }
}

void i_init(GLFWwindow* window, game_data* data) {
    key_stack = NULL;
    g_window = window;

    // lock cursor to center of window

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_move_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    g_data = data;
}

void i_cleanup() {
    key_entry* cur = key_stack;
    key_entry* next = NULL;

    while(cur != NULL) {
        next = cur->next;
        free(cur);
        cur = next;
    }
}
