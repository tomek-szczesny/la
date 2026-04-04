// svg_path.h
#ifndef SVG_PATH_H
#define SVG_PATH_H

#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "line.h"

// Optimal control point distance for approximating circular arcs with cubic Bezier curves
// Derived from: 4/3 * tan(π/8) ≈ 0.5522847498
// Minimizes the maximum radial error when approximating a quarter circle
#define BEZIER_CIRCLE_K 0.5522847498f

void add_line(float x0, float y0, float x1, float y1, Matrix m);

// Get path data attribute (handles arbitrary length)
const char* getpath(xmlNodePtr node);

// Plot a straight line segment
void plot_line(float x, float y, Matrix transform);

// Approximate quadratic Bezier curve with line segments
// P0 = (path_x, path_y), P1 = control point, P2 = end point
// max_error: maximum deviation from true curve
void plot_quadratic_bezier(float cx, float cy, float x, float y, Matrix transform, float max_error);

// Approximate cubic Bezier curve with line segments
// P0 = (path_x, path_y), P1 = control1, P2 = control2, P3 = end point
// max_error: maximum deviation from true curve
void plot_cubic_bezier(float c1x, float c1y, float c2x, float c2y, float x, float y, Matrix transform, float max_error);

// Approximate arc with line segments
// rx, ry: radii
// x_axis_rotation: rotation in degrees
// large_arc_flag, sweep_flag: arc flags
// x, y: end point
void plot_arc(float rx, float ry, float x_axis_rotation, int large_arc_flag, int sweep_flag, float x, float y, Matrix transform, float max_error);

// Change plotter's absolute position
// stored in static (private) variables inside svg_path.c
void set_pos(float x, float y0);

// Process path data
void parse_path_data(const char *path_str, Matrix transform, float max_error);

#endif // SVG_PATH_H

