#include <stdio.h>
#include <stdlib.h>
#include "gcode.h"
#include "chains.h"

/* ============================================================================
 * PUBLIC VARIABLES
 * ============================================================================ */

Line *lines = NULL;
int line_count = 0;

Chain *chains = NULL;
int ch_count = 0;

/**
 * solution_init - Initialize a solution with all chains in natural order.
 *
 * Creates a Solution containing all chains from the global chains array,
 * each with config set to 0.
 *
 * @return Initialized Solution struct
 */
Solution solution_init(void) {
    Solution sol;
    sol.chains = (Chain *)malloc(ch_count * sizeof(Chain));
    sol.configs = (int *)malloc(ch_count * sizeof(int));
    sol.count = ch_count;
    sol.user_data = NULL;

    for (int i = 0; i < ch_count; i++) {
        sol.chains[i] = chains[i];
        sol.configs[i] = 0;
    }

//  fprintf(stderr, "[optimize] solution_init: created solution with %d chains\n",
//          sol.count);
    return sol;
}

/**
 * apply_solution - Reorder lines array according to solution.
 *
 * Reconstructs the global lines array in the order specified by the solution,
 * respecting chain configurations (forward/reversed for open, rotation for closed).
 * Replaces the global lines array.
 *
 * @param sol: Solution specifying chain order and configurations
 */
void apply_solution(Solution *sol) {
//  fprintf(stderr, "[optimize] apply_solution: applying solution with %d chains\n",
//          sol->count);

    Line *new_lines = (Line *)malloc(line_count * sizeof(Line));
    int line_pos = 0;

    for (int i = 0; i < sol->count; i++) {
        Chain *ch = &sol->chains[i];
        int cfg = sol->configs[i];

        if (ch->closed) {
            /* Closed chain: start from rotated position */
            for (int j = 0; j < ch->count; j++) {
                int src_idx = ch->index + (cfg + j) % ch->count;
                new_lines[line_pos++] = lines[src_idx];
            }
        } else {
            /* Open chain: forward or reversed */
            if (cfg == 0) {
                /* Forward */
                for (int j = 0; j < ch->count; j++) {
                    new_lines[line_pos++] = lines[ch->index + j];
                }
            } else {
                /* Reversed */
                for (int j = ch->count - 1; j >= 0; j--) {
                    Line *orig = &lines[ch->index + j];
                    Line rev;
                    rev.x0 = orig->x1;
                    rev.y0 = orig->y1;
                    rev.x1 = orig->x0;
                    rev.y1 = orig->y0;
                    rev.s = orig->s;
                    rev.f = orig->f;
                    new_lines[line_pos++] = rev;
                }
            }
        }
    }

    free(lines);
    lines = new_lines;
//  fprintf(stderr, "[optimize] apply_solution: reordered %d lines\n", line_pos);
}

/**
 * solution_cost 
 *
 * @param sol: Solution to analyze
 */
float solution_cost(Solution *sol) {
    float total_fast_move = 0.0f;

    for (int i = 0; i < sol->count - 1; i++) {
        Point exit_pt = get_exit_point(&sol->chains[i], sol->configs[i]);
        Point entry_pt = get_entry_point(&sol->chains[i + 1], sol->configs[i + 1]);
        total_fast_move += get_distance(exit_pt, entry_pt);
    }

    // Include travel from and to the (0,0) position
    Point origin = {0,0};
    total_fast_move += get_distance(origin, get_entry_point(&sol->chains[0], sol->configs[0]));
    total_fast_move += get_distance(origin, get_exit_point(&sol->chains[sol->count-1], sol->configs[sol->count-1]));
    return total_fast_move;
}

/**
 * Algorithm_1 - Greedy nearest neighbor optimization.
 * 
 * Builds a solution by repeatedly selecting the nearest unvisited chain
 * from the current position. Quick and useful as a baseline.
 * 
 * @param sol: Input/output solution (will be overwritten with result)
 */
void Algorithm_1(Solution *sol) {
    fprintf(stderr, "Fast travel distance: %g\n", solution_cost(sol));
    fprintf(stderr, "Algorithm_1: starting greedy nearest neighbor\n");
    
    Solution result;

    // Try all possible starting points
    for (int init_chain = 0; init_chain < ch_count; init_chain++) {
        int init_configs = chains[init_chain].closed ? chains[init_chain].count : 2;
        for (int init_cfg = 0; init_cfg < init_configs; init_cfg++) {
            
            result = solution_init();
            
            int *visited = (int *)malloc(ch_count * sizeof(int));
            for (int i = 0; i < ch_count; i++) {
                visited[i] = 0;
            }
            
            /* Start from first chain */
            int current_chain = init_chain;
            int current_config = init_cfg;
            result.count = 0;
            visited[current_chain] = 1;
            result.chains[result.count] = chains[current_chain];
            result.configs[result.count] = current_config;
            result.count++;
            
            /* Greedily select nearest neighbor */
            for (int iter = 1; iter < ch_count; iter++) {
                float best_dist = 1e9f;
                int best_chain = -1;
                int best_config = -1;
                
                Point current_exit = get_exit_point(&chains[current_chain], current_config);
                
                for (int next_ch = 0; next_ch < ch_count; next_ch++) {
                    if (visited[next_ch]) continue;
                    
                    int next_configs = chains[next_ch].closed ? chains[next_ch].count : 2;
                    
                    for (int next_cfg = 0; next_cfg < next_configs; next_cfg++) {
                        Point next_entry = get_entry_point(&chains[next_ch], next_cfg);
                        float dist = get_distance(current_exit, next_entry);
                        
                        if (dist < best_dist) {
                            best_dist = dist;
                            best_chain = next_ch;
                            best_config = next_cfg;
                        }
                    }
                    if (best_dist == 0) break;
                }
                
                if (best_chain == -1) {
                    fprintf(stderr, "[optimize] ERROR: no unvisited chain found\n");
                    break;
                }
                
                visited[best_chain] = 1;
                current_chain = best_chain;
                current_config = best_config;
                result.chains[result.count] = chains[best_chain];
                result.configs[result.count] = best_config;
                result.count++;
            }
            
            free(visited);

            if (solution_cost(&result) < solution_cost(sol)) {
            
                /* Copy result back to input solution */
                free(sol->chains);
                free(sol->configs);
                sol->chains = result.chains;
                sol->configs = result.configs;
                sol->count = result.count;
                fprintf(stderr, "Fast travel distance: %g\n", solution_cost(&result));
            }
        }
    }
}

int main(int argc, char *argv[]) {
    
    lines = parse_gcode_file(&line_count);
    if (!lines) {
        fprintf(stderr, "Failed to parse gcode file\n");
        return 1;
    }
    
    fprintf(stderr, "Loaded %d lines\n", line_count);
    
    /* Find chains */
    chains = (Chain *)malloc(line_count * sizeof(Chain));
    find_chains(lines, line_count, chains, &ch_count);
    fprintf(stderr, "Found %d chains\n", ch_count);
    
    /* Run Algorithm 1 */
    Solution sol = solution_init();
    Algorithm_1(&sol);
    
    /* Apply solution and output */
    float td = calculate_travel_distance(lines, line_count);
    fprintf(stderr, "Travel distance before optimizations: %g\n", td);
    apply_solution(&sol);
    td = calculate_travel_distance(lines, line_count);
    fprintf(stderr, "Travel distance after optimizations: %g\n", td);
    
    export_gcode(lines, line_count);
    
    /* Cleanup */
    free(lines);
    free(chains);
    free(sol.chains);
    free(sol.configs);
    
    return 0;
}

