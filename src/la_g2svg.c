#include <cairo-svg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gcode.h"
#include "line.h"
#include "plot.h"

static cairo_status_t write_to_stdout(void *closure, const unsigned char *data, unsigned int length) {
    (void)closure;
    size_t written = fwrite(data, 1, length, stdout);
    return (written == length) ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}


void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [-agicf]\n", prog);
    fprintf(stderr, "  -a  Draw axes and grid\n");
    fprintf(stderr, "  -i  Opacity by intensity\n");
    fprintf(stderr, "  -c  Color gradient\n");
    fprintf(stderr, "  -f  Show fast moves\n");
}

int main(int argc, char *argv[]) {
    int count = 0;
    Line *lines = parse_gcode_file(&count);
    remove_zero_lines(&lines, &count);
    
    if (!lines) {
        fprintf(stderr, "Failed to parse gcode file\n");
        return 1;
    }

    PlotOptions opts = plot_options_default();
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0) opts.draw_axes = 1;
        else if (strcmp(argv[i], "-i") == 0) opts.opacity_by_intensity = 1;
        else if (strcmp(argv[i], "-c") == 0) opts.color_gradient = 1;
        else if (strcmp(argv[i], "-f") == 0) opts.show_fast_moves = 1;
        else {
            print_usage(argv[0]);
            free(lines);
            return 1;
        }
    }

    Line bbox = view_bounding_box(lines, count);
    float width = bbox.x1 - bbox.x0;
    float height = bbox.y1 - bbox.y0;

    cairo_surface_t *surface = cairo_svg_surface_create_for_stream(
    write_to_stdout, NULL, width, height);
    cairo_svg_surface_set_document_unit(surface, CAIRO_SVG_UNIT_MM);
    cairo_t *cr = cairo_create(surface);
    
    plot_to_cr(cr, lines, count, opts);
    
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    
    free(lines);
    return 0;
}


