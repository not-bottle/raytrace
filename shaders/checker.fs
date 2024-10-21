#version 330 core

out vec4 FragColour;

uniform float width;
uniform float height;

layout (std140) uniform checker_colours
{
  vec3[2] colours;
};

void main() {
    vec4 checker_a = vec4(colours[0], 0.0f);
    vec4 checker_b = vec4(colours[1], 0.0f);

    float xmodulus = width/10;
    float ymodulus = height/10;

    if (mod(gl_FragCoord.x, xmodulus) < xmodulus/2) {
        if (mod(gl_FragCoord.y, ymodulus) < ymodulus/2) {
            FragColour = checker_b;
        } else {
            FragColour = checker_a;
        }
    } else {
        FragColour = checker_b;
        if (mod(gl_FragCoord.y, ymodulus) < ymodulus/2) {
            FragColour = checker_a;
        } else {
            FragColour = checker_b;
        }
    }
}