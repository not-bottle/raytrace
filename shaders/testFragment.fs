#version 330 core

layout(pixel_center_integer, origin_upper_left) in vec4 gl_FragCoord;

in vec2 TexCoords;
uniform sampler2D randTexture;
uniform sampler2D screenTexture;

uniform float X_MIN;
uniform float X_MAX; 
uniform float Y_MIN;
uniform float Y_MAX;

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
  float time;
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
  vec3 path;
};

layout (std140) uniform Precomp_1
{
  vec3[7919] unit_vectors;
  vec3[7919] unit_disks;
};

layout (std140) uniform Precomp_2
{
  vec3[7919] rand_squares;
  vec3[7919] random_vectors;
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

float hit_sphere(vec3 origin, float radius, ray r);
hit hit_any(ray r);

ray bounce(ray r, inout rand_state state);
vec3 raycast(ray r, inout rand_state state);

vec3 shade_sky(vec3 dir, vec3 albedo);
float shlick(float cosine, float rel_refract_index);

void material_shade(inout hit h, inout ray r, inout rand_state state);
void lambertian(material m, inout hit h, inout ray r, inout rand_state state);
void metallic(material m, inout hit h, inout ray r, inout rand_state state);
void dialectric(material m, inout hit h, inout ray r, inout rand_state state);

vec3 random_unit_vector(inout rand_state s);
vec3 random_on_hemisphere(inout rand_state s, vec3 normal);
vec3 random_unit_disk(inout rand_state s);
vec3 random_square(inout rand_state s);
float random_float(inout rand_state s);

int idxcycle(inout int idx, int rand_offset);
vec3 random_unit_vector(inout cyclestate cs);
vec3 random_on_hemisphere(inout cyclestate cs, vec3 normal);
vec3 random_unit_disk(inout cyclestate cs);
vec3 random_square(inout cyclestate cs);
float random_float(inout cyclestate cs);

uint xorshift32(inout xorshift32_state state);
float rand_float(inout xorshift32_state state);
vec3 rand_vec(inout xorshift32_state state);
vec3 random_unit_vector(inout xorshift32_state state);
vec3 random_on_hemisphere(inout xorshift32_state state, vec3 normal);
vec3 random_unit_disk(inout xorshift32_state state);

void main()
{
  vec4 randtex   = texture(randTexture, TexCoords);
  vec4 screentex = texture(screenTexture, TexCoords);

  if (gl_FragCoord.x < X_MAX && gl_FragCoord.x >= X_MIN && gl_FragCoord.y < Y_MAX && gl_FragCoord.y >= Y_MIN)
  {

  rand_state state;
  cyclestate cs;

  cs.idx = int(mod(floatBitsToInt(randtex.x), 7919));
  cs.offset = int(mod(floatBitsToInt(randtex.x), 7919));
  if (cs.offset == 0)
  {
    cs.offset = 7;
  }

  state.s = cs;

  vec3 col;

  vec3 frag_loc;
  vec2 rand_square;

  vec3 colour = vec3(0.0f, 0.0f, 0.0f);
  vec3 ray_origin;

  ray r;

  for (int i=0;i<num_samples;i++)
  {
    rand_square = random_square(state).xy;
    frag_loc = viewport_top_left + (gl_FragCoord.x + rand_square.x)*delta_u 
                                  + (gl_FragCoord.y + rand_square.y)*delta_v;

    if(defocus_angle <= 0) {
      ray_origin = camera_origin;
    } else {
      vec3 rand_disk = random_unit_disk(state);
      ray_origin = camera_origin + rand_disk.x * defocus_disk_u + rand_disk.y * defocus_disk_v;
    }

    r.count = 0u;
    r.origin = ray_origin;
    r.dir = frag_loc - ray_origin;
    r.albedo = vec3(1.0f, 1.0f, 1.0f);
    r.bounce = true;
    r.time = random_float(state);
    
    colour += raycast(r, state);
  }
  
  colour = colour/num_samples;
  FragColour = vec4(colour, 0.0f);

  float gamma = 2.2;

  FragColour.rgb = pow(FragColour.rgb, vec3(1.0/gamma));
  } else {
    FragColour = screentex;
  }
}

vec3 raycast(ray r, inout rand_state state)
{
  for (uint i=0u; i<bounce_limit; i++)
  {
    r = bounce(r, state);
    if (!r.bounce) return r.albedo;
  }

  return r.albedo;
}

ray bounce(ray r, inout rand_state state)
{
  hit h = hit_any(r);

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

hit hit_any(ray r)
{
  hit h;
  h.point = vec3(0.0f, 0.0f, 0.0f);
  h.normal = vec3(0.0f, 0.0f, 0.0f);
  h.hit = false;
  h.interior = false;
  float t = -1.0f;
  float new_t;
  sphere s;

  vec3 origin_in_time;

  for (int i=0; i<num_spheres; i++)
  {
    origin_in_time = spheres[i].origin + spheres[i].path*r.time;
    new_t = hit_sphere(origin_in_time, spheres[i].radius, r);
    
    if (t < 0 || (new_t < t && new_t > 0.001)) {
      t = new_t;
      s = spheres[i];
    } 
  }

  if (t > 0.001f)
  {
    h.point = r.origin + r.dir*t;
    h.normal = (h.point - s.origin) / s.radius;
    if (dot(h.normal, r.dir) >= 0) {
      h.normal = -h.normal;
      h.interior = true;
    }
    h.mat = s.mat;
    h.hit = true;
  }

  return h;
}


float hit_sphere(vec3 origin, float radius, ray r)
{
  vec3 oc = origin - r.origin;
  float a = dot(r.dir, r.dir);
  float h = dot(r.dir, oc);
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

void material_shade(inout hit h, inout ray r, inout rand_state state)
{
  material m = materials[h.mat];

  if(m.type == 1) {
    lambertian(m, h, r, state);
  } else if (m.type == 2) {
    metallic(m, h, r, state);
  } else if (m.type == 3) {
    dialectric(m, h, r, state);
  } else {
    r.albedo = vec3(0.0, 0.0, 0.0);
    r.bounce = false;
  }

  return;
}

void lambertian(material m, inout hit h, inout ray r, inout rand_state state)
{
  r.dir = h.normal + random_on_hemisphere(state, h.normal);
  if (near_zero(r.dir)) {
    r.dir  = h.normal;
  }
  r.bounce = true;
  r.albedo *= m.albedo;
  return;
}

void metallic(material m, inout hit h, inout ray r, inout rand_state state)
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

void dialectric(material m, inout hit h, inout ray r, inout rand_state state)
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

  if (cannot_refract || shlick(cos_theta, rel_refract_index) > random_float(state)/2.0 + 1.0) {
    r.dir = reflect(unit_dir, unit_normal);
  } else { 
    r.dir = refract(unit_dir, unit_normal, rel_refract_index);
  }

  r.bounce = true;
  return;
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

bool near_zero(vec3 v) {
  float s = 1e-8;
  return (abs(v.x) < s && abs(v.y) < s && abs(v.z) < s);
}

vec3 random_unit_vector(inout rand_state s) { return random_unit_vector(s.s); }
vec3 random_on_hemisphere(inout rand_state s, vec3 normal) { return random_on_hemisphere(s.s, normal); }
vec3 random_unit_disk(inout rand_state s) { return random_unit_disk(s.s); }
vec3 random_square(inout rand_state s) { return random_square(s.s); }
float random_float(inout rand_state s) { return random_float(s.s); }

int idxcycle(inout cyclestate cs) {
  return cs.idx = int(mod(cs.idx + cs.offset, 7919));
}

vec3 random_unit_vector(inout cyclestate cs) {
  return unit_vectors[idxcycle(cs)];
}

vec3 random_on_hemisphere(inout cyclestate cs, vec3 normal) {
  vec3 on_unit_sphere = random_unit_vector(cs);

  uint dpu = floatBitsToUint(dot(on_unit_sphere, normal));
  uint ieee_sign_mask = 0x80000000u;
  uint sign = dpu & ieee_sign_mask;

  uvec3 res = floatBitsToUint(on_unit_sphere) ^ sign;

  return uintBitsToFloat(res);
}

vec3 random_unit_disk(inout cyclestate cs) {
  return unit_disks[idxcycle(cs)];
}

vec3 random_square(inout cyclestate cs) {
  return rand_squares[idxcycle(cs)];
}

float random_float(inout cyclestate cs) {
  return random_vectors[idxcycle(cs)].x;
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
