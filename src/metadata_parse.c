#include <string.h>
#include <stdlib.h> 
#include <stdio.h>

#include "others/map.h"
#include "others/cJSON.h"

#include "common.h"
#include "metadata.h"
#include "register.h"

//=============================================================================================

static char *cur_fname;

/**
 * 
 * 
 */ 
_PRIVATE(void) missing(char *what, char *type, char *in) {
    fatal("Missing field '%s' of type %s in '%s' object. File: '%s'\n",what,type,in,cur_fname);
}


/**
 * 
 * 
 */ 
_PRIVATE(cJSON *) getStringByName(cJSON *src, char *name, char *err) {
    cJSON *str = cJSON_GetObjectItem(src, name);
    if (!str || !cJSON_IsString(str)) {
        fatal(err);
    }
    return str;
}


//=============================================================================================
//
//    PARSE SCHEMA FROM FILE
//
//=============================================================================================

/**
 * 
 * 
 * 
 */ 
_PRIVATE(int) load_textfile_to_memory(const char *filename, char **result) { 
	int size = 0;
	FILE *f = fopen(filename, "rt");
	if (f == NULL) 	{ 
		*result = NULL;
		return -1; // -1 means file opening fail 
	} 
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*result = (char *)malloc(size+1);
	if (size != fread(*result, sizeof(char), size, f)) 
	{ 
		free(*result);
		return -2; // -2 means file reading fail 
	} 
	fclose(f);
	(*result)[size] = 0;
	return size;
}


/**
 * 
 * 
 * 
 */ 
_PRIVATE(cJSON *) parse_schema_from_file(char *fname, PMetaData pmd) {

    //------------------------
    //   Load file to memory
    //------------------------
    char *src;
    if (load_textfile_to_memory(fname, &src) < 0) {
        fatal("Could not load json file '%s'.",fname);
    } 

    //------------------------
    // Parse JSON string
    //------------------------
    cJSON *schema = cJSON_Parse(src);

    pmd->json_src = src; 
    //free(src);

    if (!schema) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fatal("File: %s, Error before: %s\n", fname, error_ptr);
        }
    }
    return schema;
}

//=============================================================================================
//
//    PARSE REGISTRY
//
//=============================================================================================

/**
 * 
 * 
 * 
 */ 
_PRIVATE(int) parse_registry(cJSON *schema, PMetaData pmd) {

    // locate registry
    cJSON * registry = cJSON_GetObjectItem(schema, "registry");
    if (!registry) return 0;

    // check consistency
    if (!cJSON_IsObject(registry)) {
        fatal("Registry must be an object.");
    }

    //------------------------
    //   Registry Items
    //------------------------
    cJSON * kv;
    cJSON_ArrayForEach(kv, registry) {
        if (!cJSON_IsString(kv)) {
            fatal("Invalid map value");
        }
        char *s = strdup(kv->valuestring);

        register_in_registry(kv->string, s);
        // printf("KV: %s -> %s\n", kv->string, s);
    }

    return 1;
}

//=============================================================================================
//
//    PARSE RECORD / PARSE RECORD FIELD
//
//=============================================================================================

/**
 * 
 * 
 * 
 */ 
_PRIVATE(int) parse_type_to_record(char *t, int *type, int *type_size) {

    if (!strcmp(t,"float64")  || !strcmp(t,"double")) {
        *type = MD_TYPE_FLOAT64;
        *type_size = 8;

//    } else if (!strcmp(t,"float32")  || !strcmp(t,"float")) {
//        *type = MD_TYPE_FLOAT32;
//        *type_size = 4;
//
    } else if (!strcmp(t,"int64") || !strcmp(t,"long")) {
        *type = MD_TYPE_INT64;
        *type_size = 8;

    } else if (!strcmp(t,"int32") || !strcmp(t, "int")) {
        *type = MD_TYPE_INT32;
        *type_size = 4;

    } else if (!strcmp(t,"int08") || !strcmp(t, "byte")) {
        *type = MD_TYPE_INT08;
        *type_size = 1;

//    } else if (!strcmp(t,"int16")) {
//        *type = MD_TYPE_INT16;
//        *type_size = 2;

    } else if (!strcmp(t,"string")) {
        *type = MD_TYPE_STRING;
        *type_size = 2;

    } else {
        fatal("Unknown field type: %d",t);
    }

    return *type; // never used
}

//=============================================================================================
//
//    PARSE RECORD
//
//=============================================================================================


/**
 * 
 * 
 * 
 */ 
_PRIVATE(void) parse_record(cJSON * schema, PMetaData pmd) {

    cJSON *record = cJSON_GetObjectItem(schema,"record");
    if (!record || (!cJSON_IsObject(record))) {
        fatal("Missing record object");
    }

    cJSON *fields = cJSON_GetObjectItem(record, "fields");
    if (!fields || (!cJSON_IsArray(fields))) {
        fatal("Record: missing 'fields' object");
    }


    //------------------------
    // analisa fields in order
    //------------------------
    int n = cJSON_GetArraySize(fields);

    PRecordField prf = pmd->pri->fields;

    int offset = 0;

    for (int i = 0; i < n; i++) {

        cJSON * field = cJSON_GetArrayItem(fields, i);

        if (pmd->pri->n_fields == MAX_RECORD_FIELDS) {
            fatal("too many fields");
        }

        if (!cJSON_IsObject(field)) {
            fatal("invalid record field type");
        }

        prf->field_id = i;

        // name of field
        cJSON *id = getStringByName(field, "id", "Missing 'id' of field");
        prf->name = strdup(id->valuestring);
        register_field(prf->name, i);

        //------------------------
        // type of field
        //------------------------
        cJSON *type = cJSON_GetObjectItem(field, "type");
        if (!type) {
            fatal("missing record field type");
        }              
        if (!cJSON_IsString(type)) { fatal("type must be an string");}
        parse_type_to_record(type->valuestring, &prf->type, &prf->type_size);

        //------------------------
        // ajusta o offset deste field
        //------------------------
        prf->record_offset = offset;
        offset += prf->type_size;

        cJSON *aliases = cJSON_GetObjectItem(field, "aliases");
        if (aliases) {
            if (!cJSON_IsObject(aliases)) {
                fatal("Aliases");
            }
            map_str_t m;
            map_init(&m);

            int min = 0;
            int max = 0;

            cJSON *alias;
            cJSON_ArrayForEach(alias, aliases) {
                int v = atoi(alias->string);
//                printf("%s = %d  %d  %d\n", alias->string, v, min, max);
                if (!max) {
                    min = max = v;
                } else {
                    if (v < min) { min = v; }
                    if (v > max) { max = v; }
                }
                map_set(&m, alias->string, alias->valuestring);
            }


            int n = max - min + 1;
//            printf("N: %d Min: %d  Max: %d\n", n, min, max);

            prf->n_aliases = n;
            prf->min_aliases = min;
            prf->v_aliases = calloc(n,sizeof(char *));
            
           const char *key;
            map_iter_t mi = map_iter(&m);

            while ((key = map_next(&m, &mi))) {
                int i = atoi(key) - prf->min_aliases;
                prf->v_aliases[i] = *map_get(&m, key); 
            }

            map_deinit(&m);
        }

        //------------------------
        // proximo field
        //------------------------
        pmd->pri->n_fields ++;
        prf ++;
    }
    
    pmd->pri->record_sz = offset;
}

