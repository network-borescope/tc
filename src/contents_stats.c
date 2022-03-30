#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "schema.h"
#include "metadata.h"
#include "register.h"

//=============================================================================================
//
//  counter16
//
//=============================================================================================
typedef struct __attribute__((__packed__)) {
    unsigned short counter;
}  Counter16, *PCounter16;

typedef struct __attribute__((__packed__)) {
    unsigned int counter;
}  Counter16Merge, *PCounter16Merge;


//----------------------
//
//
//
static void * counter16_insert(PRecord record, void *data, PContentData pcd) {
    PCounter16 pc = data;
    pc->counter ++;
    if (pc->counter == 0) fatal("Counter16 overflow");
    return NULL;
}

//----------------------
//
//
//
static void * counter16_remove(PRecord record, void *data, PContentData pcd) {
    PCounter16 pc = data;
    if (pc->counter > 0) pc->counter--;
    return NULL;
}

//----------------------
//
//
//
static void counter16_merge(PByte merge_data, void *data, PContentData pcd) {
    PCounter16Merge m = (PCounter16Merge) merge_data;
    PCounter16 d = (PCounter16) data;

    // printf(" BEFORE MERGE OF %d with  %d", m->counter, d->counter);
    m->counter += d->counter;
    // printf(" AFTER MERGE SUM: %d\n", m->counter);
}

//----------------------
//
//
//
static void counter16_result(PByte merge_data, PByte result_data) {
    PCounter16Merge m = (PCounter16Merge) merge_data;
    int * p = (int *) result_data;

    *p = m->counter;
}

//----------------------
//
//
//
static void counter16_register(void) {
    // Counter
    static ContentInfo ci;
    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 0;
    ci.min_params = 0;
    ci.max_params = 0;
    ci.params_size = 0;
    ci.private_size = sizeof(Counter16);
    ci.merge_size = sizeof(Counter16Merge);
    ci.result_size = sizeof(Counter16Merge);
    ci.result_is_int = 1;

    static ContentFunctions cfs;
    ci.pcfs = &cfs;

    cfs.parse_param = NULL;
    cfs.insert = &counter16_insert;
    cfs.remove = &counter16_remove;
    cfs.merge = &counter16_merge;
    cfs.result = &counter16_result;

    register_content("counter16", &ci);
}



//=============================================================================================
//
//  counter
//
//=============================================================================================
typedef struct {
    int counter;
}  Counter, *PCounter;

//----------------------
//
//
//
static void * counter_insert(PRecord record, void *data, PContentData pcd) {
    PCounter pc = data;
    pc->counter ++;
    return NULL;
}

//----------------------
//
//
//
static void * counter_remove(PRecord record, void *data, PContentData pcd) {
    PCounter pc = data;
    if (pc->counter > 0) pc->counter--;
    return NULL;
}

//----------------------
//
//
//
static void counter_merge(PByte merge_data, void *data, PContentData pcd) {
    PCounter m = (PCounter) merge_data;
    PCounter d = (PCounter) data;

    // printf(" BEFORE MERGE OF %d with  %d", m->counter, d->counter);
    m->counter += d->counter;
    // printf(" AFTER MERGE SUM: %d\n", m->counter);
}

//----------------------
//
//
//
static void counter_result(PByte merge_data, PByte result_data) {
    PCounter m = (PCounter) merge_data;
    int * p = (int *) result_data;

    *p = m->counter;
}

//----------------------
//
//
//
static void counter_register(void) {
    // Counter
    static ContentInfo ci;
    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 0;
    ci.min_params = 0;
    ci.max_params = 0;
    ci.params_size = 0;
    ci.private_size = sizeof(Counter);
    ci.merge_size = sizeof(Counter);
    ci.result_size = sizeof(Counter);
    ci.result_is_int = 1;

    static ContentFunctions cfs;
    ci.pcfs = &cfs;

    cfs.parse_param = NULL;
    cfs.insert = &counter_insert;
    cfs.remove = &counter_remove;
    cfs.merge = &counter_merge;
    cfs.result = &counter_result;

    register_content("#", &ci);
    register_content("counter", &ci);
}


//=============================================================================================
//
//    
//
//=============================================================================================

typedef struct {
    int    has_fator;
    double fator;
} FatorParam, *PFatorParam;

#define APPLY_FATOR(PTR, V)  ((PTR)->has_fator?(V)*(PTR)->fator:(V))

//----------------------
//
//
//
static void parse_fator(int id_param, char *s, PByte params) {
    PFatorParam p = (PFatorParam) params;
    if (id_param == 1) {
        p->fator = atof(s);
        if (p->fator <= 0) return;
        p->has_fator = 1;
        // printf("SUM PARAMS: %d %f\n", p->has_fator, p->fator );
    }
}


