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

/* ============================================================================
 * HELPER FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

/* External variables (defined in main .c file) */
extern Line *lines;
extern int line_count;
//extern Chain *chains = NULL;
//extern int ch_count = 0;
//extern float *dist_cache = NULL;

/**
 * feq - Compare two floats for equality with tolerance (1e-3).
 */
int feq(float f1, float f2) {
    float diff = f1 - f2;
    if (diff < 0) diff = -diff;
    return diff < 1e-3;
}

/**
 * find_chains - Identify chains of consecutive lines.
 *
 * Groups lines where endpoint of line i matches startpoint of line i+1.
 * Detects closed chains where the final line's endpoint matches the first
 * line's startpoint.
 */
void find_chains(Line *lines, int count, Chain *chains, int *ch_count) {
    fprintf(stderr, "[chains] find_chains: processing %d lines\n", count);

    int chain_idx = 0;
    int i = 0;

    while (i < count) {
        int chain_start = i;
        int chain_len = 1;

        /* Extend chain as long as endpoints match */
        while (i + chain_len < count) {
            Line *current = &lines[i + chain_len - 1];
            Line *next = &lines[i + chain_len];

            /* Check if current line's end matches next line's start */
            if (feq(current->x1, next->x0) && feq(current->y1, next->y0)) {
                chain_len++;
            } else {
                break;
            }
        }

        /* Check if chain is closed (last line endpoint == first line startpoint) */
        int is_closed = 0;
        if (chain_len > 1) {
            Line *first = &lines[chain_start];
            Line *last = &lines[chain_start + chain_len - 1];

            if (feq(last->x1, first->x0) && feq(last->y1, first->y0)) {
                is_closed = 1;
                fprintf(stderr, "[chains]   Chain %d: CLOSED loop, index %d, "
                        "length %d\n", chain_idx, chain_start, chain_len);
            } else {
                fprintf(stderr, "[chains]   Chain %d: open, index %d, length %d\n",
                        chain_idx, chain_start, chain_len);
            }
        } else {
            fprintf(stderr, "[chains]   Chain %d: singleton, index %d\n",
                    chain_idx, chain_start);
        }

        chains[chain_idx].index = chain_start;
        chains[chain_idx].count = chain_len;
        chains[chain_idx].closed = is_closed;

        chain_idx++;
        i += chain_len;
    }

    *ch_count = chain_idx;
    fprintf(stderr, "[chains] find_chains: identified %d chains total\n", chain_idx);
}

/**
 * get_entry_point - Get the entry point of a chain.
 * 
 * For open chains:
 *   config=0 (forward): entry is start of first line (x0, y0)
 *   config=1 (reversed): entry is end of last line (x1, y1)
 * 
 * For closed chains:
 *   entry is the start of the line at index (chain->index + config)
 *   This allows rotation of the loop entry/exit point.
 */
Point get_entry_point(Chain *chain, int config) {
    Point p;
    
    if (chain->closed) {
        /* Closed chain: config is rotation index within the chain */
        int line_idx = chain->index + config;
        p.x = lines[line_idx].x0;
        p.y = lines[line_idx].y0;
//      fprintf(stderr, "[chains] get_entry_point: closed chain at index %d, "
//              "config %d -> line %d (%.2f, %.2f)\n",
//              chain->index, config, line_idx, p.x, p.y);
    } else {
        /* Open chain: config is 0 (forward) or 1 (reversed) */
        if (config == 0) {
            /* Forward: entry is start of first line */
            int line_idx = chain->index;
            p.x = lines[line_idx].x0;
            p.y = lines[line_idx].y0;
//          fprintf(stderr, "[chains] get_entry_point: open chain (forward) "
//                  "at index %d -> line %d (%.2f, %.2f)\n",
//                  chain->index, line_idx, p.x, p.y);
        } else {
            /* Reversed: entry is end of last line */
            int line_idx = chain->index + chain->count - 1;
            p.x = lines[line_idx].x1;
            p.y = lines[line_idx].y1;
//          fprintf(stderr, "[chains] get_entry_point: open chain (reversed) "
//                  "at index %d -> line %d (%.2f, %.2f)\n",
//                  chain->index, line_idx, p.x, p.y);
        }
    }
    
    return p;
}

/**
 * get_exit_point - Get the exit point of a chain.
 * 
 * For open chains:
 *   config=0 (forward): exit is end of last line (x1, y1)
 *   config=1 (reversed): exit is start of first line (x0, y0)
 * 
 * For closed chains:
 *   exit is same as entry (the loop returns to itself)
 */
