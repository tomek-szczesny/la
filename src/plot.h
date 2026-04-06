#ifndef PLOT_H
#define PLOT_H

#include <cairo.h>
#include <math.h>
#include "line.h"

typedef struct {
    int draw_axes;
    float draw_grid;
    int opacity_by_intensity;
    int color_gradient;
    int show_fast_moves;
} PlotOptions;

static inline PlotOptions plot_options_default(void) {
    return (PlotOptions){0};
}

Line view_bounding_box(Line *lines, int count);
void plot_to_cr(cairo_t *cr, Line *lines, int count, PlotOptions opts);

#endif

