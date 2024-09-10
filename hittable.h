#ifndef HITTABLE_H
#define HITTABLE_H

#include <iostream>

class hittable
{
    public:
    int size;

    hittable() : size{0} {};
    hittable(int my_size) : size{my_size} {};

    virtual void add(unsigned int glBuffer, int offset) const = 0;
};

#endif