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

gcode_token gcode_get_token(void) {
    gcode_token token = {0, 0, 0.0f};
    int ch;

    // Skip whitespace and handle comments
    while ((ch = getchar()) != EOF) {
        if (ch == ';') {
            // Comment detected - discard until LF
            while ((ch = getchar()) != EOF && ch != '\n');
            continue;
        }

        if (isupper(ch)) {
            // Found token letter
            token.c = ch;
            break;
        }

        // Skip anything else
    }

    if (token.c == 0) {
        return token;  // EOF reached
    }

    // Parse the number (integer or float)
    char num_buffer[32] = {0};
    int num_idx = 0;
    int has_dot = 0;

    while ((ch = getchar()) != EOF) {
        if (isdigit(ch)) {
            if (num_idx < 31) {
                num_buffer[num_idx++] = ch;
            }
        } else if (ch == '.' && !has_dot) {
            has_dot = 1;
            if (num_idx < 31) {
                num_buffer[num_idx++] = ch;
            }
        } else if (ch == '-' && num_idx == 0) {
            // Negative sign at start
            if (num_idx < 31) {
                num_buffer[num_idx++] = ch;
            }
        } else {
            // End of number - put character back
            ungetc(ch, stdin);
            break;
        }
    }

    if (num_idx > 0) {
        token.f = strtof(num_buffer, NULL);
        token.i = (int)token.f;
    }

    return token;
}

Line* parse_gcode_file(int *out_count) {
    Line *lines = malloc(sizeof(Line) * 1024);  // Initial capacity
    int capacity = 1024;
    int count = 0;
    
    gcode_token token;
    float current_x = 0.0f, current_y = 0.0f;
    float feed_rate = 0.0f, power = 0.0f;
    int absolute_mode = 1;  // G90 = absolute, G91 = relative
    int last_was_g1 = 0;    // Track if last G-code was G1
    
    while (1) {
        token = gcode_get_token();
        
        if (token.c == 0) {
            // EOF reached
            break;
        }
        
        switch (token.c) {
            case 'G': {
                // G-code command
                switch (token.i) {
                    case 0:
                        // G0 - rapid move (no cutting)
                        last_was_g1 = 0;
                        break;
                    
                    case 1: {
                        // G1 - linear move (cutting)
                        if (count >= capacity) {
                            capacity *= 2;
                            lines = realloc(lines, sizeof(Line) * capacity);
                        }
                        
                        lines[count].x0 = current_x;
                        lines[count].y0 = current_y;
                        lines[count].x1 = current_x;
                        lines[count].y1 = current_y;
                        lines[count].s = power;
                        lines[count].f = feed_rate;
                        
                        count++;
                        last_was_g1 = 1;
                        break;
                    }
                    
                    case 90:
                        // G90 - absolute positioning
                        absolute_mode = 1;
                        break;
                    
                    case 91:
                        // G91 - relative positioning
                        absolute_mode = 0;
                        break;
                    
                    default:
                        // Ignore unknown G-codes
                        break;
                }
                break;
            }
            
            case 'X': {
                // X coordinate
                if (absolute_mode) {
                    current_x = token.f;
                } else {
                    current_x += token.f;
                }
                
                // Update last line's end point only if last G-code was G1
                if (last_was_g1 && count > 0) {
                    lines[count - 1].x1 = current_x;
                }
                break;
            }
            
            case 'Y': {
                // Y coordinate
                if (absolute_mode) {
                    current_y = token.f;
                } else {
                    current_y += token.f;
                }
                
                // Update last line's end point only if last G-code was G1
                if (last_was_g1 && count > 0) {
                    lines[count - 1].y1 = current_y;
                }
                break;
            }
            
            case 'S': {
                // Power/laser strength
                power = token.f;
                
                // Update last line's power only if last G-code was G1
                if (last_was_g1 && count > 0) {
                    lines[count - 1].s = power;
                }
                break;
            }
            
            case 'F': {
                // Feed rate
                feed_rate = token.f;
                
                // Update last line's feed rate only if last G-code was G1
                if (last_was_g1 && count > 0) {
                    lines[count - 1].f = feed_rate;
                }
                break;
            }
            
            default:
                // Ignore unknown parameters
                break;
        }
    }
    
    *out_count = count;
    return lines;
}



void export_gcode(Line *lines, int count) {
    float last_s = -1, last_f = -1;

    for (int i = 0; i < count; i++) {
        // Check if we need a G0 move
        if (i == 0 || lines[i].x0 != lines[i-1].x1 || lines[i].y0 != lines[i-1].y1) {
            printf("G0 X%.2f Y%.2f\n", lines[i].x0, lines[i].y0);
        }

        // Always print G1 with X1, Y1
        printf("G1 X%.2f Y%.2f", lines[i].x1, lines[i].y1);

        // Add S if different from last
        if (lines[i].s != last_s) {
            printf(" S%.2f", lines[i].s);
            last_s = lines[i].s;
        }

        // Add F if different from last
        if (lines[i].f != last_f) {
            printf(" F%.0f", lines[i].f);
            last_f = lines[i].f;
        }

        printf("\n");
    }
}

#endif
