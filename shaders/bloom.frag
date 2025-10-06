#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;

layout(push_constant) uniform BloomParams {
    float threshold;
    float intensity;
    float radius;
} params;

// Enhanced multi-sample bloom with better glow distribution
vec3 bloom(sampler2D tex, vec2 uv, float radius) {
    vec2 texelSize = 1.0 / textureSize(tex, 0);
    vec3 result = vec3(0.0);
    float totalWeight = 0.0;
    
    // Multi-pass sampling with varying radii for better glow spread
    const int passes = 3;
    for(int pass = 0; pass < passes; pass++) {
        float currentRadius = radius * (1.0 + float(pass) * 0.5);
        float stepSize = max(1.0, currentRadius / 4.0);
        
        for(float x = -currentRadius; x <= currentRadius; x += stepSize) {
            for(float y = -currentRadius; y <= currentRadius; y += stepSize) {
                vec2 offset = vec2(x, y) * texelSize;
                vec3 sampleColor = texture(tex, uv + offset).rgb;
                
                // Enhanced brightness extraction with softer falloff
                float brightness = dot(sampleColor, vec3(0.2126, 0.7152, 0.0722));
                float brightnessFactor = brightness / (params.threshold + 0.001);
                float contribution = smoothstep(0.8, 1.5, brightnessFactor);
                
                // Distance-based weight with smoother falloff
                float dist = length(vec2(x, y)) / currentRadius;
                float weight = exp(-dist * dist * 2.0) * (1.0 / float(pass + 1));
                
                result += sampleColor * contribution * weight;
                totalWeight += weight * contribution;
            }
        }
    }
    
    if(totalWeight > 0.0) {
        result /= totalWeight;
    }
    
    // Boost the bloom intensity with non-linear scaling
    result = pow(result, vec3(0.9)) * params.intensity * 2.5;
    
    return result;
}

void main() {
    vec3 sceneColor = texture(sceneTexture, inUV).rgb;
    vec3 bloomColor = bloom(sceneTexture, inUV, params.radius);
    
    // Enhanced additive blending with color preservation
    vec3 finalColor = sceneColor + bloomColor;
    
    // Improved ACES tonemapping with better highlight preservation
    vec3 a = finalColor * 2.51;
    vec3 b = finalColor * 0.03 + 0.59;
    vec3 c = finalColor * 2.43 + 0.14;
    finalColor = clamp((a) / (b + c), 0.0, 1.0);
    
    // Subtle contrast boost for better visual pop
    finalColor = pow(finalColor, vec3(0.95));
    
    outColor = vec4(finalColor, 1.0);
}
