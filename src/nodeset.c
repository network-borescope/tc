/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdlib.h>
#include <memory.h>

#include "logit.h"
#include "nodeset.h"

static int  bits_to_size [] = {
		31,   //  0  1
		31,   //  1  2
		31,   //  2  4
		31,   //  3  8
		31,   //  4  16
		67,   //  5  32
		67,   //  6  64
		131,  //  7  128
		257,  //  8  256
		521,  //  9  512
		1009, // 10  1024
		2053, // 11  2048
		4099, // 12  4096
		8209, // 13  8192
		16411,// 14  16384
		32771,// 15  32768
		65537 // 16  65536
};

static PHashNode hnodes = NULL;

/**
 * 
 * 
 * 
 */
PPointers pointers_create(int capacity) {
	PPointers ps = calloc(sizeof(Pointers),1);
	int bits = 0; 
	while (capacity > 0) {
		capacity >>= 1;
		bits ++;
	}
	if (bits > 16) bits = 16;
	ps->sz = bits_to_size[bits];	
	ps->entries = calloc(sizeof(HashEntry), ps->sz);
	return ps; 
}


/**
 * 
 * 
 * 
 */
void pointers_destroy(PPointers ps) {
    PHashEntry pe; PHashNode pn, pn2;

	// esvazia todos os nodes
    for (pe = ps->used_entries; pe; pe = pe->next) {
        for (pn = pe->first; pn; pn = pn2) {
            pn2 = pn->next;

			// encadeia pn em hnodes
			pn->next = hnodes;
			hnodes = pn;
        }
    }
	free(ps->entries);
    free(ps);
}


/**
 * 
 * 
 * 
 */
int  pointers_count(PPointers pns, void *ptr) {    

	int pos = ((long int) ptr) % pns->sz;
        
    // 
    PHashEntry p; PHashNode pn;
    for (p = pns->entries + pos, pn = p->first; pn; pn = pn->next) {
        if (pn->ptr == ptr) return pn->counter;  // already exist
    }

    return 0;
}


/**
 * 
 * 
 * 
 */
int  pointers_insert(PPointers pns, void *ptr) {    
    int pos = ((long int) ptr) % pns->sz;
        
    // 
    PHashEntry p; PHashNode pn;
    for (p = pns->entries + pos, pn = p->first; pn; pn = pn->next) {
        if (pn->ptr == ptr) {
            pn->counter ++;
            return 0;  // already exist
        }
    }

    // Is the entry not inside used_entries?
    if (!p->first) {
        p->next = pns->used_entries;
        pns->used_entries = p;
    }
    
	// get a hash node from pool or perform actual allocation
	if (!hnodes) {
    	pn = malloc(sizeof(HashNode));
	} else {
		//
		pn = hnodes;
		hnodes = pn->next;
	}

    // insert ptr in hash's node list
    pn->ptr = ptr;
    pn->counter = 1;
    pn->next = p->first;

    p->first = pn;

    return 1;
}


/**
 * 
 * 
 * 
 */
int  pointers_remove(PPointers ps, void *ptr) {    
    int pos = ((long int) ptr) % ps->sz;

    // 
    PHashEntry pe; PHashNode pn, pna;
    for (pe = ps->entries + pos, pn = pe->first, pna=NULL; pn; pna = pn, pn = pn->next) {
		// already exist?
		if (pn->ptr == ptr) {
			LOGIT("NODESET_REMOVE");
			if (pna) {
				pna->next = pn->next;
			} else {
				pe->first = pn->next;
			}

			// encadeia pn em hnodes
			pn->next = hnodes;
			hnodes = pn;

			free(pn);
			return 1;  
		}
    }

	return 0;
}


/**
 * 
 * 
 * 
 */
void pointers_cleanup(void) {
	PHashNode ph, ph2;

	for (ph = hnodes; ph; ph = ph2) {
		ph2 = ph->next;
		free(ph);
	}

	hnodes = NULL;
}
