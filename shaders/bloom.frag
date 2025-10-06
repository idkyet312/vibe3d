#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;

layout(push_constant) uniform BloomParams {
    float threshold;
    float intensity;
    float radius;
} params;

// Extract bright pixels with smooth falloff (like Unity/Unreal bright pass)
vec3 extractBrightAreas(vec3 color) {
    float brightness = max(color.r, max(color.g, color.b));
    
    // Smooth threshold transition instead of hard cutoff
    float softThreshold = smoothstep(params.threshold * 0.5, params.threshold, brightness);
    
    // Preserve color while extracting bright areas
    return color * softThreshold * softThreshold;
}

// 13-tap blur for smoother, more realistic glow
vec3 gaussianBlur(sampler2D tex, vec2 uv, float radius) {
    vec2 texelSize = 1.0 / textureSize(tex, 0);
    vec3 result = vec3(0.0);
    float totalWeight = 0.0;
    
    // Gaussian-style sampling pattern with proper weights
    const int samples = 9;
    const float offsets[9] = float[](0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
    const float weights[9] = float[](0.16, 0.15, 0.13, 0.11, 0.09, 0.07, 0.05, 0.03, 0.01);
    
    for(int i = 0; i < samples; i++) {
        float offset = offsets[i] * radius;
        float weight = weights[i];
        
        // Sample in 4 directions (up, down, left, right)
        result += texture(tex, uv + vec2(offset, 0.0) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(-offset, 0.0) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(0.0, offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(0.0, -offset) * texelSize).rgb * weight;
        
        totalWeight += weight * 4.0;
    }
    
    // Diagonal samples for rounder glow
    for(int i = 1; i < 5; i++) {
        float offset = float(i) * radius * 0.7;
        float weight = weights[i] * 0.5;
        
        result += texture(tex, uv + vec2(offset, offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(-offset, offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(offset, -offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(-offset, -offset) * texelSize).rgb * weight;
        
        totalWeight += weight * 4.0;
    }
    
    return result / totalWeight;
}

// Multi-pass downsampled bloom for performance and quality
vec3 bloom(sampler2D tex, vec2 uv) {
    vec3 color = texture(tex, uv).rgb;
    
    // Extract bright areas with soft threshold
    vec3 bright = extractBrightAreas(color);
    
    // Multiple blur passes at different scales for realistic glow spread
    vec3 bloom1 = gaussianBlur(tex, uv, params.radius * 0.5);  // Tight glow
    vec3 bloom2 = gaussianBlur(tex, uv, params.radius * 1.0);  // Medium glow
    vec3 bloom3 = gaussianBlur(tex, uv, params.radius * 2.0);  // Wide glow
    
    // Combine different blur sizes for natural falloff
    vec3 combinedBloom = bloom1 * 0.5 + bloom2 * 0.3 + bloom3 * 0.2;
    
    // Extract bright areas from combined bloom
    return extractBrightAreas(combinedBloom) * 2.0;
}

void main() {
    vec3 sceneColor = texture(sceneTexture, inUV).rgb;
    vec3 bloomColor = bloom(sceneTexture, inUV);
    
    // Additive blending with intensity control
    vec3 finalColor = sceneColor + bloomColor * params.intensity;
    
    // ACES Filmic tonemapping (keeps colors vibrant in HDR)
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    vec3 mapped = (finalColor * (a * finalColor + b)) / (finalColor * (c * finalColor + d) + e);
    finalColor = clamp(mapped, 0.0, 1.0);
    
    // Slight saturation boost for more vibrant glow
    float luminance = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
    finalColor = mix(vec3(luminance), finalColor, 1.05);
    
    outColor = vec4(finalColor, 1.0);
}
