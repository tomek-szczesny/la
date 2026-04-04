#include "gcode.h"
#include "util.h"

int main(int argc, char *argv[]) {

    int n = parse_int_arg(argc, argv, "-n", 1);

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

