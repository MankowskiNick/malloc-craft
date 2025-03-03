#version 330 core

in vec2 texCoord; // coordinate of the quad [0, 1]
in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]
in float dist; // distance from the camera
in float underwater;
in float y;
in vec3 normal;

out vec4 FragColor;

// textures
uniform float atlasSize;
uniform sampler2D atlas;
uniform sampler2D caustic;

// fog
uniform float fogDistance;

// water values
uniform float waterDistance;
uniform float waterLevel;
uniform float waterOffset;
uniform float time;

// sun
uniform vec3 sunPos;
uniform vec3 sunColor;
uniform float sunIntensity;

// ambient lighting
uniform vec3 ambientLight;


vec4 calculateChromaticAberration(vec2 coord) {
    vec4 causticColor = texture(caustic, coord);
    causticColor.r = texture(caustic, vec2(coord.x + 0.01, coord.y)).r;
    causticColor.b = texture(caustic, vec2(coord.x - 0.01, coord.y)).b;
    causticColor.g = texture(caustic, vec2(coord.x, coord.y + 0.01)).g;
    return causticColor;
}

vec4 getCausticColor(vec2 coord) {
    vec2 causticCoord = vec2(coord.x + sin(time * 0.08), coord.y + cos(time * 0.08));

    return calculateChromaticAberration(causticCoord);
}

vec4 getUnderwaterColor(vec2 coord) {
    vec4 causticColor = getCausticColor(texCoord);
    return mix(
            causticColor + texture(atlas, coord), 
            vec4(0.0, 0.0, 1.0, 1.0), 
            clamp(dist / waterDistance, 0.3, 0.8));
}

vec4 getStandardColor(vec2 coord) {
    return mix(texture(atlas, coord), 
        vec4(1.0, 1.0, 1.0, 1.0), 
        clamp(dist / fogDistance, 0.0, 1.0));
}

void main() {
    vec2 coord = (atlasCoord + texCoord) / atlasSize;

    vec3 sun = normalize(sunPos);
    float intensity = max(dot(normal, sun), 0.0) * sunIntensity;
    vec3 lightIntensity = ambientLight + (sunColor * intensity);
    
    if (underwater == 1.0 && y <= waterLevel + 1.0 - waterOffset) {
        vec4 baseColor = getUnderwaterColor(coord); 

        
        FragColor = vec4(baseColor.rgb * lightIntensity, baseColor.a);
    }
    else {
        vec4 baseColor = getStandardColor(coord);
        FragColor = vec4(baseColor.rgb * lightIntensity, baseColor.a);
    }
}