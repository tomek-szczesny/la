#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

typedef struct {
    float x0, y0;  // start point (from previous line)
    float x1, y1;  // end point (current G1)
    float s;       // power (S parameter)
    float f;       // feed rate (F parameter)
} Line;

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

// Compare function for qsort
int compare_lines(const void *a, const void *b) {
    const Line *la = (const Line *)a;
    const Line *lb = (const Line *)b;

    if (la->x0 != lb->x0) return (la->x0 > lb->x0) - (la->x0 < lb->x0);
    if (la->y0 != lb->y0) return (la->y0 > lb->y0) - (la->y0 < lb->y0);
    if (la->x1 != lb->x1) return (la->x1 > lb->x1) - (la->x1 < lb->x1);
    if (la->y1 != lb->y1) return (la->y1 > lb->y1) - (la->y1 < lb->y1);
    return 0;
}

int detect_passes(Line *lines, int count) {
    if (count == 0) return 0;

    // Create a copy and sort it
    Line *sorted = malloc(count * sizeof(Line));
    if (!sorted) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    memcpy(sorted, lines, count * sizeof(Line));
    qsort(sorted, count, sizeof(Line), compare_lines);

    // Count minimum repetitions
    int min_passes = count;
    int current_count = 1;

    for (int i = 1; i < count; i++) {
        if (compare_lines(&sorted[i], &sorted[i-1]) == 0) {
            current_count++;
        } else {
            min_passes = (current_count < min_passes) ? current_count : min_passes;
            current_count = 1;
        }
    }
    min_passes = (current_count < min_passes) ? current_count : min_passes;

    free(sorted);
    return min_passes;
}

void remove_zero_lines(Line **lines, int *count) {
    Line *nozero = malloc(*count * sizeof(Line));
    if (!nozero) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    int nozero_count = 0;

    for (int i = 0; i < *count; i++) {
        float dx = (*lines)[i].x1 - (*lines)[i].x0;
        float dy = (*lines)[i].y1 - (*lines)[i].y0;
        float llen = sqrt(dx*dx+dy*dy);
        if (llen > 1e-4) {
            //fprintf(stderr, "Line %d carried over with length %f\n", i, llen);
            nozero[nozero_count++] = (*lines)[i];
        }
    }

    free(*lines);
    *lines = nozero;
    *count = nozero_count;
}

void deduplicate_lines(Line **lines, int *count) {
    Line *dedup = malloc(*count * sizeof(Line));
    if (!dedup) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    int dedup_count = 0;

    for (int i = 0; i < *count; i++) {
        int is_duplicate = 0;

        // Check if this line already exists in dedup array
        for (int j = 0; j < dedup_count; j++) {
            if (compare_lines(&(*lines)[i], &dedup[j]) == 0) {
                is_duplicate = 1;
                break;
            }
        }

        // Add if not duplicate
        if (!is_duplicate) {
            dedup[dedup_count++] = (*lines)[i];
        }
    }

    free(*lines);
    *lines = dedup;
    *count = dedup_count;
}

void print_line(Line *line, int index) {
    fprintf(stderr, "Line %d: (%.6f, %.6f) -> (%.6f, %.6f) S=%.2f F=%.2f\n",
            index, line->x0, line->y0, line->x1, line->y1, line->s, line->f);
}

int are_collinear(Line *line1, Line *line2) {
    // Vector from line1.start to line1.end
    double dx1 = (double)line1->x1 - (double)line1->x0;
    double dy1 = (double)line1->y1 - (double)line1->y0;

    // Vector from line1.start to line2.start
    double dx2 = (double)line2->x0 - (double)line1->x0;
    double dy2 = (double)line2->y0 - (double)line1->y0;

    // Cross product should be ~0
    double cross1 = dx1 * dy2 - dy1 * dx2;

    // Vector from line1.start to line2.end
    double dx3 = (double)line2->x1 - (double)line1->x0;
    double dy3 = (double)line2->y1 - (double)line1->y0;

    // Cross product should be ~0
    double cross2 = dx1 * dy3 - dy1 * dx3;

    // Use small epsilon for floating point comparison
    double epsilon = 1e-10;

    /*
    // DEBUG
    if (fabs(cross1) < epsilon && fabs(cross2) < epsilon) {
        fprintf(stderr, "Collinearity detected:\n");
        print_line(line1, 1);
        print_line(line2, 2);
    }
    // END DEBUG
    */

    return fabs(cross1) < epsilon && fabs(cross2) < epsilon;
}

