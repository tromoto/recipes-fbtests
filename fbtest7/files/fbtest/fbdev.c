/*
 * fbdev -- Frame buffer device test application
 *
 * Copyright (C) 2011 Laurent Pinchart <laurent.pinchart@ideasonboard.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <linux/fb.h>
#include <linux/videodev2.h>

#define min(a, b)	((a) < (b) ? (a) : (b))
#define ARRAY_SIZE(a)	(sizeof(a)/sizeof((a)[0]))

struct device {
	int fd;
	void *mem;

	struct fb_fix_screeninfo fix_info;
	struct fb_var_screeninfo var_info;

	struct fb_bitfield red;
	struct fb_bitfield green;
	struct fb_bitfield blue;
	struct fb_bitfield alpha;

	unsigned int fourcc;
};

enum fb_fill_mode {
	FB_FILL_NONE = 0,
	FB_FILL_DISPLAY = 1,
	FB_FILL_VIRTUAL = 2,
};

enum fb_fill_pattern {
	FB_PATTERN_SMPTE = 0,
	FB_PATTERN_LINES = 1,
	FB_PATTERN_GRADIENT = 2,
};

/* -----------------------------------------------------------------------------
 * Format helpers
 */

struct fb_format_info {
	unsigned int fourcc;
	const char *name;
};

#ifndef V4L2_PIX_FMT_NV24
#define V4L2_PIX_FMT_NV24	v4l2_fourcc('N', 'V', '2', '4')
#define V4L2_PIX_FMT_NV42	v4l2_fourcc('N', 'V', '4', '2')
#endif

static const struct fb_format_info formats[] = {
	{ V4L2_PIX_FMT_BGR24, "BGR24" },
	{ V4L2_PIX_FMT_BGR32, "BGR32" },
	{ V4L2_PIX_FMT_NV12, "NV12" },
	{ V4L2_PIX_FMT_NV16, "NV16" },
	{ V4L2_PIX_FMT_NV21, "NV21" },
	{ V4L2_PIX_FMT_NV24, "NV24" },
	{ V4L2_PIX_FMT_NV42, "NV42" },
	{ V4L2_PIX_FMT_NV61, "NV61" },
	{ V4L2_PIX_FMT_RGB24, "RGB24" },
	{ V4L2_PIX_FMT_RGB32, "RGB32" },
	{ V4L2_PIX_FMT_RGB565, "RGB565" },
};

#ifdef FB_VISUAL_FOURCC
static const char *fb_format_name(unsigned int fourcc)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(formats); ++i) {
		if (formats[i].fourcc == fourcc)
			return formats[i].name;
	}

	return "Unknown";
}
#endif

/*
 * fb_var_to_fourcc - Return the format fourcc associated with the var info
 * @dev: FB device
 * @var_info: Variable screen information
 *
 * If the device doesn't support the fourcc-based API, or if the format is a
 * legacy RGB format, emulate fourcc support based on the bits per pixel value.
 *
 * Return the format fourcc or 0 if the format isn't known.
 */
static unsigned int fb_var_to_fourcc(const struct device *dev,
				     const struct fb_var_screeninfo *var_info)
{
#ifdef FB_VISUAL_FOURCC
	if (dev->fix_info.visual == FB_VISUAL_FOURCC)
		return var_info->grayscale;
#else
	(void)dev;
#endif

	switch (var_info->bits_per_pixel) {
	case 16:
		return V4L2_PIX_FMT_RGB565;
	case 24:
		if (var_info->red.offset == 0)
			return V4L2_PIX_FMT_RGB24;
		else
			return V4L2_PIX_FMT_BGR24;
	case 32:
		if (var_info->red.offset == 0)
			return V4L2_PIX_FMT_RGB32;
		else
			return V4L2_PIX_FMT_BGR32;
	default:
		return 0;
	}
}

/*
 * fb_fourcc_to_rgba - Fill the given var info RGBA bitfields based on a fourcc
 * @dev: FB device
 * @fourcc: Format fourcc
 * @var_info: Variable screen information
 *
 * Fill the bits_per_pixel, red, green, blue and transp fields based on the
 * fourcc.
 *
 * Return -EINVAL if the fourcc doesn't correspond to an RGB format or 0
 * otherwise.
 */
static int fb_fourcc_to_rgba(unsigned int fourcc,
			     struct fb_var_screeninfo *var_info)
{
	var_info->grayscale = 0;

