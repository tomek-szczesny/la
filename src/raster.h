#ifndef RASTER_H
#define RASTER_H

#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint8_t r, g, b, a;
} RGBAPixel;

typedef struct {
    uint32_t width;
    uint32_t height;
    RGBAPixel *pixels;  // width * height elements
} RGBAImage;

void rgbaimage_free(RGBAImage *img);

#endif

