#version 330 core

layout(pixel_center_integer, origin_upper_left) in vec4 gl_FragCoord;

uniform int num_samples;

uniform vec3 delta_u;
uniform vec3 delta_v;

uniform vec3 camera_origin;
uniform vec3 viewport_top_left;

uniform int num_spheres;

out vec4 FragColour;

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

vec3 raycast(vec3 camera_origin, vec3 frag_loc);

float rand(vec2 co);
float rand_range(vec2 co, float min, float max);

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
    
    colour += raycast(camera_origin, frag_loc);
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

vec3 raycast(vec3 camera_origin, vec3 frag_loc)
{
  vec3 ray_dir = frag_loc - camera_origin;

  vec3 sphere_origin;
  float sphere_radius;
  float t = -1.0f;
  float new_t;
  
  for (int i=0; i<num_spheres; i++)
  {
    new_t = hit_sphere(spheres[i].origin, spheres[i].radius, ray_dir, camera_origin);
    
    if (t < 0 || (new_t < t && new_t > 0)) {
      t = new_t;
      sphere_origin = spheres[i].origin;
      sphere_radius = spheres[i].radius;
    } 
  }

  if (t > 0.0f)
  {
    vec3 sphere_point = camera_origin + ray_dir*t;
    vec3 sphere_normal = normalize(sphere_point - sphere_origin);
    if (dot(ray_dir, sphere_normal) >= 0) {
       return vec3(0.2f*(sphere_normal.x + 1.0f), 0.2f*(sphere_normal.y + 1.0f), sphere_normal.z + 1.0f);
    } else {
      return 0.5f*vec3(sphere_normal.x + 1.0f, sphere_normal.y + 1.0f, sphere_normal.z + 1.0f);
    }
  }

  vec3 norm = normalize(ray_dir);
  float a = 0.5f*(1.0f + norm.y);
  return (1.0f-a)*vec3(1.0f, 1.0f, 1.0f) + a*vec3(0.5f, 0.7f, 1.0f);
}

float rand(vec2 co){
  return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float rand_range(vec2 co, float min, float max){
  return min + (max - min)*rand(co);
}