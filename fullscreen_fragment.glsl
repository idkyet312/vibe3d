#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D screenTexture;
uniform float exposure;
uniform bool enableToneMapping;

void main()
{
    vec3 color = texture(screenTexture, TexCoord).rgb;
    
    if (enableToneMapping) {
        // Tone mapping (ACES approximation)
        color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);
    }
    
    // Exposure adjustment
    color *= exposure;
    
    FragColor = vec4(color, 1.0);
}