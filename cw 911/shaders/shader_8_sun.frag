#version 430 core

uniform sampler2D sunTexture;  // Sun texture

in vec2 fragTexCoord;  // Received texture coordinates from the vertex shader

uniform vec3 color;
uniform float exposition;

out vec4 outColor;

void main()
{
    // Sample sun texture using texture coordinates
    vec3 sunColor = texture(sunTexture, fragTexCoord).rgb;

    // Apply color and exposure
    outColor = vec4(vec3(1.0) - exp(-sunColor * color * exposition), 1);
}
