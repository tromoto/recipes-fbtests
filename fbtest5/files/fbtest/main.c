#include "main.h"

int main(int argc, char* argv[], char* env[]) {
    int i, j;

    printf("Welcome to framebuffer test\n");

    framebuffer* fb = fb_open(0, FBTEST_FLAGS_NONE);

    /* Do a test */
    printf("Bits Per Pixel: %d\n"
    "Value | Off | Len | Shift\n"
    "  Red | %3d | %3d | %3d\n"
    "Green | %3d | %3d | %3d\n"
    " Blue | %3d | %3d | %3d\n"
    "Alpha | %3d | %3d | %3d\n",
    fb->vinfo.bits_per_pixel,
    fb->vinfo.red.offset, fb->vinfo.red.length, fb->vinfo.red.msb_right,
    fb->vinfo.green.offset, fb->vinfo.green.length, fb->vinfo.green.msb_right,
    fb->vinfo.blue.offset, fb->vinfo.blue.length, fb->vinfo.blue.msb_right,
    fb->vinfo.transp.offset, fb->vinfo.transp.length, fb->vinfo.transp.msb_right);
    printf("Assuming 32bpp ARGB format\n");

    draw_setfb(fb);

    camera3d cam = {
        .pos = {0.0, 2.0, -10.0},
        .rot = {0.0, 0.0, 0.0},
        .fov = 1.0,
        .distort = 0.0
    }; /* camera position/rotation */

    test_render(cam);

    /* Get a nice keyboard. Later we might use raw mode */
    static struct termios old_t, new_t;
    tcgetattr(STDIN_FILENO, &old_t);
    new_t = old_t;
    new_t.c_lflag &= ~(ICANON|ECHO); /* disable ICANON and ECHO */
    /* cfmakeraw(new_t); */
    tcsetattr(STDIN_FILENO, TCSANOW, &new_t);


    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("\033[H\033[?25l");

    while(1) {
        char input = getchar();
        double t = cam.rot.y;
        switch(input) {
            case '\033':
                if(getchar() != '[') break;
                if(getchar() != '1') break;
                if(getchar() != '~') break;
            case '7':
                cam.pos.x = 0.0;
                cam.pos.y = 2.0;
                cam.pos.z = 0.0;
            case '1':
                cam.rot.y = 0.0;
                cam.rot.x = 0.0;
                cam.fov = 1.0;
                cam.distort = 0.0;
                break;
            case '4':
                cam.rot.y -= 0.1/cam.fov;
                break;
            case '6':
                cam.rot.y += 0.1/cam.fov;
                break;
            case '8':
                cam.rot.x += 0.1/cam.fov;
                if(cam.rot.x > HALF_PI) cam.rot.x = HALF_PI;
                break;
            case '5':
            case '2':
                cam.rot.x -= 0.1/cam.fov;
                if(cam.rot.x < -HALF_PI) cam.rot.x = -HALF_PI;
                break;
            case '+':
                cam.fov *= 1.25;
                break;
            case '-':
                cam.fov *= 0.8;
                break;
            case 'w':
                cam.pos.z += 0.8*cos(t);
                cam.pos.x += 0.8*sin(t);
                break;
            case 's':
                cam.pos.z -= 0.8*cos(t);
                cam.pos.x -= 0.8*sin(t);
                break;
            case 'a':
                cam.pos.z += 0.8*sin(t);
                cam.pos.x -= 0.8*cos(t);
                break;
            case 'd':
                cam.pos.z -= 0.8*sin(t);
                cam.pos.x += 0.8*cos(t);
                break;
            case 'r':
                cam.pos.y += 0.8;
                break;
            case 'f':
                cam.pos.y -= 0.8;
                break;
            case '*':
                cam.distort += 0.1;
                break;
            case '/':
                cam.distort -= 0.1;
                break;
            case 'q':
                goto close;
        }
        test_render(cam);
    }
    close:
    //printf("\033[2J\033[H\033[?25h");
    printf("\033[?25h");
    setvbuf(stdout, NULL, _IOFBF, 0);
    setvbuf(stdin, NULL, _IOFBF, 0);
    tcsetattr(STDIN_FILENO, TCSANOW, &old_t);
    fb_close(fb);
    return 0;
}

