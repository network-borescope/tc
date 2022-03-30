#include <stdlib.h>
#include <stdio.h>

#include "metadata.h"
#include "schema.h"
#include "query_processor.h"

//-----------------------
//
//
//
//
static void before_detect(FILE *f, PQuery pq) {
    char vals_buf[2048], s[1024];

    // char *sepk = "";
    int offset = 0;

    int  last_idx = pq->n_group_by_indexes - 1;
    
    int cs[MAX_GROUP_BY_INDEXES];
    memset(cs, 0, sizeof(int) * MAX_GROUP_BY_INDEXES);

    int total_items = pq->group_by_total_amount;

    // scan all items of grid
    for (int m = 0; m < total_items; m ++, offset += pq->channels_result_size) {

        //---------------------
        // Imprime os results
        //---------------------
        vals_buf[0] = '\0';
        int non_zero = 1;

        PTerminalChannel *slots = pq->ps->pti->p_slots;
        int *r_offs = pq->result_offset;

        // columns loop
        for (int i=0; i<pq->n_channels; i++,  r_offs ++) {
            
            if (i != pq->ml_position) continue;

            PTerminalChannel ptcx = slots[pq->query_slots[i]];
            PByte pval = pq->result_memory + offset + *r_offs;

            if (ptcx->result_is_int) {
                int v = * (int *) pval;
                if (v != 0) non_zero = 1;
                sprintf(s,"%d",v);

            } else {
                double v = * (double *) pval;
                if (v != 0.0) non_zero = 1;
                sprintf(s,"%.f",v);
            }
            strcat(vals_buf,s);
        }
        
        if (non_zero) {
            fprintf(f,"%s\n",vals_buf);
        }

        // increments the counters 
        int j = last_idx;
        while (j >= 0) {
            cs[j] ++;
            if (cs[j] < pq->group_by_distinct_count[j]) break;
            cs[j] = 0;
            j --;
        } 
    }
}

//-----------------------
//
//
//
//
static void after_detect(FILE *f, PQuery pq) {
    char buf[2048];

    // char *sepk = "";
    int offset = 0;

    int  last_idx = pq->n_group_by_indexes - 1;
    
    int cs[MAX_GROUP_BY_INDEXES];
    memset(cs, 0, sizeof(int) * MAX_GROUP_BY_INDEXES);

    int total_items;
    if (!pq->n_group_bys) { 
        total_items = 1;
    } else {
//        total_items = pq->group_by_inner_amount[0] * pq->group_by_distinct_count[0];
        total_items = pq->group_by_total_amount;
    }
    
    #define MODE_MUST_READ  0
    #define MODE_SEARCHING  1
    #define MODE_EOF        2

    int modo = MODE_MUST_READ;
    int data_pos = -1;
    int pos = 0;

    PByte memory = pq->result_memory + offset;
    int increment = pq->channels_result_size;

    // scan all items of grid
    for (int m = 0; m < total_items; m ++, 
        offset += pq->channels_result_size, memory += increment ) {

        //---------------------
        // Zera results
        //---------------------
        PTerminalChannel *slots = pq->ps->pti->p_slots;
        int *r_offs = pq->result_offset;

        // columns loop
        for (int i=0; i<pq->n_channels; i++, r_offs ++) {
           
            if (i != pq->ml_position) continue;

            if (modo==MODE_MUST_READ) {
                if (!f || !fgets(buf,2048,f)) {
                    modo = MODE_EOF;
                } else {
                    data_pos = atoi(buf);
                    modo = MODE_SEARCHING;
                }
            }
           
            PTerminalChannel ptcx = slots[pq->query_slots[i]];

            if (data_pos == pos) {
                //printf("%d\n", pos);
                if (modo == MODE_SEARCHING) modo = MODE_MUST_READ;
            } else {
                // clear data on result
                PByte pval = pq->result_memory + offset + *r_offs;
                if (ptcx->result_is_int) {
                    * (int *) pval = 0;
                } else {
                    * (double *) pval = 0;
                }
            }
            pos ++;
        }

        // increments the counters 
        int j = last_idx;
        while (j >= 0) {
            cs[j] ++;
            if (cs[j] < pq->group_by_distinct_count[j]) break;
            cs[j] = 0;
            j --;
        } 
    }
}

