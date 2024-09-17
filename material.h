#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "vec3.h"

class material {
    public:
    int id;
    vec3 albedo;
    float param1;
    const int size = 32;

    material() : id{0} {};
    material(vec3 my_albedo) : albedo{my_albedo} {};
    material(vec3 my_albedo, float my_param) : albedo{my_albedo}, param1{my_param} {};

    virtual void add(unsigned int glBuffer, int offset) const = 0;
};

class lambertian : public material {
    public:

    lambertian(vec3 my_albedo) : material(my_albedo) {};

    virtual void add(unsigned int glBuffer, int offset) const override {

        glBindBuffer(GL_UNIFORM_BUFFER, glBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(int), &id);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, glBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 4, sizeof(float), &param1);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, glBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 16, sizeof(float) * 3, &albedo);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        return;
    }
};

#endif