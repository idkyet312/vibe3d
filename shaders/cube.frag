#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec4 fragPosLightSpace[4]; // 4 cascade levels
layout(location = 7) in float fragViewDepth;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

layout(set = 0, binding = 1) uniform ShadowUBO {
    mat4 lightSpaceMatrices[4]; // 4 cascades
    vec4 cascadeSplits; // x, y, z = split distances, w = unused
    vec3 lightDirection;
    float receiverBiasMultiplier; // NEW: for shader-side bias
    vec4 cascadeBiasValues; // NEW: x=cascade0, y=cascade1, z=cascade2, w=cascade3
} shadow;

layout(set = 0, binding = 2) uniform MaterialUBO {
    vec3 albedo;
    float roughness;
    vec3 emissive;
    float metallic;
    float ambientStrength;
    float lightIntensity;
    float emissiveStrength;
} material;

layout(set = 0, binding = 3) uniform sampler2D shadowMaps[4]; // 4 individual shadow maps for cascades

layout(push_constant) uniform PushConstants {
    mat4 model;
    int debugMode; // 0 = normal, 1 = show shadows, 2 = show cascades
    int objectID;  // 0 = cube, 1 = floor
} pushConstants;

const float PI = 3.14159265359;
const vec3 F0_dielectric = vec3(0.04);

// === Cook-Torrance GGX Functions ===

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float roughness) {
    float energyBias = mix(0.0, 0.5, roughness);
    float energyFactor = mix(1.0, 1.0 / 1.51, roughness);
    float fd90 = energyBias + 2.0 * LdotH * LdotH * roughness;
    float lightScatter = (1.0 + (fd90 - 1.0) * pow(1.0 - NdotL, 5.0));
    float viewScatter = (1.0 + (fd90 - 1.0) * pow(1.0 - NdotV, 5.0));
    
    return lightScatter * viewScatter * energyFactor;
}

// === Cascaded Shadow Mapping ===

int selectCascadeIndex() {
    if (fragViewDepth < shadow.cascadeSplits.x) {
        return 0;
    } else if (fragViewDepth < shadow.cascadeSplits.y) {
        return 1;
    } else if (fragViewDepth < shadow.cascadeSplits.z) {
        return 2;
    } else {
        return 3;
    }
}

float calculateShadowPCF(vec4 fragPosLight, int cascadeIndex, float baseBias) {
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    vec3 projCoords01 = projCoords * 0.5 + 0.5;

    if (projCoords01.z > 1.0 || projCoords01.x < 0.0 || projCoords01.x > 1.0 ||
        projCoords01.y < 0.0 || projCoords01.y > 1.0) {
        return 1.0;
    }

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMaps[cascadeIndex], 0));
    
    // Use bias multiplier from UBO (controlled by GUI)
    float bias = baseBias * shadow.receiverBiasMultiplier;

    float shadowSum = 0.0;
    float currentDepth = projCoords01.z;
    const int kernelRadius = 2;
    const float sampleCount = float((kernelRadius * 2 + 1) * (kernelRadius * 2 + 1));

    for (int x = -kernelRadius; x <= kernelRadius; ++x) {
        for (int y = -kernelRadius; y <= kernelRadius; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            float pcfDepth = texture(shadowMaps[cascadeIndex], projCoords01.xy + offset).r;
            shadowSum += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }

    return shadowSum / sampleCount;
}

float calculateCascadedShadow(vec3 worldPos, vec3 N, vec3 L) {
    int cascadeIndex = selectCascadeIndex();

    // No normal offset - rely entirely on depth bias
    vec4 lightSpace = shadow.lightSpaceMatrices[cascadeIndex] * vec4(worldPos, 1.0);
    
    // Use cascade-specific receiver bias from UBO (controlled by GUI)
    float receiverBias = shadow.cascadeBiasValues[cascadeIndex];

    return calculateShadowPCF(lightSpace, cascadeIndex, receiverBias);
}

void main() {
    // Choose material properties based on object ID
    vec3 albedo;
    float roughness;
    float metallic;
    float ambientStrength;
    float lightIntensity;
    vec3 emissive;
    float emissiveStrength;
    
    if (pushConstants.objectID == 0) {
        // Cube: Use material properties from UBO (controlled by GUI)
        albedo = material.albedo;
        roughness = material.roughness;
        metallic = material.metallic;
        ambientStrength = material.ambientStrength;
        lightIntensity = material.lightIntensity;
        emissive = material.emissive;
        emissiveStrength = material.emissiveStrength;
    } else {
        // Floor: Use fixed material properties (gray diffuse surface)
        albedo = vec3(0.5, 0.5, 0.5);  // Gray
        roughness = 0.8;                // Rough
        metallic = 0.0;                 // Non-metallic
        ambientStrength = 0.001;        // Low ambient
        lightIntensity = 8.0;           // Same light intensity
        emissive = vec3(0.0);           // No emission
        emissiveStrength = 0.0;
    }
    
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(camera.position.xyz - fragPos);
    
    vec3 F0 = F0_dielectric;
    F0 = mix(F0, albedo, metallic);
    
    vec3 Lo = vec3(0.0);
    
    // === Directional Light from Above ===
    vec3 L = normalize(-shadow.lightDirection);
    vec3 H = normalize(V + L);
    
    // Use light intensity
    vec3 radiance = vec3(lightIntensity);
    
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV_val = max(dot(N, V), 0.0);
    float LdotH = max(dot(L, H), 0.0);
    float diffuseFactor = DisneyDiffuse(NdotV_val, NdotL, LdotH, roughness);
    vec3 diffuse = (kD * albedo / PI) * diffuseFactor;
    
    // Calculate shadow
    float shadowFactor = calculateCascadedShadow(fragPos, N, L);
    
    // Debug modes (Press B to cycle: 0=normal, 1=shadows, 2=cascades)
    if (pushConstants.debugMode == 1) {
        // Show shadow factor as grayscale (black = shadow, white = lit)
        outColor = vec4(vec3(shadowFactor), 1.0);
        return;
    } else if (pushConstants.debugMode == 2) {
        // Show cascade levels
    int cascadeIndex = selectCascadeIndex();
        vec3 cascadeColor = vec3(0.0);
        if (cascadeIndex == 0) cascadeColor = vec3(1.0, 0.0, 0.0);      // Red
        else if (cascadeIndex == 1) cascadeColor = vec3(0.0, 1.0, 0.0); // Green
        else if (cascadeIndex == 2) cascadeColor = vec3(0.0, 0.0, 1.0); // Blue
        else cascadeColor = vec3(1.0, 1.0, 0.0);                        // Yellow
        
        // Mix cascade color with albedo and shadow factor
        vec3 mixColor = cascadeColor * 0.5 + albedo * 0.5;
        mixColor *= (shadowFactor * 0.7 + 0.3); // Show shadows in cascade mode too
        outColor = vec4(mixColor, 1.0);
        return;
    }
    
    // Normal rendering with STRONG shadow contrast
    Lo += (diffuse + specular) * radiance * NdotL * shadowFactor;
    
    // Ambient lighting
    vec3 ambient = vec3(ambientStrength) * albedo;
    
    // Add emissive component (self-illumination)
    vec3 emissiveColor = emissive * emissiveStrength;
    
    vec3 color = ambient + Lo + emissiveColor;
    
    // HDR tonemapping
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, 1.0);
}
