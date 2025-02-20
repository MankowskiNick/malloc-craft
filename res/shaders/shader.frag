#version 330 core

in vec2 texCoord;
in vec2 atlasCoord;
out vec4 FragColor;

uniform sampler2D atlas;

void main() {

    vec2 coord = vec2((texCoord.x / 32.0) + atlasCoord.x, (texCoord.y / 32.0) + atlasCoord.y);
    FragColor = texture(atlas, coord);
}