#version 430 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosScreen;
in vec4 FragPosLightSpace;

out vec4 FragColor;

// Uniforms
uniform ivec2 screenSize;
uniform ivec2 numTiles;
uniform vec3 viewPos;
uniform vec3 objectColor = vec3(1.0);
uniform int useMaterial = 1;

// Material properties
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform Material material;

// Shadow uniform buffer (binding 1 matches C++ descriptor layout)
layout(std140, binding = 1) uniform ShadowUBO {
    mat4 lightSpaceMatrix;
    vec4 cascadeSplits; // unused
    vec3 lightDirection;
    float depthBiasConstant;
    vec4 cascadeBiasValues; // unused
} shadow;

// Shadow map sampler (binding 3 matches C++ descriptor layout)
uniform sampler2D shadowMap;

// Light data structure
struct Light {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;
};

// Shader storage buffers
layout(std430, binding = 0) readonly buffer LightDataBuffer {
    Light lights[];
};

layout(std430, binding = 1) readonly buffer VisibleLightIndicesBuffer {
    uint visibleLightIndices[];
};

layout(std430, binding = 2) readonly buffer LightListBuffer {
    uint lightCount[];
};

const uint MAX_LIGHTS_PER_TILE = 1024;

// Shadow calculation function
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if outside shadow map bounds
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0; // Not in shadow if outside shadow map
    }
    
    // Get depth of current fragment
    float currentDepth = projCoords.z;
    
    // Calculate bias to prevent shadow acne (adjusted for better results)
    float bias = max(0.002 * (1.0 - dot(normal, lightDir)), 0.0005);
    bias *= shadow.depthBiasConstant;
    
    // PCF (Percentage Closer Filtering) for softer shadows with larger kernel
    float shadowValue = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    int samples = 0;
    
    for(int x = -2; x <= 2; ++x) {
        for(int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadowValue += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            samples++;
        }
    }
    shadowValue /= float(samples);
    
    // Make shadows visible but not too dark - 50% opacity
    return shadowValue * 0.5;
}

void main()
{
    // Calculate tile coordinates
    ivec2 tileID = ivec2(gl_FragCoord.xy) / 16;
    uint tileIndex = tileID.y * numTiles.x + tileID.x;
    
    // Get number of lights affecting this tile
    uint numLightsInTile = lightCount[tileIndex];
    
    // Initialize lighting calculation
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Use material or object color
    vec3 albedo = useMaterial == 1 ? material.diffuse : objectColor;
    vec3 ambient = useMaterial == 1 ? material.ambient : albedo * 0.15;
    vec3 specular = useMaterial == 1 ? material.specular : vec3(0.5);
    float shininess = useMaterial == 1 ? material.shininess : 32.0;
    
    // Start with moderate ambient lighting
    vec3 result = ambient * albedo * 1.5;
    
    // Add directional light (sun/main light) with shadows
    vec3 lightDir = normalize(-shadow.lightDirection);
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Calculate shadow (0 = no shadow, 1 = full shadow)
    float shadowFactor = calculateShadow(FragPosLightSpace, norm, lightDir);
    
    // Directional light contribution (with visible shadows)
    // Shadow blocks 50% of light, leaving 50% even in shadow
    float lightAmount = 1.0 - (shadowFactor * 0.5);
    vec3 directionalDiffuse = diff * albedo * lightAmount * 1.2;
    
    // Directional specular (reduced in shadow areas)
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 directionalSpecular = spec * specular * lightAmount * 0.4;
    
    result += directionalDiffuse + directionalSpecular;
    
    // Add contribution from each light in this tile
    for (uint i = 0; i < numLightsInTile && i < MAX_LIGHTS_PER_TILE; ++i) {
        uint lightIndex = visibleLightIndices[tileIndex * MAX_LIGHTS_PER_TILE + i];
        Light light = lights[lightIndex];
        
        // Calculate light direction and distance
        vec3 lightDir = light.position - FragPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);
        
        // Attenuation
        float attenuation = light.intensity / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        attenuation = min(attenuation, 1.0);
        
        // Skip if light is too far
        if (distance > light.radius) continue;
        
        // Diffuse shading
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * light.color * albedo;
        
        // Specular shading
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        vec3 specularContrib = spec * light.color * specular;
        
        // Add light contribution
        result += (diffuse + specularContrib) * attenuation;
    }
    
    // If no lights in tile, add a basic light to avoid pure ambient
    if (numLightsInTile == 0) {
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        float diff = max(dot(norm, lightDir), 0.0);
        result += diff * vec3(1.0) * albedo * 0.5;
    }
    
    FragColor = vec4(result, 1.0);
}