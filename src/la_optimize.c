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

float *dist_cache = NULL;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/**
 * fetch_dist - Retrieve precomputed distance from cache.
 * 
 * Returns the distance from exit of chain1 with config1 to entry of chain2
 * with config2.
 * 
 * @param chain1: Index of source chain
 * @param chain2: Index of destination chain
 * @param conf1: Configuration of source chain
 * @param conf2: Configuration of destination chain
 * 
 * @return Distance value from cache
 */
float inline fetch_dist(int chain1, int chain2, int conf1, int conf2) {
    
    int to_configs = chains[chain2].closed ? chains[chain2].count : 2;
    int row = chain1 * ch_count + chain2;
    int col = conf1 * to_configs + conf2;
    
    float dist = dist_cache[row * (ch_count * ch_count) + col];
    
//    fprintf(stderr, "[optimize] fetch_dist: chain%d(cfg%d) -> chain%d(cfg%d) = %.2f\n",
//            chain1, conf1, chain2, conf2, dist);
    
    return dist;
}

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

    fprintf(stderr, "[optimize] solution_init: created solution with %d chains\n",
            sol.count);
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
    fprintf(stderr, "[optimize] apply_solution: applying solution with %d chains\n",
            sol->count);

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
    fprintf(stderr, "[optimize] apply_solution: reordered %d lines\n", line_pos);
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
    return total_fast_move;
}

/**
 * print_solution_stats - Print human-readable statistics about a solution.
 *
 * Displays total fast move distance and completion percentage if incomplete.
 *
 * @param sol: Solution to analyze
 */
void print_solution_stats(Solution *sol) {
    float total_fast_move = solution_cost(sol);

    float completion = (sol->count > 0) ? (100.0f * sol->count / ch_count) : 0.0f;

    fprintf(stderr, "\n[optimize] === SOLUTION STATS ===\n");
    fprintf(stderr, "[optimize] Chains included: %d / %d (%.1f%%)\n",
            sol->count, ch_count, completion);
    fprintf(stderr, "[optimize] Total fast move distance: %.2f\n", total_fast_move);
    fprintf(stderr, "[optimize] =====================\n\n");
}

/**
 * Algorithm_1 - Greedy nearest neighbor optimization.
 * 
 * Builds a solution by repeatedly selecting the nearest unvisited chain
 * from the current position. Quick and useful as a baseline.
 * 
 * @param sol: Input/output solution (will be overwritten with result)
 * @param depth: Recursion depth (unused in this algorithm, kept for interface)
 */
void Algorithm_1(Solution *sol, int depth) {
    fprintf(stderr, "[optimize] Algorithm_1: starting greedy nearest neighbor\n");
    
    int *visited = (int *)malloc(ch_count * sizeof(int));
    for (int i = 0; i < ch_count; i++) {
        visited[i] = 0;
    }
    
    Solution result = solution_init();
    result.count = 0;
    
    /* Start from first chain */
    int current_chain = 0;
    int current_config = 0;
    visited[current_chain] = 1;
    result.chains[result.count] = chains[current_chain];
    result.configs[result.count] = current_config;
    result.count++;
    
    fprintf(stderr, "[optimize]   selected chain %d with config %d\n",
            current_chain, current_config);
    
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
        
        fprintf(stderr, "[optimize]   selected chain %d with config %d (dist=%.2f)\n",
                best_chain, best_config, best_dist);
    }
    
    free(visited);
    
    /* Copy result back to input solution */
    free(sol->chains);
    free(sol->configs);
    sol->chains = result.chains;
    sol->configs = result.configs;
    sol->count = result.count;
    
    print_solution_stats(sol);
}

/**
 * Algorithm_2 - Picky nearest neighbor optimization.
 * 
 * Builds a solution by repeatedly selecting the nearest unvisited chain
 * from the current position. If there are many equally nearby choices, explores all of them.
 * 
 * @param sol: Input/output solution (will be overwritten with result)
 * @param depth: Recursion depth (unused in this algorithm, kept for interface)
 */
/*
typedef struct {    // For selecting the next move (ability to sort)
    float cost;
    Chain * chain;
    int config;
} a2_rank;
int comp_a2_rank(const void* a, const void* b) {
    const a2_rank *ra = (const a2_rank *)a;
    const a2_rank *rb = (const a2_rank *)b;
    if (ra->cost > rb->cost) return 1;
    if (ra->cost < rb->cost) return -1;
    return 0;
}

static void a2_recurse(int *visited, Solution *current, float current_cost) {
    int rank_cap = 10000;
    int rank_count = 0;
    a2_rank * rank = malloc(rank_cap * sizeof(a2_rank));
    
    Point current_exit = get_exit_point(current->chains[depth], current->configs[depth]);
    
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
    }
*/







/**
 * Algorithm_2 - Branch-and-bound with pruning.
 *
 * Recursively explores the solution space, pruning branches that exceed
 * the best known solution cost. Maintains intermediate solutions for each
 * recursion depth.
 *
 * @param sol: Input/output solution (will be overwritten with result)
 * @param depth: Recursion depth (tracks current position in chain ordering)
 */

static Solution *bb_solutions = NULL;
static float bb_best_cost = 1e9f;
static Solution bb_best_solution;

/**
 * bb_recurse - Recursive branch-and-bound helper.
 */
