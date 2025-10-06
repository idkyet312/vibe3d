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
    
    // Smooth threshold transition
    float softThreshold = smoothstep(params.threshold * 0.5, params.threshold, brightness);
    
    // VERY AGGRESSIVE amplification - makes objects bright FAST with low emissive values
    // Exponential scaling for rapid brightness increase
    float amplification = 1.0 + pow((brightness - params.threshold) * 5.0, 2.0);
    amplification = max(amplification, 1.0);
    
    // Preserve color while extracting and amplifying bright areas
    return color * softThreshold * softThreshold * amplification;
}

// Blur with quadratic (inverse square) falloff like real light
vec3 gaussianBlur(sampler2D tex, vec2 uv, float radius) {
    vec2 texelSize = 1.0 / textureSize(tex, 0);
    vec3 result = vec3(0.0);
    float totalWeight = 0.0;
    
    // Sample pattern with quadratic distance falloff
    const int samples = 9;
    const float offsets[9] = float[](0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
    
    for(int i = 0; i < samples; i++) {
        float offset = offsets[i] * radius;
        
        // QUADRATIC FALLOFF: 1 / (1 + distance^2)
        // This mimics real light intensity falloff (inverse square law)
        float distance = float(i) + 1.0; // +1 to avoid division by zero at center
        float quadraticFalloff = 1.0 / (1.0 + distance * distance);
        float weight = quadraticFalloff;
        
        // Sample in 4 directions (up, down, left, right)
        result += texture(tex, uv + vec2(offset, 0.0) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(-offset, 0.0) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(0.0, offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(0.0, -offset) * texelSize).rgb * weight;
        
        totalWeight += weight * 4.0;
    }
    
    // Diagonal samples with quadratic falloff
    for(int i = 1; i < 5; i++) {
        float offset = float(i) * radius * 0.6;
        
        // QUADRATIC FALLOFF for diagonals (sqrt(2) times further)
        float distance = float(i) * 1.414; // diagonal distance
        float quadraticFalloff = 1.0 / (1.0 + distance * distance);
        float weight = quadraticFalloff * 0.5; // Reduced influence for diagonals
        
        result += texture(tex, uv + vec2(offset, offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(-offset, offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(offset, -offset) * texelSize).rgb * weight;
        result += texture(tex, uv + vec2(-offset, -offset) * texelSize).rgb * weight;
        
        totalWeight += weight * 4.0;
    }
    
    return result / totalWeight;
}

// Multi-pass bloom with minimal environmental spread
vec3 bloom(sampler2D tex, vec2 uv) {
    vec3 color = texture(tex, uv).rgb;
    
    // Extract bright areas - this preserves the COLOR of the emissive
    vec3 bright = extractBrightAreas(color);
    
    // VERY tight glow - minimal environmental spread
    vec3 bloom1 = gaussianBlur(tex, uv, params.radius * 0.2);  // Ultra tight
    vec3 bloom2 = gaussianBlur(tex, uv, params.radius * 0.4);  // Tight
    
    // Almost entirely from ultra-tight bloom (95% ultra-tight, 5% tight)
    // This keeps glow on objects, minimal environmental light
    vec3 combinedBloom = bloom1 * 0.95 + bloom2 * 0.05;
    
    // Extract bright areas from bloom to amplify the glow
    // BUT preserve the color ratios better
    vec3 bloomBright = extractBrightAreas(combinedBloom);
    
    // Calculate color direction (hue/saturation) from the bloom
    float bloomLuminance = max(bloomBright.r, max(bloomBright.g, bloomBright.b));
    vec3 colorDirection = vec3(1.0);
    if (bloomLuminance > 0.001) {
        colorDirection = bloomBright / bloomLuminance;  // Normalize to preserve color
    }
    
    // Amplify while preserving color
    return colorDirection * bloomLuminance * 2.0;
}

void main() {
    vec3 sceneColor = texture(sceneTexture, inUV).rgb;
    vec3 bloomColor = bloom(sceneTexture, inUV);
    
    // Make the object MUCH brighter than its glow
    // Amplify the base bright areas significantly more than the bloom spread
    float brightness = max(sceneColor.r, max(sceneColor.g, sceneColor.b));
    float objectBoost = 1.0;
    if (brightness > params.threshold) {
        // Objects above threshold get MASSIVE brightness boost
        float excess = (brightness - params.threshold) * 8.0;
        objectBoost = 1.0 + pow(excess, 2.0);
    }
    
    // Apply boost to base color, add dimmer bloom glow
    vec3 finalColor = sceneColor * objectBoost + bloomColor * params.intensity * 0.3;
    
    // ACES Filmic tonemapping (keeps colors vibrant in HDR)
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    vec3 mapped = (finalColor * (a * finalColor + b)) / (finalColor * (c * finalColor + d) + e);
    finalColor = clamp(mapped, 0.0, 1.0);
    
    // Boost saturation to keep emissive colors vibrant
    float luminance = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
    finalColor = mix(vec3(luminance), finalColor, 1.15);  // Increased from 1.05 to enhance color
    
    outColor = vec4(finalColor, 1.0);
}
