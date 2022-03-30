#ifndef __INPUT
#define __INPUT

#include <stdio.h>

#define  INPUT_MAX_FIELDS 100
#define  INPUT_MAX_LENGTH 300


typedef struct {
    struct {    
        int insert;
        int remove;
        int show;
        int plot;
        int n_fields;
    } s;

    char fields[INPUT_MAX_FIELDS][INPUT_MAX_LENGTH];
} InputRecord, *PInputRecord;


typedef int (*input_read_t)(struct s_input * pi, PInputRecord pir);
typedef int (*input_term_t)(struct s_input * pi);

typedef struct s_input {    
    int max_fields;

    input_read_t f_read;
    input_term_t f_term;
    void *driver_data;

} Input, *PInput;


#endif

#include <stdlib.h>
#include <memory.h>
#include "common.h"
// #include "input.h"

//=========================[ WORDS ]==========================

#define MAX_LINE 4096

typedef struct {
    FILE *f;
    char line[MAX_LINE+1];
} InputWords, *PInputWords;

/**
 * 
 * 
 * 
 */ 
int words_input_read(struct s_input * pi, PInputRecord pir) {

    PInputWords p = (PInputWords) pi->driver_data;
    memset(&pir->s,0,sizeof(pir->s));

    fgets(p->line, MAX_LINE, p->f);
    
    int idx = -1;

    for (char *c=p->line;*c; c++) {
        if (*c >= 'A' && *c<='Z') {
            if (idx<0) {
                idx = 0;
            }
            pir->fields[idx][0] = *c - 'A';
            pir->fields[idx][1] = '\0';
            idx ++;

        } else if (*c == '+') {
            if (idx >=0) fatal("Unexpected char '%c'",*c);

            pir->s.insert = 1;
            pir->s.remove = 0;

        } else if (*c == '-') {

        }
    }

    pir->s.n_fields = idx;
    return idx;
}

/**
 * 
 * 
 * 
 */ 
int words_input_term(PInput pi) {
    free(pi->driver_data);
    pi->driver_data = NULL;
    return 1;
}

/**
 * 
 * 
 * 
 */ 
int words_input_init(PInput pi) {
    pi->driver_data = calloc(1, sizeof(InputWords));
    pi->f_read = words_input_read;
    pi->f_term = words_input_term;
    return 1;
}

//=========================[ CSV ]==========================

#include <string.h>

/**
 * 
 * 
 * 
 */ 
PInput input_create(char *driver_name) {
    PInput pi = calloc(1,sizeof(Input));
    
    if (strcmp(driver_name,"words")==0) {
        words_input_init(pi);

    } else if (strcmp(driver_name,"csv")==0) {
        words_input_init(pi);
    
    } else {
        free(pi);
        pi = NULL;
    }
    return pi;
}


/**
 * 
 * 
 * 
 */ 
_PUBLIC(int) input_read(PInput pi, PInputRecord pir) {
    return pi->f_read(pi, pir);
}


/**
 * 
 * 
 * 
 */ 
_PUBLIC(void) input_destroy(PInput pi) {
    pi->f_term(pi);
    free(pi);
}


