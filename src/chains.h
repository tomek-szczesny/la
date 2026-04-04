#ifndef CHAINS_H
#define CHAINS_H

#include "line.h"
#include <math.h>

/* ============================================================================
 * HELPER STRUCTS
 * ============================================================================ */

typedef struct {
    float x, y;
} Point;

typedef struct {
    int index;      /* Index of first line in this chain within lines array */
    int count;      /* Number of consecutive lines in this chain */
    int closed;     /* 1 if closed loop, 0 if open chain */
} Chain;

typedef struct {
    Chain *chains;  /* Array of chains in this solution */
    int *configs;   /* Array of configs (same count as chains) */
    int count;      /* Number of chains in this solution */
    void *user_data;/* Pointer for user data (e.g., recursion context) */
} Solution;

inline int feq(float f1, float f2) {
    float diff = f1 - f2;
    if (diff < 0) diff = -diff;
    return diff < 1e-3;
}

void find_chains(Line *lines, int count, Chain *chains, int *ch_count);
Point get_entry_point(Line *lines, Chain *chain, int config);
Point get_exit_point(Line *lines, Chain *chain, int config);
float get_distance(Point p1, Point p2);

#endif /* CHAINS_H */