Point get_exit_point(Chain *chain, int config) {
    Point p;
    
    if (chain->closed) {
        /* Closed chain: exit same as entry */
        p = get_entry_point(chain, config);
//      fprintf(stderr, "[chains] get_exit_point: closed chain -> same as entry "
//              "(%.2f, %.2f)\n", p.x, p.y);
    } else {
        /* Open chain: opposite of entry */
        if (config == 0) {
            /* Forward: exit is end of last line */
            int line_idx = chain->index + chain->count - 1;
            p.x = lines[line_idx].x1;
            p.y = lines[line_idx].y1;
//          fprintf(stderr, "[chains] get_exit_point: open chain (forward) "
//                  "at index %d -> line %d (%.2f, %.2f)\n",
//                  chain->index, line_idx, p.x, p.y);
        } else {
            /* Reversed: exit is start of first line */
            int line_idx = chain->index;
            p.x = lines[line_idx].x0;
            p.y = lines[line_idx].y0;
//          fprintf(stderr, "[chains] get_exit_point: open chain (reversed) "
//                  "at index %d -> line %d (%.2f, %.2f)\n",
//                  chain->index, line_idx, p.x, p.y);
        }
    }
    
    return p;
}

/**
 * get_distance - Calculate Euclidean distance between two points.
 */
float get_distance(Point p1, Point p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float dist = sqrtf(dx * dx + dy * dy);
    
//  fprintf(stderr, "[chains] get_distance: (%.2f, %.2f) -> (%.2f, %.2f) = %.2f\n",
//          p1.x, p1.y, p2.x, p2.y, dist);
    
    return dist;
}

/**
 * precompute_distances - Precompute all inter-chain distances.
 *
 * Allocates a 2D matrix and fills it with distances between all pairs of chains
 * in all possible configurations. This avoids recalculating distances during
 * optimization algorithms.
 *
 * Matrix layout:
 *   dist_cache[from_chain * ch_count + to_chain][from_config * (to_cfg_max+1) + to_config]
 *   where to_cfg_max depends only on the "to" chain's properties.
 */
void precompute_distances(Chain *chains, int ch_count, float **dist_cache) {
    fprintf(stderr, "[chains] precompute_distances: ch_count=%d\n", ch_count);

    /* Determine max configs needed per chain */
    int max_configs = 0;
    for (int i = 0; i < ch_count; i++) {
        int chain_configs = chains[i].closed ? chains[i].count : 2;
        if (chain_configs > max_configs) {
            max_configs = chain_configs;
        }
    }
    fprintf(stderr, "[chains]   max_configs per chain: %d\n", max_configs);

    /* Allocate 2D matrix: [ch_count * ch_count][max_configs * max_configs] */
    int matrix_rows = ch_count * ch_count;
    int matrix_cols = max_configs * max_configs;

    *dist_cache = (float *)malloc(matrix_rows * matrix_cols * sizeof(float));
    if (!*dist_cache) {
        fprintf(stderr, "[chains] ERROR: failed to allocate dist_cache\n");
        return;
    }

    fprintf(stderr, "[chains]   allocated %.2f MB for distance matrix\n",
            (matrix_rows * matrix_cols * sizeof(float)) / (1024.0 * 1024.0));

    /* Fill matrix */
    int computed = 0;
    for (int from_ch = 0; from_ch < ch_count; from_ch++) {
        int from_configs = chains[from_ch].closed ? chains[from_ch].count : 2;

        for (int to_ch = 0; to_ch < ch_count; to_ch++) {
            int to_configs = chains[to_ch].closed ? chains[to_ch].count : 2;

            for (int from_cfg = 0; from_cfg < from_configs; from_cfg++) {
                for (int to_cfg = 0; to_cfg < to_configs; to_cfg++) {
                    Point exit_pt = get_exit_point(&chains[from_ch], from_cfg);
                    Point entry_pt = get_entry_point(&chains[to_ch], to_cfg);
                    float dist = get_distance(exit_pt, entry_pt);

                    int row = from_ch * ch_count + to_ch;
                    int col = from_cfg * (to_configs) + to_cfg;
                    (*dist_cache)[row * matrix_cols + col] = dist;

                    computed++;
                }
            }
        }
    }

    fprintf(stderr, "[chains]   computed %d distances\n", computed);
}



#endif /* CHAINS_H */

