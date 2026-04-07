#ifndef RASTER_H
#define RASTER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t r, g, b, a;
} RGBAPixel;

typedef struct {
    uint32_t width;
    uint32_t height;
    RGBAPixel *pixels;
} RGBAImage;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t *pixels;  // grayscale values
} GrayImage;

void rgbaimage_free(RGBAImage *img);
void grayimage_free(GrayImage *img);

GrayImage* rgba_to_gray(const RGBAImage *img);

// Scale grayimage in place to new dimensions
// Returns 0 on success, -1 on failure
int grayimage_scale(GrayImage *img, uint32_t new_width, uint32_t new_height);

// Export grayimage as PGM to stdout
// Returns 0 on success, -1 on failure
int grayimage_export_pgm(const GrayImage *img);

// Dither grayimage to reduced color depth (2-32 levels)
// Modifies image in place. Returns 0 on success, -1 on failure
int grayimage_dither_floyd_steinberg(GrayImage *img, int levels);

// Contrast stretch grayimage by percentile clipping (0.0-0.5)
// Returns 0 on success, -1 on failure
int grayimage_stretch_contrast(GrayImage *img, float clip);

#endif