//-----------------------
//
//
//
//
static int before_predict(FILE *f, PQuery pq) {
    char vals_buf[2048], s[1024];

    // char *sepk = "";
    int offset = 0;
    int total_items = pq->group_by_total_amount;

    int limit = (pq->ml_offset >= 0)? pq->ml_offset: total_items;
    int last_non_zero = -1;
    
    // scan all items of grid
    for (int m = 0; m < total_items; m ++, offset += pq->channels_result_size) {
        if (m >=limit) break;
        PTerminalChannel *slots = pq->ps->pti->p_slots;
        int *r_offs = pq->result_offset;
        for (int i=0; i<pq->n_channels; i++,  r_offs ++) {
            if (i != pq->ml_position) continue;
            PTerminalChannel ptcx = slots[pq->query_slots[i]];
            PByte pval = pq->result_memory + offset + *r_offs;
            if (ptcx->result_is_int) {
                int v = * (int *) pval;
                if (v) last_non_zero = m;
            } else {
                double v = * (double *) pval;
                if (v) last_non_zero = m;
            }
        }
    }

    if (pq->ml_len < 0) {
        pq->ml_len = limit - last_non_zero;
    }

    if (pq->ml_offset < 0) {
        limit = last_non_zero;
        pq->ml_offset =  limit;
    }


    offset = 0;
    // scan all items of grid
    for (int m = 0; m < total_items; m ++, offset += pq->channels_result_size) {

        if (m >=limit) break;

        //---------------------
        // Imprime os results
        //---------------------
        vals_buf[0] = '\0';

        PTerminalChannel *slots = pq->ps->pti->p_slots;
        int *r_offs = pq->result_offset;

        // columns loop
        for (int i=0; i<pq->n_channels; i++,  r_offs ++) {
            
            if (i != pq->ml_position) continue;

            PTerminalChannel ptcx = slots[pq->query_slots[i]];
            PByte pval = pq->result_memory + offset + *r_offs;

            if (ptcx->result_is_int) {
                int v = * (int *) pval;
                sprintf(s,"%d",v);

            } else {
                double v = * (double *) pval;
                sprintf(s,"%.f",v);
            }
            strcat(vals_buf,s);
        }
        fprintf(f,"%d,%s\n",m, vals_buf);
    }
    
    return last_non_zero + 1;
    //return limit;
}

//-----------------------
//
//
//
//
static void after_predict(FILE *f, PQuery pq) {
    char buf[2048];

    // char *sepk = "";
    int offset = 0;

#if 0
    int  last_idx = pq->n_group_by_indexes - 1;
    int cs[MAX_GROUP_BY_INDEXES];
    memset(cs, 0, sizeof(int) * MAX_GROUP_BY_INDEXES);
#endif
    
    int total_items = pq->group_by_total_amount;
    
    #define MODE_MUST_READ  0
    #define MODE_SEARCHING  1
    #define MODE_EOF        2

    int modo = MODE_MUST_READ;

    PByte memory = pq->result_memory + offset;
    int increment = pq->channels_result_size;

    double prediction = 0;

    // scan all items of grid
    for (int m = 0; m < total_items; m ++, 
        offset += pq->channels_result_size, memory += increment ) {

        //---------------------
        // Zera results
        //---------------------
        PTerminalChannel *slots = pq->ps->pti->p_slots;
        int *r_offs = pq->result_offset;

        // columns loop
        for (int i=0; i<pq->n_channels; i++, r_offs ++) {
           
            if (i != pq->ml_position) continue;

            if (m >= pq->ml_offset) {
                if (modo==MODE_MUST_READ) {
                    if (!f || !fgets(buf,2048,f)) {
                        modo = MODE_EOF;
                        prediction = 0;
                    } else {
                        prediction = atof(buf);
                    }
                }
            }

            PTerminalChannel ptcx = slots[pq->query_slots[i]];
            PByte pval = pq->result_memory + offset + *r_offs;
            if (ptcx->result_is_int) {
                * (int *) pval = (int) (prediction + 0.5);
            } else {
                * (double *) pval = prediction;
            }
        }
#if 0
        // increments the counters 
        int j = last_idx;
        while (j >= 0) {
            cs[j] ++;
            if (cs[j] < pq->group_by_distinct_count[j]) break;
            cs[j] = 0;
            j --;
        } 
#endif    
    }
}


#define prefix "../anomaly/bin/Release/netcoreapp2.2/publish/"

/*--------------------------
*
*
*
*/
void mlnet_compute(PQuery pq) {
    char buf[4096];

    if (!pq->ml_on) return;

    if (!pq->n_group_by_indexes) query_fatal("MLNET: Machine Learning algorithms requires a group-by.");
    if (pq->n_group_by_indexes != 1) query_fatal("MLNET: Machine Learning algorithms requires a unique group-by.");
    // if (pq->n_channels != 1) query_fatal("MLNET: Machine Learning algorithms requires a unique output channel.");

    int detect = (strncmp(pq->ml_method, "Detect",6)==0);
    int predict = (strncmp(pq->ml_method,"Prediction",10)==0);
    int must_run = detect || predict;
    if (!must_run) goto escape;

    // write arguments (time-series)
    sprintf(buf,"%s%s",prefix, "ts-in.txt");

    FILE *f = fopen(buf, "wt");
    if (!f) query_fatal("MLNET: fail to create comm file '%s'",buf);
    if (detect) {
        before_detect(f, pq);
    } else if (predict) {
        int limit = before_predict(f,pq);
        char buf[100];
        sprintf(buf, "%d %d", limit, pq->ml_len);
        pq->ml_parameters = strdup(buf);
    }
    fclose(f);

    // call dotnet
    sprintf(buf," %s %s%s %s %s","dotnet",prefix,"anomaly.dll",pq->ml_method,pq->ml_parameters);
    // printf("Calling DOTNET: %s\n", buf);
    system(buf);

    // read response
    sprintf(buf,"%s%s",prefix, "ts-out.txt");
    f = fopen(buf,"rt");
    if (detect) {
        after_detect(f,pq);
    } else if (predict) {
        after_predict(f,pq);
    }
    if (f) fclose(f);

escape:    
    if (pq->ml_method) {
        free(pq->ml_method);
        pq->ml_method = NULL;
    }

    if (pq->ml_parameters) {
        free(pq->ml_parameters);
        pq->ml_parameters = NULL;
    }
}