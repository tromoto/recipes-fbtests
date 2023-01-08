#include "draw.h"

framebuffer* draw_fb;
ARGB_color active_color;

const int CLIP_INSIDE = 0;
const int CLIP_LEFT = 1;
const int CLIP_RIGHT = 2;
const int CLIP_BOTTOM = 4;
const int CLIP_TOP = 8;

void draw_setfb(framebuffer* fb) {
    draw_fb = fb;
}

framebuffer* draw_getfb(framebuffer* fb) {
    return draw_fb;
}

pixel draw_getres() {
    return (pixel){draw_fb->width, draw_fb->height};
}

void draw_setcolor(ARGB_color color) {
    active_color = color;
}

typedef void (*_linedraw_func)(vec2, vec2);

int _clip_get_outcode(vec2 point) {
    int code = CLIP_INSIDE;
    int xmax = draw_fb->width-1, ymax = draw_fb->height-1;
    if(point.x<0) code |= CLIP_LEFT;
    else if(point.x>xmax) code |= CLIP_RIGHT;
    if(point.y<0) code |= CLIP_BOTTOM;
    else if(point.y>ymax) code |= CLIP_TOP;
    return code;
}

void _clip(_linedraw_func func, vec2 start, vec2 end) {
    /* Cohen-Sutherland viewport clipping */
    const int xmin = 0, ymin = 0;
    int xmax = draw_fb->width-1, ymax = draw_fb->height-1;

    int outcode0 = _clip_get_outcode(start);
    int outcode1 = _clip_get_outcode(end);
    int accept = 0;
    while(1) {
        if(!(outcode0 | outcode1)) {
            /* Line is entirely within the viewport */
            accept = 1;
            break;
        } else if(outcode0 & outcode1) {
            /* A line drawn between these points will not cross the viewport at all */
            break;
        } else {
            /* A line drawn between these points will cross the edge of the viewport */
            /* So we need to clip it */
            double x, y;

            /* at least one outcode is outside the viewport, grab the first one */
            int outside = outcode0 ? outcode0 : outcode1;
            int dx = (end.x - start.x), dy = (end.y - start.y);

            /* Find the intersection point */
            if(outside & CLIP_TOP) { // point is above the viewport
                x = start.x + dx * (ymax - start.y) / dy;
                y = ymax;
            } else if(outside & CLIP_BOTTOM) { // point is below the viewport
                x = start.x + dx * (ymin - start.y) / dy;
                y = ymin;
            } else if(outside & CLIP_RIGHT) { // point is to the right
                y = start.y + dy * (xmax - start.x) / dx;
                x = xmax;
            } else if(outside & CLIP_LEFT) { // to the left
                y = start.y + dy * (xmin - start.x) / dx;
                x = xmin;
            }

            /* Move outside point to intersection point */
            if(outside == outcode0) {
                start = (vec2){x, y};
                outcode0 = _clip_get_outcode(start);
            } else {
                end = (vec2){x, y};
                outcode1 = _clip_get_outcode(end);
            }
        }
    }

    if(accept) (*func)(start, end);
}

void _draw_line(vec2 start, vec2 end) {
    /* Implements Bresenham's line algorithm in software */
    int8_t xsign = (end.x>start.x)?1:-1, ysign = (end.y>start.y)?1:-1;

    double x0 = round(start.x), x1 = round(end.x);
    double y0 = round(start.y), y1 = round(end.y);    

    double dx = (x1-x0)*xsign, dy = (y1-y0)*ysign, error = 0.0, delta;

    int i, x=x0, y=y0;
    vec2 point;

    /* To improve accuracy, we swap x and y depending on slope */
    if(dx > dy) {
        for(i=0; i<=dx; i++, x+=xsign) {
            point = (vec2){x, y};
            draw_point(point);
            error += dy;
            while(error >= dx) {
                y += ysign;
                error -= dx;
            }
        }
    } else {
        if(dy<0.5) return; // escape if line is zero length
        for(i=0; i<=dy; i++, y+=ysign) {
            point = (vec2){x, y};
            draw_point(point);
            error += dx;
            while(error >= dy) {
                x += xsign;
                error -= dy;
            }
        }
    }
}

