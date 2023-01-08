#ifndef __VECTOR_H
#define __VECTOR_H

typedef struct {
    double x;
    double y;
} vec2;

typedef struct {
    double x; //(Euler: pitch)
    double y; //(Euler: yaw)
    double z; //(Euler: roll)
} vec3;

typedef struct {
    double x;
    double y;
    double z;
    double w;
} vec4;

vec3 vec3add(const vec3, const vec3);
void vec3addto(vec3*, const vec3*);
vec3 vec3sub(const vec3, const vec3);
void vec3subfrom(vec3*, const vec3*);
vec3 vec3scale(const vec3, const double);
vec3 vec3mult(const vec3, const vec3);
double vec3dot(const vec3, const vec3);

#endif
