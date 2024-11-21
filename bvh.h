#ifndef BVH_H
#define BVH_H

#include <vector>
#include <memory>
#include <algorithm>
#include <queue>

#include "hittable_list.h"
#include "hittable.h"
#include "aabb.h"
#include "randgen.h"

class bvh_node : public hittable {
    public:
    bvh_node() {}
    bvh_node(std::shared_ptr<hittable> &object) : object{object}, leaf{true}, bbox{object->bounding_box()} {}
    bvh_node(hittable_list list) : bvh_node(list.objects, 0, list.objects.size()) {}

    bvh_node(std::vector<std::shared_ptr<hittable>> &objects, size_t start, size_t end) 
    {
        randgen r {};
        int axis = r.randint(0, 2);

        auto comparator = (axis == 0) ? box_compare_x 
                        : (axis == 1) ? box_compare_y 
                                      : box_compare_z;

        size_t object_span = end - start;

        if (object_span == 1) {
            left = std::make_shared<bvh_node>(objects[start]);
            right = std::make_shared<bvh_node>(objects[start]);
        } else if (object_span == 2) {
            left = std::make_shared<bvh_node>(objects[start]);
            right = std::make_shared<bvh_node>(objects[start+1]);
        } else {
            std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);

            size_t mid = start + object_span/2;
            left = std::make_shared<bvh_node>(objects, start, mid);
            right = std::make_shared<bvh_node>(objects, mid, end);
        }

        bbox = aabb(left->bounding_box(), right->bounding_box());           
    }

    aabb bounding_box() const override { return bbox; }

    void toUBO(UBO ubo, int offset, int idx) override {
        std::queue<std::shared_ptr<bvh_node>> queue;

        std::shared_ptr<bvh_node> node = std::make_shared<bvh_node>(*this);
        queue.push(node);

        int offset0 = 0;
        int obj_idx = 0;

        while(!queue.empty()) {
            node = queue.front();

            if (node->leaf) {
                obj_idx = node->object->id; // NOTE! Assumes objects have correct idx initialized
            } else {
                queue.push(node->left);
                queue.push(node->right);
            }

            // Insert node into UBO
            // bool       - 4 bytes 
            // obj_idx    - 4 bytes
            // interval-x - 2*4bytes (blank)
            // interval-y - 2*4bytes (blank)
            // interval-z - 2*4bytes (blank)

            ubo.sub(&leaf, sizeof(bool), offset + offset0);
            ubo.sub(&obj_idx, sizeof(int), offset + offset0 + 4);
            ubo.sub(&bbox.x, sizeof(float) * 2, offset + offset0 + 8);
            ubo.sub(&bbox.y, sizeof(float) * 2, offset + offset0 + 16);
            ubo.sub(&bbox.z, sizeof(float) * 2, offset + offset0 + 24);

            offset0 += 32;

            queue.pop();
        }
    }

    private:
    bool leaf = false;
    std::shared_ptr<hittable> object;
    std::shared_ptr<bvh_node> left;
    std::shared_ptr<bvh_node> right;
    aabb bbox;

    static bool box_compare(const std::shared_ptr<hittable> a, const std::shared_ptr<hittable> b, int axis) {
        auto axis_interval_a = a->bounding_box().axis_interval(axis);
        auto axis_interval_b = b->bounding_box().axis_interval(axis);
        return axis_interval_a.min < axis_interval_b.min;
    }

    static bool box_compare_x(const std::shared_ptr<hittable> a, const std::shared_ptr<hittable> b) {
        return box_compare(a, b, 0);
    }

    static bool box_compare_y(const std::shared_ptr<hittable> a, const std::shared_ptr<hittable> b) {
        return box_compare(a, b, 1);
    }

    static bool box_compare_z(const std::shared_ptr<hittable> a, const std::shared_ptr<hittable> b) {
        return box_compare(a, b, 2);
    }
};

#endif