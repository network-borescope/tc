#ifndef __CLASS_PREFIX
#define __CLASS_PREFIX

union {
    int     i;
    double  d;
    void   *p;
} UCommonClassParam;

#define   N_COMMON_CLASS_PARAMS  4
#define   COMMOM_CLASS_PARAMS    UCommonClassParam class_params[N_COMMON_CLASS_PARAMS]

#endif