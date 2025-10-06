#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;

layout(push_constant) uniform BloomParams {
    float threshold;
    float intensity;
    float radius;
} params;

// Simple kawase blur for bloom
vec3 bloom(sampler2D tex, vec2 uv, float radius) {
    vec2 texelSize = 1.0 / textureSize(tex, 0);
    vec3 result = vec3(0.0);
    float totalWeight = 0.0;
    
    // Sample in a box pattern
    for(float x = -radius; x <= radius; x += 1.0) {
        for(float y = -radius; y <= radius; y += 1.0) {
            vec2 offset = vec2(x, y) * texelSize;
            vec3 sampleColor = texture(tex, uv + offset).rgb;
            
            // Extract bright pixels
            float brightness = dot(sampleColor, vec3(0.2126, 0.7152, 0.0722));
            float contribution = max(0.0, brightness - params.threshold);
            
            float weight = 1.0 / (1.0 + length(vec2(x, y)));
            result += sampleColor * contribution * weight;
            totalWeight += weight * contribution;
        }
    }
    
    if(totalWeight > 0.0) {
        result /= totalWeight;
    }
    
    return result * params.intensity;
}

void main() {
    vec3 sceneColor = texture(sceneTexture, inUV).rgb;
    vec3 bloomColor = bloom(sceneTexture, inUV, params.radius);
    
    // Additive blending
    vec3 finalColor = sceneColor + bloomColor;
    
    // ACES tonemapping
    vec3 a = finalColor * 2.51;
    vec3 b = finalColor * 0.03 + 0.59;
    vec3 c = finalColor * 2.43 + 0.14;
    finalColor = clamp((a) / (b + c), 0.0, 1.0);
    
    outColor = vec4(finalColor, 1.0);
}
