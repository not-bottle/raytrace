#version 330 core

out vec4 FragColour;

uniform float GRID_SIZE;
uniform float CONTRAST;
uniform int LAYERS;
uniform float LACURNITY;

float interpolate(float a0, float a1, float w);
vec2 randomGradient(int ix, int iy);
float dotGridGradient(int ix, int iy, float x, float y);
float perlin(float x, float y);

void main() 
{
    float value = 0.0f;
    float freq = 1.0f;
    float amp = 1.0f;

    for (int i = 0; i < LAYERS; i++) {
        value += perlin(gl_FragCoord.x * freq / GRID_SIZE, gl_FragCoord.y * freq / GRID_SIZE) * amp;

        freq *= LACURNITY;
        amp /= 2;
    }

    value = value * CONTRAST;

    if (value > 1.0f) {
        value = 1.0f;
    }
    if (value < -1.0f) {
        value = -1.0f;
    }

    value = (value * 0.5) + 0.5;

    vec3 colour = vec3(value, value, value);

    FragColour = vec4(colour, 0.0f);
}

float interpolate(float a0, float a1, float w) {
    return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
}

vec2 randomGradient(int ix, int iy){
    vec2 uv = vec2(ix, iy);
    uv = vec2( dot(uv, vec2(127.1,311.7) ),
               dot(uv, vec2(269.5,183.3) ) );
    return -1.0 + 2.0 * fract(sin(uv) * 43758.5453123);
}

float dotGridGradient(int ix, int iy, float x, float y) {
    vec2 gradient = randomGradient(ix, iy);

    float dx = x - float(ix);
    float dy = y - float(iy);

    return (dx*gradient.x + dy*gradient.y);
}

float perlin(float x, float y) {
    int x0 = int(floor(x));
    int x1 = x0 + 1;
    int y0 = int(floor(y));
    int y1 = y0 + 1;

    float sx = x - float(x0);
    float sy = y - float(y0);

    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(x0, y0, x, y);
    n1 = dotGridGradient(x1, y0, x, y);
    ix0 = interpolate(n0, n1, sx);


    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix1 = interpolate(n0, n1, sx);

    value = interpolate(ix0, ix1, sy);

    return value;
}

