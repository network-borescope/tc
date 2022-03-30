#include <stdlib.h>
#include <math.h>

#include "metadata.h"
#include "register.h"
#include "query_processor.h"

//=============================================================================================
//
//    Bitmap
//
//=============================================================================================

typedef struct {
    int  x0, y0;
    int  w, h;
    PByte bitmap;
} Bitmap, *PBitmap;

/*-----------------
*
*
*
*/
static PBitmap create_bitmap(int left, int top, int right, int bottom) {
    int w = right - left + 1 + 2;
    int h = bottom - top + 1 + 2;
    //printf("GEO: bitmap w=%d h=%d sz=%d\n", w, h,w*h); fflush(stdout);

    PBitmap pb  = calloc(1, sizeof(Bitmap));
    pb->bitmap = calloc(w * h, sizeof(Byte));
    pb->x0 = left;
    pb->y0 = top;
    pb->w  = w;
    pb->h  = h;

    return pb;
}

/*-----------------
*
*
*
*/
static void set_pixel(PBitmap pb, int x, int y) {
    //printf("GEO: set_pixel %d %d\n",x, y); fflush(stdout);
    int xb = x - pb->x0 + 1;
    int yb = y - pb->y0 + 1;
    int offset = (yb * pb->w) + xb;
    //printf("GEO: offset %d xb=%d yb=%d\n", offset, xb, yb); fflush(stdout);
    pb->bitmap[offset] = 1;
}

/*-----------------
*
*
*
*/
static int get_pixel(PBitmap pb, int x, int y) {
    //printf("GEO: get_pixel %d %d\n",x, y); fflush(stdout);
    int xb = x - pb->x0 + 1;
    int yb = y - pb->y0 + 1;
    if (xb < 0 || xb > pb->w) return 0;
    if (yb < 0 || yb > pb->h) return 0;
    int offset = (yb * pb->w) + xb;
    //printf("GEO: offset %d xb=%d yb=%d\n", offset, xb, yb); fflush(stdout);
    return pb->bitmap[offset];
}


/*-----------------
*
*
*
*/
static void line_dda(PBitmap pb, int x0, int y0, int x1, int y1) {
    int x, y, dx, dy, increment;

    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

    dx = abs(x1 - x0);
    dy = abs(y1 - y0);

    increment = (dx >= dy) ? dx: dy;
    if (!increment) return;

    dx = dx / increment;
    dy = dy / increment;

    x = x0; y = y0;
    for(int count = 0; count < increment; count++)  {
        set_pixel(pb, x, y);
        x = x + dx;
        y = y + dy;
    }
}

#if 0
/*-----------------
*
*
*
*/
static void ellipse_draw_points(PBitmap pb, int xCenter, int yCenter, int x, int y) {
    line_dda(pb, xCenter + x, yCenter - y, xCenter - x, yCenter - y);
    line_dda(pb, xCenter + x, yCenter - y, xCenter - x, yCenter - y);
    //printf("GEO: ellipse_draw_line %d %d %d %d\n",xCenter, yCenter, x, y);
}
#endif

/*-----------------
*
*
*
*/
static void ellipse_draw_line(PBitmap pb, int xCenter, int yCenter, int x, int y) {
    line_dda(pb, xCenter + x, yCenter - y, xCenter - x, yCenter - y);
    line_dda(pb, xCenter + x, yCenter + y, xCenter - x, yCenter + y);
}

/*-----------------
*
*
*
*/
void dump_bitmap(PBitmap pb) {
    int y, x;
    Byte *p = pb->bitmap;
    printf("H=%d, W=%d",pb->h, pb->w);
    printf("\n----------------\n");
    for (y = 0; y < pb->h; y ++) {
        for (x = 0; x < pb->w; x ++, p++) {
            if (*p) { 
                printf("1");
            } else {
                printf("0");
            }
        }
        printf("\n");
    }
    printf("\n");
    exit(0);
}

/*-----------------
*
*
*
*/
static void fill_poly(PBitmap pb) {
    int y, x;
    int inside = 0;
    Byte *p = pb->bitmap;
    for (y = 0; y < pb->h; y ++) {
        inside = 0;
        for (x = 0; x < pb->w; x ++, p++) {
            if (*p) { 
                inside = !inside; 
            } else if (inside) {
                *p = 1;
            }
        }
    }
}


