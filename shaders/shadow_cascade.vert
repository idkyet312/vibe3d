#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(push_constant) uniform PushConstants {
    mat4 lightSpaceMatrix;
    mat4 model;
} pushConstants;

void main() {
    gl_Position = pushConstants.lightSpaceMatrix * pushConstants.model * vec4(inPosition, 1.0);
}
