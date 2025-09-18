#ifndef SPHERE_H
#define SPHERE_H

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include <memory>

#include "vec3.h"
#include "hittable.h"
#include "material.h"

#include "glhelp.h"

class sphere : public hittable
{
    public:
    float radius;
    vec3 origin;
    vec3 path;
    std::shared_ptr<material> mat;
    aabb bbox;

    sphere(float my_radius, vec3 my_origin, vec3 my_origin2, std::shared_ptr<material> my_material) : hittable{48, SPHERE},  
                radius{my_radius}, origin{my_origin}, mat{my_material} 
                {
                    path = my_origin2 - origin;

                    vec3 rvec = vec3(radius, radius, radius);
                    aabb box0 = aabb(origin - rvec, origin + rvec);
                    aabb box1 = aabb(my_origin2 - rvec, my_origin2 + rvec);

                    bbox = aabb(box0, box1);
                }
    sphere(float my_radius, vec3 my_origin, std::shared_ptr<material> my_material) : hittable{48, SPHERE},  
                radius{my_radius}, origin{my_origin}, path{vec3(0.0f, 0.0f, 0.0f)}, mat{my_material} 
                {
                    vec3 rvec = vec3(radius, radius, radius);
                    bbox = aabb(origin - rvec, origin + rvec);
                }

    aabb bounding_box() const override { return bbox; }

    virtual void toUBO(UBO ubo, int offset) override {
        ubo.sub(&(mat->id), sizeof(int), offset);
        ubo.sub(&radius, sizeof(int), offset + 4);
        ubo.subVec3(origin, offset + 16);
        ubo.subVec3(path, offset + 32);
    }
};

#endif