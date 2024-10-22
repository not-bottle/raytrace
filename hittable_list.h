#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"
#include "glhelp.h"

class hittable_list
{   
    public:

    int num;
    int offset;

    hittable_list() : num{0}, offset{0} {};

    void add(UBO ubo, hittable &h)
    {
        h.add(ubo, offset);
        offset += h.size;
        num += 1;
    }
};

#endif