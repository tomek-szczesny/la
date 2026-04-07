#include "raster.h"

void rgbaimage_free(RGBAImage *img) {
    if (!img) return;
    free(img->pixels);
    free(img);
}