//=============================================================================================
//
// to_field conversion functions    
//
//=============================================================================================

//-----------------------
//
//
//
//
_PRIVATE(void) to_field_letter_to_int(
    char inputs[MD_INPUT_MAX_IDS][MD_INPUT_MAX_STRING], 
    void *params, 
    int *id_inputs, 
    void *result) {
    
    char c = inputs[id_inputs[0]][0];
    long res = c - 'A' + 1; // converte letras ASCII para numeros 
    * ((long *) result) = res;
    // printf("to_field_letter_to_int: Input = %d %s. Out = %ld\n",id_inputs[0],  inputs[id_inputs[0]], res);
}   

//-----------------------
//
//
//
//
_PRIVATE(void) to_field_op_to_int(
    char inputs[MD_INPUT_MAX_IDS][MD_INPUT_MAX_STRING], 
    void *params, 
    int *id_inputs, 
    void *result) {

    char c = inputs[id_inputs[0]][0];
    long res = (c == '-') ? 1: 0; // '-' =>1 else 0
    * ((long *) result) = res;
}   



static int max_order;

typedef struct {
    char *format;
    char *origin;
    char *what;
    unsigned long base;
} InputParams_datetime_to_epoch;




#include "extra/str_to_epoch.h"

//-----------------------
//
//
//
//
_PRIVATE(void) to_field_str_epoch_to_record(
    char inputs[MD_INPUT_MAX_IDS][MD_INPUT_MAX_STRING], 
    void *params, 
    int *id_inputs, 
    void *result) {

    //InputParams_datetime_to_epoch *ip = (InputParams_datetime_to_epoch *) params;

    long res = string_to_epoch(inputs[id_inputs[0]]) ; //- ip->base;
    * ((long *) result) = res;
    // printf("datetime_to_epoch: %s, %s, %ld. Input = %d %s. Out = %ld\n",ip->format, ip->origin, ip->base,id_inputs[0],  inputs[id_inputs[0]], res);
}   

//-----------------------
//
//
//
//
_PRIVATE(void) to_field_datetime_to_record(
    char inputs[MD_INPUT_MAX_IDS][MD_INPUT_MAX_STRING], 
    void *params, 
    int *id_inputs, 
    void *result) {

    InputParams_datetime_to_epoch *ip = (InputParams_datetime_to_epoch *) params;

    long res = datetime_to_epoch(ip->format, inputs[id_inputs[0]],ip->what) ;
    if (res < 0) fatal("invalid datetime: %s", inputs[id_inputs[0]]);
    res -= ip->base;
    * ((int32_t *) result) = res;
    // char *s = ip->what ? ip->what : "e";
    //printf("datetime_to_epoch: (%s) %s, %s, %ld. Input = %d %s. Out = %ld\n",s, ip->format, ip->origin, ip->base,id_inputs[0],  inputs[id_inputs[0]], res);
}   


//-----------------------
//
// [ "lt", 10, 20, 30  ]
//      0 = [-inf, 10), 1 =[10, 20), 2=[20, 30), 3=[30, +inf]
//
// [ "lt", 10, 20, 30, null  ]
//      0 = [-inf, 10), 1 =[10, 20), 2=[20, 30), ERR=[30, +inf]
//
// [ "lt", null, 10, 20, 30  ]  
//      ERR = [-inf, 10), 0 =[10, 20), 1=[20, 30), 2=[30, +inf]
//
// [ "lt", null, 10, 20, 30, null ] 
//      ERR = [-inf, 10), 0 =[10, 20), 1=[20, 30), ERR=[30, +inf]
//
//
// [ "le", 10, 20, 30  ]
//      0 = [-inf, 10], 1 =(10, 20], 2=(20, 30], 3=(30, +inf]
//
// ( "lt", 10, 20, 30, null ]
//      0 = [-inf, 10], 1 =(10, 20], 2=(20, 30], ERR=(30, +inf]
//
// ( "lt", null, 10, 20, 30  ]  
//      ERR = [-inf, 10], 0 =(10, 20], 1=(20, 30], 2=(30, +inf]
//
// ( "lt", null, 10, 20, 30, null ] 
//      ERR = [-inf, 10], 0 =(10, 20], 1=(20, 30], ERR=(30, +inf]
//

_PRIVATE(void) parse_map_range(cJSON *map_range, PTargetField ptf, PRecordField prf) {

    ptf->default_input_to_field = 1;
    ptf->inputs_ids[0] = ptf->input_id;

    if (!cJSON_IsArray(map_range)) {
        fatal("The 'map-range' field must be an object");
    }

    cJSON *le = cJSON_GetObjectItem(map_range, "<=");
    if (le) {
        if (!cJSON_IsBool(le)) { fatal("boolean"); }
        // lower_equal = le->valueint;
    }

}

//-----------------------
//
//
//
//
_PRIVATE(void) parse_map_string(cJSON *map_string, PTargetField ptf, PRecordField prf) {

    if (!cJSON_IsObject(map_string)) {
        fatal("The 'map-string' field must be an object");
    }
    ptf->inputs_ids[0] = ptf->input_id;

    if ((prf->type >= MD_TYPE_INT08) && (prf->type <= MD_TYPE_INT32)) {
        prf->type = MD_TYPE_INT32;
        ptf->use_map_input = INT_MAP_INPUT;
        map_init(&ptf->map_input_int);

    } else if ( (prf->type == MD_TYPE_FLOAT32 ) ||  (prf->type == MD_TYPE_FLOAT64 )) {
        ptf->use_map_input = DOUBLE_MAP_INPUT;
        map_init(&ptf->map_input_double);

    } else {
        fatal("Map types must be a integer or double type");
    }

    cJSON *kv;
    cJSON_ArrayForEach(kv, map_string) {
        if (!cJSON_IsNumber(kv)) {
            fatal("Invalid map value %s",kv->valuestring);
        }
        if (ptf->use_map_input == INT_MAP_INPUT) {
            if (map_get(&ptf->map_input_int, kv->string)) {
                fatal("duplicated key");
            }
            map_set(&ptf->map_input_int, kv->string, kv->valueint);
        } else {
            if (map_get(&ptf->map_input_double, kv->string)) {
                fatal("duplicated key");
            }
            map_set(&ptf->map_input_double, kv->string, kv->valuedouble);
        }
    }

}