static void bb_recurse(int *visited, Solution *current, float current_cost, int depth) {

    if (depth < 10) fprintf(stderr, "At depth: %d\n", depth);
    /* Base case: all chains visited */
    if (depth == ch_count) {
        if (current_cost < bb_best_cost) {
            bb_best_cost = current_cost;

            free(bb_best_solution.chains);
            free(bb_best_solution.configs);
            bb_best_solution.chains = (Chain *)malloc(ch_count * sizeof(Chain));
            bb_best_solution.configs = (int *)malloc(ch_count * sizeof(int));

            for (int i = 0; i < ch_count; i++) {
                bb_best_solution.chains[i] = current->chains[i];
                bb_best_solution.configs[i] = current->configs[i];
            }
            bb_best_solution.count = ch_count;

            fprintf(stderr, "[opt2]   new best: cost=%.2f\n", bb_best_cost);
        }
        return;
    }

    /* Pruning: if current cost is already too high, stop exploring this branch */
    if (current_cost >= bb_best_cost * 
            ((float)depth / (float)ch_count)) {
//            (1-pow(((float)depth / (float)ch_count)-1, 2))) {
        return;
    }

    /* Try promising unvisited chains */
    Point current_exit = get_exit_point(&current->chains[depth - 1],
                                        current->configs[depth - 1]);
    
    float cheapest = 1e8;

    for (int next_ch = 0; next_ch < ch_count; next_ch++) {
        if (visited[next_ch]) continue;

        //int next_configs = chains[next_ch].closed ? chains[next_ch].count : 2;
        int next_configs = 2;

        for (int next_cfg = 0; next_cfg < next_configs; next_cfg++) {
            Point next_entry = get_entry_point(&chains[next_ch], next_cfg);
            float edge_cost = get_distance(current_exit, next_entry);
            if (edge_cost < cheapest) cheapest = edge_cost;
        }
    }

    int tries = 0;
    for (int next_ch = 0; next_ch < ch_count; next_ch++) {
        if (visited[next_ch]) continue;

        //int next_configs = chains[next_ch].closed ? chains[next_ch].count : 2;
        int next_configs = 2;

        for (int next_cfg = 0; next_cfg < next_configs; next_cfg++) {
            Point next_entry = get_entry_point(&chains[next_ch], next_cfg);
            float edge_cost = get_distance(current_exit, next_entry);
            if (feq(edge_cost, cheapest)) {
                visited[next_ch] = 1;
                current->chains[depth] = chains[next_ch];
                current->configs[depth] = next_cfg;

                bb_recurse(visited, current, current_cost + edge_cost, depth + 1);
                tries++;
                visited[next_ch] = 0;
                if (edge_cost > 1e-1) goto escape;
            }

        }
    }
escape:
}

void Algorithm_2(Solution *sol, int depth) {
    fprintf(stderr, "[opt2] Algorithm_2: starting branch-and-bound\n");

    bb_best_cost = solution_cost(sol);
    bb_best_solution.chains = (Chain *)malloc(ch_count * sizeof(Chain));
    bb_best_solution.configs = (int *)malloc(ch_count * sizeof(int));
    bb_best_solution.count = 0;

    Solution current = solution_init();
    int *visited = (int *)malloc(ch_count * sizeof(int));

    for (int i = 0; i < ch_count; i++) {
        visited[i] = 0;
    }

    /* Try starting from each chain */
    for (int start_ch = 0; start_ch < ch_count; start_ch++) {
        int start_configs = chains[start_ch].closed ? chains[start_ch].count : 2;

        for (int start_cfg = 0; start_cfg < start_configs; start_cfg++) {
            visited[start_ch] = 1;
            current.chains[0] = chains[start_ch];
            current.configs[0] = start_cfg;

            bb_recurse(visited, &current, 0.0f, 1);

            visited[start_ch] = 0;
        }
    }

    free(visited);
    free(current.chains);
    free(current.configs);

    /* Copy best solution to output */
    free(sol->chains);
    free(sol->configs);
    sol->chains = bb_best_solution.chains;
    sol->configs = bb_best_solution.configs;
    sol->count = bb_best_solution.count;

    fprintf(stderr, "[opt2] Algorithm_2: best cost found = %.2f\n", bb_best_cost);
    print_solution_stats(sol);
}


int main(int argc, char *argv[]) {
    fprintf(stderr, "[main] Laser cutter G-Code optimizer starting\n");
    
    lines = parse_gcode_file(&line_count);
    if (!lines) {
        fprintf(stderr, "Failed to parse gcode file\n");
        return 1;
    }
    
    fprintf(stderr, "[main] Loaded %d lines\n", line_count);
    
    /* Find chains */
    chains = (Chain *)malloc(line_count * sizeof(Chain));
    find_chains(lines, line_count, chains, &ch_count);
    fprintf(stderr, "[main] Found %d chains\n", ch_count);
    
    /* Precompute distances */
    precompute_distances(chains, ch_count, &dist_cache);
    fprintf(stderr, "[main] Distance cache ready\n");
    
    /* Run Algorithm 1 */
    Solution sol = solution_init();
    Algorithm_1(&sol, 0);
    Algorithm_2(&sol, 0);
    
    /* Apply solution and output */
    float td = calculate_travel_distance(lines, line_count);
    
    apply_solution(&sol);
    fprintf(stderr, "[main] Solution applied to lines\n");

    fprintf(stderr, "Travel distance before optimizations: %g\n", td);
    td = calculate_travel_distance(lines, line_count);
    fprintf(stderr, "Travel distance after optimizations: %g\n", td);
    
    export_gcode(lines, line_count);
    
    /* Cleanup */
    free(lines);
    free(chains);
    free(dist_cache);
    free(sol.chains);
    free(sol.configs);
    
    fprintf(stderr, "[main] Done!\n");
    return 0;
}

