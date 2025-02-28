#version 330 core

#define FOG_DIST 40000000.0f

in vec2 texCoord;
in vec2 atlasCoord;
in float dist;
out vec4 FragColor;

uniform sampler2D atlas;

void main() {
    vec2 coord = vec2((texCoord.x / 32.0) + atlasCoord.x, (texCoord.y / 32.0) + atlasCoord.y);

    float u = dist / FOG_DIST;
    FragColor = mix(texture(atlas, coord), vec4(1.0, 1.0, 1.0, 1.0), u);
}