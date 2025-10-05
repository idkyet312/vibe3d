#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragColor;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint materialIndex;
} pushConstants;

void main() {
    vec4 worldPos = pushConstants.model * vec4(inPosition, 1.0);
    fragPosition = worldPos.xyz;
    
    // Transform normal to world space
    mat3 normalMatrix = transpose(inverse(mat3(pushConstants.model)));
    fragNormal = normalize(normalMatrix * inNormal);
    
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    
    gl_Position = camera.projection * camera.view * worldPos;
}
