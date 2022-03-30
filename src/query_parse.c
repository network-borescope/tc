#include <stdlib.h>

#include "metadata.h"
#include "schema.h"
#include "register.h"
#include "query_processor.h"

//-----------------------
//
//
//
//
static PQCallInfo create_call_info(int idx_value) {
    // Inicializacao da CallInfo
    PQCallInfo pci = calloc(1,sizeof(QCallInfo));
    pci->value_index = idx_value;
    pci->qargs.n_args = 0;
    pci->qargs.index_reduce = 0;
    return pci;
}



/*-----------------
*
*
*
*/
static PClass get_class_by_value(PMetaData pmd, PSchema ps, int idx_value) {
    PClass pclass;
    PAddressValue pav = pmd->pai->p_values[idx_value];
    if (pav) {
        pclass = pav->pclass;
    } else {
        int idx_dim = ps->dim_of_value[idx_value];
        pclass = pmd->pai->p_dimensions[idx_dim]->pclass;
    }
    return pclass;
}

/*-----------------
*
*
*
*/
static void handle_selected_channel(char *channel_name, PQuery pq, PMetaData pmd) {
    // preparado para lidar com multiplos resultados
    // trata apenas os resultados individuais
    int idx_slot;
    if (!register_locate_slot(channel_name, &idx_slot)) {
        query_fatal("QUERY: channel '%s' not found.",channel_name);
    }

//    idx_channel --;
//    printf("QUERY: slot: %d\n", idx_slot);

    // seleciona o slot correto
    PTerminalChannel ptc = pmd->pti->p_slots[idx_slot];

    if (ptc->is_container) {
        query_fatal("QUERY: Container terminal '%s' cannot be used in select",channel_name);
    }

#define MANDATORY_CONTENT_CLASS(X) \
    ((X)->use_content_class & (CONTENT_CLASS_GROUPBY | CONTENT_CLASS_SELECT))

    if (MANDATORY_CONTENT_CLASS(pq)) {  
        if (!ptc->is_subchannel) {
        }
    }

    if (ptc->is_subchannel) {
//        printf("SUBCHANNEL\n");
        if (!pq->use_content_class) {
            query_fatal("QUERY: Contained channel '%s' cannot be used in 'select' of this query.",channel_name);
        }

        if (!(pq->use_content_class & CONTENT_CLASS_GROUPBY)) {
            pq->id_container = ptc->id_parent_slot;
//            printf("SET ID_CONTENT_CLASS\n");

        } else if (pq->id_container != idx_slot) {
            //query_fatal("slot %d != %d!!",pq->id_container,idx_slot);
        }
        pq->use_content_class |= CONTENT_CLASS_SELECT;
        

    } else if (MANDATORY_CONTENT_CLASS(pq)) {
        query_fatal("QUERY: You can only use contained channels %d", pq->use_content_class);
    }  

    pq->query_slots[pq->n_channels] = idx_slot;

    // prepara para fazer o merge
    pq->merge_offsets[pq->n_channels] = pq->channels_merge_size; 
    pq->channels_merge_size += ptc->merge_size; //ptc->pcfs->inquire(CONTENT_MERGE_SZ);

    pq->result_offset[pq->n_channels] = pq->channels_result_size;
    pq->channels_result_size +=  ptc->result_size; // ptc->pcfs->inquire(CONTENT_RESULT_SZ);

    PTerminalChannel *slots = pq->ps->pti->p_slots;
    pq->result_is_int[pq->n_channels] = slots[idx_slot]->result_is_int;

    pq->n_channels ++;
    //printf("SELECT DONE!\n");
}

/*-----------------
*
*
*
*/
static void fill_dimension(PQuery pq, PSchema ps, int idx_value, PQCallInfo pci) {
    int i;
    int idx_dim = ps->dim_of_value[idx_value];
    for (i=ps->top_of_dim[idx_dim]; i<ps->n_values_of_dim[idx_dim]; i++) {
        pq->ppci[i] = pci;
    }
}


