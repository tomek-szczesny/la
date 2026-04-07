#include "png_parser.h"
#include <png.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const uint8_t *data;
    size_t len;
    size_t pos;
} PNGMemory;

static void png_read_fn(png_structp png, png_bytep data, png_size_t length) {
    PNGMemory *mem = (PNGMemory *)png_get_io_ptr(png);
    if (mem->pos + length > mem->len) {
        png_error(png, "Read beyond buffer");
        return;
    }
    memcpy(data, mem->data + mem->pos, length);
    mem->pos += length;
}

RGBAImage* png_parse(const uint8_t *data, size_t data_len) {
    if (!data || data_len < 8) return NULL;
    
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return NULL;
    
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        return NULL;
    }
    
    PNGMemory mem = {data, data_len, 0};
    png_set_read_fn(png, &mem, png_read_fn);
    
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }
    
    png_read_info(png, info);
    uint32_t width = png_get_image_width(png, info);
    uint32_t height = png_get_image_height(png, info);
    uint8_t color_type = png_get_color_type(png, info);
    uint8_t bit_depth = png_get_bit_depth(png, info);
    
    // Normalize to RGBA
    if (bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_RGB)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    
    png_read_update_info(png, info);
    
    RGBAImage *img = malloc(sizeof(RGBAImage));
    if (!img) {
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }
    
    img->width = width;
    img->height = height;
    img->pixels = malloc(width * height * sizeof(RGBAPixel));
    
    if (!img->pixels) {
        free(img);
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }
    
    png_bytep *row_pointers = malloc(height * sizeof(png_bytep));
    for (uint32_t i = 0; i < height; i++) {
        row_pointers[i] = (png_bytep)(img->pixels + i * width);
    }
    
    png_read_image(png, row_pointers);
    free(row_pointers);
    
    png_destroy_read_struct(&png, &info, NULL);
    return img;
}

