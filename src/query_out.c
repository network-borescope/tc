#include <stdlib.h>
#include <stdio.h>

#include "metadata.h"
#include "schema.h"
#include "query_processor.h"

static printf_t printf_f;

//-----------------------
//
//
//
//
static void out_no_group_by(PQuery pq) {
//        printf_f("N_CHANNELS: %d\n", pq->n_channels);
    printf_f("\"id\":%d, \"tp\":1,\"result\":[", pq->id);
    char *sep = "";

    PTerminalChannel *slots = pq->ps->pti->p_slots;
    int *r_offs = pq->result_offset;
    for (int i=0; i < pq->n_channels; i++, r_offs ++) {
        PTerminalChannel ptcx = slots[pq->query_slots[i]];
        if (ptcx->result_is_int) {
            int v = * (int *) (pq->result_memory  + *r_offs);
            printf_f("%s%d",sep,v);
        } else {
            double v = * (double *) (pq->result_memory  + *r_offs);
            printf_f("%s%.f",sep,v);
        }

//            printf_f("%s%f",sep,* (double *) (pq->result_memory + *r_offs));
        sep = ",";
    }
    printf_f("]");
}

//-----------------------
//
//
//
//
static void out_group_by_ks_vs(PQuery pq) {
    char idxs_buf[2048], vals_buf[2048], s[1024];

    // char *sepk = "";
    int offset = 0;
    printf_f("\"id\":%d, \"tp\":2,\"result\":[", pq->id);
    char *sep = "";
    char *sep2;

    int  last_idx = pq->n_group_by_indexes - 1;
    
    int cs[MAX_GROUP_BY_INDEXES];
    memset(cs, 0, sizeof(int) * MAX_GROUP_BY_INDEXES);

    int total_items = pq->group_by_total_amount;// * pq->group_by_distinct_count[0];
    // printf("Total de Items: %d\n", total_items);

    // scan all items of grid
    for (int m = 0; m < total_items; m ++ ) {

        idxs_buf[0] = '\0';
        sep2 = "";

        for (int k = 0; k < pq->n_group_by_indexes; k++) {
            // 
            sprintf(s,"%s%d",sep2,cs[k] * pq->group_by_bin_size[k] + pq->group_by_distinct_base[k]);
            sep2 = ",";
            strcat(idxs_buf,s);
        }
        
        //---------------------
        // Imprime os results
        //---------------------
        vals_buf[0] = '\0';
        int has_value = 0;
        sep2 = "";

        PTerminalChannel *slots = pq->ps->pti->p_slots;
        int *r_offs = pq->result_offset;
        for (int i=0; i<pq->n_channels; i++,  r_offs ++) {
            
            PTerminalChannel ptcx = slots[pq->query_slots[i]];
            if (ptcx->result_is_int) {
                int v = * (int *) (pq->result_memory + offset + *r_offs);
                //printf("Is Int\n");
                if (v != 0) has_value = 1;
                sprintf(s,"%s%d",sep2,v);

            } else {
                double v = * (double *) (pq->result_memory + offset + *r_offs);
                //printf("Is Double\n");
                if (v != 0.0) has_value = 1;
                sprintf(s,"%s%.f",sep2,v);
            }
            sep2 = ",";
            strcat(vals_buf,s);
        }
        //printf("Index Vals: %s %s\n", idxs_buf, vals_buf);

        
        if (has_value) {
            printf_f("%s{\"k\":[%s],\"v\":[%s]}",sep,idxs_buf,vals_buf);
            sep = ",";
        }

        // increments the counters 
        int j = last_idx;
        while (j >= 0) {
            cs[j] ++;
            if (cs[j] < pq->group_by_distinct_count[j]) break;
            cs[j] = 0;
            j --;
        }

        offset += pq->channels_result_size; 
    }

    printf_f("]");
}


