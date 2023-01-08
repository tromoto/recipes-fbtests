#ifndef __MAIN_H
#define __MAIN_H

#define _POSIX_C_SOURCE 199506L

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#include "framebuffer.h"
#include "draw.h"
#include "draw3d.h"
#include "vector.h"

/* Main.h */

#define HALF_PI 1.57079632679

void demo_rainbow(framebuffer* fb);
ARGB_color get_rainbow(float percent, float alpha);

unsigned int iabs(int);

void test_render(camera3d cam);

#endif
