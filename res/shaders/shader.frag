#version 330 core

in vec2 texCoord; // coordinate of the quad [0, 1]
in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]

out vec4 FragColor;

uniform sampler2D atlas;
uniform float atlasSize;
uniform float textureSize;

void main() {
    
    vec2 coord = (atlasCoord + texCoord) / atlasSize;
    FragColor = texture(atlas, coord);
}