double fast_rsqrt(double number) {
    int64_t i;
    double x2, y;
    const double threehalfs = 1.5;

    x2 = number * 0.5;
    y  = number;
    i  = *(int64_t*) &y;                        // evil floating point bit level hacking
    i  = 0x5fe6eb50c7b537a9 - ( i >> 1 );       // what the fuck? 
    y  = *(double*) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//  y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

    return y;
}

unsigned int fastDist(int x, int y) {
    unsigned int dx = iabs(x), dy = iabs(y), w;
    if(dy < dx) {
        w = dy >> 2;
        return (dx + w + (w >> 1));
    } else {
        w = dx >> 2;
        return (dy + w + (w >> 1));
    }
}

unsigned int fastDist2(int x, int y) {
    unsigned int dx = iabs(x), dy = iabs(y), min, max;
    min = (dx<dy)?dx:dy;
    max = (dx<dy)?dy:dx;

    // coefficients equivalent to ( 123/128 * max ) and ( 51/128 * min )
    return ((( max << 8 ) + ( max << 3 ) - ( max << 4 ) - ( max << 1 ) +
         ( min << 7 ) - ( min << 5 ) + ( min << 3 ) - ( min << 1 )) >> 8 );
}

unsigned int iabs(int val) {
    int temp = val >> 8*sizeof(int)-1;
    val ^= temp;
    //return val + temp&1;
    return val - temp;
}

