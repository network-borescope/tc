/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dump1.c
 * Author: nilson
 * 
 * Created on September 19, 2018, 3:07 PM
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "schema.h"
#include "dump1.h"
#include "nodeset.h"
#include "terminal.h"

//---------------------------------------------------------------


//---------------------------------------------------------------

typedef struct s_data {
    PNode node;
    PNode parent;
    int idx;
    Value v;
    int shared;
    int content;
    struct s_data *next;
} *PData, Data;

PData first = NULL;
PData last = NULL;

void queue_append(PNode node, int idx, PNode parent, Value v, int shared,int content) {
    PData pd = calloc(sizeof(Data),1);
    pd->v = v;
    pd->parent = parent;
    pd->node = node;
    pd->idx = idx;
    pd->shared = shared;
    pd->content = content;
    if (!last) {
        first = pd;
    } else {
       last->next = pd; 
    }
    last = pd;
    
}

PNode queue_dequeue(PNode *parent, int *idx, Value *v, int *shared,int *content) {
    if (!first) return NULL;

    PData pd = first;
    first = pd->next;
    
    if (!first) last = NULL;
    
    *v = pd->v;
    *parent = pd->parent;
    *idx = pd->idx;
    *shared = pd->shared;
    *content = pd->content;
    
    return pd->node;
}

int queue_empty(void) {
    return !first; 
}


char buf[50];

char *id_to_str(int idx, int id1, int id2) {
    sprintf(buf,"%x_%x_%x",idx, id1, id2);
    return buf;
#if 0
    buf[0] = (id1 / 26) + 97;
    buf[1] = (id1 % 26) + 96;

    buf[2] = 0;
    if (!id2) return buf; 

    buf[2] = '_';
    buf[3] = id2 / 26 + 97;
    buf[4] = id2 % 26 + 96;
    buf[5] = 0;
#endif
}

static char ranks[100][4000];
static char *dim_fill[] = { "#d0d0ff", "#d0ffd0", "#ffd0d0", "#d0d0d0", "#ffd0ff", "#d0ffff"};
static char *dim_pen[] = { "#4040ff", "#40ff40", "#ff4040", "#404040", "#ff40ff", "#40ffff"};
// static char *idx_color[100]; // cores dos niveis


void *make_ptr(int id1, int id2, int content) {
    return (void *) (long int) (content * 1000000 + id1 * 1000 + id2);
}


