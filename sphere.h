#ifndef SPHERE_H
#define SPHERE_H

#include <iostream>
#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "vec3.h"
#include "hittable.h"

class sphere : public hittable
{
    public:
    float radius;
    vec3 origin;
    int material;

    sphere() : radius{0.0f}, origin{vec3(0.0f, 0.0f, 0.0f)} , material{0} {};
    sphere(float my_radius, vec3 my_origin, int my_material) : hittable{32},  
                radius{my_radius}, origin{my_origin}, material{my_material} {};

    virtual void add(unsigned int glBuffer, int offset) const override {
        glBindBuffer(GL_UNIFORM_BUFFER, glBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(float), &radius);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, glBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 16, sizeof(float) * 3, &origin);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        std::cout << "sphere::add" << std::endl;
    }
};

#endif