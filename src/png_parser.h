#ifndef PNG_PARSER_H
#define PNG_PARSER_H

#include "raster.h"
#include <stddef.h>

// Parse PNG data from memory buffer
// Returns allocated RGBWImage on success, NULL on failure
RGBAImage* png_parse(const uint8_t *data, size_t data_len);

#endif

