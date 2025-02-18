#include <input.h>
#include <camera.h>
#include <cglm/cglm.h>

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

void update_pos(camera* cam, int key) {
    vec3 front, up, right;
    glm_normalize_to(cam->front, front);
    glm_normalize_to(cam->up, up);
    glm_vec3_cross(front, up, right);
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
        case GLFW_KEY_LEFT_CONTROL:
            dy = -DELTA_Y;
            break;
        default:
            break;
    }

    update_position(cam, (float[3]){dx, dy, dz});
}

void update_camera() {
    key_entry* cur = key_stack;
    while(cur != NULL) {
        update_pos(cam, cur->key);
        cur = cur->next;
    }

    update_orientation(cam);
    
    printf("Position: (%f, %f, %f)\n", cam->position[0], cam->position[1], cam->position[2]);
}

void handle_press(int key) {
    key_entry* new_entry = malloc(sizeof(key_entry));
    new_entry->key = key;
    new_entry->next = key_stack;
    key_stack = new_entry;
}

void handle_release(int key) {
    key_entry* cur = key_stack;
    key_entry* prev = NULL;

    while (cur != NULL && cur->key != key) {
        prev = cur;
        cur = key_stack->next;
    }

    if (cur == NULL)
        return;

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
            handle_press(key);
            break;
        case GLFW_RELEASE:
            handle_release(key);
            break;
        default:
            break;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    double dx = xpos - mouse.x;
    double dy = ypos - mouse.y;

    mouse.x = xpos;
    mouse.y = ypos;

    cam->yaw += dx * SENSITIVITY;
    cam->pitch -= dy * SENSITIVITY;
}

void i_init(GLFWwindow* window, camera* player_cam) {
    key_stack = NULL;

    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

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