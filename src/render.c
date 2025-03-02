#include <render.h>

#include <world_renderer.h>
#include <skybox.h>

#include <settings.h>

world_renderer wr;
skybox sky;

void r_init(shader_program* program, camera* camera) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    #ifdef WIREFRAME
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    #endif

    glViewport(0, 0, WIDTH, HEIGHT);

    wr = create_world_renderer(camera, "res/atlas.png");
    sky = create_skybox();
}

void r_cleanup() {
    destroy_world_renderer(wr);
    skybox_cleanup(&sky);
}

void render(camera cam, shader_program program) {
    
    render_skybox(&sky, &cam);
    render_world(&wr);
}