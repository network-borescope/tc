#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "metadata.h"
#include "schema.h"

#include "node_std.h"
#include "terminal.h"
#include "register.h"

//=============================================================================================
//
//  Schema
//
//=============================================================================================

static NodeFunctions nfs;
static TerminalNodeFunctions tfs;


/**
 * @brief 
 * 
 * @param pmd 
 * @return PSchema 
 */
PSchema metadata_create_schema(PMetaData pmd) {
    PSchema ps = schema_begin();
    
    PAddressInfo pai = pmd->pai;
    PAddressDim pad;
    PAddressValue pav;
    
    node_std_init_functions(&nfs);
    terminal_init_functions(&tfs);

    for (pad = pai->first; pad; pad = pad->next) {
        schema_new_level(ps);
        
        if (pad->record_to_address) {
//            printf("Dimensional: %d\n", pad->n_values);
            schema_new_value(ps, pad->n_values, &nfs);

        } else {
            for (pav = pad->first; pav; pav = pav->next) {
//                printf("Level: %d\n", 1);
                schema_new_value(ps, 1, &nfs);
            }
        }
    }
    
    schema_end(ps, &tfs);

    ps->pti = pmd->pti;

    return ps;
} 

void metadata_release_schema(PSchema ps) {
    terminal_term_functions(&tfs);
    
    node_std_cleanup();
    schema_release_all(ps);   
} 




//=============================================================================================
//
//  RECORD, ADDRESS
//
//=============================================================================================


//-----------------------
//
//
//
//
void * metadata_create_record(PMetaData pmd) {
    // printf("Create record. Size = %d\n",pmd->pri->record_sz);
    void *p = malloc(pmd->pri->record_sz);
    return p;
}

//-----------------------
//
//
//
//
void* metadata_release_record(void *pr) {
    free(pr);
    return NULL;
}


//-----------------------
//
//
//
//
_PUBLIC(PAddr) metadata_create_address(PMetaData pmd) {
    PAddr pa = calloc(1, sizeof(PAddr) + ( pmd->pai->n_values - 1 ) * sizeof(int) ); 
    pa->n_values = pmd->pai->n_values;
    return pa;
}

//-----------------------
//
//
//
//
_PUBLIC(PAddr) metadata_release_address(PAddr pa) {
    free(pa);
    return NULL;
}

//-----------------------
//
//
//
//
int metadata_record_to_address(PMetaData pmd, void *record, PAddr pa) {
    PAddressInfo pai = pmd->pai;
    PAddressDim pad;
    
    int pos = 0;
    int *values = pa->values;
    for (pad = pai->first; pad; pad = pad->next) {
        if (pad->record_to_address) {
            pad->record_to_address(record, pmd->pri->fields, pad->dim_params, pad->ids, pad->n_values, values);
            values += pad->n_values;
            pos += pad->n_values;
        } else {
            PAddressValue pav;
            for (pav = pad->first; pav; pav = pav->next) {
                pav->record_to_address(record, pmd->pri->fields, pav->val_params, pav->ids, 1, values);
                values += 1;
                pos += 1;
            }
        }
    } 
    if (pos != pmd->pai->n_values) {
        fatal("Error converting record to address");
    }
    return pos;
}

//=============================================================================================
//
//   CREATE & RELEASE    
//
//=============================================================================================


//-----------------------
//
//
//
//
PMetaData metadata_create(void) {
    PMetaData pmd = calloc(1, sizeof(MetaData));
    
    pmd->pii = &pmd->input_info;

    pmd->pai = &pmd->address_info;
    
    pmd->pri = &pmd->record_info;
    pmd->pti = &pmd->terminal_info;

    //register_channel("count", 0, 0);

    return pmd;
}

//-----------------------
//
//
//
//
PMetaData metadata_release(PMetaData pmd) {
    free(pmd);
    return NULL;
}

//=============================================================================================
//
//   DUMP    
//
//=============================================================================================

//-----------------------
//
//
//
//
void metadata_dump_record(PMetaData pmd, void *record) {
    int i;
    for (i = 0; i < pmd->pri->n_fields; i++) {
        PRecordField prf = pmd->pri->fields + i;
        printf("    '%s'[%2d] (sz:%d) #%d -> ", prf->name, prf->record_offset, prf->type_size, prf->field_id);
        char *ptr = (char *) record + prf->record_offset;
        switch(prf->type) {
            case MD_TYPE_FLOAT64:
                printf("%f \n",*(double *) ptr);
                break;
            case MD_TYPE_INT08:
            case MD_TYPE_INT32:
                printf("%d \n",* (int *) ptr);
                break;
            case MD_TYPE_INT64:
                printf("%lld \n",*(long long *) ptr);
                break;
            case MD_TYPE_STRING:
                printf("%s \n",ptr);
                break;
        }    
    }
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
__int64_t get_record_value(PRecord record, PContentData pcd, int idx) {
    int type = pcd->record_type[idx];
    void *p = record + pcd->record_offsets[idx];

    if (type == MD_TYPE_INT32) return * (int *) p;
    if (type == MD_TYPE_INT64) return * (__int64_t *) p;
    if (type == MD_TYPE_INT16) return * (short *) p;
    if (type == MD_TYPE_INT08) return * (char *) p;
    fatal("GET_RECORD_VALUE: invalid type");
    return 0;
}




