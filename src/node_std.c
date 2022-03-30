#include <assert.h>
#include <stdlib.h>
#include <memory.h>

#include "stats.h"
#include "pool.h"
#include "schema.h"

//=======================================================================================
//
//
//
//=======================================================================================

typedef struct __attribute__((__packed__)) s_arc {
    struct s_arc      *next;
    PNode             node;

//    int    deleted;
//    int    shared;
//    int    value;        
    struct {
        int    deleted: 1;
        int    shared:  1;
        int    value:   14;        
    } data;
} Arc, *PArc;

typedef struct __attribute__((__packed__)) s_vertix {
    Header rc;

//    int      shared_content;
//    int      beta_content;

    PNode    content;
    // struct s_arc      *shared_edges_first;
    PArc     arcs;
} Vertix, *PVertix; 

static int my_type = -1;

//=======================================================================================
//
//
//
//=======================================================================================

static PFreePool v_pool;
static PFreePool a_pool;

//-----------------------
//
//
//
//
static PVertix alloc_vertix(void) {
    return pool_alloc_item(v_pool);
}

//-----------------------
//
//
//
//
static PArc alloc_arc(void) {
    return pool_alloc_item(a_pool);
}


//-----------------------
//
//
//
//
static void  free_arc(PArc pa) {
    pool_free_item(a_pool, pa);
}

//=======================================================================================
//
//
//
//=======================================================================================

//-----------------------
//
//
//
//
static void _destructor(void * pn) {
    #ifdef __STATS__
        stats.n_nodes--;
        if (HEADER_GET_BETA(pn)) {
            stats.n_betas --;
        }
#if 0
        // trata o caso de nodes descartados antes de poderem virar proper
        if (!HEADER_GET_PROPER_CHILD(pn)) {
            stats.n_proper_child_edges ++;
            stats.n_shared_child_edges --;
        }
#endif
    #endif   
    LOGIT_1("NODE_STD: DESTRUCTING %03d",map_ptr(pn));

    #ifdef __LOGIT
    map_free_ptr(pn);
    #endif

    pool_free_item(v_pool, pn);
}


//-----------------------
//
//
//
//
static PNode _create_node(int beta) {
    PVertix pv = alloc_vertix();
    
    #ifdef __STATS__
        stats.n_nodes++;
        if (beta) {
            stats.n_betas ++;
        }
    #endif   

    if (my_type == -1) {
        my_type = header_register_type(_destructor);
    }
    header_init(pv, beta, my_type);
    return (PNode) pv;
}

//-----------------------
//
//
//
//
static int _delete_node(PNode pn) {
    return header_deleted(pn, TRUE);
}

#define CONTENT_NODE 0
#define CHILD_NODE   1

#define CONTENT_EDGE 0 
#define CHILD_EDGE   1

//-----------------------
//
// Adiciona um edge CHILD entre os nodes <pn> e <pchild> com valor <v>
// e <shared> ou proper
//
static PNode _add_child_edge(PNode pn, PNode pchild, int shared, Value v) {

    PArc pa = alloc_arc();

    pa->data.shared = shared;
    pa->data.value = v;
    pa->node = pchild;

    #ifdef __STATS__
        if (shared) { 
            stats.n_shared_child_edges ++; 
            if (HEADER_GET_TERMINAL(pa->node)) {
                stats.n_shared_child_terminals ++;
            }
        } else {
            stats.n_proper_child_edges ++;
            if (HEADER_GET_TERMINAL(pa->node)) {
                stats.n_proper_child_terminals ++;
            }
        }
    #endif

    // incrementa a contagem de referencias
    header_adjust(pchild, CHILD, shared, 1);
    
    pa->next = ((PVertix) pn)->arcs;
    ((PVertix) pn)->arcs = pa;
    return pchild;
}

