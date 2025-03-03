#version 330 core

in vec2 texCoord; // coordinate of the quad [0, 1]
in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]
in float dist; // distance from the camera

out vec4 FragColor;

uniform sampler2D atlas;
uniform float atlasSize;
uniform float fogDistance;

void main() {
    float u = dist / fogDistance;
    u = clamp(u, 0.0, 1.0);
    vec2 coord = (atlasCoord + texCoord) / atlasSize;
    FragColor = mix(texture(atlas, coord), vec4(1.0, 1.0, 1.0, 1.0), u);
}