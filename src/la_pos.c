#include "gcode.h"
#include "util.h"

int main(int argc, char *argv[]) {

    float x = parse_float_arg(argc, argv, "-x", 0.0f);
    float y = parse_float_arg(argc, argv, "-y", 0.0f);

    int count = 0;
    Line *lines = parse_gcode_file(&count);
    if (!lines) {
        fprintf(stderr, "[la_pos] Failed to parse gcode file\n");
        return 1;
    }
    
    Line bb = bounding_box(lines, count);
    x -= bb.x0;
    y -= bb.y0;
    translate_lines(lines, count, x, y);

    fprintf(stderr, "[la_pos] Moved by (%g, %g).\n", x, y);

    export_gcode(lines, count);

    free(lines);
    return 0;
}

