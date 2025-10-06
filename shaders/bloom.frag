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
    const int passes = 2;
    for(int pass = 0; pass < passes; pass++) {
        float currentRadius = radius * (1.0 + float(pass) * 0.4);
        float stepSize = max(1.0, currentRadius / 3.0);
        
        for(float x = -currentRadius; x <= currentRadius; x += stepSize) {
            for(float y = -currentRadius; y <= currentRadius; y += stepSize) {
                vec2 offset = vec2(x, y) * texelSize;
                vec3 sampleColor = texture(tex, uv + offset).rgb;
                
                // More aggressive brightness extraction - only very bright areas
                float brightness = dot(sampleColor, vec3(0.2126, 0.7152, 0.0722));
                float brightnessFactor = brightness / (params.threshold + 0.001);
                // Stricter threshold - only affects truly bright areas
                float contribution = smoothstep(1.2, 2.0, brightnessFactor);
                
                // Distance-based weight with smoother falloff
                float dist = length(vec2(x, y)) / currentRadius;
                float weight = exp(-dist * dist * 2.5) * (1.0 / float(pass + 1));
                
                result += sampleColor * contribution * weight;
                totalWeight += weight * contribution;
            }
        }
    }
    
    if(totalWeight > 0.0) {
        result /= totalWeight;
    }
    
    // More moderate intensity boost
    result = pow(result, vec3(0.95)) * params.intensity * 1.8;
    
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
