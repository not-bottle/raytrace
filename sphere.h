#ifndef SPHERE_H
#define SPHERE_H

#include <glad/glad.h>
#include <SDL2/SDL.h>

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
    material *mat;

    sphere() : radius{0.0f}, origin{vec3(0.0f, 0.0f, 0.0f)}, path{vec3(0.0f, 0.0f, 0.0f)} {};
    sphere(float my_radius, vec3 my_origin, vec3 my_path, material *my_material) : hittable{48},  
                radius{my_radius}, origin{my_origin}, path{my_path}, mat{my_material} {};
    sphere(float my_radius, vec3 my_origin, material *my_material) : hittable{48},  
                radius{my_radius}, origin{my_origin}, path{vec3(0.0f, 0.0f, 0.0f)}, mat{my_material} {};

    virtual void add(UBO ubo, int offset) const override {

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