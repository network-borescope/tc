#include <stdlib.h>
#include <memory.h>

#include <stdio.h>

#include "common.h"
#include "nodeset.h"

#include "stats.h"
#include "schema.h"
#include "terminal.h"

#include "logit.h"

//***************************************************************************
//
//    REMOVE: BETAS
//
//***************************************************************************

typedef struct s_node_link {
    PNode               pn;
    int                 shared;
    int                 idx_content;
    PNode               pcontent;
    struct s_node_link *next;
} NodeLink, *PNodeLink;

static PNodeLink betas_list = NULL;

/**
 * 
 * 
 * 
 */
static void betas_add(int shared, PNode pn, int idx_content, PNode pcontent) {
    
    // coloca o mais recente no inicio da lista
    PNodeLink pnl = calloc(sizeof(NodeLink),1);
    pnl->shared = shared;
    pnl->pn = pn;
    pnl->idx_content = idx_content;
    pnl->pcontent = pcontent;
    pnl->next = betas_list;
    betas_list = pnl;
}

#if 0
/**
 * 
 * 
 * 
 */
static void branch_assign(Branch *dst, Branch *src) {
    dst->prev = src->prev;
    dst->p = src->p;
}
#endif

//***************************************************************************
//
//    REMOVE: GHOST
//
//***************************************************************************

/**
 * 
 * 
 * 
 */
static int make_ghost(PSchema ps, int idx, PNode pchild, PNode pcontent) {
    PNode pn = pchild? pchild : pcontent;  

    HEADER_SET_GHOST(pn, 1);
    
    PNodeFunctions pf = ps->nfs+idx;

    // esta no nivel do terminal?
    if (idx == ps->n_values) {
        LOGIT_1("GHOST CALL - TERMINAL %03d", map_ptr(pn));

    } else {
        LOGIT_1("GHOST CALL - NODE %03d", map_ptr(pn));

        //-------------------------------
        //  content
        //-------------------------------
        int shared_content;
        PNode pcontent = pf->get_content_info(pn, &shared_content);
        if (!pcontent) {
            LOGIT_1("GHOST: CONTENT ALREADY REMOVED %03d", map_ptr(pn));
            fatal("FAIL TO SET GHOST CONTENT - CONTENT IS NULL");
        } else {
            LOGIT_2("GHOST: REMOVING CONTENT REFERENCE %03d [%p]", map_ptr(pcontent),pcontent);
            int idx_content = SCHEMA_IDX_CONTENT(ps,idx);

            if (!shared_content) {
                make_ghost(ps, idx_content, NULL, pcontent);
            }
        }
        
        //-------------------------------
        //  Children
        //-------------------------------
        PNode pchild;
        int shared; 
        Branch branch;
        
        // varre todos os filhos de pn
        LOGIT_1("START OF %03d CHILDREN",map_ptr(pn));

        pchild = pf->get_child(pn, NULL, &shared, &branch, TRUE, pf+1);
        if (!pchild) {
            LOGIT_1("GHOST: CHILDREN ALREADY REMOVED %03d", map_ptr(pn));
        } else {
            while (branch.p) {
                LOGIT_4("   GHOST: ANALIZING BRANCH TO %s CHILD %03d [%p]. RC=%d",
                    shared?"SHARED":"PROPER", map_ptr(pchild), pchild, HEADER_COUNT(pchild));

                if (!shared) {
                    make_ghost(ps, idx+1, pchild,  NULL);
                } else {
                    LOGIT_1("GHOST: SHARED CHILD %03d",map_ptr(pn));
                }       

                pchild = pf->get_child(NULL, NULL, &shared, &branch, FALSE, pf+1);
//                if (!pchild) break;
            }
            LOGIT_1("END OF %03d CHILDREN",map_ptr(pn));
        }
    }

    return 1;
}

//***************************************************************************
//
//    REMOVE: PURGE
//
//***************************************************************************



/**
 * 
 * 
 * 
 */
