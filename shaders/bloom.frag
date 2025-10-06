#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;

layout(push_constant) uniform BloomParams {
    float threshold;
    float intensity;
    float radius;
} params;

// Extract bright pixels with aggressive amplification
vec3 extractBrightAreas(vec3 color) {
    float brightness = max(color.r, max(color.g, color.b));
    
    // Sharp threshold with minimal soft edge - more realistic
    float softThreshold = smoothstep(params.threshold * 0.8, params.threshold, brightness);
    
    // More controlled amplification
    float excess = max(brightness - params.threshold, 0.0);
    float amplification = 1.0 + pow(excess * 3.0, 1.8);
    
    // Preserve color while extracting bright areas
    return color * softThreshold * amplification;
}

// Realistic bloom blur with exponential falloff
vec3 gaussianBlur(sampler2D tex, vec2 uv, float radius) {
    vec2 texelSize = 1.0 / textureSize(tex, 0);
    vec3 result = vec3(0.0);
    float totalWeight = 0.0;
    
    // Fewer samples, tighter pattern for more realistic glow
    const int samples = 6;
    const float offsets[6] = float[](0.0, 0.7, 1.4, 2.1, 2.8, 3.5);
    
    for(int i = 0; i < samples; i++) {
        float offset = offsets[i] * radius;
        
        // EXPONENTIAL FALLOFF - more realistic than quadratic
        // Light intensity drops off exponentially in real bloom
        float distance = float(i) + 0.5;
        float exponentialFalloff = exp(-distance * distance * 0.4);
        float weight = exponentialFalloff;
        
        // Sample in 4 directions
        result += texture(tex, uv + vec2(offset, 0.0) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(-offset, 0.0) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(0.0, offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(0.0, -offset) * texelSize).rgb * weight;
        
        totalWeight += weight * 4.0;
    }
    
    // Diagonal samples with exponential falloff
    for(int i = 1; i < 4; i++) {
        float offset = float(i) * radius * 0.8;
        float distance = float(i) * 1.2;
        float exponentialFalloff = exp(-distance * distance * 0.4);
        float weight = exponentialFalloff * 0.6;
        
        result += texture(tex, uv + vec2(offset, offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(-offset, offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(offset, -offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(-offset, -offset) * texelSize).rgb * weight;
        
        totalWeight += weight * 4.0;
    }
    
    return result / totalWeight;
}

// Realistic multi-scale bloom
vec3 bloom(sampler2D tex, vec2 uv) {
    vec3 color = texture(tex, uv).rgb;
    
    // Extract bright areas - preserves color
    vec3 bright = extractBrightAreas(color);
    
    // Multi-scale blur for realistic glow
    // Tight inner glow + subtle outer glow
    vec3 bloom1 = gaussianBlur(tex, uv, params.radius * 0.15);  // Very tight core
    vec3 bloom2 = gaussianBlur(tex, uv, params.radius * 0.35);  // Medium spread
    vec3 bloom3 = gaussianBlur(tex, uv, params.radius * 0.7);   // Subtle outer glow
    
    // Weighted combination: 70% tight, 25% medium, 5% outer
    // This creates realistic falloff like real lens bloom
    vec3 combinedBloom = bloom1 * 0.70 + bloom2 * 0.25 + bloom3 * 0.05;
    
    // Extract bright areas to enhance contrast
    vec3 bloomBright = extractBrightAreas(combinedBloom);
    
    // Preserve color saturation
    float bloomLuminance = max(bloomBright.r, max(bloomBright.g, bloomBright.b));
    vec3 colorDirection = vec3(1.0);
    if (bloomLuminance > 0.001) {
        colorDirection = bloomBright / bloomLuminance;
        // Boost saturation slightly for more vibrant glow
        vec3 avgColor = vec3((colorDirection.r + colorDirection.g + colorDirection.b) / 3.0);
        colorDirection = mix(avgColor, colorDirection, 1.3);
    }
    
    // Apply amplification with color preservation
    return colorDirection * bloomLuminance * 1.5;
}

void main() {
    vec3 sceneColor = texture(sceneTexture, inUV).rgb;
    vec3 bloomColor = bloom(sceneTexture, inUV);
    
    // Enhance emissive objects significantly
    float brightness = max(sceneColor.r, max(sceneColor.g, sceneColor.b));
    float objectBoost = 1.0;
    if (brightness > params.threshold) {
        // Strong but controlled boost for emissive objects
        float excess = (brightness - params.threshold) * 6.0;
        objectBoost = 1.0 + pow(excess, 1.6);
    }
    
    // Combine: boosted object + controlled bloom
    // Reduced bloom contribution to prevent washing out the scene
    vec3 finalColor = sceneColor * objectBoost + bloomColor * params.intensity * 0.2;
    
    // ACES Filmic tonemapping (keeps colors vibrant)
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    vec3 mapped = (finalColor * (a * finalColor + b)) / (finalColor * (c * finalColor + d) + e);
    finalColor = clamp(mapped, 0.0, 1.0);
    
    // Strong saturation boost for vivid emissive colors
    float luminance = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
    finalColor = mix(vec3(luminance), finalColor, 1.25);
    
    outColor = vec4(finalColor, 1.0);
}
