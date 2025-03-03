#include <skybox.h>
#include <settings.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <cglm/cglm.h>
#include <glad/glad.h>

#define PI 3.141592653

float* get_vertices(int* count) {
    // Each stack has 2*(slices+1) vertices (for a triangle strip)
    // Each vertex has 5 elements (x,y,z,tx,ty)
    int vertices_per_stack = 2 * (SKYBOX_SLICES + 1);
    int total_vertices = SKYBOX_STACKS * vertices_per_stack;
    int total_floats = total_vertices * 5; // 3 for position + 2 for tex coords
    
    float* vertices = malloc(total_floats * sizeof(float));
    
    int index = 0;
    
    for (int stack = 0; stack < SKYBOX_STACKS; stack++) {
        // Calculate angles for this stack and the next
        float theta1 = (float)stack / SKYBOX_STACKS * PI;
        float theta2 = (float)(stack + 1) / SKYBOX_STACKS * PI;
        
        for (int slice = 0; slice <= SKYBOX_SLICES; slice++) {
            float phi = (float)slice / SKYBOX_SLICES * 2 * PI;
            
            // Texture coordinates for bottom point (u,v)
            // u is normalized longitude [0,1]
            // v is normalized latitude [0,1]
            float u = (float)slice / SKYBOX_SLICES;
            float v2 = (float)(stack + 1) / SKYBOX_STACKS;
            
            // Vertex from next stack (bottom point) with texture coordinates
            vertices[index++] = SKYBOX_RADIUS * sin(theta2) * cos(phi);  // x
            vertices[index++] = SKYBOX_RADIUS * cos(theta2);             // y
            vertices[index++] = SKYBOX_RADIUS * sin(theta2) * sin(phi);  // z
            vertices[index++] = u;                                       // tx
            vertices[index++] = v2;                                      // ty
            
            float v1 = (float)stack / SKYBOX_STACKS;
            
            vertices[index++] = SKYBOX_RADIUS * sin(theta1) * cos(phi);  // x
            vertices[index++] = SKYBOX_RADIUS * cos(theta1);             // y
            vertices[index++] = SKYBOX_RADIUS * sin(theta1) * sin(phi);  // z
            vertices[index++] = u;                                       // tx
            vertices[index++] = v1;                                      // ty
        }
    }
    
    *count = index;
    return vertices;
}

skybox create_skybox(camera* cam) {

    int count;
    float* data = get_vertices(&count);

    // create VAO and VBO
    VAO vao = create_vao();
    bind_vao(vao);
    VBO vbo = create_vbo(GL_STATIC_DRAW);
    use_vbo(vbo);
    
    // buffer data to GPU
    buffer_data(vbo, GL_STATIC_DRAW, data, count * sizeof(float));
    f_add_attrib(&vbo, 0, 3, 0, 5 * sizeof(float)); // position
    f_add_attrib(&vbo, 1, 2, 3 * sizeof(float), 5 * sizeof(float)); // texture coords

    // load sky texture
    texture sky = t_init(SKYBOX_PATH, SKYBOX_TEXTURE_INDEX);

    // shader program
    shader vert_shader = create_shader("res/shaders/skybox.vert", GL_VERTEX_SHADER);
    shader frag_shader = create_shader("res/shaders/skybox.frag", GL_FRAGMENT_SHADER);
    shader_program program = create_program(vert_shader, frag_shader);
    delete_shader(vert_shader);
    delete_shader(frag_shader);

    skybox s = {
        .vertices = data,
        .vertex_count = count,
        .vao = vao,
        .vbo = vbo,
        .program = program,
        .texture = sky,
        .cam = cam
    };

    return s;
}

void skybox_cleanup(skybox* s) {
    free(s->vertices);
    free(s->texture_coords);
}

void get_rotation_matrix(camera* cam, mat4* out) {
    mat4 view;
    glm_mat4_identity(view);
    glm_rotate(view, -cam->yaw, (vec3){0.0f, 1.0f, 0.0f});
    glm_rotate(view, cam->pitch, (vec3){1.0f, 0.0f, 0.0f});
    glm_mat4_inv(view, *out);
}

void send_sky_view_matrix(skybox* s, camera* cam) {
    mat4 view;
    get_rotation_matrix(cam, &view);
    uint view_loc = glGetUniformLocation(s->program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
}

void send_sky_proj_matrix(skybox* s) {
    mat4 proj;
    get_projection_matrix(&proj, 45.0f, 800.0f / 600.0f, 0.1f, RENDER_DISTANCE);
    uint proj_loc = glGetUniformLocation(s->program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);
}

void send_texture(skybox* s) {
    glActiveTexture(GL_TEXTURE0 + s->texture.tex_index);
    glBindTexture(GL_TEXTURE_2D, s->texture.id);
    uint tex_loc = glGetUniformLocation(s->program.id, "skybox");
    glUniform1i(tex_loc, s->texture.tex_index);
}

void render_skybox(skybox* s) {
    glDisable(GL_DEPTH_TEST);
    use_program(s->program);
    bind_vao(s->vao);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    send_sky_view_matrix(s, s->cam);
    send_sky_proj_matrix(s);
    send_texture(s);

    int vertices_per_stack = 2 * (SKYBOX_SLICES + 1);
    
    for (int stack = 0; stack < SKYBOX_STACKS; stack++) {
        int start_vertex = stack * vertices_per_stack;
        glDrawArrays(GL_TRIANGLE_STRIP, start_vertex, vertices_per_stack);
    }

    glEnable(GL_DEPTH_TEST);
}