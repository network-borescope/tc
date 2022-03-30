/* 
 * File:   schema.h
 * Author: nilson
 *
 * Created on August 27, 2018, 2:45 PM
 */

#ifndef SCHEMA_H
#define SCHEMA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "opaque.h"
   
//***************************************************************************
//
//    Constants
//
//***************************************************************************

#define TRUE        1
#define FALSE       0
    
#define PROPER      0
#define SHARED      1

#define CONTENT     0    
#define CHILD       1

#define NON_BETA    0
#define BETA        1

#define OP_NONE     0
#define OP_NEW      1
#define OP_DESHARE  2

    
//***************************************************************************
//
//    Base Types
//
//***************************************************************************
    
typedef int   Value;
typedef Value *PAddress;
typedef Value *Values;

typedef void *PObject;

//***************************************************************************
//
//    Structural Types
//
//***************************************************************************
typedef void *PNode;
typedef unsigned char Byte;
typedef Byte *PByte;
typedef PByte PRecord;

typedef struct s_branch {
	void *prev;
	void *p;
    struct s_node_api *  pf_child_content;
} Branch;


//***************************************************************************
//
//    Header: Common Data for Nodes and Terminals (like a SuperClass)
//
//***************************************************************************

#define HEADER_FIELD_DELETED        1
#define HEADER_FIELD_BETA           1
#define HEADER_FIELD_PROPER_CHILD   1
#define HEADER_FIELD_TERMINAL       1
#define HEADER_FIELD_GHOST          1
#define HEADER_FIELD_TYPE           3

#define HEADER_FIELD_N_USERBITS     4
#define HEADER_FIELD_USERBIT        1
#define HEADER_FIELD_COUNTER       20

#define HEADER_TOTAL          ( \
    HEADER_FIELD_DELETED + HEADER_FIELD_BETA + \
    HEADER_FIELD_PROPER_CHILD + HEADER_FIELD_TERMINAL + \
    HEADER_FIELD_GHOST + \
    HEADER_FIELD_N_USERBITS * HEADER_FIELD_USERBIT +  \
    HEADER_FIELD_TYPE  +  HEADER_FIELD_COUNTER)

#if (HEADER_TOTAL != 32)
#error "Invalid HEADER  Size"
#endif

#define HEADER_INFO                                                \
    struct {                                                       \
        unsigned int deleted:          HEADER_FIELD_DELETED;       \
        unsigned int beta:             HEADER_FIELD_BETA;          \
        unsigned int proper_child:     HEADER_FIELD_PROPER_CHILD;  \
        unsigned int terminal:         HEADER_FIELD_TERMINAL;      \
        unsigned int ghost:            HEADER_FIELD_GHOST;         \
        unsigned int type:             HEADER_FIELD_TYPE;          \
        unsigned int refcount:         HEADER_FIELD_COUNTER;       \
        unsigned int shared_content:   HEADER_FIELD_USERBIT;       \
        unsigned int beta_content:     HEADER_FIELD_USERBIT;       \
        unsigned int userbit1:         HEADER_FIELD_USERBIT;       \
        unsigned int userbit2:         HEADER_FIELD_USERBIT;       \
    } hdr


typedef struct {
    HEADER_INFO;
} Header, *PHeader;

int header_init (PNode pn, int beta, int type);
int header_adjust (PNode pn, int child, int shared, int inc);
int header_make_proper_child (PNode pn);
int header_deleted (PNode pn, int set);

#define HEADER_COUNT(PTR)               (((PHeader) (PTR))->hdr.refcount )
#define HEADER_GET_PROPER_CHILD(PTR)    (((PHeader) (PTR))->hdr.proper_child)
#define HEADER_GET_TERMINAL(PTR)        (((PHeader) (PTR))->hdr.terminal)
#define HEADER_SET_TERMINAL(PTR,V)      ((PHeader) (PTR))->hdr.terminal = (V)
#define HEADER_GET_BETA(PTR)            (((PHeader) (PTR))->hdr.beta)
#define HEADER_GET_DELETED(PTR)         (((PHeader) (PTR))->hdr.deleted)

#define HEADER_GET_GHOST(PTR)        (((PHeader) (PTR))->hdr.ghost)
#define HEADER_SET_GHOST(PTR,V)      ((PHeader) (PTR))->hdr.ghost = (V)

