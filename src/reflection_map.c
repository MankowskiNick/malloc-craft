#include <reflection_map.h>

#include <glad/glad.h>
#include <vao.h>
#include <vbo.h>
#include <block.h>
#include <fbo.h>

FBO create_reflection_map(uint width, uint height) {

    // create fbo
    uint fbo;
    glGenFramebuffers(1, &fbo);

    // create program
    shader vert_shader = create_shader("res/shaders/reflection.vert", GL_VERTEX_SHADER);
    shader frag_shader = create_shader("res/shaders/reflection.frag", GL_FRAGMENT_SHADER);
    shader_program program = create_program(vert_shader, frag_shader);
    delete_shader(vert_shader);
    delete_shader(frag_shader);

    // create color texture
    uint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // create depth renderbuffer
    uint depthBuffer;
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    // create vao
    VAO vao = create_vao();
    bind_vao(vao);

    // create cube vbo
    VBO cube_vbo = create_vbo(GL_STATIC_DRAW);
    VBO instance_vbo = create_vbo(GL_STATIC_DRAW);

    // bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    // attach color texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    
    // attach depth renderbuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

    // enable writes to color buffer
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // check framebuffer
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Error: Reflection framebuffer is not complete\n");
    }

    // unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    FBO map = {
        .width = width,
        .height = height,
        .fbo = fbo,
        .texture = texture,
        .program = program,
        .vao = vao,
        .cube_vbo = cube_vbo,
        .instance_vbo = instance_vbo
    };

    return map;
}

void render_reflection_map(FBO* map, camera* cam, float water_level, world_mesh* packet) {
    // Save current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glBindFramebuffer(GL_FRAMEBUFFER, map->fbo);
    glViewport(0, 0, map->width, map->height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    use_program(map->program);

    send_reflection_matrices(&(map->program), cam, water_level);

    // send cube vbo
    send_cube_vbo(map->vao, map->cube_vbo);

    if (packet != NULL) {
        render_depth(map, packet->opaque_data, packet->num_opaque_sides);
        render_depth(map, packet->transparent_data, packet->num_transparent_sides);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Restore the viewport to what it was before
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}
