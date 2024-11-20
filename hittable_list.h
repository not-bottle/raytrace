#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include <vector>
#include <memory>

#include "aabb.h"
#include "hittable.h"
#include "glhelp.h"

class hittable_list
{   
    public:
    std::vector<std::shared_ptr<hittable>> objects;

    hittable_list() {};

    void add(std::shared_ptr<hittable> h) 
    { 
        objects.push_back(h);
        bbox = aabb(h->bounding_box(), bbox); 
    }

    void toUBO(UBO ubo)
    {
        size_t offset = 0;
        for (std::vector<std::shared_ptr<hittable>>::iterator it = objects.begin(); it != objects.end(); it++)
        {
            (*it)->toUBO(ubo, offset);
            offset += (*it)->size;
        }
    }

    aabb bounding_box() const { return bbox; }

    private:

    aabb bbox;
};

#endif