//===================================[ Destructor ]============================================
typedef void (*header_destructor) (void *);

int header_register_type(header_destructor destructor);


//***************************************************************************
//
//    MANDATORY NODE FUNCTIONS
//
//***************************************************************************

struct s_node_api;

typedef PNode(*f_create_node) 
	(int beta);

typedef int (*f_delete_node) 
	(PNode pn);

typedef PNode (*f_get_content_info)
    (PNode pn, int *shared);

typedef PNode(*f_set_content_info)
    (PNode pn, PNode pcontent, int beta, int shared, int ignore); //, struct s_node_api *pf);

typedef PNode (*f_shallow_copy)
	(PNode pn, int beta, struct s_node_api *pf);

#define MANDATORY_NODE_FUNCTIONS                                \
    f_create_node               create_node;                 \
	f_delete_node               delete_node;                 \
    f_set_content_info          set_content_info;            \
    f_get_content_info          get_content_info;            \
    f_shallow_copy              shallow_copy;                


//***************************************************************************
//
//    Handling Functions: FOR USUAL NODES
//
//***************************************************************************

typedef PNode (*f_share_child_content) 
	(PNode pn);

// Child retrive
typedef PNode (*f_locate_child)
	(PNode pn, Value v, int *shared, Branch *pb, struct s_node_api *pf);

typedef PNode(*f_get_child) 
	(PNode pn, Value *v, int *shared, Branch *ref, int change_proper, struct s_node_api *pf);

// Child edge handling 
typedef PNode (*f_add_child_edge) 
	(PNode pn, PNode pnc, int shared, Value v);

typedef PNode (*f_remove_child_edge) 
	(PNode pn, Branch *pb, int *shared);

typedef PNode (*f_replace_child_on_branch)
	(Branch *branch, PNode pchild, int shared, struct s_node_api *pf);

// Informational functions
typedef int   (*f_get_children_class)
	(PNode pn);

typedef int  (*f_has_single_child)
    (PNode pn);

typedef struct s_node_api {

    MANDATORY_NODE_FUNCTIONS

    f_locate_child              locate_child;
    f_get_child                 get_child;
	
    f_add_child_edge            add_child_edge;
    f_remove_child_edge         remove_child_edge;
            
    f_replace_child_on_branch   replace_child_on_branch;

    f_has_single_child          has_single_child;
	f_get_children_class        get_children_class;
    
} NodeFunctions, *PNodeFunctions;

//***************************************************************************
//
//    Handling Functions: FOR TERMINALS
//
//***************************************************************************

typedef struct s_terminal_api {
    MANDATORY_NODE_FUNCTIONS
} TerminalNodeFunctions, *PTerminalNodeFunctions;


//***************************************************************************
//
//    Terminal
//
//***************************************************************************

#define TERMINAL_OP_INIT 1
#define TERMINAL_OP_TERM 2
#define TERMINAL_OP_INSERT 3
#define TERMINAL_OP_REMOVE 4

#define MAX_RECORD_FIELDS_USED 10
#define PARAM_BUFFER_SZ       128

//*****************************************
//    CONTENT DATA
//*****************************************
typedef struct {
    int           total_data_size;
    int           bin_size;
    int           record_offsets[MAX_RECORD_FIELDS_USED]; 
    int           record_type[MAX_RECORD_FIELDS_USED]; 
    Byte          params[PARAM_BUFFER_SZ];  // to be used by class
} ContentData, *PContentData;

//*****************************************
//    CONTENT FUNCTIONS
//*****************************************
#define CONTENT_MERGE_SZ      1
#define CONTENT_RESULT_SZ     2

typedef void  (*content_parse_t)      (int id_param,      char *s,        PByte params);
typedef void  (*content_prepare_t)    (int terminal_op,   void *content,  PContentData pcd);
typedef void *(*content_alter_t)      (PRecord pr,        void  *content, PContentData pcd);
typedef void  (*content_merge_t)      (PByte merge_data,  void  *content, PContentData pcd);
typedef void  (*content_result_t)     (PByte merge_data,  PByte result_data);
typedef int   (*content_afterall_t)   (PByte result_data, int n);

typedef void  (*content_shallow_copy_t) (PByte dst, PByte src, int n_bytes);
typedef void  (*content_to_string_t) (void  *content, PContentData pcd);
 
