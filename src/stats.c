#include <memory.h>
#include <stdio.h>
#include "stats.h"


Stats stats;

/**
 * 
 *  
 */
void stats_reset(void) {
    memset(&stats, 0, sizeof(Stats));
}

void stats_line_to_file(FILE *fout, int tot_mem_kb, int tot_time_ns) {
    if (!fout) fout = stderr;
    fprintf(fout,"%d, %d, %d, %d, %d, %d, %d, %d,  %d, %d\n",
        tot_mem_kb,
        tot_time_ns,

//    fprintf(fout,"Total Insertions : %d\n",stats.n_insertions);
        stats.n_insertions,
//    fprintf(fout,"Total Removals : %d\n",stats.n_removals);
        stats.n_removals,
//    fprintf(fout,"Total Nodes (all): %d\n",stats.n_nodes);
        stats.n_nodes,
//    fprintf(fout,"Total Terminals: %d\n",stats.n_terminals);
        stats.n_terminals,
//    fprintf(fout,"Total Non-Terminals: %d\n",stats.n_nodes - stats.n_terminals);
        stats.n_nodes - stats.n_terminals,
        stats.n_proper_child_edges,
//    fprintf(fout,"Shared Child Edges: %d\n",stats.n_shared_child_edges);
        stats.n_shared_child_edges,
//   fprintf(fout,"Shared Child Terminals: %d\n",stats.n_shared_child_terminals);
        stats.n_shared_child_terminals
        );
}


void stats_dump_to_file(FILE *fout) {
    if (!fout) fout = stderr;

    fprintf(fout,"_____________________________________\n");
    fprintf(fout,"Total Insertions : %d\n",stats.n_insertions);
    fprintf(fout,"Total Removals : %d\n",stats.n_removals);
    fprintf(fout,"______\n");
    fprintf(fout,"Total Nodes (all): %d\n",stats.n_nodes);
    fprintf(fout,"Total Terminals: %d\n",stats.n_terminals);
    fprintf(fout,"Total Non-Terminals: %d\n",stats.n_nodes - stats.n_terminals);
    fprintf(fout,"______\n");
    fprintf(fout,"Total Primary Terminals: %d\n",stats.n_primary_terminals);
    fprintf(fout,"Total Non-Primary Terminals: %d\n",stats.n_terminals - stats.n_primary_terminals);
    fprintf(fout,"______\n");
    fprintf(fout,"Betas: %d\n",stats.n_betas);
    fprintf(fout,"Betas - Terminal: %d\n",stats.n_beta_terminals);
    fprintf(fout,"Betas - Non-Terminal : %d\n",stats.n_betas - stats.n_beta_terminals);
    fprintf(fout,"______\n");
    fprintf(fout,"Proper Child Terminals: %d\n",stats.n_proper_child_terminals);
    fprintf(fout,"Shared Child Terminals: %d\n",stats.n_shared_child_terminals);
    fprintf(fout,"Proper Child Primary Terminals: %d\n",stats.n_primary_terminals);
    fprintf(fout,"Proper Child Non-Primary Terminals: %d\n",
        stats.n_proper_child_terminals - stats.n_primary_terminals);
    fprintf(fout,"______\n");
    fprintf(fout,"Proper Child Edges: %d\n",stats.n_proper_child_edges);
    fprintf(fout,"Shared Child Edges: %d\n",stats.n_shared_child_edges);
    fprintf(fout,"______\n");
    fprintf(fout,"Proper Child Edges + Betas: %d\n",
        stats.n_proper_child_edges + stats.n_betas);
    fprintf(fout,"Proper Child Terminals + Beta Terminals: %d\n",
        stats.n_proper_child_terminals + stats.n_beta_terminals);
    fprintf(fout,"Migration Shared to Proper: %d\n",stats.n_shared_to_proper);
    fprintf(fout,"Terminal Shallow Copies: %d\n",stats.terminal_shallow_copies);
    fprintf(fout,"______\n");
    fprintf(fout,"Maximum Reference Count Value: %d\n",stats.max_ref_counter);
    fprintf(fout,"______\n");
    fprintf(fout,"Reinsertions: %d\n",stats.n_reinserts);
    fprintf(fout,"Inedits: %d\n",stats.n_inedits);
    fprintf(fout,"Deshares: %d\n",stats.n_deshares);
    fprintf(fout,"______\n");
    fprintf(fout,"Maximum Used Memory (Kb): %lld\n",stats.max_memory_used_kb);
    fprintf(fout,"Average Used Memory (Kb): %lld\n",stats.acc_memory_used_kb / stats.count_memory_used);
}

void stats_dump(void) {
    stats_dump_to_file(stdout);
}
