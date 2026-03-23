// svg_transform.h
#ifndef SVG_TRANSFORM_H
#define SVG_TRANSFORM_H

#include <math.h>
#include "line.h"

// Identity matrix
static inline Matrix matrix_identity() {
    return (Matrix){1, 0, 0, 1, 0, 0};
}

// Transform: translate(x, y)
static inline Matrix transform_translate(double tx, double ty) {
    return (Matrix){1, 0, 0, 1, tx, ty};
}

// Transform: scale(x [, y])
static inline Matrix transform_scale(double sx, double sy) {
    return (Matrix){sx, 0, 0, sy, 0, 0};
}

// Transform: rotate(angle [, cx, cy])
// angle in degrees
static inline Matrix transform_rotate(double angle, double cx, double cy) {
    double rad = angle * M_PI / 180.0;
    double cos_a = cos(rad);
    double sin_a = sin(rad);
    
    // rotate around origin
    Matrix rot = (Matrix){cos_a, sin_a, -sin_a, cos_a, 0, 0};
    
    // if rotation center specified, translate before and after
    if (cx != 0 || cy != 0) {
        Matrix t1 = transform_translate(-cx, -cy);
        Matrix t2 = transform_translate(cx, cy);
        rot = matrix_multiply(t2, matrix_multiply(rot, t1));
    }
    
    return rot;
}

// Transform: skewX(angle)
static inline Matrix transform_skewX(double angle) {
    double rad = angle * M_PI / 180.0;
    return (Matrix){1, 0, tan(rad), 1, 0, 0};
}

// Transform: skewY(angle)
static inline Matrix transform_skewY(double angle) {
    double rad = angle * M_PI / 180.0;
    return (Matrix){1, tan(rad), 0, 1, 0, 0};
}

// Transform: matrix(a, b, c, d, e, f)
static inline Matrix transform_matrix(double a, double b, double c, double d, double e, double f) {
    return (Matrix){a, b, c, d, e, f};
}

// Parse a single transform function and return the matrix
// Returns identity if parsing fails
Matrix parse_transform_function(const char *str) {
    // Skip whitespace
    while (*str && (*str == ' ' || *str == '\t')) str++;
    
    // Identify function type
    if (strncmp(str, "translate", 9) == 0) {
        double tx = 0, ty = 0;
        if (sscanf(str, "translate(%lf,%lf)", &tx, &ty) == 2 ||
            sscanf(str, "translate(%lf %lf)", &tx, &ty) == 2) {
            return transform_translate(tx, ty);
        } else if (sscanf(str, "translate(%lf)", &tx) == 1) {
            return transform_translate(tx, 0);
        }
    }
    else if (strncmp(str, "scale", 5) == 0) {
        double sx = 1, sy = 1;
        if (sscanf(str, "scale(%lf,%lf)", &sx, &sy) == 2 ||
            sscanf(str, "scale(%lf %lf)", &sx, &sy) == 2) {
            return transform_scale(sx, sy);
        } else if (sscanf(str, "scale(%lf)", &sx) == 1) {
            return transform_scale(sx, sx);
        }
    }
    else if (strncmp(str, "rotate", 6) == 0) {
        double angle = 0, cx = 0, cy = 0;
        if (sscanf(str, "rotate(%lf,%lf,%lf)", &angle, &cx, &cy) == 3 ||
            sscanf(str, "rotate(%lf %lf %lf)", &angle, &cx, &cy) == 3) {
            return transform_rotate(angle, cx, cy);
        } else if (sscanf(str, "rotate(%lf)", &angle) == 1) {
            return transform_rotate(angle, 0, 0);
        }
    }
    else if (strncmp(str, "skewX", 5) == 0) {
        double angle = 0;
        if (sscanf(str, "skewX(%lf)", &angle) == 1) {
            return transform_skewX(angle);
        }
    }
    else if (strncmp(str, "skewY", 5) == 0) {
        double angle = 0;
        if (sscanf(str, "skewY(%lf)", &angle) == 1) {
            return transform_skewY(angle);
        }
    }
    else if (strncmp(str, "matrix", 6) == 0) {
        double a, b, c, d, e, f;
        if (sscanf(str, "matrix(%lf,%lf,%lf,%lf,%lf,%lf)", &a, &b, &c, &d, &e, &f) == 6 ||
            sscanf(str, "matrix(%lf %lf %lf %lf %lf %lf)", &a, &b, &c, &d, &e, &f) == 6) {
            return transform_matrix(a, b, c, d, e, f);
        }
    }
    
    return matrix_identity();
}

// Parse full transform attribute string
// Handles multiple transforms: "translate(10,20) rotate(45)"
// Applies them right-to-left as per SVG spec
Matrix parse_transform_string(const char *str) {
    if (!str || !*str) return matrix_identity();
    
    // Make a mutable copy
    char buf[1024];
    strncpy(buf, str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    
    // Find all function calls and store their positions
    // We'll parse right-to-left
    typedef struct {
        char *start;
        char *end;
    } FuncSpan;
    
    FuncSpan funcs[16];
    int func_count = 0;
    
    char *p = buf;
    while (*p && func_count < 16) {
        // Skip whitespace
        while (*p && (*p == ' ' || *p == '\t')) p++;
        if (!*p) break;
        
        // Find opening paren
        char *func_start = p;
        while (*p && *p != '(') p++;
        if (!*p) break;
        
        // Find closing paren
        int paren_depth = 1;
        p++;
        while (*p && paren_depth > 0) {
            if (*p == '(') paren_depth++;
            else if (*p == ')') paren_depth--;
            p++;
        }
        
        funcs[func_count].start = func_start;
        funcs[func_count].end = p;
        func_count++;
    }
    
    // Apply transforms right-to-left
    Matrix result = matrix_identity();
    for (int i = func_count - 1; i >= 0; i--) {
        // Null-terminate this function
        char saved = *funcs[i].end;
        *funcs[i].end = '\0';
        
        Matrix m = parse_transform_function(funcs[i].start);
        result = matrix_multiply(m, result);
        
        *funcs[i].end = saved;
    }
    
    return result;
}

#endif // SVG_TRANSFORM_H

