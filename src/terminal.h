#ifndef __TERMINAL__
#define __TERMINAL__

#include "schema.h"

void terminal_init_functions(TerminalNodeFunctions *tnfs);
void terminal_term_functions(TerminalNodeFunctions *tnfs);
void terminal_set_schema(PSchema ps);

int  terminal_insert(PSchema ps, PNode pn, void *precord);
int  terminal_remove(PSchema ps, PNode pn, void *precord);
int  terminal_get_count(PSchema ps, PNode pn);

#endif