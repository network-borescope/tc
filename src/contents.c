#include <memory.h>
#include <stdlib.h>

#include "schema.h"
#include "metadata.h"
#include "register.h"

#include <stdio.h>

//=============================================================================================
//
//    
//
//=============================================================================================

typedef struct {
    int c;
} TopKData;

static void stream_top_k_register(void) {
    // SUM
    static ContentFunctions cfs;
    static ContentInfo ci;
    memset(&ci, 0, sizeof(ContentInfo));
    ci.n_param_fields = 1;
    ci.min_params = 0;
    ci.max_params = 0;
    ci.params_size = 0;
    ci.private_size = sizeof(TopKData);
    ci.pcfs = &cfs;

    cfs.parse_param = NULL;
    cfs.insert = NULL;
    cfs.remove = NULL;

    register_content("top-k", &ci);
}

//=============================================================================================
//
//    
//
//=============================================================================================
void init_contents(void) {
    // prevention against reinitialization during execution
    static int done = 0; 
    if (!done) done = 1; else return;


    stream_top_k_register();
}
