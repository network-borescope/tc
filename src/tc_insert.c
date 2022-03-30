#include <stdlib.h>
#include <memory.h>

#include <stdio.h>

#include "common.h"
#include "nodeset.h"

#include "stats.h"
#include "schema.h"
#include "terminal.h"

#include "logit.h"

#if 0
//***************************************************************************
//
//    DESHARE
//
//***************************************************************************
typedef struct s_deshare{
    PNode  old;
    PNode  new;
    struct s_deshare * next;
} DeshareEntry, *PDeshareEntry;

static PDeshareEntry deshare_list = NULL;


/**
 * 
 * 
 * 
 */
static PNode deshare_map(PNode pn, PNode pnew) {
    PDeshareEntry p, pnext;
    
    if (!pn) {
        // esvazia a lista
        for (p = deshare_list; p; p = pnext) {
            pnext = p->next;
            free(p);
        }
        deshare_list = NULL;
        return NULL;
    } 
    
    
    // tenta localizar
    for (p = deshare_list; p; p = p->next) {
        if (p->old == pn) {
            if (!pnew) return p->new; // existe um mapeamento
            if (pnew == p->new) return p->new;
            LOGIT_1("duplicidade %d",map_ptr(pnew));
            // fatal("duplicidade");
            return pnew;
        }
    }
    if (!pnew) return NULL;
    
    p = calloc(1,sizeof(DeshareEntry));
    p->old = pn;
    p->new = pnew;
    p->next = deshare_list;
    deshare_list = p;
	return NULL;
}
#endif

#define __NANO__NO


//***************************************************************************
//
//    INSERT
//
//***************************************************************************

#ifndef __NANO__
static void copy_nodes(PNode *dst, PNode *src,int nvalues, int ichild) {
    memcpy(dst+ichild, src+ichild, (nvalues - ichild + 1) * sizeof (PNode));
}
#endif

#define PROPER 0
#define SHARED 1


//***************************************************************************
//
//    INSERT
//
//***************************************************************************

#define __EXPERIMENTAL__NO

#define GET_PNODE(PCHILD,PBASE,ICHILD) \
{PTree pt; for(pt = (PBASE); pt->max < (ICHILD); pt++); (PCHILD) = pt->nodes[(ICHILD)];} 

#define SET_PNODE(PBASE,ICHILD,PCHILD) \
{ (PBASE)->nodes[ICHILD] = (PCHILD); (PBASE)->max = (ICHILD);}

/**
 * 
 * 
 * 
 */
