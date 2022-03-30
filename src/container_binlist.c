#include <stdlib.h>
#include "metadata.h"
#include "register.h"
#include "query.h"

#define __USE_POOL__NO

__int64_t content_total_alocado = 0;

//=============================================================================================
//
//    
//
//=============================================================================================

#define POOL_STEP 4096

typedef struct s_free_item {
    void *next;
} FreeNode, *PFreeItem;

typedef struct s_block {
    struct s_block *next;
    int   offset_next_free;
    int   free_tail;
} Block, *PBlock;

typedef struct s_pool {
    PFreeItem *freelist;

    int item_size;
    int nodes_per_block;

    int n_blocks;
    PBlock blocks;
} Pool, *PPool;

// Inicializa o Pool
static Pool pool =  { NULL, 0, 0, 0, NULL};

//-----------------------
//
//
//
//
void release_pool(void) {
    PBlock p, q;
    for (p = pool.blocks; p; p = q) {
        q = p->next;
        free(p);
    }
    memset(&pool, 0, sizeof(Pool));
}

//-----------------------
//
//
//
//
PBlock alloc_block(PPool pp) {
    int sz = sizeof(Block) + pp->item_size * pp->nodes_per_block;
    PBlock pb = malloc(sz);
    
    content_total_alocado += sz;

    if (!pb) fatal("BinList: allocation");

    pb->free_tail = pp->nodes_per_block;
    pb->offset_next_free = sizeof(Block);
    //printf("Size: %d, Free: %d Offset: %d\n",sz,pb->free_tail, pb->offset_next_free); fflush(stdout);
    return pb;
}

//-----------------------
//
//
//
//
void *pool_alloc(int item_size) {
    PPool pp = &pool;
    void *res;

    if (!pp->item_size) {
        pp->item_size = item_size;
        pp->nodes_per_block = POOL_STEP;
        pp->blocks = alloc_block(pp);

    } else if (pp->item_size != item_size) {
        fatal("pool item size");
    }

    PBlock pb = NULL;

    // aloca da lista livre primeiro
    res = pp->freelist;
    if (res) {
        pp->freelist = ((PFreeItem) res)->next;

    // nao tem espaco no final 
    } else {
        if (!pp->blocks->free_tail) {
            // aloca um novo bloco 
            pb = alloc_block(pp); 
            pb->next = pp->blocks;
            pp->blocks = pb;
            //printf("New ");
        } else {
            pb  = pp->blocks;
            //printf("Current ");

        }
        res = ((char *) pb) + pb->offset_next_free;
        pb->offset_next_free += pp->item_size;
        pb->free_tail --;
        //printf("Item size: %d, Free: %d Offset: %d\n",pp->item_size,pb->free_tail, pb->offset_next_free); fflush(stdout);
    }

    // limpa
    memset(res,0,pp->item_size);
    return res;
}


//=============================================================================================
//
//    
//
//=============================================================================================

typedef struct s_bnode *PBinNode;
#define SHORT_BIN_NO

#ifdef SHORT_BIN
typedef unsigned short bin_t;
#else
typedef unsigned int bin_t;
#endif

#define __INSERT_ONLY__

typedef struct __attribute__((__packed__))  s_bnode {
    PBinNode  next_bin;
    bin_t id_bin;
#ifndef __INSERT_ONLY__
    int count;
#endif    
    Byte data [];
} BinNode;

typedef struct __attribute__((__packed__)) {
    PBinNode first; 
    PBinNode last;
    struct {
        __int64_t count:40;
        unsigned int bin_size:13;
        unsigned int node_size:10;
        unsigned int shallow:1;
    } data;
#ifdef __HIST__    
    int n_nodes;
#endif    
} BinList, *PBinList;

typedef struct {
    unsigned int list_bin_size0;
} BParams, *PBinListParams;

#ifdef __HIST__    
int total_n_nodes = 0;
int max_blist_n_nodes = 0;
#endif

#ifdef __HIST__
int hist[25001];
#endif

static Byte shallow_data[1024];