//-----------------------
//
//
//
//
_PRIVATE(void) parse_conv_function(cJSON * conv, PTargetField ptf, PMetaData pmd) {

    if (!cJSON_IsArray(conv)) {
        fatal("The 'conv' field must be an array that defines a function");
    }

    //-----------------------
    // convertion function 
    //-----------------------
    cJSON * item = cJSON_GetArrayItem(conv, 0);

    if (!item) {
        // treat as simple type
        fatal ("Missing conversion function name.");

    // function name?
    }  if (cJSON_IsString(item)) {
        char *s = item->valuestring;

        if (strcmp(s,"datetime_to_epoch")==0) {
            InputParams_datetime_to_epoch *ip = (InputParams_datetime_to_epoch *) ptf->input_params;

            // pega os parametros do registry
            char *s; 
            if (!register_locate_in_registry("format", ptf->name, &s)) {
                fatal("Not format found.");
            }
            ip->format = strdup(s);

            ip->what = "e";
            register_locate_in_registry("what", ptf->name, &(ip->what));

            char w =  ip->what[0];
            if ((w != 'e') && (w!='H') && (w!='M')){
                ip->base = 0;
            } else {
                if (!register_locate_in_registry("origin", ptf->name, &s)) {
                    fatal("Not origin found.");
                }
                ip->origin = strdup(s);

                ip->base = datetime_to_epoch(ip->format, ip->origin, (w=='e')?NULL:ip->what);
                if (!ip->base) {
                    fatal("datetime_to_epoch error: [%s]",datetime_to_epoch_err());
                }
                ip->base--; // descarta o segundo a mais 
            }

            // indica quem e o parametro do input a ser convertido
            ptf->inputs_ids[0] = ptf->input_id;
            ptf->conv_to_field = to_field_datetime_to_record;

        } else if (strcmp(s,"string_to_epoch")==0) {
            InputParams_datetime_to_epoch *ip = (InputParams_datetime_to_epoch *) ptf->input_params;

            // pega os parametros do registry
            char *s; 
            if (!register_locate_in_registry("format", ptf->name, &s)) {
                fatal("Not format found.");
            }
            ip->format = strdup(s);

            if (!register_locate_in_registry("origin", ptf->name, &s)) {
                fatal("Not origin found.");
            }
            ip->origin = strdup(s);

            ip->base = datetime_to_epoch(ip->format, ip->origin, NULL);
            if (!ip->base) {
                fatal("datetime_to_epoch: %s",datetime_to_epoch_err());
            }

            // indica quem e o parametro do input a ser convertido
            ptf->inputs_ids[0] = ptf->input_id;
            ptf->conv_to_field = to_field_str_epoch_to_record;

        } else if (strcmp(s,"letter_to_int")==0) {
            ptf->inputs_ids[0] = ptf->input_id;
            ptf->conv_to_field = to_field_letter_to_int;

        } else if (strcmp(s,"op_to_int")==0) {
            ptf->inputs_ids[0] = ptf->input_id;
            ptf->conv_to_field = to_field_op_to_int;

        } else {
            fatal("Unknown function '%s'.",s);
        }
    // Unknown
    } else {
        fatal("Convertion function must have a name.");
    }
}


//=============================================================================================
//
//    PARSE INPUT
//
//=============================================================================================


//=============================================================================================
//
//    PARSE INPUT
//
//=============================================================================================


/**
 * 
 * 
 * 
 */ 
_PRIVATE(int) parse_datum_field(cJSON  *datum, PMetaData pmd, PTargetField ptf) {

    cJSON *id = cJSON_GetObjectItem(datum, "id");

    if (!id || !cJSON_IsString(id)) {
        fatal("field od data");
    }

    // localiza o campo no record
    if (!register_locate_field(id->valuestring, &ptf->id_record)) {
        fatal("Field %s desconhecido",id->valuestring);
    }

    // store the name of field
    strncpy(ptf->name, id->valuestring, MD_INPUT_MAX_NAME);

    // prf points to correspondent Record data
    PRecordField prf = pmd->pri->fields + ptf->id_record;

    // cJSON *domain = cJSON_GetObjectItem(value,"domain");

    //------------------------
    //   data.conv ?
    //------------------------
    cJSON *conv = cJSON_GetObjectItem(datum, "conv"); 
    if (conv) {
        parse_conv_function(conv, ptf, pmd);
        return 0;
    }

    //------------------------
    //   data.map_string ?
    //------------------------
    cJSON *map_string = cJSON_GetObjectItem(datum, "map-string");
    if (map_string) {
        parse_map_string(map_string, ptf, prf);
        return 0;
    }

    //------------------------
    //   data.map_range ?
    //------------------------
    cJSON *map_range = cJSON_GetObjectItem(datum, "map-range");
    if (map_range) {
        parse_map_range(map_range, ptf, prf);
        return 0;
    }

    //------------------------
    //   set value ?
    //------------------------
    cJSON *set_value = cJSON_GetObjectItem(datum, "set-value");
    if (set_value) {
        ptf->set_value = strdup(set_value->valuestring); // armazena o valor 
//                printf("Set_value = %s\n",pif->set_value);
    }

    //------------------------
    //   default 
    //------------------------
    cJSON *default_value = cJSON_GetObjectItem(datum, "default");
    if (default_value) {
        ptf->default_value = strdup(default_value->valuestring); // armazena o valor 
//                printf("Set_value = %s\n",pif->set_value);
    }

    //------------------------
    //   Default convertion
    //------------------------
    ptf->default_input_to_field = 1;
    ptf->inputs_ids[0] = ptf->input_id;
    
    return 1;
}

/**
 * 
 * 
 * 
 */ 
