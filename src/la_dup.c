#include "gcode.h"

float parse_float_arg(int argc, char *argv[], const char *flag, float default_val) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return atof(argv[i + 1]);
        }
    }
    return default_val;
}

int main(int argc, char *argv[]) {

    float x = parse_float_arg(argc, argv, "-n", 1.0f);
    int n = (int)round(x);

    int count = 0;
    Line *lines = parse_gcode_file(&count);
    if (!lines) {
        fprintf(stderr, "[la_dup] Failed to parse gcode file\n");
        return 1;
    }
    
    if (n <= 1) {
        export_gcode(lines, count);
        fprintf(stderr, "[la_dup] Did nothing.\n");
        return 0;
    }
    
    for (int i = 0; i < n; i++) {
        export_gcode(lines, count);
    }

    fprintf(stderr, "[la_dup] Duplicated passes %d times.\n", n);

    free(lines);
    return 0;
}

