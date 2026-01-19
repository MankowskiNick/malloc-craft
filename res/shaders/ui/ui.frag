#version 330 core

in vec2 TexCoord;

uniform sampler2D atlas;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(atlas, TexCoord);

    // Discard transparent pixels
    if (texColor.a < 0.1) {
        discard;
    }

    FragColor = texColor;
}
