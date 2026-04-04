// svg_transform.h
#ifndef SVG_TRANSFORM_H
#define SVG_TRANSFORM_H

#include <math.h>
#include "line.h"
#include "matrix.h"

// Parse a single transform function and return the matrix
// Returns identity if parsing fails
Matrix parse_transform_function(const char *str);

// Parse full transform attribute string
// Handles multiple transforms: "translate(10,20) rotate(45)"
// Applies them right-to-left as per SVG spec
Matrix parse_transform_string(const char *str);

#endif // SVG_TRANSFORM_H