_PRIVATE(void) parse_input(cJSON * schema, PMetaData pmd) {

    // initialize data_fields
    max_order = 0;

    // numero de ids existentes para o input
    int n_input_ids = 0; 

    //------------------------
    //   input
    //------------------------
    cJSON * input = cJSON_GetObjectItemCaseSensitive(schema, "input");
    if (!input || !cJSON_IsObject(input)) {
        missing("input","Object","schema");
    }
    
    pmd->pii->sep[0] = '\0';
    
    //------------------------
    //   type
    //------------------------
    cJSON *type =  cJSON_GetObjectItem(input,"type");
    if (!type) {
        pmd->pii->type = XDATA_CSV; // default csv 
    } else if (!cJSON_IsString(type)) {
        fatal("Invalid 'input' type");
    } else {
        char *s = type->valuestring;
        if (!strcmp(s,"csv")) {
            pmd->pii->type = XDATA_CSV; 
        } else if (!strcmp(s,"test")) {
            pmd->pii->type = XDATA_TEST; 
        } else {
            fatal("Invalid 'input' type: %s", s);
        }
    }

    if (pmd->pii->type == XDATA_CSV) {
        //------------------------
        //   data.sep
        //------------------------
        cJSON *sep =  cJSON_GetObjectItem(input,"sep");
        if (!sep) {
            pmd->pii->sep[0] = ';';
        } else if (!cJSON_IsString(sep)) {
            fatal("Invalid 'sep' type");
        } else if (sep->valuestring[0] == '\\') {
            char c = sep->valuestring[1];
            if (c != 't') fatal("'\\' implies '\\t'");
            pmd->pii->sep[0] = '\t';
        } else if (sep->valuestring[1] != '\0') {
            fatal("Invalid 'sep' value: %s", sep->valuestring);
        } else {
            pmd->pii->sep[0] = sep->valuestring[0];
        }
        pmd->pii->sep[1] = '\0';
        pmd->pii->sep[2] = '\0';
    }

    //------------------------
    //   input.data
    //------------------------
    cJSON * data = cJSON_GetObjectItemCaseSensitive(input, "data");
    if (!data || !cJSON_IsArray(data)) {
        missing("data","Array","input");
    }

    //------------------------
    // for each datum
    //------------------------
    cJSON * datum;
    cJSON_ArrayForEach(datum, data) {

        //------------------------
        // Allocate the array item
        //------------------------
        PInputField pif = calloc(1, sizeof(InputField));

        // encadeia na lista
        pif->next = NULL;
        if (pmd->pii->last) {
            pmd->pii->last->next = pif;
        } else {
            pmd->pii->first = pif;
        }
        pmd->pii->last = pif;

        //------------------------
        // ignore null values
        //------------------------
        if (cJSON_IsNull(datum)) {        
            pif->ignore = 1;
            continue;
        }

        pif->ignore = 0;

        //------------------------
        // generates an id for the input
        //------------------------
        pif->input_id = n_input_ids;
        n_input_ids ++;

        pif->n_targets = 0;
        PTargetField ptf = pif->targets + pif->n_targets;

        //---------------------------
        // this datum is used as part of another value 
        //---------------------------
        if (cJSON_IsString(datum)) {
            
            // guarda o nome do campo
            strncpy(ptf->name, datum->valuestring, MD_INPUT_MAX_NAME);

            ptf->do_not_convert = 1;
            pif->n_targets ++;

            continue;
        } 

        //---------------------------
        //
        //---------------------------
        if (cJSON_IsObject(datum)) {

            cJSON *fields = cJSON_GetObjectItem(datum, "fields");
            if (!fields) {
                ptf->input_id = pif->input_id;
                parse_datum_field(datum, pmd, ptf);
                pif->n_targets ++;
            
            } else {
                cJSON *target;
                cJSON_ArrayForEach(target, fields) {
                    ptf->input_id = pif->input_id;
                    parse_datum_field(target, pmd, ptf);
                    ptf++;
                    pif->n_targets ++;
                }
            }

        } else {
            fatal("Wrong input definition.");
        }        
    }

}


//=============================================================================================
//
//  PARSE DIMENSIONS    
//
//=============================================================================================

//-----------------------
//
//
//
//
void check_type(char t, int type) {
    if ((t == 'D') && (type != MD_TYPE_FLOAT64)) {
        fatal("Invalid type of parameter");
    } else if ((t == 'I') && (type != MD_TYPE_INT32)) {
        fatal("Invalid type of parameter");
    }
}

//-----------------------
//
//
//
//
void parse_dimensional_class(cJSON *class, PAddressValue pav, PAddressDim pad, PMetaData pmd) {
    cJSON *item;
    int id_field = 0;

    cJSON * class_name = cJSON_GetArrayItem(class, 0);

    if (!class_name || !cJSON_IsString(class_name)) { 
        fatal("The first item of a 'class' must be its name");
    } 

    PClass pclass;
    if (!register_locate_class(class_name->valuestring, &pclass)) {
        fatal("Class '%s' not found\n", class_name->valuestring);
    }

    int *ids;
    if (pad) {
        ids = pad->ids; 
        pad->pclass = pclass;
        pad->record_to_address = pclass->to_address;
    } else {
        ids = pav->ids;
        pav->record_to_address = pclass->to_address;
        pav->pclass = pclass;
    }

    int idx = 1;
    
    char *c, *t;
    for (c = pclass->to_address_fmt, t = pclass->to_address_types; *c && *t; c++, t++, idx++) {
        if (*c == 'F') {
            item = cJSON_GetArrayItem(class, idx);

            if ((idx == 1) && pav) {
                register_value(item->valuestring, pmd->pai->n_values);
                // printf("register_value: %s %d\n",item->valuestring, pmd->pai->n_values);
                pmd->pai->n_values ++;

                pav->name = strdup(item->valuestring);
            }

            register_locate_field(item->valuestring, &idx);

            check_type(*t, pmd->pri->fields[idx].type);

            ids[id_field] = idx;
            // printf("PARSE_CLASS: %s %d\n",item->valuestring, idx);
            id_field ++;
        }
    }
}


//-----------------------
//
//
//
//
_PRIVATE(void) parse_levels(cJSON *levels, PAddressDim pad, PMetaData pmd) {
    cJSON * level;
    cJSON_ArrayForEach(level, levels) {

        if (!cJSON_IsObject(level)) {
            fatal("An level must be an object");
        }

        PAddressValue pav = calloc(1, sizeof(AddressValue));

        if (pad->last) {
            pad->last->next = pav;
        } else {
            pad->first = pav;
        }
        pad->last = pav;
        pad->n_values ++;

        cJSON *class = cJSON_GetObjectItem(level, "class");
        if (!class || !cJSON_IsArray(class)) {
            fatal("Missing class element");
        }

        parse_dimensional_class(class, pav, NULL ,pmd);
    }
}