//=============================================================================================
//
//    
//
//=============================================================================================

typedef struct {
    double sum;
} SumData, *PSumData;

//----------------------
//
//
//
static void * sum_insert(PRecord record, void *data, PContentData pcd) {
    PSumData p = (PSumData) data;
    PFatorParam pfp = (PFatorParam) pcd->params;

    __int64_t v = get_record_value(record, pcd, 0);

//    printf("INSERT SUM(before): %f  value: %d  offs: %d", p->sum, v, offs);
    p->sum += APPLY_FATOR(pfp,v);
//    printf("  SUM: %f\n", p->sum);

    return NULL;
}

//----------------------
//
//
//
static void * sum_remove(PRecord record, void *data, PContentData pcd) {
    PSumData p = (PSumData) data;
    PFatorParam pfp = (PFatorParam) pcd->params;

    __int64_t v = get_record_value(record, pcd, 0);

    p->sum += APPLY_FATOR(pfp,v);
    // printf("  SUM: %f\n", p->sum);

    return NULL;
}


//----------------------
//
//
//
static void sum_merge(PByte merge_data, void *data, PContentData pcd) {
    PSumData m = (PSumData) merge_data;
    PSumData p = (PSumData) data;

    // printf(" BEFORE MERGE OF %f with  %f", m->sum, p->sum);
    m->sum += p->sum;
    // printf(" AFTER MERGE SUM: %f\n", m->sum);
}

//----------------------
//
//
//
static void sum_result(PByte merge_data, PByte result_data) {
    PSumData m = (PSumData) merge_data;
    double * p = (double *) result_data;

    *p = m->sum;
}

//----------------------
//
//
//
static void sum_register(void) {
    // SUM
    static ContentFunctions cfs;
    static ContentInfo ci;

    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 1;
    ci.min_params = 0;
    ci.max_params = 1;
    ci.params_size = sizeof(double);
    ci.private_size = sizeof(SumData);
    ci.merge_size = ci.private_size;
    ci.result_size = sizeof(double);
    ci.result_is_int = 0;

    ci.pcfs = &cfs;
    cfs.parse_param = &parse_fator;
    cfs.insert = &sum_insert;
    cfs.remove = &sum_remove;
    cfs.merge = &sum_merge;
    cfs.result = &sum_result;

    // cfs.inquire = &sum_inquire;

    register_content("sum", &ci);
}

//=============================================================================================
//
//    Average 1 parameter
//
//=============================================================================================

typedef struct __attribute__((__packed__)) {
    double sum;
    double n;
} AvgData, *PAvgData;

//----------------------
//
//
//
static void * avg_insert(PRecord record, void *data, PContentData pcd) {
    PAvgData p = (PAvgData) data;
    PFatorParam pfp = (PFatorParam) pcd->params;

    __int64_t v = get_record_value(record, pcd, 0);

//    printf("INSERT SUM(before): %f  value: %d  offs: %d", p->sum, v, offs);
    p->sum += APPLY_FATOR(pfp,v);
    p->n ++;
//    printf("  SUM: %f\n", p->sum);

    return NULL;
}

//----------------------
//
//
//
static void * avg_remove(PRecord record, void *data, PContentData pcd) {
    PAvgData p = (PAvgData) data;
    PFatorParam pfp = (PFatorParam) pcd->params;

    __int64_t v = get_record_value(record, pcd, 0);

    // printf("REMOVE SUM(before): %f  value: %d  offs: %d", p->sum, v, offs);
    p->sum -= APPLY_FATOR(pfp,v);
    p->n --;
    // printf("  SUM: %f\n", p->sum);

    return NULL;
}

//----------------------
//
//
//
static void * avg2_insert(PRecord record, void *data, PContentData pcd) {
    PAvgData p = (PAvgData) data;
    PFatorParam pfp = (PFatorParam) pcd->params;

    __int64_t v1 = get_record_value(record, pcd, 0);
    __int64_t v2 = get_record_value(record, pcd, 1);

//    printf("INSERT SUM(before): %f  value: %d  offs: %d", p->sum, v, offs);
    p->sum += APPLY_FATOR(pfp,v1);
    p->n += v2;
//    printf("  SUM: %f\n", p->sum);

    return NULL;
}

