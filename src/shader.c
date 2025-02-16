#include <shader.h>
#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>

char* read_file(const char* path) {
    FILE* file = fopen(path, "r");

    if (!file) {
        printf("Failed to open file %s\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);

    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* contents = (char*)malloc(length + 1);
    if (!contents) {
        printf("Failed to allocate memory\n");
        fclose(file);
        return NULL;
    }

    fread(contents, 1, length, file);
    contents[length] = '\0';
    fclose(file);
    return contents;
}

shader* create_shader(const char* source, uint type) {
    shader* shader = malloc(sizeof(shader));
    shader->id = glCreateShader(type);
    shader->type = type;

    const char* shader_source = read_file(source);

    // compile shader
    glShaderSource(shader->id, 1, &shader_source, NULL);
    glCompileShader(shader->id);

    // Check for shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(shader->id, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader->id, 512, NULL, infoLog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    }

    return shader;
}

void delete_shader(shader* shader) {
    glDeleteShader(shader->id);
    free(shader);
}

shader_program* create_program(shader* vert_shader, shader* frag_shader) {
    shader_program* program = malloc(sizeof(shader_program));
    program->id = glCreateProgram();
    glAttachShader(program->id, vert_shader->id);
    glAttachShader(program->id, frag_shader->id);
    glLinkProgram(program->id);

    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(program->id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program->id, 512, NULL, infoLog);
        printf("ERROR::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }

    return program;
}

void delete_program(shader_program* program) {
    glDeleteProgram(program->id);
    free(program);
}

void use_program(shader_program* program) {
    glUseProgram(program->id);
}