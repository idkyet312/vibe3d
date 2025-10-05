#version 450
#extension GL_ARB_separate_shader_objects : enable

#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 1024
#define PI 3.14159265359

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

// Light structure
struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;
};

// Material structure
struct Material {
    vec3 albedo;
    float metallic;
    vec3 emission;
    float roughness;
    float ao;
    uint textureFlags;
    vec2 padding;
};

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

layout(set = 0, binding = 1) readonly buffer LightBuffer {
    PointLight lights[];
};

layout(set = 0, binding = 3) readonly buffer VisibleLightIndices {
    uint visibleLightIndices[];
};

layout(set = 0, binding = 4) readonly buffer LightGrid {
    uint lightCount[];
};

layout(set = 1, binding = 0) readonly buffer MaterialBuffer {
    Material materials[];
};

layout(push_constant) uniform PushConstants {
    layout(offset = 64) uint materialIndex;
    uint numTilesX;
    uint screenWidth;
    uint screenHeight;
} pushConst;

// PBR Functions
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 calculateLighting(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, float ao) {
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    vec3 Lo = vec3(0.0);
    
    // Calculate tile index
    ivec2 tileID = ivec2(gl_FragCoord.xy) / TILE_SIZE;
    uint tileIndex = tileID.y * pushConst.numTilesX + tileID.x;
    uint numLights = lightCount[tileIndex];
    uint offset = tileIndex * MAX_LIGHTS_PER_TILE;
    
    // Iterate through visible lights for this tile
    for (uint i = 0; i < numLights && i < MAX_LIGHTS_PER_TILE; i++) {
        uint lightIndex = visibleLightIndices[offset + i];
        PointLight light = lights[lightIndex];
        
        // Calculate per-light radiance
        vec3 L = normalize(light.position - fragPosition);
        vec3 H = normalize(V + L);
        float distance = length(light.position - fragPosition);
        float attenuation = 1.0 / (distance * distance);
        attenuation *= max(0.0, 1.0 - (distance / light.radius));
        vec3 radiance = light.color * light.intensity * attenuation;
        
        // Cook-Torrance BRDF
        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // Ambient
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
    
    return color;
}

void main() {
    Material material = materials[pushConst.materialIndex];
    
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(camera.position.xyz - fragPosition);
    
    vec3 albedo = material.albedo * fragColor;
    float metallic = material.metallic;
    float roughness = material.roughness;
    float ao = material.ao;
    
    vec3 color = calculateLighting(N, V, albedo, metallic, roughness, ao);
    
    // Add emission
    color += material.emission;
    
    // Tone mapping
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, 1.0);
}