#define ROUND(a) ((int) (a + 0.5))

/*-----------------
*
*
*
*/
static void fill_ellipse(PBitmap pb, int x0, int y0, int x1, int y1) {
    int Rx = (x1 - x0) / 2;
    int Ry = (y1 - y0) / 2;
    int xCenter = Rx + x0;
    int yCenter = Ry + y0;;
    int Rx2 = Rx*Rx;
    int Ry2 = Ry*Ry;
    int twoRx2 = 2 * Rx2;
    int twoRy2 = 2 * Ry2;
    int p;
    int x = 0;
    int y = Ry;
    int px = 0;
    int py = twoRx2 * y;

    ellipse_draw_line(pb, xCenter, yCenter, x, y);

    /* For Region 1 */
    p = ROUND(Ry2 - (Rx2*Ry) + (0.25) * Rx2);
    while(px < py){
        x++;
        px += twoRy2;
        if (p < 0){
            p += Ry2 + px;
        } else {
            y--;
            py -= twoRx2;
            p += Ry2 + px - py;
        }
        ellipse_draw_line(pb, xCenter, yCenter, x, y);
    }
    /* For Region 2*/
    p = ROUND(Ry2 * (x + 0.5)*(x + 0.5) + Rx2 * (y - 1)*(y - 1) - Rx2 * Ry2);
    while(y > 0){
        y--;
        py -= twoRx2;
        if (p > 0) {
            p += Rx2 - py;
        } else {
            x++;
            px += twoRy2;
            p += Rx2 - py + px;
        }
        ellipse_draw_line(pb, xCenter, yCenter, x, y);
    }
}

//=============================================================================================
//
//    CLASS geo
//
//=============================================================================================

#define MAX_LATITUDE 85.05
#define BIT_RESOLUTION 100
#define N_WORK_BITS     25

#define PI 3.141592634

static int lon2tile_x(double lon, int z) { 
	return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z))); 
}

static int lat2tile_y(double lat, int z) { 
	return (int)(floor((1.0 - log( tan(lat * PI/180.0) + 1.0 / cos(lat * PI/180.0)) / PI) / 2.0 * pow(2.0, z))); 
}

#if 0
static double tilex2long(int x, int z) {
	return x / pow(2.0, z) * 360.0 - 180;
}

static double tiley2lat(int y, int z) {
	double n = PI - 2.0 * M_PI * y / pow(2.0, z);
	return 180.0 / PI * atan(0.5 * (exp(n) - exp(-n)));
}
#endif

/*--------------------------
*
*
*
*/
static int lat_lon_get_value(int tile_x, int tile_y, int pos, int n_bits) {
    long long acc = 0;
    long long  mask;
    int n;

    mask = 0x1000000UL;
    if (pos>1) {
        mask >>= pos-1;
    }
    acc = tile_x & mask;
    acc <<= 1;
    acc |= tile_y & mask;
    n = n_bits - pos;
    if (n) {
        acc >>= n;
    }

    return (int) acc;
}

/**
 * 
 * 
 */
static void lat_lon_to_address(double lat, double lon, int n_values, int *values) {
    
    int tile_x = lon2tile_x(lon, N_WORK_BITS);
    int tile_y = lat2tile_y(lat, N_WORK_BITS);
    // printf("LatLon: %f %d %f %d\n",lat,tile_y,lon,tile_x); fflush(stdout);
    //exit(0);
    //printf("address_from_lat_lon: n=%d\n",n);
    int i, n;
//    static int count = 0;
//    if (count < 50) printf("GEO for (%f, %f) => (%d %d):", lat, lon, tile_y, tile_x);
    for (i = 0, n = N_WORK_BITS; n; n--, i++) {
        values[i] = lat_lon_get_value(tile_x, tile_y, i+1, N_WORK_BITS);
 //       if (count < 50) printf(" %1d",values[i]);
    }
 //   if (count < 50) printf("\n");
//    count ++;
}


/**
 * 
 * 
 */