//-----------------------
//
//
//
//
static void *binlist_insert(PRecord record, void *data, PContentData pcd) {
    PBinList pbl = (PBinList) data;
    PBinListParams pblp = (PBinListParams) pcd->params;
    pbl->data.count ++;
    
    if (pbl->data.bin_size != pblp->list_bin_size0) 
        fatal("BIN_SIZE: [%p %d] != [%p %d]", pbl, pbl->data.bin_size, pblp, pblp->list_bin_size0);

#if 0    
    if (!max_blist_n_nodes) {
        memset(hist,0,sizeof(int) * 25000);
    }
#endif

    if(pbl->data.shallow) return &shallow_data;

    __int64_t value = get_record_value(record, pcd,0);
    value /= pblp->list_bin_size0;
#ifdef SHORT_BIN
    if (value > __INT16_MAX__) fatal("BINLINST: overflow\n");
#endif
    bin_t id_bin = (bin_t) value;

    PBinNode p = pbl->last;
    PBinNode ant;
    int tail = 1;

    //printf("pbl = %p  l = %p\n", pbl, p);
    if (!p) {
        pbl->data.node_size = sizeof(BinNode) + pcd->total_data_size;

    } else {

        //printf("BINLIST: found id_bin %d %d  Count: %d\n", p->id_bin, id_bin, p->count);
        //printf("Epoch %d  Bin = %d Last = %p\n", value, id_bin, p );
equal_bin:
        if (id_bin == p->id_bin) {
#ifndef __INSERT_ONLY__
            p->count ++;
#endif
            return p->data;
        }

        if (id_bin < p->id_bin) {
            tail = 0;
            for (p = pbl->first, ant = NULL; p; ant=p, p = p->next_bin) {
                if (id_bin < p->id_bin) break;
                if (id_bin == p->id_bin) goto equal_bin;
            }
            // printf("BINLIST: id_bin %d  %d  %d\n", id_bin, ant ? ant->id_bin:-1, p ? p->id_bin:-1);

        }
    }
    // printf("BINLIST: new id_bin %d  %d  %p\n", id_bin, value, pbl);

    // allocates space to store subchannel data

#ifdef __USE_POOL__    
    p = (PBinNode) pool_alloc(pbl->data.node_size);        
#else
    p = (PBinNode) calloc(1, pbl->data.node_size);        
#endif

    p->id_bin = id_bin;

#ifndef __INSERT_ONLY__
    p->count = 1;
#endif
 
    if (tail) {
        // encadeia no final da lista
        if (!pbl->last) { 
            pbl->first = p; 
        } else { 
            pbl->last->next_bin = p; 
        } 
        pbl->last = p;

    } else if (ant) {
        p->next_bin = ant->next_bin;
        ant->next_bin = p;

    } else {
        p->next_bin = pbl->first;
        pbl->first = p;
    }
#ifdef __HIST__        
    pbl->n_nodes ++;
    total_n_nodes ++;
#endif

#ifdef __HIST__
    hist[pbl->n_nodes] ++;
    if (pbl->n_nodes > max_blist_n_nodes) max_blist_n_nodes = pbl->n_nodes;
#endif

    return p->data;
}


//-----------------------
//
//
//
//
static void *binlist_remove(PRecord record, void *data, PContentData pcd) {
#ifdef __INSERT_ONLY__
    return NULL;
#else
    PBinList pbl = (PBinList) data;
    PBinListParams pblp = (PBinListParams) pcd->params;
    pbl->count --;
    
    if(pbl->data.shallow) return &shallow_data;

    __int64_t value = get_record_value(record, pcd,0);
    value /= pblp->list_bin_size;
    bin_t id_bin = (bin_t) value;

    PBinNode prev = NULL;
    PBinNode p = pbl->first;
    for ( ; p; prev = p, p = p->next) {
        if (p->id_bin == id_bin) {
            p->count --;   // pode ter zerado
            if (p->count) return p->data;
            if (prev) { 
                prev->next = p->next; 
            } else {
                pbl->first = p->next;
            }
            free(p);
#ifdef __HIST__
            pbl->n_nodes --;
#endif
            return NULL;
        } else if (p->id_bin > id_bin) {
            return NULL;
        }
    }
    return NULL;
#endif
}


