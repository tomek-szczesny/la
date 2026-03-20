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

    float x = parse_float_arg(argc, argv, "-x", 0.0f);
    float y = parse_float_arg(argc, argv, "-y", 0.0f);

    int count = 0;
    Line *lines = parse_gcode_file(&count);
    if (!lines) {
        fprintf(stderr, "Failed to parse gcode file\n");
        return 1;
    }
    
    Line bb = bounding_box(lines, count);
    translate_lines(lines, count, x - bb.x0, y - bb.y0);

    export_gcode(lines, count);

    free(lines);
    return 0;
}

