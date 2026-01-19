#include <outline_renderer.h>
#include <settings.h>
#include <glad/glad.h>
#include <stdio.h>

outline_renderer create_outline_renderer(camera* cam) {
    // Create VAO and VBO for cube edges
    VAO vao = create_vao();
    bind_vao(vao);
    VBO vbo = create_vbo(GL_STATIC_DRAW);

    // Define the 12 edges of a cube (24 vertices - 2 per edge)
    // Each edge is defined by its start and end vertex
    float edges[] = {
        // Bottom face (z=0)
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,  // edge 1
        
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,  // edge 2
        
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,  // edge 3
        
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,  // edge 4
        
        // Top face (z=1)
        0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,  // edge 5
        
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,  // edge 6
        
        1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,  // edge 7
        
        0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f,  // edge 8
        
        // Vertical edges
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,  // edge 9
        
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,  // edge 10
        
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,  // edge 11
        
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f,  // edge 12
    };

    // Buffer the edge data
    buffer_data(vbo, GL_STATIC_DRAW, edges, sizeof(edges));
    
    // Set vertex attributes
    use_vbo(vbo);
    f_add_attrib(&vbo, 0, 3, 0, 3 * sizeof(float)); // position

    // Create shaders
    shader vertex_shader = create_shader("res/shaders/outline.vert", GL_VERTEX_SHADER);
    shader fragment_shader = create_shader("res/shaders/outline.frag", GL_FRAGMENT_SHADER);
    shader_program program = create_program(vertex_shader, fragment_shader);
    delete_shader(vertex_shader);
    delete_shader(fragment_shader);

    outline_renderer or = {
        .vao = vao,
        .vbo = vbo,
        .program = program,
        .cam = cam,
    };

    return or;
}

void destroy_outline_renderer(outline_renderer or) {
    delete_vao(or.vao);
    delete_vbo(or.vbo);
    delete_program(or.program);
}

void send_outline_view_matrix(outline_renderer* or) {
    mat4 view;
    get_view_matrix(*(or->cam), &view);
    uint view_loc = glGetUniformLocation(or->program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
}

void send_outline_proj_matrix(outline_renderer* or) {
    mat4 proj;
    get_projection_matrix(&proj, RADS(FOV), (float)WIDTH / (float)HEIGHT, 0.1f, RENDER_DISTANCE);
    uint proj_loc = glGetUniformLocation(or->program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);
}

void render_outline(outline_renderer* or, int block_x, int block_y, int block_z) {
    if (or == NULL) {
        return;
    }

    // Use the outline shader program
    use_program(or->program);

    // Set up matrices
    send_outline_view_matrix(or);
    send_outline_proj_matrix(or);

    // Create model matrix for the block position
    mat4 model = GLM_MAT4_IDENTITY_INIT;
    glm_translate(model, (vec3){(float)block_x, (float)block_y, (float)block_z});

    uint model_loc = glGetUniformLocation(or->program.id, "model");
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float*)model);

    // Enable polygon offset to render outline on top of the block
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-2.0f, -2.0f);

    // Use LEQUAL depth function so outline renders even at same depth
    glDepthFunc(GL_LEQUAL);

    // Set line width
    glLineWidth(2.0f);

    // Enable line rendering and bind VAO
    bind_vao(or->vao);

    // Draw the 12 edges (24 vertices, GL_LINES draws 2 vertices per line)
    glDrawArrays(GL_LINES, 0, 24);

    // Reset to normal depth function and polygon offset
    glDepthFunc(GL_LESS);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glLineWidth(1.0f);
}