typedef int   (*content_inquire_t)  (int what);

typedef int   (*content_find_bounds_t)  (PByte pcd, int *bounds);

typedef struct s_content_functions {
    content_shallow_copy_t shallow_copy;
    content_to_string_t    to_string;

    content_prepare_t  prepare; 
    content_alter_t    insert;
    content_alter_t    remove;

    content_parse_t    parse_param;
    
    content_merge_t    merge;
    content_result_t   result;

    content_find_bounds_t find_bounds;

    content_afterall_t after_all;
} ContentFunctions;

//*****************************************
//    CONTENT INFO
//*****************************************
typedef struct s_content_info {
    int                  n_param_fields;
    int                  min_params;
    int                  max_params;
    int                  private_size;
    int                  params_size;
    char                 result_is_int;

    int                  merge_size;
    int                  result_size;

    PClass               pclass;

    PContentFunctions    pcfs;
} ContentInfo;

//*****************************************
//    TERMINAL CHANNEL
//*****************************************
typedef struct s_terminal_channel {
    int                 data_size;
    int                 data_offset;
    PContentFunctions   pcfs;
    ContentData         tfd;

    int                 merge_size;
    int                 result_size;
    int                 result_is_int;
    
    // group
    PClass              pclass;
    int                 is_container;
    int                 n_channels;
    int                 base_slot;

    // subchannel
    int                 is_subchannel;
    int                 id_parent_slot;

} TerminalChannel;

//*****************************************
//    MD TERMINAL CHANNEL
//*****************************************
typedef struct s_md_terminal_channel {
    char                    *name;
    PMetadataTerminalChannel     next;

    PMetadataTerminalChannel     first;
    PMetadataTerminalChannel     last;

    TerminalChannel         tc;

} MD_TerminalChannel;


//*****************************************
//    TERMINAL INFO
//*****************************************
typedef struct {
    PMetadataTerminalChannel first; 
    PMetadataTerminalChannel last; 

    int total_data_size; 
    int n_channels;
    int n_slots;

    PTerminalChannel *p_slots;

} TerminalInfo, *PTerminalInfo;


//***************************************************************************
//
//    Terminal
//
//***************************************************************************
typedef struct s_terminal {
    Header rc;    
    unsigned int   counter;
    unsigned char  data[];
} Terminal, *PTerminal;

//***************************************************************************
//
//    Tree
//
//***************************************************************************

typedef struct s_tree {
    int    op;
    int    max;  // __EXPERIMENTAL__
    PNode  *nodes;
} Tree, *PTree;


//***************************************************************************
//
//    Schema
//
//***************************************************************************

typedef struct s_schema {
    int             n_dims;
    int             n_values;

    int             *n_values_of_dim;
    int             *top_of_dim;
    int             *level_of_value;
    int             *dim_of_value;
    int             *content_of_value;

    NodeFunctions   *content_nfs;
    
    NodeFunctions     *nfs;

    PTerminalInfo     pti;

    PTree             trees;

    PNode             root; // just for convinience                 

} Schema, *PSchema;

//==================================================================================

PSchema schema_begin(void);
void    schema_end(PSchema ps, PTerminalNodeFunctions pcfs);
void    schema_release_all(PSchema ps);

void    schema_new_level(PSchema ps);
void    schema_new_value(PSchema ps, int n, PNodeFunctions pnfs);

Values schema_create_address(PSchema ps);
void schema_release_address(Values values);

PTree  schema_get_tree(PSchema ps, int level);
int    schema_compute_sum(PSchema ps, int idx, PNode pn, int *err);

PNode schema_create_root(PSchema ps);
void  schema_release_root(PNode root);

#define SCHEMA_IDX_CONTENT(PS, IDX) ((PS)->top_of_dim[(PS)->dim_of_value[IDX] + 1])

//==================================================================================

void  tinycubes_insert(PSchema ps, PNode root, PAddress values, PObject o);
PNode tinycubes_remove(PSchema ps, PNode root, PAddress values, PObject o);

//==================================================================================

#include "query.h"

#define MAX_RESULTS 10

typedef struct s_call_info {
    void*       op;
    void*       op_group_by;

    int         value_index;
    QArgs       qargs;
    void        *op_data;

    void        *params;  

    int         call_group_by;
    int         group_by_idx;
} QCallInfo, *PQCallInfo;

