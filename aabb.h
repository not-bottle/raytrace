#ifndef AABB_H
#define AABB_H

#include "vec3.h"
#include "interval.h"

class aabb {
    public:
    interval x, y, z;

    aabb() {};
    
    aabb(interval x, interval y, interval z)
     : x(x), y(y), z(z) {}

     aabb(const point3& a, const point3& b) {
        x = a[0] <= b[0] ? interval(a[0], b[0]) : interval(b[0], a[0]);
        y = a[1] <= b[1] ? interval(a[1], b[1]) : interval(b[1], a[1]);
        z = a[2] <= b[2] ? interval(a[2], b[2]) : interval(b[2], a[2]);
     }

     aabb(const aabb& a, const aabb&b) {
        x = interval(a.x, b.x);
        y = interval(a.y, b.y);
        z = interval(a.z, b.z);
     }

     const interval& axis_interval(int n) {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
     }

     friend std::ostream& operator<< (std::ostream& out, const aabb& bbox);
};

std::ostream& operator<< (std::ostream& out, const aabb& bbox) 
{
    out << "x" << bbox.x << std::endl;
    out << "y" << bbox.y << std::endl;
    out << "z" << bbox.z << std::endl;

    return out;
}

#endif