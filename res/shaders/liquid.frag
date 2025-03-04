
#version 330 core

in vec2 texCoord; // coordinate of the quad [0, 1]
in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]
in float underwater; // 1 if the fragment is underwater, 0 otherwise
in float y;
in vec3 normal;

out vec4 FragColor;

// texture info
uniform sampler2D atlas;
uniform float atlasSize;

// fog
uniform float fogDistance;

// water values
uniform float waterLevel;
uniform float waterOffset;

// sun
uniform vec3 sunPos;
uniform vec3 sunColor;
uniform float sunIntensity;

uniform vec3 ambientLight;

void main() {
    // vec3 normal = vec3(0.0, 1.0, 0.0);

    vec3 sun = normalize(sunPos);
    float intensity = max(dot(normal, sun), 0.0) * sunIntensity;
    vec3 lightIntensity = ambientLight + (sunColor * intensity);
    
    vec2 coord = (atlasCoord + texCoord) / atlasSize;
    
    if (y == waterLevel + 1.0) {
        vec4 baseColor = texture(atlas, coord);
        FragColor = vec4(baseColor.rgb * lightIntensity, baseColor.a);
    }
    else {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}