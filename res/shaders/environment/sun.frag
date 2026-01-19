#version 330 core

out vec4 FragColor;

uniform float sunIntensity;
uniform vec3 sunColor;

void main() {
    FragColor = vec4(sunIntensity * sunColor, 1.0);
}