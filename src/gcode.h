#ifndef gcode_h
#define gcode_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "line.h"

typedef struct {
    char c;      // Capital letter token
    int i;       // Integer value
    float f;     // Float value
} gcode_token;

gcode_token gcode_get_token(void);
Line* parse_gcode_file(int *out_count);
void export_gcode(Line *lines, int count);

#endif