/*-----------------
*
*
*
*/
static PQCallInfo prepare_group_by_pci_value(PQuery pq, PClass pclass, PSchema ps, int idx_value) {
    PQCallInfo pci = pq->ppci[idx_value];

    if (!pci) {
        /// printf("XXX\n");
        pci = create_call_info(idx_value);
        pci->op = pclass->op_all_plain;
        pci->op_group_by = pclass->op_all_group_by;
        
        // coloca o operador na posicao da query
        if (pclass->class_type == LEVEL_CLASS) {
            pq->ppci[idx_value] = pci;

        } else if (pclass->class_type == DIMENSIONAL_CLASS) {
            fill_dimension(pq, ps, idx_value, pci);
        }  
    }

    pci->call_group_by = 1;
    pci->group_by_idx = pq->n_group_bys;

    return pci;
}

/*-----------------
*
*
*
*/
static void handle_bounds_channel(char *channel_name, PQuery pq, PMetaData pmd) {
    // preparado para lidar com multiplos resultados
    // trata apenas os resultados individuais
    if (pq->n_group_bys) query_fatal("Bounds: pre-existent group-by");
    if (pq->n_channels) query_fatal("Bounds: pre-existent channel selected");

    int idx;
    PClass pclass;
    PSchema ps = pq->ps;

    if (register_locate_value(channel_name, &idx)) {
        // value
        pq->bounds_type = QUERY_VALUE_BOUNDS;
        pq->bounds_idx = idx;

        pclass = get_class_by_value(pmd, ps, idx);

        PQCallInfo pci = prepare_group_by_pci_value(pq, pclass, ps, idx);

        // printf("X3: %d\n", pq->n_group_by_indexes); fflush(stdout);
        if (pci && pci->op) {
            //printf("X3\n");
            for (int i = 0; i < pclass->n_group_by_indexes; i++)  {
                pq->group_by_bin_size[i] = 1;
                pq->group_by_n_points[i] = 0;
            }

            pclass->get_distinct_info(pci->op, 
                &pci->qargs, pci->params,
                pq->group_by_bin_size[pq->n_group_by_indexes],
                pq->group_by_distinct_count  + pq->n_group_by_indexes, 
                pq->group_by_distinct_base  + pq->n_group_by_indexes);

            pci->qargs.index_count = pq->group_by_distinct_count[pq->n_group_by_indexes];

#if 0
            printf("X3 base: %d  count: %d reduce: %d\n",
                pq->group_by_distinct_base[0], 
                pci->qargs.index_count, 
                pci->qargs.index_reduce);
#endif
        }

        pq->n_group_by_indexes += pclass->n_group_by_indexes;
        pq->n_group_bys ++;

    } else if (!register_locate_slot(channel_name, &idx)) {
        query_fatal("QUERY: bounds name '%s' not found.",channel_name);
        return;  // just to avoid warnings

    } else {
        // slot
        pq->bounds_type = QUERY_CONTAINER_BOUNDS;
        pq->bounds_idx = idx;

        PTerminalChannel ptc = pmd->pti->p_slots[idx];

        if (!ptc->is_container) {
            query_fatal("Bounds: terminal '%s' is not a container",channel_name);
        } else if (!pq->use_content_class) {
            pq->id_container = idx;
        } else if (pq->id_container != idx) {
            query_fatal("Query: it is not possible 'bounds' using more than one container channel.");
        }
        pq->use_content_class |= CONTENT_CLASS_GROUPBY;

        // printf("X0: %d\n", pq->n_group_by_indexes); fflush(stdout);
        // 

        // localiza a classe do valor
        pclass = ptc->pclass;

        //
        // configure indexes
        //        
        pq->group_by_inner_amount[0] = 1;

        for (int i = 0; i < pclass->n_group_by_indexes; i++) {
            pq->group_by_distinct_count[pq->n_group_by_indexes] = 2; 
            pq->group_by_distinct_base[pq->n_group_by_indexes] = 0;
            //printf("QueryParse(1): bin_size: %d of %d\n", ptc->bin_size, pclass->n_group_by_indexes);
            pq->group_by_bin_size[pq->n_group_by_indexes] = ptc->tfd.bin_size;
            pq->group_by_n_points[pq->n_group_by_indexes] = 0;
            pq->n_group_by_indexes ++;
        }
        pq->n_group_bys ++;
    }

    // prepara onde colocar o resultado
    for (int i=0; i< pclass->n_group_by_indexes; i++) {
        pq->merge_offsets[pq->n_channels] = pq->channels_merge_size; 
        pq->channels_merge_size += sizeof(int) * 2; 

        pq->result_offset[pq->n_channels] = pq->channels_result_size;
        pq->channels_result_size +=  sizeof(int) * 2; 

        pq->result_is_int[pq->n_channels] = 1;
        pq->n_channels ++;
    }

}




