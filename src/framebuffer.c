#include <framebuffer.h>

void framebuffer_cleanup(framebuffer* map) {
    glDeleteFramebuffers(1, &map->fbo);
    glDeleteTextures(1, &map->texture);
    glDeleteProgram(map->program.id);
    glDeleteVertexArrays(1, &map->vao.id);
    glDeleteBuffers(1, &map->cube_vbo.id);
    glDeleteBuffers(1, &map->instance_vbo.id);
}