void test_render(camera3d cam) {
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);


    draw_clear();
    pixel res = draw_getres();

    /* Draw horizon line */
    draw_setcolor(ARGB_GREY);
    double hpos = (0.5+tan(cam.rot.x)*cam.fov)*res.y;
    draw_line((vec2){0.0, hpos}, (vec2){res.x, hpos});

    //*
    const double plane = 0.0;
    // Define a set of lines to draw
    double lines[23][6] = {
        {-50.0, plane, -50.0, -50.0, plane, 50.0},
        {-40.0, plane, -50.0, -40.0, plane, 50.0},
        {-30.0, plane, -50.0, -30.0, plane, 50.0},
        {-20.0, plane, -50.0, -20.0, plane, 50.0},
        {-10.0, plane, -50.0, -10.0, plane, 50.0},
        {0.0, plane, -50.0, 0.0, plane, 50.0},
        {10.0, plane, -50.0, 10.0, plane, 50.0},
        {20.0, plane, -50.0, 20.0, plane, 50.0},
        {30.0, plane, -50.0, 30.0, plane, 50.0},
        {40.0, plane, -50.0, 40.0, plane, 50.0},
        {50.0, plane, -50.0, 50.0, plane, 50.0},
        {-50.0, plane, -50.0, 50.0, plane, -50.0},
        {-50.0, plane, -40.0, 50.0, plane, -40.0},
        {-50.0, plane, -30.0, 50.0, plane, -30.0},
        {-50.0, plane, -20.0, 50.0, plane, -20.0},
        {-50.0, plane, -10.0, 50.0, plane, -10.0},
        {-50.0, plane, 0.0, 50.0, plane, 0.0},
        {-50.0, plane, 10.0, 50.0, plane, 10.0},
        {-50.0, plane, 20.0, 50.0, plane, 20.0},
        {-50.0, plane, 30.0, 50.0, plane, 30.0},
        {-50.0, plane, 40.0, 50.0, plane, 40.0},
        {-50.0, plane, 50.0, 50.0, plane, 50.0},
        {NAN, NAN, NAN, NAN, NAN, NAN}
    };
    for(double* line=(double*)lines;!isnan(line[0]);line+=6) {
        vec3 start = {line[0], line[1], line[2]};
        vec3 end = {line[3], line[4], line[5]};
        draw3d_line(start, end, cam);
    }
    // */

    /* Draw point grid */
    draw_setcolor(ARGB_WHITE);
    int sx = (int)cam.pos.x & 0xFFFFFFFE,
        sz = (int)cam.pos.z & 0xFFFFFFFE;
    const int GRIDSIZE = 160;
    int endx = sx+GRIDSIZE, endz = sz+GRIDSIZE;
    double alpha;

    for(int x=sx-GRIDSIZE; x<=endx; x+=2) {
        for(int z=sz-GRIDSIZE; z<=endz; z+=2) {
            if(!((x&15)&&(z&15))) {
                //int dx = iabs(sx-x), dz = iabs(sz-z);
                //alpha = ((double)dx / GRIDSIZE) * ((double)dz / GRIDSIZE);

                alpha = 1.0 - (double)fastDist2(sx-x, sz-z) / GRIDSIZE;

                if(alpha > 0.0) draw3d_point_alpha((vec3){x, 0.0, z}, cam, alpha*alpha);
            }
        }
    }

    /* Draw a cube */
    /* Define a set of lines to draw */
    double cube[13][6] = {
        {5.5, 0.0, 5.5, 6.5, 0.0, 5.5},
        {5.5, 0.0, 5.5, 5.5, 0.0, 6.5},
        {6.5, 0.0, 5.5, 6.5, 0.0, 6.5},
        {5.5, 0.0, 6.5, 6.5, 0.0, 6.5},
        {5.5, 1.0, 5.5, 6.5, 1.0, 5.5},
        {5.5, 1.0, 5.5, 5.5, 1.0, 6.5},
        {6.5, 1.0, 5.5, 6.5, 1.0, 6.5},
        {5.5, 1.0, 6.5, 6.5, 1.0, 6.5},
        {5.5, 0.0, 5.5, 5.5, 1.0, 5.5},
        {5.5, 0.0, 6.5, 5.5, 1.0, 6.5},
        {6.5, 0.0, 5.5, 6.5, 1.0, 5.5},
        {6.5, 0.0, 6.5, 6.5, 1.0, 6.5},
        {NAN, NAN, NAN, NAN, NAN, NAN}
    };
    for(double* line=(double*)cube;!isnan(line[0]);line+=6) {
        vec3 start = {line[0], line[1], line[2]};
        vec3 end = {line[3], line[4], line[5]};
        draw3d_line(start, end, cam);
    }

    clock_gettime(CLOCK_MONOTONIC, &t2);
    long nanos = (1000000000L*t2.tv_sec + t2.tv_nsec) - (1000000000L*t1.tv_sec + t1.tv_nsec);
    float msec = nanos/1000000.0;
    printf("\033[HFrame: %4.2fms", msec);
}


void demo_rainbow(framebuffer* fb) {
    pixel px;
    for(px.y=0; px.y<120; px.y++) {
        for(px.x=0; px.x<fb->width; px.x++) {
            float alpha = px.y<20?1.f:1.f-((px.y-20)/100.f);
            ARGB_color color = get_rainbow((float)px.x/(float)fb->width, alpha);
            fb_blend_over(fb, px, color);
        }
    }
}

ARGB_color get_rainbow(float percent, float alpha) {
    uint8_t red, green, blue;
    float redf, greenf, bluef;

    redf = 0.0; greenf = 0.0; bluef = 0.0;

    double band = 0;
    double hue = modf(percent*6.0, &band);

    switch((int)band%6) {
        case 0:
            redf = 1.f; greenf = hue;
            break;
        case 1:
            redf = 1.f-hue; greenf = 1.f;
            break;
        case 2:
            greenf = 1.f; bluef = hue;
            break;
        case 3:
            greenf = 1.f-hue; bluef = 1.f;
            break;
        case 4:
            bluef = 1.f; redf = hue;
            break;
        case 5:
            bluef = 1.f-hue; redf = 1.f;
            break;
    }
    red = (uint8_t)(redf*255);
    green = (uint8_t)(greenf*255);
    blue = (uint8_t)(bluef*255);

    return (uint8_t)(alpha*255)<<24 | red<<16 | green<<8 | blue;
}
