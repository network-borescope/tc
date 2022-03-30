#include <stdlib.h>
#include <stdio.h>

#include "metadata.h"
#include "schema.h"
#include "query_processor.h"

#define MAX_TOTAL_AMOUNT  (30*(1000*1000))

jmp_buf query_jmp;
char query_error_string[MAX_QUERY_ERROR_SZ];

//-----------------------
//
//
//
//
void query_fatal(char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    if (fmt) {
        vsprintf(query_error_string,fmt,args);
    }
    va_end(args);
    longjmp(query_jmp, 10);
}

//=============================================================================================
//
//  DO MERGE    
//
//=============================================================================================

/*--------------------------
*
*
*
*/
static int do_merge(PQuery pq, PByte data, int position) {
    int i;
    int offset = 0;

    //printf("MERGE TERMINAL DATA\n");
    // ajusta o offset para os group bys
    if (pq->n_group_bys > 0) {
        offset = position;
        //printf("n_group_bys %d\n",pq->n_group_bys);
        for (i = 0; i < pq->n_group_by_indexes; i++) {
            offset += pq->group_by_indexes[i] * pq->group_by_inner_amount[i];
            //printf("GROUP BY OFFSET IDX %d M %d\n",pq->group_by_indexes[i], pq->group_by_inner_amount[i]);
        }
        //printf("GROUP BY OFFSET: %d\n",offset);
    }
    //printf("OFFSET: %d   CHANNELS_MERGE_SIZE: %d\n",offset, pq->channels_merge_size);
    PByte pmerge = pq->merge_memory + (offset * pq->channels_merge_size);

    offset = 0;

    // ajusta o offset para cada merge
    PTerminalChannel *slots = pq->ps->pti->p_slots;

    int *offs = pq->merge_offsets;

    for (int i=0; i < pq->n_channels; i++,  offs ++) {
        
        PTerminalChannel ptc = slots[pq->query_slots[i]];
    //printf("MERGE ID_SLOT: %d DATA: %d  OFFSET: %d Position: %d\n", pq->query_slots[i], ptc->data_size, ptc->data_offset,position);

        ptc->pcfs->merge(pmerge + *offs, data + ptc->data_offset, &ptc->tfd);
    }

    return 0; 
}

/*--------------------------
*
*
*
*/
static int compute_offset(PQuery pq, int position) {

    //printf("MERGE TERMINAL DATA\n");
    // ajusta o offset para os group bys
    if (!pq->n_group_bys) return 0;

    int offset = position;
    int i;
    
    //printf("n_group_bys %d\n",pq->n_group_bys);
    for (i = 0; i < pq->n_group_by_indexes; i++) {
        offset += pq->group_by_indexes[i] * pq->group_by_inner_amount[i];
        //printf("GROUP BY OFFSET IDX %d M %d\n",pq->group_by_indexes[i], pq->group_by_inner_amount[i]);
    }
    //printf("GROUP BY OFFSET: %d\n",offset);

    //printf("OFFSET: %d   CHANNELS_MERGE_SIZE: %d\n",offset, pq->channels_merge_size);
    return offset;
}

/*--------------------------
*
*
*
*/
static int do_merge_vet(PQuery pq, int position,void ** vet_row, int vet_size) {
    int offset = compute_offset(pq, position);
    PByte pmerge = pq->merge_memory + (offset * pq->channels_merge_size);

    offset = 0;

    // ajusta o offset para cada merge
    PTerminalChannel *slots = pq->ps->pti->p_slots;

    int *offs = pq->merge_offsets;

    int i, r;
    for (int i=0; i < pq->n_channels; i++,  offs ++) {
        PTerminalChannel ptc = slots[pq->query_slots[i]];
        content_merge_t merge = ptc->pcfs->merge;
        int   content_offset = ptc->data_offset;
        void *prow;
        int   r;

        for (prow = vet_row, r = 0; r < vet_size; r ++, prow++) {
            merge(pmerge + *offs, ((PByte) prow) + content_offset, &ptc->tfd);
        }
    }

    return 0; 
}

//=============================================================================================
//
//  DO QUERY
//
//=============================================================================================


