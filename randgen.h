#ifndef RANDGEN_H
#define RANDGEN_H

#include <random>
#include <cmath>
#include <iostream>

#include "vec3.h"

struct vec4 {
    vec3 v;
    float pad;
};

const uint seed =  1234u;

class randgen {
    private:
    std::mt19937 mt{seed};
    std::uniform_real_distribution<float> rd{-1, 1};

    public:
    randgen() {};

    float randfloat() {
        float r = mt() / UINT32_MAX + 1.0;
        return rd(mt);
    }

    vec3 randvec3() {
        return vec3(randfloat(), randfloat(), randfloat());
    }   

    vec3 randvec2() {
        return vec3(randfloat(), randfloat(), 0);
    }

    vec3 random_square() {
        return randvec2() / 2;
    }

    vec3 random_unit_vector() {
        vec3 r;
        float len;
        while(true) {
            r = randvec3();
            len = r.length_squared();
            if (1e-160 <len <= 1) {
                return r / sqrt(len);
            }
        }
    }

    vec3 random_unit_disk() {
        vec3 r;
        float len;
        while(true) {
            r = randvec2();
            len = r.length_squared();
            if (len <= 1) {
                return r;
            }
        }
    }

    void gen_unit_vectors(vec4 v[], int amount) {
        for (int i = 0;i < amount; i++) {
            v[i].v = random_unit_vector();
            v[i].pad = 0;
        }
    }  

    void gen_unit_disks(vec4 v[], int amount) {
        for (int i = 0;i < amount; i++) {
            v[i].v = random_unit_disk();
            v[i].pad = 0;
        }
    }

    void gen_rand_squares(vec4 v[], int amount) {
        for (int i = 0;i < amount; i++) {
            v[i].v = random_square();
            v[i].pad = 0;
        }
    }

    void gen_rand_vectors(vec4 v[], int amount) {
        for (int i = 0;i < amount; i++) {
            v[i].v = randvec3();
            v[i].pad = 0;
        }
    }
};

#endif