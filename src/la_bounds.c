#include "gcode.h"


int main(void) {
    int count = 0;
    Line *lines = parse_gcode_file(&count);
    
    if (!lines) {
        fprintf(stderr, "Failed to parse gcode file\n");
        return 1;
    }
    
    Line bb = bounding_box(lines, count);

    fprintf(stdout, "; Bounds: X%.2f Y%.2f to X%.2f Y%.2f\n",
            bb.x0, bb.y0, bb.x1, bb.y1);

    free(lines);
    return 0;
}

