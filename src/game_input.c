#include <input.h>
#include <block_models.h>
#include <block.h>
#include <world.h>
#include <camera.h>
#include <chunk_mesh.h>
#include <settings.h>
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

void update_pos(int key, vec3 front, vec3 right) {
    float dx = 0.0f, dy = 0.0f, dz = 0.0f;
    switch(key) {
        case GLFW_KEY_W:
            dx = DELTA_X * front[0];
            dy = DELTA_Y * front[1];
            dz = DELTA_Z * front[2];
            break;
        case GLFW_KEY_S:
            dx = -DELTA_X * front[0];
            dy = -DELTA_Y * front[1];
            dz = -DELTA_Z * front[2];
            break;
        case GLFW_KEY_A:
            dx = -DELTA_X * right[0];
            dy = -DELTA_Y * right[1];
            dz = -DELTA_Z * right[2];
            break;
        case GLFW_KEY_D:
            dx = DELTA_X * right[0];
            dy = DELTA_Y * right[1];
            dz = DELTA_Z * right[2];
            break;
        case GLFW_KEY_SPACE:
            dy = DELTA_Y;
            break;
        case GLFW_KEY_LEFT_SHIFT:
            dy = -DELTA_Y;
            break;
        default:
            break;
    }

    camera* cam = &(g_data->player->cam);
    update_camera_pos(cam, (float[3]){dx, dy, dz});
}

void update_position() {
    key_entry* cur = key_stack;
    while(cur != NULL) {
        vec3 front, up, right;
        camera* cam = &(g_data->player->cam);
        glm_normalize_to(cam->front, front);
        glm_normalize_to(cam->up, up);
        glm_vec3_cross(front, up, right);
        glm_normalize_to(right, right);

        update_pos(cur->key, front, right);
        cur = cur->next;
    }
}

void update_camera() {
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

    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
        int index = key - GLFW_KEY_0;
        if (index < g_data->player->hotbar_size) {
            g_data->player->selected_block = index;
        }
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
                break_block(*g_data->player);
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                g_data->mesh_requires_update = TRUE;
                place_block(*g_data->player);
                break;
            default:
                break;
        }
        unlock_mesh();
    }
}

void i_init(GLFWwindow* window, game_data* data) {
    key_stack = NULL;

    // lock cursor to center of window

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_move_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

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
