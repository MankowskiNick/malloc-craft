#include <camera.h>

void get_view_matrix(camera c, mat4* view) {
    vec3 dest = {
        c.position[0] + c.front[0], 
        c.position[1] + c.front[1], 
        c.position[2] + c.front[2]
    };
    glm_lookat(c.position, dest, c.up, *view);
}

void get_projection_matrix(mat4* proj, float fov, float aspect, float near, float far) {
    glm_perspective(fov, aspect, near, far, *proj);
}

void update_orientation(camera* cam) {
    if (cam->pitch > 1.57f)
        cam->pitch = 1.57f;
    if (cam->pitch < -1.57f)
        cam->pitch = -1.57f;

    if (cam->yaw > 6.28f)
        cam->yaw = 0.0f;
    if (cam->yaw < 0.0f)
        cam->yaw = 6.28f;

    vec3 front = {
        cos(cam->yaw) * cos(cam->pitch),
        sin(cam->pitch),
        sin(cam->yaw) * cos(cam->pitch)
    };

    glm_vec3_normalize(front);
    glm_vec3_copy(front, cam->front);
}

void update_position(camera* cam, float direction[3]) {
    cam->position[0] += direction[0];
    cam->position[1] += direction[1];
    cam->position[2] += direction[2];
}