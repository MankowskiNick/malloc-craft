#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 view;
uniform mat4 proj;
uniform vec3 sunPos;

void main() {
    gl_Position = proj * view * vec4(position + sunPos, 1.0);
}