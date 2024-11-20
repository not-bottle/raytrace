#ifndef HITTABLE_H
#define HITTABLE_H

#include <iostream>

#include "glhelp.h"
#include "aabb.h"

class hittable
{
    public:
    int size;

    hittable() : size{0} {};
    hittable(int my_size) : size{my_size} {};

    virtual void toUBO(UBO ubo, int offset) const = 0;

    virtual aabb bounding_box() const = 0;
};

#endif