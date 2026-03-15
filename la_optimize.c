#include "gcode.h"


int main(void) {
    int count = 0;
    Line *lines = parse_gcode_file(&count);
    
    if (!lines) {
        fprintf(stderr, "Failed to parse gcode file\n");
        return 1;
    }
    
    int chain_count = 0;
    Chain *chains = identify_chains(lines, count, &chain_count);
    
    if (!chains) {
        fprintf(stderr, "Failed to identify chains\n");
        free(lines);
        return 1;
    }
    
    float old_travel = calculate_travel_distance(lines, count);
    float cut_distance = calculate_cut_distance(lines, count);
    
    fprintf(stderr, "Original travel distance: %.2f\n", old_travel);
    fprintf(stderr, "Total cut distance: %.2f\n", cut_distance);
    fprintf(stderr, "Chains found: %d\n", chain_count);
    fprintf(stderr, "Optimizing...\n");
    
    // Prepare arrays for optimization
    int *metachain = malloc(chain_count * sizeof(int));
    int *used = malloc(2 * chain_count * sizeof(int));
    int *best_metachain = malloc(chain_count * sizeof(int));
    int *best_reversed = malloc(chain_count * sizeof(int));
    
    memset(used, 0, 2 * chain_count * sizeof(int));
    float best_travel = calculate_travel_distance(lines, count);
    
    optimize_chains_recursive(lines, chains, chain_count, metachain, 0, used,
                             0.0, &best_travel, best_metachain, best_reversed);
    
    fprintf(stderr, "Optimized travel distance: %.2f\n", best_travel);
    fprintf(stderr, "Improvement: %.2f%% (%.2f units saved)\n",
            ((old_travel - best_travel) / old_travel) * 100.0,
            old_travel - best_travel);
    
    // Reorder lines according to best metachain
    Line *optimized = malloc(count * sizeof(Line));
    int pos = 0;
    
    for (int i = 0; i < chain_count; i++) {
        int chain_idx = best_metachain[i];
        int reversed = best_reversed[i];
        
        if (reversed) {
            // Copy chain in reverse
            for (int j = chains[chain_idx].end; j >= chains[chain_idx].start; j--) {
                Line tmp = lines[j];
                float temp_x = tmp.x0;
                float temp_y = tmp.y0;
                tmp.x0 = tmp.x1;
                tmp.y0 = tmp.y1;
                tmp.x1 = temp_x;
                tmp.y1 = temp_y;
                optimized[pos++] = tmp;
            }
        } else {
            // Copy chain normally
            for (int j = chains[chain_idx].start; j <= chains[chain_idx].end; j++) {
                optimized[pos++] = lines[j];
            }
        }
    }
    
    export_gcode(optimized, count);
    
    free(lines);
    free(chains);
    free(metachain);
    free(used);
    free(best_metachain);
    free(best_reversed);
    free(optimized);
    
    return 0;
}

