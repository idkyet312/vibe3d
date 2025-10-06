#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputImage;

layout(push_constant) uniform BloomParams {
    float threshold;
    float knee;
} params;

void main() {
    vec3 color = texture(inputImage, inUV).rgb;
    
    // Calculate brightness using perceived luminance
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // Soft threshold
    float soft = brightness - params.threshold + params.knee;
    soft = clamp(soft, 0.0, 2.0 * params.knee);
    soft = soft * soft / (4.0 * params.knee + 0.00001);
    
    float contribution = max(soft, brightness - params.threshold);
    contribution /= max(brightness, 0.00001);
    
    outColor = vec4(color * contribution, 1.0);
}
