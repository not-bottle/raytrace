#ifndef MATERIAL_LIST_H
#define MATERIAL_LIST_H

#include "material.h"
#include "glhelp.h"

#include <vector>
#include <memory>

class material_list
{   
    public:
    std::vector<std::shared_ptr<material>> materials;
    int num;

    material_list() : num{0} {};

    void add(std::shared_ptr<material> &m)
    {
        m->id = num; // Set material id sequentially (as they are added)
        num += 1;
        materials.push_back(m);
    }

    void toUBO(UBO ubo)
    {
        size_t offset = 0;
        for (std::vector<std::shared_ptr<material>>::iterator it = materials.begin(); it != materials.end(); it++)
        {
            (*it)->toUBO(ubo, offset);
            offset += (*it)->size;
        }
    }
};

#endif