//-----------------------
//
// Remove o node indicado por <pb>, fazendo <pb> apontar para o primeiro node de <pn>
// e retornado o <child> associado ao node.
//
//
static PNode _remove_child_edge (PNode pn, Branch *pb, int *shared) {
    PVertix pv = (PVertix) pn;
    if (!pb) fatal("pb");

    PArc pa = (PArc) (pb->p);

    // tem o anterior?
    if (!pb->prev) {
        pv->arcs = pa->next;
    } else {
        ((PArc) pb->prev)->next = pa->next;
    }
    
    #ifdef __STATS__
        if (pa->data.shared) { 
            stats.n_shared_child_edges --; 
            if (HEADER_GET_TERMINAL(pa->node)) {
                stats.n_shared_child_terminals --;
            }
        } else {
            stats.n_proper_child_edges --;
            if (HEADER_GET_TERMINAL(pa->node)) {
               stats.n_proper_child_terminals --;
            }
        }
    #endif

    LOGIT_2("NODE_STD: REMOVE CHILD EDGE %03d RefCount %d",map_ptr(pa->node),HEADER_COUNT(pa->node)-1);
    
    // ajusta as variaveis de edge do node
    LOGIT_DECL(int map_pa_node = map_ptr(pa->node));
    int released = header_adjust(pa->node, CHILD, pa->data.shared, -1);
    if (released) {
        LOGIT_1("   NODE_STD: RELEASED NODE %03d",map_pa_node);
    }           
    free_arc(pa);

    // primeiro elemento da lista
    pa = pv->arcs;
    
    // ajusta o branch para apontar para o primeiro node
    pb->p = pa;
    pb->prev = NULL;
#if 0
    if (pa) {
        if (header_make_proper_child(pa->node)) {
            pa->data.shared = 0;
        }
        if (shared) *shared = pa->data.shared;
    }
#endif    
    return pa? pa->node: NULL;
}

//-----------------------
//
//
//
//
static PNode _shallow_copy(PNode pn, int beta, struct s_node_api *pf) {
    PVertix pv = (PVertix) pn;
    PArc pa;
    PVertix pnew = (PVertix) _create_node(beta);
    
    // cria novos arcos shared apontando para os nodes já existentes
    for (pa = pv->arcs; pa; pa=pa->next) {
        assert(pa->node != NULL);
        pf->add_child_edge(pnew, pa->node, SHARED, pa->data.value); 
    }
    
    // Compartilha o content com o novo node
    pnew->content = pv->content;
    pnew->rc.hdr.shared_content = 1;
    pnew->rc.hdr.beta_content = pv->rc.hdr.beta_content;

    // incrementa o contador de uso do content
    header_adjust(pnew->content, CONTENT, PROPER, 1);

    return pnew;
}


//-----------------------
//
//
//
//
static PNode _replace_child_on_branch(Branch *branch, PNode pchild, int shared, PNodeFunctions pf) {
    PArc pa = branch->p;

    assert(pa != NULL);
    assert(pa->node != NULL);
    PNode next_node = pchild? pchild: pf->shallow_copy(pa->node, FALSE, pf);  
      
    pa->data.shared = shared;
    
    PNode pnc = pa->node;

    header_adjust(pnc, CHILD, TRUE, -1);
    pa->node = next_node;
    header_adjust(pa->node, CHILD, shared, 1);

    #ifdef __STATS__
        if (!shared) {
            stats.n_shared_child_edges --;
            stats.n_proper_child_edges ++;
            if (HEADER_GET_TERMINAL(pnc)) {
                stats.n_shared_child_terminals --;
                stats.n_proper_child_terminals ++;
            }
        }
    #endif

    return pa->node;
}

