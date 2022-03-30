/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   common.h
 * Author: nilson
 *
 * Created on August 27, 2018, 3:16 PM
 */

#ifndef COMMON_H
#define COMMON_H

#define _PRIVATE(X) static X
#define _PUBLIC(X)         X
#define UNUSED_VAR(X)   X __attribute__((unused))

#define UNUSED_RESULT(x) ((void) (x))

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <time.h>

#include "logit.h"
    
typedef unsigned char uint8;

    
void tracef(char *id, char *fmt,...);
void fatal(char *fmt, ...);

long timediff(clock_t t1, clock_t t2);

void map_reset(void);
int  map_ptr(void *ptr);
void map_free_ptr(void *ptr);

void initialize_randomizer(void);

//===============================

typedef void * PNode;

//===============================
void app_init(void);

//===============================
void     zalloc_init(void);
void     zalloc_reset_total_allocated(void);
int      zalloc_n_alloc(void);
long int zalloc_total_allocated(void);
void    *zmalloc(int sz);
void    *zcalloc(int n, int sz);
void     zfree(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */

