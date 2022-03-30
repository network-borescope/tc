#ifndef __CLASS_PREFIX
#define __CLASS_PREFIX

union {
    int     i;
    double  d;
    void   *p;
} UClassPrefixParam;

#define   CLASS_PREFIX_N_PARAMS  4
#define   CLASS_PREFIX_PARAMS    UClassPrefixParam class_param[CLASS_PREFIX_N_PARAMS];

#endif