#include <stdlib.h>
#include <memory.h>

#include "stats.h"
#include "schema.h"
#include "terminal.h"

#include "dump1.h"

//***************************************************************************
//
//    SCHEMA
//
//***************************************************************************

/***
   
source/csv: lat, lon, operadora, _, _, dt
 

level 1
 * quadtree lat lon 25

level 2
 * map operadora 
    vivo 1 Vivo 
    Tim 
    Claro Nextel 
 
*/

#define MAX_LEVELS  20
#define MAX_VALUES 100


static NodeFunctions nfs[MAX_VALUES];
static int n_values_of_dim[MAX_LEVELS];

/**
 * 
 * @return 
 */
PSchema schema_begin(void) {
    
    PSchema ps = calloc(1,sizeof(Schema));
    return ps;
}


/**
 * 
 * @return 
 */
void schema_new_level(PSchema ps) {
    if (ps->n_dims >= MAX_LEVELS) {
        fatal("max_levels");
    }
    n_values_of_dim[ps->n_dims] = 0;
    ps->n_dims ++;
}


/**
 * 
 * @return 
 */
void schema_new_value(PSchema ps, int n, PNodeFunctions pnfs) {
    while (n>0) {
        memcpy(nfs +ps->n_values, pnfs, sizeof(NodeFunctions));
        n_values_of_dim[ps->n_dims-1] ++;
        ps->n_values ++;
        n--;
    }
}

#include <stdio.h>

/**
 * 
 *  
 */
void schema_end(PSchema ps, PTerminalNodeFunctions pcfs) {

    // set mandatory terminal's node functions  
    PNodeFunctions pf = nfs + ps->n_values;

    pf->create_node = pcfs->create_node;
    pf->delete_node = pcfs->delete_node;
    
    pf->shallow_copy = pcfs->shallow_copy;
    
    pf->get_content_info = pcfs->get_content_info;
    pf->set_content_info = pcfs->set_content_info;

    if (!pf->create_node || !pf->delete_node 
        || !pf->shallow_copy 
        || !pf->get_content_info || !pf->set_content_info) {
        fatal("Missing mandadoty terminal node's function");
    }
    
    // allocates and initialize Node Functions Array
    ps->nfs = calloc((ps->n_values + 1), sizeof(NodeFunctions));

    // copies everything including the teminal functions
    memcpy(ps->nfs,nfs,(ps->n_values+1) * sizeof(NodeFunctions) );
    
    ps->n_values_of_dim = calloc(ps->n_dims+1,sizeof(int));
    memcpy(ps->n_values_of_dim,n_values_of_dim,sizeof(int) * (ps->n_dims+1));
        
    ps->level_of_value = calloc(ps->n_values + 1, sizeof (int));

    ps->top_of_dim = calloc(ps->n_values + 1, sizeof (int));
    ps->dim_of_value = calloc(ps->n_values + 1, sizeof (int));

    ps->content_of_value = calloc(ps->n_values + 1, sizeof (int));
    
    // computes top_of_dim and dim_of_value 
    for (int idx_dim = 0, idx_value = 0; idx_dim < ps->n_dims; idx_dim++) {
        ps->top_of_dim[idx_dim] = idx_value;
        for (int level = 0; level < ps->n_values_of_dim[idx_dim]; level++) {
            ps->dim_of_value[idx_value] = idx_dim;
            ps->level_of_value[idx_value] = level;
            idx_value++;
        }
    }
    
    // set terminal level
    ps->n_values_of_dim[ps->n_dims] = 1;
    ps->dim_of_value[ps->n_values] = ps->n_dims;
    ps->top_of_dim[ps->n_dims] = ps->n_values;
      
    // allocate all trees
    ps->trees = calloc(ps->n_dims,sizeof(Tree));
    
    // allocates nodes for each derived tree
    for (int i=0; i < ps->n_dims; i++) {
        // creates n_values + 1 (terminal) nodes
        ps->trees[i].nodes = (PNode *) calloc(ps->n_values+1, sizeof(PNode));
    }

    ps->content_nfs = calloc(ps->n_values+1,sizeof(NodeFunctions));
    
    // computes content accelerators
    for (int i = 0; i < ps->n_values; i++) {
        ps->content_of_value[i] = ps->top_of_dim[ps->dim_of_value[i]+1];
        ps->content_nfs[i] = ps->nfs[ps->content_of_value[i]];
    }
}

/**
 * 
 *  
 */
void schema_release_all(PSchema ps) {
    int i;

    // allocates nodes for each derived tree
    for (i=0; i < ps->n_dims; i++) {
        free(ps->trees[i].nodes);
    }
    free(ps->trees);
    free(ps->content_nfs);
    free(ps->content_of_value);
    free(ps->top_of_dim);
    free(ps->level_of_value);
    free(ps->dim_of_value);
    free(ps->n_values_of_dim);
    free(ps->nfs);
    free(ps);
}

