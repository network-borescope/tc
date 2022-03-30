#include "metadata.h"
#include "register.h"
#include "query.h"

typedef struct {
//    query_op_t      op;
    query_op_t      op;
    query_og_t      op_group_by;
    int             min_args;
    int             max_args;    
} QueryOpInfo;

#define IS_GRID(A) ((A)->n_args )

/*--------------------------
*
*
*
*/
static int op_all(int v, int argset, PQArgs pargs, int level, PQOpData op_data) {
    return QUERY_TRUE;
}


/*--------------------------
*
*
*
*/
static int cat_op_all_group_by(int v, int argset, PQArgs pargs, int *indexes, int level, PQOpData op_data ) {
    indexes[0] = v;
    return QUERY_TRUE;
}

/*-----------------
*
*
*
*/
static int op_eq(int v, int argset, PQArgs args, int level, PQOpData op_data) {
    int i; union i_d *vs;

    for (i = 0, vs = args->args; i < args->n_args; i++, vs++) {
        if (v == vs[0].i) {
            return QUERY_TRUE;
        }
    }
    return QUERY_FALSE;
}

/*-----------------
*
*
*
*/
static int op_eq_group_by(int v, int argset, PQArgs args, int *indexes, int level, PQOpData op_data) {
    int i; union i_d *vs;

    for (i = 0, vs = args->args; i < args->n_args; i++, vs++) {
        if (v == vs[0].i) {
            indexes[0] = v;
            return QUERY_TRUE;
        }
    }
    return QUERY_FALSE;
}


/*-----------------
*
*
*
*/
static void register_cat_ops(void) {
#define REGISTER_CAT_OP(X,S,Y)  \
    static QOpInfo qoi_##X;\
    qoi_##X.op = op_##X;\
    qoi_##X.op_group_by = op_##X##_group_by;\
    qoi_##X.min_args = Y;\
    qoi_##X.max_args = MAX_ARGS;\
    qoi_##X.fmt = "I";\
    register_op("cat", S, &qoi_##X)

    REGISTER_CAT_OP(eq,"eq",1);

    //REGISTER_CAT_OP(all,"all",0);
    //qoi_all.max_args = 0;
    
    //REGISTER_CAT_OP(neq,1);
    //REGISTER_CAT_OP(lt,"lt",1);
    //REGISTER_CAT_OP(le,1);
    //REGISTER_CAT_OP(gt,1);
    //REGISTER_CAT_OP(ge,1);
    //REGISTER_CAT_OP(inside,"inside",1);
    //REGISTER_CAT_OP(outside,"outside",2);

}

//=============================================================================================
//
//    CLASS Cat
//
//=============================================================================================

/*--------------------------
*
*
*
*/
static void to_address(PByte record, PRecordField prf, void *params, int *id_fields, int n_values, int *values) {
    values[0] = * (int *) (record + prf[id_fields[0]].record_offset);
 }

/*--------------------------
*
*
*
*/
static int get_distinct_info(void *op, PQArgs pa, PByte params, int bin_size, int *distinct_count, int *distinct_base) {
    distinct_count[0] = 255;
    distinct_base[0] = 0;
    return 1;
}


/*--------------------------
*
*
*
*/
void register_class_cat(void) {

    // prevention against reinitialization during execution
    static int done = 0; 
    if (!done) done = 1; else return;

    static Class c;
    c.name = "cat";
    c.class_type = LEVEL_CLASS;
    
    c.to_address = to_address;
    c.to_address_fmt = "F"; // Field
    c.to_address_types = "I"; // integer

    c.op_all_plain = op_all;
    c.op_all_group_by = cat_op_all_group_by;

    c.n_group_by_indexes = 1;
    c.group_by_requires_rule = 0;

    c.create_op_data = NULL;
    c.destroy_op_data = NULL;

//    c.merge_bounds = NULL;
//    c.result_bounds = NULL;

    c.get_distinct_info = &get_distinct_info;

    register_class(&c);
    register_cat_ops();
}


