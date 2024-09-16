#version 330 core

layout(pixel_center_integer, origin_upper_left) in vec4 gl_FragCoord;

uniform int num_samples;

uniform vec3 delta_u;
uniform vec3 delta_v;

uniform vec3 camera_origin;
uniform vec3 viewport_top_left;

uniform int num_spheres;

uniform int bounce_limit;

out vec4 FragColour;

struct hit
{
  vec3 point;
  vec3 normal;
  bool hit;
};

struct ray
{
  vec3 origin;
  vec3 dir;
  bool bounce;
  int count;
  float factor;
};

struct sphere 
{
  int material;
  float radius;
  vec3 origin;
};

layout (std140) uniform Spheres 
{
  sphere[64] spheres;
};

float hit_sphere(vec3 origin, float radius, vec3 ray_dir, vec3 ray_orig);
hit hit_any(vec3 ray_orig, vec3 ray_dir);

ray bounce(ray r);
vec3 raycast(vec3 ray_orig, vec3 ray_dir);

float rand(vec2 co);
float rand_range(vec2 co, float min, float max);
vec3 rand_range_vec(vec2 co, float min, float max);
vec3 random_unit_vector(vec2 co);
vec3 random_on_hemisphere(vec2 co, vec3 normal);

void main()
{
  vec3 frag_loc;
  vec2 rand_square;

  vec3 colour = vec3(0.0f, 0.0f, 0.0f);

  for (int i=0;i<num_samples;i++)
  {
    rand_square = vec2(rand_range(vec2(i, i+1), -0.5f, 0.5f), rand_range(vec2(i+1, i), -0.5f, 0.5f));
    frag_loc = viewport_top_left + (gl_FragCoord.x + rand_square.x)*delta_u 
                                  + (gl_FragCoord.y + rand_square.y)*delta_v;
    
    colour += raycast(camera_origin, frag_loc - camera_origin);
  }
 
  FragColour = vec4(colour/num_samples, 0.0f);
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
    if (t <= 0.0f) {
      t = (h + sqrt(discriminant)) / a;
      if (t <= 0.0f) {
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
  vec3 sphere_origin;

  for (int i=0; i<num_spheres; i++)
  {
    new_t = hit_sphere(spheres[i].origin, spheres[i].radius, ray_dir, ray_orig);
    
    if (t < 0 || (new_t < t && new_t > 0)) {
      t = new_t;
      sphere_origin = spheres[i].origin;
    } 
  }

  if (t > 0.0f)
  {
    h.point = ray_orig + ray_dir*t;
    h.normal = normalize(h.point - sphere_origin);
    h.hit = true;
  }

  return h;
}

ray bounce(ray r)
{
  if (r.count >= bounce_limit) {
    r.origin = vec3(0.0f, 0.0f, 0.0f);
    r.bounce = false;
  } else {
    hit h = hit_any(r.origin, r.dir);

    if (h.hit)
    {
      r.origin = h.point;
      r.dir = random_on_hemisphere(h.normal.xy, h.normal);
      r.bounce = true;
      r.count = r.count + 1;
      r.factor = r.factor * 0.5f;
    } else {
      float a =  0.5f*(1.0f + normalize(r.dir).y);
      r.origin = (1.0f-a)*vec3(1.0f, 1.0f, 1.0f) + a*vec3(0.5f, 0.7f, 1.0f);
      r.bounce = false;
    }
  }

  return r;
}

vec3 raycast(vec3 ray_orig, vec3 ray_dir)
{
  ray r;
  r.count = 0;
  r.factor = 1.0f;
  r.origin = ray_orig;
  r.dir = ray_dir;
  r.bounce = true;

  while(true) 
  {
    r = bounce(r);
    if (!r.bounce) break;
  }

  return r.origin * r.factor;
}

float rand(vec2 co){
  return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float rand_range(vec2 co, float min, float max){
  return min + (max - min)*rand(co);
}

vec3 rand_vec(vec2 co) {
  return vec3(rand(co + vec2(0, 1)), rand(co + vec2(1, 0)), rand(co));
}

vec3 rand_range_vec(vec2 co, float min, float max) {
  return vec3(rand_range(co + vec2(0, 1), min, max), rand_range(co + vec2(1, 0), min, max), rand_range(co, min, max));
}

vec3 random_unit_vector(vec2 co) {
  vec3 p;
  float lensq;
  float i = 0.0f;
  while (true)
  {
    p = rand_range_vec(co + vec2(0, i), -1.0f, 1.0f);
    lensq = dot(p, p);
    if (lensq <= 1)
    {
      return p / sqrt(lensq);
    }
    i = i + 0.01f;
  }
}

vec3 random_on_hemisphere(vec2 co, vec3 normal) {
  vec3 on_unit_sphere = random_unit_vector(co);
  if (dot(on_unit_sphere, normal) >= 0) {
    return on_unit_sphere;
  } else {
    return -on_unit_sphere;
  }
}

