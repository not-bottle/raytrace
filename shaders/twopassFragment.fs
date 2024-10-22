#version 330 core 
out vec4 FragColour;

uniform bool s;
uniform float width;

in vec2 texCoords;

uniform sampler2D screenTexture;

void main() 
{
    if (s) {
        if (gl_FragCoord.x < width/2) {
            FragColour = vec4(1.0f, 0.7f, 0.5f, 1.0f); 
        } else {
            FragColour = texture(screenTexture, texCoords);
        }
    } else {
        if (gl_FragCoord.x >= width/2) {
            FragColour = vec4(0.5f, 0.7f, 1.0f, 1.0f); 
        } else {
            FragColour = texture(screenTexture, texCoords);
        }
    }
}