static int purge(PSchema ps, int idx, PNode pchild, PNode pcontent) {

    PNode pn = pchild? pchild : pcontent;   
    
    PNodeFunctions pf = ps->nfs+idx;

    // esta no nivel do terminal?
    if (idx == ps->n_values) {
        LOGIT_1("PURGE: PURGE TERMINAL %03d", map_ptr(pn));

    } else {
        LOGIT_1("PURGE: PURGE NODE %03d", map_ptr(pn));

        //-------------------------------
        //  Purge the content
        //-------------------------------
        PNode pcontent = pf->get_content_info(pn, NULL);
        if (!pcontent) {
            LOGIT_1("PURGE: CONTENT ALREADY REMOVED %03d", map_ptr(pn));
        } else {
            LOGIT_2("PURGE: REMOVING CONTENT REFERENCE %03d [%p]", map_ptr(pcontent),pcontent);
            int idx_content = SCHEMA_IDX_CONTENT(ps,idx);

            // remove a beta tree
            if (HEADER_COUNT(pcontent) == 1) {
                purge(ps, idx_content, NULL, pcontent);
            }

            // remove a contagem deste content
            pf->set_content_info(pn, NULL, 0, 0, FALSE);
        }
        
        //-------------------------------
        //  Purge Children
        //-------------------------------
        PNode pchild;
        int shared; 
        Branch branch;
        
        // varre todos os filhos de pn
        LOGIT_1("START OF %03d CHILDREN",map_ptr(pn));

        pchild = pf->get_child(pn, NULL, &shared, &branch, TRUE, pf+1);
        if (!pchild) {
            LOGIT_1("PURGE: CHILDREN ALREADY REMOVED %03d", map_ptr(pn));
            fatal("Non-terminal node without children");
        } else {
            while (branch.p) {

                LOGIT_4("   ANALIZING BRANCH TO %s CHILD %03d [%p]. RC=%d",
                    shared?"SHARED":"PROPER", map_ptr(pchild), pchild, HEADER_COUNT(pchild));

                if (HEADER_COUNT(pchild) == 1 ) {
                    purge(ps, idx+1, pchild,  NULL);
                } else {
                    LOGIT_1("  RC > 1, SHOULD DELETE CHILDREN OF %03d",map_ptr(pn));
                }

                pchild = pf->remove_child_edge(pn, &branch, &shared);
                
            }
            LOGIT_1("END OF %03d CHILDREN",map_ptr(pn));
        }
    }

    return 1;
}

/**
 * 
 * 
 * 
 */
static void purge_beta(PSchema ps, int shared, PNode pn, int idx_content, PNode pcontent) {

    LOGIT_2("**** PURGE BETA @%d: %03d",idx_content, map_ptr(pcontent));

    make_ghost(ps, idx_content, NULL, pcontent);

     // do not purge if still shared
    int rc = HEADER_COUNT(pcontent); 
    if (rc == 1) {
        
        // purge the node
        purge(ps, idx_content, NULL, pcontent);
    }

    
    // diminui a contagem para o content
    header_adjust(pcontent, TRUE, shared, -1);
}

/**
 * 
 * 
 * 
 */
static void purge_betas(PSchema ps) {
    PNodeLink p, pnext;
    for (p = betas_list; p; p = pnext) {
        pnext = p->next;
        purge_beta(ps, p->shared, p->pn, p->idx_content, p->pcontent);
        zfree(p);
    }
    betas_list = NULL;
}


//***************************************************************************
//
//    REMOVE
//
//***************************************************************************

/**
 * 
 * 
 * 
 */
