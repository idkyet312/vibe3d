#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

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
    gl_Position = camera.projection * camera.view * pushConstants.model * vec4(inPosition, 1.0);
}
