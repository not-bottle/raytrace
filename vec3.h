#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <cstdlib>
#include <iostream>

// Constants

const double pi = 3.1415926535897932385;

// Utility Functions

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double random_double() {
    // Returns a random real in [0,1).
    return std::rand() / (RAND_MAX + 1.0);
}

inline double random_double(double min, double max) {
    // Returns a random real in [min,max).
    return min + (max-min)*random_double();
}

class vec3 {
    public:
        double e[3];

        vec3() : e{0, 0, 0} {};
        vec3(double e0, double e1, double e2) : e{e0, e1, e2} {};

        double x() const {return e[0];}
        double y() const {return e[1];}
        double z() const {return e[2];}

        vec3 operator-() const {return vec3(-e[0], e[1], -e[2]);}
        double operator[](int i) const {return e[i];} 
        double& operator[](int i) {return e[i];} // (remember, this is so I can do things like vec[0] = 2; (LH))
        // (See the += operator below. I can only do that because of this line.)

        // More LH operators (that modify and return the address)
        vec3& operator+=(const vec3 &v) {
            e[0] += v[0];
            e[1] += v[1];
            e[2] += v[2];
            return *this;
        }

        vec3& operator*=(double t) {
            e[0] *= t;
            e[1] *= t;
            e[2] *= t;
            return *this;
        }

        vec3& operator/=(double t) {
            return *this *= 1/t;
        }

        double length() const {
            return sqrt(length_squared());
        }

        double length_squared() const {
            return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
        }

        bool near_zero() const {
            // Return true if vector is close to zero in all dimensions
            auto s = 1e-8;
            return (fabs(e[0]) < s) && fabs(e[1] < s) && fabs(e[2] < 2);
        }

        static vec3 random() {
            return vec3(random_double(), random_double(), random_double());
        }

        static vec3 random(double min, double max) {
            return vec3(random_double(min, max), random_double(min, max), random_double(min, max)); 
        }
};

// vec3 Type aliases
using point3 = vec3; // 3D point
using colour = vec3; // RGB colour

// vec3 Utility Functions

std::ostream& operator<<(std::ostream &out, const vec3 &v) 
{
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

vec3 operator+(const vec3 &u, const vec3 &v) 
{
    return vec3{u[0] + v[0], u[1] + v[1], u[2] + v[2]};
}

vec3 operator-(const vec3 &u, const vec3 &v) 
{
    return vec3{u[0] - v[0], u[1] - v[1], u[2] - v[2]};
}

vec3 operator*(const vec3 &u, const vec3 &v) 
{
    return vec3{u[0] * v[0], u[1] * v[1], u[2] * v[2]};
}

vec3 operator*(const vec3 &u, double t) 
{
    return vec3{t*u[0], t*u[1], t*u[2]};
}

vec3 operator*(double t, const vec3 &u) 
{
    return u * t;
}

vec3 operator/(const vec3 &u, double t) 
{
    return (1/t) * u;
}

double dot(const vec3 &u, const vec3 &v)
{
    return u[0] * v[0]
         + u[1] * v[1]
         + u[2] * v[2];
}

vec3 cross(const vec3 &u, const vec3 &v)
{
    return vec3{u[1] * v[2] - u[2] * v[1],
                u[2] * v[0] - u[0] * v[2],
                u[0] * v[1] - u[1] * v[0] };
}

vec3 unit_vector(const vec3 &v)
{
    return v / v.length();
}

#endif