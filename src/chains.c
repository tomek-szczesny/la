#include "chains.h"

/**
 * find_chains - Identify chains of consecutive lines.
 *
 * Groups lines where endpoint of line i matches startpoint of line i+1.
 * Detects closed chains where the final line's endpoint matches the first
 * line's startpoint.
 */
void find_chains(Line *lines, int count, Chain *chains, int *ch_count) {
//  fprintf(stderr, "[chains] find_chains: processing %d lines\n", count);

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
//              fprintf(stderr, "[chains]   Chain %d: CLOSED loop, index %d, "
//                      "length %d\n", chain_idx, chain_start, chain_len);
            } else {
//              fprintf(stderr, "[chains]   Chain %d: open, index %d, length %d\n",
//                      chain_idx, chain_start, chain_len);
            }
        } else {
//          fprintf(stderr, "[chains]   Chain %d: singleton, index %d\n",
//                  chain_idx, chain_start);
        }

        chains[chain_idx].index = chain_start;
        chains[chain_idx].count = chain_len;
        chains[chain_idx].closed = is_closed;

        chain_idx++;
        i += chain_len;
    }

    *ch_count = chain_idx;
//  fprintf(stderr, "[chains] find_chains: identified %d chains total\n", chain_idx);
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
Point get_entry_point(Line *lines, Chain *chain, int config) {
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
Point get_exit_point(Line *lines, Chain *chain, int config) {
    Point p;
    
    if (chain->closed) {
        /* Closed chain: exit same as entry */
        p = get_entry_point(lines, chain, config);
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

