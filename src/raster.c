#include "raster.h"
#include <math.h>

void rgbaimage_free(RGBAImage *img) {
    if (!img) return;
    free(img->pixels);
    free(img);
}

void grayimage_free(GrayImage *img) {
    if (!img) return;
    free(img->pixels);
    free(img);
}

static uint8_t rgba_pixel_to_gray(RGBAPixel pix) {
    // Standard luminance weights
    uint32_t gray = (uint32_t)(0.299 * pix.r + 0.587 * pix.g + 0.114 * pix.b);

    // Alpha as whitening: transparent -> white
    uint32_t result = (gray * pix.a + 255 * (255 - pix.a)) / 255;
    return (uint8_t)result;
}

GrayImage* rgba_to_gray(const RGBAImage *img) {
    if (!img) return NULL;

    GrayImage *gray = malloc(sizeof(GrayImage));
    if (!gray) return NULL;

    gray->width = img->width;
    gray->height = img->height;
    gray->pixels = malloc(img->width * img->height);

    if (!gray->pixels) {
        free(gray);
        return NULL;
    }

    for (uint32_t i = 0; i < img->width * img->height; i++) {
        gray->pixels[i] = rgba_pixel_to_gray(img->pixels[i]);
    }

    return gray;
}

static float cubic_kernel(float x) {
    // Catmull-Rom cubic kernel
    x = fabsf(x);
    if (x < 1.0f) {
        return 1.0f - 2.5f * x * x + 1.5f * x * x * x;
    } else if (x < 2.0f) {
        return -0.5f * (x - 1.0f) * (x - 2.0f) * (x - 2.0f);
    }
    return 0.0f;
}

static uint8_t sample_gray_cubic(const uint8_t *pixels, uint32_t width, uint32_t height,
                                  float x, float y) {
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    float dx = x - ix;
    float dy = y - iy;

    float value = 0.0f;
    for (int j = -1; j <= 2; j++) {
        for (int i = -1; i <= 2; i++) {
            int px = ix + i;
            int py = iy + j;

            // Clamp to image bounds
            if (px < 0) px = 0;
            if (px >= (int)width) px = width - 1;
            if (py < 0) py = 0;
            if (py >= (int)height) py = height - 1;

            float kernel = cubic_kernel(i - dx) * cubic_kernel(j - dy);
            value += kernel * pixels[py * width + px];
        }
    }

    value = fmaxf(0.0f, fminf(255.0f, value));
    return (uint8_t)(value + 0.5f);
}

int grayimage_scale(GrayImage *img, uint32_t new_width, uint32_t new_height) {
    if (!img || new_width == 0 || new_height == 0) return -1;

    uint8_t *new_pixels = malloc(new_width * new_height);
    if (!new_pixels) return -1;

    // Horizontal then vertical scaling
    for (uint32_t y = 0; y < new_height; y++) {
        float src_y = (y + 0.5f) * img->height / new_height - 0.5f;
        for (uint32_t x = 0; x < new_width; x++) {
            float src_x = (x + 0.5f) * img->width / new_width - 0.5f;
            new_pixels[y * new_width + x] = sample_gray_cubic(img->pixels, img->width, img->height, src_x, src_y);
        }
    }

    free(img->pixels);
    img->pixels = new_pixels;
    img->width = new_width;
    img->height = new_height;
    return 0;
}

// Trivial bitmap output for testing and debugging
int grayimage_export_pgm(const GrayImage *img) {
    if (!img) return -1;

    // Write PGM header
    fprintf(stdout, "P5\n%u %u\n255\n", img->width, img->height);

    // Write binary pixel data
    size_t written = fwrite(img->pixels, 1, img->width * img->height, stdout);
    fflush(stdout);

    return (written == img->width * img->height) ? 0 : -1;
}

int grayimage_dither_floyd_steinberg(GrayImage *img, int levels) {
    if (!img || levels < 2 || levels > 32) return -1;

    // Create error accumulation buffer
    float *error = calloc(img->width * img->height, sizeof(float));
    if (!error) return -1;

    float level_spacing = 255.0f / (levels - 1);

    for (uint32_t y = 0; y < img->height; y++) {
        for (uint32_t x = 0; x < img->width; x++) {
            uint32_t idx = y * img->width + x;

            // Current pixel + accumulated error
            float current = img->pixels[idx] + error[idx];

            // Quantize to nearest level
            int level = (int)(current / level_spacing + 0.5f);
            level = (level < 0) ? 0 : (level >= levels) ? levels - 1 : level;

            float quantized = level * level_spacing;
            float err = current - quantized;

            img->pixels[idx] = (uint8_t)quantized;

            // Distribute error: right 7/16, bottom-left 3/16, bottom 5/16, bottom-right 1/16
            if (x + 1 < img->width) {
                error[idx + 1] += err * 7.0f / 16.0f;
            }
            if (y + 1 < img->height) {
                if (x > 0) error[(y + 1) * img->width + (x - 1)] += err * 3.0f / 16.0f;
                error[(y + 1) * img->width + x] += err * 5.0f / 16.0f;
                if (x + 1 < img->width) error[(y + 1) * img->width + (x + 1)] += err * 1.0f / 16.0f;
            }
        }
    }

    free(error);
    return 0;
}

static int compare_uint8(const void *a, const void *b) {
    return (int)*(uint8_t*)a - (int)*(uint8_t*)b;
}

int grayimage_stretch_contrast(GrayImage *img, float clip) {
    if (!img || clip < 0.0f || clip > 0.5f) return -1;

    uint32_t total = img->width * img->height;
    uint8_t *sorted = malloc(total);
    if (!sorted) return -1;

    memcpy(sorted, img->pixels, total);
    qsort(sorted, total, sizeof(uint8_t), compare_uint8);

    uint32_t idx_dark = (uint32_t)(total * clip);
    uint32_t idx_bright = (uint32_t)(total * (1.0f - clip));

    uint8_t level_dark = sorted[idx_dark];
    uint8_t level_bright = sorted[idx_bright];

    free(sorted);

    if (level_dark == level_bright) return 0;  // No contrast

    float scale = 255.0f / (level_bright - level_dark);
    for (uint32_t i = 0; i < total; i++) {
        float val = (img->pixels[i] - level_dark) * scale;
        val = fmaxf(0.0f, fminf(255.0f, val));
        img->pixels[i] = (uint8_t)(val + 0.5f);
    }

    return 0;
}


