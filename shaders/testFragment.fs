#version 330 core

layout(pixel_center_integer, origin_upper_left) in vec4 gl_FragCoord;

uniform int num_samples;

uniform vec3 delta_u;
uniform vec3 delta_v;

uniform vec3 camera_origin;
uniform vec3 viewport_top_left;

uniform int num_spheres;

int max_bounces = 2;

out vec4 FragColour;

struct ray
{
  vec3 origin;
  vec3 dir;
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

  FragColour = vec4(random_on_hemisphere(gl_FragCoord.xy, vec3(0.0f, 1.0f, 0.0f)), 0.0f);
  return;

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

vec3 raycast(vec3 ray_orig, vec3 ray_dir)
{
  vec3 sphere_origin;
  float sphere_radius;
  float t = -1.0f;
  float new_t;

  vec3 ray_colour = vec3(1.0f, 1.0f, 1.0f);
  float bounce_factor = 1.0f;

  for (int i=0; i<max_bounces; i++)
  {
    for (int i=0; i<num_spheres; i++)
    {
      new_t = hit_sphere(spheres[i].origin, spheres[i].radius, ray_dir, ray_orig);
      
      if (t < 0 || (new_t < t && new_t > 0)) {
        t = new_t;
        sphere_origin = spheres[i].origin;
        sphere_radius = spheres[i].radius;
      } 
    }

    if (t > 0.0f)
    {
      vec3 hit_point = ray_orig + ray_dir*t;
      vec3 hit_normal = normalize(hit_point - sphere_origin);
      if (dot(ray_dir, hit_normal) >= 0) {
        ray_colour = vec3(1.0f, 0.7f, 0.5f);
        break;
      } else {
        ray_orig = hit_point;
        ray_dir = random_on_hemisphere(hit_normal.xy, hit_normal);
        bounce_factor = bounce_factor * 0.5f;
        continue;
      }
    }

    vec3 norm = normalize(ray_dir);
    float a = 0.5f*(1.0f + norm.y);
    ray_colour = (1.0f-a)*vec3(1.0f, 1.0f, 1.0f) + a*vec3(0.5f, 0.7f, 1.0f);

    break;
  }

  return ray_colour*bounce_factor;
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