//----------------------
//
//
//
//
static void _shallow_copy(PByte dst, PByte src, int n_bytes) {    
    PBinList s = (PBinList) src;
    PBinList d = (PBinList) dst;
    PBinNode p, q;

    //d->data.shallow = 1;
    //return;

    memset(d,0,sizeof(BinList));
    d->data.bin_size = s->data.bin_size;
    d->data.node_size = s->data.node_size;

    // printf(" SHALLOW %d ",d->data.bin_size); fflush(stdout);
    
    for (p = s->first; p; p = p->next_bin) {
#ifdef __USE_POOL__
        q = pool_alloc(d->data.node_size);
#else
        q = calloc(1, d->data.node_size);
#endif
        
        // deveria fazer um shallow copy recursivo
        memcpy(q->data,p->data,s->data.node_size - sizeof(BinNode));
        
#ifndef __INSERT_ONLY__
        q->count = p->count;
#endif
        q->id_bin = p->id_bin;

        if (!d->last) { 
            d->first = q; 
        } else { 
            d->last->next_bin = q; 
        }
        d->last = q;

#ifndef __INSERT_ONLY__
        d->count += p->count;
#endif
#ifdef __HIST__
        d->n_nodes ++;
        total_n_nodes ++;
#endif
    }

#ifdef __HIST__
    for (int i = 1; i<=s->n_nodes; i++) {
        [i] += s->n_nodes;
    }
#endif
}

//----------------------
//
//
//
//
static void _prepare(int op, void *content,  PContentData pcd) {
    PBinList pbl = (PBinList) content;
    PBinListParams pblp = (PBinListParams) pcd->params;

    if (op == TERMINAL_OP_INIT) {
        pblp->list_bin_size0 = pcd->bin_size;
        pbl->data.bin_size = pcd->bin_size;
        // printf(" [PREPARE %p %d] ", pbl, pbl->data.bin_size); fflush(stdout);
    }
}

//----------------------
//
//
//
//
static void _parse_param(int id_param, char *s, PByte params) {
    // printf("BINLIST ParseParam:\n");
    PBinListParams pblp = (PBinListParams) params;
    if (id_param == 1) {
        fatal("Binlist has no parameters");
        pblp->list_bin_size0 = atoi(s);
        // printf("BIN LIST: BinSize %d\n", pblp->list_bin_size); fflush(stdout);
        if (!pblp->list_bin_size0) fatal("BINLIST: BinSize Error. ");
    } else {
        fatal("BINLIST: Extra parameter 1 %d\n",id_param);        
    }
}

/*-----------------
*
*
*
*/
static int binlist_find_bounds(PByte pdata, int *bounds) {
    PBinList pbl = (PBinList) pdata;

    PBinNode p = pbl->first;
    if (!p) return 0; 

    bounds[0] = p->id_bin  * pbl->data.bin_size;

    while (p->next_bin) p = p->next_bin;
    bounds[1] = p->id_bin * pbl->data.bin_size;

    return 1;
}


//----------------------
//
//
//
//
static void register_content_binlist(PClass pclass) {

    static ContentFunctions cfs;
    cfs.parse_param = _parse_param;
    cfs.prepare = _prepare;
    cfs.shallow_copy = _shallow_copy;
    cfs.insert = &binlist_insert;
    cfs.remove = &binlist_remove;
    cfs.find_bounds = &binlist_find_bounds;
    // cfs.remove

    static ContentInfo ci;
    memset(&ci, 0, sizeof(ContentInfo));

    ci.n_param_fields = 1;
    ci.min_params = 0;
    ci.max_params = 0;
    ci.params_size = sizeof(BParams);
    ci.private_size = sizeof(BinList);
    ci.pclass = pclass;

    ci.pcfs = &cfs;

    register_content("binlist", &ci);
}

//=============================================================================================
//
//    
//
//=============================================================================================

static int end_of_data;
#define END_OF_DATA (&end_of_data)

