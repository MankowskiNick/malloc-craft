#version 330 core

in vec2 TexCoord;
in vec2 AtlasCoord;

uniform sampler2D atlas;
uniform float atlasSize;

out vec4 FragColor;

void main() {
    // Calculate proper atlas texture coordinates like in world.frag
    vec2 coord = (AtlasCoord + TexCoord) / atlasSize;
    vec4 texColor = texture(atlas, coord);
    
    // Discard transparent pixels
    if (texColor.a < 0.1) {
        discard;
    }
    
    FragColor = texColor;
}
