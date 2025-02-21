#include <input.h>
#include <block_models.h>
#include <block.h>
#include <world.h>
#include <camera.h>
#include <settings.h>
#include <cglm/cglm.h>

#ifndef PI
    #define PI 3.141592653
#endif

typedef struct key_entry {
    int key;
    struct key_entry* next;
} key_entry;

typedef struct {
    double x, y;
} mouse_capture;

mouse_capture mouse = {0.0f, 0.0f};
key_entry* key_stack;

camera* cam;

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

    update_camera_pos(cam, (float[3]){dx, dy, dz});
}

void update_position() {
    key_entry* cur = key_stack;
    while(cur != NULL) {
        vec3 front, up, right;
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
    update_orientation(cam);
}



void handle_keypress(int key) {
    // non movement key presses
    switch(key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
            return;
        default:
            break;
    }

    // check for existing entry
    key_entry* cur = key_stack;
    while(cur != NULL) {
        if (cur->key == key)
            return;
        cur = cur->next;
    }

    // new entry
    key_entry* new_entry = malloc(sizeof(key_entry));
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
    double dx = xpos - mouse.x;
    double dy = ypos - mouse.y;

    mouse.x = xpos;
    mouse.y = ypos;

    cam->yaw += dx * SENSITIVITY;
    cam->pitch -= dy * SENSITIVITY;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        uint chunk_x, chunk_z;

        chunk* c = get_chunk_at(cam->position[0], cam->position[2], &chunk_x, &chunk_z);

        printf("Player position: %f, %f, %f\n", cam->position[0], cam->position[1], cam->position[2]);
        printf("Chunk: %d, %d\n", c->x, c->z);
        printf("Coords within chunk: %i, %i\n\n", chunk_x, chunk_z);
    }
}

void i_init(GLFWwindow* window, camera* player_cam) {
    key_stack = NULL;

    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_move_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    cam = player_cam;
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