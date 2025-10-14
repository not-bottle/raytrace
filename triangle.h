#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <memory>
#include <glm/glm.hpp>

#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "aabb.h"
#include "interval.h"

// std140 layout size
// Must be a multiple of 4N where N = 4 bytes
// P[3] = 3*16 (vec3 stored as vec4)
// normal = 16 ** Removed from UBO for size reasons
// mat = 4
// 4*16 + 4 = 68, pad to 80
const int SIZE = 80;

class triangle : public hittable {
    public:
    vec3 P[3];
    vec3 normal;
    std::shared_ptr<material> mat;
    aabb bbox;

    triangle(vec3 a, vec3 b, vec3 c, std::shared_ptr<material> material) : hittable{SIZE, TRIANGLE}, mat{material},
        P{vec3(a), vec3(b), vec3(c)}
    {
        // CCW?
        vec3 ac = c - a;
        vec3 ab = b - a;
        normal = vec3(unit_vector(cross(ac, ab)));

        interval x = interval((a.x() < b.x()) ? ((a.x() < c.x()) ? a.x() : c.x()) : ((b.x() < c.x()) ? b.x() : c.x()), 
                                (a.x() > b.x()) ? ((a.x() > c.x()) ? a.x() : c.x()) : ((b.x() > c.x()) ? b.x() : c.x()));
        interval y = interval((a.y() < b.y()) ? ((a.y() < c.y()) ? a.y() : c.y()) : ((b.y() < c.y()) ? b.y() : c.y()), 
                                (a.y() > b.y()) ? ((a.y() > c.y()) ? a.y() : c.y()) : ((b.y() > c.y()) ? b.y() : c.y()));
        interval z = interval((a.z() < b.z()) ? ((a.z() < c.z()) ? a.z() : c.z()) : ((b.z() < c.z()) ? b.z() : c.z()), 
                                (a.z() > b.z()) ? ((a.z() > c.z()) ? a.z() : c.z()) : ((b.z() > c.z()) ? b.z() : c.z()));
        bbox = aabb(x, y, z);
    }

    aabb bounding_box() const override { return bbox; }

    virtual void toUBO(UBO ubo, int offset) override {
        ubo.subVec3(P[0], offset + 0);
        ubo.subVec3(P[1], offset + 16);
        ubo.subVec3(P[2], offset + 32);
        ubo.subVec3(P[3], offset + 48);
        ubo.sub(&(mat->id), sizeof(int), offset + 64);
    }
};

#endif // TRIANGLE_H

