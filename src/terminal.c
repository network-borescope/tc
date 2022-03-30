#include <assert.h>
#include <stdlib.h>
#include <memory.h>

#include "stats.h"
#include "schema.h"

static PSchema cur_schema;

//-----------------------
//
//
//
//
void terminal_set_schema(PSchema ps) {
    cur_schema = ps;
}


//=============================================================================

PSchema ps;

#include <stdio.h>

//-----------------------
//
//
//
//
static PTerminal alloc(void) {
    int sz = sizeof(Terminal);
    assert(cur_schema);
    assert(cur_schema->pti);
    sz += cur_schema->pti->total_data_size;
//    printf("ALLOC: %d\n", sz );
    return zcalloc(sz,1);
}

//=============================================================================

static int my_type = -1;

//-----------------------
//
//
//
//
static void _destructor(void * pn) {
    #ifdef __STATS__
        stats.n_nodes --;
        stats.n_terminals --;
        if (HEADER_GET_BETA(pn)) {
            stats.n_betas --;
            stats.n_beta_terminals --;
        }
    #endif
    LOGIT_1("TERMINAL COUNT: DESTRUCTING %03d",map_ptr(pn));
    #ifdef __LOGIT
    map_free_ptr(pn);
    #endif
    free(pn);
}


//=============================================================================

//-----------------------
//
//
//
//
static PNode _create_terminal_node(PSchema ps, int beta) {
    PTerminal pt = alloc();

    if (my_type == -1) {
        my_type = header_register_type(_destructor);
    }

    header_init(pt, beta, my_type);
    HEADER_SET_TERMINAL(pt,1);

    #ifdef __STATS__
        stats.n_nodes ++;
        stats.n_terminals ++;
        if (beta) {
            stats.n_betas ++;
            stats.n_beta_terminals ++;
        }
    #endif

    int i;
    PTerminalInfo pti = cur_schema->pti;
    PTerminalChannel * pptc = pti->p_slots;
    for (i = 0; i < pti->n_channels; i++, pptc++) {
        PTerminalChannel ptc = *pptc;
        if (ptc->pcfs->prepare) {
            //printf("PREPARE\n");
            ptc->pcfs->prepare(TERMINAL_OP_INIT, pt->data + ptc->data_offset, &ptc->tfd);
        }
    }

    return pt;

}

//-----------------------
//
//
//
//
static PNode _create_node(int beta) {
    assert(cur_schema);
    return _create_terminal_node(cur_schema, beta);
}

//-----------------------
//
//
//
//
static int _delete_node(PNode pn) {
    header_deleted(pn, TRUE);

    return 1;
}

//-----------------------
//
//
//
//
static PNode _shallow_copy(PNode pn, int beta, struct s_node_api *pf) {
    LOGIT_1("TERMINAL: SHALLOW COPY %03d", map_ptr(pn));

    PTerminal pt = _create_node(beta);

    // copia o content (contagem) para o novo node
    pt->counter = ((PTerminal) pn)->counter;

    stats.terminal_shallow_copies ++;
    
    int i;
    PTerminalInfo pti = cur_schema->pti;
    PTerminalChannel * pptc = pti->p_slots;
    PByte src = ((PTerminal) pn)->data;
    PByte dst = pt->data;
    for (i = 0; i < pti->n_channels; i++, pptc++) {
        PTerminalChannel ptc = *pptc;
        if (!ptc->pcfs->shallow_copy) {
            memcpy(dst, src, ptc->data_size);
        } else {
            ptc->pcfs->shallow_copy(dst, src, ptc->data_size);
        }
        src += ptc->data_size;
        dst += ptc->data_size;
    }


        
    return (PNode) pt;
}

//=============================================================================

//-----------------------
//
//
//
//
static PNode _get_content_info(PNode pn, int *shared) {
    if (shared) *shared = 0;
    LOGIT("TERMINAL: ABNORMAL CALL TO GET CONTENT INFO FOR TERMINAL");
    return pn;
}

