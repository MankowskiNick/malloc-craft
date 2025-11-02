#include <render.h>

#include <block_renderer.h>
#include <blockbench_renderer.h>
#include <liquid_renderer.h>
#include <foliage_renderer.h>
#include <skybox.h>
#include <glad/glad.h>
#include <chunk_mesh.h>
#include <mesh.h>
#include <sort.h>
#include <world.h>
#include <util.h>
#include <settings.h>
#include <world_mesh.h>
#include <fbo.h>
#include <shadow_map.h>
#include <reflection_map.h>
#include <assert.h>'
#include <GLFW/glfw3.h>

renderer create_renderer(camera* camera) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (WIREFRAME) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    w_init();
    m_init(camera);

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = (int)(camera->position[0]) - CHUNK_RENDER_DISTANCE + i;
            int z = (int)(camera->position[2]) - CHUNK_RENDER_DISTANCE + j;
            get_chunk_mesh(x, z);
            load_chunk();
        }
    }

    block_renderer wr = create_block_renderer(camera, ATLAS_PATH, BUMP_PATH, CAUSTIC_PATH);
    block_renderer lr = create_liquid_renderer(camera, ATLAS_PATH, BUMP_PATH, CAUSTIC_PATH);
    block_renderer fr = create_foliage_renderer(camera, ATLAS_PATH, BUMP_PATH, CAUSTIC_PATH);
    blockbench_renderer br = create_blockbench_renderer(camera, ATLAS_PATH, BUMP_PATH);

    skybox sky = create_skybox(camera);
    sun s = create_sun(camera, 1.0f, 1.0f, 1.0f);
    FBO shadow_map = create_shadow_map(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
    FBO reflection_map = create_reflection_map(WIDTH, HEIGHT);
    FBO main_framebuffer = create_main_framebuffer(WIDTH, HEIGHT);

    renderer r = {
        .wr = wr,
        .lr = lr,
        .fr = fr,
        .br = br,
        .sky = sky,
        .s = s,
        .cam_cache = {
            .x = camera->position[0],
            .z = camera->position[2],
        },
        .cam = camera,
        .shadow_map = shadow_map,
        .reflection_map = reflection_map,
        .main_framebuffer = main_framebuffer,
    };

    return r;
}

void destroy_renderer(renderer* r) {
    destroy_block_renderer(r->wr);
    destroy_block_renderer(r->lr);
    destroy_blockbench_renderer(r->br);
    skybox_cleanup(&(r->sky));
}

void render(renderer* r, world_mesh* packet, int num_packets) {
    assert(r != NULL && "Renderer is NULL\n");
    assert(packet != NULL && "World mesh is NULL\n");

    if (num_packets <= 0) {
        return; // No packets to render
    }

    render_shadow_map(&(r->shadow_map), &(r->s), packet);
    render_reflection_map(&(r->reflection_map), r->cam, (float)WORLDGEN_WATER_LEVEL, packet);
    
    // Bind main framebuffer for scene rendering
    glBindFramebuffer(GL_FRAMEBUFFER, r->main_framebuffer.fbo);
    glViewport(0, 0, r->main_framebuffer.width, r->main_framebuffer.height);
    
    glActiveTexture(GL_TEXTURE0 + SHADOW_MAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, r->shadow_map.texture);
    
    glActiveTexture(GL_TEXTURE0 + REFLECTION_MAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, r->reflection_map.texture);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    render_skybox(&(r->sky));
    glEnable(GL_DEPTH_TEST);

    render_sun(&(r->s));

    glClear(GL_DEPTH_BUFFER_BIT);
    
    render_solids(&(r->wr), &(r->s), &(r->shadow_map), packet);
    render_liquids(&(r->lr), &(r->s), &(r->shadow_map), &(r->reflection_map), packet);
    render_foliage(&(r->fr), &(r->s), &(r->shadow_map), packet);
    render_transparent(&(r->wr), &(r->s), &(r->shadow_map), packet);
    render_blockbench_models(&(r->br), &(r->s), &(r->shadow_map), packet);

    // Unbind framebuffer - render to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, WIDTH, HEIGHT);
}