static void rec_lat_lon_to_address(PByte record, PRecordField prf, void *params, int *id_fields, int n_values, int *values) {
    int off0 = prf[id_fields[0]].record_offset;
    int off1 = prf[id_fields[1]].record_offset;
    //printf("Geo OFF: %d %d\n",off0, off1);

    double lat = * (double *) (record + off0);
    double lon = * (double *) (record + off1);


    lat_lon_to_address(lat, lon, n_values, values);
}

//=============================================================================================
//
//    OPs
//
//=============================================================================================

void sp(int i) {
    while (i) { 
        printf("  ");
        i --;
    }
}


#define QUAD_BIN(V) ((V)==0?"00":(V)==1?"01":(V)==2?"10":"11")

//==============================================================

#define X_MASK      0x02   // longitude first
#define Y_MASK      0x01   // then latitude 

#define X_PART(X)   ((X) & X_MASK)
#define Y_PART(X)   ((X) & Y_MASK)

#define TOP_BIT     0x0001
#define BOTTOM_BIT  0x0002
#define LEFT_BIT    0x0004
#define RIGHT_BIT   0x0008
#define ALL_BITS    ( BOTTOM_BIT | TOP_BIT | LEFT_BIT | RIGHT_BIT )

#define MAX_DEPTH   (N_WORK_BITS+6)   

typedef struct {
    int     flags[MAX_DEPTH];
    int     top_left[MAX_DEPTH];
    int     bottom_right[MAX_DEPTH];
    int     tile_x[MAX_DEPTH]; 
    int     tile_y[MAX_DEPTH];
    int     max_level;
    int     x0, y0;
    int     x1, y1;
    PBitmap pb;
} GeoData, *PGeoData;

#define  INRECT_NAME  "zrect"

/*--------------------------
*
*
*
*/
static int op_all(int v, int argset, PQArgs pa, int level, PQOpData op_data) {
    return QUERY_TRUE;
}

//=============================================================================================
//
//    ZRect
//
//=============================================================================================

/*--------------------------
*
*
*
*/
static int do_zrect(int v,  int argset, PQArgs args, int *indexes, int level, PQOpData op_data) {
    PGeoData pod = (PGeoData) op_data;
 
    int top_left = pod->top_left[level];
    int bottom_right = pod->bottom_right[level];
    int flags = pod->flags[level];

    int vx = X_PART(v);
    int vy = Y_PART(v);

    int top_bit = (flags & TOP_BIT);
    int bottom_bit = (flags & BOTTOM_BIT);
    int left_bit = (flags & LEFT_BIT);
    int right_bit = (flags & RIGHT_BIT);

    if ( 
        // LATITUDES:  TOP > BOTTOM ( ----  INVERTED ----)
        (top_bit || (vy >= Y_PART(top_left) ))
        &&
        (bottom_bit || (vy <= Y_PART(bottom_right) ))

        && 
        
        // LONGITUDES: LEFT > RIGHT
        (left_bit || (vx >= X_PART(top_left) ))
        &&
        (right_bit || (vx <= X_PART(bottom_right) ))
    ) {
        // compute the child flags
        int child_flags = flags;

        if (!top_bit && (vy > Y_PART(top_left))) child_flags |= TOP_BIT; 
        if (!bottom_bit && (vy < Y_PART(bottom_right))) child_flags |= BOTTOM_BIT; 
        if (!left_bit && (vx> X_PART(top_left))) child_flags |= LEFT_BIT; 
        if (!right_bit && (vx < X_PART(bottom_right))) child_flags |= RIGHT_BIT; 

#ifdef PRINT_2
         sp(level); printf("IDX:%2d: VALUE:%d %s TB:(%d/%d) vy: %d(%d/%d) LR: (%d/%d) vx: %d(%d/%d) FLAGS:%X\n",
           level, v, QUAD_BIN(v), 
            top_bit, bottom_bit, vy, Y_PART(top_left), Y_PART(bottom_right),  
            left_bit, right_bit, vx, X_PART(top_left), X_PART(bottom_right),  
           flags); fflush(stdout);
#endif        
        int child_tile_x, child_tile_y;

        if (indexes) {
            child_tile_x = pod->tile_x[level];
            child_tile_y = pod->tile_y[level];

            child_tile_x <<= 1;
            child_tile_y <<= 1;

            if (vx) child_tile_x |= 1;
            if (vy) child_tile_y |= 1;
        }

        // pode violar o tamanho maximo se nao for criado adequadamente
        level ++;
        pod->flags[level] = child_flags;

        //int excess = N_WORK_BITS - level;
        int res = QUERY_TRUE;

        if (!indexes) {
            if (pod->flags[level] == ALL_BITS) {
                res |= QUERY_DIM_DONE;

            } else if (level >= pod->max_level) {
                res |= QUERY_DIM_DONE;
            }

        } else {
           
            pod->tile_x[level] = child_tile_x;
            pod->tile_y[level] = child_tile_y;

            if (level >= pod->max_level) {
                indexes[0] = child_tile_y - pod->y0;
                indexes[1] = child_tile_x - pod->x0;
    /*
                printf("Geo: indexes %d %d\n",indexes[0],indexes[1]);
                printf("Geo: tiles   %d %d\n",child_tile_y,child_tile_x);
                printf("Geo: bases   %d %d\n",pod->y0,pod->x0);
    */
                res |= QUERY_DIM_DONE;
            }

        }
        return res;
    }
#ifdef PRINT_2
    sp(level); printf("ERR IDX:%2d: VALUE:%d %s  TB:(%d/%d) vy: %d(%d/%d) LR: (%d/%d) vx: %d(%d/%d) FLAGS:%X BITS: %d %d %d %d\n",
        level, v, QUAD_BIN(v), 
        top_bit, bottom_bit, vy, Y_PART(top_left), Y_PART(bottom_right),  
        left_bit, right_bit, vx, X_PART(top_left), X_PART(bottom_right),  
        flags, 
        top_bit, left_bit, bottom_bit, right_bit); fflush(stdout);
#endif        
    return QUERY_FALSE;
}