	switch (fourcc) {
	case V4L2_PIX_FMT_BGR24:
	case 24:
		var_info->bits_per_pixel = 24;
		var_info->red.offset = 16;
		var_info->red.length = 8;
		var_info->red.msb_right = 0;
		var_info->green.offset = 8;
		var_info->green.length = 8;
		var_info->green.msb_right = 0;
		var_info->blue.offset = 0;
		var_info->blue.length = 8;
		var_info->blue.msb_right = 0;
		var_info->transp.offset = 0;
		var_info->transp.length = 0;
		var_info->transp.msb_right = 0;
		break;
	case V4L2_PIX_FMT_BGR32:
	case 32:
		var_info->bits_per_pixel = 32;
		var_info->red.offset = 16;
		var_info->red.length = 8;
		var_info->red.msb_right = 0;
		var_info->green.offset = 8;
		var_info->green.length = 8;
		var_info->green.msb_right = 0;
		var_info->blue.offset = 0;
		var_info->blue.length = 8;
		var_info->blue.msb_right = 0;
		var_info->transp.offset = 24;
		var_info->transp.length = 8;
		var_info->transp.msb_right = 0;
		break;
	case V4L2_PIX_FMT_RGB24:
		var_info->bits_per_pixel = 24;
		var_info->red.offset = 0;
		var_info->red.length = 8;
		var_info->red.msb_right = 0;
		var_info->green.offset = 8;
		var_info->green.length = 8;
		var_info->green.msb_right = 0;
		var_info->blue.offset = 16;
		var_info->blue.length = 8;
		var_info->blue.msb_right = 0;
		var_info->transp.offset = 0;
		var_info->transp.length = 0;
		var_info->transp.msb_right = 0;
		break;
	case V4L2_PIX_FMT_RGB32:
		var_info->bits_per_pixel = 32;
		var_info->red.offset = 0;
		var_info->red.length = 8;
		var_info->red.msb_right = 0;
		var_info->green.offset = 8;
		var_info->green.length = 8;
		var_info->green.msb_right = 0;
		var_info->blue.offset = 16;
		var_info->blue.length = 8;
		var_info->blue.msb_right = 0;
		var_info->transp.offset = 24;
		var_info->transp.length = 8;
		var_info->transp.msb_right = 0;
		break;
	case V4L2_PIX_FMT_RGB565:
	case 16:
		var_info->bits_per_pixel = 16;
		var_info->red.offset = 11;
		var_info->red.length = 5;
		var_info->red.msb_right = 0;
		var_info->green.offset = 5;
		var_info->green.length = 6;
		var_info->green.msb_right = 0;
		var_info->blue.offset = 0;
		var_info->blue.length = 5;
		var_info->blue.msb_right = 0;
		var_info->transp.offset = 0;
		var_info->transp.length = 0;
		var_info->transp.msb_right = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * fb_fourcc_to_var - Fill the given var info format based on a fourcc
 * @dev: FB device
 * @var_info: Variable screen information
 * @fourcc: Format fourcc
 *
 * If the device doesn't support the fourcc-based API, fill the red, green, blue
 * and transp components fields based on the fourcc.
 *
 * If the fourcc is smaller than or equal to 32, it will be interpreted as a bpp
 * value and the legacy API will be used.
 *
 * Return -EINVAL if the device doesn't support the fourcc-based API and the
 * given fourcc isn't an RGB format.
 */
static int fb_fourcc_to_var(const struct device *dev, unsigned int fourcc,
			    struct fb_var_screeninfo *var_info)
{
#ifdef FB_CAP_FOURCC
	if (dev->fix_info.capabilities & FB_CAP_FOURCC && fourcc > 32) {
		memset(&var_info->red, 0, sizeof var_info->red);
		memset(&var_info->green, 0, sizeof var_info->green);
		memset(&var_info->blue, 0, sizeof var_info->blue);
		memset(&var_info->transp, 0, sizeof var_info->transp);
		var_info->grayscale = fourcc;
		return 0;
	}
#else
	(void)dev;
#endif

	return fb_fourcc_to_rgba(fourcc, var_info);
}

/* -----------------------------------------------------------------------------
 * FB information display
 */

struct fb_value_name {
	unsigned int value;
	const char *name;
};

static const struct fb_value_name fb_type_names[] = {
	{ FB_TYPE_PACKED_PIXELS, "Packed Pixels" },
	{ FB_TYPE_PLANES, "Non Interleaved Planes" },
	{ FB_TYPE_INTERLEAVED_PLANES, "Interleaved Planes" },
	{ FB_TYPE_TEXT, "Text/Attributes" },
	{ FB_TYPE_VGA_PLANES, "EGA/VGA Planes" },
#ifdef FB_TYPE_FOURCC
	{ FB_TYPE_FOURCC, "FourCC" },
#endif
};

static const struct fb_value_name fb_visual_names[] = {
	{ FB_VISUAL_MONO01, "Monochrome (white 0, black 1)" },
	{ FB_VISUAL_MONO10, "Monochrome (white 1, black 0)" },
	{ FB_VISUAL_TRUECOLOR, "True Color" },
	{ FB_VISUAL_PSEUDOCOLOR, "Pseudo Color" },
	{ FB_VISUAL_DIRECTCOLOR, "Direct Color" },
	{ FB_VISUAL_STATIC_PSEUDOCOLOR, "Pseudo Color (read-only)" },
#ifdef FB_VISUAL_FOURCC
	{ FB_VISUAL_FOURCC, "FourCC" },
#endif
};

static const struct fb_value_name fb_accel_names[] = {
	{ FB_ACCEL_NONE, "no hardware accelerator" },
	{ FB_ACCEL_ATARIBLITT, "Atari Blitter" },
	{ FB_ACCEL_AMIGABLITT, "Amiga Blitter" },
	{ FB_ACCEL_S3_TRIO64, "Cybervision64 (S3 Trio64)" },
	{ FB_ACCEL_NCR_77C32BLT, "RetinaZ3 (NCR 77C32BLT)" },
	{ FB_ACCEL_S3_VIRGE, "Cybervision64/3D (S3 ViRGE)" },
	{ FB_ACCEL_ATI_MACH64GX, "ATI Mach 64GX family" },
	{ FB_ACCEL_DEC_TGA, "DEC 21030 TGA" },
	{ FB_ACCEL_ATI_MACH64CT, "ATI Mach 64CT family" },
	{ FB_ACCEL_ATI_MACH64VT, "ATI Mach 64CT family VT class" },
	{ FB_ACCEL_ATI_MACH64GT, "ATI Mach 64CT family GT class" },
	{ FB_ACCEL_SUN_CREATOR, "Sun Creator/Creator3D" },
	{ FB_ACCEL_SUN_CGSIX, "Sun cg6" },
	{ FB_ACCEL_SUN_LEO, "Sun leo/zx" },
	{ FB_ACCEL_IMS_TWINTURBO, "IMS Twin Turbo" },
	{ FB_ACCEL_3DLABS_PERMEDIA2, "3Dlabs Permedia 2" },
	{ FB_ACCEL_MATROX_MGA2064W, "Matrox MGA2064W (Millenium)" },
	{ FB_ACCEL_MATROX_MGA1064SG, "Matrox MGA1064SG (Mystique)" },
	{ FB_ACCEL_MATROX_MGA2164W, "Matrox MGA2164W (Millenium II)" },
	{ FB_ACCEL_MATROX_MGA2164W_AGP, "Matrox MGA2164W (Millenium II)" },
	{ FB_ACCEL_MATROX_MGAG100, "Matrox G100 (Productiva G100)" },
	{ FB_ACCEL_MATROX_MGAG200, "Matrox G200 (Myst, Mill, ...)" },
	{ FB_ACCEL_SUN_CG14, "Sun cgfourteen" },
	{ FB_ACCEL_SUN_BWTWO, "Sun bwtwo" },
	{ FB_ACCEL_SUN_CGTHREE, "Sun cgthree" },
	{ FB_ACCEL_SUN_TCX, "Sun tcx" },
	{ FB_ACCEL_MATROX_MGAG400, "Matrox G400" },
	{ FB_ACCEL_NV3, "nVidia RIVA 128" },
	{ FB_ACCEL_NV4, "nVidia RIVA TNT" },
	{ FB_ACCEL_NV5, "nVidia RIVA TNT2" },
	{ FB_ACCEL_CT_6555x, "C&T 6555x" },
	{ FB_ACCEL_3DFX_BANSHEE, "3Dfx Banshee" },
	{ FB_ACCEL_ATI_RAGE128, "ATI Rage128 family" },
	{ FB_ACCEL_IGS_CYBER2000, "CyberPro 2000" },
	{ FB_ACCEL_IGS_CYBER2010, "CyberPro 2010" },
	{ FB_ACCEL_IGS_CYBER5000, "CyberPro 5000" },
	{ FB_ACCEL_SIS_GLAMOUR, "SiS 300/630/540" },
	{ FB_ACCEL_3DLABS_PERMEDIA3, "3Dlabs Permedia 3" },
	{ FB_ACCEL_ATI_RADEON, "ATI Radeon family" },
	{ FB_ACCEL_I810, "Intel 810/815" },
	{ FB_ACCEL_SIS_GLAMOUR_2, "SiS 315, 650, 740" },
	{ FB_ACCEL_SIS_XABRE, "SiS 330 (\"Xabre\")" },
	{ FB_ACCEL_I830, "Intel 830M/845G/85x/865G" },
	{ FB_ACCEL_NV_10, "nVidia Arch 10" },
	{ FB_ACCEL_NV_20, "nVidia Arch 20" },
	{ FB_ACCEL_NV_30, "nVidia Arch 30" },
	{ FB_ACCEL_NV_40, "nVidia Arch 40" },
	{ FB_ACCEL_XGI_VOLARI_V, "XGI Volari V3XT, V5, V8" },
	{ FB_ACCEL_XGI_VOLARI_Z, "XGI Volari Z7" },
	{ FB_ACCEL_OMAP1610, "TI OMAP16xx" },
	{ FB_ACCEL_TRIDENT_TGUI, "Trident TGUI" },
	{ FB_ACCEL_TRIDENT_3DIMAGE, "Trident 3DImage" },
	{ FB_ACCEL_TRIDENT_BLADE3D, "Trident Blade3D" },
	{ FB_ACCEL_TRIDENT_BLADEXP, "Trident BladeXP" },
	{ FB_ACCEL_CIRRUS_ALPINE, "Cirrus Logic 543x/544x/5480" },
	{ FB_ACCEL_NEOMAGIC_NM2070, "NeoMagic NM2070" },
	{ FB_ACCEL_NEOMAGIC_NM2090, "NeoMagic NM2090" },
	{ FB_ACCEL_NEOMAGIC_NM2093, "NeoMagic NM2093" },
	{ FB_ACCEL_NEOMAGIC_NM2097, "NeoMagic NM2097" },
	{ FB_ACCEL_NEOMAGIC_NM2160, "NeoMagic NM2160" },
	{ FB_ACCEL_NEOMAGIC_NM2200, "NeoMagic NM2200" },
	{ FB_ACCEL_NEOMAGIC_NM2230, "NeoMagic NM2230" },
	{ FB_ACCEL_NEOMAGIC_NM2360, "NeoMagic NM2360" },
	{ FB_ACCEL_NEOMAGIC_NM2380, "NeoMagic NM2380" },
	{ FB_ACCEL_SAVAGE4, "S3 Savage4" },
	{ FB_ACCEL_SAVAGE3D, "S3 Savage3D" },
	{ FB_ACCEL_SAVAGE3D_MV, "S3 Savage3D-MV" },
	{ FB_ACCEL_SAVAGE2000, "S3 Savage2000" },
	{ FB_ACCEL_SAVAGE_MX_MV, "S3 Savage/MX-MV" },
	{ FB_ACCEL_SAVAGE_MX, "S3 Savage/MX" },
	{ FB_ACCEL_SAVAGE_IX_MV, "S3 Savage/IX-MV" },
	{ FB_ACCEL_SAVAGE_IX, "S3 Savage/IX" },
	{ FB_ACCEL_PROSAVAGE_PM, "S3 ProSavage PM133" },
	{ FB_ACCEL_PROSAVAGE_KM, "S3 ProSavage KM133" },
	{ FB_ACCEL_S3TWISTER_P, "S3 Twister" },
	{ FB_ACCEL_S3TWISTER_K, "S3 TwisterK" },
	{ FB_ACCEL_SUPERSAVAGE, "S3 Supersavage" },
	{ FB_ACCEL_PROSAVAGE_DDR, "S3 ProSavage DDR" },
	{ FB_ACCEL_PROSAVAGE_DDRK, "S3 ProSavage DDR-K" },
};

static const char *fb_value_name(const struct fb_value_name *names,
				 unsigned int size, unsigned int value)
{
	unsigned int i;

	for (i = 0; i < size; ++i) {
		if (names[i].value == value)
			return names[i].name;
	}

	return "Unknown";
}

static const char *fb_type_name(unsigned int type)
{
	return fb_value_name(fb_type_names, ARRAY_SIZE(fb_type_names), type);
}

static const char *fb_visual_name(unsigned int visual)
{
	return fb_value_name(fb_visual_names, ARRAY_SIZE(fb_visual_names), visual);
}

static const char *fb_accel_name(unsigned int accel)
{
	return fb_value_name(fb_accel_names, ARRAY_SIZE(fb_accel_names), accel);
}

/*
 * fb_print_fix - Print fixed screen information
 * @dev: FB device
 * @var: fixed screen information
 */
static void fb_print_fix(struct device *dev __attribute__((__unused__)),
			 struct fb_fix_screeninfo *fix)
{
	printf("--- Fixed screen info ---\n");
	printf(" Type:\t\t%s\n", fb_type_name(fix->type));
	printf(" Visual:\t%s\n", fb_visual_name(fix->visual));
	printf(" Chip/card:\t%s\n", fb_accel_name(fix->accel));

	printf(" Memory:\t%u bytes @0x%08lx\n", fix->smem_len,
		fix->smem_start);

	if (fix->xpanstep == 0)
		printf(" X Pan:\t\tUnsupported\n");
	else
		printf(" X Pan Step:\t%u\n", fix->xpanstep);

	if (fix->ypanstep == 0)
		printf(" Y Pan:\t\tUnsupported\n");
	else
		printf(" Y Pan Step:\t%u\n", fix->ypanstep);

	printf(" Line Length:\t%u bytes\n", fix->line_length);
}

/*
 * fb_print_var - Print fixed screen information
 * @dev: FB device
 * @var: variable screen information
 */
static void fb_print_var(struct device *dev, struct fb_var_screeninfo *var)
{
	unsigned int i;

	printf("--- Variable screen info ---\n");
	printf(" Resolution:\t\t%ux%u\n", var->xres, var->yres);
	printf(" Virtual Resolution:\t%ux%u\n", var->xres_virtual,
		var->yres_virtual);
	printf(" X/Y Offset:\t\t(%u,%u)\n", var->xoffset, var->yoffset);
	printf(" Size:\t\t\t%umm x %umm\n", var->width, var->height);

	if (dev->fix_info.visual == FB_VISUAL_TRUECOLOR) {
		printf(" Pixel organization:\t");
		for (i = var->bits_per_pixel - 1; i < var->bits_per_pixel; --i) {
			if (var->red.offset <= i && var->red.offset + var->red.length > i)
				printf("R");
			else if (var->green.offset <= i && var->green.offset + var->green.length > i)
				printf("G");
			else if (var->blue.offset <= i && var->blue.offset + var->green.length > i)
				printf("B");
			else if (var->transp.offset <= i && var->transp.offset + var->transp.length > i)
				printf("A");
		}
		printf("\n  (%u bits per pixel)\t", var->bits_per_pixel);
		for (i = var->bits_per_pixel - 1; i < var->bits_per_pixel; --i) {
			if (var->red.offset <= i &&
			    var->red.offset + var->red.length > i)
				printf("%u", var->red.msb_right ?
					var->red.offset + var->red.length - 1 - i :
					i - var->red.offset);
			else if (var->green.offset <= i &&
				 var->green.offset + var->green.length > i)
				printf("%u", var->green.msb_right ?
					var->green.offset + var->green.length - 1 - i :
					i - var->green.offset);
			else if (var->blue.offset <= i &&
				 var->blue.offset + var->green.length > i)
				printf("%u", var->blue.msb_right ?
					var->blue.offset + var->blue.length - 1 - i :
					i - var->blue.offset);
			else if (var->transp.offset <= i &&
				 var->transp.offset + var->transp.length > i)
				printf("%u", var->transp.msb_right ?
					var->transp.offset + var->transp.length - 1 - i :
					i - var->transp.offset);
		}
		printf("\n");
	}
#ifdef FB_VISUAL_FOURCC
	else if (dev->fix_info.visual == FB_VISUAL_FOURCC) {
		printf(" Format:\t\t%s (%08x)\n", fb_format_name(var->grayscale),
			var->grayscale);
	}
#endif
}

/* -----------------------------------------------------------------------------
 * Memory mapping
 */

/*
 * fb_map_memory - Map the frame buffer memory to userspace
 * @dev: FB device
 */
static int fb_map_memory(struct device *dev)
{
	void *mem;

	mem = mmap(NULL, dev->fix_info.smem_len, PROT_READ | PROT_WRITE,
		   MAP_SHARED, dev->fd, 0);
	if (mem == MAP_FAILED) {
		printf("Error: FB memory map failed: %s (%d)\n",
			strerror(errno), errno);
		return -1;
	}

	dev->mem = mem;
	return 0;
}

/*
 * fb_unmap_memory - Unmap the frame buffer memory from userspace
 * @dev: FB device
 */
static void fb_unmap_memory(struct device *dev)
{
	if (dev->mem == MAP_FAILED)
		return;

	munmap(dev->mem, dev->fix_info.smem_len);
}

/* -----------------------------------------------------------------------------
 * Open/close
 */

static int fb_update_info(struct device *dev)
{
	struct fb_var_screeninfo var_info;
	int ret;

	ret = ioctl(dev->fd, FBIOGET_FSCREENINFO, &dev->fix_info);
	if (ret < 0) {
		printf("Error updating fixed screen info.\n");
		return ret;
	}

	ret = ioctl(dev->fd, FBIOGET_VSCREENINFO, &dev->var_info);
	if (ret < 0) {
		printf("Error updating variable screen info.\n");
		return ret;
	}

	dev->fourcc = fb_var_to_fourcc(dev, &dev->var_info);

	ret = fb_fourcc_to_rgba(dev->fourcc, &var_info);
	if (ret < 0) {
		memset(&dev->red, 0, sizeof dev->red);
		memset(&dev->green, 0, sizeof dev->green);
		memset(&dev->blue, 0, sizeof dev->blue);
		memset(&dev->alpha, 0, sizeof dev->alpha);
	} else {
		dev->red = var_info.red;
		dev->green = var_info.green;
		dev->blue = var_info.blue;
		dev->alpha = var_info.transp;
	}

	return 0;
}

/*
 * fb_open - Open a frame buffer device
 * @dev: FB device
 * @devname: FB device node name and path
 *
 * Open the FB devide referenced by devname. Retrieve fixed and variable screen
 * information, map the frame buffer memory and fill the dev structure.
 */
static int fb_open(struct device *dev, const char *devname)
{
	int ret;

	memset(dev, 0, sizeof *dev);
	dev->fd = -1;
	dev->mem = MAP_FAILED;

	dev->fd = open(devname, O_RDWR);
	if (dev->fd < 0) {
		printf("Error opening device %s: %d.\n", devname, errno);
		return dev->fd;
	}

	ret = fb_update_info(dev);
	if (ret < 0) {
		printf("Error opening device %s: unable to get screen info.\n",
			devname);
		close(dev->fd);
		return ret;
	}

	printf("Device %s opened: %s\n\n", devname, dev->fix_info.id);
	fb_print_fix(dev, &dev->fix_info);
	printf("\n");
	fb_print_var(dev, &dev->var_info);
	printf("\n");

	ret = fb_map_memory(dev);
	if (ret < 0) {
		close(dev->fd);
		return ret;
	}

	printf("FB memory mapped at %p\n", dev->mem);
	return 0;
}

/*
 * fb_close - Close a frame buffer device
 * @dev: FB device
 *
 * Close a frame buffer device previously opened by fb_open().
 */
static void fb_close(struct device *dev)
{
	fb_unmap_memory(dev);
	close(dev->fd);
}

/* -----------------------------------------------------------------------------
 * Blanking and sync
 */

/*
 * fb_blank - Control screen blanking
 * @dev: FB device
 * @blank: Blanking state
 *
 * Set the frame buffer screen blanking state. Acceptable values for the blank
 * parameter are
 *
 * FB_BLANK_UNBLANK		Blanking off, screen active
 * FB_BLANK_NORMAL		Blanked, HSync on,  VSync on
 * FB_BLANK_VSYNC_SUSPEND	Blanked, HSync on,  VSync off
 * FB_BLANK_HSYNC_SUSPEND	Blanked, HSync off, VSync on
 * FB_BLANK_POWERDOWN		Blanked, HSync off, VSync off
 */
static int fb_blank(struct device *dev, int blank)
{
	int ret;

	ret = ioctl(dev->fd, FBIOBLANK, blank);
	if (ret < 0) {
		printf("Error: blank failed: %s (%d)\n", strerror(errno), errno);
		return ret;
	}

	return 0;
}

/*
 * fb_wait_for_vsync - Wait for vsync
 * @dev: FB device
 * @screen: Screen number
 *
 * Unblank the screen to make sure vsync events are generated and wait for 1000
 * 1000 vsync events on the given screen. Print the average refresh rate when
 * done.
 */
static int fb_wait_for_vsync(struct device *dev, unsigned int screen)
{
	struct timespec start, end;
	unsigned int i;
	double fps;
	int ret;

	/* Can't wait for vsync if the displayed is blanked. */
	fb_blank(dev, FB_BLANK_UNBLANK);

	printf("waiting for 1000 vsync events... ");
	fflush(stdout);

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0; i < 1000; ++i) {
		ret = ioctl(dev->fd, FBIO_WAITFORVSYNC, &screen);
		if (ret < 0) {
			printf("\nError: wait for vsync failed: %s (%d)\n",
				strerror(errno), errno);
			return ret;
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	end.tv_sec -= start.tv_sec;
	end.tv_nsec -= start.tv_nsec;
	if (end.tv_nsec < 0) {
		end.tv_sec--;
		end.tv_nsec += 1000000000;
	}

	fps = i / (end.tv_sec + end.tv_nsec / 1000000000.);

	printf("done\n");
	printf("%u vsync interrupts in %lu.%06lu s, %f Hz\n",
		i, end.tv_sec, end.tv_nsec / 1000, fps);

	return 0;
}

/* -----------------------------------------------------------------------------
 * Formats, resolution and pan
 */

/*
 * fb_set_format - Set the frame buffer pixel format
 * @dev: FB device
 * @fourcc: Format code
 */
static int fb_set_format(struct device *dev, unsigned int fourcc)
{
	struct fb_var_screeninfo var_info;
	int ret;

	var_info = dev->var_info;
	var_info.activate = FB_ACTIVATE_NOW;

	ret = fb_fourcc_to_var(dev, fourcc, &var_info);
	if (ret < 0) {
		printf("Error: device doesn't support non-RGB format %08x\n",
			fourcc);
		return ret;
	}

	ret = ioctl(dev->fd, FBIOPUT_VSCREENINFO, &var_info);
	if (ret < 0) {
		printf("Error: set format failed: %s (%d)\n",
			strerror(errno), errno);
		return ret;
	}

	dev->var_info = var_info;

	printf("Format set to %u bits per pixel\n\n",
		var_info.bits_per_pixel);

	/* Fixed screen information might have changed, re-read it. */
	fb_update_info(dev);

	fb_print_var(dev, &var_info);

	return 0;
}

/*
 * fb_set_resolution - Set the frame buffer real and virtual resolutions
 * @dev: FB device
 * @xres: Horizontal resolution
 * @yres: Vertical resolution
 * @xres_virtual: Horizontal virtual resolution
 * @yres_virtual: Vertical virtual resolution
 *
 * Modify the real and virtual resolutions of the frame buffer to (xres, yres)
 * and (xres_virtual, yres_virtual). The real or virtual resolution can be kept
 * unchanged by setting its value to (-1, -1).
 */
static int fb_set_resolution(struct device *dev, int xres, int yres,
			     int xres_virtual, int yres_virtual)
{
	struct fb_var_screeninfo var_info;
	int ret;

	var_info = dev->var_info;

	if (xres != -1 && yres != -1 ) {
		var_info.xres = xres;
		var_info.yres = yres;
	}

	if (xres_virtual != -1 && yres_virtual != -1 ) {
		var_info.xres_virtual = xres_virtual;
		var_info.yres_virtual = yres_virtual;
	}

	printf("Setting resolution to %ux%u (virtual %ux%u)\n",
		var_info.xres, var_info.yres,
		var_info.xres_virtual, var_info.yres_virtual);

	var_info.bits_per_pixel = 16;
	var_info.activate = FB_ACTIVATE_NOW;

	ret = ioctl(dev->fd, FBIOPUT_VSCREENINFO, &var_info);
	if (ret < 0) {
		printf("Error: set resolution failed: %s (%d)\n",
			strerror(errno), errno);
		return ret;
	}

	dev->var_info = var_info;

	printf("Resolution set to %ux%u (virtual %ux%u)\n\n",
		var_info.xres, var_info.yres,
		var_info.xres_virtual, var_info.yres_virtual);

	/* Fixed screen information might have changed, re-read it. */
	fb_update_info(dev);

	fb_print_var(dev, &var_info);

	return 0;
}

/*
 * fb_pan - Pan the display
 * @dev: FB device
 * @x: Horizontal offset
 * @y: Vertical offset
 *
 * Pan the display to set the virtual point (x, y) on the top left corner of the
 * screen.
 */
static int fb_pan(struct device *dev, unsigned int x, unsigned int y)
{
	struct fb_var_screeninfo var_info;
	int ret;

	memset(&var_info, 0, sizeof var_info);
	var_info.xoffset = x;
	var_info.yoffset = y;

	ret = ioctl(dev->fd, FBIOPAN_DISPLAY, &var_info);
	if (ret < 0) {
		printf("Error: pan failed: %s (%d)\n", strerror(errno), errno);
		return ret;
	}

	dev->var_info.xoffset = var_info.xoffset;
	dev->var_info.yoffset = var_info.yoffset;

	return 0;
}

/* -----------------------------------------------------------------------------
 * Test pattern
 */

struct fb_color_yuv {
	unsigned char y;
	unsigned char u;
	unsigned char v;
};

enum fb_yuv_order {
	FB_YUV_YCbCr,
	FB_YUV_YCrCb,
};

#define FB_MAKE_YUV_601_Y(r, g, b) \
	((( 66 * (r) + 129 * (g) +  25 * (b) + 128) >> 8) + 16)
#define FB_MAKE_YUV_601_U(r, g, b) \
	(((-38 * (r) -  74 * (g) + 112 * (b) + 128) >> 8) + 128)
#define FB_MAKE_YUV_601_V(r, g, b) \
	(((112 * (r) -  94 * (g) -  18 * (b) + 128) >> 8) + 128)

#define FB_MAKE_YUV_601(dev, r, g, b) \
	{ .y = FB_MAKE_YUV_601_Y(r, g, b), \
	  .u = FB_MAKE_YUV_601_U(r, g, b), \
	  .v = FB_MAKE_YUV_601_V(r, g, b) }

static void
fb_fill_smpte_nv(struct device *dev, unsigned int xoffset, unsigned int yoffset,
		 unsigned int xres, unsigned int yres,
		 unsigned int xsub, unsigned int ysub,
		 enum fb_yuv_order order)
{
	const struct fb_color_yuv colors_top[] = {
		FB_MAKE_YUV_601(dev, 192, 192, 192),	/* grey */
		FB_MAKE_YUV_601(dev, 192, 192, 0),	/* yellow */
		FB_MAKE_YUV_601(dev, 0, 192, 192),	/* cyan */
		FB_MAKE_YUV_601(dev, 0, 192, 0),	/* green */
		FB_MAKE_YUV_601(dev, 192, 0, 192),	/* magenta */
		FB_MAKE_YUV_601(dev, 192, 0, 0),	/* red */
		FB_MAKE_YUV_601(dev, 0, 0, 192),	/* blue */
	};
	const struct fb_color_yuv colors_middle[] = {
		FB_MAKE_YUV_601(dev, 0, 0, 192),	/* blue */
		FB_MAKE_YUV_601(dev, 19, 19, 19),	/* black */
		FB_MAKE_YUV_601(dev, 192, 0, 192),	/* magenta */
		FB_MAKE_YUV_601(dev, 19, 19, 19),	/* black */
		FB_MAKE_YUV_601(dev, 0, 192, 192),	/* cyan */
		FB_MAKE_YUV_601(dev, 19, 19, 19),	/* black */
		FB_MAKE_YUV_601(dev, 192, 192, 192),	/* grey */
	};
	const struct fb_color_yuv colors_bottom[] = {
		FB_MAKE_YUV_601(dev, 0, 33, 76),	/* in-phase */
		FB_MAKE_YUV_601(dev, 255, 255, 255),	/* super white */
		FB_MAKE_YUV_601(dev, 50, 0, 106),	/* quadrature */
		FB_MAKE_YUV_601(dev, 19, 19, 19),	/* black */
		FB_MAKE_YUV_601(dev, 9, 9, 9),		/* 3.5% */
		FB_MAKE_YUV_601(dev, 19, 19, 19),	/* 7.5% */
		FB_MAKE_YUV_601(dev, 29, 29, 29),	/* 11.5% */
		FB_MAKE_YUV_601(dev, 19, 19, 19),	/* black */
	};
	uint8_t *y_mem = dev->mem + dev->fix_info.line_length * yoffset
		       + xoffset;
	uint8_t *c_mem = dev->mem
		       + dev->fix_info.line_length * dev->var_info.yres_virtual
		       + dev->fix_info.line_length * yoffset / ysub
		       + xoffset * 2 / xsub;
	unsigned int u = order != FB_YUV_YCbCr;
	unsigned int v = order == FB_YUV_YCbCr;
	unsigned int x;
	unsigned int y;

	/* Luma */
	for (y = 0; y < yres * 6 / 9; ++y) {
		for (x = 0; x < xres; ++x)
			y_mem[x] = colors_top[x * 7 / xres].y;
		y_mem += dev->fix_info.line_length;
	}

	for (; y < yres * 7 / 9; ++y) {
		for (x = 0; x < xres; ++x)
			y_mem[x] = colors_middle[x * 7 / xres].y;
		y_mem += dev->fix_info.line_length;
	}

	for (; y < yres; ++y) {
		for (x = 0; x < xres * 5 / 7; ++x)
			y_mem[x] = colors_bottom[x * 4 / (xres * 5 / 7)].y;
		for (; x < xres * 6 / 7; ++x)
			y_mem[x] = colors_bottom[(x - xres * 5 / 7) * 3
						 / (xres / 7) + 4].y;
		for (; x < xres; ++x)
			y_mem[x] = colors_bottom[7].y;
		y_mem += dev->fix_info.line_length;
	}

	/* Chroma */
	for (y = 0; y < yres / ysub * 6 / 9; ++y) {
		for (x = 0; x < xres; x += xsub) {
			c_mem[2/xsub*x+u] = colors_top[x * 7 / xres].u;
			c_mem[2/xsub*x+v] = colors_top[x * 7 / xres].v;
		}
		c_mem += dev->fix_info.line_length * 2 / xsub;
	}

	for (; y < yres / ysub * 7 / 9; ++y) {
		for (x = 0; x < xres; x += xsub) {
			c_mem[2/xsub*x+u] = colors_middle[x * 7 / xres].u;
			c_mem[2/xsub*x+v] = colors_middle[x * 7 / xres].v;
		}
		c_mem += dev->fix_info.line_length * 2 / xsub;
	}

	for (; y < yres / ysub; ++y) {
		for (x = 0; x < xres * 5 / 7; x += xsub) {
			c_mem[2/xsub*x+u] =
				colors_bottom[x * 4 / (xres * 5 / 7)].u;
			c_mem[2/xsub*x+v] =
				colors_bottom[x * 4 / (xres * 5 / 7)].v;
		}
		for (; x < xres * 6 / 7; x += xsub) {
			c_mem[2/xsub*x+u] = colors_bottom[(x - xres * 5 / 7) *
							3 / (xres / 7) + 4].u;
			c_mem[2/xsub*x+v] = colors_bottom[(x - xres * 5 / 7) *
							  3 / (xres / 7) + 4].v;
		}
		for (; x < xres; x += xsub) {
			c_mem[2/xsub*x+u] = colors_bottom[7].u;
			c_mem[2/xsub*x+v] = colors_bottom[7].v;
		}
		c_mem += dev->fix_info.line_length * 2 / xsub;
	}
}

#define FB_MAKE_RGB(dev, r, g, b) \
	((((r) >> (8 - (dev)->red.length)) << (dev)->red.offset) | \
	 (((g) >> (8 - (dev)->green.length)) << (dev)->green.offset) | \
	 (((b) >> (8 - (dev)->blue.length)) << (dev)->blue.offset))

static void
fb_fill_smpte_rgb16(struct device *dev, unsigned int xoffset,
		    unsigned int yoffset, unsigned int xres, unsigned int yres)
{
	const uint16_t colors_top[] = {
		FB_MAKE_RGB(dev, 192, 192, 192),	/* grey */
		FB_MAKE_RGB(dev, 192, 192, 0),		/* yellow */
		FB_MAKE_RGB(dev, 0, 192, 192),		/* cyan */
		FB_MAKE_RGB(dev, 0, 192, 0),		/* green */
		FB_MAKE_RGB(dev, 192, 0, 192),		/* magenta */
		FB_MAKE_RGB(dev, 192, 0, 0),		/* red */
		FB_MAKE_RGB(dev, 0, 0, 192),		/* blue */
	};
	const uint16_t colors_middle[] = {
		FB_MAKE_RGB(dev, 0, 0, 192),		/* blue */
		FB_MAKE_RGB(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB(dev, 192, 0, 192),		/* magenta */
		FB_MAKE_RGB(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB(dev, 0, 192, 192),		/* cyan */
		FB_MAKE_RGB(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB(dev, 192, 192, 192),	/* grey */
	};
	const uint16_t colors_bottom[] = {
		FB_MAKE_RGB(dev, 0, 33, 76),		/* in-phase */
		FB_MAKE_RGB(dev, 255, 255, 255),	/* super white */
		FB_MAKE_RGB(dev, 50, 0, 106),		/* quadrature */
		FB_MAKE_RGB(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB(dev, 9, 9, 9),		/* 3.5% */
		FB_MAKE_RGB(dev, 19, 19, 19),		/* 7.5% */
		FB_MAKE_RGB(dev, 29, 29, 29),		/* 11.5% */
		FB_MAKE_RGB(dev, 19, 19, 19),		/* black */
	};
	void *mem = dev->mem + dev->fix_info.line_length * yoffset
		  + xoffset * dev->var_info.bits_per_pixel / 8;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < yres * 6 / 9; ++y) {
		for (x = 0; x < xres; ++x)
			((uint16_t *)mem)[x] = colors_top[x * 7 / xres];
		mem += dev->fix_info.line_length;
	}

	for (; y < yres * 7 / 9; ++y) {
		for (x = 0; x < xres; ++x)
			((uint16_t *)mem)[x] = colors_middle[x * 7 / xres];
		mem += dev->fix_info.line_length;
	}

	for (; y < yres; ++y) {
		for (x = 0; x < xres * 5 / 7; ++x)
			((uint16_t *)mem)[x] =
				colors_bottom[x * 4 / (xres * 5 / 7)];
		for (; x < xres * 6 / 7; ++x)
			((uint16_t *)mem)[x] =
				colors_bottom[(x - xres * 5 / 7) * 3
					      / (xres / 7) + 4];
		for (; x < xres; ++x)
			((uint16_t *)mem)[x] = colors_bottom[7];
		mem += dev->fix_info.line_length;
	}
}

struct fb_color24 {
	unsigned int value:24;
} __attribute__((__packed__));

#define FB_MAKE_RGB24(dev, r, g, b) \
	{ .value = FB_MAKE_RGB(dev, r, g, b) }

static void
fb_fill_smpte_rgb24(struct device *dev, unsigned int xoffset,
		    unsigned int yoffset, unsigned int xres, unsigned int yres)
{
	const struct fb_color24 colors_top[] = {
		FB_MAKE_RGB24(dev, 192, 192, 192),	/* grey */
		FB_MAKE_RGB24(dev, 192, 192, 0),	/* yellow */
		FB_MAKE_RGB24(dev, 0, 192, 192),	/* cyan */
		FB_MAKE_RGB24(dev, 0, 192, 0),		/* green */
		FB_MAKE_RGB24(dev, 192, 0, 192),	/* magenta */
		FB_MAKE_RGB24(dev, 192, 0, 0),		/* red */
		FB_MAKE_RGB24(dev, 0, 0, 192),		/* blue */
	};
	const struct fb_color24 colors_middle[] = {
		FB_MAKE_RGB24(dev, 0, 0, 192),		/* blue */
		FB_MAKE_RGB24(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB24(dev, 192, 0, 192),	/* magenta */
		FB_MAKE_RGB24(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB24(dev, 0, 192, 192),	/* cyan */
		FB_MAKE_RGB24(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB24(dev, 192, 192, 192),	/* grey */
	};
	const struct fb_color24 colors_bottom[] = {
		FB_MAKE_RGB24(dev, 0, 33, 76),		/* in-phase */
		FB_MAKE_RGB24(dev, 255, 255, 255),	/* super white */
		FB_MAKE_RGB24(dev, 50, 0, 106),		/* quadrature */
		FB_MAKE_RGB24(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB24(dev, 9, 9, 9),		/* 3.5% */
		FB_MAKE_RGB24(dev, 19, 19, 19),		/* 7.5% */
		FB_MAKE_RGB24(dev, 29, 29, 29),		/* 11.5% */
		FB_MAKE_RGB24(dev, 19, 19, 19),		/* black */
	};
	void *mem = dev->mem + dev->fix_info.line_length * yoffset
		  + xoffset * dev->var_info.bits_per_pixel / 8;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < yres * 6 / 9; ++y) {
		for (x = 0; x < xres; ++x)
			((struct fb_color24 *)mem)[x] =
				colors_top[x * 7 / xres];
		mem += dev->fix_info.line_length;
	}

	for (; y < yres * 7 / 9; ++y) {
		for (x = 0; x < xres; ++x)
			((struct fb_color24 *)mem)[x] =
				colors_middle[x * 7 / xres];
		mem += dev->fix_info.line_length;
	}

	for (; y < yres; ++y) {
		for (x = 0; x < xres * 5 / 7; ++x)
			((struct fb_color24 *)mem)[x] =
				colors_bottom[x * 4 / (xres * 5 / 7)];
		for (; x < xres * 6 / 7; ++x)
			((struct fb_color24 *)mem)[x] =
				colors_bottom[(x - xres * 5 / 7) * 3
					      / (xres / 7) + 4];
		for (; x < xres; ++x)
			((struct fb_color24 *)mem)[x] = colors_bottom[7];
		mem += dev->fix_info.line_length;
	}
}

#define FB_MAKE_RGBA(dev, r, g, b, a) \
	((((r) >> (8 - (dev)->red.length)) << (dev)->red.offset) | \
	 (((g) >> (8 - (dev)->green.length)) << (dev)->green.offset) | \
	 (((b) >> (8 - (dev)->blue.length)) << (dev)->blue.offset) | \
	 (((a) >> (8 - (dev)->alpha.length)) << (dev)->alpha.offset))

static void
fb_fill_smpte_rgb32(struct device *dev, unsigned int xoffset,
		    unsigned int yoffset, unsigned int xres, unsigned int yres,
		    uint8_t alpha)
{
	const uint32_t colors_top[] = {
		FB_MAKE_RGBA(dev, 192, 192, 192, alpha),	/* grey */
		FB_MAKE_RGBA(dev, 192, 192, 0, alpha),		/* yellow */
		FB_MAKE_RGBA(dev, 0, 192, 192, alpha),		/* cyan */
		FB_MAKE_RGBA(dev, 0, 192, 0, alpha),		/* green */
		FB_MAKE_RGBA(dev, 192, 0, 192, alpha),		/* magenta */
		FB_MAKE_RGBA(dev, 192, 0, 0, alpha),		/* red */
		FB_MAKE_RGBA(dev, 0, 0, 192, alpha),		/* blue */
	};
	const uint32_t colors_middle[] = {
		FB_MAKE_RGBA(dev, 0, 0, 192, alpha),		/* blue */
		FB_MAKE_RGBA(dev, 19, 19, 19, alpha),		/* black */
		FB_MAKE_RGBA(dev, 192, 0, 192, alpha),		/* magenta */
		FB_MAKE_RGBA(dev, 19, 19, 19, alpha),		/* black */
		FB_MAKE_RGBA(dev, 0, 192, 192, alpha),		/* cyan */
		FB_MAKE_RGBA(dev, 19, 19, 19, alpha),		/* black */
		FB_MAKE_RGBA(dev, 192, 192, 192, alpha),	/* grey */
	};
	const uint32_t colors_bottom[] = {
		FB_MAKE_RGBA(dev, 0, 33, 76, alpha),		/* in-phase */
		FB_MAKE_RGBA(dev, 255, 255, 255, alpha),	/* super white */
		FB_MAKE_RGBA(dev, 50, 0, 106, alpha),		/* quadrature */
		FB_MAKE_RGBA(dev, 19, 19, 19, alpha),		/* black */
		FB_MAKE_RGBA(dev, 9, 9, 9, alpha),		/* 3.5% */
		FB_MAKE_RGBA(dev, 19, 19, 19, alpha),		/* 7.5% */
		FB_MAKE_RGBA(dev, 29, 29, 29, alpha),		/* 11.5% */
		FB_MAKE_RGBA(dev, 19, 19, 19, alpha),		/* black */
	};
	void *mem = dev->mem + dev->fix_info.line_length * yoffset
		  + xoffset * dev->var_info.bits_per_pixel / 8;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < yres * 6 / 9; ++y) {
		for (x = 0; x < xres; ++x)
			((uint32_t *)mem)[x] = colors_top[x * 7 / xres];
		mem += dev->fix_info.line_length;
	}

	for (; y < yres * 7 / 9; ++y) {
		for (x = 0; x < xres; ++x)
			((uint32_t *)mem)[x] = colors_middle[x * 7 / xres];
		mem += dev->fix_info.line_length;
	}

	for (; y < yres; ++y) {
		for (x = 0; x < xres * 5 / 7; ++x)
			((uint32_t *)mem)[x] =
				colors_bottom[x * 4 / (xres * 5 / 7)];
		for (; x < xres * 6 / 7; ++x)
			((uint32_t *)mem)[x] =
				colors_bottom[(x - xres * 5 / 7) * 3
					      / (xres / 7) + 4];
		for (; x < xres; ++x)
			((uint32_t *)mem)[x] = colors_bottom[7];
		mem += dev->fix_info.line_length;
	}
}

/*
 * fb_fill_smpte - Fill the frame buffer with an SMPTE test pattern
 * @dev: FB device
 * @xoffset: Horizontal offset, in pixels
 * @yoffset: Vertical offset, in pixels
 * @xres: Horizontal size, in pixels
 * @yres: Vertical size, in pixels
 * @alpha: Transparency value (only for formats that support alpha values)
 *
 * Fill the display (when mode is FB_FILL_DISPLAY) or virtual frame buffer area
 * (when mode is FB_FILL_VIRTUAL) with an SMPTE color bars pattern.
 */
static void
fb_fill_smpte(struct device *dev, unsigned int xoffset, unsigned int yoffset,
	      unsigned int xres, unsigned int yres, uint8_t alpha)
{
	switch (dev->fourcc) {
	case V4L2_PIX_FMT_NV12:
		return fb_fill_smpte_nv(dev, xoffset, yoffset, xres, yres,
					2, 2, FB_YUV_YCbCr);
	case V4L2_PIX_FMT_NV21:
		return fb_fill_smpte_nv(dev, xoffset, yoffset, xres, yres,
					2, 2, FB_YUV_YCrCb);
	case V4L2_PIX_FMT_NV16:
		return fb_fill_smpte_nv(dev, xoffset, yoffset, xres, yres,
					2, 1, FB_YUV_YCbCr);
	case V4L2_PIX_FMT_NV61:
		return fb_fill_smpte_nv(dev, xoffset, yoffset, xres, yres,
					2, 1, FB_YUV_YCrCb);
	case V4L2_PIX_FMT_NV24:
		return fb_fill_smpte_nv(dev, xoffset, yoffset, xres, yres,
					1, 1, FB_YUV_YCbCr);
	case V4L2_PIX_FMT_NV42:
		return fb_fill_smpte_nv(dev, xoffset, yoffset, xres, yres,
					1, 1, FB_YUV_YCrCb);
	case V4L2_PIX_FMT_RGB565:
		return fb_fill_smpte_rgb16(dev, xoffset, yoffset, xres, yres);
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB24:
		return fb_fill_smpte_rgb24(dev, xoffset, yoffset, xres, yres);
	case V4L2_PIX_FMT_BGR32:
	case V4L2_PIX_FMT_RGB32:
		return fb_fill_smpte_rgb32(dev, xoffset, yoffset, xres, yres, alpha);
	}
}

static void
fb_fill_lines_nv(struct device *dev, unsigned int xoffset, unsigned int yoffset,
		 unsigned int xres, unsigned int yres,
		 unsigned int xsub, unsigned int ysub,
		 enum fb_yuv_order order)
{
	const struct fb_color_yuv colors[] = {
		FB_MAKE_YUV_601(dev, 192, 192, 0),	/* yellow */
		FB_MAKE_YUV_601(dev, 19, 19, 19),	/* black */
		FB_MAKE_YUV_601(dev, 0, 192, 0),	/* green */
		FB_MAKE_YUV_601(dev, 19, 19, 19),	/* black */
		FB_MAKE_YUV_601(dev, 192, 0, 0),	/* red */
		FB_MAKE_YUV_601(dev, 19, 19, 19),	/* black */
		FB_MAKE_YUV_601(dev, 0, 0, 192),	/* blue */
		FB_MAKE_YUV_601(dev, 19, 19, 19),	/* black */
	};
	uint8_t *y_mem = dev->mem + dev->fix_info.line_length * yoffset
		       + xoffset;
	uint8_t *c_mem = dev->mem
		       + dev->fix_info.line_length * dev->var_info.yres_virtual
		       + dev->fix_info.line_length * yoffset / ysub
		       + xoffset * 2 / xsub;
	unsigned int u = order != FB_YUV_YCbCr;
	unsigned int v = order == FB_YUV_YCbCr;
	unsigned int x_min;
	unsigned int y_min;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < yres; ++y) {
		y_min = min(y, yres - y - 1);
		for (x = 0; x < xres; ++x) {
			x_min = min(x, xres - x - 1);
			y_mem[x] = colors[min(x_min, y_min) %
					  ARRAY_SIZE(colors)].y;
		}
		y_mem += dev->fix_info.line_length;
	}

	for (y = 0; y < yres; y += ysub) {
		y_min = min(y, yres - y - 1);
		for (x = 0; x < xres; x += xsub) {
			x_min = min(x, xres - x - 1);
			c_mem[2/xsub*x+u] = colors[min(x_min, y_min) %
						 ARRAY_SIZE(colors)].u;
			c_mem[2/xsub*x+v] = colors[min(x_min, y_min) %
						   ARRAY_SIZE(colors)].v;
		}
		c_mem += dev->fix_info.line_length * 2 / xsub;
	}
}

static void
fb_fill_lines_rgb16(struct device *dev, unsigned int xoffset,
		    unsigned int yoffset, unsigned int xres, unsigned int yres)
{
	const uint16_t colors[] = {
		FB_MAKE_RGB(dev, 192, 192, 0),		/* yellow */
		FB_MAKE_RGB(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB(dev, 0, 192, 0),		/* green */
		FB_MAKE_RGB(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB(dev, 192, 0, 0),		/* red */
		FB_MAKE_RGB(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB(dev, 0, 0, 192),		/* blue */
		FB_MAKE_RGB(dev, 19, 19, 19),		/* black */
	};
	void *mem = dev->mem + dev->fix_info.line_length * yoffset
		  + xoffset * dev->var_info.bits_per_pixel / 8;
	unsigned int x_min;
	unsigned int y_min;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < yres; ++y) {
		y_min = min(y, yres - y - 1);
		for (x = 0; x < xres; ++x) {
			x_min = min(x, xres - x - 1);
			((uint16_t *)mem)[x] =
				colors[min(x_min, y_min) % ARRAY_SIZE(colors)];
		}
		mem += dev->fix_info.line_length;
	}
}

static void
fb_fill_lines_rgb24(struct device *dev, unsigned int xoffset,
		    unsigned int yoffset, unsigned int xres, unsigned int yres)
{
	const struct fb_color24 colors[] = {
		FB_MAKE_RGB24(dev, 192, 192, 0),	/* yellow */
		FB_MAKE_RGB24(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB24(dev, 0, 192, 0),		/* green */
		FB_MAKE_RGB24(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB24(dev, 192, 0, 0),		/* red */
		FB_MAKE_RGB24(dev, 19, 19, 19),		/* black */
		FB_MAKE_RGB24(dev, 0, 0, 192),		/* blue */
		FB_MAKE_RGB24(dev, 19, 19, 19),		/* black */
	};
	void *mem = dev->mem + dev->fix_info.line_length * yoffset
		  + xoffset * dev->var_info.bits_per_pixel / 8;
	unsigned int x_min;
	unsigned int y_min;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < yres; ++y) {
		y_min = min(y, yres - y - 1);
		for (x = 0; x < xres; ++x) {
			x_min = min(x, xres - x - 1);
			((struct fb_color24 *)mem)[x] =
				colors[min(x_min, y_min) % ARRAY_SIZE(colors)];
		}
		mem += dev->fix_info.line_length;
	}
}

static void
fb_fill_lines_rgb32(struct device *dev, unsigned int xoffset,
		    unsigned int yoffset, unsigned int xres, unsigned int yres,
		    uint8_t alpha)
{
	const uint32_t colors[] = {
		FB_MAKE_RGBA(dev, 192, 192, 0, alpha),		/* yellow */
		FB_MAKE_RGBA(dev, 19, 19, 19, alpha),		/* black */
		FB_MAKE_RGBA(dev, 0, 192, 0, alpha),		/* green */
		FB_MAKE_RGBA(dev, 19, 19, 19, alpha),		/* black */
		FB_MAKE_RGBA(dev, 192, 0, 0, alpha),		/* red */
		FB_MAKE_RGBA(dev, 19, 19, 19, alpha),		/* black */
		FB_MAKE_RGBA(dev, 0, 0, 192, alpha),		/* blue */
		FB_MAKE_RGBA(dev, 19, 19, 19, alpha),		/* black */
	};
	void *mem = dev->mem + dev->fix_info.line_length * yoffset
		  + xoffset * dev->var_info.bits_per_pixel / 8;
	unsigned int x_min;
	unsigned int y_min;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < yres; ++y) {
		y_min = min(y, yres - y - 1);
		for (x = 0; x < xres; ++x) {
			x_min = min(x, xres - x - 1);
			((uint32_t *)mem)[x] =
				colors[min(x_min, y_min) % ARRAY_SIZE(colors)];
		}
		mem += dev->fix_info.line_length;
	}
}

/*
 * fb_fill_lines - Fill the frame buffer with a lines test pattern
 * @dev: FB device
 * @xoffset: Horizontal offset, in pixels
 * @yoffset: Vertical offset, in pixels
 * @xres: Horizontal size, in pixels
 * @yres: Vertical size, in pixels
 * @alpha: Transparency value (only for formats that support alpha values)
 *
 * Fill the display (when mode is FB_FILL_DISPLAY) or virtual frame buffer area
 * (when mode is FB_FILL_VIRTUAL) with a lines pattern.
 */
static void
fb_fill_lines(struct device *dev, unsigned int xoffset, unsigned int yoffset,
	      unsigned int xres, unsigned int yres, uint8_t alpha)
{
	switch (dev->fourcc) {
	case V4L2_PIX_FMT_NV12:
		return fb_fill_lines_nv(dev, xoffset, yoffset, xres, yres,
					2, 2, FB_YUV_YCbCr);
	case V4L2_PIX_FMT_NV21:
		return fb_fill_lines_nv(dev, xoffset, yoffset, xres, yres,
					2, 2, FB_YUV_YCrCb);
	case V4L2_PIX_FMT_NV16:
		return fb_fill_lines_nv(dev, xoffset, yoffset, xres, yres,
					2, 1, FB_YUV_YCbCr);
	case V4L2_PIX_FMT_NV61:
		return fb_fill_lines_nv(dev, xoffset, yoffset, xres, yres,
					2, 1, FB_YUV_YCrCb);
	case V4L2_PIX_FMT_NV24:
		return fb_fill_lines_nv(dev, xoffset, yoffset, xres, yres,
					1, 1, FB_YUV_YCbCr);
	case V4L2_PIX_FMT_NV42:
		return fb_fill_lines_nv(dev, xoffset, yoffset, xres, yres,
					1, 1, FB_YUV_YCrCb);
	case V4L2_PIX_FMT_RGB565:
		return fb_fill_lines_rgb16(dev, xoffset, yoffset, xres, yres);
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB24:
		return fb_fill_lines_rgb24(dev, xoffset, yoffset, xres, yres);
	case V4L2_PIX_FMT_BGR32:
	case V4L2_PIX_FMT_RGB32:
		return fb_fill_lines_rgb32(dev, xoffset, yoffset, xres, yres, alpha);
	}
}

static void
fb_fill_gradient_nv(struct device *dev, unsigned int xoffset, unsigned int yoffset,
		 unsigned int xres, unsigned int yres,
		 unsigned int xsub, unsigned int ysub,
		 enum fb_yuv_order order)
{
	uint8_t *y_mem = dev->mem + dev->fix_info.line_length * yoffset
		       + xoffset;
	uint8_t *c_mem = dev->mem
		       + dev->fix_info.line_length * dev->var_info.yres_virtual
		       + dev->fix_info.line_length * yoffset / ysub
		       + xoffset * 2 / xsub;
	unsigned int u = order != FB_YUV_YCbCr;
	unsigned int v = order == FB_YUV_YCbCr;
	unsigned int x;
	unsigned int y;

	/* Luma */
	for (y = 0; y < yres / 3; ++y) {
		for (x = 0; x < xres; ++x)
			y_mem[x] = FB_MAKE_YUV_601_Y(255 * x / xres, 0, 0);
		y_mem += dev->fix_info.line_length;
	}
	for (; y < yres * 2 / 3; ++y) {
		for (x = 0; x < xres; ++x)
			y_mem[x] = FB_MAKE_YUV_601_Y(0, 255 * x / xres, 0);
		y_mem += dev->fix_info.line_length;
	}
	for (; y < yres; ++y) {
		for (x = 0; x < xres; ++x)
			y_mem[x] = FB_MAKE_YUV_601_Y(0, 0, 255 * x / xres);
		y_mem += dev->fix_info.line_length;
	}

	/* Chroma */
	for (y = 0; y < yres / 3; y += ysub) {
		for (x = 0; x < xres; x += xsub) {
			c_mem[2/xsub*x+u] = FB_MAKE_YUV_601_U(255 * x / xres, 0, 0);
			c_mem[2/xsub*x+v] = FB_MAKE_YUV_601_V(255 * x / xres, 0, 0);
		}
		c_mem += dev->fix_info.line_length * 2 / xsub;
	}
	for (; y < yres * 2 / 3; y += ysub) {
		for (x = 0; x < xres; x += xsub) {
			c_mem[2/xsub*x+u] = FB_MAKE_YUV_601_U(0, 255 * x / xres, 0);
			c_mem[2/xsub*x+v] = FB_MAKE_YUV_601_V(0, 255 * x / xres, 0);
		}
		c_mem += dev->fix_info.line_length * 2 / xsub;
	}
	for (; y < yres; y += ysub) {
		for (x = 0; x < xres; x += xsub) {
			c_mem[2/xsub*x+u] = FB_MAKE_YUV_601_U(0, 0, 255 * x / xres);
			c_mem[2/xsub*x+v] = FB_MAKE_YUV_601_V(0, 0, 255 * x / xres);
		}
		c_mem += dev->fix_info.line_length * 2 / xsub;
	}
}

static void
fb_fill_gradient_rgb16(struct device *dev, unsigned int xoffset,
		       unsigned int yoffset, unsigned int xres, unsigned int yres)
{
	void *mem = dev->mem + dev->fix_info.line_length * yoffset
		  + xoffset * dev->var_info.bits_per_pixel / 8;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < yres / 3; ++y) {
		for (x = 0; x < xres; ++x)
			((uint16_t *)mem)[x] = FB_MAKE_RGB(dev, 255 * x / xres, 0, 0);
		mem += dev->fix_info.line_length;
	}
	for (; y < yres * 2 / 3; ++y) {
		for (x = 0; x < xres; ++x)
			((uint16_t *)mem)[x] = FB_MAKE_RGB(dev, 0, 255 * x / xres, 0);
		mem += dev->fix_info.line_length;
	}
	for (; y < yres; ++y) {
		for (x = 0; x < xres; ++x)
			((uint16_t *)mem)[x] = FB_MAKE_RGB(dev, 0, 0, 255 * x / xres);
		mem += dev->fix_info.line_length;
	}
}

static void
fb_fill_gradient_rgb24(struct device *dev, unsigned int xoffset,
		       unsigned int yoffset, unsigned int xres, unsigned int yres)
{
	void *mem = dev->mem + dev->fix_info.line_length * yoffset
		  + xoffset * dev->var_info.bits_per_pixel / 8;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < yres / 3; ++y) {
		for (x = 0; x < xres; ++x)
			((struct fb_color24 *)mem)[x].value =
				FB_MAKE_RGB(dev, 255 * x / xres, 0, 0);
		mem += dev->fix_info.line_length;
	}
	for (; y < yres * 2 / 3; ++y) {
		for (x = 0; x < xres; ++x)
			((struct fb_color24 *)mem)[x].value =
				FB_MAKE_RGB(dev, 0, 255 * x / xres, 0);
		mem += dev->fix_info.line_length;
	}
	for (; y < yres; ++y) {
		for (x = 0; x < xres; ++x)
			((struct fb_color24 *)mem)[x].value =
				FB_MAKE_RGB(dev, 0, 0, 255 * x / xres);
		mem += dev->fix_info.line_length;
	}
}

static void
fb_fill_gradient_rgb32(struct device *dev, unsigned int xoffset,
		       unsigned int yoffset, unsigned int xres, unsigned int yres,
		       uint8_t alpha)
{
	void *mem = dev->mem + dev->fix_info.line_length * yoffset
		  + xoffset * dev->var_info.bits_per_pixel / 8;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < yres / 3; ++y) {
		for (x = 0; x < xres; ++x)
			((uint32_t *)mem)[x] = FB_MAKE_RGBA(dev, 255 * x / xres, 0, 0, alpha);
		mem += dev->fix_info.line_length;
	}
	for (; y < yres * 2 / 3; ++y) {
		for (x = 0; x < xres; ++x)
			((uint32_t *)mem)[x] = FB_MAKE_RGBA(dev, 0, 255 * x / xres, 0, alpha);
		mem += dev->fix_info.line_length;
	}
	for (; y < yres; ++y) {
		for (x = 0; x < xres; ++x)
			((uint32_t *)mem)[x] = FB_MAKE_RGBA(dev, 0, 0, 255 * x / xres, alpha);
		mem += dev->fix_info.line_length;
	}
}

/*
 * fb_fill_gradient - Fill the frame buffer with a gradient test pattern
 * @dev: FB device
 * @xoffset: Horizontal offset, in pixels
 * @yoffset: Vertical offset, in pixels
 * @xres: Horizontal size, in pixels
 * @yres: Vertical size, in pixels
 * @alpha: Transparency value (only for formats that support alpha values)
 *
 * Fill the display (when mode is FB_FILL_DISPLAY) or virtual frame buffer area
 * (when mode is FB_FILL_VIRTUAL) with a gradient pattern.
 */
static void
fb_fill_gradient(struct device *dev, unsigned int xoffset, unsigned int yoffset,
	      unsigned int xres, unsigned int yres, uint8_t alpha)
{
	switch (dev->fourcc) {
	case V4L2_PIX_FMT_NV12:
		return fb_fill_gradient_nv(dev, xoffset, yoffset, xres, yres,
					   2, 2, FB_YUV_YCbCr);
	case V4L2_PIX_FMT_NV21:
		return fb_fill_gradient_nv(dev, xoffset, yoffset, xres, yres,
					   2, 2, FB_YUV_YCrCb);
	case V4L2_PIX_FMT_NV16:
		return fb_fill_gradient_nv(dev, xoffset, yoffset, xres, yres,
					   2, 1, FB_YUV_YCbCr);
	case V4L2_PIX_FMT_NV61:
		return fb_fill_gradient_nv(dev, xoffset, yoffset, xres, yres,
					   2, 1, FB_YUV_YCrCb);
	case V4L2_PIX_FMT_NV24:
		return fb_fill_gradient_nv(dev, xoffset, yoffset, xres, yres,
					   1, 1, FB_YUV_YCbCr);
	case V4L2_PIX_FMT_NV42:
		return fb_fill_gradient_nv(dev, xoffset, yoffset, xres, yres,
					   1, 1, FB_YUV_YCrCb);
	case V4L2_PIX_FMT_RGB565:
		return fb_fill_gradient_rgb16(dev, xoffset, yoffset, xres, yres);
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB24:
		return fb_fill_gradient_rgb24(dev, xoffset, yoffset, xres, yres);
	case V4L2_PIX_FMT_BGR32:
	case V4L2_PIX_FMT_RGB32:
		return fb_fill_gradient_rgb32(dev, xoffset, yoffset, xres, yres, alpha);
	}
}

/*
 * fb_fill - Fill the frame buffer with a test pattern
 * @dev: FB device
 * @mode: Fill mode
 * @pattern: Test pattern
 * @alpha: Transparency value (only for formats that support alpha values)
 *
 * Fill the display (when mode is FB_FILL_DISPLAY) or virtual frame buffer area
 * (when mode is FB_FILL_VIRTUAL) with the test pattern specified by the pattern
 * parameter. Only RGB16, RGB24 and RGB32 on true color visuals are supported.
 */
static void
fb_fill(struct device *dev, enum fb_fill_mode mode,
	enum fb_fill_pattern pattern, uint8_t alpha)
{
	unsigned int xoffset, yoffset;
	unsigned int xres, yres;

	if (dev->fix_info.visual != FB_VISUAL_TRUECOLOR
#ifdef FB_VISUAL_FOURCC
	    && dev->fix_info.visual != FB_VISUAL_FOURCC
#endif
	   ) {
		printf("Error: test pattern is only supported for true color "
			"and fourcc visuals.\n");
		return;
	}

	if (dev->fourcc != V4L2_PIX_FMT_BGR24 &&
	    dev->fourcc != V4L2_PIX_FMT_BGR32 &&
	    dev->fourcc != V4L2_PIX_FMT_NV12 &&
	    dev->fourcc != V4L2_PIX_FMT_NV16 &&
	    dev->fourcc != V4L2_PIX_FMT_NV21 &&
	    dev->fourcc != V4L2_PIX_FMT_NV24 &&
	    dev->fourcc != V4L2_PIX_FMT_NV42 &&
	    dev->fourcc != V4L2_PIX_FMT_NV61 &&
	    dev->fourcc != V4L2_PIX_FMT_RGB24 &&
	    dev->fourcc != V4L2_PIX_FMT_RGB32 &&
	    dev->fourcc != V4L2_PIX_FMT_RGB565) {
		printf("Error: display format %08x (%u bpp) not supported.\n",
			dev->fourcc, dev->var_info.bits_per_pixel);
		return;
	}

	if (mode == FB_FILL_DISPLAY) {
		xoffset = dev->var_info.xoffset;
		yoffset = dev->var_info.yoffset;
		xres = dev->var_info.xres;
		yres = dev->var_info.yres;
	} else {
		xoffset = 0;
		yoffset = 0;
		xres = dev->var_info.xres_virtual;
		yres = dev->var_info.yres_virtual;
	}

	switch (pattern) {
	case FB_PATTERN_SMPTE:
		printf("Filling frame buffer with SMPTE test pattern\n");
		return fb_fill_smpte(dev, xoffset, yoffset, xres, yres, alpha);

	case FB_PATTERN_LINES:
		printf("Filling frame buffer with lines test pattern\n");
		return fb_fill_lines(dev, xoffset, yoffset, xres, yres, alpha);

	case FB_PATTERN_GRADIENT:
		printf("Filling frame buffer with gradient test pattern\n");
		return fb_fill_gradient(dev, xoffset, yoffset, xres, yres, alpha);

	default:
		printf("Error: unsupported test pattern %u.\n", pattern);
		break;
	}
}

/* -----------------------------------------------------------------------------
 * Main
 */

static void usage(const char *argv0)
{
	unsigned int i;

	printf("Usage: %s [options] device\n", argv0);
	printf("Supported options:\n");
	printf("-a, --alpha value		Transparency value for test patterns\n");
	printf("-b, --blank mode		Set blanking mode\n");
	printf("-f, --fill[=mode]		Fill the frame buffer with a test pattern\n");
	printf("-F, --format fourcc		Set the frame buffer format\n");
	printf("-h, --help			Show this help screen\n");
	printf("-p, --pan x,y			Pan the display to position (x,y)\n");
	printf("-P, --pattern name		Test pattern name\n");
	printf("-r, --resolution wxh		Set the display resolution to width x height\n");
	printf("-v, --virtual wxh		Set the display virtual resolution to width x height\n");
	printf("-w, --wait-vsync[=screen]	Wait for VSync on the given screen\n");
	printf("\n");
	printf("Support fill modes are:\n");
	printf(" display	Fill the displayed frame buffer only\n");
	printf(" virtual	Fill the whole virtual frame buffer\n");
	printf("Support formats:\n ");
	for (i = 0; i < ARRAY_SIZE(formats); ++i)
		printf("%s ", formats[i].name);
	printf("\n");
	printf(" 16 24 32 (bpp value, will use the legacy API)\n");
	printf("Support test pattern are:\n");
	printf(" smpte		SMPTE color bars\n");
	printf(" lines		Color lines\n");
	printf("Supported blanking modes are:\n");
	printf(" off		Blanking off, screen active\n");
	printf(" on		Blanked, HSync on,  VSync on\n");
	printf(" vsync		Blanked, HSync on,  VSync off\n");
	printf(" hsync		Blanked, HSync off, VSync on\n");
	printf(" powerdown	Blanked, HSync off, VSync off\n");
}

static struct option opts[] = {
	{"alpha", 1, 0, 'a'},
	{"blank", 1, 0, 'b'},
	{"fill", 2, 0, 'f'},
	{"format", 1, 0, 'F'},
	{"help", 0, 0, 'h'},
	{"pan", 1, 0, 'p'},
	{"pattern", 1, 0, 'P'},
	{"resolution", 1, 0, 'r'},
	{"virtual", 1, 0, 'v'},
	{"wait-vsync", 2, 0, 'w'},
	{0, 0, 0, 0}
};

static int fb_blank_parse(const char *arg, int *value)
{
	static const struct fb_value_name names[] = {
		{ FB_BLANK_UNBLANK, "off" },
		{ FB_BLANK_NORMAL, "on" },
		{ FB_BLANK_VSYNC_SUSPEND, "vsync" },
		{ FB_BLANK_HSYNC_SUSPEND, "hsync" },
		{ FB_BLANK_POWERDOWN, "powerdown" },
	};
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(names); ++i) {
		if (strcmp(names[i].name, arg) == 0) {
			*value = names[i].value;
			return 0;
		}
	}

	return -1;
}

static int fb_format_parse(const char *arg, unsigned int *fourcc)
{
	unsigned int bpp;
	unsigned int i;
	char *endp;

	for (i = 0; i < ARRAY_SIZE(formats); ++i) {
		if (strcasecmp(formats[i].name, arg) == 0) {
			*fourcc = formats[i].fourcc;
			return 0;
		}
	}

	bpp = strtoul(arg, &endp, 10);
	if (*endp != '\0')
		return -1;

	if (bpp != 16 && bpp != 24 && bpp != 32)
		return -1;

	*fourcc = bpp;
	return 0;
}

static int fb_point_parse(const char *arg, unsigned int *x, unsigned int *y)
{
	unsigned long value;
	char *endptr;

	value = strtoul(arg, &endptr, 10);
	if (endptr == arg || *endptr != ',')
		return -1;
	*x = value;
	arg = endptr + 1;

	value = strtoul(arg, &endptr, 10);
	if (endptr == arg || *endptr != '\0')
		return -1;
	*y = value;

	return 0;
}

static int fb_size_parse(const char *arg, int *x, int *y)
{
	unsigned long value;
	char *endptr;

	value = strtoul(arg, &endptr, 10);
	if (endptr == arg || *endptr != 'x')
		return -1;
	*x = value;
	arg = endptr + 1;

	value = strtoul(arg, &endptr, 10);
	if (endptr == arg || *endptr != '\0')
		return -1;
	*y = value;

	return 0;
}

int main(int argc, char *argv[])
{
	struct device dev;
	int ret;

	/* Options parsing. */
	bool do_blank = false;
	int blank = 0;

	enum fb_fill_mode fill_mode = FB_FILL_NONE;
	enum fb_fill_pattern fill_pattern = FB_PATTERN_SMPTE;
	uint8_t alpha = 255;

	bool do_format = false;
	unsigned int fourcc = 0;

	bool do_pan = false;
	unsigned int pan_x = 0;
	unsigned int pan_y = 0;

	bool do_resolution = false;
	int xres = -1;
	int yres = -1;
	int xres_virtual = -1;
	int yres_virtual = -1;

	bool do_wait_for_vsync = false;
	unsigned int screen = 0;

	int c;

	opterr = 0;
	while ((c = getopt_long(argc, argv, "a:b:f::F:hp:P:r:v:w::", opts, NULL)) != -1) {

		switch (c) {
		case 'a':
			alpha = atoi(optarg);
			break;
		case 'b':
			do_blank = true;
			if (fb_blank_parse(optarg, &blank) < 0) {
				printf("Invalid blanking mode `%s'\n", optarg);
				printf("Run %s -h for help.\n", argv[0]);
				return 1;
			}
			break;
		case 'f':
			if (optarg == NULL)
				fill_mode = FB_FILL_DISPLAY;
			else if (strcmp(optarg, "display") == 0)
				fill_mode = FB_FILL_DISPLAY;
			else if (strcmp(optarg, "virtual") == 0)
				fill_mode = FB_FILL_VIRTUAL;
			else {
				printf("Invalid fill mode `%s'\n", optarg);
				printf("Run %s -h for help.\n", argv[0]);
				return 1;
			}
			break;
		case 'F':
			do_format = true;
			if (fb_format_parse(optarg, &fourcc) < 0) {
				printf("Invalid format `%s'\n", optarg);
				printf("Run %s -h for help.\n", argv[0]);
				return 1;
			}
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		case 'p':
			do_pan = true;
			if (fb_point_parse(optarg, &pan_x, &pan_y) < 0) {
				printf("Invalid pan point `%s'\n", optarg);
				printf("Run %s -h for help.\n", argv[0]);
				return 1;
			}
			break;
		case 'P':
			if (strcmp(optarg, "smpte") == 0)
				fill_pattern = FB_PATTERN_SMPTE;
			else if (strcmp(optarg, "lines") == 0)
				fill_pattern = FB_PATTERN_LINES;
			else if (strcmp(optarg, "gradient") == 0)
				fill_pattern = FB_PATTERN_GRADIENT;
			else {
				printf("Invalid test pattern `%s'\n", optarg);
				printf("Run %s -h for help.\n", argv[0]);
				return 1;
			}
			break;
		case 'r':
			do_resolution = true;
			if (fb_size_parse(optarg, &xres, &yres) < 0) {
				printf("Invalid resolution `%s'\n", optarg);
				printf("Run %s -h for help.\n", argv[0]);
				return 1;
			}
			break;
		case 'v':
			do_resolution = true;
			if (fb_size_parse(optarg, &xres_virtual, &yres_virtual) < 0) {
				printf("Invalid virtual resolution `%s'\n", optarg);
				printf("Run %s -h for help.\n", argv[0]);
				return 1;
			}
			break;
		case 'w':
			do_wait_for_vsync = true;
			if (optarg)
				screen = atoi(optarg);
			break;
		default:
			printf("Invalid option -%c\n", c);
			printf("Run %s -h for help.\n", argv[0]);
			return 1;
		}
	}

	if (optind >= argc) {
		usage(argv[0]);
		return 1;
	}

	ret = fb_open(&dev, argv[optind]);
	if (ret < 0)
		return 1;

	if (do_blank)
		fb_blank(&dev, blank);

	if (do_format)
		fb_set_format(&dev, fourcc);

	if (do_resolution)
		fb_set_resolution(&dev, xres, yres, xres_virtual, yres_virtual);

	if (fill_mode != FB_FILL_NONE)
		fb_fill(&dev, fill_mode, fill_pattern, alpha);

	if (do_pan)
		fb_pan(&dev, pan_x, pan_y);

	if (do_wait_for_vsync)
		fb_wait_for_vsync(&dev, screen);

	fb_close(&dev);
	return 0;
}
