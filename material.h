#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "vec3.h"
#include "glhelp.h"

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

    material() : id{0} {std::cout << "ho!" << std::endl;};
    material(int my_type, vec3 my_albedo) : type{my_type}, albedo{my_albedo} {};
    material(int my_type, float my_param) : type{my_type}, param1{my_param} {};
    material(int my_type, vec3 my_albedo, float my_param) : type{my_type}, albedo{my_albedo}, param1{my_param} {};

    void toUBO(UBO ubo, int offset) {
        ubo.sub(&id, sizeof(int), offset);
        ubo.sub(&type, sizeof(int), offset + 4);
        ubo.sub(&param1, sizeof(float), offset + 8);
        ubo.subVec3(albedo, offset + 16);
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

class dialectric : public material {
    public:

    dialectric(float rel_refract_index) : material(DIALECTRIC, rel_refract_index) {};
};

#endif