//----------------------
//
//
//
static void * avg2_remove(PRecord record, void *data, PContentData pcd) {
    PAvgData p = (PAvgData) data;
    PFatorParam pfp = (PFatorParam) pcd->params;

    __int64_t v1 = get_record_value(record, pcd, 0);
    __int64_t v2 = get_record_value(record, pcd, 1);

//    printf("REMOVE SUM(before): %f  value: %d  offs: %d", p->sum, v, offs);
    p->sum -= APPLY_FATOR(pfp,v1);
    p->n -= v2;
//    printf("  SUM: %f\n", p->sum);

    return NULL;
}

//----------------------
//
//
//
static void avg_merge(PByte merge_data, void *data, PContentData pcd) {
    PAvgData m = (PAvgData) merge_data;
    PAvgData p = (PAvgData) data;

    // printf(" BEFORE MERGE OF %f with  %f", m->sum, p->sum);
    m->sum += p->sum;
    m->n += p->n;
    // printf(" AFTER MERGE SUM: %f\n", m->sum);
}

//----------------------
//
//
//
static void avg_result(PByte merge_data, PByte result_data) {
    PAvgData m = (PAvgData) merge_data;
    double * p = (double *) result_data;

    if (m->n <= 0) {
        *p = 0;
    } else {
        //printf(" BEFORE RESULT OF %f with  %d", m->sum, m->n);
        *p = m->sum / m->n;
    }
    // printf(" AFTER MERGE SUM: %f\n", m->sum);
}

//----------------------
//
//
//
static void avg_register(void) {
    // SUM
    static ContentFunctions cfs;
    static ContentInfo ci;

    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 1;
    ci.min_params = 0;
    ci.max_params = 0;
    ci.params_size = 0;
    ci.private_size = sizeof(AvgData);
    ci.merge_size = ci.private_size;
    ci.result_size = sizeof(double);
    ci.result_is_int = 0;

    ci.pcfs = &cfs;
    cfs.parse_param = &parse_fator;
    cfs.insert = &avg_insert;
    cfs.remove = &avg_remove;
    cfs.merge = &avg_merge;
    cfs.result = &avg_result;
    
    register_content("avg", &ci);
}

//----------------------
//
//
//
static void avg2_register(void) {
    // SUM
    static ContentFunctions cfs;
    static ContentInfo ci;

    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 2;
    ci.min_params = 0;
    ci.max_params = 0;
    ci.params_size = 0;
    ci.private_size = sizeof(AvgData);
    ci.merge_size = ci.private_size;
    ci.result_size = sizeof(double);
    ci.result_is_int = 0;

    ci.pcfs = &cfs;
    cfs.parse_param = &parse_fator;
    cfs.insert = &avg2_insert;
    cfs.remove = &avg2_remove;
    cfs.merge = &avg_merge;
    cfs.result = &avg_result;
    
    register_content("avg2", &ci);
}



//=============================================================================================
//
//    
//
//=============================================================================================

typedef struct __attribute__((__packed__)) {
    double sum2;
    double sum;
    int n;
} VarData, *PVarData;

//----------------------
//
//
//
static void * var_insert(PRecord record, void *data, PContentData pcd) {
    PVarData p = (PVarData) data;
    PFatorParam pfp = (PFatorParam) pcd->params;

    __int64_t v = get_record_value(record, pcd, 0);

//    printf("INSERT SUM(before): %f  value: %d  offs: %d", p->sum, v, offs);
    v = APPLY_FATOR(pfp,v);
    p->sum2 += (v * v);
    p->sum += v;
    p->n ++;
//    printf("  SUM: %f\n", p->sum);

    return NULL;
}

//----------------------
//
//
//
static void * var_remove(PRecord record, void *data, PContentData pcd) {
    PVarData p = (PVarData) data;
    PFatorParam pfp = (PFatorParam) pcd->params;

    __int64_t v = get_record_value(record, pcd, 0);

    // printf("REMOVE SUM(before): %f  value: %d  offs: %d", p->sum, v, offs);
    v = APPLY_FATOR(pfp,v);
    p->sum2 -= v * v;
    p->sum -= v;
    p->n --;
    // printf("  SUM: %f\n", p->sum);

    return NULL;
}



//----------------------
//
//
//
static void var_merge(PByte merge_data, void *data, PContentData pcd) {
    PVarData m = (PVarData) merge_data;
    PVarData p = (PVarData) data;

    //printf(" BEFORE MERGE OF %f with %f %f with %f", m->sum2, p->sum2, m->sum, p->sum);
    m->sum2 += p->sum2;
    m->sum += p->sum;
    m->n += p->n;
    //printf(" AFTER MERGE SUM: %f %f\n", m->sum2, m->sum);
#if 0
    if ( (m->sum2 < 0) || m->sum < 0) {
        printf("NEGATIVE VAR MERGE: %f %f %d\n", p->sum2, p->sum, p->n);
        printf("   AFTER VAR MERGE: %f %f %d\n", m->sum2, m->sum, m->n);

    }
#endif
}

