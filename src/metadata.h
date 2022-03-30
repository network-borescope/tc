#ifndef __METADATA
#define __METADATA

#include <stdio.h>

#include "others/map.h"
#include "others/cJSON.h"

#include "opaque.h"
#include "schema.h"

#define MD_MAX_IDS               4

//*****************************************
//    TYPES
//*****************************************
#define MD_TYPE_STRING    1

#define MD_TYPE_FLOAT32   2
#define MD_TYPE_FLOAT64   3

#define MD_TYPE_INT08     4
#define MD_TYPE_INT16     5
#define MD_TYPE_INT32     6
#define MD_TYPE_INT64     7

#define MD_TYPE_UINT08    8
#define MD_TYPE_UINT16    9
#define MD_TYPE_UINT32   10
#define MD_TYPE_UINT64   11


//*****************************************
//    INPUT
//*****************************************
#define MD_INPUT_MAX_NAME   300
#define MD_INPUT_MAX_STRING 300
#define MD_INPUT_MAX_IDS     20

typedef void (*conv_to_field_t) (
    char inputs[MD_INPUT_MAX_IDS][MD_INPUT_MAX_STRING], void *params, int *id_inputs, void *result);

#define INT_MAP_INPUT     1
#define DOUBLE_MAP_INPUT  2

typedef struct s_tf {

    // field name
    char name[MD_INPUT_MAX_NAME+1];

    // don't convert this datum
    int  do_not_convert;

    // set_value
    char *set_value;

    // default value
    char *default_value;

    // position of this field in input 
    int                 input_id;
    
    // identifies the record field associated to this input 
    int                 id_record;

    // conversion from input uses defaullt type based functions
    int                 default_input_to_field;

    // points to other input to field proc
    conv_to_field_t     conv_to_field;

    // identifies a field of input used by conversion proc
    int                 inputs_ids[MD_MAX_IDS];

    // store value of additional parameters
    unsigned char       input_params[PARAM_BUFFER_SZ];

    // use map
    int                 use_map_input; // 1 - Int, 2 - Double
    
    // map string -> int
    map_t(int)          map_input_int;

    // map string -> double
    map_t(double)       map_input_double;

    // use ranges
    int                 use_ranges;

    // ranges
    int                 range_op;

} TargetField, *PTargetField;

#define MAX_TARGETS     100

typedef struct s_if{

    // position of this field in input 
    int                 input_id;

    // ignore this datum
    int  ignore;

    //  targets
    int                 n_targets;
    TargetField         targets[MAX_TARGETS];
#if 0
    char name[MD_INPUT_MAX_NAME+1];

    // identifies the record field associated to this input 
    int                 id_record;

    // conversion from input uses defaullt type based functions
    int                 default_input_to_field;

    // points to other input to field proc
    conv_to_field_t     conv_to_field;

    // identifies a field of input used by conversion proc
    int                 inputs_ids[MD_MAX_IDS];

    // store value of additional parameters
    unsigned char       input_params[PARAM_BUFFER_SZ];

    // use map
    int                 use_map_input; // 1 - Int, 2 - Double
    
    // map string -> int
    map_t(int)          map_input_int;

    // map string -> double
    map_t(double)          map_input_double;

    // use ranges
    int                 use_ranges;

    // ranges
    int                 range_op;

    // set_value
    char *set_value;
#endif

    struct s_if *next;
} InputField, *PInputField;

#define XDATA_CSV  0
#define XDATA_TEST 1

typedef struct {
    int  type;
    char sep[3];
    
    PInputField first, last;
} InputInfo, *PInputInfo;

//*****************************************
//    RECORD FIELD
//*****************************************
typedef struct s_record_field {
    // offset of this field 
    int                 record_offset;

    // name of this field
    char *              name;

    // position of this field in record
    int                 field_id;

    // type of record field (also )
    int                 type;

    // number of bytes actually required in record
    int                 type_size;

    // aliases
    int                 n_aliases;
    int                 min_aliases;
    char **             v_aliases;

} RecordField;

//*****************************************
//    RECORD INFO
//*****************************************

#define MAX_RECORD_FIELDS 20

typedef struct s_record_info {
    int n_fields;
    int record_sz;
    RecordField fields[MAX_RECORD_FIELDS];
} RecordInfo;

//*****************************************
//    RECORD FIELD TO ADDRESS
//*****************************************

#define MD_OK             0
#define MD_ERR_N_PARAMS   1

#define MD_ERR_PARAM_1    2
#define MD_ERR_PARAM_2    3
#define MD_ERR_PARAM_3    4
#define MD_ERR_PARAM_4    5

