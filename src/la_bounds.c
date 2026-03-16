#include "gcode.h"


int main(void) {
    int count = 0;
    Line *lines = parse_gcode_file(&count);
    
    if (!lines) {
        fprintf(stderr, "Failed to parse gcode file\n");
        return 1;
    }
    
    float x0, y0, x1, y1;

    x0 = lines[0].x0;
    x1 = x0;
    y0 = lines[0].y0;
    y1 = y0;

    for (int i = 0; i < count; i++) {
        x0 = fminf(x0, lines[i].x0);
        x0 = fminf(x0, lines[i].x1);
        x1 = fmaxf(x1, lines[i].x0);
        x1 = fmaxf(x1, lines[i].x1);
        y0 = fminf(y0, lines[i].y0);
        y0 = fminf(y0, lines[i].y1);
        y1 = fmaxf(y1, lines[i].y0);
        y1 = fmaxf(y1, lines[i].y1);
    }

    fprintf(stdout, "; Bounds: X%.2f Y%.2f to X%.2f Y%.2f\n",
            x0, y0, x1, y1);

    free(lines);
    return 0;
}

