#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aColor;

out vec3 vertexColor;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vertexColor = vec3(aColor.x, aColor.y, 0.0); // Use texture coords as color (R,G components)
}