#ifndef MATRIX_H
#define MATRIX_H

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Transform matrices
// a c e
// b d f
// 0 0 1
typedef struct {
    double a, b, c, d, e, f;
} Matrix;

inline Matrix matrix_multiply(Matrix a, Matrix b) {
    return (Matrix){
        a.a * b.a + a.b * b.c,
        a.a * b.b + a.b * b.d,
        a.c * b.a + a.d * b.c,
        a.c * b.b + a.d * b.d,
        a.e * b.a + a.f * b.c + b.e,
        a.e * b.b + a.f * b.d + b.f
    };
}

// Identity matrix
inline Matrix matrix_identity() {
    return (Matrix){1, 0, 0, 1, 0, 0};
}

// Transform: translate(x, y)
inline Matrix transform_translate(double tx, double ty) {
    return (Matrix){1, 0, 0, 1, tx, ty};
}

// Transform: scale(x [, y])
inline Matrix transform_scale(double sx, double sy) {
    return (Matrix){sx, 0, 0, sy, 0, 0};
}

// Transform: rotate(angle [, cx, cy])
// angle in degrees
inline Matrix transform_rotate(double angle, double cx, double cy) {
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
inline Matrix transform_skewX(double angle) {
    double rad = angle * M_PI / 180.0;
    return (Matrix){1, 0, tan(rad), 1, 0, 0};
}

// Transform: skewY(angle)
inline Matrix transform_skewY(double angle) {
    double rad = angle * M_PI / 180.0;
    return (Matrix){1, tan(rad), 0, 1, 0, 0};
}

// Transform: matrix(a, b, c, d, e, f)
inline Matrix transform_matrix(double a, double b, double c, double d, double e, double f) {
    return (Matrix){a, b, c, d, e, f};
}

#endif