/*--------------------------
*
*
*
*/
static void merge_bounds(PQuery pq, int *indexes) {
//    printf("\n------------ NEW MERGE ----------\n");
    int *pmerge = (int *) pq->merge_memory;
    for (int i = 0; i< 2; i++) {
        for (int channel = 0; channel < pq->n_channels; channel ++) {
            int v = indexes[channel]; 
            if ( (!i && v < *pmerge) || (i && v > *pmerge)) *pmerge = v;
//            printf("[%d,%d] = %d  | Merge:  %d \n",i,channel,v,*pmerge);
            pmerge++;
        }
    }
}


/*--------------------------
*
*
*
*/
static void do_query(int idx_value, PNode pn, int dim_done, PQuery pq) {
    PSchema ps = pq->ps;
    PNodeFunctions pnfs = ps->nfs + idx_value;

    // must jump imediatelly to the next dimension?
    // usually required by a deep hierarchical operation 
    // like quadtree's  inrect
    if (dim_done) {
        // jump to next dimension if still in the same dimension
        if (ps->dim_of_value[idx_value-1] == ps->dim_of_value[idx_value]) {
            idx_value = ps->content_of_value[idx_value];
            pn = pnfs->get_content_info(pn, NULL);
        }
    } 

    PQCallInfo pci = pq->ppci[idx_value];
    //printf("V0\n");
	//{ char *s="M10"; printf("%s ++\n",s); fflush(stdout); 
		//void *p = calloc(1,88); free(p); 
		//printf("%s --\n",s); fflush(stdout); }


    // goes down until reach a terminal or find an operation  
    while ( (idx_value < ps->n_values) && !pci) {
        idx_value = ps->content_of_value[idx_value];
        pn = pnfs->get_content_info(pn, NULL);
        pci = pq->ppci[idx_value];
    }
    
    //printf("IDX_VALUE: %d n_values: %d\n", idx_value, ps->n_values);

    // reached terminal
    if (idx_value == ps->n_values) {
        PTerminal pt = (PTerminal) pn;

        // printf("USE: %d\n", pq->use_content_class);
        if (pq->bounds_type) {
            if (pq->bounds_type == QUERY_VALUE_BOUNDS) {
                merge_bounds(pq,pq->group_by_indexes);
            } else {
                int bounds[2];
                PTerminalChannel ptc = ps->pti->p_slots[pq->id_container];
                if (ptc->pcfs->find_bounds(pt->data + ptc->data_offset, bounds)) {
                    merge_bounds(pq,bounds+0);
                    merge_bounds(pq,bounds+1);
                }
            }

        } else if (!pq->use_content_class) {
            //printf("NOT USE: %d\n", pq->use_content_class);
            do_merge(pq, pt->data, 0);

        } else {
            //printf("HANDLING CONTENT CLASS: %d (%d)\n", idx_value, ps->n_values);
            pci = pq->ppci[idx_value];
            // printf("PCI = %p\n",pci);
            query_container_op_t cop = pci->op;
            query_container_og_t cog = pci->op_group_by;
            //printf("-\n");

            PTerminalChannel ptc = ps->pti->p_slots[pq->id_container];
            void *state = NULL;
            // printf("id: %d Offset : %d\n", pq->id_container, ptc->data_offset);
    
            //printf("??\n");
            // Does not need the data inside content_class
            // Just checks if the container clause is valie
            if (!(pq->use_content_class & CONTENT_CLASS_SELECT)) {
                //printf("A\n");
                if (cop(&pci->qargs, pt->data + ptc->data_offset, &state)) {
                    do_merge(pq, pt->data, 0);
                }

            // it is not a group by
            // Acumulates (merges) data 
            } else if (!(pq->use_content_class & CONTENT_CLASS_GROUPBY)) {
                //printf("B\n");
                state = NULL;
                while (1) {
#if 0
                    pci->qargs.n_rows = -1;
                    PByte pdata = cop(&pci->qargs, pt->data + ptc->data_offset, &state);
                    if (!pdata) break;
                    if (pci->qargs.n_rows > 0) {
                        //printf("N ROWS: %d\n",pci->qargs.n_rows);
                        do_merge_vet(pq, 0, pci->qargs.prow_vet, pci->qargs.n_rows);                
                    } else {
                        do_merge(pq, pdata, 0);
                    }
#else
#if 1
                    PByte pdata = cop(&pci->qargs, pt->data + ptc->data_offset, &state);
                    if (!pdata) break;
                    do_merge(pq, pdata, 0);
#else          
                    int position = 0;
                    PByte pdata = cog(&pci->qargs, pt->data + ptc->data_offset, &position, &state);
                    if (!pdata) break;
                    do_merge(pq, pdata, position);
#endif          
#endif
            }

            // it's a group by and needs the data inside content_class
            } else {
                //printf("C\n");
                state = NULL;
                // varre todos os elementos em pt->data, descartanto os elementos fora de interesse
                while (1) {
                    int position;
             //printf("C %p %p %p %d\n",cog,&pci->qargs, pt->data, ptc->data_offset);
                    PByte pdata = cog(&pci->qargs, pt->data + ptc->data_offset, &position, &state);
                    if (!pdata) break;
                    //printf("Position: %d\n", position, pq->);
                    do_merge(pq, pdata, position);
                }
            }
        }
        //printf("QUERY DONE  RETURN\n");
        return;
    } 

    //-----------------------------
    //  Non terminal nodes
    //-----------------------------
    PNode p = pn;
    PNode pchild;
    int v;
    Branch b;
    int level = ps->level_of_value[idx_value];
     
    // there is an op to call
    if (!pci->call_group_by) {
        query_op_t op = pci->op;

//        printf("OP IDX %d\n",0);
        while ((pchild = pnfs->get_child(p, &v, NULL, &b, 0, NULL)) != NULL) {
            p = NULL;
            //printf("OP V %d\n",v);
            int res;
            int argset = 0; // 1;
            do {
                res  = op(v, argset, &pci->qargs, level, pci->op_data);
                if (res & QUERY_BIT_TRUE) {
                    do_query(idx_value + 1, pchild, res & QUERY_DIM_DONE, pq);
                }
                argset ++;
            } while (res & QUERY_BIT_MORE);
        }

    } else {
        query_og_t op = pci->op_group_by;
        int *indexes = pq->group_by_indexes + pci->group_by_idx;
        // printf("GROUP BY IDX %d\n",pci->group_by_idx);
        while ((pchild = pnfs->get_child(p, &v, NULL, &b, 0, NULL)) != NULL) {
            p = NULL;
            int res;
            int argset = 0; // 1;
            do {
                res = op(v, argset, &pci->qargs, indexes, level, pci->op_data);
                if (res & QUERY_BIT_TRUE) {
                    do_query(idx_value + 1, pchild, res & QUERY_DIM_DONE, pq);
                }
                argset ++;
            } while (res & QUERY_BIT_MORE);
        } 
    }
    // printf("FINAL RETURN\n");
}

