#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "vec3.h"

enum MATERIAL_TYPE {
  NOTHING,
  LAMBERTIAN,
  METALLIC,
  DIALECTRIC
}; 

class material {
    public:
    int id;
    int type;
    vec3 albedo;
    float param1;
    const int size = 32;

    material() : id{0} {};
    material(int my_type, vec3 my_albedo) : type{my_type}, albedo{my_albedo} {};
    material(int my_type, vec3 my_albedo, float my_param) : type{my_type}, albedo{my_albedo}, param1{my_param} {};

    void add(unsigned int glBuffer, int offset) {
        glBindBuffer(GL_UNIFORM_BUFFER, glBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(int), &id);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, glBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 4, sizeof(int), &type);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, glBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 8, sizeof(float), &param1);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, glBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 16, sizeof(float) * 3, &albedo);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        return;
    }
};

class lambertian : public material {
    public:

    lambertian(vec3 my_albedo) : material(LAMBERTIAN, my_albedo) {};
};

class metallic : public material {
    public:

    metallic(vec3 my_albedo, float roughness) : material(METALLIC, my_albedo, roughness) {};
};

#endif