static PNode remove_from_tree(PSchema ps, PNode root, int idx0, PAddress values, PObject o, PPointers visited, PNodeLink betas) {
    Value v;
    int shared;
    PNode pn, pchild;
    Branch branches[30];
    PNode nodes[30];
    PNodeFunctions pf, pf1;
    int idx, idx1;
    int bottom = 0;
    int deleting = 0;
    
    pn = root;
    LOGIT_2("+++ ENTER into %03d #%d;",map_ptr(pn), idx0);    

    nodes[idx0] = root;
    
    // repeat until reach the terminal vertix
    for (idx=idx0, idx1=idx + 1, pf = ps->nfs+idx0, pf1 = pf+1; ;idx++) {
        
        v = values[idx];
        
        // tries to locate the child with the value v
        branches[idx].pf_child_content = ps->nfs + SCHEMA_IDX_CONTENT(ps,idx+1);

        pchild = pf->locate_child(pn, v, &shared, &branches[idx], pf1);
        
        // not found
        if (pchild == NULL) {
            LOGIT_3("NOT FOUND[%02d] at %03d; #%d", v, map_ptr(pn), idx);  
            return root;
                
        // Does a shared child already exist?
        } else if (shared) {
            int was_deleted = header_deleted(pchild, FALSE);
            
            if (was_deleted) {
                deleting = 1;
            }
            
            LOGIT_5("FS[%02d] at %03d ~> %03d  Deleting=%d; #%d",v , map_ptr(pn), map_ptr(pchild), was_deleted, idx);  
            break;
                        
        // Had found a proper child
        } else {
            // do nothing
            // just continue the descent path
            LOGIT_4("FP[%02d]/DOWN at %03d ~> %03d. #%d",v,map_ptr(pn), map_ptr(pchild), idx);
        }
       
        // save the child found
        nodes[idx1] = pchild;

        // BREAK if idx1 is the terminal level!
        if (idx1 == ps->n_values) {
            LOGIT("BOTTOM");
            bottom = 1;
            break;
        }
        
        pn = pchild;
        pf   ++;
        pf1  ++;
        idx1 ++;
    }
        
    // chegou até o final ?
    if (bottom) {
        // pf stills points to node before terminal
        // pchild tem a folha
        // pf1 tem as funcoes de manipulacao de folha
        // adiciona o objeto ao node 
        int count = terminal_remove(ps, pchild, o);
        LOGIT_1("REMOVE OBJECT FROM ALPHA %03d",map_ptr(pchild));

        // ficou vazio?
        if (!count) {
            LOGIT_1("REMOVE ALPHA NODE %03d",map_ptr(pchild));
            pf1->delete_node(pchild);
            deleting = 1;
            #ifdef __STATS__
            if (idx0 == 0) {
                stats.n_primary_terminals --;
            }
            #endif
        }
    }

    if (!deleting) {
        LOGIT("UPWARD;");
    } else {
        LOGIT("UPWARD DELETING;");
    }
    
    //------------------------------------------------------
    // retorna pelo caminho percorrido ajustando os "content"
    //------------------------------------------------------
    for (; idx >= idx0; idx--) {
        pn = nodes[idx];
        
        LOGIT_2("LOOP IDX=%d NODE at %03d;", idx, map_ptr(pn));  

        // computes the index of content
        int idx_content = ps->top_of_dim[ps->dim_of_value[idx] + 1];
        
        // prepare content stuff
        int shared_content;
        PNode pcontent = pf->get_content_info(pn, &shared_content);
        
        // deletou o node inferior?
        if (deleting) {
            // deleta o branch que levava ao node deletado
            // atualiza pchild para algum child node do node atual
            // util para quando sobrar apenas 1 filho
            pf->remove_child_edge(pn, &branches[idx], NULL);
            LOGIT_2("REMOVE BRANCH at %03d -> %03d;", map_ptr(pn), map_ptr(pchild));  
        }

        // avalia quantos elementos ficaram no node apos 
        // uma eventual delecao do branch
        int n = pf->get_children_class(pn);

        // ficou com mais de um filho?
        if (n > 1) { 
            LOGIT("AAA");
            // se eh proper content ou se shared mas ainda nao processado?
            if (!shared_content || !pointers_count(visited, pcontent)) {
                if (shared_content) fprintf(stderr,"tc_remove: processing shared content.\n");
                LOGIT_1("VISITING BETA NODE %03d",map_ptr (pcontent));
                if (idx_content == ps->n_values) {
                    terminal_remove(ps, pcontent, o);
                    //LOGIT_DECL(int v);
                    //LOGIT_DECL(ps->nfs.get_content_data(pcontent, &v));
                    //LOGIT_2("REMOVING OBJECT FROM %03d count = %d",map_ptr (pcontent), v);
                } else {
                    remove_from_tree(ps, pcontent, idx_content, values, o, visited, betas);
                }
                pointers_insert(visited, pcontent);
            }
            deleting = 0;

        // No offspring? (implicates "deleting" mode)
        } else if (n == 0) {
            LOGIT("BBB");
            // sinalizes the end of reference to the content node
            // allowing the decrement of refcount in content node
            pf->set_content_info(pn, NULL, 0, 0, FALSE);
            
            if (idx == idx0) {
                LOGIT_1("TRYING TO DELETING ROOT NODE %03d",map_ptr (pn));
            } else {
                LOGIT_1("DELETING NODE %03d",map_ptr (pn));
                pf->delete_node(pn);
            }
            deleting = 1; // desnecessario but just in case
            
        // ja tinha apenas um filho 
        } else if (!deleting) {
            LOGIT("CCC");
            LOGIT_DECL(PNode old = pcontent);
            LOGIT_DECL(int map_old = map_ptr(old));
            
            // Is the child the start of a chain?
            if (idx + 1 == idx_content) {
                pcontent = pchild;
            } else {
                // set this node content as the content of its unique child
                pcontent = pf1->get_content_info(pchild, NULL);
            }
            // 
            pf->set_content_info(pn, pcontent, FALSE, SHARED, FALSE); // @@
            
            LOGIT_4("Adjust Single Child at %03d. %03d -> %03d. RC = %d", map_ptr(pn),
                    map_old, map_ptr(pcontent), HEADER_COUNT(pcontent));
            
            deleting = 0;
            
        // ficou com apenas um filho apos deleted 
        } else {
            LOGIT("DDD");
            //---------- part A
            // trata a arvore beta
            LOGIT_DECL(PNode pold = pcontent);
            LOGIT_DECL(int map_old = map_ptr(pold));
            
            if (!shared_content) {
                // primeiro, ajusta os contadores na arvore beta
                if (idx_content == ps->n_values) {
                    terminal_remove(ps, pcontent, o);
                    LOGIT_1("REMOVE OBJECT FROM TERMINAL BETA %03d",map_old);
                } else {
                    remove_from_tree(ps, pcontent, idx_content, values, o, visited,betas);
                }
            }
            
            // depois faz o purge na arvore ja ajustada
            LOGIT_2("ADD TO BETAS @%d: %03d",idx_content, map_ptr(pcontent));
            betas_add(shared_content, pn, idx_content, pcontent);                
            
            //---------- part B
            // faz o content de "pn" ser o mesmo content do filho, 
            // exceto se o filho já é um novo inicio de chain, 
            // neste caso o content de "pn" é o proprio filho

            // pega o node do filho restante
            pchild = pf->get_child(pn, NULL, NULL, NULL, TRUE, pf1);

            // the child is the content?
            if (idx + 1 == idx_content) {
                pcontent = pchild;
            } else {
                // set this node content as the content of its unique child
                pcontent = pf1->get_content_info(pchild,NULL);
            }
            LOGIT_4("2 -> 1 (shared) at %03d: %03d -> %03d; RC = %d", 
                    map_ptr(pn), map_old, map_ptr(pcontent), HEADER_COUNT(pcontent));  

            // ajusta o content
            pf->set_content_info(pn, pcontent, FALSE, SHARED, TRUE);

            deleting = 0;
        }

        pchild = pn;
        pf --;
        pf1 --;
    }
    LOGIT_1("--- LEAVE %03d",map_ptr(root));

    return root;
}

