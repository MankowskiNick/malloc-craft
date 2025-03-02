#include <skybox.h>
// #include <camera.h>
#include <settings.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <cglm/cglm.h>

#include <stdio.h>

#define PI 3.141592653

float* get_vertices(int* count, float** texture_coords) {
    // Each stack has 2*(slices+1) vertices (for a triangle strip)
    // Each vertex has 3 coordinates (x,y,z)
    int vertices_per_stack = 2 * (SKYBOX_SLICES + 1);
    int total_vertices = SKYBOX_STACKS * vertices_per_stack;
    int total_floats = total_vertices * 3;
    
    float* vertices = malloc(total_floats * sizeof(float));
    
    int index = 0;
    
    for (int stack = 0; stack < SKYBOX_STACKS; stack++) {
        // Calculate angles for this stack and the next
        float theta1 = (float)stack / SKYBOX_STACKS * PI;
        float theta2 = (float)(stack + 1) / SKYBOX_STACKS * PI;
        
        for (int slice = 0; slice <= SKYBOX_SLICES; slice++) {
            float phi = (float)slice / SKYBOX_SLICES * 2 * PI;
            
            // Vertex from next stack (bottom point)
            vertices[index++] = SKYBOX_RADIUS * sin(theta2) * cos(phi);  // x
            vertices[index++] = SKYBOX_RADIUS * cos(theta2);             // y
            vertices[index++] = SKYBOX_RADIUS * sin(theta2) * sin(phi);  // z
            
            // Vertex from current stack (top point)
            vertices[index++] = SKYBOX_RADIUS * sin(theta1) * cos(phi);  // x
            vertices[index++] = SKYBOX_RADIUS * cos(theta1);             // y
            vertices[index++] = SKYBOX_RADIUS * sin(theta1) * sin(phi);  // z
        }
    }
    
    *count = index;  // Total number of float values
    return vertices;
}

skybox* create_skybox() {
    skybox* s = malloc(sizeof(skybox));

    int count;
    s->vertices = get_vertices(&count);
    s->vertex_count = count;

    // Create and bind VAO first
    s->vao = create_vao();
    bind_vao(s->vao);
    
    // Create, bind VBO and upload data
    s->vbo = create_vbo(GL_STATIC_DRAW);
    use_vbo(s->vbo);
    
    // Load buffer data during initialization
    glBufferData(GL_ARRAY_BUFFER, s->vertex_count * sizeof(float), s->vertices, GL_STATIC_DRAW);
    
    // Set up vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    shader vert_shader = create_shader("res/shaders/skybox.vert", GL_VERTEX_SHADER);
    shader frag_shader = create_shader("res/shaders/skybox.frag", GL_FRAGMENT_SHADER);
    s->program = create_program(vert_shader, frag_shader);

    return s;
}

void destroy_skybox(skybox* s) {
    free(s->vertices);
    free(s);
}

void get_rotation_matrix(camera* cam, mat4* out) {
    mat4 view;
    glm_mat4_identity(view);
    glm_rotate(view, -cam->yaw, (vec3){0.0f, 1.0f, 0.0f});
    glm_rotate(view, cam->pitch, (vec3){1.0f, 0.0f, 0.0f});
    glm_mat4_inv(view, *out);
}

void draw_skybox(skybox* s, camera* cam) {

    glDisable(GL_DEPTH_TEST);
    use_program(s->program);
    bind_vao(s->vao);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    mat4 view;
    get_rotation_matrix(cam, &view);
    uint view_loc = glGetUniformLocation(s->program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);

    mat4 proj;
    get_projection_matrix(&proj, 45.0f, 800.0f / 600.0f, 0.1f, RENDER_DISTANCE);
    uint proj_loc = glGetUniformLocation(s->program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);

    int vertices_per_stack = 2 * (SKYBOX_SLICES + 1);
    
    for (int stack = 0; stack < SKYBOX_STACKS; stack++) {
        int start_vertex = stack * vertices_per_stack;
        glDrawArrays(GL_TRIANGLE_STRIP, start_vertex, vertices_per_stack);
    }

    glEnable(GL_DEPTH_TEST);
}