/*-----------------
*
*
*
*/
static void *cop_between(PQArgs pa, PQOpData pdata, void **ppstate) {
    //printf("COP BETWEEN: First %p\n",pdata);
     if (*ppstate == END_OF_DATA) return NULL;

    PBinList pbl = (PBinList) pdata;
    PBinNode p = *ppstate;
    if (!p) {
        // printf("BETWEEN: First\n");
        p =  pbl->first;
    } else {
        p = p->next_bin;
        if (!p) { *ppstate = END_OF_DATA; return NULL; }
    }

    //printf("1"); fflush(stdout);
    bin_t t0 = pa->args[0].i/pbl->data.bin_size;
	
    bin_t t1 = pa->args[1].i/pbl->data.bin_size;
    if (t0 == t1) t1 ++;
//    printf("t0 = %ld  t1=%ld\n",t0);

    //printf("X"); fflush(stdout);
    // printf("BETWEEN: %d %d %p BIN: %d COunt: %d PBL Count %d  %d\n", t0, t1, p, p->id_bin, p->count, pbl->count, pbl->n_nodes);

    // localiza o inicio do between
    if (p->id_bin < t0) {
        do {
            // printf("P.BIN %d\n",p->id_bin);
            p = p->next_bin;
        } while (p && (p->id_bin < t0));
        if (!p) { *ppstate = END_OF_DATA; return NULL; }
    }

    // enquanto estiver 
    if ( (p->id_bin >= t0) && (p->id_bin <= t1)) {
        // printf("P.BIN FOUND %d\n",p->id_bin);
        *ppstate = p;
        return p->data;
    }

    // printf("END OF DATA\n");
    *ppstate = END_OF_DATA;
    return NULL;
}

#define mprintf(...) fprintf (stderr, __VA_ARGS__)

/*-----------------
*
*
*
*/
static void *cog_between(PQArgs pa, PQOpData pdata, int *indexes, void **ppstate) {
    if (*ppstate == END_OF_DATA) return NULL;

    PBinList pbl = (PBinList) pdata;
#ifdef __HIST__        
    //printf("BETWEEN 0: %d %d %p %d\n", pa->args[0].i, pa->args[1].i,pdata, pbl->n_nodes);
#else
    //printf("BETWEEN 0: %d %d %p\n", pa->args[0].i, pa->args[1].i,pdata);
#endif
    PBinNode p = *ppstate;
    if (!p) {
        //printf("BETWEEN: First\n");
        p =  pbl->first;
#ifdef __HIST__        
    //printf("Count: %d Size: %d\n", pbl->count, pbl->data.bin_size);
#else
    //printf("Size: %d\n", pbl->data.bin_size);
#endif
    } else {
        p = p->next_bin;
    }
    
    if (!p) { 
        // printf("X\n");
        *ppstate = END_OF_DATA; 
        return NULL; 
    }
  
    //printf(" G %d ",pbl->data.bin_size);fflush(stdout);
    bin_t t0 = pa->args[0].i / pbl->data.bin_size;
    bin_t t1 = pa->args[1].i / pbl->data.bin_size;
    if (t0 == t1) t1 ++;

    //printf("COG t0 = %d\n",t0);
    //printf("X");fflush(stdout);
    
    //printf("BETWEEN BEGIN: %d %d\n", t0, t1);

    // localiza o inicio do between
    while (p) {
        //printf("%d\n",p->id_bin);
        if ((p->id_bin >= t0)) break;
        p = p->next_bin;
    }
    if (!p) { 
		//printf("  BETWEEN STEP: %d %d\n", t0, t1);
		*ppstate = END_OF_DATA; 
		return NULL; 
	}

    //printf("  BETWEEN FOUND INITIAL: %d %d %d\n", pa->args[0].i, pa->args[1].i, p->id_bin);
	//printf("     A"); fflush(stdout);

    // enquanto estiver 
    if ( p && (p->id_bin >= t0) && (p->id_bin <= t1)) {
		//printf("B"); fflush(stdout);
		*ppstate = p;
		if (indexes) { 
			//printf("C"); fflush(stdout);
			//indexes[0] = (p->id_bin   - t0) * pbl->data.bin_size; 
#if 0 
				indexes[0] = (p->id_bin   - t0);  // Posicao relativa aos bins
#else            
			//printf("D"); fflush(stdout);
			if (!pa->index_n_points || pa->index_n_points >= pa->index_count) {
			//printf("E"); fflush(stdout);
//				printf("d\n",p->id_bin - t0); fflush(stdout);
				indexes[0] = (p->id_bin   - t0);  // Posicao relativa aos bins
			} else {
			//printf("F"); fflush(stdout);
				int idx0;
				idx0 = (p->id_bin   - t0);
				indexes[0] = ( idx0 * pa->index_n_points ) / pa->index_count;  // Posicao relativa aos bins
			//printf("G"); fflush(stdout);
				// printf("N, Countet: %d, %d, %d -> %d %d\n", pa->index_n_points, (t1 - t0), idx0, indexes[0], pa->index_count);
			}
#endif            
		} // indexes

		//printf("H"); fflush(stdout);
		//printf("OFFS: %d \n", p->id_bin);
		//printf("BETWEEN DONE: %d %d\n", t0, t1);
		return p->data;
    }
	//printf("I"); fflush(stdout);

    *ppstate = END_OF_DATA;
    return NULL;
}

