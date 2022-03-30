/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   logit.c
 * Author: nilson
 * 
 * Created on September 19, 2018, 12:51 PM
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "logit.h"
 
static FILE *flog = NULL;
static char filename[1024];

void logit_set_filename(char *fname) {
    strcpy(filename,fname);
    if (flog) {
        fclose(flog);
        flog = fopen(filename,"wt");
    }
}


void logit(char *msg,...) {
    va_list args;

    va_start(args, msg);
    if (!flog) {
        flog = fopen(filename,"wt");
    }
    
    if (msg) {
        vfprintf(flog,msg,args);
        fprintf(flog,"\n");
        fflush(flog);
    }
    va_end(args);

}