//=============================================================================================
//
//    
//
//=============================================================================================

//-----------------------
//
//
//
//
static void compute_result(PQuery pq) {
    
    //printf("222-1\n");
    if (pq->total_result_size == 0 || pq->bounds_type) {
        //printf("NO TOTAL RESULT SIZE\n");
        pq->result_memory = pq->merge_memory;
        pq->merge_memory = NULL;
        return;
    } 
    //printf("222-2 %d\n", pq->total_result_size);

    // alloc result buffer
    pq->result_memory =  calloc(1,pq->total_result_size);    
    //printf("222-3\n");fflush(stdout);
    //printf("Result Memory: %d\n",pq->total_result_size); fflush(stdout);
    //printf("222-4\n");fflush(stdout);
    
    // compute the total number of items in result buffer
    int total_items;
    if (pq->n_group_bys) { 
        // total_items = pq->group_by_inner_amount[0] * pq->group_by_distinct_count[0];
        total_items = pq->group_by_total_amount;
    } else {
        total_items = 1;
    }

    //printf("Compute Result: %d\n",total_items);

    // speed things up 
    PByte pmerge = pq->merge_memory;
    PByte presult = pq->result_memory;
    PTerminalChannel *slots = pq->ps->pti->p_slots; 

    //
    //  Merge with 2 channels:  M1C1, M1C2, M2C1, M2C2 ... MnC1, MnC2
    //      or 
    //  Merge with 2 channels:  M1C1, M2C1, ... MnC1, M2C1, M2C2 ... MnC2
    //

    // scan all items of the grid
    for (int k = 0; k < total_items; k ++ ) {
        // PTerminalChannel *slots = slots0;
        int *m_offs = pq->merge_offsets;
        int *r_offs = pq->result_offset;

        for (int i=0; i<pq->n_channels; i++, m_offs ++, r_offs ++) {

            PTerminalChannel ptc = slots[pq->query_slots[i]];
            // printf("QUERY SLOT: %d\n", pq->query_slots[i]);

            // printf("MERGE -> RESULT: %d %d\n", *m_offs, *r_offs);
            // check if it is necessary to compute the result from merge data or 
            // if a simple copy is sufficient
            if (!ptc->pcfs->result) {
                query_fatal("Query: No result function");
                // memcpy(presult + *r_offs, pmerge + *m_offs, ptc->result_size);
            } else {
                ptc->pcfs->result(pmerge + *m_offs, presult + *r_offs);
            }
        }

        pmerge += pq->channels_merge_size;
        presult += pq->channels_result_size;        
    }
}