//-----------------------
//
//
//
//
static void out_group_by_vs_ks(PQuery pq) {
    char idxs_buf[2048], vals_buf[2048], s[1024];

    // char *sepk = "";
    int offset = 0;
    printf_f("\"id\":%d, \"tp\":6,\"result\":[", pq->id);
    char *sep = "";
    char *sep2;

    int  last_idx = pq->n_group_by_indexes - 1;
    
    int cs[MAX_GROUP_BY_INDEXES];
    memset(cs, 0, sizeof(int) * MAX_GROUP_BY_INDEXES);

    int total_items = pq->group_by_total_amount;// * pq->group_by_distinct_count[0];
    // printf("Total de Items: %d\n", total_items);

    // scan all items of grid
    for (int m = 0; m < total_items; m ++ ) {

        idxs_buf[0] = '\0';
        sep2 = "";

        for (int k = 0; k < pq->n_group_by_indexes; k++) {
            // * pq->group_by_bin_size[k]
            int bin = pq->group_by_bin_size[k];
//            int v = (cs[k]  + pq->group_by_distinct_base[k] / bin) * bin;
            int v = cs[k];
            if (pq->group_by_n_points[k] && (pq->group_by_n_points[k] < pq->group_by_distinct_count[k])) {
                v = v * pq->group_by_distinct_count[k] / pq->group_by_n_points[k];
            } 
            v = v * bin + pq->group_by_distinct_base[k];
            // printf("Bounds (out_group_vs_ks): base: %d binsize %d => %d\n",pq->group_by_distinct_base[k], pq->group_by_bin_size[k], v);
            sprintf(s,"%s%d",sep2,v);
            sep2 = ",";
            strcat(idxs_buf,s);
        }
        
        //---------------------
        // Imprime os results
        //---------------------
        vals_buf[0] = '\0';
        int has_value = 0;
        sep2 = "";

        PTerminalChannel *slots = pq->ps->pti->p_slots;
        int *r_offs = pq->result_offset;
        for (int i=0; i<pq->n_channels; i++,  r_offs ++) {
            
            PTerminalChannel ptcx = slots[pq->query_slots[i]];
            if (ptcx->result_is_int) {
                int v = * (int *) (pq->result_memory + offset + *r_offs);
                //printf("Is Int\n");
                if (v != 0) has_value = 1;
                sprintf(s,"%s%d",sep2,v);

            } else {
                double v = * (double *) (pq->result_memory + offset + *r_offs);
                //printf("Is Double\n");
                if (v != 0.0) has_value = 1;
                sprintf(s,"%s%.f",sep2,v);
            }
            sep2 = ",";
            strcat(vals_buf,s);
        }
        //printf("Index Vals: %s %s\n", idxs_buf, vals_buf);

        
        if (has_value) {
            printf_f("%s[%s,%s]",sep,vals_buf,idxs_buf);
            sep = ",";
        }

        // increments the counters 
        int j = last_idx;
        while (j >= 0) {
            cs[j] ++;
            if (cs[j] < pq->group_by_distinct_count[j]) break;
            cs[j] = 0;
            j --;
        }

        offset += pq->channels_result_size; 
    }

    printf_f("]");
}