// Project point onto line direction, returns parameter t
float project_point(Line *line, float px, float py) {
    float dx = line->x1 - line->x0;
    float dy = line->y1 - line->y0;
    float len_sq = dx * dx + dy * dy;

    if (len_sq < 1e-4) {
        fprintf(stderr, "ERROR: Zero length line detected in project_point(). This should not have happened!\n");
        exit(1);  // Degenerate line
    }

    float dpx = px - line->x0;
    float dpy = py - line->y0;
    return (dpx * dx + dpy * dy) / len_sq;
}

// Returns: -1 = no overlap, 0 = line2 embedded in line1,
//          1 = line1 embedded in line2, 2 = partial overlap
int check_overlap(Line *line1, Line *line2) {
    float t2_start = project_point(line1, line2->x0, line2->y0);
    float t2_end = project_point(line1, line2->x1, line2->y1);

    // Ensure t2_start <= t2_end
    if (t2_start > t2_end) {
        float temp = t2_start;
        t2_start = t2_end;
        t2_end = temp;
    }

    // No overlap if completely outside [0,1]
    if (t2_end < -1e-7 || t2_start > 1 + 1e-7) return -1;

    // line2 embedded in line1
    if (t2_start > -1e-7 && t2_end < 1 + 1e-7) return 0;

    // line1 embedded in line2
    if (t2_start < -1e-7 && t2_end > 1 + 1e-7) return 1;

    // Partial overlap
    return 2;
}

void remove_overlapping_lines(Line **lines, int *count) {
    int i = 0;

    while (i < *count) {
        int j = i + 1;
        int removed_any = 0;

        while (j < *count) {
            if (!are_collinear(&(*lines)[i], &(*lines)[j])) {
                j++;
                continue;
            }

            int overlap = check_overlap(&(*lines)[i], &(*lines)[j]);

            if (overlap == -1) {
                // No overlap, move to next
                j++;
            } else if (overlap == 0) {
                // line[j] embedded in line[i], remove j
                //fprintf(stderr, "Line %d embedded in line %d, removing\n", j, i);
                memmove(&(*lines)[j], &(*lines)[j+1],
                        (*count - j - 1) * sizeof(Line));
                (*count)--;
                removed_any = 1;
            } else if (overlap == 1) {
                // line[i] embedded in line[j], remove i and restart
                //fprintf(stderr, "Line %d embedded in line %d, removing\n", i, j);
                memmove(&(*lines)[i], &(*lines)[i+1],
                        (*count - i - 1) * sizeof(Line));
                (*count)--;
                removed_any = 1;
                break;
            } else if (overlap == 2) {
                // Partial overlap - merge into line[i]
                //fprintf(stderr, "Lines %d and %d partially overlap, merging\n", i, j);
                float t_j_start = project_point(&(*lines)[i], (*lines)[j].x0, (*lines)[j].y0);
                float t_j_end = project_point(&(*lines)[i], (*lines)[j].x1, (*lines)[j].y1);

                float t_min = fminf(0, fminf(t_j_start, t_j_end));
                float t_max = fmaxf(1, fmaxf(t_j_start, t_j_end));

                (*lines)[i].x0 += t_min * ((*lines)[i].x1 - (*lines)[i].x0);
                (*lines)[i].y0 += t_min * ((*lines)[i].y1 - (*lines)[i].y0);
                (*lines)[i].x1 = (*lines)[i].x0 + t_max * ((*lines)[i].x1 - (*lines)[i].x0);
                (*lines)[i].y1 = (*lines)[i].y0 + t_max * ((*lines)[i].y1 - (*lines)[i].y0);

                // Remove line[j]
                memmove(&(*lines)[j], &(*lines)[j+1],
                        (*count - j - 1) * sizeof(Line));
                (*count)--;
                removed_any = 1;
            }
        }

        if (!removed_any) i++;
    }
}

float calculate_travel_distance(Line *lines, int count) {
    float total = 0.0;
    float current_x = 0.0, current_y = 0.0;

    for (int i = 0; i < count; i++) {
        // Travel from current position to start of cut
        float dx = lines[i].x0 - current_x;
        float dy = lines[i].y0 - current_y;
        total += sqrtf(dx * dx + dy * dy);

        // Update position to end of cut
        current_x = lines[i].x1;
        current_y = lines[i].y1;
    }

    // Travel back to origin at the end
    float dx = 0.0 - current_x;
    float dy = 0.0 - current_y;
    total += sqrtf(dx * dx + dy * dy);

    return total;
}

