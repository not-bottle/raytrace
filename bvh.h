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
        #ifdef DEBUG
        axis = 0;
        //std::cout << "Split at axis: " << axis << std::endl;
        #endif

        bbox = aabb::empty;

        for (size_t object_index=start; object_index < end; object_index++) {
            bbox = aabb(bbox, objects[object_index]->bounding_box());
        }

        int axis = bbox.longest_axis();

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

        // Removed due to bbox optimization constructing each bbox from the list of objects
        //bbox = aabb(left->bounding_box(), right->bounding_box());           
    }

    aabb bounding_box() const override { return bbox; }

    void toUBO(UBO ubo, int offset) override {
        std::queue<std::shared_ptr<bvh_node>> queue;

        std::shared_ptr<bvh_node> root = std::make_shared<bvh_node>(*this);
        std::shared_ptr<bvh_node> node;

        int idx = 0;
        id = 0;

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

        build_links();

        int offset0 = 0;
        int obj_idx;
        int hit_id;
        int miss_id;

        root->hit_node = left; // Fix all this this is scuffed lmao

        queue.push(root);
        while(!queue.empty()) {
            node = queue.front();
            
            if (node->leaf == 0) {
                obj_idx = 0;
                
                if (node->hit_node == NULL) {
                    hit_id = -1;
                } else {
                    hit_id = node->hit_node->id;
                }

                if (node->miss_node == NULL) {
                    miss_id = -1;
                } else {
                    miss_id = node->miss_node->id;
                }

                queue.push(node->left);
                queue.push(node->right);
            } else {
                obj_idx = node->object->id;

                if (node->hit_node != NULL) {
                    hit_id = node->hit_node->id;
                } else {
                    hit_id = -1;
                }

                miss_id = hit_id;
            }

            // Insert node into UBO
            // leaf       - 4 bytes 
            // obj_idx    - 4 bytes
            // left_id    - 4 bytes
            // right_id   - 4 bytes
            // interval-x - 2*4bytes (blank)
            // interval-y - 2*4bytes (blank)
            // interval-z - 2*4bytes (blank)

            #ifdef DEBUG
            std::cout << "Node - "  << node->id << std::endl;
            std::cout << "leaf: " << node->leaf << std::endl;
            std::cout << "obj_idx: " << obj_idx << std::endl;
            std::cout << "hit_id: " << hit_id << std::endl;
            std::cout << "miss_id: " << miss_id << std::endl;
            std::cout << node->bbox << std::endl;
            #endif
            
            ubo.sub(&(node->leaf), sizeof(int), offset + offset0);
            ubo.sub(&obj_idx, sizeof(int), offset + offset0 + 4);
            ubo.sub(&hit_id, sizeof(int), offset + offset0 + 8);
            ubo.sub(&miss_id, sizeof(int), offset + offset0 + 12);
            ubo.sub(&(node->bbox.x.min), sizeof(float), offset + offset0 + 16);
            ubo.sub(&(node->bbox.x.max), sizeof(float), offset + offset0 + 20);
            ubo.sub(&(node->bbox.y.min), sizeof(float), offset + offset0 + 32);
            ubo.sub(&(node->bbox.y.max), sizeof(float), offset + offset0 + 36);
            ubo.sub(&(node->bbox.z.min), sizeof(float), offset + offset0 + 48);
            ubo.sub(&(node->bbox.z.max), sizeof(float), offset + offset0 + 52);

            offset0 += 64;

            queue.pop();
        }
    }

    int leaf = 0;
    std::shared_ptr<hittable> object;
    std::shared_ptr<bvh_node> left;
    std::shared_ptr<bvh_node> right;
    std::shared_ptr<bvh_node> parent;
    std::shared_ptr<bvh_node> hit_node = NULL;
    std::shared_ptr<bvh_node> miss_node = NULL;
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

    /* Given already constructed tree, build hit-or-miss links recursively. */
    void build_links(std::shared_ptr<bvh_node> next_right = NULL) {
        if (!leaf) {
            hit_node = left;
            miss_node = next_right;

            left->build_links(right);
            right->build_links(next_right);
        } else {
            hit_node = next_right;
            miss_node = hit_node;
        }
    }
};

randgen bvh_node::rand{}; 

#endif