//-----------------------
//
//
//
//
static void out_group_by_lists(PQuery pq) {
    char *sep;
    char *sep2;

    if (pq->group_by_output == GROUP_BY_OUTPUT_K_LISTS) {
        printf_f("\"id\":%d, \"tp\":3,\"result\":{",pq->id);
        sep2 = "";
        printf_f("\"ks\": [");
        for (int gbi = 0; gbi < pq->n_group_by_indexes; gbi++) {
            printf_f("%s {\"base\": %d, \"multi\": %d, \"length\": %d} ",
                sep2,
                pq->group_by_distinct_base[gbi],
                pq->group_by_bin_size[gbi],
                pq->group_by_distinct_count[gbi]);
            sep2 = ",";
        }
        printf_f("],");

    } else {
        if (pq->group_by_output == GROUP_BY_OUTPUT_V_LISTS) {
            printf_f("\"id\":%d, \"tp\":5,\"result\":{", pq->id);

        } else if (pq->group_by_output == GROUP_BY_OUTPUT_FULL_LISTS) { 
            printf_f("\"id\":%d, \"tp\":4,\"result\":{", pq->id);
                sep = "";
                printf_f("\"ks\": [");
                for (int gbi = 0; gbi < pq->n_group_by_indexes; gbi++) {
                    printf_f("%s[",sep);
                    int n, ki;
                    sep2 = "";
                    int inc = pq->group_by_bin_size[gbi];
                printf("Bounds (out_group_by_list): binsize %d\n",pq->group_by_bin_size[gbi]);

                    for (ki = pq->group_by_distinct_base[gbi], n=pq->group_by_distinct_count[gbi]; n; n--, ki += inc ) {
                        printf_f("%s%d",sep2,ki);
                        sep2 = ",";
                    }
                    printf_f("]");
                    sep = ",";
                }
                printf_f("],");
        } else {
            query_fatal("Invalid group-by-output type: %d", pq->group_by_output);
        }
    }

    sep = "";
    printf_f("\"vs\": [");

    int offset = 0;
    PTerminalChannel *slots = pq->ps->pti->p_slots;
    int *r_offs = pq->result_offset;
    for (int i=0; i<pq->n_channels; i++, r_offs ++) {

        PTerminalChannel ptcx = slots[pq->query_slots[i]];

        sep2 = "";
        printf_f("%s[",sep);
        offset = *r_offs;

        // scan all items of grid
        for (int m = 0; m < pq->group_by_total_amount; m ++ ) {
            
            //---------------------
            // Imprime os results
            //---------------------
            if (ptcx->result_is_int) {
                int v = * (int *) (pq->result_memory + offset );
                printf_f("%s%d",sep2,v);
            } else {
                double v = * (double *) (pq->result_memory + offset);
                printf_f("%s%.f",sep2,v);
            }
            sep2 = ",";
            offset += pq->channels_result_size; 

        }
        printf_f("]");
        sep = ",";
    }
    printf_f("]}");
}


//-----------------------
//
//
//
//
static void out_bounds(PQuery pq) {
    char *sep;
    char *sep2;

    printf_f("\"id\":%d, \"tp\":200,\"result\":{", pq->id);

    sep = "";
    printf_f("\"vs\": [");

    int offset = 0;
    for (int m = 0; m < 2; m ++ ) {
        sep2 = "";
        printf_f("%s[",sep);

        // scan all items of grid
        for (int i=0; i<pq->n_channels; i++) {
            PByte p = pq->result_memory + offset;
            if (1) {
                // * pq->group_by_bin_size[i]
                int v = * (int *) p + pq->group_by_distinct_base[i];                
                printf_f("%s%d",sep2,v);
            } else {
                double v = * (double *) p;
                printf_f("%s%.f",sep2,v);
            }
            sep2 = ",";
            offset += sizeof(int); 
        }
        printf_f("]");
        sep = ",";
    }
    printf_f("]}");
}


//-----------------------
//
//
//
//
void query_out_as_json(PQuery pq, printf_t _printf_f) {
    printf_f = _printf_f;

//    printf_f("\n");
    if (pq->schema_query) {
        printf_f("\"id\":%d, \"tp\":100, \"result\": %s",pq->id, pq->json_src);

    } else if (pq->bounds_type) {
        out_bounds(pq);

    } else if (pq->n_group_bys == 0) {
        out_no_group_by(pq);
    } else if (pq->group_by_output == GROUP_BY_OUTPUT_KS_VS) {
        out_group_by_ks_vs(pq);
    } else if (pq->group_by_output == GROUP_BY_OUTPUT_VS_KS) {
        out_group_by_vs_ks(pq);
    } else {
        out_group_by_lists(pq);
    }
//    printf_f("\n");
}
