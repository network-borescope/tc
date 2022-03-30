/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "common.h"

long timediff(clock_t t1, clock_t t2) {
    long ms;
    ms = ((double)t2 - t1) / CLOCKS_PER_SEC * 1000;
    return ms;
}

//=====================================================

void fatal(char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    if (fmt) {
        vfprintf(stderr,fmt,args);
        fprintf(stderr,"\n");
    }
    va_end(args);
    exit(99);
}

#define TRACESTR "cd;shc;nb;na;nm;"

void tracef(char *id, char *fmt,...) {
    return;
#if 0
    va_list args;

    va_start(args, fmt);
    if (strstr(TRACESTR,id)) {
        vprintf(fmt, args);
    }
    va_end(args);
#endif    
}  

static int id_ptr = 0;

typedef struct s_ptr_hash {
    int    id;
    void   *ptr;
    struct s_ptr_hash *next;
} *PHash, Hash;

#define PTR_HASH_SZ 1021
static int hash_ready = 0;
static PHash ptr_hash[PTR_HASH_SZ];

void map_free_all(void) {
    int i;
    PHash p, q;
    for (i=0; i<PTR_HASH_SZ; i++ ) {
        for (p=ptr_hash[i]; p; p=q) {
            q = p->next;
            free(p);
        }
        ptr_hash[i] = NULL;
    }
}

void map_reset(void) {
    if (!hash_ready) {
        memset(ptr_hash, 0, sizeof(PHash) * PTR_HASH_SZ);
        hash_ready = 1;
    } else {
        map_free_all();
    }
    id_ptr = 0;
}


int  map_ptr(void *ptr) {
    if (id_ptr == 0) {
        memset(ptr_hash, 0, sizeof(PHash) * PTR_HASH_SZ);
    }
    
    int pos = ((long int) ptr) % PTR_HASH_SZ;
        
    PHash p;

    for (p = ptr_hash[pos]; p; p = p->next) {
        if (p->ptr == ptr) return (p->id);
    }
    
    id_ptr ++;
    p = malloc(sizeof(Hash));
    p->id = id_ptr;
    p->ptr = ptr;
    p->next = ptr_hash[pos];
    ptr_hash[pos] = p;
    return p->id;
}

void  map_free_ptr(void *ptr) {
    int pos = ((long int) ptr) % PTR_HASH_SZ;
        
    PHash p, p0;

    for (p = ptr_hash[pos], p0 = NULL; p; p0 = p, p = p->next) {
        if (p->ptr == ptr) {
            if (p0) {
                p0->next = p->next;
            } else {
                ptr_hash[pos] = p->next;
            }
            p->ptr = NULL;
            free(p);
            return;
        }
    }
    fatal("PTR not found");
}


void initialize_randomizer(void) {
static int done = 0;
    if (done) return;
    srand(time(NULL)); 
    done = 1;  
}

//=====================================================

static int n_allocs;
static long int total_allocated;

void zalloc_init(void) {
    n_allocs = 0;
    total_allocated = 0;
}

void zalloc_reset_total_allocated(void) {
    total_allocated = 0;
}

int zalloc_n_alloc(void) {
    return n_allocs;
}

long int zalloc_total_allocated(void) {
    return total_allocated;
}

void *zmalloc(int sz) {
    n_allocs ++;
    total_allocated += sz;
    return malloc(sz);
}

void *zcalloc(int n, int sz) {
    n_allocs ++;
    total_allocated += n * sz;
    return calloc(n, sz);
}

void zfree(void *ptr) {
    n_allocs --;
    free(ptr);
}

//==========================================================
//
//  INITIALIZATION
//
//==========================================================

void init_register();
void init_contents();
void init_contents_stats(void);
void register_class_cat();
void register_class_geo();
void register_container_binlist(void);
void register_container_binlist_addonly(void);

void app_init(void) {
    init_register();

    init_contents();
    init_contents_stats();

    register_class_cat();
    register_class_geo();

    register_container_binlist();
    register_container_binlist_addonly();

}

