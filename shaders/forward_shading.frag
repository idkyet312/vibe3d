#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragPosScreen;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUniforms {
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
} camera;

layout(set = 0, binding = 1, std430) readonly buffer LightBuffer {
    vec4 lights[]; // position.xyz, radius, color.rgb, intensity
};

layout(set = 0, binding = 2, std430) readonly buffer TileDataBuffer {
    uint tileData[];
};

layout(set = 0, binding = 3, std430) readonly buffer VisibleLightIndicesBuffer {
    uint visibleLightIndices[];
};

layout(push_constant) uniform PushConstants {
    mat4 model;
    uvec2 screenSize;
    uvec2 numTiles;
    uint numLights;
} push;

const uint MAX_LIGHTS_PER_TILE = 256;
const uint TILE_SIZE = 16;

void main() {
    // Calculate tile coordinates
    uvec2 tileID = uvec2(gl_FragCoord.xy) / TILE_SIZE;
    uint tileIndex = tileID.y * push.numTiles.x + tileID.x;
    
    // Get number of lights in this tile
    uint numLightsInTile = tileData[tileIndex];
    
    // Initialize lighting
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(camera.cameraPos - fragWorldPos);
    
    // Material properties (simplified)
    vec3 albedo = vec3(0.7, 0.7, 0.7);
    vec3 specularColor = vec3(0.3, 0.3, 0.3);
    float shininess = 32.0;
    
    // Ambient lighting
    vec3 ambient = vec3(0.1) * albedo;
    vec3 color = ambient;
    
    // Process each light in the tile
    for (uint i = 0; i < numLightsInTile && i < MAX_LIGHTS_PER_TILE; ++i) {
        uint lightIndex = visibleLightIndices[tileIndex * MAX_LIGHTS_PER_TILE + i];
        
        // Get light properties
        vec3 lightPos = lights[lightIndex * 2].xyz;
        float lightRadius = lights[lightIndex * 2].w;
        vec3 lightColor = lights[lightIndex * 2 + 1].xyz;
        float lightIntensity = lights[lightIndex * 2 + 1].w;
        
        // Calculate lighting
        vec3 lightDir = lightPos - fragWorldPos;
        float distance = length(lightDir);
        
        // Skip if outside light radius
        if (distance > lightRadius) continue;
        
        lightDir = normalize(lightDir);
        
        // Attenuation
        float attenuation = lightIntensity / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        
        // Diffuse
        float diffuse = max(dot(normal, lightDir), 0.0);
        
        // Specular (Blinn-Phong)
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float specular = pow(max(dot(normal, halfwayDir), 0.0), shininess);
        
        // Add contribution
        color += (diffuse * albedo + specular * specularColor) * lightColor * attenuation;
    }
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    outColor = vec4(color, 1.0);
}