/**
 * 
 * 
 * 
 */ 
_PRIVATE(void) parse_dimensions(cJSON *schema, PMetaData pmd) {
    //------------------------
    //   address
    //------------------------
    cJSON * chains = cJSON_GetObjectItemCaseSensitive(schema, "dimensions");
    if (!chains || !cJSON_IsArray(chains)) {
        missing("dimensions","Array","Schema");
    }

    cJSON *chain;

    //------------------------
    // for each chains
    //------------------------
    cJSON_ArrayForEach(chain, chains) {
        pmd->pai->n_dimensions ++;

        if (!cJSON_IsObject(chain)) {
            fatal("A chdimension must be an object");
        }

        //------------------------
        // Allocate the Address Level
        //------------------------
        PAddressDim pad = calloc(1, sizeof(AddressDim));
        pad->next = NULL;
        if (pmd->pai->last) {
            pmd->pai->last->next = pad;
        } else {
            pmd->pai->first = pad;
        }
        pmd->pai->last = pad;
        
        //------------------------
        // name
        //------------------------
        cJSON * UNUSED_VAR(name) = 
            getStringByName(chain,"id","Missing parameter 'name' of the dimensions");
        
            
        //------------------------
        // length "height"
        //------------------------
        cJSON * n =  cJSON_GetObjectItem(chain, "height");
        int length;
        if (n) {
            length = n->valueint;
        } else {
            length = 1;
        }

        //------------------------
        // class
        //------------------------
        cJSON *class = cJSON_GetObjectItem(chain, "class");
        if (class) {

            register_value(name->valuestring, pmd->pai->n_values);
            // printf("register_value: %s %d\n",name->valuestring, pmd->pai->n_values);

            if (!cJSON_IsArray(class)) { fatal("'class' must be an array"); }

            pad->n_values = length;

            parse_dimensional_class(class, NULL, pad, pmd);

            if ((pad->pclass->class_type == LEVEL_CLASS) && (length != 1)) {
                fatal("A level class has to be length of 1");
            }
            pmd->pai->n_values += pad->n_values;

            continue;
        } 
        
        //------------------------
        // members
        //------------------------
        cJSON *members = cJSON_GetObjectItem(chain, "members");
        if (members) {
            // parse_levels
            if (!cJSON_IsArray(members)) {
                fatal("Field 'values' must be an array");
            }
            if (length < 1) {
                fatal("'members' requires 'height' > 0.");
            }

            parse_levels(members, pad, pmd);

            if (n->valueint != pad->n_values) {
                fatal("Actual number of members is different from parameter 'height'");
            }

            //pmd->pai->n_values += pad->n_values;
            continue;
        }

        fatal("Error: Either field 'class' or field 'values' must exist");
    }

    // create a quick access to dimensions
    {
        PAddressDim p, *q;
        q = pmd->pai->p_dimensions = malloc(sizeof(PAddressDim) * pmd->pai->n_dimensions); 
        for (p = pmd->pai->first; p; p = p->next, q++) {
            *q = p;
        }
    }
    // create a quick access to values
    {
        PAddressDim d;
        PAddressValue p, *q;
        pmd->pai->p_values = calloc(sizeof(PAddressValue), pmd->pai->n_values); 
        q = pmd->pai->p_values;
        for (d = pmd->pai->first; d; d = d->next) {
            if (!d->first) {
                q += d->n_values;
            } else {
                for (p = d->first; p; p=p->next, q++) {
                    *q = p;
                }
            }
        }
    }
}

//=============================================================================================
//
//    TERMINAL related functions
//
//=============================================================================================

//-----------------------
//
//
//
//
 PContentInfo create_content_info() {
    return NULL;
}

//-----------------------
//
//
//
//
static void parse_formula(cJSON *formula, PRecordInfo pri, PMetadataTerminalChannel pmtc, PMetaData pmd) {
    if (!cJSON_IsArray(formula)) {
        fatal("invalid formula type");
    }

    cJSON *item;
    int i = 0;
    PContentInfo pci = NULL;
    int n_params = 0;
    
    cJSON_ArrayForEach(item, formula) {
        if ((pci) && (i > (pci->n_param_fields + pci->max_params))) {
            fatal("Too many arguments in formula");
        }

        if (!cJSON_IsString(item)) {
            fatal("arguments must be Strings");
        }
        
        char *s = item->valuestring;

        // tipo de content 
        if (i == 0) {
            if (!register_locate_content(s, &pci)) {
               fatal("Unknown content ");
            }
            pmtc->tc.data_size = pci->private_size;
            pmtc->tc.pcfs = pci->pcfs;
            pmtc->tc.merge_size = pci->merge_size;
            pmtc->tc.result_size = pci->result_size;
            pmtc->tc.result_is_int = pci->result_is_int;
            pmtc->tc.pclass = pci->pclass; 

        // fields
        } else if (i - 1 < pci->n_param_fields) {
            int id;
            register_locate_field(s, &id);
            pmtc->tc.tfd.record_offsets[i-1] = pri->fields[id].record_offset;
            pmtc->tc.tfd.record_type[i-1] = pri->fields[id].type;
            // printf("RECORD_OFFSET (%s): %d\n",s,pri->fields[id].record_offset);

        // parametros
        } else if (i - pci->n_param_fields - 1 < pci->max_params) {
            if (pci->pcfs->parse_param) {
                pci->pcfs->parse_param(i - pci->n_param_fields, s, pmtc->tc.tfd.params);
            }
            n_params ++;

        } else {
            // parametros de mais
        }
        i ++;
    }

    if (n_params < pci->min_params) {
        fatal("Missing parameters for class/formula [%s].",formula->valuestring);
    }
}