/*--------------------------
*
*
*
*/
static int op_zrect(int v,  int argset, PQArgs args, int level, PQOpData op_data) {
    return do_zrect(v, argset, args, NULL, level, op_data);

}

    
/*--------------------------
*
*
*
*/
static int op_zrect_group_by(int v, int argset, PQArgs args, int *indexes, int level, PQOpData op_data) {
    return do_zrect(v, argset, args, indexes, level, op_data);
}

//=============================================================================================
//
//    zellipse
//
//=============================================================================================

/*--------------------------
*
*
*
*/
static int do_bitmap(int v,  int argset, PQArgs args, int *indexes, int level, PQOpData op_data) {
    PGeoData pod = (PGeoData) op_data;
 
    //printf("Geo: do bitmap 1 level = %d\n",level);
    int top_left = pod->top_left[level];
    int bottom_right = pod->bottom_right[level];
    int flags = pod->flags[level];
    //printf("Geo: do bitmap 1b\n");

    int vx = X_PART(v);
    int vy = Y_PART(v);

    int top_bit = (flags & TOP_BIT);
    int bottom_bit = (flags & BOTTOM_BIT);
    int left_bit = (flags & LEFT_BIT);
    int right_bit = (flags & RIGHT_BIT);
    //printf("Geo: do bitmap 2\n");

    if ( 
        // LATITUDES:  TOP > BOTTOM ( ----  INVERTED ----)
        (top_bit || (vy >= Y_PART(top_left) ))
        &&
        (bottom_bit || (vy <= Y_PART(bottom_right) ))

        && 
        
        // LONGITUDES: LEFT > RIGHT
        (left_bit || (vx >= X_PART(top_left) ))
        &&
        (right_bit || (vx <= X_PART(bottom_right) ))
    ) {
        // compute the child flags
        int child_flags = flags;

        if (!top_bit && (vy > Y_PART(top_left))) child_flags |= TOP_BIT; 
        if (!bottom_bit && (vy < Y_PART(bottom_right))) child_flags |= BOTTOM_BIT; 
        if (!left_bit && (vy > X_PART(top_left))) child_flags |= LEFT_BIT; 
        if (!right_bit && (vy < X_PART(bottom_right))) child_flags |= RIGHT_BIT; 

#ifdef PRINT_2
         sp(level); printf("IDX:%2d: VALUE:%d %s TB:(%d/%d) vy: %d(%d/%d) LR: (%d/%d) vx: %d(%d/%d) FLAGS:%X\n",
           level, v, QUAD_BIN(v), 
            top_bit, bottom_bit, vy, Y_PART(top_left), Y_PART(bottom_right),  
            left_bit, right_bit, vx, X_PART(top_left), X_PART(bottom_right),  
           flags); fflush(stdout);
#endif        

        //printf("Geo: do bitmap 3\n");
        int child_tile_x = pod->tile_x[level];
        int child_tile_y = pod->tile_y[level];
        //printf("Geo: do bitmap 4\n");

        child_tile_x <<= 1;
        child_tile_y <<= 1;

        if (vx) child_tile_x |= 1;
        if (vy) child_tile_y |= 1;

        // pode violar o tamanho maximo se nao for criado adequadamente
        level ++;
        pod->flags[level] = child_flags;
        pod->tile_x[level] = child_tile_x;
        pod->tile_y[level] = child_tile_y;
        //printf("Geo: tiles   %d %d\n",child_tile_y,child_tile_x);

        int res;
        if (level < pod->max_level) {
            res = QUERY_TRUE;

        } else if (!get_pixel(pod->pb, child_tile_x, child_tile_y)) {
            res = QUERY_FALSE;

        } else {
            if (indexes) {
                indexes[0] = child_tile_y - pod->y0;
                indexes[1] = child_tile_x - pod->x0;
    /*
                printf("Geo: indexes %d %d\n",indexes[0],indexes[1]);
                printf("Geo: tiles   %d %d\n",child_tile_y,child_tile_x);
                printf("Geo: bases   %d %d\n",pod->y0,pod->x0);
    */
            }
            res = QUERY_TRUE | QUERY_DIM_DONE;
        }
        return res;
    }
#ifdef PRINT_2
    sp(level); printf("ERR IDX:%2d: VALUE:%d %s  TB:(%d/%d) vy: %d(%d/%d) LR: (%d/%d) vx: %d(%d/%d) FLAGS:%X BITS: %d %d %d %d\n",
        level, v, QUAD_BIN(v), 
        top_bit, bottom_bit, vy, Y_PART(top_left), Y_PART(bottom_right),  
        left_bit, right_bit, vx, X_PART(top_left), X_PART(bottom_right),  
        flags, 
        top_bit, left_bit, bottom_bit, right_bit); fflush(stdout);
#endif        
    return QUERY_FALSE;
}