/*-----------------
*
*
*
*/
static void handle_group_by_string(char *name, PSchema ps, PQuery pq, PMetaData pmd, int n_points) {
    int idx_value;
    int idx_slot;

    // try to locate a index value 

    if (register_locate_value(name, &idx_value)) {
        //
        // locates dimensional information
        //
        PClass pclass = get_class_by_value(pmd, ps, idx_value);

        if (pclass->group_by_requires_rule) {
            if (!pq->ppci[idx_value]) {
                query_fatal("Missing a where clause on '%s' required by its use in group-by.", name);
            }
        }

        //
        // configurate indexes
        // 
        PQCallInfo pci = prepare_group_by_pci_value(pq, pclass, ps, idx_value);
        int count;
        pclass->get_distinct_info(pci->op, &pci->qargs, pci->params,
            pq->group_by_bin_size[pq->n_group_by_indexes],
            pq->group_by_distinct_count  + pq->n_group_by_indexes, 
            pq->group_by_distinct_base  + pq->n_group_by_indexes);

        count = pq->group_by_distinct_count [pq->n_group_by_indexes];

        for (int j = 0; j< pclass->n_group_by_indexes; j++) {
            pq->group_by_n_points[pq->n_group_by_indexes+j] = n_points;
        }

        pci->qargs.index_n_points  = n_points;
        pci->qargs.index_count     = count;
        pci->qargs.index_reduce = (n_points > 0) && (count > n_points);

        pq->n_group_by_indexes += pclass->n_group_by_indexes;
        pq->n_group_bys ++;

    // tries to locate as a complex slot
    } else if (register_locate_slot(name, &idx_slot)) {
        // printf("GROUP BY SLOT : %d\n",idx_slot);
        if (!pq->use_content_class) {
            pq->id_container = idx_slot;
        } else {
            if (pq->id_container != idx_slot) {
                // it is not possible to use two complex terminals as group by
                query_fatal("Query: it is not possible to group by using more than one container channel.");
            }
        }
        pq->use_content_class |= CONTENT_CLASS_GROUPBY;

        //printf("X0: %d\n", pq->n_group_by_indexes); fflush(stdout);
        // 
        PTerminalChannel ptc = pmd->pti->p_slots[idx_slot];

        // localiza a classe do valor
        PClass pclass = ptc->pclass;
        
        // pq->n_group_by_indexes += 1;
        //printf("X1: %d\n", pq->n_group_by_indexes); fflush(stdout);

        pq->n_group_bys ++;
        pq->group_by_inner_amount[0] = 1;

        pq->group_by_distinct_count[0] = GROUP_BY_DISTINCT_COUNT;
        pq->group_by_distinct_base[0] = 0;
        // printf("QueryParse(2): bin_size[0]: %d of %d\n", ptc->bin_size, pclass->n_group_by_indexes);        
        pq->group_by_bin_size[0] = ptc->tfd.bin_size;

        idx_value = pmd->pai->n_values;
        //printf("X2: %d\n", idx_value); fflush(stdout);

        PQCallInfo pci = pq->ppci[idx_value];
        //printf("X5: %d\n", pq->n_group_by_indexes); fflush(stdout);
        if (pci && pci->op) {
            int count;
            // printf("X3\n");
            pclass->get_distinct_info(pci->op, &pci->qargs, pci->params,
            pq->group_by_bin_size[pq->n_group_by_indexes],
            pq->group_by_distinct_count  + pq->n_group_by_indexes, 
            pq->group_by_distinct_base  + pq->n_group_by_indexes);

            count = pq->group_by_distinct_count[pq->n_group_by_indexes];
            for (int j = 0; j< pclass->n_group_by_indexes; j++) {
                pq->group_by_n_points[pq->n_group_by_indexes+j] = n_points;
            }
            pci->qargs.index_n_points  = n_points;
            pci->qargs.index_count     = count;
            pci->qargs.index_reduce = (n_points > 0) && (count > n_points);

#if 0
            printf("X5 base: %d  count: %d reduce: %d\n",
                pq->group_by_distinct_base[0], 
                pci->qargs.index_count, 
                pci->qargs.index_reduce);
#endif
        }
        //printf("X4: %d\n", pq->n_group_by_indexes); fflush(stdout);

        pq->n_group_by_indexes += pclass->n_group_by_indexes;
    // 
    } else {
        query_fatal("group-by field '%s' is unknonw", name);
    }
}