#if CLASS_PARSE
//-----------------------
//
//
//
//
static void parse_container(cJSON *container, PRecordInfo pri, PMetadataTerminalChannel pmtc, PMetaData pmd) {

    if (!cJSON_IsArray(container)) {
        fatal("invalid container type");
    }

    cJSON *item;
    int i = 0;
    PClass pc = NULL;
    int n_params = 0;
    
    cJSON_ArrayForEach(item, container) {

        if ((pc) && (i > (pc->n_fields + pc->n_params))) {
            fatal("Too many arguments.");
        }

        if (!cJSON_IsString(item)) {
            fatal("arguments must be Strings");
        }
        
        char *s = item->valuestring;

        // tipo de content 
        if (i == 0) {
            if (!register_locate_class(s, &pc)) {
               fatal("Unknown content ");
            }
            printf("Container: %s n_fields=%d max_params=%d\n",s,pc->n_fields, pc->n_params);
            pmtc->tc.pclass = pc;

        // fields
        } else if (i - 1 < pc->n_fields) {
            int id;
            register_locate_field(s, &id);
            pmtc->tc.tfd.record_offsets[i-1] = pri->fields[id].record_offset;
            pmtc->tc.tfd.record_type[i-1] = pri->fields[id].type;
            printf("CONTAINER FIELD (%s): %d\n",s,pri->fields[id].record_offset);

        // parametros
        } else if (i - 1 >= pc->n_fields) {
            printf("CONTAINER PARAMETER (%s) %d\n",s, i);
            if (pc->parse_param) {
                pc->parse_param(i  - pc->n_fields, s, pmtc->tc.tfd.params);
            }
            n_params ++;

        } else {
            // parametros de mais
        }
        i ++;
    }

//    if (n_params < pc->min_params) {
//        fatal("Missing parameters for container [%s].",container->valuestring);
//    }
}
#endif

//-----------------------
//
//
//
//
static void parse_channels(cJSON *channels, PMetaData pmd, PMetadataTerminalChannel ptc0) {

    PRecordInfo pri = pmd->pri;
    // int id_master_channel = pmd->pti->n_channels;

    //------------------------
    // for each channel
    //------------------------
    cJSON *channel;
    int id_channel = 1;
    cJSON_ArrayForEach(channel, channels) {

        //------------------------
        // name
        //------------------------
        cJSON *id = cJSON_GetObjectItem(channel, "id");
        if (!id || !cJSON_IsString(id)) { fatal("Missing channel id"); }

        //register_channel(id->valuestring, id_master_channel, id_channel);
       
        //------------------------
        // Allocate Channel
        //------------------------
        PMetadataTerminalChannel ptc = calloc(1, sizeof(MD_TerminalChannel));

        ptc->name = strdup(id->valuestring);
        if (ptc0->last) {
            ptc0->last->next = ptc;
        } else {
            ptc0->first = ptc;
        }
        ptc0->last = ptc;
        ptc0->tc.n_channels ++;

        //------------------------
        // content
        //------------------------
        cJSON *content = cJSON_GetObjectItem(channel, "formula");
        if (content) {

            parse_formula(content, pri, ptc, pmd);
            ptc0->tc.tfd.total_data_size += ptc->tc.data_size;
            continue;
        } 

        id_channel ++;
    }
}


//-----------------------
//
//
//
//
static void parse_container(cJSON *container, PRecordInfo pri, PMetadataTerminalChannel pmtc, PMetaData pmd) {

    if (!cJSON_IsObject(container)) {
        fatal("invalid container type. Must be an object.");
    }

    cJSON *class = cJSON_GetObjectItem(container, "class");
    if (!class) {
        fatal("Missing container's class.");
    }
    parse_formula(class, pri, pmtc, pmd);

    //------------------------
    // bin
    //------------------------
    cJSON *bin = cJSON_GetObjectItem(container, "bin");
    if (!bin) {
        pmtc->tc.tfd.bin_size = 1;
    } else if (cJSON_IsNumber(bin)) {
        pmtc->tc.tfd.bin_size = bin->valueint;
    } else {
        fatal("Invalid bin = %s\n",bin->string);
    }

    //------------------------
    // contents
    //------------------------
    cJSON *contents = cJSON_GetObjectItem(container, "contents");
    if (!contents) {
        fatal("Missing  'contents'");
    }

    pmtc->tc.is_container = 1;
    pmd->pti->total_data_size += pmtc->tc.data_size;

    parse_channels(contents, pmd, pmtc);
    pmd->pti->n_slots += pmtc->tc.n_channels;
}