#define REGISTER_BINLIST_OP(X,S,Y)  \
    static QOpInfo qoi_##X;\
    qoi_##X.op = cop_##X;\
    qoi_##X.op_group_by = cog_##X;\
    qoi_##X.min_args = Y;\
    qoi_##X.max_args = MAX_ARGS;\
    qoi_##X.fmt = "II"   ;\
    register_op("binlist", S, &qoi_##X)

/*-----------------
*
*
*
*/
static void register_ops(void) {
    REGISTER_BINLIST_OP(between,"between",2);
    qoi_between.max_args = 2;
}


//=============================================================================================
//
//    CLASS 
//
//=============================================================================================

/*--------------------------
*
*
*
*/
static int get_distinct_info(void * cop, PQArgs pa, PByte params, int bin_size, int *distinct_count, int *distinct_base) {
    //printf("Z1\n"); fflush(stdout);
    PBinListParams pblp = (PBinListParams) params;

    if (cop == cop_between) {
        int begin = pa->args[0].i;
        int end   = pa->args[1].i;
		
		begin = begin - (begin % bin_size);
		int extra = end % bin_size;
		if (extra > 0) end += bin_size - extra;
		
        //printf("Z1 begin %d end %d\n", begin, end); fflush(stdout);
        
        //printf("BINLIST: %d\n", pblp->list_bin_size); fflush(stdout);

        if (!bin_size) fatal("container_binlist: zero bin_size");
//        *distinct_count = ((end - begin) / pblp->list_bin_size); 
        *distinct_count = ((end - begin) / bin_size) + 10; 
        if (!*distinct_count) *distinct_count = 1;
        *distinct_base = begin;
        // printf("BEGIN Z1 %d  COUNT %d\n",begin,*distinct_count); fflush(stdout);

        return 1;
    }
    return 0;
}


/*--------------------------
*
*
*
*/
static void *create_op_data(void * cop, PQArgs pa) {

    if (cop == cop_between) {
        return calloc(1,sizeof(BinList));
    } 
    return NULL;
}

/*--------------------------
*
*
*
*/
static void destroy_op_data(void *cop, void *op_data) {
    // assume que o conteudo foi liberado
    if (op_data) free(op_data);
}

/*--------------------------
*
*
*
*/
void register_container_binlist(void) {

    // prevention against reinitialization during execution
    static int done = 0; 
    if (!done) done = 1; else return;
    
    static Class c;
    memset(&c, 0, sizeof(c));

    c.name = "binlist";
    c.class_type = CONTAINER_CLASS;

    c.n_group_by_indexes = 1;
    c.group_by_requires_rule = 1;  // was 0

    c.get_distinct_info = &get_distinct_info;

    c.create_op_data = &create_op_data;
    c.destroy_op_data = &destroy_op_data;

#if CLASS_PARSE
    c.n_fields = 1;
    c.n_params = 1;
    c.parse_param = _parse_param;
#endif

    register_class(&c);
    register_content_binlist(&c);
    register_ops();
}


