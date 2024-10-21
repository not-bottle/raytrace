#ifndef RAYTRACE_H
#define RAYTRACE_H

#include "vec3.h"

class Camera {
    public:

    // Screen setup values
    float aspectRatio = 16.0 / 9.0;
    int screenWidth = 1200;
    int screenHeight = 1;

    int renderWidth = 1200;
    int renderHeight = 1;

    Camera(int myScreenWidth, int myRenderWidth, float myAspectRatio)
            : screenWidth{myScreenWidth}, renderWidth{myRenderWidth}, aspectRatio{myAspectRatio}
    {
    screenHeight = int(screenWidth / aspectRatio);
    screenHeight = (screenHeight < 1) ? 1 : screenHeight;

    renderHeight = int(renderWidth / aspectRatio);
    renderHeight = (renderHeight < 1) ? 1 : renderHeight;

    std::cout << "renderHeight: " << renderHeight << std::endl;
    std::cout << "renderWidth: " << renderWidth << std::endl;
    }
    
    struct orientation 
    {
    point3 lookfrom = point3(0, 0, 0);
    point3 lookat = point3(0, 0, -1);
    vec3 vup = vec3(0, 1, 0);
    float vfov = 90.0;
    float defocus_angle = 0.0;
    float focus_dist = 10;

    vec3 delta_u;
    vec3 delta_v;
    point3 viewport_top_left;
    vec3 defocus_disk_u;
    vec3 defocus_disk_v;
    };

    void cameraSetup(orientation &uvw) 
    {
    // Raytracing setup
    vec3 u, v, w;

    auto theta = degrees_to_radians(uvw.vfov);
    auto h = std::tan(theta / 2);
    auto viewport_height = 2*h*uvw.focus_dist;
    auto viewport_width = (double(renderWidth) / double(renderHeight)) * viewport_height;

    w = unit_vector(uvw.lookfrom - uvw.lookat);
    u = unit_vector(cross(uvw.vup, w));
    v = cross(w, u);

    vec3 viewport_u = viewport_width * u;
    vec3 viewport_v = viewport_height * -v;

    uvw.delta_u = viewport_u / renderWidth;
    uvw.delta_v = viewport_v / renderHeight;

    uvw.viewport_top_left = uvw.lookfrom - (uvw.focus_dist*w)
                                    - viewport_u/2
                                    - viewport_v/2;

    auto defocus_radius = uvw.focus_dist * std::tan(degrees_to_radians(uvw.defocus_angle / 2));
    uvw.defocus_disk_u = u * defocus_radius;
    uvw.defocus_disk_v = v * defocus_radius;

    std::cout << "viewport_height: " << viewport_height << std::endl;
    std::cout << "viewport_width: " << viewport_width << std::endl;
    std::cout << "viewport_top_left: " << uvw.viewport_top_left << std::endl;
    std::cout << "delta_u: " << uvw.delta_u << std::endl;
    std::cout << "delta_v: " << uvw.delta_v << std::endl;
    }
};

#endif