//-----------------------
//
//
//
//
static PNode _locate_child(PNode pn, Value v,int *shared, Branch *pb, 
                           struct s_node_api *pf) {
    PArc pa, p;

    for (pa = ((PVertix) pn)->arcs, p = NULL; pa; p = pa, pa = pa->next) {

        // skip it if  is not the desired value 
        if (pa->data.value != v) continue;

        if (HEADER_GET_GHOST(pa->node)) {

            LOGIT_DECL(int map_old = map_ptr(pa->node));
            LOGIT_2("LOCATE CHILD: REPLACING GHOST NODE %03d. Parent = %03d",
                map_old, map_ptr(pn));

            /* block */ {
                #ifdef __STATS__
                if (pa->data.shared) {
                    stats.n_shared_child_edges --;                    
                    stats.n_proper_child_edges ++;
                    if (HEADER_GET_TERMINAL(pa->node)) {
                        stats.n_shared_child_terminals --;
                        stats.n_proper_child_terminals ++;
                    }
                } else {

                }
                #endif

                PNode new = pf->shallow_copy(pa->node, NON_BETA, pf);

                // decrementa a contagem para o content antigo
                header_adjust(pa->node, CHILD, pa->data.shared, -1);


                pa->node = new;
                pa->data.shared = 0;

                // incrementa a contagem para o content antigo
                header_adjust(pa->node, CHILD,PROPER, 1);
            }

            LOGIT_3("LOCATE CHILD: REPLACED GHOST NODE %03d ~> %03d. Parent = %03d",
                map_old, map_ptr(pa->node), map_ptr(pn));
            
            PNode pcontent = pf->get_content_info(pa->node, NULL);

            // Troca um eventual ghost content
            if ((pcontent != pa->node) && HEADER_GET_GHOST(pcontent)) {

                LOGIT_2("LOCATE CHILD: SHALLOW-COPING GHOST CONTENT %03d. Parent = %03d",
                    map_ptr(pcontent), map_ptr(pa->node));

                struct s_node_api *pfcontent = pb->pf_child_content;

                PNode new_pcontent = pfcontent->shallow_copy(pcontent, BETA, pfcontent+1);
                
                pf->set_content_info(pa->node, new_pcontent, TRUE, PROPER, FALSE);

                LOGIT_3("LOCATE CHILD: NEW PROPER NON-GHOST CONTENT %03d ~> %03d. Parent = %03d",
                    map_ptr(pcontent), map_ptr(new_pcontent), map_ptr(pa->node));
#if 0
                // decrementa a contagem para o content antigo
                header_adjust(pcontent,CONTENT,0,-1);

                // incrementa a contagem para o content antigo
                header_adjust(new_pcontent,CONTENT,0,1);
#endif
            }
        }

        // retorna a informacao
        *shared = pa->data.shared;
        if (pb) {
                pb->p = pa;
                pb->prev = p;
        }
        
        return pa->node;
    }
    return NULL;
}


//-----------------------
//
//
//
//
void _remove_content_edge (PNode pn, PNodeFunctions pf) {
    PVertix pv = (PVertix) pn;

    // deveria limpar o campo content????
    // pv->content = NULL; //?????????

    // ajusta as variaveis de edge do node
    header_adjust(pv->content, CONTENT, 0, -1);
}

//-----------------------
// 
// returns
// 0 -> No children
// 1 -> One child
// 2 -> Two or more children
//
static int _get_children_class(PNode pn) {
    PVertix pv = (PVertix) pn;
    if (!pv->arcs) return 0;
    if (!pv->arcs->next) return 1;
    return 2;
}



