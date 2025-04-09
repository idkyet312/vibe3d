#version 330 core
out vec4 FragColor;

// in vec2 TexCoord; // If using textures
// uniform sampler2D texture1; // If using textures

in vec3 ourColor;

void main()
{
    // FragColor = texture(texture1, TexCoord); // If using textures
    FragColor = vec4(ourColor, 1.0f);
} 