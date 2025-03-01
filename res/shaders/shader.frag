#version 330 core

in vec2 texCoord; // coordinate of the quad [0, 1]
in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]

out vec4 FragColor;

uniform sampler2D atlas;
uniform float atlasSize;
uniform float textureSize;

void main() {
    vec2 offset = vec2(atlasCoord.x / atlasSize, atlasCoord.y / atlasSize);
    vec2 coord = vec2(texCoord.x / atlasSize + offset.x, texCoord.y / atlasSize + offset.y);
    FragColor = texture(atlas, coord);
    // FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}