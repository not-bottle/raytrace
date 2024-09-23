#version 330 core

out vec4 FragColour;

uniform uint timevalue;

uint hash( uint x );
uint hash(uvec2 v);

float uintTransFloat(uint x);

float randf(vec2 seed);

void main()
{
    vec3 luma = vec3(randf(gl_FragCoord.xy));

    FragColour = vec4(luma, 0.0f);
}

uint hash(uvec2 v) {
    return (hash(v.x ^ hash(v.y)));
}

uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

float uintTransFloat(uint x) {
  const uint ieeeMantissa = 0x007FFFFFu;
  const uint ieeeOne      = 0x3F800000u;
  x &= ieeeMantissa;
  x |= ieeeOne;

  float f = uintBitsToFloat(x);
  f = f - 1.0f;

  return f;
}

float randf(vec2 seed) {
    return uintTransFloat(hash(floatBitsToUint(seed)));
}