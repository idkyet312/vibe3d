#version 430 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosScreen;

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
    vec3 ambient = useMaterial == 1 ? material.ambient : albedo * 0.1;
    vec3 specular = useMaterial == 1 ? material.specular : vec3(0.5);
    float shininess = useMaterial == 1 ? material.shininess : 32.0;
    
    // Start with ambient lighting
    vec3 result = ambient * albedo;
    
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