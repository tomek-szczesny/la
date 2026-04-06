#include "plot.h"

static float _min_intensity = 0, _max_intensity = 0;

static void _compute_intensity_range(Line *lines, int count) {
    _min_intensity = INFINITY;
    _max_intensity = -INFINITY;

    for (int i = 0; i < count; i++) {
        float intensity = lines[i].s / lines[i].f;
        if (intensity < _min_intensity) _min_intensity = intensity;
        if (intensity > _max_intensity) _max_intensity = intensity;
    }
}

static void _set_line_color(cairo_t *cr, int index, int count, Line *line, PlotOptions opts) {
    float opacity = 1.0;
    float r = 0, g = 0, b = 0;

    if (opts.opacity_by_intensity && _max_intensity > _min_intensity) {
        float intensity = line->s / line->f;
        opacity = 0.2 + 0.8 * (intensity - _min_intensity) / (_max_intensity - _min_intensity);
    }

    const float r0 = 1.0, g0 = 45.0/255.0, b0 = 149.0/255.0; // #FF2D95 (magenta)
    const float r1 = 0.0, g1 = 180.0/255.0, b1 = 216.0/255.0; // #00B4D8 (cyan)
    if (opts.color_gradient) {
        float t = (count > 1) ? (float)index / (count - 1) : 0;
        r = r0 * (1.0f - t) + r1 * t;
        g = g0 * (1.0f - t) + g1 * t;
        b = b0 * (1.0f - t) + b1 * t;
    } else {
        r = g = b = 0.0;
    }

    cairo_set_source_rgba(cr, r, g, b, opacity);
}

Line view_bounding_box(Line *lines, int count) {
    Line bbox = bounding_box(lines, count);
    float min_x = bbox.x0, max_x = bbox.x1;
    float min_y = bbox.y0, max_y = bbox.y1;
    float range_x = max_x - min_x;
    float range_y = max_y - min_y;

    // Expand and include origin
    float pad_x = range_x * 0.2;
    float pad_y = range_y * 0.2;
    float view_x0 = min_x - pad_x, view_x1 = max_x + pad_x;
    float view_y0 = min_y - pad_y, view_y1 = max_y + pad_y;

    if (view_x0 > 0) view_x0 = 0;
    if (view_y0 > 0) view_y0 = 0;
    if (view_x1 < 0) view_x1 = 0;
    if (view_y1 < 0) view_y1 = 0;

    Line ret;
    ret.x0 = view_x0;
    ret.x1 = view_x1;
    ret.y0 = view_y0;
    ret.y1 = view_y1;

    return ret;
}

static int _grid_step(float range) {
    int step = 1;
    while (range / step > 50) step *= 10;
    while (range / step < 5) step /= 10;
    return step;
}

static void _plot_axes_and_grid(cairo_t *cr, Line *lines, int count) {
    if (count == 0) return;

    Line bbox = view_bounding_box(lines, count);
    float view_x0 = bbox.x0, view_x1 = bbox.x1;
    float view_y0 = bbox.y0, view_y1 = bbox.y1;

    float plot_width = view_x1 - view_x0;
    float plot_height = view_y1 - view_y0;

    // Grid
    int step_x = _grid_step(plot_width);
    int step_y = _grid_step(plot_height);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.25);
    cairo_set_line_width(cr, 0.1);

    for (int x = (int)ceil(view_x0 / step_x) * step_x; x <= view_x1; x += step_x) {
        cairo_move_to(cr, x, view_y0);
        cairo_line_to(cr, x, view_y1);
    }
    for (int y = (int)ceil(view_y0 / step_y) * step_y; y <= view_y1; y += step_y) {
        cairo_move_to(cr, view_x0, y);
        cairo_line_to(cr, view_x1, y);
    }
    cairo_stroke(cr);

    // Axes
    cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
    cairo_set_line_width(cr, 0.2);
    cairo_move_to(cr, view_x0, 0);
    cairo_line_to(cr, view_x1, 0);
    cairo_move_to(cr, 0, view_y0);
    cairo_line_to(cr, 0, view_y1);
    cairo_stroke(cr);
}

static void _draw_fast_move(cairo_t *cr, Line *lines, int i, PlotOptions opts) {
    if (!opts.show_fast_moves || i == 0) return;

    float prev_x1 = lines[i-1].x1;
    float prev_y1 = lines[i-1].y1;
    float curr_x0 = lines[i].x0;
    float curr_y0 = lines[i].y0;

    // Check if there's a discontinuity
    if (fabs(prev_x1 - curr_x0) < 1e-6 && fabs(prev_y1 - curr_y0) < 1e-6) {
        return;
    }

    // Draw dotted line
    cairo_stroke(cr);
    double dashes[] = {1, 1};
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_move_to(cr, prev_x1, prev_y1);
    cairo_line_to(cr, curr_x0, curr_y0);
    cairo_stroke(cr);

    // Restore settings
    cairo_set_dash(cr, NULL, 0, 0);
}

// Move the entire plot to fit negative coordinates in non-negative space
// Also flip it to match Y axis direction in PDF and SVG
static void _transform(cairo_t *cr, Line *lines, int count) {
    if (count == 0) return;

    Line bbox = view_bounding_box(lines, count);
    float view_x0 = bbox.x0, view_x1 = bbox.x1;
    float view_y0 = bbox.y0, view_y1 = bbox.y1;

    // The center of the entire view
    float bbcx = (view_x0 + view_x1)/2;
    float bbcy = (view_y0 + view_y1)/2;
    
    cairo_matrix_t m;
    cairo_matrix_init_translate(&m, -view_x0, -view_y0);        // global translate
    cairo_matrix_translate(&m, bbcx, bbcy);             // move origin to center
    cairo_matrix_scale(&m, 1.0, -1.0);              // flip vertically
    cairo_matrix_translate(&m, -bbcx, -bbcy);           // move origin back
    cairo_transform(cr, &m);
}

void plot_to_cr(cairo_t *cr, Line *lines, int count, PlotOptions opts) {
    if (count == 0) return;

    _transform(cr, lines, count);

    if (opts.draw_axes) {
        _plot_axes_and_grid(cr, lines, count);
    }

    if (opts.opacity_by_intensity || opts.color_gradient) {
        _compute_intensity_range(lines, count);
    }

    //cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 0.2);

    float polyline_start_x = 0, polyline_start_y = 0;
    float current_x = 0, current_y = 0;
    int in_polyline = 0;

    for (int i = 0; i < count; i++) {
        // Check if line continues from current position
        if (!in_polyline ||
            fabs(lines[i].x0 - current_x) > 1e-6 ||
            fabs(lines[i].y0 - current_y) > 1e-6) {

            _draw_fast_move(cr, lines, i, opts);
            // Start new polyline
            if (in_polyline) {
                cairo_stroke(cr);
            }
            cairo_move_to(cr, lines[i].x0, lines[i].y0);
            polyline_start_x = lines[i].x0;
            polyline_start_y = lines[i].y0;
            in_polyline = 1;
        }

        _set_line_color(cr, i, count, &lines[i], opts);
        cairo_line_to(cr, lines[i].x1, lines[i].y1);
        current_x = lines[i].x1;
        current_y = lines[i].y1;

        // Close path if we've returned to start
        if (fabs(current_x - polyline_start_x) < 1e-6 &&
            fabs(current_y - polyline_start_y) < 1e-6) {
            cairo_close_path(cr);
            cairo_stroke(cr);
            in_polyline = 0;
        }
    }

    if (in_polyline) {
        cairo_stroke(cr);
    }
}

