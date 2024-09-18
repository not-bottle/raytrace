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

uniform int num_spheres;

uniform uint bounce_limit;

out vec4 FragColour;

struct xorshift32_state {
  uint a;
};

struct hit
{
  vec3 point;
  vec3 normal;
  bool hit;
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

layout (std140) uniform Materials
{
  material[64] materials;
};

layout (std140) uniform Spheres 
{
  sphere[64] spheres;
};

bool near_zero(vec3 v);

float hit_sphere(vec3 origin, float radius, vec3 ray_dir, vec3 ray_orig);
hit hit_any(vec3 ray_orig, vec3 ray_dir);

ray bounce(ray r, inout xorshift32_state state);
vec3 raycast(vec3 ray_orig, vec3 ray_dir, inout xorshift32_state state);

vec3 shade_sky(vec3 dir, vec3 albedo);

void material_shade(inout hit h, inout ray r, inout xorshift32_state state);
void lambertian(material m, inout hit h, inout ray r, inout xorshift32_state state);
void metallic(material m, inout hit h, inout ray r, inout xorshift32_state state);
void dialectric(material m, inout hit h, inout ray r, inout xorshift32_state state);

uint xorshift32(inout xorshift32_state state);
float rand_float(inout xorshift32_state state);
vec3 rand_vec(inout xorshift32_state state);
vec3 random_unit_vector(inout xorshift32_state state);
vec3 random_on_hemisphere(inout xorshift32_state state, vec3 normal);

uint cantor(uint k1, uint k2);
float lcg(uint x);

void main()
{
  vec4 tex = texture(screenTexture, TexCoords);
  uint seed = floatBitsToUint(tex.x + tex.y + tex.z);
  seed ^= cantor(uint(gl_FragCoord.x), uint(gl_FragCoord.y));

  xorshift32_state state;
  state.a = cantor(time_u32t, seed);

  vec3 frag_loc;
  vec2 rand_square;

  vec3 colour = vec3(0.0f, 0.0f, 0.0f);

  for (int i=0;i<num_samples;i++)
  {
    rand_square = vec2(rand_float(state) - 0.5, rand_float(state) - 0.5);
    frag_loc = viewport_top_left + (gl_FragCoord.x + rand_square.x)*delta_u 
                                  + (gl_FragCoord.y + rand_square.y)*delta_v;
    
    colour += raycast(camera_origin, frag_loc - camera_origin, state);
  }
  
  colour = colour/num_samples;
  FragColour = vec4(colour, 0.0f);

  float gamma = 2.2;

  FragColour.rgb = pow(FragColour.rgb, vec3(1.0/gamma));
}

float hit_sphere(vec3 origin, float radius, vec3 ray_dir, vec3 ray_orig)
{
  vec3 oc = origin - ray_orig;
  float a = dot(ray_dir, ray_dir);
  float h = dot(ray_dir, oc);
  float c = dot(oc, oc) - radius*radius;

  float discriminant = h*h - a*c;
  if (discriminant >= 0) {
    float t = (h - sqrt(discriminant)) / a;
    if (!(0.001 < t)) {
      t = (h + sqrt(discriminant)) / a;
      if (!(0.001 < t)) {
        t = -1.0f;
      }
    }
    return t;
  } else {
    return -1.0f;
  }
}

hit hit_any(vec3 ray_orig, vec3 ray_dir)
{
  hit h;
  h.point = vec3(0.0f, 0.0f, 0.0f);
  h.normal = vec3(0.0f, 0.0f, 0.0f);
  h.hit = false;
  float t = -1.0f;
  float new_t;
  sphere s;

  for (int i=0; i<num_spheres; i++)
  {
    new_t = hit_sphere(spheres[i].origin, spheres[i].radius, ray_dir, ray_orig);
    
    if (t < 0 || (new_t < t && new_t > 0)) {
      t = new_t;
      s = spheres[i];
    } 
  }

  if (t > 0.0f)
  {
    h.point = ray_orig + ray_dir*t;
    h.normal = normalize(h.point - s.origin);
    if (dot(h.normal, ray_dir) > 0) {
      h.normal = -h.normal;
    }
    h.mat = s.mat;
    h.hit = true;
  }

  return h;
}

ray bounce(ray r, inout xorshift32_state state)
{
  if (r.count >= bounce_limit) {
    r.albedo = vec3(0.0f, 0.0f, 0.0f);
    r.bounce = false;
  } else {
    hit h = hit_any(r.origin, r.dir);

    if (h.hit)
    {
      r.origin = h.point;
      r.count = r.count + 1u;

      material_shade(h, r, state);

    } else {
      r.albedo *= shade_sky(r.dir, r.albedo);
      r.bounce = false;
    }
  }

  return r;
}

void material_shade(inout hit h, inout ray r, inout xorshift32_state state)
{
  material m = materials[h.mat];

  if(m.type == 1) {
    lambertian(m, h, r, state);
  } else if (m.type == 2) {
    metallic(m, h, r, state);
  } else {
    r.albedo = shade_sky(r.dir, r.albedo);
    r.bounce = false;
  }

  return;
}

void lambertian(material m, inout hit h, inout ray r, inout xorshift32_state state)
{
  r.dir = h.normal + random_on_hemisphere(state, h.normal);
  if (near_zero(r.dir)) {
    r.dir  = h.normal;
  }
  r.bounce = true;
  r.albedo *= m.albedo;
  return;
}

void metallic(material m, inout hit h, inout ray r, inout xorshift32_state state)
{
  r.dir = r.dir + (-2*dot(r.dir, h.normal))*h.normal;
  r.dir = normalize(r.dir) + (m.param1 * random_unit_vector(state));
  r.albedo *= m.albedo;

  r.bounce = dot(r.dir, h.normal) > 0;
  if (!r.bounce) {
    r.albedo = vec3(0.0f, 0.0f, 0.0f);
  }
  return;
}

void dialectric(material m, inout hit h, inout ray r, inout xorshift32_state state)
{
  r.dir = refract(r.dir, h.normal, m.param1);
  r.bounce = true;
  return;
}


vec3 raycast(vec3 ray_orig, vec3 ray_dir, inout xorshift32_state state)
{
  ray r;
  r.count = 0u;
  r.origin = ray_orig;
  r.dir = ray_dir;
  r.albedo = vec3(1.0f, 1.0f, 1.0f);
  r.bounce = true;

  while(true) 
  {
    r = bounce(r, state);
    if (!r.bounce) break;
  }

  return r.albedo;
}


vec3 shade_sky(vec3 dir, vec3 albedo) {

  float a =  0.5f*(1.0f + normalize(dir).y);
  albedo *= (1.0f-a)*vec3(1.0f, 1.0f, 1.0f) + a*vec3(0.5f, 0.7f, 1.0f);

  return albedo;
}

uint xorshift32(inout xorshift32_state state) {
  uint x = state.a;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return state.a = x;
}

float rand_float(inout xorshift32_state state) {
  uint x = xorshift32(state);
  const uint ieeeMantissa = 0x007FFFFFu;
  const uint ieeeOne      = 0x3F800000u;
  x &= ieeeMantissa;
  x |= ieeeOne;

  float f = uintBitsToFloat(x);
  f = f - 1.0f;

  return f;
}

vec3 rand_vec(inout xorshift32_state state) {
  return vec3(rand_float(state), rand_float(state), rand_float(state));
}

vec3 random_unit_vector(inout xorshift32_state state) {
  vec3 p;
  float lensq;
  uint i = 0u;
  while (true)
  {
    state.a += i;
    p = rand_vec(state);
    lensq = dot(p, p);
    if (1e-160 < lensq && lensq <= 1)
    {
      return p / sqrt(lensq);
    }
    i += 1u;
  }
}

vec3 random_on_hemisphere(inout xorshift32_state state, vec3 normal) {
  vec3 on_unit_sphere = random_unit_vector(state);
  if (dot(on_unit_sphere, normal) >= 0) {
    return on_unit_sphere;
  } else {
    return -on_unit_sphere;
  }
}

uint cantor(uint k1, uint k2) {
  uint x = (k1 + k2)*(k1 + k2 + 1u);
  x = x >> 1;
  return x + k2;
}

bool near_zero(vec3 v) {
  float s = 1e-8;
  return (abs(v.x) < s && abs(v.y) < s && abs(v.z) < s);
}
