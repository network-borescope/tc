#include "others/map.h"
#include "common.h"
#include "query.h"
#include "metadata.h"
#include "schema.h"

//=============================================================================================
//
//    
//
//=============================================================================================

typedef map_t(PClass)   map_pclass;
typedef map_t(PQOpInfo) map_op;

static map_int_t    map_fields;
static map_int_t    map_values;
static map_int_t    map_channels;
static map_int_t    map_slots;
static map_str_t    registry;
static map_pclass   map_class;
static map_op       map_ops;
static map_t(PContentInfo) map_content;

void init_register(void) {
    map_init(&map_fields);
    map_init(&map_values);
    map_init(&map_channels);
    map_init(&map_slots);
    map_init(&registry);
    map_init(&map_class);
    map_init(&map_ops);
    map_init(&map_content);
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
void register_in_registry(char *key, char *value) {
    if (map_get(&registry,key)) {
        fatal("Redefinition of key '%s'", key); 
    }
    
    map_set(&registry, key, value);
}

//-----------------------
//
//
//
//
int register_locate_in_registry(char *key, char *path, char **value) {
    static char buf[100];

    char *sep;
    if (path && *path) sep = "/"; else sep = "";

    snprintf(buf, 99, "%s%s%s",path,sep,key);
    char **p = map_get(&registry, buf);
    if (!p) {
        fatal("Could not find '%s' in registry",buf);
    }
    *value = *p;

    return 1;
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
void register_slot(char *slot_name, int id_slot) {
    if (map_get(&map_slots,slot_name)) {
        fatal("Redefinition of slot '%s'", slot_name); 
    }
    
    map_set(&map_slots, slot_name, id_slot);
}

//-----------------------
//
//
//
//
int register_locate_slot(char *slot_name, int *id_slot) {
    int *p = map_get(&map_slots,slot_name);
    if (!p) return 0;
    *id_slot = *p;
    return 1;
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
void register_value(char *value_name, int value) {
    map_int_t * map = &map_values; 

    int *v = map_get(map, value_name);
    if (!v) {
        // printf("Register Value %s: %d\n",value_name, value);
        map_set(map, value_name, value);
    } else if (*v != value) {
        fatal ("Trying to reuse name of address value"); 
    }
}

/**
 * 
 * 
 * 
 */ 
_PUBLIC(int) register_locate_value(char * value_name, int *id_value) {
    int *p = map_get(&map_values, value_name);
    if (!p) return 0;
    // printf("Locate Value: '%s' = %d\n", value_name, *id_value);
    *id_value = *p;
    return 1;
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
void register_field(char *value_name, int value) {
    map_int_t * map = &map_fields; 

    int *v = map_get(map, value_name);
    if (!v) {
        map_set(map, value_name, value);
    } else if (*v != value) {
        fatal ("Trying to reuse name of address value"); 
    }
}

//-----------------------
//
//
//
//
int register_locate_field(char *name, int *id_field) {
    int *p = map_get(&map_fields,name);
    if (!p) fatal("Field '%s'  not found", name);
    *id_field = *p;
    return 1;
}            
//=============================================================================================
//
//    
//
//=============================================================================================


/*--------------------------
*
*
*
*/
void register_class(PClass pclass) {
    if (map_get(&map_class, pclass->name)) {
        fatal("CLASS: name '%' reused ",pclass->name);
    }

    map_set(&map_class, pclass->name, pclass);
}

/*--------------------------
*
*
*
*/

int register_locate_class(char *name, PClass *pclass) {
    PClass *p = map_get(&map_class, name);
    *pclass = *p;
    return p != NULL;
}


//=============================================================================================
//
//    
//
//=============================================================================================

#define MAX_REGISTER_STRING 128

/*-----------------
*
*
*
*/
void register_op(char *classname, char *opstr, PQOpInfo poi) {

    char buf[MAX_REGISTER_STRING+1];
    strncpy(buf,classname,MAX_REGISTER_STRING);
    strncat(buf,"::",MAX_REGISTER_STRING);
    strncat(buf,opstr,MAX_REGISTER_STRING);

    if (map_get(&map_ops, buf)) {
        fatal("already registered");   
    }

    map_set(&map_ops, buf, poi);
}

/*-----------------
*
*
*
*/
int register_locate_op(char *classname, char *opstr, PQOpInfo *poi) {
    char buf[MAX_REGISTER_STRING+1];
    strncpy(buf,classname,MAX_REGISTER_STRING);
    strncat(buf,"::",MAX_REGISTER_STRING);
    strncat(buf,opstr,MAX_REGISTER_STRING);

    PQOpInfo * p = map_get(&map_ops, buf);
    if (!p) fatal("operator '%s' not found", opstr);
    *poi = *p;
    return 1;
}


//***************************************************************************
//
//    Content
//
//***************************************************************************

#include "others/map.h"


#include <stdio.h>

void *missing_alter(PByte record, void *data, PContentData pcd) {
    printf("missing INSERT/REMOVE function\n");
    return NULL;
}

//-----------------------
//
//
//
//
void register_content(char *name, PContentInfo pci) {


    if (map_get(&map_content, name)) {
        fatal("Content '%s' already registered", name);
    }

#if 0
    // printf("REGISTER TERMINAL FUNCTION: %s\n",name);
    if (!pci->pcfs->insert) {   
        pci->pcfs->insert = missing_alter;
    }
#endif

    map_set(&map_content, name, pci);
}

//-----------------------
//
//
//
//
int register_locate_content(char *name, PContentInfo *pci) {
    PContentInfo *p = map_get(&map_content, name);

    if (!p) fatal(" funcao '%s' desconhecida", name);
    *pci = *p;

    return 1;
}