//-----------------------
//
//
//
//
static PNode _get_content_info(PNode pn, int *shared) {
    PVertix pv = (PVertix) pn;

#if 0
    // transforma o ultimo content em proper
    if ((pv->content) && ( HEADER_COUNT(pv->content) == 1) {
        header_make_proper (pv->content, TRUE);
        pv->rc.hdr.shared_content = 1;
    }
#endif
    
    if (shared) *shared = pv->rc.hdr.shared_content;
    return pv->content;
}

//-----------------------
//
//
//
//
static PNode _set_content_info(PNode pn, PNode pcontent, int beta, int shared, int ignore) { //, struct s_node_api *pf) {
    PVertix pv = (PVertix) pn;

    // It didn't change anything? Just return
    if (pv->content == pcontent) {
        LOGIT_1("NODE STD: SET CONTENT INFO: NO CHANGES %p",pcontent);
        return pcontent;
    }

    // se existe content, entao decrementa a contagem para ele
    if (!ignore && pv->content /* && pv->content != pcontent */) {
        LOGIT_DECL(int rc = HEADER_COUNT(pv->content) - 1);
        LOGIT_3("NODE STD: ADJUST CONTENT %03d. RC = %d. Parent: %03d",
            map_ptr(pv->content), rc, map_ptr(pn));

        header_adjust(pv->content, FALSE, pv->rc.hdr.shared_content, -1);
    }


    pv->rc.hdr.shared_content = shared;
    pv->content = pcontent;
    pv->rc.hdr.beta_content = beta;

    // se existe um novo content, entao incrementa a contagem
    if (pcontent) {
        header_adjust(pcontent, FALSE, shared, 1);
        LOGIT_DECL(int rc = HEADER_COUNT(pv->content));
        LOGIT_3("NODE STD: ADJUST CONTENT %03d. RC = %d. Parent = %03d",
            map_ptr(pv->content), rc, map_ptr(pn));
    }
    
    return pcontent;    
}

//-----------------------
//
//
//
//
static PNode _get_child(PNode pn, Value *v, int *shared, Branch *ref, int change_proper, struct s_node_api *pf) {
    PVertix pv = (PVertix) pn;
    PArc pa;

    // primeira chamada
    if (pn) {
        pa = pv->arcs; if (!pa) return NULL;
        if (v) *v = pa->data.value;

#if 0
        // converts a shared child to a proper child if necessary
        if (change_proper && header_make_proper_child(pa->node)) {
            shared_to_proper(pn, pa, "STD GET CHILD");
        }        
#else

#endif    
        if (shared) *shared = pa->data.shared;
        if (ref) { ref->prev = NULL; ref->p = pa; }
        return pa->node;

    // chamadas seguintes
    } else if (ref) {
        pa = ref->p;
        pa = pa->next;

        ref->prev = ref->p; 
        ref->p = pa;
        if (!pa) return NULL;

        if (v) *v = pa->data.value;
#if 0
        // converts a shared child to a proper child if necessary
        if (change_proper && header_make_proper_child(pa->node)) {
            shared_to_proper(pn, pa, "STD GET CHILD");
        }        
#else
#endif    
        if (shared) *shared = pa->data.shared;
        return pa->node;        
    } else {
        return NULL;
    }
}





//-----------------------
//
//
//
//
static int _has_single_child (PNode pn) {
    PVertix pv = (PVertix) pn;
    
    return (pv->arcs != NULL) && (pv->arcs->next == NULL);
}


//-----------------------
//
//
//
//
void node_std_init_functions(NodeFunctions *pfs) {
    pfs->create_node = _create_node;
    pfs->delete_node = _delete_node;
    
    pfs->add_child_edge = _add_child_edge;
    pfs->remove_child_edge = _remove_child_edge;
    
    pfs->get_children_class = _get_children_class;
    
    pfs->locate_child = _locate_child;
    pfs->shallow_copy = _shallow_copy;
    pfs->replace_child_on_branch = _replace_child_on_branch;
    pfs->get_child = _get_child;

    pfs->get_content_info = _get_content_info;
    pfs->set_content_info = _set_content_info;

    pfs->has_single_child = _has_single_child;

    v_pool = pool_create(sizeof(Vertix), 1);
    a_pool = pool_create(sizeof(Arc), 1);
}

//-----------------------
//
//
//
//
void node_std_cleanup(void) {
    pool_reset(v_pool);
    pool_reset(a_pool);
}
