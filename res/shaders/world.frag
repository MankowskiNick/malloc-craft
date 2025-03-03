#version 330 core

in vec2 texCoord; // coordinate of the quad [0, 1]
in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]
in float dist; // distance from the camera
in float underwater;
in float y;

out vec4 FragColor;

uniform sampler2D atlas;
uniform sampler2D caustic;
uniform float atlasSize;
uniform float fogDistance;
uniform float waterDistance;
uniform float waterLevel;
uniform float waterOffset;
uniform float time;

vec4 calculateChromaticAberration(vec2 coord) {
    vec4 causticColor = texture(caustic, coord);
    causticColor.r = texture(caustic, vec2(coord.x + 0.01, coord.y)).r;
    causticColor.b = texture(caustic, vec2(coord.x - 0.01, coord.y)).b;
    causticColor.g = texture(caustic, vec2(coord.x, coord.y + 0.01)).g;
    return causticColor;
}

void main() {
    vec2 coord = (atlasCoord + texCoord) / atlasSize;

    if (underwater == 1.0 && y <= waterLevel + 1.0 - waterOffset) {
        vec2 causticCoord = vec2(texCoord.x + sin(time * 0.08), texCoord.y + cos(time * 0.08));
        
        // chromatic aberration
        vec4 causticColor = calculateChromaticAberration(causticCoord);

        FragColor = mix(
            causticColor + texture(atlas, coord), 
            vec4(0.0, 0.0, 1.0, 1.0), 
            clamp(dist / waterDistance, 0.3, 0.8));
    }
    else {
        FragColor = mix(texture(atlas, coord), vec4(1.0, 1.0, 1.0, 1.0), clamp(dist / fogDistance, 0.0, 1.0));
    }
}