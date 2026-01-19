#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/cglm.h>

typedef struct {
    float x, y, z;
} camera_cache;

typedef struct {
    vec3 position;
    vec3 up;
    vec3 front;

    float yaw;
    float pitch;
} camera;

void update_orientation(camera* cam);

void get_view_matrix(camera c, mat4* view);
void get_projection_matrix(mat4* proj, float fov, float aspect, float near, float far);

#endif