void mlnet_compute(PQuery pq);

//-----------------------
//
//
//
//
PQuery query_begin(void) {
    return calloc(1, sizeof(Query));
}

//-----------------------
//
//
//
//
int query_process(PQuery pq, char *src, PSchema ps, PMetaData pmd) {

    if (!query_parse(pq, pmd, ps, src)) return 0;
    if (pq->schema_query) return 1;

    //-------------------------------
    // Calcula os multiplicadores
    //-------------------------------
    int mult = 1; 

    if (pq->bounds_type) {
        pq->group_by_total_amount = sizeof(int) * 2 * pq->n_channels;

        pq->total_merge_size = 4 * pq->channels_merge_size;
        pq->total_result_size = pq->total_merge_size;         

        pq->merge_memory = calloc(1,pq->total_merge_size);

        for (int i=0; i<pq->n_channels;i++) {
            ((int *) pq->merge_memory)[i] = (~ (unsigned int) 0)>>1; // MaxInt
        }
    } else {
        //printf("1212\n");
        if (pq->n_group_bys > 0) {
            int i;
            for (i = pq->n_group_by_indexes-1; i >= 0; i--) {
                pq->group_by_inner_amount[i] = mult;
                mult *= pq->group_by_distinct_count[i];
                // printf("Distinct Count: %d\n",pq->group_by_distinct_count[i]);
                //printf("QUERY: Mult %d %d\n",mult, i);
            }
        }
        pq->group_by_total_amount = mult;
        // printf("QUERY: group_by_total_amount %d\n",pq->group_by_total_amount);
        if (pq->group_by_total_amount > MAX_TOTAL_AMOUNT) {
            query_fatal("QUERY: Result size exceeds maximum limit (%d)\n",MAX_TOTAL_AMOUNT);
            return 0;
        }

        pq->total_merge_size = mult * pq->channels_merge_size;
        pq->total_result_size = mult * pq->channels_result_size;         

        pq->merge_memory = calloc(1,pq->total_merge_size);
        // printf("Total Merge size: %d, %d, %d\n", pq->total_merge_size, mult, pq->channels_merge_size);
    }

    //printf("333\n");
    do_query(0, ps->root, 0, pq);
    
	//printf("222\n");

    compute_result(pq);
    //printf("111\n");

    // ML.NET
    mlnet_compute(pq);
    //printf("000\n");

    return 1;
}

//-----------------------
//
//
//
//
void query_end(PQuery pq) {
    if (pq->merge_memory != pq->result_memory) {
        //printf("End: 111-a\n");
        free(pq->result_memory);
        //printf("End: 111-b\n");
        pq->result_memory = NULL;
    }
    if (pq->merge_memory) {
        //printf("End: 112-a\n");
        free(pq->merge_memory);
        //printf("End: 112-b\n");
        pq->merge_memory = NULL;
    }
    if (pq) {
        free(pq);
        pq = NULL;
    }
    //printf("End: 000\n");
}

