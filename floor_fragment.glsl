#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

void main()
{
    // Create a cross pattern
    float lineWidth = 0.02;
    float pattern = 0.0;
    
    // Vertical lines
    if (abs(fract(TexCoord.x) - 0.5) < lineWidth)
        pattern = 1.0;
    
    // Horizontal lines
    if (abs(fract(TexCoord.y) - 0.5) < lineWidth)
        pattern = 1.0;
    
    // Base grey color
    vec3 baseColor = vec3(0.3, 0.3, 0.3);
    
    // Mix base color with white for the pattern
    vec3 finalColor = mix(baseColor, vec3(0.5, 0.5, 0.5), pattern);
    
    FragColor = vec4(finalColor, 1.0);
} 