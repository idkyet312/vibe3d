#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    vec4 position;
} camera;

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

void main() {
    // Material base color (albedo)
    vec3 albedo = fragColor;
    
    // Setup vectors
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(camera.position.xyz - fragPos);
    
    // Light setup (can be expanded to multiple lights)
    vec3 lightPositions[3];
    vec3 lightColors[3];
    
    lightPositions[0] = vec3(5.0, 5.0, 5.0);
    lightPositions[1] = vec3(-5.0, 5.0, -3.0);
    lightPositions[2] = vec3(0.0, -5.0, 3.0);
    
    lightColors[0] = vec3(300.0, 300.0, 300.0); // Bright white
    lightColors[1] = vec3(150.0, 150.0, 200.0); // Cool blue
    lightColors[2] = vec3(100.0, 80.0, 60.0);   // Warm orange
    
    // Calculate reflectance at normal incidence
    vec3 F0 = F0_dielectric;
    F0 = mix(F0, albedo, metallic);
    
    // Reflectance equation
    vec3 Lo = vec3(0.0);
    
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
    
    // Ambient lighting (IBL approximation)
    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;
    
    // HDR tonemapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, 1.0);
}
