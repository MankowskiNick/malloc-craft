#include <texture.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>

atlas texture_atlas;

void t_init() {
    int width, height, num_channels;
    // flip the image vertically
    stbi_set_flip_vertically_on_load(1);
    unsigned char *data = stbi_load("res/atlas.png", &width, &height, &num_channels, 0); 

    if (!data) {
        printf("ERROR::TEXTURE::FAILED_TO_LOAD_TEXTURE\n");
        return;
    }

    uint texture_id;
    glGenTextures(1, &texture_id);

    glBindTexture(GL_TEXTURE_2D, texture_id);

    #ifdef USE_MIPMAP
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Use mipmapping for minification and linear filtering for magnification
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        // Generate mipmaps for the texture
        glGenerateMipmap(GL_TEXTURE_2D);
    #else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    #endif

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    texture_atlas.id = texture_id;
    texture_atlas.width = width;
    texture_atlas.height = height;
    texture_atlas.num_channels = num_channels;

    stbi_image_free(data);
}

void t_cleanup() {
    glDeleteTextures(1, &texture_atlas.id);
}