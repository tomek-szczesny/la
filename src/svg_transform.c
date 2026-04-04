
#include "svg_transform.h"

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


