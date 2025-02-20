#version 330 core

layout (location=0) in vec3 aPos;
layout (location=1) in vec2 aTexCoord;
layout (location=2) in vec2 aAtlasCoord;

out vec2 texCoord;
out vec2 atlasCoord;

uniform mat4 view;
uniform mat4 proj;

void main() {
    atlasCoord = aAtlasCoord;
    texCoord = aTexCoord;
    gl_Position = proj * view * vec4(aPos, 1.0);
}