/**
 * 
 *  
 */
Values schema_create_address(PSchema ps) {
    Values values = calloc(sizeof (int), ps->n_values);
    values[0] = 10;
    return values;
}

/**
 * 
 *  
 */
void schema_release_address(Values values) {
    free(values);
}



/**
 * 
 *  
 */
void init_tree(PTree pt) {
    memset(pt,0,sizeof(Tree));
}

/**
 * 
 *  
 */
PTree schema_get_tree(PSchema ps, int level) {
    PTree pt;
    
    if (level == 0) return NULL;
    if (level > ps->n_dims) {
        fatal("");
    }
    pt = & ps->trees[level-1];
    return pt;
}



#include <stdio.h>

void dump_children(PSchema ps, int idx, PNode pn, int _shared) {
#ifndef __LOGIT
#else 

    int v; 
    int shared;
    Branch branch;
    PNode pchild;
    PNodeFunctions pf = ps->nfs+idx;
    PNode p = pn;
    char spaces[80];

    int i = 0;
    while (i < idx*2) {
        spaces[i] = ' '; spaces[i+1] = ' ';
        i += 2;
    }
    spaces[i] = '\0'; 
    logit("  %2d: %s%d %s",idx, spaces, map_ptr(pn),_shared?"S":"P");

    if (idx < ps->n_values) {
        while (1) {
            pchild = pf->get_child(p, &v, &shared, &branch, FALSE, pf+1);
            if (!pchild) break;
            dump_children(ps, idx+1, pchild, shared);
            p=0;
        }
    }
#endif
}



static int path[MAX_VALUES+1];

void dump_path(int n) {
    int i;
    char buf[100], num[10];
    buf[0]='\0';
    for (i=0; i<n; i++) {
        sprintf(num,"%c",buf[i]+65);
        strcat(buf,num);
    }
    logit("PATH: %s",buf);
}

/**
 * 
 *  
 */
int schema_compute_sum(PSchema ps, int idx, PNode pn, int *err) {
    if (!pn) {
        LOGIT_1("SUM of NULL [%d]",idx);
        return 0;
    }
    
    // LOGIT_2("SUM of %d %03d:",idx, map_ptr(pn));
    if (idx == ps->n_values) {
        int sum = terminal_get_count(ps, pn);
        return sum;
    } else {
        int v; 
        int shared, shared_content;
        Branch branch;
        PNode pchild;
        PNodeFunctions pf = ps->nfs+idx;
        int sum1 = 0;
        int sum2;
        PNode p = pn;
        PNode pcontent = pf->get_content_info(pn, &shared_content);

        while (1) {
            pchild = pf->get_child(p,&v,&shared,&branch, FALSE, pf+1);
            if (!pchild) break;
#if 0
        int child_shared_content;
        PNode child_pcontent;

            if (shared_content) {
                if  (ps->dim_of_value[idx] == ps->dim_of_value[idx+1]) {
                    child_pcontent = (ps->nfs+(idx+1))->get_content_info(pchild, &child_shared_content);
                    if (pcontent != child_pcontent) {
                        fatal("Inconsistency [%d]: node (%p) shared content (%p) <> child content(%p)",idx, pn, pcontent, child_pcontent);
                    }
                } else {
                    if (pcontent != pchild) {
                        LOGIT_4("Inconsistency [%d]: node (%d) shared content (%d) <> child(%d)",
                            idx, map_ptr(pn), map_ptr(pcontent), map_ptr(pchild));
                        fatal("Inconsistency [%d]: node (%p) shared content (%p) <> child(%p)",idx, pn, pcontent, pchild);
                    }

                }
            }
#endif
            path[idx+1-1] = v;     
            sum1 += schema_compute_sum(ps,idx+1,pchild,err);
            p = 0;
        }
        int idx2 = ps->top_of_dim[ps->dim_of_value[idx]+1];
        sum2 = schema_compute_sum(ps,idx2,pcontent,err);
        if (sum1 != sum2) {
#ifdef __LOGIT            
            logit("Error Sum Children of %d [%d] = %d Content at %d[%d] = %d", 
                map_ptr(pn), idx, sum1,  map_ptr(pcontent), idx2, sum2);
#endif
            printf("ERROR SUM!");
            
            dump(ps,pn,"dump.dot","error",idx,1);

            dump_path(idx);
            dump_children(ps,idx,pn,0);
            *err = 1;
        } else if (*err) {
            // LOGIT_3("SUM of %03d [%d] = %d",map_ptr(pn), idx, sum1);
        }
        idx = idx2;
        return sum1;
    }
}

/**
 * 
 *  
 */
PNode schema_create_root(PSchema ps) {
    void * root = ps->nfs[0].create_node(FALSE);
    ps->root = root;
    return root;
}

/**
 * 
 *  
 */
void schema_release_root(PNode root) {
    free(root);
    #ifdef __STATS__
        stats.n_nodes --;
    #endif
}


