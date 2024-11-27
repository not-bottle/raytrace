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
    bvh_node(std::shared_ptr<hittable> &object) : object{object}, leaf{1}, bbox{object->bounding_box()} {}
    bvh_node(hittable_list list) : bvh_node(list.objects, 0, list.objects.size()) {}

    bvh_node(std::vector<std::shared_ptr<hittable>> &objects, size_t start, size_t end) 
    {
        int axis = rand.randint(0, 2);
        //std::cout << "Split at axis: " << axis << std::endl;

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

    void toUBO(UBO ubo, int offset) const override {
        std::queue<std::shared_ptr<bvh_node>> queue;

        std::shared_ptr<bvh_node> root = std::make_shared<bvh_node>(*this);
        std::shared_ptr<bvh_node> node;

        int idx = 0;

        // Set ids
        queue.push(root);
        while(!queue.empty()) {
            node = queue.front();
            node->id = idx;
            if (!node->leaf) {
                queue.push(node->left);
                queue.push(node->right);
            }
            queue.pop();
            idx += 1;
        }
        std::cout << "BVH max idx: " << idx << std::endl;

        int offset0 = 0;
        int obj_idx;
        int left_id;
        int right_id;

        queue.push(root);
        while(!queue.empty()) {
            node = queue.front();
            
            if (!node->leaf) {
                obj_idx = 0;
                left_id = node->left->id;
                right_id = node->right->id;
                queue.push(node->left);
                queue.push(node->right);
            } else {
                obj_idx = node->object->id;
                left_id = 0;
                right_id = 0;
            }

            // Insert node into UBO
            // leaf       - 4 bytes 
            // obj_idx    - 4 bytes
            // left_id    - 4 bytes
            // right_id   - 4 bytes
            // interval-x - 2*4bytes (blank)
            // interval-y - 2*4bytes (blank)
            // interval-z - 2*4bytes (blank)

            // std::cout << "Node - " << std::endl;
            ubo.sub(&(node->leaf), sizeof(int), offset + offset0);
            // std::cout << "leaf: " << node->leaf << std::endl;
            ubo.sub(&obj_idx, sizeof(int), offset + offset0 + 4);
            // std::cout << "obj_idx: " << obj_idx << std::endl;
            ubo.sub(&left_id, sizeof(int), offset + offset0 + 8);
            // std::cout << "left_id: " << obj_idx << std::endl;
            ubo.sub(&right_id, sizeof(int), offset + offset0 + 12);
            // std::cout << "right_id: " << obj_idx << std::endl;
            ubo.sub(&(node->bbox.x.min), sizeof(float), offset + offset0 + 16);
            ubo.sub(&(node->bbox.x.max), sizeof(float), offset + offset0 + 20);
            ubo.sub(&(node->bbox.y.min), sizeof(float), offset + offset0 + 32);
            ubo.sub(&(node->bbox.y.max), sizeof(float), offset + offset0 + 36);
            ubo.sub(&(node->bbox.z.min), sizeof(float), offset + offset0 + 48);
            ubo.sub(&(node->bbox.z.max), sizeof(float), offset + offset0 + 52);
            // std::cout << node->bbox << std::endl;

            offset0 += 64;

            queue.pop();
        }
    }

    int leaf = 0;
    std::shared_ptr<hittable> object;
    std::shared_ptr<bvh_node> left;
    std::shared_ptr<bvh_node> right;
    aabb bbox;

    static randgen rand;

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

randgen bvh_node::rand{}; 

#endif