
#include "vector.h"

vec3 vec3add(const vec3 first, const vec3 second) {
    vec3 result = first;
    vec3addto(&result, &second);
    return result;
}

void vec3addto(vec3* first, const vec3* second) {
    first->x += second->x;
    first->y += second->y;
    first->z += second->z;
}


vec3 vec3sub(const vec3 first, const vec3 second) {
    vec3 result = first;
    vec3subfrom(&result, &second);
    return result;
}

void vec3subfrom(vec3* first, const vec3* second) {
    first->x -= second->x;
    first->y -= second->y;
    first->z -= second->z;
}

vec3 vec3scale(const vec3 vec, const double scalar) {
    return (vec3){
        .x = scalar * vec.x,
        .y = scalar * vec.y,
        .z = scalar * vec.z
    };
}

vec3 vec3mult(const vec3 a, const vec3 b) {
    return (vec3){
        .x = a.x*b.x,
        .y = a.y*b.y,
        .z = a.z*b.z
    };
}

double vec3dot(const vec3 a, const vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