char* buffer_screen_to_char(renderer* r) {
    int width = r->main_framebuffer.width;
    int height = r->main_framebuffer.height;

    if (width <= 0 || height <= 0) {
        return NULL;
    }

    size_t pixel_count = (size_t)width * (size_t)height;
    size_t output_size = pixel_count * 3; // RGB
    unsigned char* output_buffer = (unsigned char*)malloc(output_size);
    if (!output_buffer) {
        printf("Failed to allocate memory for pixel buffer\n");
        return NULL;
    }

    // Bind the framebuffer and read from its texture
    glBindFramebuffer(GL_FRAMEBUFFER, r->main_framebuffer.fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, output_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Estimate worst-case terminal buffer size:
    // Each half-block may emit up to ~64 chars (escape sequences + digits).
    // Number of half-blocks = width * ceil(height / 2)
    int rows = (height + 1) / 2;
    size_t estimated = (size_t)width * (size_t)rows * 64 + (size_t)rows * 16 + 1;

    char* terminal_buffer = (char*)malloc(estimated);
    if (!terminal_buffer) {
        printf("Failed to allocate memory for terminal buffer\n");
        free(output_buffer);
        return NULL;
    }

    char* write_ptr = terminal_buffer;
    size_t remaining = estimated;

    for (int y = height - 1; y >= 1; y -= 2) {
        for (int x = 0; x < width; x++) {
            int idx_top = (y * width + x) * 3;
            unsigned int r_top = output_buffer[idx_top + 0];
            unsigned int g_top = output_buffer[idx_top + 1];
            unsigned int b_top = output_buffer[idx_top + 2];

            int idx_bottom = ((y - 1) * width + x) * 3;
            unsigned int r_bottom = output_buffer[idx_bottom + 0];
            unsigned int g_bottom = output_buffer[idx_bottom + 1];
            unsigned int b_bottom = output_buffer[idx_bottom + 2];

            int written = snprintf(write_ptr, remaining,
                                   "\033[38;2;%u;%u;%um\033[48;2;%u;%u;%um▀",
                                   r_top, g_top, b_top,
                                   r_bottom, g_bottom, b_bottom);
            if (written < 0) {
                // encoding error
                free(output_buffer);
                free(terminal_buffer);
                return NULL;
            }

            if ((size_t)written >= remaining) {
                // Need more space: reallocate (grow buffer)
                size_t used = write_ptr - terminal_buffer;
                estimated = estimated * 2 + (size_t)written;
                char* newbuf = (char*)realloc(terminal_buffer, estimated);
                if (!newbuf) {
                    free(output_buffer);
                    free(terminal_buffer);
                    return NULL;
                }
                terminal_buffer = newbuf;
                write_ptr = terminal_buffer + used;
                remaining = estimated - used;

                // write again
                written = snprintf(write_ptr, remaining,
                                   "\033[38;2;%u;%u;%um\033[48;2;%u;%u;%um▀",
                                   r_top, g_top, b_top,
                                   r_bottom, g_bottom, b_bottom);
                if (written < 0 || (size_t)written >= remaining) {
                    free(output_buffer);
                    free(terminal_buffer);
                    return NULL;
                }
            }

            write_ptr += written;
            remaining -= (size_t)written;
        }

        int w = snprintf(write_ptr, remaining, "\033[0m\n");
        if (w < 0) {
            free(output_buffer);
            free(terminal_buffer);
            return NULL;
        }
        if ((size_t)w >= remaining) {
            size_t used = write_ptr - terminal_buffer;
            estimated = estimated * 2 + (size_t)w;
            char* newbuf = (char*)realloc(terminal_buffer, estimated);
            if (!newbuf) {
                free(output_buffer);
                free(terminal_buffer);
                return NULL;
            }
            terminal_buffer = newbuf;
            write_ptr = terminal_buffer + used;
            remaining = estimated - used;

            w = snprintf(write_ptr, remaining, "\033[0m\n");
            if (w < 0 || (size_t)w >= remaining) {
                free(output_buffer);
                free(terminal_buffer);
                return NULL;
            }
        }

        write_ptr += w;
        remaining -= (size_t)w;
    }

    *write_ptr = '\0'; // Null-terminate the string

    free(output_buffer);
    return terminal_buffer;
}
