#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosScreen;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Shadow uniform buffer (binding 1 matches C++ descriptor layout)
layout(std140, binding = 1) uniform ShadowUBO {
    mat4 lightSpaceMatrix;
    vec4 cascadeSplits; // unused
    vec3 lightDirection;
    float depthBiasConstant;
    vec4 cascadeBiasValues; // unused
} shadow;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * worldPos;
    FragPosScreen = gl_Position;
    
    // Calculate fragment position in light space for shadows
    FragPosLightSpace = shadow.lightSpaceMatrix * worldPos;
}