#version 330 core

layout(pixel_center_integer, origin_upper_left) in vec4 gl_FragCoord;

uniform vec3 delta_u;
uniform vec3 delta_v;

uniform vec3 camera_origin;
uniform vec3 viewport_top_left;

uniform int num_spheres;

out vec4 FragColour;

struct sphere 
{
  float radius;
  vec3 origin;
};

layout (std140) uniform Spheres 
{
  sphere[64] spheres;
};

float hit_sphere(vec3 origin, float radius, vec3 ray_dir, vec3 ray_orig);

void main()
{
  vec3 frag_loc = viewport_top_left + gl_FragCoord.x*delta_u + gl_FragCoord.y*delta_v;

  vec3 ray_dir = frag_loc - camera_origin;

  vec3 sphere_origin;
  float sphere_radius;
  float t;

  for (int i=0; i<num_spheres; i++)
  {
    sphere_origin = spheres[i].origin;
    sphere_radius = spheres[i].radius;

    t = hit_sphere(sphere_origin, sphere_radius, ray_dir, camera_origin);
    if (t > 0.0f)
    {
    vec3 sphere_point = camera_origin + ray_dir*t;
    vec3 sphere_normal = normalize(sphere_point - sphere_origin);
    if (dot(ray_dir, sphere_normal) >= 0) {
      FragColour = vec4(1.0f, 0.7f, 0.5f, 0.0f);
    } else {
      FragColour = 0.5f*vec4(sphere_normal.x + 1.0f, sphere_normal.y + 1.0f, sphere_normal.z + 1.0f, 0.0f);
    }
    return;
    }
  }

  vec3 norm = normalize(ray_dir);
  float a = 0.5f*(1.0f + norm.y);
  FragColour = (1.0f-a)*vec4(1.0f, 1.0f, 1.0f, 0.0f) + a*vec4(0.5f, 0.7f, 1.0f, 0.0f);
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