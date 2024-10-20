#version 330 core

layout(pixel_center_integer, origin_upper_left) in vec4 gl_FragCoord;

in vec2 TexCoords;
uniform sampler2D screenTexture;

uniform uint time_u32t;

uniform int num_samples;

uniform vec3 delta_u;
uniform vec3 delta_v;

uniform vec3 camera_origin;
uniform vec3 viewport_top_left;

uniform vec3 defocus_disk_u;
uniform vec3 defocus_disk_v;
uniform float defocus_angle;

uniform int num_spheres;

uniform uint bounce_limit;

out vec4 FragColour;

struct cyclestate {
  int idx;
  int offset;
};

struct xorshift32_state {
  uint a;
};

struct rand_state {
  cyclestate s;  
};

struct hit
{
  vec3 point;
  vec3 normal;
  bool hit;
  bool interior;
  int mat;
};

struct ray
{
  vec3 origin;
  vec3 dir;
  bool bounce;
  uint count;
  vec3 albedo;
};

struct material
{
  int id;
  int type;
  float param1;
  vec3 albedo;
};

struct sphere 
{
  int mat;
  float radius;
  vec3 origin;
};

layout (std140) uniform Precomp_1
{
  vec3[2048] unit_vectors;
  vec3[2048] unit_disks;
};

layout (std140) uniform Precomp_2
{
  vec3[2048] rand_squares;
  vec3[2048] random_vectors;
};

layout (std140) uniform Materials
{
  material[64] materials;
};

layout (std140) uniform Spheres 
{
  sphere[64] spheres;
};

void main()
{
  vec4 tex = texture(screenTexture, TexCoords);

  rand_state state;
  cyclestate cs;

  cs.idx = int(mod(floatBitsToInt(tex.x), 2011));
  cs.offset = 7;

  state.s = cs;

  vec3 col;

  vec3 frag_loc;
  vec2 rand_square;

  vec3 colour = vec3(0.0f, 0.0f, 0.0f);
  vec3 ray_origin;

  for (int i=0;i<num_samples;i++)
  {
    for (int i=0; i<num_spheres) {
        
    }
  }
  
  colour = colour/num_samples;
  FragColour = vec4(colour, 0.0f);

  float gamma = 2.2;

  FragColour.rgb = pow(FragColour.rgb, vec3(1.0/gamma));
}