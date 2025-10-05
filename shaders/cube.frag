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
    float padding;
} shadow;

layout(set = 0, binding = 2) uniform sampler2D shadowMaps[4]; // 4 individual shadow maps for cascades

const float PI = 3.14159265359;

// PBR material properties (can be passed as uniforms or push constants later)
const float roughness = 0.5;
const float metallic = 0.0;
const vec3 F0_dielectric = vec3(0.04); // Base reflectivity for dielectrics

// === Cook-Torrance GGX Functions ===

// GGX/Trowbridge-Reitz normal distribution function
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

// Schlick-GGX geometry function (Smith's method)
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

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// === Disney/Burley Diffuse ===
float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float roughness) {
    float energyBias = mix(0.0, 0.5, roughness);
    float energyFactor = mix(1.0, 1.0 / 1.51, roughness);
    float fd90 = energyBias + 2.0 * LdotH * LdotH * roughness;
    vec3 f0 = vec3(1.0);
    float lightScatter = (1.0 + (fd90 - 1.0) * pow(1.0 - NdotL, 5.0));
    float viewScatter = (1.0 + (fd90 - 1.0) * pow(1.0 - NdotV, 5.0));
    
    return lightScatter * viewScatter * energyFactor;
}

// === Cascaded Shadow Mapping ===

// Select appropriate cascade based on view depth
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

// PCF (Percentage Closer Filtering) for soft shadows
float calculateShadowPCF(vec4 fragPosLight, int cascadeIndex, float bias) {
    // Perspective divide
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Outside shadow map bounds - no shadow
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    
    float currentDepth = projCoords.z;
    
    // PCF with 3x3 kernel
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMaps[cascadeIndex], 0).xy;
    
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            float pcfDepth = texture(shadowMaps[cascadeIndex], projCoords.xy + offset).r;
            shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    
    shadow /= 9.0; // Average of 9 samples
    
    return shadow;
}

// Calculate shadow with cascade selection
float calculateCascadedShadow(vec3 N, vec3 L) {
    int cascadeIndex = selectCascadeIndex();
    
    // Adaptive bias based on surface angle and cascade level
    float bias = max(0.005 * (1.0 - dot(N, L)), 0.0005);
    bias *= (1.0 + float(cascadeIndex) * 0.5); // Increase bias for farther cascades
    
    return calculateShadowPCF(fragPosLightSpace[cascadeIndex], cascadeIndex, bias);
}

void main() {
    // Material base color (albedo)
    vec3 albedo = fragColor;
    
    // Setup vectors
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(camera.position.xyz - fragPos);
    
    // Calculate reflectance at normal incidence
    vec3 F0 = F0_dielectric;
    F0 = mix(F0, albedo, metallic);
    
    // Reflectance equation
    vec3 Lo = vec3(0.0);
    
    // === Directional Light with Cascaded Shadows ===
    {
        vec3 L = normalize(-shadow.lightDirection);
        vec3 H = normalize(V + L);
        
        // Strong sun intensity for visible shadows
        vec3 radiance = vec3(15.0); // Increased sun intensity
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        // Disney/Burley diffuse
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float LdotH = max(dot(L, H), 0.0);
        float diffuseFactor = DisneyDiffuse(NdotV, NdotL, LdotH, roughness);
        vec3 diffuse = (kD * albedo / PI) * diffuseFactor;
        
        // Calculate shadow
        float shadowFactor = calculateCascadedShadow(N, L);
        
        // Add directional light contribution with shadow
        Lo += (diffuse + specular) * radiance * NdotL * shadowFactor;
    }
    
    // === Point Lights (existing) ===
    vec3 lightPositions[3];
    vec3 lightColors[3];
    
    lightPositions[0] = vec3(5.0, 5.0, 5.0);
    lightPositions[1] = vec3(-5.0, 5.0, -3.0);
    lightPositions[2] = vec3(0.0, -5.0, 3.0);
    
    lightColors[0] = vec3(300.0, 300.0, 300.0); // Bright white
    lightColors[1] = vec3(150.0, 150.0, 200.0); // Cool blue
    lightColors[2] = vec3(100.0, 80.0, 60.0);   // Warm orange
    
    for(int i = 0; i < 3; ++i) {
        // Calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - fragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - fragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        // Disney/Burley diffuse
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float LdotH = max(dot(L, H), 0.0);
        float diffuseFactor = DisneyDiffuse(NdotV, NdotL, LdotH, roughness);
        vec3 diffuse = (kD * albedo / PI) * diffuseFactor;
        
        // Add to outgoing radiance Lo
        Lo += (diffuse + specular) * radiance * NdotL;
    }
    
    // Ambient lighting (reduced for dramatic shadows)
    vec3 ambient = vec3(0.01) * albedo; // Very dark ambient
    vec3 color = ambient + Lo;
    
    // HDR tonemapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    // Debug: Visualize cascade selection (optional)
    // int cascadeIndex = selectCascadeIndex();
    // vec3 cascadeColors[4] = vec3[](vec3(1,0,0), vec3(0,1,0), vec3(0,0,1), vec3(1,1,0));
    // color = mix(color, cascadeColors[cascadeIndex], 0.3);
    
    outColor = vec4(color, 1.0);
}
