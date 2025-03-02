#include <world_renderer.h>
#include <mesh.h>
#include <sort.h>
#include <world.h>
#include <glad/glad.h>

#define CAMERA_POS_TO_CHUNK_POS(x) x >= 0 ? (int)(x / CHUNK_SIZE) : (int)(x / CHUNK_SIZE) - 1

camera* r_cam_ref;

float chunk_distance_to_camera(const void* item) {
    chunk_mesh* packet = *(chunk_mesh**)item;
    // camera coords to chunk coords
    float x = (r_cam_ref->position[0] / (float)CHUNK_SIZE);
    float z = (r_cam_ref->position[2] / (float)CHUNK_SIZE);

    return -1.0f * sqrt(
        pow((float)(packet->x + 0.5f) - x, 2) +
        pow((float)(packet->z + 0.5f) - z, 2)
    );
}

world_renderer create_world_renderer(camera* cam, char* atlas_path) {
    r_cam_ref = cam;
    
    texture atlas = t_init(atlas_path, 0);
    
    camera_cache cam_cache = {
        .x = cam->position[0],
        .z = cam->position[2],
    };

    VAO vao = create_vao();
    bind_vao(vao);
    VBO cube_vbo = create_vbo(GL_STATIC_DRAW);
    VBO instance_vbo = create_vbo(GL_STATIC_DRAW);

    shader vertex_shader = create_shader("res/shaders/world.vert", GL_VERTEX_SHADER);
    shader fragment_shader = create_shader("res/shaders/world.frag", GL_FRAGMENT_SHADER);
    shader_program program = create_program(vertex_shader, fragment_shader);
    delete_shader(vertex_shader);
    delete_shader(fragment_shader);

    w_init();
    m_init(cam);

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = cam_cache.x - CHUNK_RENDER_DISTANCE + i;
            int z = cam_cache.z - CHUNK_RENDER_DISTANCE + j;
            get_chunk_mesh(x, z);
            load_chunk();
        }
    }

    world_renderer wr = {
        .cam = cam,
        .cam_cache = cam_cache,
        .program = program,
        .atlas = atlas,
        .vao = vao,
        .cube_vbo = cube_vbo,
        .instance_vbo = instance_vbo
    };

    return wr;
}

void destroy_world_renderer(world_renderer wr) {
    delete_vao(wr.vao);
    delete_vbo(wr.cube_vbo);
    delete_vbo(wr.instance_vbo);
    delete_program(wr.program);
    t_cleanup(&(wr.atlas));
    m_cleanup();
}

void send_view_matrix(world_renderer* wr) {
    mat4 view;
    get_view_matrix(*(wr->cam), &view);
    uint view_loc = glGetUniformLocation(wr->program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
}

void send_proj_matrix(world_renderer* wr) {
    mat4 proj;
    get_projection_matrix(&proj, 45.0f, 800.0f / 600.0f, 0.1f, RENDER_DISTANCE);
    uint proj_loc = glGetUniformLocation(wr->program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);
}

void send_atlas(world_renderer* wr) {
    glActiveTexture(GL_TEXTURE0 + wr->atlas.tex_index);
    glBindTexture(GL_TEXTURE_2D, wr->atlas.id);
    uint atlas_loc = glGetUniformLocation(wr->program.id, "atlas");
    glUniform1i(atlas_loc, wr->atlas.tex_index);

    uint atlas_size_loc = glGetUniformLocation(wr->program.id, "atlasSize");
    glUniform1f(atlas_size_loc, (float)ATLAS_SIZE);

    uint tex_size_loc = glGetUniformLocation(wr->program.id, "texSize");
    glUniform1f(tex_size_loc, (float)TEXTURE_SIZE);
}

void send_cube_vbo(world_renderer* wr) {
    float faceVertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    bind_vao(wr->vao);
    buffer_data(wr->cube_vbo, GL_STATIC_DRAW, faceVertices, 6 * 3 * sizeof(float));
    f_add_attrib(&(wr->cube_vbo), 0, 3, 0, 3 * sizeof(float)); // position
    use_vbo(wr->cube_vbo);
}

void render_sides(world_renderer* wr, int* side_data, int num_sides) {
    bind_vao(wr->vao);
    buffer_data(wr->instance_vbo, GL_STATIC_DRAW, side_data, num_sides * VBO_WIDTH * sizeof(int));
    i_add_attrib(&(wr->instance_vbo), 1, 3, 0 * sizeof(int), VBO_WIDTH * sizeof(int)); // position
    i_add_attrib(&(wr->instance_vbo), 2, 2, 3 * sizeof(int), VBO_WIDTH * sizeof(int)); // atlas coords
    i_add_attrib(&(wr->instance_vbo), 3, 1, 5 * sizeof(int), VBO_WIDTH * sizeof(int)); // side
    use_vbo(wr->instance_vbo);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
        
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, num_sides);
}

void render_world(world_renderer* wr) {
    use_program(wr->program);
    bind_vao(wr->vao);

    send_view_matrix(wr);
    send_proj_matrix(wr);
    send_atlas(wr);

    send_cube_vbo(wr);
    glClear(GL_DEPTH_BUFFER_BIT);

    int x = wr->cam->position[0];
    int z = wr->cam->position[2];
    int player_chunk_x = CAMERA_POS_TO_CHUNK_POS(x);
    int player_chunk_z = CAMERA_POS_TO_CHUNK_POS(z);

    int movedBlocks = ((int)x == (int)wr->cam_cache.x && (int)z == (int)wr->cam_cache.z) ? 0 : 1;
    if (movedBlocks) {
        wr->cam_cache.x = x;
        wr->cam_cache.z = z;
    }

    chunk_mesh** packet = NULL;
    int num_packets = 0;

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = player_chunk_x - CHUNK_RENDER_DISTANCE + i;
            int z = player_chunk_z - CHUNK_RENDER_DISTANCE + j;

            if (sqrt(pow(x - player_chunk_x, 2) + pow(z - player_chunk_z, 2)) > CHUNK_RENDER_DISTANCE) {
                continue;
            }

            chunk_mesh* mesh = get_chunk_mesh(x, z);

            if (mesh == NULL) {
                continue;
            }

            packet = realloc(packet, (num_packets + 1) * sizeof(chunk_mesh*));
            packet[num_packets] = mesh;
            num_packets++;

            if (x >= player_chunk_x - 1 
                && x <= player_chunk_x + 1 
                && z >= player_chunk_z - 1
                && z <= player_chunk_z + 1
                && movedBlocks) {
                queue_chunk_for_sorting(mesh);
            }
        }
    }

    sort_chunk();
    load_chunk();

    // may want to preserve packet between frames to prevent the need for continuous quicksorting, would allow us to only update
    // if the camera has moved between chunk boundaries
    quicksort(packet, num_packets, sizeof(chunk_mesh*), chunk_distance_to_camera);

    for (int i = 0; i < num_packets; i++) {
        if (packet[i] == NULL) {
            continue;
        }
        render_sides(wr,
            packet[i]->opaque_data,
            packet[i]->num_opaque_sides);
        render_sides(wr,
            packet[i]->transparent_data,
            packet[i]->num_transparent_sides);
    }

    free(packet);
    stop_program();
}
