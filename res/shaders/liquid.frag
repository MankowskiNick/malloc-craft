
#version 330 core

in vec2 texCoord; // coordinate of the quad [0, 1]
in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]
in float underwater; // 1 if the fragment is underwater, 0 otherwise
in float y;

out vec4 FragColor;

uniform sampler2D atlas;
uniform float atlasSize;
uniform float fogDistance;
uniform float waterLevel;
uniform float waterOffset;

void main() {
    vec2 coord = (atlasCoord + texCoord) / atlasSize;
    if (y < waterLevel + 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
    else {
        FragColor = texture(atlas, coord);
    }
}