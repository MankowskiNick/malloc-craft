#include <sun.h>
#include <skybox.h>
#include <settings.h>
#include <math.h>
#include <util.h>
#include <glad/glad.h>

float* get_sun_vertices(float color[3], int* count) {
    // Each stack has 2*(slices+1) vertices (for a triangle strip)
    // Each vertex has 5 elements (x,y,z,tx,ty)
    int vertices_per_stack = 2 * (SUN_SLICES + 1);
    int total_vertices = SUN_STACKS * vertices_per_stack;
    int total_floats = total_vertices * 3; // 3 for position
    
    float* vertices = malloc(total_floats * sizeof(float));
    
    int index = 0;
    
    for (int stack = 0; stack < SUN_STACKS; stack++) {
        // Calculate angles for this stack and the next
        float theta1 = (float)stack / SUN_STACKS * PI;
        float theta2 = (float)(stack + 1) / SUN_STACKS * PI;
        
        for (int slice = 0; slice <= SUN_SLICES; slice++) {
            float phi = (float)slice / SUN_SLICES * 2 * PI;
            
            // Vertex from next stack (bottom point) with texture coordinates
            vertices[index++] = SUN_RADIUS * sin(theta2) * cos(phi);  // x
            vertices[index++] = SUN_RADIUS * cos(theta2);             // y
            vertices[index++] = SUN_RADIUS * sin(theta2) * sin(phi);  // z
            
            float v1 = (float)stack / SUN_STACKS;
            
            vertices[index++] = SUN_RADIUS * sin(theta1) * cos(phi);  // x
            vertices[index++] = SUN_RADIUS * cos(theta1);             // y
            vertices[index++] = SUN_RADIUS * sin(theta1) * sin(phi);  // z
        }
    }
    
    *count = index;
    return vertices;
}

sun create_sun(camera* cam, float r, float b, float g) {

    VAO vao = create_vao();
    VBO vbo = create_vbo(GL_STATIC_DRAW);

    shader vertex_shader = create_shader("res/shaders/sun.vert", GL_VERTEX_SHADER);
    shader fragment_shader = create_shader("res/shaders/sun.frag", GL_FRAGMENT_SHADER);
    shader_program program = create_program(vertex_shader, fragment_shader);

    delete_shader(vertex_shader);
    delete_shader(fragment_shader);

    float color[3] = {r, g, b};

    int count;
    float* vertices = get_sun_vertices(color, &count);

    // buffer points to GPU
    buffer_data(vbo, GL_STATIC_DRAW, vertices, count * sizeof(float));
    f_add_attrib(&vbo, 0, 3, 0, 3 * sizeof(float));

    sun s = {
        .x = 0.0f,
        .y = 0.0f,
        .z = 0.0f,

        .r = r,
        .g = g,
        .b = b,

        .cam = cam,

        .vao = vao,
        .vbo = vbo,
        .program = program,
        .vertices = vertices,
        .vertex_count = count
    };

    return s;
}

void sun_cleanup(sun* s) {
    delete_vao(s->vao);
    delete_vbo(s->vbo);
    delete_program(s->program);
    free(s->vertices);
}

void update_sun(sun* s, float t) {
    s->y = SKYBOX_RADIUS * cos(t * TIME_SCALE);
    s->z = SKYBOX_RADIUS * sin(t * TIME_SCALE);
}

void render_sun(sun* s) {
    update_sun(s, 0.0f);

    use_program(s->program);
    bind_vao(s->vao);
    use_vbo(s->vbo);

    send_sky_view_matrix(&(s->program), s->cam);
    send_sky_proj_matrix(&(s->program));

    glUniform1f(glGetUniformLocation(s->program.id, "intensity"), SUN_INTENSITY);
    glUniform3f(glGetUniformLocation(s->program.id, "color"), s->r, s->g, s->b);
    glUniform3f(glGetUniformLocation(s->program.id, "sunPos"), s->x, s->y, s->z);
    

    int vertices_per_stack = 2 * (SKYBOX_SLICES + 1);
    
    for (int stack = 0; stack < SUN_STACKS; stack++) {
        int start_vertex = stack * vertices_per_stack;
        glDrawArrays(GL_TRIANGLE_STRIP, start_vertex, vertices_per_stack);
    }
}
