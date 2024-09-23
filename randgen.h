#ifndef RANDGEN_H
#define RANDGEN_H

#include <random>
#include <cmath>

#include "vec3.h"

class randgen {
    private:
    std::mt19937 mt{};
    std::uniform_real_distribution<> rd{-1, 1};

    public:
    randgen() {};

    float randfloat() {
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

    void gen_unit_vectors(vec3 v[], int amount) {
        for (int i = 0;i < amount; i++) {
            v[i] = random_unit_vector();
        }
    }  

    void gen_unit_disks(vec3 v[], int amount) {
        for (int i = 0;i < amount; i++) {
            v[i] = random_unit_disk();
        }
    }

    void gen_rand_squares(vec3 v[], int amount) {
        for (int i = 0;i < amount; i++) {
            v[i] = random_square();
        }
    }
};

#endif