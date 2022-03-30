/* 
 * File:   node_std.h
 * Author: nilson
 *
 * Created on August 27, 2018, 6:40 PM
 */

#ifndef NODE_STD_H
#define NODE_STD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "schema.h"
    
void node_std_init_functions(NodeFunctions *pfs);
void node_std_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* NODE_STD_H */

