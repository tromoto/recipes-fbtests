#ifndef __FRAMEBUFFER_H
#define __FRAMEBUFFER_H

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FBTEST_FLAGS_NONE 0
#define FBTEST_BPP 32;

//#define DEBUG_TRAP_OOB

typedef struct {
    int x;
    int y;
} pixel;

typedef struct {
    int fd;
    uint16_t width, height;
    uint32_t* buffer;
    size_t size;
    uint32_t flags;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
} framebuffer;

typedef uint32_t ARGB_color;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
typedef union {
    ARGB_color argb;
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    };
} ARGB;
#else
typedef union {
    ARGB_color argb;
    struct {
        uint8_t a;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
} ARGB;
#endif

framebuffer* fb_open(int index, int flags);
int fb_close(framebuffer* fb);

void fb_fill(framebuffer* fb, uint32_t color);


void fb_setpixel(framebuffer* fb, pixel px, uint32_t value);
void fb_blend_over(framebuffer* fb, pixel px, uint32_t value);
ARGB_color fb_getpixel(framebuffer* fb, pixel px);
void fb_clear(framebuffer* fb);

ARGB_color fb_rgb(framebuffer* fb, uint8_t r, uint8_t g, uint8_t b);
ARGB_color fb_rgba(framebuffer* fb, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

#endif
