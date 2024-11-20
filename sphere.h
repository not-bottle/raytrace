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

    sphere(float my_radius, vec3 my_origin, vec3 my_path, std::shared_ptr<material> my_material) : hittable{48},  
                radius{my_radius}, origin{my_origin}, path{my_path}, mat{my_material} 
                {
                    vec3 rvec = vec3(radius, radius, radius);
                    bbox = aabb(origin - rvec, origin + rvec);
                }
    sphere(float my_radius, vec3 my_origin, std::shared_ptr<material> my_material) : hittable{48},  
                radius{my_radius}, origin{my_origin}, path{vec3(0.0f, 0.0f, 0.0f)}, mat{my_material} 
                {
                    vec3 rvec = vec3(radius, radius, radius);
                    point3 origin1 = origin + path*1;
                    aabb box0 = aabb(origin - rvec, origin + rvec);
                    aabb box1 = aabb(origin1 - rvec, origin1 + rvec);

                    bbox = aabb(box0, box1);
                }

    aabb bounding_box() const override { return bbox; }

    virtual void toUBO(UBO ubo, int offset) const override {
        glBindBuffer(GL_UNIFORM_BUFFER, ubo.id);
        glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(int), &(mat->id));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, ubo.id);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 4, sizeof(float), &radius);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, ubo.id);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 16, sizeof(float) * 3, &origin);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, ubo.id);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 32, sizeof(float) * 3, &path);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
};

#endif