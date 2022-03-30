
#include <stdlib.h>
#include <memory.h>

#include "pool.h"

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
PFreePool pool_create(int item_sz, int clear) {
    PFreePool pfp = calloc(1, sizeof(FreePool));
    pfp->item_sz = item_sz;
    pfp->clear = clear;
    pfp->clock = clock();
    return pfp;
}



//-----------------------
//
//
//
//
void pool_reset(PFreePool pfp) {
    PFreeItem pfi;
    while ((pfi = pfp->first)) {
        pfp->first = pfi->next;
        free(pfi);
    }
}

//-----------------------
//
//
//
//
void pool_destroy(PFreePool pfp) {
    pool_reset(pfp);
    free(pfp);
}


//-----------------------
//
//
//
//
void *pool_alloc_item(PFreePool pfp) {
    if (!pfp->first) {
        if (pfp->clear) { return calloc(1,pfp->item_sz); }
        return malloc(pfp->item_sz);
    } 

    PFreeItem psecond = pfp->first->next;
    void *result = pfp->first;
    pfp->first = psecond;
    
    if (pfp->clear) {
        memset(result, 0, pfp->item_sz);
    }

    pfp->n_items --;
    return result;
}

//-----------------------
//
//
//
//
void pool_free_percentage(PFreePool pfp, int percent) {

    if (percent<=0) percent = 50;

    int n = (pfp->n_items * percent) / 100; 
    if (n <= 30) return;

    PFreeItem pfi;
    while ((pfi = pfp->first) && (n)) {
        pfp->first = pfi->next;
        free(pfi);
        n--;
    }
}

//-----------------------
//
//
//
//
void pool_free_item(PFreePool pfp, void *item) {

    PFreeItem pfi = (PFreeItem) item; 
    pfi->next = pfp->first;
    pfp->first = pfi;
    pfp->n_items ++;
#if 1
    clock_t now = clock();
    long ms = ((double) now - pfp->clock) / CLOCKS_PER_SEC * 1000; 
    pfp->clock = now;
    if (ms > 100) {
        pool_free_percentage(pfp, 0);
    }
#endif    
}
