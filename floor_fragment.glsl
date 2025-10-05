#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 Color;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

// Enhanced floor features
uniform float time;

// Calculate Fresnel reflection
float calculateFresnel(vec3 viewDir, vec3 normal, float ior) {
    float cosTheta = max(dot(viewDir, normal), 0.0);
    float r0 = pow((1.0 - ior) / (1.0 + ior), 2.0);
    return r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Enhanced grid pattern with distance-based fading
    float gridSize = 1.0;
    vec2 gridPos = mod(FragPos.xz, vec2(gridSize));
    float lineWidth = 0.05;
    float gridLine = 1.0 - smoothstep(0.0, lineWidth, min(gridPos.x, 1.0 - gridPos.x)) * 
                          smoothstep(0.0, lineWidth, min(gridPos.y, 1.0 - gridPos.y));
    
    // Distance-based grid fading
    float distanceFromCamera = length(viewPos - FragPos);
    float gridFade = 1.0 - smoothstep(10.0, 50.0, distanceFromCamera);
    gridLine *= gridFade;
    
    // Enhanced grid coloring with slight blue tint
    vec3 baseColor = Color;
    vec3 gridColor = mix(baseColor, baseColor * vec3(0.6, 0.7, 1.0), gridLine * 0.3);
    
    // Enhanced lighting calculation
    // Ambient with slight environment tint
    float ambientStrength = 0.15;
    vec3 ambientColor = lightColor * mix(vec3(1.0), vec3(0.7, 0.8, 1.0), 0.3);
    vec3 ambient = ambientStrength * ambientColor;

    // Diffuse lighting with soft falloff
    float diff = max(dot(norm, lightDir), 0.0);
    diff = pow(diff, 0.8); // Softer falloff
    vec3 diffuse = diff * lightColor;

    // Enhanced specular with environment reflection
    float specularStrength = 0.15;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Add subtle environment reflection
    float fresnel = calculateFresnel(viewDir, norm, 1.33); // Water-like reflection
    vec3 envReflection = vec3(0.6, 0.7, 0.9) * fresnel * 0.1; // Subtle sky reflection
    
    // Subtle distance fog
    float fogDensity = 0.01;
    float fogFactor = exp(-distanceFromCamera * fogDensity);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    // Combine all lighting effects
    vec3 result = (ambient + diffuse + specular) * gridColor + envReflection;
    
    // Apply fog
    vec3 fogColor = vec3(0.7, 0.8, 0.9);
    result = mix(fogColor, result, fogFactor);
    
    // Subtle color grading
    result = pow(result, vec3(0.95)); // Slight gamma adjustment
    
    FragColor = vec4(result, 1.0);
}