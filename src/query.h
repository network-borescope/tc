#ifndef __QUERY__
#define __QUERY__


#define MAX_ARGS    101

typedef struct s_query  * PQuery;
typedef struct s_args   * PQArgs;
typedef void            * PQOpData;

typedef union i_d {
    int     i;
    double  d;
    void   *p;
} UArgs;

#define         MAX_SYS_ARGS  4

typedef struct s_args {
    int         inquire_size;
    UArgs       sysargs[MAX_SYS_ARGS];

    int         index_reduce;
    int         index_count;
    int         index_n_points;
    
    int         n_args;
    UArgs       args[MAX_ARGS];
} QArgs; 


#define QUERY_FALSE        0x0000
#define QUERY_TRUE         0x0001
#define QUERY_OK           QUERY_TRUE
#define QUERY_FAIL         QUERY_FALSE

#define QUERY_BIT_TRUE    QUERY_TRUE
#define QUERY_BIT_MORE    0x0002

#define QUERY_DIM_DONE    0x8000

// typedef void (* query_op_t) (PQuery query, PQArgs args, PQOpData op_data);

typedef int  (*query_op_t) (int v, int argset, PQArgs args, int level, PQOpData op_data);
typedef int  (*query_og_t) (int v, int argset, PQArgs args, int *indexes, int level, PQOpData op_data);

typedef void * (*query_container_op_t) (PQArgs pa, PQOpData op_data, void **state);
typedef void * (*query_container_og_t) (PQArgs pa, PQOpData op_data, int *indexes, void **state);

typedef struct {
    void *  op;
    void *  op_group_by;
    
    int         min_args;
    int         max_args;
    char        * fmt; // D I S
} QOpInfo, * PQOpInfo;


#endif