static int insert_into_tree(PSchema ps, PNode proot, int iroot, PAddress options, 
                     PTree ptree, PTree pbase, PObject po, 
                     PPointers visited, PPointers increased) {
    Value v;
    int shared;
    PNode pn, pchild;
    Branch branch;
    PNodeFunctions pf, pf1;
    int inode, ichild;
    int bottom = 0;
#ifdef __EXPERIMENTAL__
PNode pchild2;
#endif    

    int already_increased = 0; // 2019-02-13

    pn = proot;
    LOGIT_2("+++ ENTER into %03d #%d;",map_ptr(pn), 0);    
    ptree->op = OP_NONE;
    
    SET_PNODE(ptree, iroot, pn);
    ptree->nodes[iroot] = pn;
 
    // repeat until reach the terminal vertix
    for (inode=iroot, ichild=inode + 1, pf = ps->nfs+iroot, pf1=pf+1; ;inode++) {
        
        v = options[inode];
        
        // LOGIT_DECL(int level = ps->dim_of_value[inode]);

        // tries to locate the child with the value v
        branch.pf_child_content = ps->nfs + SCHEMA_IDX_CONTENT(ps,inode+1);
        pchild = pf->locate_child(pn, v, &shared, &branch, pf1);

        // not found
        if (pchild == NULL) {
#ifndef __NANO__
            // Is this a derived tree and a new node was created on base tree?
            if (pbase != NULL && pbase->op != OP_NONE) {

#ifdef __EXPERIMENTAL__
    GET_PNODE(pchild,pbase,ichild);
    //if (pchild != pchild2) fatal("Experimental 1");
#else    
                pchild = pbase->nodes[ichild]; // use the base tree node
#endif
                LOGIT_3("NOTFOUND[%02d]/SHR at %03d ~> %03d;",v,map_ptr(pn), map_ptr(pchild));

                // experimental:
                //deshare_map(pn,pn);
                #ifdef __STATS__
                stats.n_reinserts ++;
                #endif
                
                // create a shared child edge since v never happened as child of pn
                pf->add_child_edge(pn, pchild, SHARED, v);
                
                // completes ptree nodes with pbase nodes 
                copy_nodes(ptree->nodes, pbase->nodes, ps->n_values, ichild);
#ifdef __EXPERIMENTAL__
    SET_PNODE(ptree, ichild, pchild);
#else
                ptree->nodes[ichild] = pchild;
#endif
                ptree->op = pbase->op;
                break; // returns the level
            }
#endif

            // this is either a base tree or 
            // is a derived tree and a new node was not created on the base tree
            pchild = pf1->create_node(FALSE);
            pf->add_child_edge(pn, pchild, PROPER, v);
            
#ifndef __NANO__
            if (ptree->op == OP_NONE) {
                ptree->op = OP_NEW;
                LOGIT_3("NOTFOUND[%02d]/ADD! at %03d ~> %03d", v, map_ptr(pn), map_ptr(pchild));
            } else {
                LOGIT_3("NOTFOUND[%02d]/ADD  at %03d ~> %03d", v, map_ptr(pn), map_ptr(pchild));
            }
            
            if (ichild == ps->n_values) {
                LOGIT_2("NEW-TERMINAL %03d (child of %03d);",map_ptr(pchild), map_ptr(pn));
            }
#endif                
        // Does a shared child already exist?
        } else if (shared) {
#ifndef __NANO__           
            if (pbase == NULL) {
                // erro severo!
                // It should be impossible to occur
                // shared child inside the base tree
                fatal("Shared Child in base tree");                
            } 
            
            // if no new child was created on base tree then
            if (pbase->op == OP_NONE) {
                LOGIT_3("SHARED[%02d]/OP_NONE at %03d -> %03d;",v,map_ptr(pn),map_ptr(pchild));

               // complements ptree->NODES with nodes from pbase
#ifdef __EXPERIMENTAL__
// Do not set nodes!
    SET_PNODE(ptree, ichild, pchild);
#endif
                copy_nodes(ptree->nodes,pbase->nodes,ps->n_values,ichild);
                break;

            // Is the shared child the one changed on base tree?
            }

#ifdef __EXPERIMENTAL__
    GET_PNODE(pchild2,pbase,ichild);
    //if ( != pchild2) fatal("Experimental");
#else
#endif
            PNode pchild2 = pbase->nodes[ichild];
            if (pchild == pchild2) {

                LOGIT_3("SHARED[%02d]/LINK at %03d -> %03d;",v,map_ptr(pn),map_ptr(pchild));
                // complementa ptree->NODES com os dados em pbase
                // 2019-01-10: if (pbase && pbase->op != OP_NONE) {
                    ptree->op = pbase->op; // propagates change signal
#ifdef __EXPERIMENTAL__
// Do not set nodes!
//    SET_PNODE(ptree, ichild, pchild);
#else
                    copy_nodes(ptree->nodes,pbase->nodes,ps->n_values,ichild);
#endif
                    #ifdef __STATS__
                    stats.n_inedits ++;
                    #endif
                    
                // 2019-01-10: }
                break;
                
            // the shared child is not the one changed on base tree
            } else {
            
                // replaces the node pointed by the shared child edge  
                // by a new node that "inherits" its contents
 #endif                   // creates a new proper child 
                PNode old = pchild;
 #ifndef __NANO__
                LOGIT_DECL(int map_old = map_ptr(old));
                LOGIT_DECL(int count_old = HEADER_COUNT(old));
                pchild = NULL; //deshare_map(old, NULL);
                
                // Nao achou um reshare anterior?
                if (!pchild) {
 #endif                   // creates a new proper child 
                    pchild = pf->replace_child_on_branch(&branch, NULL, PROPER, pf1);
                    // deshare_mapdeshare_map(old, pchild); // register the new information

#ifndef  __NANO__
                    #ifdef __STATS__
                    stats.n_deshares ++;
                    #endif

                    LOGIT_6("SHARED[%02d]/PROPER at %03d from %03d (RC:%d)-> %03d (RC:%d); ",
                            v, map_ptr(pn), map_old, count_old, 
                            map_ptr(pchild),HEADER_COUNT(pchild));  
                } else {
                    // faz o child edge apontar para uma nova child
                    pchild = pf->replace_child_on_branch(&branch, pchild, SHARED, NULL);
                    LOGIT_6("SHARED[%02d]/RESHARE at %03d from %03d (RC:%d)-> %03d (RC:%d); ",
                            v, map_ptr(pn), map_old, count_old, 
                            map_ptr(pchild),HEADER_COUNT(pchild));  
                    // novo
                    if (ptree->op == OP_NONE) {
                        ptree->op = OP_DESHARE;
                        #ifdef __STATS__
                        stats.n_inedits --;
                        stats.n_deshares ++;
                        #endif
                    }
#ifdef __EXPERIMENTAL__
//    Do not copy pchild
//    SET_PNODE(ptree, ichild, pchild);
#else                    
                    copy_nodes(ptree->nodes,pbase->nodes,ps->n_values,ichild);
#endif
                    break;
                }

                if (ptree->op == OP_NONE) {
                    ptree->op = OP_DESHARE;
                    LOGIT_5("  [%02d] => REPLACE! at %03d from %03d -> %03d   !%03d;",
                            v, map_ptr(pn), map_old, map_ptr(pchild),
                            map_ptr(pbase->nodes[ichild]));   

                } else {
                    LOGIT_4("  [%02d] => REPLACE  at %03d from %03d -> %03d;",
                            v, map_ptr(pn), map_old, map_ptr(pchild));                    
                }

                // 2019-02-13: Se o DESHARE criou um node terminal, 
                // então TEM DE PARAR para não haver contagem dupla
                if (ichild == ps->n_values) {
                    already_increased = (pointers_count(increased, old)>0)? 1 : 0;
                    
                    #ifdef __LOGIT

                    int already_visited = (pointers_count(visited, old)>0)? 1 : 0;

                    if (already_increased && !already_visited) {
                        LOGIT("[inserted but not visited]");
                        fprintf(stderr,"[inserted but not visited]");
                    }
                    #endif
                    
                    LOGIT_2("  TERMINAL DESHARED. Already Inserted [%03d]?  %d!", 
                        map_old, already_increased);
                //   break;
                }                
            }
#else
                if (ichild == ps->n_values) {
                    already_increased = (pointers_count(increased, old)>0)? 1 : 0;
                    
                    #ifdef __LOGIT

                    int already_visited = (pointers_count(visited, old)>0)? 1 : 0;

                    if (already_increased && !already_visited) {
                        LOGIT("[inserted but not visited]");
                        fprintf(stderr,"[inserted but not visited]");
                    }
                    
                    LOGIT_2("  TERMINAL DESHARED. Already Inserted [%03d]?  %d!", 
                        map_old, already_increased);
                    #endif
                //   break;
                }                

#endif                
            
        // a proper child was found
        } else {
            // do nothing
            // just continue the descent path
            LOGIT_3("PROPER[%02d]/DOWN at %03d ~> %03d;",v,map_ptr(pn), map_ptr(pchild));
        }
       
        // save the child found
#ifdef __EXPERIMENTAL__
    SET_PNODE(ptree, ichild, pchild);
#else
        ptree->nodes[ichild] = pchild;
#endif
        ptree->nodes[ichild] = pchild;

        // BREAK if ichild is the terminal level!
        if (ichild == ps->n_values) {
            LOGIT("BOTTOM");
            bottom = 1; //
            break;
        }
        
        pn = pchild;
        pf   ++; 
        pf1  ++; 
        ichild ++;
    }
        
    // chegou até o final ?
//    if (bottom) {
    if (bottom && !already_increased) { // 2019-02-13
        // pf stills points to node before terminal
        // pchild tem a folha
        // pf1 tem as funcoes de manipulacao de folha
        
        //LOGIT_DECL(int d0);
        //LOGIT_DECL(ps->nfs.get_content_data(pchild,&d0));

        // adiciona o objeto ao node 
        #ifndef __STATS__
            terminal_insert(pchild, po);
        #else
            // printf("Increment!\n"); fflush(stdout);
            int n = terminal_insert(ps, pchild, po);
            if (!pbase) {
                if (n == 1) {
                    stats.n_primary_terminals ++;
                    stats.n_distinct_insertions ++;
                } else if (n == 2) {
                    stats.n_distinct_insertions --;
                }
            }
        #endif

        pointers_insert(increased, pchild); // 2019-02-13

        //LOGIT_DECL(int d);
        //LOGIT_DECL(ps->nfs.get_content_data(pchild,&d));
        
        //LOGIT_4("INS-ALPHA %03d => %d -> %d; RC=%d", map_ptr(pchild),d0, d,HEADER_COUNT(pchild));
    }

    LOGIT("GOING UPWARD;");
    
    // retorna pelo caminho percorrido ajustando os "content"
    for (; inode >= iroot; inode--) {
    
        
#ifdef __EXPERIMENTAL__
    GET_PNODE(pn,ptree,inode);
    //if (pn!= pchild2) fatal("Experimental 2");
#else    
    pn = ptree->nodes[inode];
#endif
        // prepare content stuff
        int shared_content;
        PNode pcontent = pf->get_content_info(pn, &shared_content);
        
        // computes the index of content
        int icontent = ps->top_of_dim[ps->dim_of_value[inode] + 1];
        PNodeFunctions pfc = ps->nfs + icontent;

        int must_update_content = 0;
        
        // single child ?
        if (pf->has_single_child(pn)) {
            LOGIT_1("- FOUND SINGLE CHILD of %03d;",map_ptr(pn));
            PNode old = pcontent;
            
            // the child is the content?
            if (inode + 1 == icontent) {
                pcontent = pchild;
            } else {
                // set this node content as the content of its unique child
                pcontent = pf1->get_content_info(pchild,NULL);
            }

            // only set content as shared if it is really necessary
            // avoiding artifitial increment of usage counter
            if ( (old != pcontent) || (shared_content != SHARED) ) {
                pf->set_content_info(pn, pcontent, FALSE, SHARED, FALSE);
                LOGIT_3("  SET SHARED CONTENT of %03d -> %03d; RC=%d",
                    map_ptr(pn), map_ptr(pcontent), HEADER_COUNT(pcontent));
            }

        // this should never happen
        } else if (!pcontent) {
            fatal("INSERT: No content found going upward!!");
            
        // This node has more than one child and 
        // has it a shared content? Derive it!
        } else if (shared_content) {

            int processed = pointers_count(visited, pcontent);

            // tries to insert "content" in nodeset
            // success means that node wasn't in nodeset
            if (processed) {

                LOGIT_3("- CANCELLING NEW-BETA for %03d: %03d already processed.: %d",
                        map_ptr(pn), map_ptr(pcontent), ps->dim_of_value[icontent]);
                
            } else {
                
                // 
                LOGIT_DECL(PNode old = pcontent);
                LOGIT_DECL(int map_old = map_ptr(old));
                LOGIT_DECL(int counter_old = HEADER_COUNT(old));
                
                // shallow copy shared content to a new content
                pcontent = pfc->shallow_copy(pcontent, TRUE, pf1);
                
                // set the new content as the content of pn            
                pf->set_content_info(pn, pcontent, TRUE, PROPER, FALSE);    

                LOGIT_5("- CREATE NEW-BETA for %03d: %03d from %03d level: %d. RC_OLD = %d",
                        map_ptr(pn), map_ptr(pcontent),map_old, 
                        ps->dim_of_value[icontent],counter_old);
                
                                
                must_update_content = !processed;
            }
            
        // proper content always must be updated
        } else {
           must_update_content = 1;
           LOGIT_2("- FOUND PROPER BETA for %03d Content(BETA): %03d", map_ptr(pn), map_ptr(pcontent));        
        }

        if (must_update_content) {
            if (icontent == ps->n_values) {
                terminal_insert(ps, pcontent,po);
           
                //LOGIT_DECL(int d); 
                //LOGIT_DECL(ps->tfs.get_content_data(pcontent,&d));
                //LOGIT_2("  INSERT INTO BETA %03d => %d;",map_ptr(pcontent), d);
            } else {            
                insert_into_tree(ps, pcontent, icontent, options, ptree+1, ptree,po, 
                visited, increased);
            }
            
            pointers_insert(visited, pcontent);
        }
        pchild = pn;
        pf1 = pf;
        pf --;
    }
    LOGIT_1("--- LEAVING %03d",map_ptr(proot));

    return 0;
}

//***************************************************************************
//
//    TINYCUBES_INSERT
//
//***************************************************************************

#ifdef __STATS__
#include <sys/resource.h>
#endif


/**
 * 
 * 
 * 
 */
void tinycubes_insert(PSchema ps, PNode proot, PAddress address, PObject po) {
    PTree ptree = (PTree) schema_get_tree(ps,1);

    PPointers visited = pointers_create(256);
    PPointers increased = pointers_create(256);

    // deshare_map (NULL,NULL);
    #ifdef __STATS__
        stats.n_insertions++;
    #endif

    insert_into_tree(ps,proot,0,address,ptree,NULL,po, visited, increased);

    #ifdef __STATS__
        struct rusage r_usage;
        getrusage(RUSAGE_SELF,&r_usage);
        
        if (r_usage.ru_maxrss > stats.max_memory_used_kb) {
            stats.max_memory_used_kb = r_usage.ru_maxrss;
        } 
        stats.acc_memory_used_kb += r_usage.ru_maxrss;
        stats.count_memory_used ++;
    #endif 

    pointers_destroy (visited);
    pointers_destroy (increased);
}
