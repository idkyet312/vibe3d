#version 330 core
layout (location = 0) in vec3 aPos;

// We might add texCoords later: layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// out vec2 TexCoord; // If using textures

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    // TexCoord = aTexCoord; // Pass texCoord to fragment shader if using textures
} 