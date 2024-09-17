#ifndef MATERIAL_LIST_H
#define MATERIAL_LIST_H

#include "material.h"

class material_list
{   
    public:

    int num;
    int offset;

    material_list() : num{0}, offset{0} {};

    void add(unsigned int ubo, material &m)
    {
        m.id = num; // Set material id sequentially (as they are added)
        m.add(ubo, offset);
        offset += m.size;
        num += 1;
    }
};

#endif