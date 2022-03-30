#ifndef __QUERY_PROCESSOR
#define __QUERY_PROCESSOR

#include <setjmp.h>
#include "metadata.h"
#include "schema.h"
#include "opaque.h"

extern jmp_buf query_jmp;

PQuery query_begin(void);
int    query_parse(PQuery pq, PMetaData pmd, PSchema ps, char *src);
void   query_fatal(char *fmt, ...);

int    query_process(PQuery pq, char *src, PSchema ps, PMetaData pmd);
void   query_end(PQuery pq);

typedef int (*printf_t) (const char * fmt, ...);
void query_out_as_json(PQuery pq, printf_t _printf_f);

#endif