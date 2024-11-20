#ifndef BVH_H
#define BVH_H

#include <vector>
#include <memory>

#include "hittable_list.h"
#include "hittable.h"
#include "aabb.h"

class bvh_node : public hittable {

    bvh_node(hittable_list list) {}

    bvh_node(std::vector<std::shared_ptr<hittable>> list, size_t start, size_t end) {
        
    }


    aabb bounding_box() const override { return bbox; }

    private:
    std::shared_ptr<hittable> left;
    std::shared_ptr<hittable> right;
    aabb bbox;
};

#endif