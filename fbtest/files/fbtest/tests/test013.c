
/*
 *  Test013
 *
 *  (C) Copyright 2019 Geert Uytterhoeven
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <math.h>
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

static void fill_circles(unsigned long n, void *data)
{
    struct param *param = data;

    while (n--)
	fill_circle(param->size + lrand48() % param->xrange,
		    param->size + lrand48() % param->yrange,
		    param->size, lrand48() & param->pixelmask);
}

static void benchmark_circles(u32 size)
{
    struct param param;
    double rate;

    param.xrange = fb_var.xres_virtual-2*size;
    param.yrange = fb_var.yres_virtual-2*size;
    param.size = size;
    param.pixelmask = (1ULL << fb_var.bits_per_pixel)-1;

    rate = benchmark(fill_circles, &param);
    if (rate < 0)
	return;

    printf("R%u circles: %.2f Mpixels/s\n", size, rate*size*size*M_PI/1e6);
}

static enum test_res test013_func(void)
{
    unsigned int i;
    u32 sizes[3] = { 10, 20, 50 };
    u32 size;

    while (1)
	for (i = 0; i < sizeof(sizes)/sizeof(*sizes); i++) {
	    size = sizes[i];
	    if (size > fb_var.xres_virtual || size > fb_var.yres_virtual)
		goto out;
	    benchmark_circles(size/2);
	    sizes[i] *= 10;
	}

out:
    wait_for_key(10);
    return TEST_OK;
}

const struct test test013 = {
    .name =	"test013",
    .desc =	"Filling circles",
    .visual =	VISUAL_GENERIC,
    .func =	test013_func,
};