/*-----------------
*
*
*
*/
static void handle_group_by_object(cJSON *group_by_object, PSchema ps, PQuery pq, PMetaData pmd) {

    int n_points = 0;

    //-------------------------------
    //  n
    //-------------------------------
    cJSON *n = cJSON_GetObjectItem(group_by_object, "n");
    if (n) {
        if (!cJSON_IsNumber(n) || (n->valueint < 0)) {
            query_fatal("Query: GroupBy 'n' must be an integer non negative");
        }
        n_points = n->valueint;
    }

    //-------------------------------
    //  field
    //-------------------------------
    cJSON *field = cJSON_GetObjectItem(group_by_object, "field");
    if (!field) {
        fatal("Missing field in group-by");
    }
    if (!cJSON_IsString(field)) {
        query_fatal("Query: GroupBy 'field' must be a string");
    }
    handle_group_by_string(field->valuestring,ps, pq, pmd, n_points);
}


//-----------------------
//
//
//
//
static PQCallInfo parse_operator(cJSON *rule, PSchema ps, PClass pclass, int idx_value) {
    // 
    // operador  
    //
    cJSON *opstr = cJSON_GetArrayItem(rule,1);
    if (!opstr || !cJSON_IsString(opstr)) query_fatal("Operator must be a string.");

    PQOpInfo poi;
    if (!register_locate_op(pclass->name,opstr->valuestring, &poi)) {
        query_fatal ("Unknown operator ''%s", opstr);
    }
    
    PQCallInfo pci = create_call_info(idx_value);
    pci->op = poi->op;
    pci->op_group_by = poi->op_group_by;
    
    // parse dos argumentos 
    int n = cJSON_GetArraySize(rule);
    int i, j;
    char *c, tp;
    char last_c = '\0';
    char default_tp = 'D';

    for (i = 2, j =0, c = poi->fmt; i<n; i++, j++) {
        
        if (pci->qargs.n_args > poi->max_args) {
            query_fatal("Too many arguments [%d] in class '%s' [%d]",pci->qargs.n_args,pclass->name,poi->max_args);
        }

        cJSON *arg = cJSON_GetArrayItem(rule,i);

        if (!cJSON_IsNumber(arg)) query_fatal ("Number expected in class '%s'",pclass->name);

        if (!*c) {
            tp = last_c ? last_c: default_tp;
        } else {
            tp = *c;
            last_c = *c;
             c ++;
        }

        if (tp == 'd') {
            tp = 'D';
        } else if (tp == 'i') {
            tp = 'I';
        }

        if (tp == 'I') {
            //printf("ARG[%d]: %d\n", j, arg->valueint);
            pci->qargs.args[j].i = arg->valueint;

        } else if (tp == 'D') {
            //printf("ARG[%d]: %f\n", j,  arg->valuedouble);
            pci->qargs.args[j].d = arg->valuedouble;

        } else {
            query_fatal("Invalid parameter format [%d] '%c' in class '%s'.",j+1, tp, pclass->name);
        }
        pci->qargs.n_args ++;
    }

    // allows preparation of op_data 
    if (pclass->create_op_data) {
        pci->op_data = pclass->create_op_data(pci->op, &pci->qargs);
    }

    return pci;
}


