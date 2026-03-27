#include "gcode.h"

float parse_float_arg(int argc, char *argv[], const char *flag, float default_val) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return atof(argv[i + 1]);
        }
    }
    return default_val;
}

int has_flag(int argc, char *argv[], const char *flag) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return 1;
        }
    }
    return 0;
}


float optimize_dimension(Line *lines, int count, int dimension, float tolerance) {
    // dimension: 0 for x, 1 for y
    float angles[] = {-90, -60, -30, 30, 60, 90};
        
    Line bbox = bounding_box(lines, count);
    float best_angle = 0;
    float best_dim = (dimension == 0) ? (bbox.x1 - bbox.x0) : (bbox.y1 - bbox.y0);

    for (int i = 0; i < 6; i++) {
        Line *temp = malloc(count * sizeof(Line));
        memcpy(temp, lines, count * sizeof(Line));
        rotate_lines(temp, count, angles[i]);
        bbox = bounding_box(temp, count);
        float dim = (dimension == 0) ? (bbox.x1 - bbox.x0) : (bbox.y1 - bbox.y0);
        if (dim < best_dim) {
            best_dim = dim;
            best_angle = angles[i];
        }
        free(temp);
    }

    float low = best_angle - 30, high = best_angle + 30;
    float d1 = tolerance * 2;
    float d2 = 0;
    while (fabsf(d1 - d2) > tolerance) {
        float mid1 = low + (high - low) / 3;
        float mid2 = high - (high - low) / 3;

        Line *t1 = malloc(count * sizeof(Line));
        Line *t2 = malloc(count * sizeof(Line));
        memcpy(t1, lines, count * sizeof(Line));
        memcpy(t2, lines, count * sizeof(Line));
        rotate_lines(t1, count, mid1);
        rotate_lines(t2, count, mid2);
        Line b1 = bounding_box(t1, count);
        Line b2 = bounding_box(t2, count);
        d1 = (dimension == 0) ? (b1.x1 - b1.x0) : (b1.y1 - b1.y0);
        d2 = (dimension == 0) ? (b2.x1 - b2.x0) : (b2.y1 - b2.y0);
        free(t1);
        free(t2);

        if (d1 < d2) high = mid2;
        else low = mid1;
    }

    if (best_dim < d1 && best_dim < d2) return best_angle;
    else {
        d1 = (low+high)/2;
        if (d1 >  90) d1 -= 180;
        if (d1 < -90) d1 += 180;
        return d1;
    }
}


int main(int argc, char *argv[]) {

    float a = parse_float_arg(argc, argv, "-a", 0.0f);
    int minx = has_flag(argc, argv, "-minx");
    int miny = has_flag(argc, argv, "-miny");

    int count = 0;
    Line *lines = parse_gcode_file(&count);
    if (!lines) {
        fprintf(stderr, "[la_rot] Failed to parse gcode file\n");
        return 1;
    }
    
    if (a != 0) {
        if (minx) fprintf(stderr, "[la_rot] Warning: -minx flag ignored.\n");
        if (miny) fprintf(stderr, "[la_rot] Warning: -miny flag ignored.\n");
        rotate_lines(lines, count, a);
        fprintf(stderr, "[la_rot] Rotated by %g degrees.\n", a);
        export_gcode(lines, count);
        free(lines);
        return 0;
    }
    if (minx) {
        if (miny) fprintf(stderr, "[la_rot] Warning: -miny flag ignored.\n");
        a = optimize_dimension(lines, count, 0, 0.001);
        rotate_lines(lines, count, a);
        fprintf(stderr, "[la_rot] Rotated by %g degrees.\n", a);
        export_gcode(lines, count);
        free(lines);
        return 0;
    }
    if (miny) {
        a = optimize_dimension(lines, count, 1, 0.001);
        rotate_lines(lines, count, a);
        fprintf(stderr, "[la_rot] Rotated by %g degrees.\n", a);
        export_gcode(lines, count);
        free(lines);
        return 0;
    }

    if (miny) fprintf(stderr, "[la_rot] Warning: la_rotate got nothing to do.\n");
    export_gcode(lines, count);
    free(lines);
    return 0;
}

