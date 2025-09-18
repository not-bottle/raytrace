#ifndef HITTABLE_H
#define HITTABLE_H

#include <iostream>

#include "glhelp.h"
#include "aabb.h"

enum HITTABLE_TYPE {
  TRIANGLE,
  SPHERE
};

class hittable
{
    public:
    int id;
    int size;
    HITTABLE_TYPE type;

    hittable() : size{0} {};
    hittable(int my_size, HITTABLE_TYPE type) : size{my_size}, type{type} {};

    virtual void toUBO(UBO ubo, int offset) = 0;

    virtual aabb bounding_box() const = 0;
};

#endif