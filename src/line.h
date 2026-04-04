#ifndef line_h
#define line_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "matrix.h"

typedef struct {
    float x0, y0;  // start point (from previous line)
    float x1, y1;  // end point (current G1)
    float s;       // power (S parameter)
    float f;       // feed rate (F parameter)
} Line;

// Compare function for qsort
int compare_lines(const void *a, const void *b);

void remove_zero_lines(Line **lines, int *count);

void deduplicate_lines(Line **lines, int *count);

void print_line(Line *line, int index);

int are_collinear(Line *line1, Line *line2);

float project_point(Line *line, float px, float py);
int check_overlap(Line *line1, Line *line2);

void remove_overlapping_lines(Line **lines, int *count);

float calculate_travel_distance(Line *lines, int count);

float calculate_cut_distance(Line *lines, int count);

Line bounding_box(Line *lines, int count);

void transform_line(Line *line, Matrix m);

void transform_lines(Line *lines, int count, Matrix m);

void translate_lines(Line *lines, int count, float tx, float ty);

void rotate_lines(Line *lines, int count, float angle_deg);

#endif