/*void draw_line_int(int x0, int y0, int x1, int y1) {
    _clip(&_draw_line, (double)x0, (double)y0, (double)x1, (double)y1);
}*/

void draw_line(vec2 start, vec2 end) {
    _clip(&_draw_line, start, end);
}

inline double round(const double r) { return floor(r)+0.5; }

extern inline void _draw_swap2(double* a1, double* b1, double* a2, double* b2) {
    double tmp;
    tmp=*a1; *a1=*b1; *b1=tmp;
    tmp=*a2; *a2=*b2; *b2=tmp;
}

extern inline void draw_point_asqrt(double x, double y, double alpha) {
    draw_point_alpha((vec2){x,y}, sqrt(alpha));
}

void _draw_line_aa(vec2 start, vec2 end) {
    /* Implements Xiaolin Wu's line algorithm in software */
    int steep = fabs(end.y-start.y) > fabs(end.x-start.x);
    if(steep) _draw_swap2(&start.x, &start.y, &end.x, &end.y);
    if(start.x > end.x) _draw_swap2(&start.x, &end.x, &start.y, &end.y);

    double dx = end.x - start.x, dy = end.y - start.y;
    double gradient = dy / dx;

    double xend, yend, xgap, ygap, temp;

    /* Calculate first endpoint */
    xend = round(start.x);
    yend = start.y + gradient * (xend - start.x);
    xgap = 1.0-modf(start.x+0.5, &temp); /* throw away integer part */
    ygap = modf(yend, &temp);
    int intx1 = (int)xend, inty1 = (int)temp; /* will be used in the main loop */

    /* Draw first endpoint */
    if(steep) {
        draw_point_asqrt(inty1,   intx1, (1.0-ygap)*xgap);
        draw_point_asqrt(inty1+1, intx1, ygap*xgap);
    } else {
        draw_point_asqrt(intx1, inty1, (1.0-ygap)*xgap);
        draw_point_asqrt(intx1, inty1+1, ygap*xgap);
    }

    double y_inter = yend + gradient; /* First y-intersect for main loop */

    /* Calculate second endpoint */
    xend = round(end.x);
    yend = end.y + gradient * (xend - end.x);
    xgap = modf(end.x+0.5, &temp); /* throw away integer part */
    ygap = modf(yend, &temp);
    int intx2 = (int)xend, inty2 = (int)temp;

    /* Draw second endpoint */
    if(steep) {
        draw_point_asqrt(inty2,   intx2, (1.0-ygap)*xgap);
        draw_point_asqrt(inty2+1, intx2, ygap*xgap);
    } else {
        draw_point_asqrt(intx2, inty2, (1.0-ygap)*xgap);
        draw_point_asqrt(intx2, inty2+1, ygap*xgap);
    }

    /* main loop */
    double f_yint;
    int x, i_yint;
    for(x = intx1+1; x<intx2; x++) {
        f_yint = modf(y_inter, &temp); i_yint = (int)temp;
        if(steep) {
            draw_point_asqrt(i_yint,   x, 1.0-f_yint);
            draw_point_asqrt(i_yint+1, x, f_yint);
        } else {
            draw_point_asqrt(x, i_yint, 1.0-f_yint);
            draw_point_asqrt(x, i_yint+1, f_yint);
        }
        y_inter += gradient;
    }
}

void draw_line_aa(vec2 start, vec2 end) {
    _clip(&_draw_line_aa, start, end);
}

void draw_point(vec2 point) {
    if(_clip_get_outcode(point)) return;
    fb_setpixel(draw_fb, (pixel){point.x, point.y}, active_color);
}

void draw_point_alpha(vec2 point, double alpha) {
    if(_clip_get_outcode(point)) return;
    /* Compositing */
    ARGB color;
    color.argb = active_color; color.a = alpha*255;
    fb_blend_over(draw_fb, (pixel){point.x, point.y}, color.argb);
}

void draw_clear(void) {
    fb_clear(draw_fb);
}