/*-----------------
*
*
*
*/
static void handle_where_value(int idx_value, cJSON *rule, PQuery pq, PSchema ps, PMetaData pmd) {
    if (pq->ppci[idx_value]) {
        query_fatal("Query: reappling a filter over a field %d", idx_value);
    }

    // gets the dimension of the value
    int idx_dim = ps->dim_of_value[idx_value];

    // localiza a classe do valor
    PClass pclass = get_class_by_value(pmd, ps, idx_value);

    // pega o operador
    PQCallInfo pci = parse_operator(rule, ps, pclass, idx_value);

    // coloca o operador na posicao da query
    if (pclass->class_type == LEVEL_CLASS) {
        pq->ppci[idx_value] = pci;

    } else if (pclass->class_type == DIMENSIONAL_CLASS) {
        int i;
        for (i=ps->top_of_dim[idx_dim]; i<ps->n_values_of_dim[idx_dim]; i++) {
            pq->ppci[i] = pci;
        }

    } else {
        query_fatal("ERROR: 138383");
    }

    //
    // locates dimensional information
    //
    PAddressDim pad = pmd->pai->p_dimensions[idx_dim];

    // if the value is a class, must check more things...
    if (pad->record_to_address){
        if (idx_value != ps->top_of_dim[idx_dim]) {
            query_fatal("idx - not top");
        }   
    }

}


/*-----------------
*
*
*
*/
static void handle_where_slot(int idx_slot, cJSON *rule, PQuery pq, PSchema ps, PMetaData pmd) {

    PTerminalChannel ptc = pmd->pti->p_slots[idx_slot];

    if (!ptc->is_container) {
        query_fatal("Query: Only 'class' terminals can be used in 'where'");
    }

    if (!pq->use_content_class) {
        pq->id_container = idx_slot;

    } else if (pq->id_container != idx_slot) {
        query_fatal("Query: it is not possible to use more than one class terminal");
    }
    pq->use_content_class |= CONTENT_CLASS_WHERE;
//    printf("SLOT IDX:%d  Use_Content_Slot: %d\n", idx_slot, pq->use_content_class);

    int idx_value = ps->n_values;

    if (pq->ppci[idx_value]) {
        query_fatal("Query: reappling a filter over a field");
    }
    
    // localiza a classe do valor
    PClass pclass = ptc->pclass;

    // pega o operador
    PQCallInfo pci = parse_operator(rule, ps, pclass, idx_value);

    // coloca o operador na posicao da query
    pq->ppci[idx_value] = pci;

    // coloca os parametros do container a disposicao do PCI
    pci->params = ptc->tfd.params;
}

