#ifndef __DRAW3D_H
#define __DRAW3D_H

#include <stdio.h>
#include <math.h>

#include "vector.h"
#include "draw.h"

typedef struct {
    vec3 pos;
    vec3 rot;
    double fov;
    double distort;
} camera3d;


// Function defs

void draw3d_point(vec3 point, camera3d cam);
void draw3d_point_alpha(vec3 point, camera3d cam, double alpha);
void draw3d_line(vec3 start, vec3 end, camera3d cam);
void draw3d_screen(vec2 pos);

#endif
