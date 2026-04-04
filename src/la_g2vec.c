
#ifdef BUILD_PDF
#include <cairo-pdf.h>
#else
#include <cairo-svg.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gcode.h"
#include "line.h"
#include "plot.h"
#include "util.h"

static cairo_status_t write_to_stdout(void *closure, const unsigned char *data, unsigned int length) {
    (void)closure;
    size_t written = fwrite(data, 1, length, stdout);
    return (written == length) ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}

int main(int argc, char *argv[]) {
    int count = 0;
    Line *lines = parse_gcode_file(&count);
    remove_zero_lines(&lines, &count);
    
    if (!lines) {
        fprintf(stderr, "[la_g2svg] Failed to parse gcode file\n");
        return 1;
    }

    PlotOptions opts = plot_options_default();
    opts.draw_axes =                has_flag(argc, argv, "-a");
    opts.opacity_by_intensity =     has_flag(argc, argv, "-i");
    opts.color_gradient =           has_flag(argc, argv, "-c");
    opts.show_fast_moves =          has_flag(argc, argv, "-f");

    Line bbox = view_bounding_box(lines, count);
    float width = bbox.x1 - bbox.x0;
    float height = bbox.y1 - bbox.y0;

#ifdef BUILD_PDF
    cairo_surface_t *surface = cairo_pdf_surface_create_for_stream(
        write_to_stdout, NULL, width, height);
#else
    cairo_surface_t *surface = cairo_svg_surface_create_for_stream(
        write_to_stdout, NULL, width, height);
    cairo_svg_surface_set_document_unit(surface, CAIRO_SVG_UNIT_MM);
#endif
    cairo_t *cr = cairo_create(surface);
    
    plot_to_cr(cr, lines, count, opts);
    
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    fprintf(stderr, "[la_g2svg] Done.\n");
    
    free(lines);
    return 0;
}