/*--------------------------
*
*
*
*/
static int op_zellipse(int v,  int argset, PQArgs args, int level, PQOpData op_data) {
    return do_bitmap(v, argset, args, NULL, level, op_data);

}
    
/*--------------------------
*
*
*
*/
static int op_zellipse_group_by(int v, int argset, PQArgs args, int *indexes, int level, PQOpData op_data) {
    return do_bitmap(v, argset, args, indexes, level, op_data);
}

/*--------------------------
*
*
*
*/
static int op_zpoly(int v,  int argset, PQArgs args, int level, PQOpData op_data) {
    return do_bitmap(v, argset, args, NULL, level, op_data);
}
    
/*--------------------------
*
*
*
*/
static int op_zpoly_group_by(int v, int argset, PQArgs args, int *indexes, int level, PQOpData op_data) {
    return do_bitmap(v, argset, args, indexes, level, op_data);
}

//=============================================================================================
//
//    Register
//
//=============================================================================================

#define REGISTER_OP(X,S,Y)  \
    static QOpInfo qoi_##X;\
    qoi_##X.op = op_##X;\
    qoi_##X.op_group_by = op_##X##_group_by;\
    qoi_##X.min_args = Y;\
    qoi_##X.max_args = MAX_ARGS;\
    qoi_##X.fmt = "IDDDd"   ;\
    register_op("geo", S, &qoi_##X)

/*-----------------
*
*
*
*/
static void register_ops(void) {
    REGISTER_OP(zrect,"zrect",5);
    qoi_zrect.max_args = 5;

    REGISTER_OP(zellipse,"zellipse",5);
    qoi_zellipse.max_args = 5;

    REGISTER_OP(zpoly,"zpoly",101);
    qoi_zpoly.max_args = 101;

}

//=============================================================================================
//
//    
//
//=============================================================================================

/*--------------------------
*
*
*
*/
static int get_distinct_info(void *op, PQArgs pa, PByte params, int bin_size, int *distinct_count, int *distinct_base) {
    //printf("GEO: 1\n");
    if (op == op_zrect) {
        //printf("GEO: 2\n");
        int zoom = pa->args[0].i;
        double lat0 = pa->args[1].d;
        double lon0 = pa->args[2].d;
        double lat1 = pa->args[3].d;
        double lon1 = pa->args[4].d;

        if (lat1 > lat0) query_fatal("Bottom Latitude is greather than Top latitude.");
        if (lon0 > lon1) query_fatal("Right Latitude is greather than Left latitude.");

        int tile_x0 = lon2tile_x(lon0, zoom);
        int tile_x1 = lon2tile_x(lon1, zoom);
        if (tile_x0 == tile_x1) tile_x1 ++; 

        int tile_y0 = lat2tile_y(lat0, zoom);
        int tile_y1 = lat2tile_y(lat1, zoom);
        if (tile_y0 == tile_y1) tile_y1 ++;

        int d;
        distinct_count[0] = ((d = tile_y1 - tile_y0 + 1) < 0)? -d: d;;
        distinct_base[0] = d < 0? tile_y1: tile_y0;
        distinct_count[1] = ((d = tile_x1 - tile_x0 + 1) < 0)? -d: d;
        distinct_base[1] = d < 0? tile_x1: tile_x0;
/*
        printf("GEO: (%d,%d) (%d,%d)\n", 
            distinct_count[0], distinct_base[0],
            distinct_count[1], distinct_base[1]
            );
*/
        return 1;
    }
    return 0;
}


//=============================================================================================
//
//    
//
//=============================================================================================


/*--------------------------
*
*
*
*/
static PGeoData create_geodata(PQArgs pa) {
    PGeoData pg = calloc(1,sizeof(GeoData));

#ifdef PRINT_1
    printf("%d: ", pa->args[0].i);
    for (int i=1;i<pa->n_args;i++) {
        printf("%11.7f, ",pa->args[i].d);
    }
    printf("\n");
#endif        

    int zoom = pa->args[0].i;
    if (zoom >= MAX_DEPTH) { query_fatal("GEO: Zoom Overflow"); }

    double lat_top = pa->args[1].d;
    double lon_left = pa->args[2].d;

    double lat_bottom = pa->args[3].d;
    double lon_right = pa->args[4].d;

    if (lat_bottom > lat_top) query_fatal("Bottom Latitude is greather than Top latitude.");
    if (lon_left > lon_right) query_fatal("Left Latitude is greather than Right latitude.");
    // printf("GEO: coordinates checked\n"); fflush(stdout);

    lat_lon_to_address(lat_top,lon_left,zoom,pg->top_left);
    lat_lon_to_address(lat_bottom,lon_right,zoom,pg->bottom_right);
    pg->max_level = zoom;

    pg->x0 = lon2tile_x(lon_left, zoom);
    pg->y0 = lat2tile_y(lat_top , zoom);

    pg->x1 = lon2tile_x(lon_right, zoom);
    pg->y1 = lat2tile_y(lat_bottom , zoom);

    return pg;
}

/*--------------------------
*
*
*
*/
static PGeoData create_zpoly_geodata(PQArgs pa) {
    PGeoData pg = calloc(1,sizeof(GeoData));

#ifdef PRINT_1
    printf("%d: ", pa->args[0].i);
    for (int i=1;i<pa->n_args;i++) {
        printf("%11.7f, ",pa->args[i].d);
    }
    printf("\n");
#endif        

    int zoom = pa->args[0].i;
    if (zoom >= MAX_DEPTH) query_fatal("ZPOLY: Zoom Overflow"); 
   
    int n = pa->n_args - 1;
    if (n % 2 != 0)  query_fatal("ZPOLY: number of coordinates are not even.");

    if (n/2 < 3)  {
        query_fatal("ZPOLY: number of coordinates [%d] must be greather than 2.",n/2);
    }

    int max_coords = (MAX_ARGS - 1) / 2;
    if (n/2 > max_coords)  {
        query_fatal("ZPOLY: number of coordinates [%d] exceeds the maximum limit [%d].",n/2,max_coords);
    }

    int i;
    double lat0 = -300, lon0=300, lat1=300, lon1=-300;
    double lat, lon;

    // acha o menor retangulo que envolve o poligono
    for (i = 1; i < n; i += 2) {
        lat = pa->args[i].d;
        lon = pa->args[i+1].d;

        if (lat > lat0) lat0=lat;  // lat0 > lat1
        if (lat < lat1) lat1=lat;
        if (lon < lon0) lon0=lon;  // lon1 > lon0
        if (lon > lon1) lon1=lon;
    }

    if (lat1 > lat0) query_fatal("Bottom Latitude is greather than Top latitude.");
    if (lon0 > lon1) query_fatal("Left Latitude is greather than Right latitude.");

    // printf("GEO: coordinates checked\n"); fflush(stdout);
    lat_lon_to_address(lat0,lon0,zoom,pg->top_left);
    lat_lon_to_address(lat1,lon1,zoom,pg->bottom_right);

    pg->max_level = zoom;

    pg->x0 = lon2tile_x(lon0, zoom);
    pg->y0 = lat2tile_y(lat0, zoom);

    pg->x1 = lon2tile_x(lon1, zoom);
    pg->y1 = lat2tile_y(lat1, zoom);

    pg->pb = create_bitmap(pg->x0, pg->y0, pg->x1, pg->y1);

    // traca as linhas do poligono
    int xb = lon2tile_x(pa->args[2].d, zoom);
    int yb = lat2tile_y(pa->args[1].d, zoom);
    int y_ant = yb; int x_ant = xb;
    for (i = 3; i < n; i += 2) {
        int y = lat2tile_y(pa->args[i].d, zoom);
        int x = lon2tile_x(pa->args[i+1].d, zoom);
        line_dda(pg->pb, x_ant, y_ant, x, y);
        x_ant = x; y_ant = y;
    }
    line_dda(pg->pb, x_ant, y_ant, xb, yb); // fecha o poligono

    // faz o fill do poligono
    fill_poly(pg->pb);

    return pg;
}

/*--------------------------
*
*
*
*/
static void *create_op_data(void *op, PQArgs pa) {
    PGeoData pg = NULL;
    
    if (op == op_zrect) {
        pg = create_geodata(pa);

    } else if (op == op_zellipse) {
        pg = create_geodata(pa);

        pg->pb = create_bitmap(pg->x0, pg->y0, pg->x1, pg->y1);
        fill_ellipse(pg->pb,pg->x0, pg->y0, pg->x1, pg->y1);

    } else if (op == op_zpoly) {
        pg = create_zpoly_geodata(pa);
    }
    return pg;
}

/*--------------------------
*
*
*
*/
static void destroy_op_data(void *op, void *op_data) {
    PGeoData pg = (PGeoData) op_data;
    if (op == op_zrect) {
        free(pg);
    } else if (op == op_zellipse) {
        if (pg->pb) free(pg->pb);
        free(pg);
    }
}

//=============================================================================================
//
//    
//
//=============================================================================================

/*--------------------------
*
*
*
*/
void register_class_geo(void) {

    // prevention against reinitialization during execution
    static int done = 0; 
    if (!done) done = 1; else return;
    
    static Class c;
    c.name = "geo";
    c.class_type = DIMENSIONAL_CLASS;

    c.to_address = rec_lat_lon_to_address;
    c.to_address_types = "DD"; // Double Double
    c.to_address_fmt = "FF"; // Field Field

    c.op_all_plain = op_all;
    c.op_all_group_by = NULL; // lat_lon_op_all_group_by;

    c.n_group_by_indexes = 2;
    c.group_by_requires_rule = 1;

    c.get_distinct_info = &get_distinct_info;
    c.create_op_data = &create_op_data;
    c.destroy_op_data = &destroy_op_data;

    register_class(&c);
    register_ops();
}
