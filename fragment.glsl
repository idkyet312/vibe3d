#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

// Material properties
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform Material material;
uniform bool useMaterial;

// Legacy uniforms (for backward compatibility)
uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform int shadingModel; // 0 = Lambert, 1 = Blinn-Phong

// Enhanced rendering features
uniform bool enableReflections;
uniform bool enableSSAO;
uniform float ambientOcclusion;
uniform float metallicFactor;
uniform float roughnessFactor;

// Simple environment mapping
uniform samplerCube skybox;
uniform bool hasEnvironmentMap;

// Calculate Fresnel reflection
float calculateFresnel(vec3 viewDir, vec3 normal, float ior) {
    float cosTheta = max(dot(viewDir, normal), 0.0);
    float r0 = pow((1.0 - ior) / (1.0 + ior), 2.0);
    return r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0);
}

// Enhanced PBR-like shading
vec3 calculateEnhancedShading(vec3 albedo, vec3 normal, vec3 viewDir, vec3 lightDir, float metallic, float roughness) {
    // Ambient
    vec3 ambient = 0.03 * albedo * (1.0 - ambientOcclusion);
    
    // Diffuse (energy conservation for metals)
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = NdotL * lightColor * albedo * (1.0 - metallic);
    
    // Specular (Cook-Torrance approximation)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, halfwayDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);
    
    // Distribution term (simplified)
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (3.14159265 * denom * denom);
    
    // Fresnel term
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    float cosTheta = max(dot(halfwayDir, viewDir), 0.0);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    
    // Geometry term (simplified)
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G1L = NdotL / (NdotL * (1.0 - k) + k);
    float G1V = NdotV / (NdotV * (1.0 - k) + k);
    float G = G1L * G1V;
    
    // BRDF
    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = numerator / denominator;
    
    // Energy conservation
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    return ambient + (kD * diffuse + specular) * NdotL;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    if (useMaterial) {
        // Enhanced material-based rendering
        vec3 result;
        
        if (enableReflections && metallicFactor > 0.1) {
            // Enhanced PBR-like rendering
            result = calculateEnhancedShading(
                material.diffuse, 
                norm, 
                viewDir, 
                lightDir, 
                metallicFactor, 
                roughnessFactor
            );
            
            // Add environment reflections for metallic surfaces
            if (hasEnvironmentMap && metallicFactor > 0.5) {
                vec3 reflectDir = reflect(-viewDir, norm);
                vec3 envColor = texture(skybox, reflectDir).rgb;
                float fresnel = calculateFresnel(viewDir, norm, 1.5);
                result = mix(result, envColor * material.specular, fresnel * metallicFactor);
            }
        } else {
            // Standard Phong shading
            vec3 ambient = lightColor * material.ambient;
            
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = lightColor * (diff * material.diffuse);
            
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
            vec3 specular = lightColor * (spec * material.specular);
            
            result = ambient + diffuse + specular;
        }
        
        // Apply ambient occlusion
        result *= (1.0 - ambientOcclusion * 0.5);
        
        FragColor = vec4(result, 1.0);
    }
    else {
        // Legacy rendering mode with enhanced features
        
        // Ambient lighting
        float ambientStrength = 0.2;
        vec3 ambient = ambientStrength * lightColor;

        // Diffuse lighting
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        vec3 specular = vec3(0.0);
        
        if (shadingModel == 1) {
            // Enhanced Blinn-Phong with more realistic parameters
            float specularStrength = 0.8;
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(norm, halfwayDir), 0.0), 64);
            specular = specularStrength * spec * lightColor;
            
            // Add simple environment reflection for shiny objects
            if (enableReflections) {
                vec3 reflectDir = reflect(-viewDir, norm);
                // Simple procedural environment color
                float envFactor = (reflectDir.y + 1.0) * 0.5;
                vec3 envColor = mix(vec3(0.1, 0.1, 0.2), vec3(0.8, 0.9, 1.0), envFactor);
                float fresnel = calculateFresnel(viewDir, norm, 1.5);
                specular += envColor * fresnel * 0.3;
            }
        }

        // Combine results with ambient occlusion
        vec3 result = (ambient + diffuse + specular) * objectColor;
        result *= (1.0 - ambientOcclusion * 0.3);
        
        FragColor = vec4(result, 1.0);
    }
}