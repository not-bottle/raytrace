#ifndef INTERVAL_H
#define INTERVAL_H

#include <limits>

const float infinity = std::numeric_limits<float>::infinity();

class interval {
    public:
    float min, max;

    interval() : min(+infinity), max(-infinity) {}

    interval(float min, float max) : min(min), max(max) {}

    interval(const interval &a, const interval &b) {
        min = a.min <= b.min ? a.min : b.min;
        max = a.max >= b.max ? a.max : b.max;
    }

    float size() const {
        return max - min;
    }

    bool contains(float x) const {
        return x >= min && x <= max;
    }   

    bool surrounds(float x) const {
        return x > min && x < max;
    }

    interval expand(float delta) const {
        auto padding = delta/2;
        return interval(min - padding, max + padding);
    }

    static const interval empty, universe;

    friend std::ostream& operator<< (std::ostream& out, const interval&);
};

const interval interval::empty    = interval(+infinity, -infinity);
const interval interval::universe = interval(-infinity, +infinity);

std::ostream& operator<< (std::ostream& out, const interval& i) 
{
    out << "(" << i.min << ", " << i.max << ")";

    return out;
}

#endif