//-----------------------
//
// Exemplo de sintaxe:
// "terminal": {
//     "contents": [
//       
//         INSERIDO AUTOMATICAMENTE  
//         { "id": "counter", formula: [ "#"] }
//         ,
//
//         { "id": "hours",   "class": ["binlist", "seconds", "1" ],
//             "contents": [
//                 { "id": "hc", "formula":["counter"] },
//                 { "id": "havg", "formula":["avg", "ibytes"] }
//             ] 
//         }
//         ,   
//         { "id": "sum",  "formula": [ "sum", "ibytes"] } 
//     ]
// },
//
//
_PRIVATE(void) parse_terminal(cJSON *schema, PMetaData pmd) {

    //------------------------
    //   terminal
    //------------------------
    cJSON * terminal = cJSON_GetObjectItemCaseSensitive(schema, "terminal");
    if (terminal && !cJSON_IsObject(terminal)) {
        missing("terminal","Object","Schema");
    }

    //------------------------
    //   contents
    //------------------------
    cJSON * contents;
    if (!terminal) {
        contents = cJSON_CreateArray(); //   cria um array vazio para contents
    } else {
        contents = cJSON_GetObjectItem(terminal, "contents"); // pega o array para contents
        if (!contents || !cJSON_IsArray(contents)) {
            missing("contents","Array","Terminal");
        }
    }

    //--------------------------------
    // Todo terminal tem um channel "counter"
    // Insere a descrição para o "counter" em contents
    // Coloca na primeira posição do  array "contents" o 
    // objeto ficticio counter equivalente ao codigo em json: 
    //     { id: "counter", "formula": [ "#" ] } 
    //--------------------------------
    cJSON *counter =  cJSON_CreateObject();
    cJSON_InsertItemInArray(contents, 0, counter);  // insere na primeira posicao

    cJSON_AddItemToObject(counter,"id",cJSON_CreateString("counter"));
    
    // cria c_content para se ter um array referente a formula
    cJSON * c_content = cJSON_CreateArray();
    cJSON_AddItemToArray(c_content,cJSON_CreateString("#"));

    cJSON_AddItemToObject(counter,"formula",c_content);

    //------------------------
    // for each terminal channel
    //------------------------
    cJSON *item;
    cJSON_ArrayForEach(item, contents) {

        if (!cJSON_IsObject(item)) {
            fatal("An item of terminal must be an object.");
        }

        //------------------------
        // Allocate Channel
        //------------------------
        PMetadataTerminalChannel pmtc = calloc(1, sizeof(MD_TerminalChannel));
        pmtc->tc.tfd.bin_size = 1;

        // insere o canal recem criado na lista em pti
        if (pmd->pti->last) {
            pmd->pti->last->next = pmtc;
        } else {
            pmd->pti->first = pmtc;
        }
        pmd->pti->last = pmtc;
        pmd->pti->n_channels ++;

        // int id_channel = pmd->pti->n_channels;
        pmd->pti->n_slots ++;
        
        //------------------------
        // id
        //------------------------
        cJSON *id = cJSON_GetObjectItem(item, "id");
        if (!id || !cJSON_IsString(id)) { fatal("Missing channel id"); }

        //register_channel(id->valuestring, id_channel, 0);
        pmtc->name = strdup(id->valuestring);

        //------------------------
        // formula
        //------------------------
        cJSON *formula = cJSON_GetObjectItem(item, "formula");
        if (formula) {
            if (cJSON_GetObjectItem(item, "container")) {
                fatal("You cannot use fields 'formula' and 'container' in the same content");
            }
            parse_formula(formula, pmd->pri, pmtc, pmd);
            pmd->pti->total_data_size += pmtc->tc.data_size;
        }  

        //------------------------
        // container
        //------------------------
        cJSON *container = cJSON_GetObjectItem(item, "container");
        if (container) {
            if (formula) {
                fatal("You cannot use fields 'formula' and 'container' in the same content");
            }
            parse_container(container, pmd->pri, pmtc, pmd);
        } 

        if (!formula && !container) {
            fatal("You must use 'formula' or 'container' in a content");
        }
    }

    /* BLOCK OF CODE */ {
        PTerminalInfo pti = pmd->pti;
        PMetadataTerminalChannel ptc;

        //------------------------
        //   Ajusta campos do terminal
        //------------------------
        PMetadataTerminalChannel terminal_classes[20];
        int idx_terminal_classes[20];
        int n_terminal_classes = 0;

        // Verifica se o TerminalInfo em pti não possui slots
        if (!(pti->n_slots > 0)) fatal("x");
        // printf("N_SLOTS: %d N_CHANNELS: %d  SIZE: %d\n", pti->n_slots, pti->n_channels, pti->total_data_size);

        pti->p_slots = calloc(pti->n_slots, sizeof(PTerminalChannel));

        int data_offset = 0;
        int n_slots = 0;

        // canais do terminal
        for (ptc = pmd->pti->first; ptc; ptc = ptc->next) {
            ptc->tc.data_offset = data_offset;
            ptc->tc.is_subchannel = 0;

            // printf("PTC OFFSET %d SIZE %d\n", ptc->tc.data_offset, ptc->tc.data_size);
            data_offset += ptc->tc.data_size;
            pti->p_slots[n_slots] = &ptc->tc;
            register_slot(ptc->name,n_slots);
            // printf("REGISTER SLOT: %s -> %d\n",ptc->name, n_slots);

            if (ptc->tc.is_container) {
                if (n_terminal_classes == 10) fatal("Excesso de grupos");
                terminal_classes[n_terminal_classes] = ptc;
                idx_terminal_classes[n_terminal_classes] = n_slots;
                n_terminal_classes ++;
            }
            
            n_slots ++;
        }

        // gera os canais dos grupos
        for (int i=0; i < n_terminal_classes; i++) {
            PMetadataTerminalChannel group = terminal_classes[i];
            pti->p_slots[idx_terminal_classes[i]]->base_slot = n_slots;

            data_offset = 0;
            for (ptc = group->first; ptc; ptc = ptc->next) {
                ptc->tc.data_offset = data_offset;
                ptc->tc.is_subchannel = 1;
                ptc->tc.id_parent_slot = idx_terminal_classes[i];

                data_offset += ptc->tc.data_size;
                pti->p_slots[n_slots] = &ptc->tc;
                register_slot(ptc->name,n_slots);
                // printf("REGISTER SUB-SLOT: %s -> %d\n",ptc->name, n_slots);

                n_slots ++;
            }
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
_PUBLIC(int) metadata_load( PMetaData pmd, char *fname) {

    // initialize input_info
    
    //------------------------  
    // Parse Schema from file
    //------------------------
    cJSON *schema = parse_schema_from_file(fname, pmd);

    cur_fname = fname;

    //------------------------
    //   Parse Registry
    //------------------------
    parse_registry(schema, pmd);

    //------------------------
    //   Parse Record
    //------------------------
    parse_record(schema, pmd);

    //------------------------
    //   Parse Input
    //------------------------
    // parse_input(schema, pmd);

    //------------------------
    //   Parse Address
    //------------------------
    parse_dimensions(schema, pmd);

    //------------------------
    //   Parse Terminal
    //------------------------
    parse_terminal(schema, pmd);

    //------------------------
    //   Parse XDATA
    //------------------------
    parse_input(schema, pmd);

#if 0
    //------------------------
    //   Dump Record Field
    //------------------------
    int i;
    PRecordField prf;
    for(i=0, prf=pmd->pri->fields; i<pmd->pri->n_fields; i++, prf++) {
        if (prf->name) {
            printf("Name: %s\n", prf->name);
            printf("  Type (%d): %d\n",prf->type_size,prf->type);
            printf("  Use default conv: %d\n",prf->default_input_to_field);
            printf("  Use map: %d\n",prf->use_map_input);
            printf("  \n");
        }
    }
#endif

    // Release any objects created
    cJSON_Delete(schema);

    return 1;
}


#define MAX_LINE_SZ 512

/**
 * 
 * 
 * 
 */ 
_PRIVATE(void) get_inputs_from_test_line(char *line, PMetaData pmd, char inputs[MD_INPUT_MAX_IDS][MD_INPUT_MAX_STRING]) {
    char *s = line;
    PInputField pif;

    for (pif = pmd->pii->first; pif; pif = pif->next, s++) {

        inputs[pif->input_id][1] = '\0'; // all test values have length equal 1
        PTargetField ptf = pif->targets;

        if (ptf->set_value) {
            inputs[ptf->input_id][0] = ptf->set_value[0];
            continue;
        }

        if (pif->ignore) continue;
        
        inputs[pif->input_id][0] = *s;
        //printf("INPUT: %s -> '%s'\n", ptf->name, inputs[ptf->input_id]);
    } 
}



//*****************************************************************************************************************
//
//   input files
//
//*****************************************************************************************************************

/**
 * 
 * 
 * 
 */ 
_PUBLIC(void) metadata_open_input_file(PMetaData pmd, char *fname) {
    pmd->driver_fin = fopen(fname, "rt");
    if (!pmd->driver_fin) {
        fatal("Could not open input filename: %s\n", fname);
    }
}

/**
 * 
 * 
 * 
 */ 
_PUBLIC(void) metadata_close_input_file(PMetaData pmd) {
    if (!pmd->driver_fin) {
        fatal("Trying to close a input file not openned\n");
    }
    fclose(pmd->driver_fin);
}


/**
 * 
 * 
 * 
 */ 
char *strtok_ex(char *s, char *sep) {
    static char *p = NULL;
    if (s) p = s;
    if (!p || !*p) return NULL;
    char *a = p;
    while (*p && *p != sep[0]) { p++; }
    *p = '\0'; // separador vira \0
    p++;       // aponta para o inicio do proximo elemento
    return a;
}


/**
 * 
 * 
 * 
 */ 
_PRIVATE(int) get_inputs_from_line_with_sep(char *line, PMetaData pmd, char inputs[MD_INPUT_MAX_IDS][MD_INPUT_MAX_STRING]) {
    char *s = line;
    char *sep = pmd->pii->sep;
    char *token;

    PInputField pif;
    for (pif = pmd->pii->first; pif; pif = pif->next) {

        // ignora o datum?        
        if (pif->ignore) continue;
        
        PTargetField ptf = pif->targets;

        if (ptf->set_value) {
            // token is the preset value
            token = ptf->set_value;

        } else {
            // token from line
            token = strtok_ex(s,sep);
            // printf("Token: %s\n",token);
            if (!token) return 0;
            if (!*token) {
                //printf("Default for ptf [%s]: %s\n",ptf->name, ptf->default_value);
                if (!ptf->default_value) return 0;
                token = ptf->default_value;
            }
            s = NULL;
        }

        strncpy(inputs[pif->input_id], token, MD_INPUT_MAX_STRING-1);
        //printf("INPUT: %s -> '%s'\n", ptf->name, token );
    } 
    return 1;
}

/**
 * 
 * 
 * 
 */ 
_PRIVATE(void) process_xdata(char inputs[MD_INPUT_MAX_IDS][MD_INPUT_MAX_STRING], PMetaData pmd, void *record) {
    PRecordField prf;

    // char * record = record;
    int i;
    PInputField pif;
    for (i = 0, pif = pmd->pii->first; pif; pif = pif->next, i++) {
        //printf("DATUM: %d\n",i);
        if (pif->ignore) continue;
        
        PTargetField ptf = pif->targets;
        if (ptf->do_not_convert) continue;  // don't convert appers on first target only

        for (int k=0; k<pif->n_targets; k++, ptf++) {

            prf = pmd->pri->fields + ptf->id_record;
            char *ptr = (char *) record + prf->record_offset;
#if 0
            printf("InputID of '%s' = %d  Record Id: %d Record Offset: %d s:%s\n",
                ptf->name, 
                ptf->inputs_ids[0], 
                ptf->id_record, prf->record_offset,
                inputs[ptf->inputs_ids[0]]);
#endif                
            if (ptf->default_input_to_field) {
                char *v;
                v = inputs[ptf->inputs_ids[0]];
                //printf("InputID of %s = %d  '%s'\n",ptf->name, ptf->inputs_ids[0], v);
                switch(prf->type) {
                    case MD_TYPE_FLOAT64:
                        * (double *) (ptr) = (double) atof(v);
                        break;
                    case MD_TYPE_INT32:
                        //v = inputs[pif->inputs_ids[0]];
                        * (int *) (ptr) = (long) atoi(v);
                        break;
                    case MD_TYPE_INT64:
                        //v = inputs[pif->inputs_ids[0]];
                        * (long long *) (ptr) = (long long) atoll(v);
                        break;
                    case MD_TYPE_INT08:
                        //v = inputs[pif->inputs_ids[0]];
                        * (unsigned char *) (ptr) = (unsigned char) atoi(v);
                        break;
                    case MD_TYPE_STRING:
                        //v = inputs[pif->inputs_ids[0]];
                        * (char **) (ptr) = v;
                        break;
                }    
            } else if (ptf->use_map_input) {
                char *key = inputs[ptf->inputs_ids[0]];
                if (ptf->use_map_input == INT_MAP_INPUT) {
                    int * v = map_get(&ptf->map_input_int,key);
                    if (!v) {
                        fatal("Fail to convert map from '%s' [%d]",key,i);
                    }
                    * (int *) (ptr) = (int) *v;
                } else {
                    double * v = map_get(&ptf->map_input_double,key);
                    if (!v) {
                        fatal("Fail to convert map from '%s' [%d]",key,i);
                    }
                    * (double *) (ptr) = (double) *v;
                }

            } else if (ptf->conv_to_field) {
                //char *v  = inputs[ptf->inputs_ids[0]];
                //printf("InputID of %s = %d  '%s'\n",ptf->name, ptf->inputs_ids[0], v); 
                ptf->conv_to_field(inputs, ptf->input_params, ptf->inputs_ids, ptr); 

            } else {
                fatal("Could not convert Input to Record: %s", ptf->name);
            }
        }
    }

}

/**
 * 
 * 
 * 
 */ 
_PUBLIC(int) metadata_process_line(PMetaData pmd, void *record, char *line, int no_crlf) {
    if (!line) return 0;

    if (!no_crlf) {
        // retira o \r\n
        int len = strlen(line);
        if (!len) return 0;

        if (line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
            if (!len) return 0;
        }

        if (line[len-1] == '\r') {
            line[len-1] = '\0';
            len--;
            if (!len) return 0;
        }
    }

    static char inputs[MD_INPUT_MAX_IDS][MD_INPUT_MAX_STRING];

    if (pmd->pii->type == XDATA_CSV) {
        get_inputs_from_line_with_sep(line, pmd, inputs);

    } else if (pmd->pii->type == XDATA_TEST) {
        get_inputs_from_test_line(line, pmd, inputs);

    } else {
        fatal("Error 102834");
    }

    process_xdata(inputs, pmd, record);
    // printf("[Trace]");fflush(stdout);

    return 1;
}


/**
 * 
 * 
 * 
 */ 
_PUBLIC(int) metadata_read(PMetaData pmd, void *record) {
    if (!pmd->driver_fin) {
        fatal("Trying to read from a input file not openned.\n");
    }

    char line[MAX_LINE_SZ+1];
    
    // le a linha
    if ( !fgets(line, MAX_LINE_SZ, pmd->driver_fin) ) return 0;
    //printf("%s",line);
    return metadata_process_line(pmd, record, line, FALSE);
}

