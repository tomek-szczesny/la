#include "gcode.h"


int main(void) {
    int count = 0;
    Line *lines = parse_gcode_file(&count);
    
    if (!lines) {
        fprintf(stderr, "Failed to parse gcode file\n");
        return 1;
    }

    //int passes = detect_passes(lines,count);
    //fprintf(stderr, "Detected %d passes\n", passes);
    float cut_distance = calculate_cut_distance(lines, count);
    fprintf(stderr, "Total cut distance: %.2f\n", cut_distance);

    export_gcode(lines, count);
    free(lines);
    return 0;
}

