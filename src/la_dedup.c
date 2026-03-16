#include "gcode.h"


int main(void) {
    int count = 0;
    Line *lines = parse_gcode_file(&count);
    
    if (!lines) {
        fprintf(stderr, "Failed to parse gcode file\n");
        return 1;
    }

    float cut_distance = calculate_cut_distance(lines, count);
    fprintf(stderr, "Total cut distance: %.2f\n", cut_distance);

    remove_zero_lines(&lines, &count);
    deduplicate_lines(&lines, &count);

    float cut_distance_after = calculate_cut_distance(lines, count);
    fprintf(stderr, "Total cut distance after deduplication: %.2f", cut_distance_after);
    fprintf(stderr, " (%.1f%% less)\n", 100*(1-(cut_distance_after/cut_distance)));

    remove_overlapping_lines(&lines, &count);

    cut_distance_after = calculate_cut_distance(lines, count);
    fprintf(stderr, "Total cut distance after removing overlaps: %.2f", cut_distance_after);
    fprintf(stderr, " (%.1f%% less)\n", 100*(1-(cut_distance_after/cut_distance)));

    export_gcode(lines, count);
    free(lines);
    return 0;
}

