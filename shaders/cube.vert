#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec4 fragPosLightSpace[4]; // 4 cascade levels
layout(location = 7) out float fragViewDepth;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

layout(set = 0, binding = 1) uniform ShadowUBO {
    mat4 lightSpaceMatrices[4]; // 4 cascades
    vec4 cascadeSplits; // x, y, z = split distances, w = unused
    vec3 lightDirection;
    float padding;
} shadow;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pushConstants;

void main() {
    vec4 worldPos = pushConstants.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;
    fragNormal = mat3(transpose(inverse(pushConstants.model))) * inNormal;
    fragColor = inColor;
    
    // Calculate position in light space for each cascade
    for(int i = 0; i < 4; ++i) {
        fragPosLightSpace[i] = shadow.lightSpaceMatrices[i] * worldPos;
    }
    
    // Calculate view space depth for cascade selection
    vec4 viewPos = camera.view * worldPos;
    fragViewDepth = abs(viewPos.z);
    
    gl_Position = camera.projection * camera.view * worldPos;
}
