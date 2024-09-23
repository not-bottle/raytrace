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

struct xorshift32_state {
  uint a;
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

layout (std140) uniform Precomp
{
  vec3[2011] unit_vectors;
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
float shlick(float cosine, float rel_refract_index);

void material_shade(inout hit h, inout ray r, inout xorshift32_state state);
void lambertian(material m, inout hit h, inout ray r, inout xorshift32_state state);
void metallic(material m, inout hit h, inout ray r, inout xorshift32_state state);
void dialectric(material m, inout hit h, inout ray r, inout xorshift32_state state);
int idxcycle(inout int idx, int rand_offset);

uint xorshift32(inout xorshift32_state state);
float rand_float(inout xorshift32_state state);
vec3 rand_vec(inout xorshift32_state state);
vec3 random_unit_vector(inout xorshift32_state state);
vec3 random_on_hemisphere(inout xorshift32_state state, vec3 normal);
vec3 random_unit_disk(inout xorshift32_state state);

void main()
{
  if (gl_FragCoord.x < 10000) {
    vec4 tex = texture(screenTexture, TexCoords);
    xorshift32_state state;
    state.a = floatBitsToUint(tex.x);

    int rand_idx = int(mod(floatBitsToInt(tex.x), 2011));
    int rand_offset = rand_idx;

    FragColour = vec4((unit_vectors[idxcycle(rand_idx, rand_offset)]/2) + 0.5, 0.0f);
    return;

    vec3 frag_loc;
    vec2 rand_square;

    vec3 colour = vec3(0.0f, 0.0f, 0.0f);
    vec3 ray_origin;

    for (int i=0;i<num_samples;i++)
    {
      rand_square = vec2(rand_float(state) - 0.5, rand_float(state) - 0.5);
      frag_loc = viewport_top_left + (gl_FragCoord.x + rand_square.x)*delta_u 
                                    + (gl_FragCoord.y + rand_square.y)*delta_v;

      if(defocus_angle <= 0) {
        ray_origin = camera_origin;
      } else {
        vec3 rand_disk = random_unit_disk(state);
        ray_origin = camera_origin + rand_disk.x * defocus_disk_u + rand_disk.y * defocus_disk_v;
      }
      
      colour += raycast(ray_origin, frag_loc - ray_origin, state);
    }
    
    colour = colour/num_samples;
    FragColour = vec4(colour, 0.0f);

    float gamma = 2.2;

    FragColour.rgb = pow(FragColour.rgb, vec3(1.0/gamma));
  } else {
    FragColour = vec4(1.0f, 0, 0, 0);
  }

  
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
    if (t < 0.001) {
      t = (h + sqrt(discriminant)) / a;
      if (t < 0.001) {
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
  h.interior = false;
  float t = -1.0f;
  float new_t;
  sphere s;

  for (int i=0; i<num_spheres; i++)
  {
    new_t = hit_sphere(spheres[i].origin, spheres[i].radius, ray_dir, ray_orig);
    
    if (t < 0 || (new_t < t && new_t > 0.001)) {
      t = new_t;
      s = spheres[i];
    } 
  }

  if (t > 0.001f)
  {
    h.point = ray_orig + ray_dir*t;
    h.normal = (h.point - s.origin) / s.radius;
    if (dot(h.normal, ray_dir) >= 0) {
      h.normal = -h.normal;
      h.interior = true;
    }
    h.mat = s.mat;
    h.hit = true;
  }

  return h;
}

ray bounce(ray r, inout xorshift32_state state)
{
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

  return r;
}

void material_shade(inout hit h, inout ray r, inout xorshift32_state state)
{
  material m = materials[h.mat];

  if(m.type == 1) {
    lambertian(m, h, r, state);
  } else if (m.type == 2) {
    metallic(m, h, r, state);
  } else if (m.type == 3) {
    dialectric(m, h, r, state);
  } else {
    r.albedo = vec3(1.0, 0.0, 0.0);
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
  r.dir = reflect(r.dir, normalize(h.normal));
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
  float rel_refract_index = m.param1;
  if (!h.interior) {
    rel_refract_index = 1.0/rel_refract_index;
  }

  vec3 unit_dir = normalize(r.dir);
  vec3 unit_normal = normalize(h.normal);

  float cos_theta = min(dot(-unit_dir, h.normal), 1.0);
  float sin_theta =  sqrt(1.0 - cos_theta * cos_theta);
  bool cannot_refract = rel_refract_index * sin_theta > 1.0;

  if (cannot_refract || shlick(cos_theta, rel_refract_index) > rand_float(state)) {
    r.dir = reflect(unit_dir, unit_normal);
  } else { 
    r.dir = refract(unit_dir, unit_normal, rel_refract_index);
  }

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

  for (uint i=0u; i<bounce_limit; i++)
  {
    r = bounce(r, state);
    if (!r.bounce) return r.albedo;
  }

  return r.albedo;
}

float shlick(float cosine, float rel_refract_index)
{
  float r0 = (1 - rel_refract_index) / (1 + rel_refract_index);
  r0 = r0*r0;
  return r0 + (1-r0)*pow((1 - cosine), 5);
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

vec3 random_unit_disk(inout xorshift32_state state) {
  vec3 p;
  float lensq;
  uint i = 0u;
  while (true)
  {
    state.a += i;
    p = vec3(rand_float(state), rand_float(state), 0);
    lensq = dot(p, p);
    if (lensq <= 1)
    {
      return p;
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

bool near_zero(vec3 v) {
  float s = 1e-8;
  return (abs(v.x) < s && abs(v.y) < s && abs(v.z) < s);
}

int idxcycle(inout int idx, int rand_offset) {
  return idx = int(mod(idx + rand_offset, 2011));
}
