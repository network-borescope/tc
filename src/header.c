#include <stdlib.h>

#include "logit.h"
#include "common.h"

#include "stats.h"
#include "schema.h"

#define HEADER(PTR)  (((PHeader) PTR)->hdr)

/**
 * 
 * 
 * 
 */
int header_init(PNode pn, int beta, int type) {
    HEADER(pn).beta = beta;
    HEADER(pn).proper_child = 0;    
    HEADER(pn).type = type;
    return 1;
}

/**
 * 
 * 
 * 
 */
int header_deleted(PNode pn, int set)  {
    if (set) { 
        //prc->deleted = 1;
        HEADER(pn).deleted = 1; 

        //prc->proper_child = 0; 
        HEADER(pn).proper_child = 0; 
    }
    return HEADER(pn).deleted;
}

//=====================================[ DESTRUCTOR ]==============================

#define MAX_TYPES (1 << HEADER_FIELD_TYPE )
#define MAX_COUNT (1 << HEADER_FIELD_COUNTER)

typedef struct {
    header_destructor destructor;
} Type, *PType;

static int n_types = 0;
static Type types[MAX_TYPES];


/**
 * @brief 
 * 
 * @param destructor 
 * @return int 
 */
int header_register_type(header_destructor destructor) {
    if (n_types == MAX_TYPES) {
        fatal("Too many refcounter types registration: %d", n_types);
    }

    int type = n_types;
    n_types ++;
    types[type].destructor = destructor;

    return type;
}

/**
 * @brief 
 * 
 * @param pn 
 */
static void _destructor(PNode pn) {
    PType pt = types + HEADER(pn).type;
    pt->destructor(pn);
}

//========================================================================

//-----------------------
//
// Incrementa/decrementa o contador de referencia de <pn> em resposta 
// a criacao/remocao de um edge. 
// Se incrementa e edge for proper, entao reseta a falta de parent/owner 
// Se decrementa e edge for proper, entao seta a falta de parent/owner 
//
//  parent/owner:
//      2 - does not apply
//      1 - have it
//      0 - missed it
//
//
int header_adjust(PNode pn, int child, int shared, int inc) {
    int res = 0;

    // acrescimo
    if (inc > 0) {
        if (HEADER(pn).refcount == MAX_COUNT) fatal("Max Count reached!");
        HEADER(pn).refcount ++;
        #ifdef __STATS__
            if (HEADER_COUNT(pn) > stats.max_ref_counter) {
                stats.max_ref_counter = HEADER_COUNT(pn);
            }
        #endif
        //prc->refcount ++;

        if (child) {
            if (!shared) {
                // indica que setou o edge para PROPER
                res = !HEADER(pn).proper_child;
                //res = !prc->proper_child;

                HEADER(pn).proper_child = 1;
                //prc->proper_child = 1;
            }
        } else {
            // nada especial para betas
        }

    // decrescimo
    } else if (inc < 0) {
        if (!HEADER(pn).refcount) fatal("invalid edge removal");
        
        HEADER(pn).refcount--;

        if (HEADER(pn).refcount <= 0) {
            LOGIT_2("FREE NODE:: %03d  Header: %d",map_ptr(pn),HEADER_COUNT(pn));
            // zfree(pn);
            _destructor(pn);
            res = 1; // indica que zerou
            
        } else if (child) {
            if (!shared) {
                HEADER(pn).proper_child = 0;
            }

        } else {
            // nada especial para betas
        }

    // consulta ref_count
    } else {
       res = HEADER(pn).refcount;
    }
    return res;
}

/**
 * 
 * 
 * 
 **/ 
int header_make_proper_child(PNode pn) {
    if (!pn) return 0;

    if (HEADER(pn).deleted || HEADER(pn).proper_child) return 0;

    LOGIT_1("MAKED PROPER %03d",map_ptr(pn));
    #ifdef __STATS__
    stats.n_shared_to_proper ++ ;
    #endif
    HEADER(pn).proper_child = 1;

    return 1;
}