//----------------------
//
//
//
static void var_result(PByte merge_data, PByte result_data) {
    PVarData m = (PVarData) merge_data;
    double * p = (double *) result_data;

    if (m->n <= 0) {
        *p = 0;
    } else {
        //printf(" BEFORE RESULT OF sum2=%f sum=%f n=%d", m->sum2, m->sum, m->n);
        double e_x2 = m->sum2 / m->n;
        double e_x = m->sum / m->n;
        *p = e_x2 - (e_x * e_x);
        //printf(" AFTER RESULT SUM: %f\n", *p);
    }
}


//----------------------
//
//
//
static void var_register(void) {
    // Var
    static ContentFunctions cfs;
    static ContentInfo ci;

    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 1;
    ci.min_params = 0;
    ci.max_params = 0;
    ci.params_size = 0;
    ci.private_size = sizeof(VarData);
    ci.merge_size = ci.private_size;
    ci.result_size = sizeof(double);
    ci.result_is_int = 0;

    ci.pcfs = &cfs;
    cfs.parse_param = &parse_fator;
    cfs.insert = &var_insert;
    cfs.remove = &var_remove;
    cfs.merge = &var_merge;
    cfs.result = &var_result;

    register_content("var", &ci);
}
#if 0
//----------------------
//
//
//
static void var2_register(void) {
    // SUM
    static ContentFunctions cfs;
    static ContentInfo ci;

    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 2;
    ci.min_params = 0;
    ci.max_params = 0;
    ci.params_size = 0;
    ci.private_size = sizeof(VarData);
    ci.merge_size = ci.private_size;
    ci.result_size = sizeof(double);
    ci.result_is_int = 0;

    ci.pcfs = &cfs;
    cfs.parse_param = &var_parse;
    cfs.insert = &var2_insert;
    cfs.remove = &var2_remove;
    cfs.merge = &var_merge;
    cfs.result = &var_result;

    register_content("var2", &ci);
}
#endif


//=============================================================================================
//
//    
//
//=============================================================================================

//----------------------
//
//
//
static void sd_result(PByte merge_data, PByte result_data) {
    PVarData m = (PVarData) merge_data;
    double * p = (double *) result_data;

    if (m->n <= 0) {
        *p = 0;
    } else {
        //printf(" BEFORE RESULT OF %f with  %d", m->sum, m->n);
        double e_x2 = m->sum2 / m->n;
        double e_x = m->sum / m->n;
        *p = sqrt(fabs(e_x2 - (e_x * e_x)));
    }
    // printf(" AFTER MERGE SUM: %f\n", m->sum);
}




//----------------------
//
//
//
static void stdev_register(void) {
    // SUM
    static ContentFunctions cfs;
    static ContentInfo ci;

    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 1;
    ci.min_params = 0;
    ci.max_params = 0;
    ci.params_size = 0;
    ci.private_size = sizeof(VarData);
    ci.merge_size = ci.private_size;
    ci.result_size = sizeof(double);
    ci.result_is_int = 0;

    ci.pcfs = &cfs;
    cfs.parse_param = &parse_fator;
    cfs.insert = &var_insert;
    cfs.remove = &var_remove;
    cfs.merge = &var_merge;
    cfs.result = &sd_result;

    register_content("sd", &ci);
}

#if 0

//----------------------
//
//
//
static void stdev2_register(void) {
    // SUM
    static ContentFunctions cfs;
    static ContentInfo ci;

    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 2;
    ci.min_params = 0;
    ci.max_params = 0;
    ci.params_size = 0;
    ci.private_size = sizeof(VarData);
    ci.merge_size = ci.private_size;
    ci.result_size = sizeof(double);
    ci.result_is_int = 0;

    ci.pcfs = &cfs;
    cfs.parse_param = &var_parse;
    cfs.insert = &var2_insert;
    cfs.remove = &var2_remove;
    cfs.merge = &var_merge;
    cfs.result = &sd_result;

    register_content("sd2", &ci);
}
#endif
//=============================================================================================
//
//    
//
//=============================================================================================

typedef struct __attribute__((__packed__)) {
    double min;
} MinData, *PMinData;

#define MAX_STAT_VALUE 1e51

//----------------------
//
//
//
static void * min_insert(PRecord record, void *data, PContentData pcd) {
    PMinData p = (PMinData) data;

    __int64_t v = get_record_value(record, pcd, 0);

//    printf("INSERT SUM(before): %f  value: %d  offs: %d", p->sum, v, offs);
    if (!p->min || v < p->min) p->min = v;
    return NULL;
}