float calculate_cut_distance(Line *lines, int count) {
    float total = 0.0;

    for (int i = 0; i < count; i++) {
        float dx = lines[i].x1 - lines[i].x0;
        float dy = lines[i].y1 - lines[i].y0;
        total += sqrtf(dx * dx + dy * dy);
    }

    return total;
}

typedef struct {
    int start;   // Index of first line in chain
    int end;     // Index of last line in chain
    float start_x, start_y;  // Beginning of chain
    float end_x, end_y;      // End of chain
} Chain;

Chain* identify_chains(Line *lines, int count, int *out_chain_count) {
    Chain *chains = malloc(count * sizeof(Chain));
    if (!chains) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    int chain_count = 0;
    int chain_start = 0;

    for (int i = 1; i < count; i++) {
        // Check if line i connects to line i-1
        if (lines[i].x0 != lines[i-1].x1 || lines[i].y0 != lines[i-1].y1) {
            // Chain breaks, save previous chain
            chains[chain_count].start = chain_start;
            chains[chain_count].end = i - 1;
            chains[chain_count].start_x = lines[chain_start].x0;
            chains[chain_count].start_y = lines[chain_start].y0;
            chains[chain_count].end_x = lines[i-1].x1;
            chains[chain_count].end_y = lines[i-1].y1;
            chain_count++;
            chain_start = i;
        }
    }

    // Save final chain
    chains[chain_count].start = chain_start;
    chains[chain_count].end = count - 1;
    chains[chain_count].start_x = lines[chain_start].x0;
    chains[chain_count].start_y = lines[chain_start].y0;
    chains[chain_count].end_x = lines[count-1].x1;
    chains[chain_count].end_y = lines[count-1].y1;
    chain_count++;

    *out_chain_count = chain_count;
    return chains;
}


void optimize_chains_recursive(Line *lines, Chain *chains, int chain_count,
                               int *metachain, int metachain_len,
                               int *used, float current_travel,
                               float *best_travel, int *best_metachain,
                               int *best_reversed) {

    // Progress reporting
    static int last_reported = -1;
    if (metachain_len == 3) {
        int progress = (metachain[0] * 100) / chain_count;
        if (progress != last_reported) {
            fprintf(stderr, "Progress: %d%%\n", progress);
            last_reported = progress;
        }
    }

    // Base case: all chains used
    if (metachain_len == chain_count) {
        if (current_travel < *best_travel) {
            *best_travel = current_travel;
            memcpy(best_metachain, metachain, chain_count * sizeof(int));
            memcpy(best_reversed, used + chain_count, chain_count * sizeof(int));
            fprintf(stderr, "New best solution found: %.2f units travel\n", current_travel);
        }
        return;
    }
    
    // Pruning: if current travel already exceeds best, stop
    if (current_travel >= *best_travel) {
        return;
    }
    
    float current_x = (metachain_len == 0) ? 0.0 : 
                      (used[metachain[metachain_len-1] + chain_count] ? 
                       chains[metachain[metachain_len-1]].start_x :
                       chains[metachain[metachain_len-1]].end_x);
    float current_y = (metachain_len == 0) ? 0.0 : 
                      (used[metachain[metachain_len-1] + chain_count] ? 
                       chains[metachain[metachain_len-1]].start_y :
                       chains[metachain[metachain_len-1]].end_y);
    
    // Try each unused chain in both orientations
    for (int i = 0; i < chain_count; i++) {
        if (used[i]) continue;  // Already used
        
        // Try normal orientation
        float dx = chains[i].start_x - current_x;
        float dy = chains[i].start_y - current_y;
        float travel_normal = sqrtf(dx * dx + dy * dy);
        
        used[i] = 1;
        used[i + chain_count] = 0;  // not reversed
        metachain[metachain_len] = i;
        optimize_chains_recursive(lines, chains, chain_count, metachain, 
                                 metachain_len + 1, used, 
                                 current_travel + travel_normal,
                                 best_travel, best_metachain, best_reversed);
        
        // Try reversed orientation
        dx = chains[i].end_x - current_x;
        dy = chains[i].end_y - current_y;
        float travel_reversed = sqrtf(dx * dx + dy * dy);
        
        used[i + chain_count] = 1;  // reversed
        optimize_chains_recursive(lines, chains, chain_count, metachain, 
                                 metachain_len + 1, used, 
                                 current_travel + travel_reversed,
                                 best_travel, best_metachain, best_reversed);
        
        used[i] = 0;
    }
}