#ifndef __DUMP
void dump(PSchema ps, PNode pn, char *fname,char *title) {
    ranks[0][0] = 0;
}
#else
void dump(PSchema ps, PNode pn, char *fname,char *title, int level,int follow) {
    PNodeFunctions pf;
    Value v;
    PNode pchild;
    int shared, content;
    FILE *fout;
	
    int idx;
    PNode parent;
    int id, id2;
    void *ptr;
    char *s, *shape, *c;

    puts(fname);
    fout = fopen(fname,"wt");
    fprintf(fout, "digraph { \n");
    fprintf(fout, "   rankdir=TB;\n");
    fprintf(fout, "   fontsize=\"9\";\n");
//    fprintf(fout, "   size=\"16,4\";\n");

         fprintf(fout, "   labelloc=\"tl\";\n");
         fprintf(fout, "   label=\"%s\";\n",title);
    
//    fprintf(fout, "   ratio=1.0;\n");

	PPointers nodes = pointers_create(1000);

	for (idx = 0;idx< 100; idx++ ) {
        ranks[idx][0] = 0;
    }
    
    // coloca a raiz na fila
    if (pn) queue_append(pn,level,NULL,0,0,0);

    //
    while (!queue_empty()) {
        pn = queue_dequeue(&parent,&idx,&v,&shared,&content);
        id = map_ptr(pn);

        char *black = "\"#303030\"";

        int chain_root = (ps->top_of_dim[ps->dim_of_value[idx]] == idx);
        char *bg_color =  shared?  "white" : dim_fill[ps->dim_of_value[idx]];
        char *pen_color = shared?  "white" : dim_pen[ps->dim_of_value[idx]];
        int terminal = idx == ps->n_values;
        shape = shared?  "plaintext" : chain_root ? "ellipse" /* "hexagon" */ : "ellipse";
        c = shared? "" : ""; 
        // char *style = "solid";
        double pen = terminal ? 1.0 : 1.0;
		char lb[100];
        int refc = HEADER_COUNT(pn);
        if (refc < 2) {
//            *lb = 0;
		    sprintf(lb,"%02d",id);
        } else if (shared) {
		    sprintf(lb,"%02d",id);
        } else {
		    sprintf(lb,"%02d*%d",id,refc);
        }
        char *fcolor = black;

        if (content) {
            id2 = shared? map_ptr(parent): 0;
		    s = id_to_str(idx,id,id2);
			if (idx == ps->n_values && !shared) {
				int d;
				d = terminal_get_count(ps, pn);
                if (refc > 1) {
				    sprintf(lb,"%02d*%d:%d",id,refc,d);
                } else {
				    sprintf(lb,"%02d:%d",id,d);
                }
			}
            fcolor = (((PHeader)pn)->hdr.beta) && (idx != ps->n_values)? "red" : black;
			
            // style = shared ? "solid":"solid";

        // cria o no com o id_real
        } else if (!shared || follow) {
            // cria o no com o id_real
            s = id_to_str(idx,id,0);
			if (idx == ps->n_values) {
				int d = terminal_get_count(ps, pn);
                if (refc > 1) {
				    sprintf(lb,"%02d*%d:%d",id,refc,d);
                } else {
				    sprintf(lb,"%02d:%d",id,d);
                }
			}
            fcolor = black;
        } else {
            id2 = map_ptr(parent);
            s = id_to_str(idx,id,id2);
            fcolor = (((PHeader)pn)->hdr.proper_child)? "blue" : "cyan";
        }
        fprintf(fout,"a%s [label=\"%s%s\" shape=\"%s\" penwidth=%f style=\"%s\" fontsize=8 fontcolor=%s margin=0 width=0.25 height=0.25, color=\"%s\", fillcolor=\"%s\" ]; // idx=%d value=%d shared=%d content=%d\n",
                    s,lb,c,shape,pen,"filled",fcolor,pen_color,bg_color,idx,v,shared,content);                        
        if (ranks[idx][0]) strcat(ranks[idx],", ");
        strcat(ranks[idx],"a");
        strcat(ranks[idx],s);
        
        if (idx < ps->n_values) {
        
            // acha todos os valores de pn
            pf = ps->nfs;
            Branch branch;
            for (pchild = pf->get_child(pn,&v,&shared,&branch, FALSE, pf+1);
                pchild ;
                pchild = pf->get_child(NULL,&v,&shared,&branch, FALSE, pf+1)) {
				
                id = map_ptr(pchild);
                id2 = !shared ? 0 : map_ptr(pn);
                ptr = make_ptr(id, id2, 0);
              
                char *atype = shared? "normal": "none"; // "empty": "normal";
                
                if (!pointers_count(nodes, ptr)) {
                    pointers_insert(nodes, ptr);
                    queue_append(pchild, idx+1, pn, v, shared,0);
                    fprintf(fout,"a%s -> ",id_to_str(idx,map_ptr(pn),0));
                    fprintf(fout,"a%s [label=\" %d\" fontsize=8 arrowhead=\"%s\"];\n",id_to_str(idx+1,id,id2),v,atype);
                }
            }

            // coloca o content na fila
            pchild = pf->get_content_info(pn, &shared);
            if (!pchild) continue;
            
            id = map_ptr(pchild);
            id2 = shared? map_ptr(pn): 0;
            ptr = make_ptr(id, id2, 1);

            // exibe arestas content
            if (!pointers_count(nodes, ptr) ){ // && !pf->has_single_child(pn)) {
                pointers_insert(nodes, ptr);
                int idx2 = ps->top_of_dim[ps->dim_of_value[idx]+1];
                queue_append(pchild, idx2, pn, 0, shared,1);

				char *atype = shared? "normal": "none"; // "empty": "normal";
                fprintf(fout,"a%s -> ",id_to_str(idx,map_ptr(pn),0));
                fprintf(fout,"a%s [style=\"dashed\" arrowhead=\"%s\"]; \n",id_to_str(idx2,id,id2),atype);
            }
        }
        
    }
    
    for (idx=0;idx<100;idx++) {
        if (ranks[idx][0]) {
            fprintf(fout,"{ rank=same;  %s }\n",ranks[idx]);
        }
    }
    
    fprintf(fout, "}\n");
    fclose(fout);
}
#endif