//----------------------
//
//
//
static void * min_remove(PRecord record, void *data, PContentData pcd) {
    PMinData p = (PMinData) data;

    __int64_t v = get_record_value(record, pcd, 0);

//    printf("INSERT SUM(before): %f  value: %d  offs: %d", p->sum, v, offs);
    if (v <= p->min) p->min = MAX_STAT_VALUE;
    return NULL;
}


//----------------------
//
//
//
static void min_merge(PByte merge_data, void *data, PContentData pcd) {
    PMinData m = (PMinData) merge_data;
    PMinData d = (PMinData) data;

    //printf(" BEFORE MERGE OF %f with %f %f with %f", m->sum2, p->sum2, m->sum, p->sum);
    if (d->min < m->min) m->min = d->min;
}

//----------------------
//
//
//
static void min_result(PByte merge_data, PByte result_data) {
    PMinData m = (PMinData) merge_data;
    PMinData r = (PMinData) result_data;

    //printf(" BEFORE MERGE OF %f with %f %f with %f", m->sum2, p->sum2, m->sum, p->sum);
    r->min = m->min;
}

//----------------------
//
//
//
static void min_register(void) {
    // SUM
    static ContentFunctions cfs;
    static ContentInfo ci;

    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 1;
    ci.min_params = 0;
    ci.max_params = 0;
    ci.params_size = 0;
    ci.private_size = sizeof(MinData);
    ci.merge_size = ci.private_size;
    ci.result_size = sizeof(double);
    ci.result_is_int = 0;
    ci.pcfs = &cfs;

    cfs.parse_param = NULL;
    cfs.insert = &min_insert;
    cfs.remove = &min_remove;
    cfs.merge = &min_merge;
    cfs.result = &min_result;
    register_content("min", &ci);
}


//=============================================================================================
//
//    
//
//=============================================================================================

typedef struct __attribute__((__packed__)) {
    double max;
} MaxData, *PMaxData;

#define MIN_STAT_VALUE -1e51

//----------------------
//
//
//
static void * max_insert(PRecord record, void *data, PContentData pcd) {
    PMaxData p = (PMaxData) data;

    __int64_t v = get_record_value(record, pcd, 0);

//    printf("INSERT SUM(before): %f  value: %d  offs: %d", p->sum, v, offs);
    if (v > p->max) p->max = v;
    return NULL;
}

//----------------------
//
//
//
static void * max_remove(PRecord record, void *data, PContentData pcd) {
    PMaxData p = (PMaxData) data;

    __int64_t v = get_record_value(record, pcd, 0);

//    printf("INSERT SUM(before): %f  value: %d  offs: %d", p->sum, v, offs);
    if (v >= p->max) p->max = MIN_STAT_VALUE;
    return NULL;
}


//----------------------
//
//
//
static void max_merge(PByte merge_data, void *data, PContentData pcd) {
    PMaxData m = (PMaxData) merge_data;
    PMaxData p = (PMaxData) data;

    //printf(" BEFORE MERGE OF %f with %f %f with %f", m->sum2, p->sum2, m->sum, p->sum);
    if (p->max > m->max) m->max = p->max;
}

//----------------------
//
//
//
static void max_result(PByte merge_data, PByte result_data) {
    PMaxData m = (PMaxData) merge_data;
    PMaxData r = (PMaxData) result_data;

    //printf(" BEFORE MERGE OF %f with %f %f with %f", m->sum2, p->sum2, m->sum, p->sum);
    r->max = m->max;
}


//----------------------
//
//
//
static void max_register(void) {
    // SUM
    static ContentFunctions cfs;
    static ContentInfo ci;

    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 1;
    ci.max_params = 0;
    ci.max_params = 0;
    ci.params_size = 0;
    ci.private_size = sizeof(MaxData);
    ci.merge_size = ci.private_size;
    ci.result_size = sizeof(double);
    ci.result_is_int = 0;
    ci.pcfs = &cfs;

    cfs.parse_param = NULL;
    cfs.insert = &max_insert;
    cfs.remove = &max_remove;
    cfs.merge = &max_merge;
    cfs.result = &max_result;
    register_content("max", &ci);
}


//=============================================================================================
//
//    
//
//=============================================================================================
void init_contents_stats(void) {

    // prevention against reinitialization during execution
    static int done = 0; 
    if (!done) done = 1; else return;

    counter16_register();
    counter_register();
    sum_register();
    avg_register();
    avg2_register(); // <---
    var_register();
    stdev_register();
    min_register();
    max_register();
};

