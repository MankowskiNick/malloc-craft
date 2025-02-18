#version 330 core

in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D atlas;

void main() {

    vec2 coord = vec2(texCoord.x / 32.0, texCoord.y / 32.0);
    FragColor = texture(atlas, coord);
}