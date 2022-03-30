#ifndef __STATS
#define __STATS

#ifndef FILE
#include <stdio.h>
#endif

//***************************************************************************
//
//    Statistics
//
//***************************************************************************

#define __STATS__ 

typedef struct s_stats{
    int n_nodes;      // including betas (and terminals)

    int n_insertions;
    int n_distinct_insertions;
    int n_removals; 

    int n_betas;      // including terminals
    int n_beta_terminals;
    int n_terminals;
    int n_primary_terminals;

    int n_reinserts;
    int n_inedits;
    int n_deshares;

    int n_shared_child_terminals;
    int n_proper_child_terminals;

    int n_shared_child_edges;
    int n_proper_child_edges;

    int n_shared_to_proper;

    int max_ref_counter;

    int terminal_shallow_copies;

    long long max_memory_used_kb;
    long long acc_memory_used_kb;
    long long count_memory_used;


} Stats, *PStats;

extern Stats stats;

void stats_reset(void);
void stats_dump(void);
void stats_dump_to_file(FILE *fout);
void stats_line_to_file(FILE *fout, int tot_mem_kb, int tot_time_ns);


#endif