#version 330 core

// in vec2 texCoord; // coordinate of the quad [0, 1]
// in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]

out vec4 FragColor;

uniform sampler2D atlas;

void main() {
    // vec2 finalCoord = vec2(atlasCoord.x / 32.0, atlasCoord.y / 32.0) + texCoord;
    // FragColor = texture(atlas, finalCoord);
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}