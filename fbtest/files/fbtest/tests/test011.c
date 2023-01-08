
/*
 *  Test011
 *
 *  Copyright 2007 Sony Corporation
 *  Copyright (C) 2007 Sony Computer Entertainment Inc.
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <assert.h>
#include <unistd.h>

#include "types.h"
#include "fb.h"
#include "drawops.h"
#include "visual.h"
#include "test.h"
#include "util.h"

#define BLOCKSIZE	29

#define SLEEP_MS	5

struct linegen {
    u32 x1, y1, x2, y2;
    int dx, dy;
    int sx, sy;
    int e;
    int first;
};

static void linegen_init(struct linegen *gen, u32 x1, u32 y1, u32 x2, u32 y2)
{
    gen->x1 = x1;
    gen->y1 = y1;
    gen->x2 = x2;
    gen->y2 = y2;
    gen->dx = x2-x1;
    gen->dy = y2-y1;

    if (gen->dy < 0) {
	gen->dy = -gen->dy;
	gen->sy = -1;
    } else {
	gen->sy = 1;
    }
    if (gen->dx < 0) {
	gen->dx = -gen->dx;
	gen->sx = -1;
    } else {
	gen->sx = 1;
    }
    gen->e = gen->dx > gen->dy ? -gen->dx/2 : -gen->dy/2;
    gen->first = 1;
}

static int linegen_next(struct linegen *gen, u32 *x, u32 *y)
{
    if (gen->first)
	gen->first = 0;
    else if (gen->dx > gen->dy) {
	if (gen->x1 == gen->x2)
	    return 0;

	if (gen->dy) {
	    gen->e += gen->dy;
	    if (gen->e >= 0) {
		gen->y1 += gen->sy;
		gen->e -= gen->dx;
	    }
	}

	gen->x1 += gen->sx;
    } else {
	if (gen->y1 == gen->y2)
	    return 0;

	if (gen->dx) {
	    gen->e += gen->dx;
	    if (gen->e >= 0) {
		gen->x1 += gen->sx;
		gen->e -= gen->dy;
	    }
	}

	gen->y1 += gen->sy;
    }
    *x = gen->x1;
    *y = gen->y1;
    return 1;
}

static void move(u32 cx, u32 cy, u32 x1, u32 y1, u32 x2, u32 y2)
{
    struct linegen gen;
    u32 x, y;
    static int cnt = 0;

    linegen_init(&gen, x1, y1, x2, y2);
    while (linegen_next(&gen, &x, &y)) {
	fill_circle(cx+x, cy+y, 2, cnt & 4 ? black_pixel : white_pixel);
	fb_pan(x, y);
	wait_ms(SLEEP_MS);
	cnt++;
    }
}

static enum test_res test011_func(void)
{
    u32 x, y, cx, cy, dx, dy;
    int even;
    fb_var.yres_virtual = 3240; 
	Debug("test11 starts\n");
//	fill_rect(0, 0, fb_var.xres, fb_var.yres, black_pixel);
    fill_rect(0, 0, fb_var.xres_virtual, fb_var.yres_virtual, black_pixel);
    Debug("fill_rect fb_var.xres_virtual %d  fb_var.yres_virtual %d\n",fb_var.xres_virtual, fb_var.yres_virtual);
    Debug("fill_rect ok\n");
    for (y = 0; y < fb_var.yres_virtual; y += BLOCKSIZE) {
	even = (y / BLOCKSIZE) % 2;
	draw_hline(0, y, fb_var.xres_virtual, white_pixel);
    Debug("draw_hline ok\n");
	for (x = 0; x < fb_var.xres_virtual; x += BLOCKSIZE) {
	    if (even && x+4 < fb_var.xres_virtual &&
		y+4 <= fb_var.yres_virtual)
		fill_rect(x+4, y+4,
			  min(BLOCKSIZE-8, fb_var.xres_virtual-x-4),
			  min(BLOCKSIZE-8, fb_var.yres_virtual-y-4),
			  white_pixel);
	    even ^= 1;
//	Debug("loop fill_rect ok\n");
	}
    }
    for (x = 0; x < fb_var.xres_virtual; x += BLOCKSIZE)
	draw_vline(x, 0, fb_var.yres_virtual, white_pixel);
    Debug("draw_vline ok\n");
    cx = fb_var.xres/2;
    cy = fb_var.yres/2;

    dx = fb_fix.xpanstep ? fb_var.xres_virtual-fb_var.xres : 0;
    dy = fb_fix.ypanstep ? fb_var.yres_virtual-fb_var.yres : 0;

    /* move right */
    move(cx, cy, 0, 0, dx, 0);

    /* move down */
    move(cx, cy, dx, 0, dx, dy);

    /* move left */
    move(cx, cy, dx, dy, 0, dy);

    if (dx >= 2 && dy >= 2) {
	/* move up and right */
	move(cx, cy, 0, dy, dx/2, 0);

	/* move right and down */
	move(cx, cy, dx/2, 0, dx, dy/2);

	/* move left and down */
	move(cx, cy, dx, dy/2, dx/2, dy);

	/* move up and left */
	move(cx, cy, dx/2, dy, 0, 0);
    } else {
	/* move up */
	move(cx, cy, 0, dy, 0, 0);
    }

    wait_for_key(10);
    return TEST_OK;
}

const struct test test011 = {
    .name =	"test011",
    .desc =	"Panning test",
    .visual =	VISUAL_MONO,
    .reqs =	REQF_panning,
    .func =	test011_func,
};

