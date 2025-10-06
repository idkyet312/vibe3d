#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneImage;
layout(set = 0, binding = 1) uniform sampler2D bloomImage;

layout(push_constant) uniform CompositeParams {
    float bloomStrength;
    float exposure;
} params;

// ACES tonemapping
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 scene = texture(sceneImage, inUV).rgb;
    vec3 bloom = texture(bloomImage, inUV).rgb;
    
    // Combine scene and bloom
    vec3 color = scene + bloom * params.bloomStrength;
    
    // Apply exposure
    color = vec3(1.0) - exp(-color * params.exposure);
    
    // Apply ACES tonemapping
    color = ACESFilm(color);
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    outColor = vec4(color, 1.0);
}