//***************************************************************************
//
//    TINYCUBES_REMOVE
//
//***************************************************************************
#include "dump1.h"

#ifdef DUMP_REMOVE
static int rem = 0;
char buf[1000];
#endif

#ifdef __STATS__
#include <sys/resource.h>
#endif

/**
 * 
 * 
 * 
 */
PNode tinycubes_remove(PSchema ps, PNode root, PAddress address, PObject o) {
    
    PPointers visited = pointers_create(128);
    
    #ifdef __STATS__
        stats.n_removals ++;
    #endif

    root = remove_from_tree(ps,root,0,address,o, visited, betas_list);
    if (!root) {
        LOGIT("ROOT DELETED!");
        fatal("ROOT DELETED!");
    }
    
    pointers_destroy(visited);

#ifdef DUMP_REMOVE
    rem ++;
    sprintf(buf,"dump_remove_%02d.dot",rem);
    dump(ps,root,buf,buf);
    sprintf(buf,"dot -Tpng dump_remove_%02d.dot -o dump_remove_%02d.png -Gsize=30,24! -Gdpi=72",rem,rem);
    system(buf);
#endif
    
    purge_betas(ps);

    #ifdef __STATS__
        struct rusage r_usage;
        getrusage(RUSAGE_SELF,&r_usage);
        
        if (r_usage.ru_maxrss > stats.max_memory_used_kb) {
            stats.max_memory_used_kb = r_usage.ru_maxrss;
        } 
        stats.acc_memory_used_kb += r_usage.ru_maxrss;
        stats.count_memory_used ++;
    #endif 
    
    return root;
}
