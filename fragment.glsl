#version 330 core
out vec4 FragColor;

// in vec2 TexCoord; // If using textures
// uniform sampler2D texture1; // If using textures

void main()
{
    // FragColor = texture(texture1, TexCoord); // If using textures
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f); // Orange color
} 