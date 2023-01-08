#ifndef _DRAW_H
#define _DRAW_H

#include <math.h>

#include "framebuffer.h"
#include "vector.h"

#define ARGB_WHITE   0xFFFFFFFFu
#define ARGB_BLACK   0xFF000000u
#define ARGB_RED     0xFFFF0000u
#define ARGB_GREEN   0xFF00FF00u
#define ARGB_BLUE    0xFF0000FFu
#define ARGB_YELLOW  0xFFFFFF00u
#define ARGB_MAGENTA 0xFFFF00FFu
#define ARGB_CYAN    0xFF00FFFFu
#define ARGB_GREY    0xFF666666u
#define ARGB_LIGHTGREY 0xFFAAAAAAu

void draw_setfb(framebuffer* fb);
framebuffer* draw_getfb();
pixel draw_getres();
void draw_setcolor(ARGB_color color);

/*void draw_line_int(int x1, int y1, int x2, int y2);*/
void draw_line(vec2 start, vec2 end);
void draw_line_aa(vec2 start, vec2 end);

void draw_point(vec2 point);
void draw_point_alpha(vec2 point, double alpha);

void draw_clear(void);
#endif
