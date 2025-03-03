#version 330 core

out vec4 FragColor;

uniform float intensity;
uniform vec3 color;

void main() {
    FragColor = vec4(intensity * color, 1.0);
}