/*-----------------
*
*
*
*/
static void * do_query_parse(Query *pq, PMetaData pmd, PSchema ps, char *src) {

    pq->ps = ps;

    const char *return_parse_end = NULL;
    cJSON *q = cJSON_ParseWithOpts(src, &return_parse_end, 1); // cJSON_Parse(src); // 

    #define MAX_ERR_SZ 64
    char err[MAX_ERR_SZ];  
    *err = '\0';
    if (*return_parse_end) { 
        strncpy(err, return_parse_end,MAX_ERR_SZ-2);
        err[MAX_ERR_SZ-2] = '\0'; 
    }

    //-------------------------------
    // Check if JSON string was valid
    //-------------------------------
    if (*err) { 
        // printf("******\n %s\n",err);
        query_fatal("Invalid json string. Error occured just before => %s",err); 
    }

    //-------------------------------
    //  id
    //-------------------------------
    cJSON *id = cJSON_GetObjectItem(q, "id");
    if (id) {
        if (!cJSON_IsNumber(id)) {
            query_fatal("Query: Id must be a number");
        }
        pq->id = id->valueint;
    }

    //-------------------------------
    //  WHERE
    //-------------------------------
    cJSON *where = cJSON_GetObjectItem(q, "where");
    if (where) {

        if (!cJSON_IsArray(where)) {
            query_fatal("Query: where");
        }

        cJSON *rule;
        cJSON_ArrayForEach(rule, where) {
            if (!cJSON_IsArray(rule)) {
                query_fatal("Query: rule");
            }

            //
            // nome do campo da arvore
            //
            cJSON *name = cJSON_GetArrayItem(rule,0);
            if (!name || !cJSON_IsString(name)) query_fatal ("Query: name");

            //
            //  
            //
            int idx;
            if (register_locate_value(name->valuestring, &idx)) {
                handle_where_value(idx, rule, pq, ps, pmd);

            } else if (register_locate_slot(name->valuestring, &idx)) {
                handle_where_slot(idx, rule, pq, ps, pmd);

            } else {
                query_fatal("Query: name '%s' not found", name->valuestring);
            }
            // printf("Value %s -> %d\n", name->valuestring, idx);

        }
    } // where

    //-------------------------------
    //   group-by
    //-------------------------------
    cJSON *group_by = cJSON_GetObjectItem(q,"group-by");
    
    if (group_by) {
        // printf("YYY\n");
        if (cJSON_IsString(group_by)) {
            handle_group_by_string(group_by->valuestring, ps, pq, pmd, 0);
        } else if (cJSON_IsArray(group_by)) {
            cJSON *gb;
            cJSON_ArrayForEach(gb, group_by) {
                if (!cJSON_IsString(gb)) {
                    query_fatal("QUERY: all items in select must be a string"); 
                }
                handle_group_by_string(gb->valuestring, ps, pq, pmd, 0);
            }
        } else if (cJSON_IsObject(group_by)) {
            handle_group_by_object(group_by, ps, pq, pmd);

        } else {
            query_fatal("QUERY: a group-by must be a channel name or a list of channel names"); 
        }
    }

    //-------------------------------
    //   layout
    //-------------------------------
    cJSON *layout = cJSON_GetObjectItem(q,"group-by-output");
    if (layout) {
        if (!group_by) query_fatal("'group-by-output' clause requires 'group-by'");
        if (!cJSON_IsString(layout)) query_fatal("'group-by-output' parameter must be a string");

        if (strncmp("kv",layout->valuestring,2)==0) {
            pq->group_by_output = GROUP_BY_OUTPUT_KS_VS;

        } else if (strncmp("vs_ks",layout->valuestring,5)==0) {
            pq->group_by_output = GROUP_BY_OUTPUT_VS_KS;

        } else if (strncmp("v",layout->valuestring,1)==0) {
            pq->group_by_output = GROUP_BY_OUTPUT_V_LISTS;

        } else if (strncmp("list",layout->valuestring,4)==0) {
            pq->group_by_output = GROUP_BY_OUTPUT_K_LISTS;

        } else if (strncmp("full",layout->valuestring,4)==0) {
            pq->group_by_output = GROUP_BY_OUTPUT_FULL_LISTS;
        } else {
            query_fatal("'group-by-output': unknown parameter '%s'", layout->valuestring);
        }

    }


    //-------------------------------
    // ml.net
    //-------------------------------   
    cJSON *ml = cJSON_GetObjectItem(q, "ml.net");
    cJSON *ml_nm = NULL;
    pq->ml_on = 0;
    pq->ml_method = NULL;
    pq->ml_parameters = NULL;
    pq->ml_offset = -1;
    pq->ml_len = -1;

    int ml_ok = 0;
    if (ml) {

        if (!cJSON_IsArray(ml)) {
            query_fatal("ML.NET: Missing parameters (must be an JSON Array)");
        }

        //*************** Method
        cJSON *what = cJSON_GetArrayItem(ml,0);
        if (!what || !cJSON_IsString(what)) query_fatal ("ML.NET: missing ML.NET algorithm");
        pq->ml_method = strdup(what->valuestring);

        //*************** Channel
        ml_nm = cJSON_GetArrayItem(ml,1);
        if (!ml_nm || !cJSON_IsString(ml_nm)) query_fatal ("ML.NET: missing selected channel");

        //*************** Parameters
        if (strncmp(what->valuestring,"Prediction",10)!=0) {
            cJSON *params = cJSON_GetArrayItem(ml,2);
            if (params) {
                if (!cJSON_IsString(params)) query_fatal ("ML.NET: missing ML.NET algorithm");
                pq->ml_parameters = strdup(params->valuestring);
            }

        // Prediction is special
        } else {

            cJSON *ml_len = cJSON_GetArrayItem(ml,2);
            if (ml_len) {
                if (!cJSON_IsNumber(ml_len)) query_fatal("ML.NET: lenght of Prediction function must be a number.");
                if (ml_len->valueint < 1) query_fatal("ML.NET: lenght of Prediction function must be positive.");
                pq->ml_len = ml_len->valueint;

                cJSON *ml_offset = cJSON_GetArrayItem(ml,3);
                if (ml_offset) {
                    if (!cJSON_IsNumber(ml_offset)) query_fatal("ML.NET: Prediction function must specify a numeric offset.");
                    if (ml_offset->valueint < 1) query_fatal("ML.NET: Prediction function offset must be positive.");
                    pq->ml_offset = ml_offset->valueint;
                } 
            }
        }

        pq->ml_on = 1;
        pq->ml_idx_slot = -1;
        pq->ml_position = 0;
    }
 
    //-------------------------------
    //   Check Existence of bounds or schema
    //-------------------------------
    cJSON *bounds = cJSON_GetObjectItem(q,"bounds");
    cJSON *schema = cJSON_GetObjectItem(q, "schema");

    //-------------------------------
    //   SELECT
    //-------------------------------
    cJSON *select = cJSON_GetObjectItem(q,"select");
    cJSON *new_select  = NULL;
    if (select) {
        if (bounds) query_fatal("'bounds' clause is incompatible with 'select' clause");
    } else if (!bounds && !schema) { 
        // create an artificial select of "counter"
        new_select = cJSON_CreateString("counter");
        select = new_select;
    }

    if (select) {
        // Is it a simple select ?
        if (cJSON_IsString(select)) { 
            handle_selected_channel(select->valuestring, pq, pmd);
            if (ml && !ml_ok) {
                ml_ok = !strcmp(select->valuestring, ml_nm->valuestring);        
                pq->ml_idx_slot = pq->query_slots[pq->n_channels-1];
            }

        // select is a list of items!
        } else if (cJSON_IsArray(select)) {
            
            cJSON * channel;
            cJSON_ArrayForEach(channel, select) {
                if (!cJSON_IsString(channel)) {
                    query_fatal("QUERY: all items in select must be a string"); 
                }
                handle_selected_channel(channel->valuestring, pq, pmd);
                if (ml && !ml_ok) {
                    ml_ok = !strcmp(channel->valuestring, ml_nm->valuestring);
                    pq->ml_idx_slot = pq->query_slots[pq->n_channels-1];
                    if (!ml_ok) pq->ml_position ++; 
                }
            }

        // neither simple nor list    
        } else {
            query_fatal("QUERY: a select must be a channel name or a list of channel names"); 
        }
    }

    //-------------------------------
    //   BOUNDS
    //-------------------------------
    if (bounds) { 
        if (!cJSON_IsString(bounds)) query_fatal("'bouns' parameter must be a string");
        if (select) query_fatal("'bounds' clause is incompatible with 'select' clause");
        if (group_by) query_fatal("'bounds' clause is incompatible with 'group-by' clause");
        if (layout) query_fatal("'bounds' clause is incompatible with 'group-by-output' clause");
        if (ml) query_fatal("'bounds' clause is incompatible with 'group-by-output' ml.net");
        handle_bounds_channel(bounds->valuestring, pq, pmd);
    }

    //-------------------------------
    //  info query
    //-------------------------------
    if (schema) {
        if (select) query_fatal("'schema' clause is incompatible with 'select' clause");
        if (group_by) query_fatal("'schema' clause is incompatible with 'group-by' clause");
        if (layout) query_fatal("'schema' clause is incompatible with 'group-by-output' clause");
        if (ml) query_fatal("'schema' clause is incompatible with 'ml.net' clause");
        if (bounds) query_fatal("'schema' clause is incompatible with 'bounds' clause");

        pq->schema_query = 1;
        pq->json_src = pmd->json_src;
    }

    // remove elementos criados manualmente
    if (new_select) cJSON_Delete(new_select);

    // delete all cJSON items
    if (q) cJSON_Delete(q);

    if (pq->ml_on) {
        if (!ml_ok) {
            query_fatal("ml.net channel '%s' was not selected", ml_nm->valuestring);
        } else {
            //printf("ML.NET: %d %d %s %d \n", pq->ml_idx_method,pq->ml_position, ml_nm->valuestring, pq->ml_idx_slot);
        }
    }

    return pq;
}

/*-----------------
*
*
*
*/
int query_parse(PQuery pq, PMetaData pmd, PSchema ps, char *src) {
    if (do_query_parse(pq, pmd, ps, src)) return 1;
    return 0;
}

