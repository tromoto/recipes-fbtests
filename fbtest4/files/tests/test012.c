
/*
 *  Test012
 *
 *  (C) Copyright 2007 Geert Uytterhoeven
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "fb.h"
#include "drawops.h"
#include "visual.h"
#include "test.h"
#include "util.h"


struct param {
    u32 xrange;
    u32 yrange;
    u32 size;
    pixel_t pixelmask;
};

static void fill_squares(unsigned long n, void *data)
{
    struct param *param = data;

    while (n--)
	fill_rect(lrand48() % param->xrange, lrand48() % param->yrange,
		  param->size, param->size, lrand48() & param->pixelmask);
}

static void benchmark_squares(u32 size)
{
    struct param param;
    double rate;

    param.xrange = fb_var.xres_virtual-size+1;
    param.yrange = fb_var.yres_virtual-size+1;
    param.size = size;
    param.pixelmask = (1ULL << fb_var.bits_per_pixel)-1;

    rate = benchmark(fill_squares, &param);
    if (rate < 0)
	return;

    printf("%ux%u squares: %.2f Mpixels/s\n", size, size, rate*size*size/1e6);
}

static enum test_res test012_func(void)
{
    unsigned int i;
    u32 sizes[3] = { 10, 20, 50 };
    u32 size;

    while (1)
	for (i = 0; i < sizeof(sizes)/sizeof(*sizes); i++) {
	    size = sizes[i];
	    if (size > fb_var.xres_virtual || size > fb_var.yres_virtual)
		goto out;
	    benchmark_squares(size);
	    sizes[i] *= 10;
	}

out:
    wait_for_key(10);
    return TEST_OK;
}

const struct test test012 = {
    .name =	"test012",
    .desc =	"Filling squares",
    .visual =	VISUAL_GENERIC,
    .func =	test012_func,
};

