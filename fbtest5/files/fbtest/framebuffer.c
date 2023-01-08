#include "framebuffer.h"

framebuffer* fb_open(int index, int flags) {
    char filename[16];
    framebuffer* fb;

    fb = malloc(sizeof(framebuffer));

    sprintf(filename, "/dev/fb%d", index);

    /* Try to open the file */
    fb->fd = open(filename, O_RDWR);
    if(fb->fd < 0) {
        printf("Failed to open /dev/fb0 - aborting\n");
        return NULL;
    }

    /* Get information about framebuffer */
    if(ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vinfo) < 0) {
        printf("ioctl failed!\n");
        return NULL;
    }
    
    fb->vinfo.bits_per_pixel = FBTEST_BPP;
    fb->vinfo.yres_virtual = 2*fb->vinfo.yres_virtual;
    printf("Doubling buffer to %d lines (if possible)... ", fb->vinfo.yres_virtual);
    if(ioctl(fb->fd, FBIOPUT_VSCREENINFO, &fb->vinfo) < 0) {
        printf("ioctl failed!\n");
        return NULL;
    };
    fb->width = fb->vinfo.xres;
    fb->height = fb->vinfo.yres;

    printf("%d lines\n", fb->vinfo.yres_virtual);
    ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->finfo);

    /* Calculate framebuffer length in bytes */
    fb->size = fb->vinfo.yres_virtual * fb->finfo.line_length;

    printf("Framebuffer length is 0x%zx (%zd) bytes\nPerforming mmap... ", fb->size, fb->size);

    /* Map the framebuffer into memory*/
    fb->buffer = (uint32_t*)mmap(NULL, fb->size, PROT_READ|PROT_WRITE, MAP_SHARED, fb->fd, 0);
    if(fb->buffer==(uint32_t*)-1) {
        printf("Failed to mmap, bailing out\n");
        exit(1);
    }
    printf("mapped to %p\n", fb->buffer);

    return fb;
}

int fb_close(framebuffer* fb) {
    
    if(fb != NULL && fb->buffer != NULL) {
        munmap(fb->buffer, fb->size);
        fb->buffer = NULL;
        close(fb->fd);
        free(fb);
    }
}

void fb_fill(framebuffer* fb, ARGB_color color) {
    const int limit = fb->size>>2;
    int i=0;
    for(i=0; i < limit; i++) fb->buffer[i]=color;
}

int _getindex(framebuffer* fb, pixel px) {
    const int maxindex = fb->size>>2;
    /* This assumes that the buffer is 32bpp */
    int index=(px.x+fb->vinfo.xoffset) + (px.y+fb->vinfo.yoffset) * fb->vinfo.xres_virtual;
#ifdef DEBUG_TRAP_OOB
    if(index<0 || index >= fb->size>>2) {
        printf("Warning: Out of bounds write detected (x=%d, y=%d), ignoring\n", px.x, px.y);
    }
#endif
    return (index>=maxindex)?maxindex:(index<0?0:index);
}

void fb_setpixel(framebuffer* fb, pixel px, ARGB_color value) {
    fb->buffer[_getindex(fb,px)] = value;
}

void fb_blend_over(framebuffer* fb, pixel px, ARGB_color value) {
    int index = _getindex(fb,px);

    ARGB bg, fg, color;
    bg.argb = fb->buffer[index];
    fg.argb = value;

    float alpha = (float)fg.a / 255;
    color.r = fg.r*alpha + bg.r*(1-alpha);
    color.g = fg.g*alpha + bg.g*(1-alpha);
    color.b = fg.b*alpha + bg.b*(1-alpha);
    color.a = 255;

    fb->buffer[index] = color.argb;
}

ARGB_color fb_getpixel(framebuffer* fb, pixel px) {
    return fb->buffer[_getindex(fb,px)];
}

void fb_clear(framebuffer* fb) {
    memset(fb->buffer, 0, fb->size);
}

ARGB_color fb_rgb(framebuffer* fb, uint8_t r, uint8_t g, uint8_t b) {
    return (r<<fb->vinfo.red.offset) |
            (g<<fb->vinfo.green.offset) |
            (b<<fb->vinfo.blue.offset);
}

ARGB_color fb_rgba(framebuffer* fb, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return fb_rgb(fb, r, g, b) | (a<<fb->vinfo.transp.offset);
}