#define MD_ERR_FIELD_1    6
#define MD_ERR_FIELD_2    7
#define MD_ERR_FIELD_3    8
#define MD_ERR_FIELD_4    9

typedef void (* to_address_t)
    (PByte record, PRecordField prf, void *params, int *id_fields, int n_values, int *values);

typedef struct {
    int             n_members;
    to_address_t    record_to_address;
    int             id_fields[MD_MAX_IDS];
//    Byte            paramsx[PARAM_BUFFER_SZ];
} RecToAddrEntry, *PRecToAddrEntry;        

typedef struct {
    int             n_entries;
    RecToAddrEntry   entries[100];
} ToAddresses;


//*****************************************
//    CLASS
//*****************************************

typedef int    (*get_distinct_info_t)   (void *op, PQArgs pargs, PByte params, int bin_size, int *distinct_count, int *distinct_base);

typedef void * (*create_op_data_t)      (void *op, PQArgs pargs);
typedef void   (*destroy_op_data_t)     (void *op, void *op_data);

#if CLASS_PARSE
typedef void   (*class_parse_t)        (int id_param,   char *s, PByte params);
#endif

#define DIMENSIONAL_CLASS  10
#define LEVEL_CLASS        20
#define CONTAINER_CLASS    30

typedef struct s_class {
    int             class_type;
    char            * name;
    int             n_group_by_indexes;
    int             group_by_requires_rule;

    int             n_fields;
    int             n_params;

    query_op_t      op_all_plain;
    query_og_t      op_all_group_by;

    // constructor function
    to_address_t    to_address;
    char            *to_address_fmt;
    char            *to_address_types;

    // query handling functions
    create_op_data_t      create_op_data;
    destroy_op_data_t     destroy_op_data;

    get_distinct_info_t   get_distinct_info;

#if CLASS_PARSE
    class_parse_t         parse_param;
#endif    
} Class;

//*****************************************
//    ADDRESS VALUE
//*****************************************

typedef struct s_address_value {
    char            * name;
    int             ids[MD_MAX_IDS];
    char            val_params[PARAM_BUFFER_SZ];
    to_address_t    record_to_address;

    PClass          pclass;

    PAddressValue   next;
} AddressValue;

//*****************************************
//    ADDRESS DIMENSION
//*****************************************

typedef struct s_address_dim * PAddressDim; 

typedef struct s_address_dim {
    char                * name;
    int                 n_values;
    
    int                 ids[MD_MAX_IDS];
    char                dim_params[PARAM_BUFFER_SZ];
    
    to_address_t        record_to_address;
    PClass              pclass;

    PAddressValue       first;
    PAddressValue       last;
    
    PAddressDim   next;
} AddressDim;

//*****************************************
//    ADDRESS INFO
//*****************************************
#define MD_MAX_LEVELS       200
#define MD_MAX_ADDRESS_SIZE 200

typedef struct s_address_info *PAddressInfo;

typedef struct s_address_info {
    int             n_values;
    int             n_dimensions;

    PAddressDim     first;
    PAddressDim     last;

    PAddressDim     *p_dimensions;
    PAddressValue   *p_values;
} AddressInfo;

//*****************************************
//    ADDRESS 
//*****************************************

typedef struct {
    int n_values;
    int values[];
} Addr, *PAddr;

//*****************************************
//    MetaData
//*****************************************

typedef map_str_t Registry;
typedef Registry *PRegistry;

typedef struct {
    FILE           *driver_fin;
    
//    Registry       registry;

    InputInfo      input_info;
    RecordInfo     record_info;
    AddressInfo    address_info;    
    TerminalInfo   terminal_info;

    PInputInfo     pii;
    PRecordInfo    pri;
    PAddressInfo   pai;
    PTerminalInfo  pti;

    char *json_src;
} MetaData, *PMetaData;

PMetaData metadata_create(void);
PMetaData metadata_release(PMetaData pmd);

int metadata_load(PMetaData pmd, char *fname);

PSchema metadata_create_schema(PMetaData pmd);
void metadata_release_schema(PSchema ps);

void metadata_open_input_file(PMetaData pmd, char *fname);
void metadata_close_input_file(PMetaData pmd);

int  metadata_process_line(PMetaData pmd, void *record, char *line, int no_crlf);
int  metadata_read(PMetaData pmd, void *record);

void * metadata_create_record(PMetaData pmd);
void * metadata_release_record(void *pr);

void metadata_dump_record(PMetaData pmd, void *record);

PAddr metadata_create_address(PMetaData pmd);
PAddr metadata_release_address(PAddr pa);

int   metadata_record_to_address(PMetaData pmd, void * record, PAddr pa);

__int64_t get_record_value(PRecord record, PContentData pcd, int idx);

#endif