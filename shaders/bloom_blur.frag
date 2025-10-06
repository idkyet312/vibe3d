#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputImage;

layout(push_constant) uniform BlurParams {
    vec2 direction; // (1,0) for horizontal, (0,1) for vertical
    float lod;
} params;

// 9-tap Gaussian blur
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texelSize = 1.0 / textureSize(inputImage, int(params.lod));
    vec3 result = texture(inputImage, inUV).rgb * weights[0];
    
    for(int i = 1; i < 5; i++) {
        vec2 offset = params.direction * texelSize * float(i);
        result += texture(inputImage, inUV + offset).rgb * weights[i];
        result += texture(inputImage, inUV - offset).rgb * weights[i];
    }
    
    outColor = vec4(result, 1.0);
}