#define MAX_VALUES            100
#define MAX_GROUP_BY_INDEXES  10

#define CONTENT_CLASS_SELECT  0x01
#define CONTENT_CLASS_WHERE   0x02
#define CONTENT_CLASS_GROUPBY 0x04

typedef struct s_query {

    int    schema_query;
    char * json_src;
    
    int id;  // id fornecido pelo chamador
    
    //------------------------------
    // invariant during execution
    //------------------------------

    // Schema
    PSchema    ps;

    // navigation 
    PQCallInfo  ppci[MAX_VALUES];

    // current index
    int cur_index;

    // shortcut to ci[cur_index]
    PQCallInfo   pci;

    //------------------------------
    // CONTENT CLASS
    //------------------------------
    int         use_content_class;
    int         id_container;

    //------------------------------
    // CHANNELS_MERGE & CHANNELS_RESULT
    //------------------------------
#define QUERY_MAX_CHANNELS 30    
    int         n_channels;

    int         query_slots[QUERY_MAX_CHANNELS];

    int         merge_offsets[QUERY_MAX_CHANNELS];
    int         channels_merge_size;

    int         result_offset[QUERY_MAX_CHANNELS];
    int         channels_result_size;

    int         result_is_int[QUERY_MAX_CHANNELS];

    //------------------------------
    // Bounds
    //------------------------------
#define QUERY_NO_BOUNDS        0
#define QUERY_VALUE_BOUNDS     1
#define QUERY_CONTAINER_BOUNDS 2
    int                 bounds_type;
    int                 bounds_idx;

    //------------------------------
    // Group By
    //------------------------------
#define QUERY_MAX_GROUP_BYS         20    
#define QUERY_MAX_GROUP_BY_INDEXES  ( 2 * QUERY_MAX_GROUP_BYS) 
#define GROUP_BY_DISTINCT_COUNT     86400
    int         n_group_bys;        // number os variables
    int         n_group_by_indexes; // total de indices utilizado

    // may depend on the query
    // used during preparation
    int         group_by_distinct_count[QUERY_MAX_GROUP_BY_INDEXES];

    // precompute the multiplication factor
    // possibly accelerates the indexing process during execution
    int         group_by_inner_amount[QUERY_MAX_GROUP_BY_INDEXES]; // 

    // total amount of elements 
    int         group_by_total_amount; // 
    
    // may depend on the query
    // used during execution
    int         group_by_distinct_base[QUERY_MAX_GROUP_BY_INDEXES];
    
    // Bin size of container
    int         group_by_bin_size[QUERY_MAX_GROUP_BY_INDEXES];
    
    // n_points of container
    int         group_by_n_points[QUERY_MAX_GROUP_BY_INDEXES];
    
    // runtime indexes
    // used during execution
    int         group_by_indexes[QUERY_MAX_GROUP_BY_INDEXES];

    //------------------------------
    // ML.NET
    //------------------------------
    int ml_on;
    int ml_idx_method;
    int ml_idx_slot;
    int ml_position;
    char *ml_method;
    char *ml_parameters;
    int  ml_offset;
    int  ml_len;

    //------------------------------
    // MERGE MEMORY
    //------------------------------
    int         total_merge_size;
    PByte       merge_memory;

    //------------------------------
    // RESULT MEMORY
    //------------------------------
    int         total_result_size;
    PByte       result_memory;

    //------------------------------
    // layout
    //------------------------------

#define GROUP_BY_OUTPUT_KS_VS        0
#define GROUP_BY_OUTPUT_VS_KS       10
#define GROUP_BY_OUTPUT_K_LISTS     20
#define GROUP_BY_OUTPUT_V_LISTS     30
#define GROUP_BY_OUTPUT_FULL_LISTS  40

    int         group_by_output;

} Query;

#define MAX_QUERY_ERROR_SZ 8192
extern char query_error_string[MAX_QUERY_ERROR_SZ];

typedef struct s_qdata {
//    int            index;
//    PQCallInfo     pci;
//    PNode          pn;
//    Branch         b;
//    f_get_child    get_child;
    int dummy;
} QData;

typedef struct { 
    void * opaque; 
} *PQProcData;

typedef struct { 
    void * opaque; 
} *PQStep;

#ifdef __cplusplus
}
#endif

#endif /* SCHEMA_H */