//-----------------------
//
//
//
//
static PNode _set_content_info(PNode pn, PNode pcontent, int beta, int shared, int ignore) { //zstruct s_node_api *pf) {
    // Nao faz testes para ver se está "resetando" o content info 
    // por achar que nao sera chamada duas vezes para um terminal
    // porem nao esta provado
    LOGIT("TERMINAL: ABNORMAL CALL TO SET CONTENT INFO FOR TERMINAL");
    
    header_adjust((PHeader) pn, beta, shared,1);
    return pn;    
}

//=============================================================================

#define ALTER_INSERT 1
#define ALTER_REMOVE 2

static void do_change(int what, PSchema ps, PNode pn, PRecord precord, PTerminal pt) {
    PTerminalInfo pti = ps->pti;

    PTerminalChannel *pptc, *pptc2;
    PTerminalChannel ptc, ptc2;
    PByte data;
    int i, j;

    // printf("N:%d\n",pti->n_channels);
    for (i=0, pptc = pti->p_slots; i< pti->n_channels; i++, pptc++) {
        ptc = *pptc;
        assert(ptc);
        assert(ptc->pcfs);
        assert(ptc->pcfs->insert);

        //printf("TERMINAL CHANGE: %d\n",ptc->data_offset);
        if (what == ALTER_INSERT) {
            // printf("[INC 1] %p %d\n",pt->data + ptc->data_offset, ptc->data_offset);
            data = ptc->pcfs->insert(precord, pt->data + ptc->data_offset, &ptc->tfd);
        } else {
            data = ptc->pcfs->remove(precord, pt->data + ptc->data_offset, &ptc->tfd);
        }

        if ((ptc->n_channels>0)) {
            for (j=0, pptc2 = pti->p_slots + ptc->base_slot; j < ptc->n_channels; j++, pptc2++) {
                ptc2 = *pptc2;
                if (what == ALTER_INSERT) {
                    // printf("[INC 2] %p %d\n",data, ptc2->data_offset);
                    ptc2->pcfs->insert(precord, data + ptc2->data_offset, &ptc2->tfd);
                } else {
                    ptc2->pcfs->remove(precord, data + ptc2->data_offset, &ptc2->tfd);
                }
            }
        }
    }
}

//-----------------------
//
//
//
//
int terminal_insert(PSchema ps, PNode pn, PRecord precord) {
//    printf("TERMINAL INSERT\n");
    PTerminal pt = (PTerminal) pn;

    pt->counter ++;
    do_change(ALTER_INSERT, ps, pn, precord, pt);
    
    return pt->counter;
}

//-----------------------
//
//
//
//
int  terminal_remove(PSchema ps, PNode pn, PRecord precord) {
    PTerminal pt = (PTerminal) pn;
    if (!pt->counter) return 0;

    pt->counter --;
    do_change(ALTER_REMOVE, ps, pn, precord, pt);

    return pt->counter;
}

//-----------------------
//
//
//
//
int  terminal_get_count(PSchema ps, PNode pn) {
    PTerminal pt = (PTerminal) pn;
    return pt->counter;
}

//=============================================================================

//-----------------------
//
//
//
//
void terminal_init_functions(TerminalNodeFunctions *tnfs) {
    tnfs->create_node = _create_node;
	tnfs->delete_node = _delete_node;
	
    tnfs->get_content_info = _get_content_info;
    tnfs->set_content_info = _set_content_info;

    tnfs->shallow_copy = _shallow_copy;
} 


//-----------------------
//
//
//
//
void terminal_term_functions(TerminalNodeFunctions *tnfs) {
    return;
}

typedef struct {
    PTerminalInfo pti;
    Byte  Data[];
} Merge, *PMerge;

//***************************************************************************
//
//    ContentInfo
//
//***************************************************************************


#if 0
//-----------------------
//
//
//
//
PMerge terminal_begin_merge(PTerminalInfo pti, int channel, int sub_channel, int alt_channel) {
    PMerge pm = calloc(1, sizeof(Merge));
    pm->pti = pti;
    return pm;
}

//-----------------------
//
//
//
//
PMerge terminal_do_merge(PNode pterminal, PMerge pm) {
    
}

//-----------------------
//
//
//
//
PMerge terminal_end_merge(PMerge pm) {

}
#endif