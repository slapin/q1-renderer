#include <stddef.h>
#include <math.h>
#define MAXHEIGHT 1024
#define DPS_MAXSPANS MAXHEIGHT+1
#define CACHE_SIZE 32
#define ALIAS_ONSEAM 0x0020
#define MAX_LBM_HEIGHT 480

typedef unsigned char byte;
typedef int fixed8_t;
typedef enum { ALIAS_SKIN_SINGLE=0, ALIAS_SKIN_GROUP } aliasskintype_t;
typedef struct mtriangle_s {
    int                 facesfront;
    int                 vertindex[3];
} mtriangle_t;
typedef struct finalvert_s {
    int     v[6];       // u, v, s, t, l, 1/z
    int     flags;
    float   reserved;
} finalvert_t;
typedef struct {
    void            *pdest;
    short           *pz;
    int             count;
    byte            *ptex;
    int             sfrac, tfrac, light, zi;
} spanpackage_t;
typedef struct {
    aliasskintype_t     type;
    int                 skin;
} maliasskindesc_t;
typedef struct {
    void                *pskin;
    maliasskindesc_t    *pskindesc;
    int                 skinwidth;
    int                 skinheight;
    mtriangle_t         *ptriangles;
    finalvert_t         *pfinalverts;
    int                 numtriangles;
    int                 drawtype;
    int                 seamfixupX16;
} affinetridesc_t;
typedef byte pixel_t;
typedef struct {
    int     isflattop;
    int     numleftedges;
    int     *pleftedgevert0;
    int     *pleftedgevert1;
    int     *pleftedgevert2;
    int     numrightedges;
    int     *prightedgevert0;
    int     *prightedgevert1;
    int     *prightedgevert2;
} edgetable;
typedef struct {
    int     quotient;
    int     remainder;
} adivtab_t;

/* Input params */
static affinetridesc_t r_affinetridesc;
/* End of input params */

static spanpackage_t *a_spans;
static spanpackage_t *d_pedgespanpackage;

static byte *d_pcolormap;
static void *acolormap;
static int d_xdenom;
static int r_p0[6], r_p1[6], r_p2[6];
static int ystart;
static int d_aspancount;
static byte *d_ptex;
static int d_sfrac, d_tfrac;
static int d_pzbasestep;
static int d_pzextrastep;
static int d_light;
static int d_zi;
static int d_pdestbasestep;
static int screenwidth;
static int d_pdestextrastep;
static byte *d_pdest;
static pixel_t *d_viewbuffer;
static short *d_pz;
static short *d_pzbuffer;
static int r_lstepx;
static int d_countextrastep;
static int d_ptexbasestep;
static int r_sstepy;
static int r_sstepx;
static int r_tstepy;
static int r_tstepx;
static int d_sfracbasestep;
static int d_tfracbasestep;
static int d_lightbasestep;
static int r_lstepy;
static int d_zibasestep;
static int r_zistepy;
static int r_zistepx;
static int d_ptexextrastep;
static int d_sfracextrastep;
static int d_tfracextrastep;
static int d_lightextrastep;
static int d_ziextrastep;
static int a_sstepxfrac;
static int a_tstepxfrac;
static int a_ststepxwhole;


static unsigned int d_zwidth;
static int ubasestep;
static int errorterm, erroradjustup, erroradjustdown;

static edgetable *pedgetable;
short *zspantable[MAXHEIGHT];
static int d_scantable[MAXHEIGHT];
static byte *skintable[MAX_LBM_HEIGHT];
static edgetable   edgetables[12] = {
    {0, 1, r_p0, r_p2, NULL, 2, r_p0, r_p1, r_p2 },
    {0, 2, r_p1, r_p0, r_p2,   1, r_p1, r_p2, NULL},
    {1, 1, r_p0, r_p2, NULL, 1, r_p1, r_p2, NULL},
    {0, 1, r_p1, r_p0, NULL, 2, r_p1, r_p2, r_p0 },
    {0, 2, r_p0, r_p2, r_p1,   1, r_p0, r_p1, NULL},
    {0, 1, r_p2, r_p1, NULL, 1, r_p2, r_p0, NULL},
    {0, 1, r_p2, r_p1, NULL, 2, r_p2, r_p0, r_p1 },
    {0, 2, r_p2, r_p1, r_p0,   1, r_p2, r_p0, NULL},
    {0, 1, r_p1, r_p0, NULL, 1, r_p1, r_p2, NULL},
    {1, 1, r_p2, r_p1, NULL, 1, r_p0, r_p1, NULL},
    {1, 1, r_p1, r_p0, NULL, 1, r_p2, r_p0, NULL},
    {0, 1, r_p0, r_p2, NULL, 1, r_p0, r_p1, NULL},
};
static adivtab_t adivtab[32*32] = {
{1, 0}, {1, -1}, {1, -2}, {1, -3}, {1, -4}, {1, -5},
{1, -6}, {1, -7}, {2, -1}, {2, -3}, {3, 0}, {3, -3},
{5, 0}, {7, -1}, {15, 0}, {0, 0}, {-15, 0}, {-8, 1},
{-5, 0}, {-4, 1}, {-3, 0}, {-3, 3}, {-3, 6}, {-2, 1},
{-2, 3}, {-2, 5}, {-2, 7}, {-2, 9}, {-2, 11}, {-2, 13},
{-1, 0}, {-1, 1},
// numerator = -14
{0, -14}, {1, 0}, {1, -1}, {1, -2}, {1, -3}, {1, -4},
{1, -5}, {1, -6}, {2, 0}, {2, -2}, {2, -4}, {3, -2},
{4, -2}, {7, 0}, {14, 0}, {0, 0}, {-14, 0}, {-7, 0},
{-5, 1}, {-4, 2}, {-3, 1}, {-3, 4}, {-2, 0}, {-2, 2},
{-2, 4}, {-2, 6}, {-2, 8}, {-2, 10}, {-2, 12}, {-1, 0},
{-1, 1}, {-1, 2},
// numerator = -13
{0, -13}, {0, -13}, {1, 0}, {1, -1}, {1, -2}, {1, -3},
{1, -4}, {1, -5}, {1, -6}, {2, -1}, {2, -3}, {3, -1},
{4, -1}, {6, -1}, {13, 0}, {0, 0}, {-13, 0}, {-7, 1},
{-5, 2}, {-4, 3}, {-3, 2}, {-3, 5}, {-2, 1}, {-2, 3},
{-2, 5}, {-2, 7}, {-2, 9}, {-2, 11}, {-1, 0}, {-1, 1},
{-1, 2}, {-1, 3},
// numerator = -12
{0, -12}, {0, -12}, {0, -12}, {1, 0}, {1, -1}, {1, -2},
{1, -3}, {1, -4}, {1, -5}, {2, 0}, {2, -2}, {3, 0}, {4, 0},
{6, 0}, {12, 0}, {0, 0}, {-12, 0}, {-6, 0}, {-4, 0}, {-3, 0},
{-3, 3}, {-2, 0}, {-2, 2}, {-2, 4}, {-2, 6}, {-2, 8}, {-2, 10},
{-1, 0}, {-1, 1}, {-1, 2}, {-1, 3}, {-1, 4},
// numerator = -11
{0, -11}, {0, -11}, {0, -11}, {0, -11}, {1, 0}, {1, -1}, {1, -2},
{1, -3}, {1, -4}, {1, -5}, {2, -1}, {2, -3}, {3, -2}, {5, -1},
{11, 0}, {0, 0}, {-11, 0}, {-6, 1}, {-4, 1}, {-3, 1}, {-3, 4},
{-2, 1}, {-2, 3}, {-2, 5}, {-2, 7}, {-2, 9}, {-1, 0}, {-1, 1},
{-1, 2}, {-1, 3}, {-1, 4}, {-1, 5},
// numerator = -10
{0, -10}, {0, -10}, {0, -10}, {0, -10}, {0, -10}, {1, 0}, {1, -1}, {1, -2},
{1, -3}, {1, -4}, {2, 0}, {2, -2}, {3, -1}, {5, 0}, {10, 0}, {0, 0},
{-10, 0}, {-5, 0}, {-4, 2}, {-3, 2}, {-2, 0}, {-2, 2}, {-2, 4}, {-2, 6},
{-2, 8}, {-1, 0}, {-1, 1}, {-1, 2}, {-1, 3}, {-1, 4}, {-1, 5}, {-1, 6},
// numerator = -9
{0, -9}, {0, -9}, {0, -9}, {0, -9}, {0, -9}, {0, -9}, {1, 0},
{1, -1}, {1, -2}, {1, -3}, {1, -4}, {2, -1}, {3, 0}, {4, -1},
{9, 0}, {0, 0}, {-9, 0}, {-5, 1}, {-3, 0}, {-3, 3}, {-2, 1},
{-2, 3}, {-2, 5}, {-2, 7}, {-1, 0}, {-1, 1}, {-1, 2}, {-1, 3},
{-1, 4}, {-1, 5}, {-1, 6}, {-1, 7},
// numerator = -8
{0, -8}, {0, -8}, {0, -8}, {0, -8}, {0, -8}, {0, -8}, {0, -8}, {1, 0},
{1, -1}, {1, -2}, {1, -3}, {2, 0}, {2, -2}, {4, 0}, {8, 0}, {0, 0},
{-8, 0}, {-4, 0}, {-3, 1}, {-2, 0}, {-2, 2}, {-2, 4}, {-2, 6}, {-1, 0},
{-1, 1}, {-1, 2}, {-1, 3}, {-1, 4}, {-1, 5}, {-1, 6}, {-1, 7}, {-1, 8},
// numerator = -7
{0, -7}, {0, -7}, {0, -7}, {0, -7}, {0, -7}, {0, -7}, {0, -7}, {0, -7},
{1, 0}, {1, -1}, {1, -2}, {1, -3}, {2, -1}, {3, -1}, {7, 0}, {0, 0},
{-7, 0}, {-4, 1}, {-3, 2}, {-2, 1}, {-2, 3}, {-2, 5}, {-1, 0}, {-1, 1},
{-1, 2}, {-1, 3}, {-1, 4}, {-1, 5}, {-1, 6}, {-1, 7}, {-1, 8}, {-1, 9},
// numerator = -6
{0, -6}, {0, -6}, {0, -6}, {0, -6}, {0, -6}, {0, -6}, {0, -6}, {0, -6},
{0, -6}, {1, 0}, {1, -1}, {1, -2}, {2, 0}, {3, 0}, {6, 0}, {0, 0},
{-6, 0}, {-3, 0}, {-2, 0}, {-2, 2}, {-2, 4}, {-1, 0}, {-1, 1}, {-1, 2},
{-1, 3}, {-1, 4}, {-1, 5}, {-1, 6}, {-1, 7}, {-1, 8}, {-1, 9}, {-1, 10},
// numerator = -5
{0, -5}, {0, -5}, {0, -5}, {0, -5}, {0, -5}, {0, -5}, {0, -5}, {0, -5},
{0, -5}, {0, -5}, {1, 0}, {1, -1}, {1, -2}, {2, -1}, {5, 0}, {0, 0},
{-5, 0}, {-3, 1}, {-2, 1}, {-2, 3}, {-1, 0}, {-1, 1}, {-1, 2}, {-1, 3},
{-1, 4}, {-1, 5}, {-1, 6}, {-1, 7}, {-1, 8}, {-1, 9}, {-1, 10}, {-1, 11},
// numerator = -4
{0, -4}, {0, -4}, {0, -4}, {0, -4}, {0, -4}, {0, -4}, {0, -4}, {0, -4},
{0, -4}, {0, -4}, {0, -4}, {1, 0}, {1, -1}, {2, 0}, {4, 0}, {0, 0},
{-4, 0}, {-2, 0}, {-2, 2}, {-1, 0}, {-1, 1}, {-1, 2}, {-1, 3}, {-1, 4},
{-1, 5}, {-1, 6}, {-1, 7}, {-1, 8}, {-1, 9}, {-1, 10}, {-1, 11}, {-1, 12},
// numerator = -3
{0, -3}, {0, -3}, {0, -3}, {0, -3}, {0, -3}, {0, -3}, {0, -3}, {0, -3},
{0, -3}, {0, -3}, {0, -3}, {0, -3}, {1, 0}, {1, -1}, {3, 0}, {0, 0},
{-3, 0}, {-2, 1}, {-1, 0}, {-1, 1}, {-1, 2}, {-1, 3}, {-1, 4}, {-1, 5},
{-1, 6}, {-1, 7}, {-1, 8}, {-1, 9}, {-1, 10}, {-1, 11}, {-1, 12}, {-1, 13},
// numerator = -2
{0, -2}, {0, -2}, {0, -2}, {0, -2}, {0, -2}, {0, -2}, {0, -2}, {0, -2},
{0, -2}, {0, -2}, {0, -2}, {0, -2}, {0, -2}, {1, 0}, {2, 0}, {0, 0},
{-2, 0}, {-1, 0}, {-1, 1}, {-1, 2}, {-1, 3}, {-1, 4}, {-1, 5}, {-1, 6},
{-1, 7}, {-1, 8}, {-1, 9}, {-1, 10}, {-1, 11}, {-1, 12}, {-1, 13}, {-1, 14},
// numerator = -1
{0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1},
{0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {1, 0}, {0, 0},
{-1, 0}, {-1, 1}, {-1, 2}, {-1, 3}, {-1, 4}, {-1, 5}, {-1, 6}, {-1, 7},
{-1, 8}, {-1, 9}, {-1, 10}, {-1, 11}, {-1, 12}, {-1, 13}, {-1, 14}, {-1, 15},
// numerator = 0
{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
// numerator = 1
{-1, -14}, {-1, -13}, {-1, -12}, {-1, -11}, {-1, -10}, {-1, -9}, {-1, -8},
{-1, -7}, {-1, -6}, {-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1},
{-1, 0}, {0, 0}, {1, 0}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
{0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
// numerator = 2
{-1, -13}, {-1, -12}, {-1, -11}, {-1, -10}, {-1, -9}, {-1, -8}, {-1, -7},
{-1, -6}, {-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1}, {-1, 0},
{-2, 0}, {0, 0}, {2, 0}, {1, 0}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
{0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
// numerator = 3
{-1, -12}, {-1, -11}, {-1, -10}, {-1, -9}, {-1, -8}, {-1, -7}, {-1, -6},
{-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -1}, {-3, 0},
{0, 0}, {3, 0}, {1, 1}, {1, 0}, {0, 3}, {0, 3}, {0, 3}, {0, 3}, {0, 3},
{0, 3}, {0, 3}, {0, 3}, {0, 3}, {0, 3}, {0, 3}, {0, 3}, {0, 3},
// numerator = 4
{-1, -11}, {-1, -10}, {-1, -9}, {-1, -8}, {-1, -7}, {-1, -6}, {-1, -5}, {-1, -4},
{-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -2}, {-2, 0}, {-4, 0}, {0, 0}, {4, 0},
{2, 0}, {1, 1}, {1, 0}, {0, 4}, {0, 4}, {0, 4}, {0, 4}, {0, 4}, {0, 4}, {0, 4},
{0, 4}, {0, 4}, {0, 4}, {0, 4}, {0, 4},
// numerator = 5
{-1, -10}, {-1, -9}, {-1, -8}, {-1, -7}, {-1, -6}, {-1, -5}, {-1, -4},
{-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -3}, {-2, -1}, {-3, -1},
{-5, 0}, {0, 0}, {5, 0}, {2, 1}, {1, 2}, {1, 1}, {1, 0},
{0, 5}, {0, 5}, {0, 5}, {0, 5}, {0, 5}, {0, 5}, {0, 5},
{0, 5}, {0, 5}, {0, 5}, {0, 5},
// numerator = 6
{-1, -9}, {-1, -8}, {-1, -7}, {-1, -6}, {-1, -5}, {-1, -4}, {-1, -3},
{-1, -2}, {-1, -1}, {-1, 0}, {-2, -4}, {-2, -2}, {-2, 0}, {-3, 0},
{-6, 0}, {0, 0}, {6, 0}, {3, 0}, {2, 0}, {1, 2}, {1, 1},
{1, 0}, {0, 6}, {0, 6}, {0, 6}, {0, 6}, {0, 6}, {0, 6},
{0, 6}, {0, 6}, {0, 6}, {0, 6},
// numerator = 7
{-1, -8}, {-1, -7}, {-1, -6}, {-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1},
{-1, 0}, {-2, -5}, {-2, -3}, {-2, -1}, {-3, -2}, {-4, -1}, {-7, 0}, {0, 0},
{7, 0}, {3, 1}, {2, 1}, {1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 7}, {0, 7},
{0, 7}, {0, 7}, {0, 7}, {0, 7}, {0, 7}, {0, 7}, {0, 7},
// numerator = 8
{-1, -7}, {-1, -6}, {-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1}, {-1, 0},
{-2, -6}, {-2, -4}, {-2, -2}, {-2, 0}, {-3, -1}, {-4, 0}, {-8, 0}, {0, 0}, {8, 0},
{4, 0}, {2, 2}, {2, 0}, {1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 8}, {0, 8}, {0, 8},
{0, 8}, {0, 8}, {0, 8}, {0, 8}, {0, 8},
// numerator = 9
{-1, -6}, {-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -7},
{-2, -5}, {-2, -3}, {-2, -1}, {-3, -3}, {-3, 0}, {-5, -1}, {-9, 0}, {0, 0}, {9, 0},
{4, 1}, {3, 0}, {2, 1}, {1, 4}, {1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 9}, {0, 9},
{0, 9}, {0, 9}, {0, 9}, {0, 9}, {0, 9},
// numerator = 10
{-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -8},
{-2, -6}, {-2, -4}, {-2, -2}, {-2, 0}, {-3, -2}, {-4, -2}, {-5, 0},
{-10, 0}, {0, 0}, {10, 0}, {5, 0}, {3, 1}, {2, 2}, {2, 0}, {1, 4}, {1, 3},
{1, 2}, {1, 1}, {1, 0}, {0, 10}, {0, 10}, {0, 10}, {0, 10}, {0, 10}, {0, 10},
// numerator = 11
{-1, -4}, {-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -9}, {-2, -7},
{-2, -5}, {-2, -3}, {-2, -1}, {-3, -4}, {-3, -1}, {-4, -1}, {-6, -1},
{-11, 0}, {0, 0}, {11, 0}, {5, 1}, {3, 2}, {2, 3}, {2, 1}, {1, 5},
{1, 4}, {1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 11}, {0, 11}, {0, 11},
{0, 11}, {0, 11},
// numerator = 12
{-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -10}, {-2, -8}, {-2, -6},
{-2, -4}, {-2, -2}, {-2, 0}, {-3, -3}, {-3, 0}, {-4, 0}, {-6, 0}, {-12, 0},
{0, 0}, {12, 0}, {6, 0}, {4, 0}, {3, 0}, {2, 2}, {2, 0}, {1, 5}, {1, 4},
{1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 12}, {0, 12}, {0, 12}, {0, 12},
// numerator = 13
{-1, -2}, {-1, -1}, {-1, 0}, {-2, -11}, {-2, -9}, {-2, -7}, {-2, -5}, {-2, -3},
{-2, -1}, {-3, -5}, {-3, -2}, {-4, -3}, {-5, -2}, {-7, -1}, {-13, 0}, {0, 0},
{13, 0}, {6, 1}, {4, 1}, {3, 1}, {2, 3}, {2, 1}, {1, 6}, {1, 5}, {1, 4}, {1, 3},
{1, 2}, {1, 1}, {1, 0}, {0, 13}, {0, 13}, {0, 13},
// numerator = 14
{-1, -1}, {-1, 0}, {-2, -12}, {-2, -10}, {-2, -8}, {-2, -6}, {-2, -4}, {-2, -2},
{-2, 0}, {-3, -4}, {-3, -1}, {-4, -2}, {-5, -1}, {-7, 0}, {-14, 0}, {0, 0}, {14, 0},
{7, 0}, {4, 2}, {3, 2}, {2, 4}, {2, 2}, {2, 0}, {1, 6}, {1, 5}, {1, 4}, {1, 3},
{1, 2}, {1, 1}, {1, 0}, {0, 14}, {0, 14},
// numerator = 15
{-1, 0}, {-2, -13}, {-2, -11}, {-2, -9}, {-2, -7}, {-2, -5}, {-2, -3}, {-2, -1},
{-3, -6}, {-3, -3}, {-3, 0}, {-4, -1}, {-5, 0}, {-8, -1}, {-15, 0}, {0, 0},
{15, 0}, {7, 1}, {5, 0}, {3, 3}, {3, 0}, {2, 3}, {2, 1}, {1, 7}, {1, 6}, {1, 5},
{1, 4}, {1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 15},
// numerator = 16
{-2, -14}, {-2, -12}, {-2, -10}, {-2, -8}, {-2, -6}, {-2, -4}, {-2, -2}, {-2, 0},
{-3, -5}, {-3, -2}, {-4, -4}, {-4, 0}, {-6, -2}, {-8, 0}, {-16, 0}, {0, 0},
{16, 0}, {8, 0}, {5, 1}, {4, 0}, {3, 1}, {2, 4}, {2, 2}, {2, 0}, {1, 7},
{1, 6}, {1, 5}, {1, 4}, {1, 3}, {1, 2}, {1, 1}, {1, 0},
};


void D_PolysetRecursiveTriangle(int *lp1, int *lp2, int *lp3)
/*
Definition at line 334 of file d_polyse.c.

References d_pcolormap, d_scantable, d_viewbuffer, skintable, and zspantable.

Referenced by D_DrawSubdiv().
*/
{
    int     *temp;
    int     d;
    int     new[6];
    int     z;
    short   *zbuf;

    d = lp2[0] - lp1[0];
    if (d < -1 || d > 1)
        goto split;
    d = lp2[1] - lp1[1];
    if (d < -1 || d > 1)
        goto split;

    d = lp3[0] - lp2[0];
    if (d < -1 || d > 1)
        goto split2;
    d = lp3[1] - lp2[1];
    if (d < -1 || d > 1)
        goto split2;

    d = lp1[0] - lp3[0];
    if (d < -1 || d > 1)
        goto split3;
    d = lp1[1] - lp3[1];
    if (d < -1 || d > 1)
    {
split3:
        temp = lp1;
        lp1 = lp3;
        lp3 = lp2;
        lp2 = temp;

        goto split;
    }

    return;         // entire tri is filled

split2:
    temp = lp1;
    lp1 = lp2;
    lp2 = lp3;
    lp3 = temp;

split:
// split this edge
    new[0] = (lp1[0] + lp2[0]) >> 1;
    new[1] = (lp1[1] + lp2[1]) >> 1;
    new[2] = (lp1[2] + lp2[2]) >> 1;
    new[3] = (lp1[3] + lp2[3]) >> 1;
    new[5] = (lp1[5] + lp2[5]) >> 1;

// draw the point if splitting a leading edge
    if (lp2[1] > lp1[1])
        goto nodraw;
    if ((lp2[1] == lp1[1]) && (lp2[0] < lp1[0]))
        goto nodraw;


    z = new[5]>>16;
    zbuf = zspantable[new[1]] + new[0];
    if (z >= *zbuf)
    {
        int     pix;
        
        *zbuf = z;
        pix = d_pcolormap[skintable[new[3]>>16][new[2]>>16]];
        d_viewbuffer[d_scantable[new[1]] + new[0]] = pix;
    }

nodraw:
// recursively continue
    D_PolysetRecursiveTriangle (lp3, lp1, new);
    D_PolysetRecursiveTriangle (lp3, new, lp2);
}

static void D_DrawSubdiv()
/*
Definition at line 190 of file d_polyse.c.

References acolormap, ALIAS_ONSEAM, d_pcolormap, D_PolysetRecursiveTriangle(), finalvert_s::flags, affinetridesc_t::numtriangles, affinetridesc_t::pfinalverts, affinetridesc_t::ptriangles, r_affinetridesc, affinetridesc_t::seamfixupX16, finalvert_s::v, and mtriangle_s::vertindex.

Referenced by D_PolysetDraw().
*/
{
    mtriangle_t     *ptri;
    finalvert_t     *pfv, *index0, *index1, *index2;
    int             i;
    int             lnumtriangles;

    pfv = r_affinetridesc.pfinalverts;
    ptri = r_affinetridesc.ptriangles;
    lnumtriangles = r_affinetridesc.numtriangles;

#ifdef FTE_PEXT_TRANS
#ifdef GLQUAKE
    if (transbackfac)
    {
        if (r_pixbytes == 4)
            drawfnc = D_PolysetRecursiveTriangle32Trans;
        else if (r_pixbytes == 2)
            drawfnc = D_PolysetRecursiveTriangle16C;
        else //if (r_pixbytes == 1)
            drawfnc = D_PolysetRecursiveTriangleTrans;
    }
    else
#endif
#endif

    for (i=0 ; i<lnumtriangles ; i++)
    {
        index0 = pfv + ptri[i].vertindex[0];
        index1 = pfv + ptri[i].vertindex[1];
        index2 = pfv + ptri[i].vertindex[2];

        if (((index0->v[1]-index1->v[1]) *
             (index0->v[0]-index2->v[0]) -
             (index0->v[0]-index1->v[0]) * 
             (index0->v[1]-index2->v[1])) >= 0)
        {
            continue;
        }

        d_pcolormap = &((byte *)acolormap)[index0->v[4] & 0xFF00];

        if (ptri[i].facesfront)
        {
            D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);
        }
        else
        {
            int     s0, s1, s2;

            s0 = index0->v[2];
            s1 = index1->v[2];
            s2 = index2->v[2];

            if (index0->flags & ALIAS_ONSEAM)
                index0->v[2] += r_affinetridesc.seamfixupX16;
            if (index1->flags & ALIAS_ONSEAM)
                index1->v[2] += r_affinetridesc.seamfixupX16;
            if (index2->flags & ALIAS_ONSEAM)
                index2->v[2] += r_affinetridesc.seamfixupX16;

            D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);

            index0->v[2] = s0;
            index1->v[2] = s1;
            index2->v[2] = s2;
        }
    }
}

static void D_PolysetSetEdgeTable(void)
/*
Definition at line 956 of file d_polyse.c.

References r_p0, r_p1, and r_p2.

Referenced by D_DrawNonSubdiv().
*/
{
    int         edgetableindex;

    edgetableindex = 0; // assume the vertices are already in
                        //  top to bottom order

//
// determine which edges are right & left, and the order in which
// to rasterize them
//
    if (r_p0[1] >= r_p1[1])
    {
        if (r_p0[1] == r_p1[1])
        {
            if (r_p0[1] < r_p2[1])
                pedgetable = &edgetables[2];
            else
                pedgetable = &edgetables[5];

            return;
        }
        else
        {
            edgetableindex = 1;
        }
    }

    if (r_p0[1] == r_p2[1])
    {
        if (edgetableindex)
            pedgetable = &edgetables[8];
        else
            pedgetable = &edgetables[9];

        return;
    }
    else if (r_p1[1] == r_p2[1])
    {
        if (edgetableindex)
            pedgetable = &edgetables[10];
        else
            pedgetable = &edgetables[11];

        return;
    }

    if (r_p0[1] > r_p2[1])
        edgetableindex += 2;

    if (r_p1[1] > r_p2[1])
        edgetableindex += 4;

    pedgetable = &edgetables[edgetableindex];
}


static void D_PolysetCalcGradients(int skinwidth)
/*
Definition at line 553 of file d_polyse.c.

References a_sstepxfrac, a_ststepxwhole, a_tstepxfrac, d_xdenom, int, r_lstepx, r_lstepy, r_p0, r_p1, r_p2, r_sstepx, r_sstepy, r_tstepx, r_tstepy, r_zistepx, and r_zistepy.

Referenced by D_RasterizeAliasPolySmooth().

*/
{
    float   xstepdenominv, ystepdenominv, t0, t1;
    float   p01_minus_p21, p11_minus_p21, p00_minus_p20, p10_minus_p20;

    p00_minus_p20 = r_p0[0] - r_p2[0];
    p01_minus_p21 = r_p0[1] - r_p2[1];
    p10_minus_p20 = r_p1[0] - r_p2[0];
    p11_minus_p21 = r_p1[1] - r_p2[1];

    xstepdenominv = 1.0 / (float)d_xdenom;

    ystepdenominv = -xstepdenominv;

// ceil () for light so positive steps are exaggerated, negative steps
// diminished,  pushing us away from underflow toward overflow. Underflow is
// very visible, overflow is very unlikely, because of ambient lighting
    t0 = r_p0[4] - r_p2[4];
    t1 = r_p1[4] - r_p2[4];
    r_lstepx = (int)
            ceil((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
    r_lstepy = (int)
            ceil((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

    t0 = r_p0[2] - r_p2[2];
    t1 = r_p1[2] - r_p2[2];
    r_sstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
            xstepdenominv);
    r_sstepy = (int)((t1 * p00_minus_p20 - t0* p10_minus_p20) *
            ystepdenominv);

    t0 = r_p0[3] - r_p2[3];
    t1 = r_p1[3] - r_p2[3];
    r_tstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
            xstepdenominv);
    r_tstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
            ystepdenominv);

    t0 = r_p0[5] - r_p2[5];
    t1 = r_p1[5] - r_p2[5];
    r_zistepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
            xstepdenominv);
    r_zistepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
            ystepdenominv);

#if id386
    a_sstepxfrac = r_sstepx << 16;
    a_tstepxfrac = r_tstepx << 16;
#else
    a_sstepxfrac = r_sstepx & 0xFFFF;
    a_tstepxfrac = r_tstepx & 0xFFFF;
#endif

    a_ststepxwhole = skinwidth * (r_tstepx >> 16) + (r_sstepx >> 16);
}

void D_PolysetDrawSpans8(spanpackage_t *pspanpackage)
/*
Definition at line 634 of file d_polyse.c.

References a_sstepxfrac, a_ststepxwhole, a_tstepxfrac, acolormap, spanpackage_t::count, d_aspancount, d_countextrastep, erroradjustdown, erroradjustup, errorterm, spanpackage_t::light, spanpackage_t::pdest, spanpackage_t::ptex, spanpackage_t::pz, r_affinetridesc, r_lstepx, r_zistepx, spanpackage_t::sfrac, affinetridesc_t::skinwidth, spanpackage_t::tfrac, ubasestep, and spanpackage_t::zi.

Referenced by D_RasterizeAliasPolySmooth().
*/
{
    int     lcount;
    byte    *lpdest;
    byte    *lptex;
    int     lsfrac, ltfrac;
    int     llight;
    int     lzi;
    short   *lpz;

    do
    {
        lcount = d_aspancount - pspanpackage->count;

        errorterm += erroradjustup;
        if (errorterm >= 0)
        {
            d_aspancount += d_countextrastep;
            errorterm -= erroradjustdown;
        }
        else
        {
            d_aspancount += ubasestep;
        }

        if (lcount)
        {
            lpdest = pspanpackage->pdest;
            lptex = pspanpackage->ptex;
            lpz = pspanpackage->pz;
            lsfrac = pspanpackage->sfrac;
            ltfrac = pspanpackage->tfrac;
            llight = pspanpackage->light;
            lzi = pspanpackage->zi;

            do
            {
                if ((lzi >> 16) >= *lpz)
                {
                    *lpdest = ((byte *)acolormap)[*lptex + (llight & 0xFF00)];
// gel mapping                  *lpdest = gelmap[*lpdest];
                    *lpz = lzi >> 16;
                }
                lpdest++;
                lzi += r_zistepx;
                lpz++;
                llight += r_lstepx;
                lptex += a_ststepxwhole;
                lsfrac += a_sstepxfrac;
                lptex += lsfrac >> 16;
                lsfrac &= 0xFFFF;
                ltfrac += a_tstepxfrac;
                if (ltfrac & 0x10000)
                {
                    lptex += r_affinetridesc.skinwidth;
                    ltfrac &= 0xFFFF;
                }
            } while (--lcount);
        }

        pspanpackage++;
    } while (pspanpackage->count != -999999);
}

void D_PolysetScanLeftEdge(int height)
/*
Definition at line 443 of file d_polyse.c.

References spanpackage_t::count, d_aspancount, d_countextrastep, d_light, d_lightbasestep, d_lightextrastep, d_pdest, d_pdestbasestep, d_pdestextrastep, d_ptex, d_ptexbasestep, d_ptexextrastep, d_pz, d_pzbasestep, d_pzextrastep, d_sfrac, d_sfracbasestep, d_sfracextrastep, d_tfrac, d_tfracbasestep, d_tfracextrastep, d_zi, d_zibasestep, d_ziextrastep, erroradjustdown, erroradjustup, errorterm, spanpackage_t::light, spanpackage_t::pdest, spanpackage_t::ptex, spanpackage_t::pz, r_affinetridesc, spanpackage_t::sfrac, affinetridesc_t::skinwidth, spanpackage_t::tfrac, ubasestep, and spanpackage_t::zi.

Referenced by D_RasterizeAliasPolySmooth().
*/
{

    do
    {
        d_pedgespanpackage->pdest = d_pdest;
        d_pedgespanpackage->pz = d_pz;
        d_pedgespanpackage->count = d_aspancount;
        d_pedgespanpackage->ptex = d_ptex;

        d_pedgespanpackage->sfrac = d_sfrac;
        d_pedgespanpackage->tfrac = d_tfrac;

    // FIXME: need to clamp l, s, t, at both ends?
        d_pedgespanpackage->light = d_light;
        d_pedgespanpackage->zi = d_zi;

        d_pedgespanpackage++;

        errorterm += erroradjustup;
        if (errorterm >= 0)
        {
            d_pdest += d_pdestextrastep;
            d_pz += d_pzextrastep;
            d_aspancount += d_countextrastep;
            d_ptex += d_ptexextrastep;
            d_sfrac += d_sfracextrastep;
            d_ptex += d_sfrac >> 16;

            d_sfrac &= 0xFFFF;
            d_tfrac += d_tfracextrastep;
            if (d_tfrac & 0x10000)
            {
                d_ptex += r_affinetridesc.skinwidth;
                d_tfrac &= 0xFFFF;
            }
            d_light += d_lightextrastep;
            d_zi += d_ziextrastep;
            errorterm -= erroradjustdown;
        }
        else
        {
            d_pdest += d_pdestbasestep;
            d_pz += d_pzbasestep;
            d_aspancount += ubasestep;
            d_ptex += d_ptexbasestep;
            d_sfrac += d_sfracbasestep;
            d_ptex += d_sfrac >> 16;
            d_sfrac &= 0xFFFF;
            d_tfrac += d_tfracbasestep;
            if (d_tfrac & 0x10000)
            {
                d_ptex += r_affinetridesc.skinwidth;
                d_tfrac &= 0xFFFF;
            }
            d_light += d_lightbasestep;
            d_zi += d_zibasestep;
        }
    } while (--height);
}

#define PARANOID
void FloorDivMod(double numer, double denom, int *quotient, int *rem)
/*
Definition at line 297 of file mathlib.c.

References int, and Sys_Error().

Referenced by D_PolysetSetUpForLineScan().
*/
{
    int q, r;
    double x;

#ifndef PARANOID
    if (denom <= 0.0)
        Sys_Error ("FloorDivMod: bad denominator %d", denom);
#endif

    if (numer >= 0.0) {
        x = floor(numer / denom);
        q = (int) x;
        r = (int) floor(numer - (x * denom));
    } else {
        // perform operations with positive values, and fix mod to make floor-based
        x = floor(-numer / denom);
        q = -(int)x;
        r = (int)floor(-numer - (x * denom));
        if (r != 0) {
            q--;
            r = (int)denom - r;
        }
    }
    *quotient = q;
    *rem = r;
}

void D_PolysetSetUpForLineScan(fixed8_t startvertu, fixed8_t startvertv, fixed8_t endvertu, fixed8_t endvertv)
/*
Definition at line 512 of file d_polyse.c.

References erroradjustdown, erroradjustup, errorterm, FloorDivMod(), adivtab_t::quotient, adivtab_t::remainder, and ubasestep.

Referenced by D_RasterizeAliasPolySmooth().
*/
{
    double      dm, dn;
    int         tm, tn;
    adivtab_t   *ptemp;

// TODO: implement x86 version

    errorterm = -1;

    tm = endvertu - startvertu;
    tn = endvertv - startvertv;

    if (((tm <= 16) && (tm >= -15)) &&
        ((tn <= 16) && (tn >= -15)))
    {
        ptemp = &adivtab[((tm+15) << 5) + (tn+15)];
        ubasestep = ptemp->quotient;
        erroradjustup = ptemp->remainder;
        erroradjustdown = tn;
    }
    else
    {
        dm = (double)tm;
        dn = (double)tn;

        FloorDivMod (dm, dn, &ubasestep, &erroradjustup);

        erroradjustdown = dn;
    }
}

static void D_RasterizeAliasPolySmooth(void)
/*
Definition at line 742 of file d_polyse.c.

References a_spans, spanpackage_t::count, d_aspancount, d_countextrastep, d_light, d_lightbasestep, d_lightextrastep, d_pdest, d_pdestbasestep, d_pdestextrastep, D_PolysetCalcGradients(), D_PolysetDrawSpans8(), D_PolysetScanLeftEdge(), D_PolysetSetUpForLineScan(), d_ptex, d_ptexbasestep, d_ptexextrastep, d_pz, d_pzbasestep, d_pzbuffer, d_pzextrastep, d_sfrac, d_sfracbasestep, d_sfracextrastep, d_tfrac, d_tfracbasestep, d_tfracextrastep, d_viewbuffer, d_zi, d_zibasestep, d_ziextrastep, d_zwidth, edgetable::numleftedges, edgetable::numrightedges, edgetable::pleftedgevert0, edgetable::pleftedgevert1, edgetable::pleftedgevert2, edgetable::prightedgevert0, edgetable::prightedgevert1, edgetable::prightedgevert2, affinetridesc_t::pskin, r_affinetridesc, r_lstepx, r_lstepy, r_sstepx, r_sstepy, r_tstepx, r_tstepy, r_zistepx, r_zistepy, screenwidth, affinetridesc_t::skinwidth, ubasestep, and ystart.

Referenced by D_DrawNonSubdiv().
*/
{
    int             initialleftheight, initialrightheight;
    int             *plefttop, *prighttop, *pleftbottom, *prightbottom;
    int             working_lstepx, originalcount;

    plefttop = pedgetable->pleftedgevert0;
    prighttop = pedgetable->prightedgevert0;

    pleftbottom = pedgetable->pleftedgevert1;
    prightbottom = pedgetable->prightedgevert1;

    initialleftheight = pleftbottom[1] - plefttop[1];
    initialrightheight = prightbottom[1] - prighttop[1];

//
// set the s, t, and light gradients, which are consistent across the triangle
// because being a triangle, things are affine
//
    D_PolysetCalcGradients (r_affinetridesc.skinwidth);

//
// rasterize the polygon
//

//
// scan out the top (and possibly only) part of the left edge
//
    D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
                          pleftbottom[0], pleftbottom[1]);

    d_pedgespanpackage = a_spans;

    ystart = plefttop[1];
    d_aspancount = plefttop[0] - prighttop[0];

    d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
            (plefttop[3] >> 16) * r_affinetridesc.skinwidth;
#if id386
    d_sfrac = (plefttop[2] & 0xFFFF) << 16;
    d_tfrac = (plefttop[3] & 0xFFFF) << 16;
    d_pzbasestep = (d_zwidth + ubasestep) << 1;
    d_pzextrastep = d_pzbasestep + 2;
#else
    d_sfrac = plefttop[2] & 0xFFFF;
    d_tfrac = plefttop[3] & 0xFFFF;
    d_pzbasestep = d_zwidth + ubasestep;
    d_pzextrastep = d_pzbasestep + 1;
#endif
    d_light = plefttop[4];
    d_zi = plefttop[5];

    d_pdestbasestep = screenwidth + ubasestep;
    d_pdestextrastep = d_pdestbasestep + 1;
    d_pdest = (byte *)d_viewbuffer +
            ystart * screenwidth + plefttop[0];
    d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

// TODO: can reuse partial expressions here

// for negative steps in x along left edge, bias toward overflow rather than
// underflow (sort of turning the floor () we did in the gradient calcs into
// ceil (), but plus a little bit)
    if (ubasestep < 0)
        working_lstepx = r_lstepx - 1;
    else
        working_lstepx = r_lstepx;

    d_countextrastep = ubasestep + 1;
    d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
            ((r_tstepy + r_tstepx * ubasestep) >> 16) *
            r_affinetridesc.skinwidth;
#if id386
    d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) << 16;
    d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) << 16;
#else
    d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
    d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
#endif
    d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
    d_zibasestep = r_zistepy + r_zistepx * ubasestep;

    d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
            ((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
            r_affinetridesc.skinwidth;
#if id386
    d_sfracextrastep = (r_sstepy + r_sstepx*d_countextrastep) << 16;
    d_tfracextrastep = (r_tstepy + r_tstepx*d_countextrastep) << 16;
#else
    d_sfracextrastep = (r_sstepy + r_sstepx*d_countextrastep) & 0xFFFF;
    d_tfracextrastep = (r_tstepy + r_tstepx*d_countextrastep) & 0xFFFF;
#endif
    d_lightextrastep = d_lightbasestep + working_lstepx;
    d_ziextrastep = d_zibasestep + r_zistepx;

    D_PolysetScanLeftEdge (initialleftheight);

//
// scan out the bottom part of the left edge, if it exists
//
    if (pedgetable->numleftedges == 2)
    {
        int     height;

        plefttop = pleftbottom;
        pleftbottom = pedgetable->pleftedgevert2;

        D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
                              pleftbottom[0], pleftbottom[1]);

        height = pleftbottom[1] - plefttop[1];

// TODO: make this a function; modularize this function in general

        ystart = plefttop[1];
        d_aspancount = plefttop[0] - prighttop[0];
        d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
                (plefttop[3] >> 16) * r_affinetridesc.skinwidth;
        d_sfrac = 0;
        d_tfrac = 0;
        d_light = plefttop[4];
        d_zi = plefttop[5];

        d_pdestbasestep = screenwidth + ubasestep;
        d_pdestextrastep = d_pdestbasestep + 1;
        d_pdest = (byte *)d_viewbuffer + ystart * screenwidth + plefttop[0];
#if id386
        d_pzbasestep = (d_zwidth + ubasestep) << 1;
        d_pzextrastep = d_pzbasestep + 2;
#else
        d_pzbasestep = d_zwidth + ubasestep;
        d_pzextrastep = d_pzbasestep + 1;
#endif
        d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

        if (ubasestep < 0)
            working_lstepx = r_lstepx - 1;
        else
            working_lstepx = r_lstepx;

        d_countextrastep = ubasestep + 1;
        d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
                ((r_tstepy + r_tstepx * ubasestep) >> 16) *
                r_affinetridesc.skinwidth;
#if id386
        d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) << 16;
        d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) << 16;
#else
        d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
        d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
#endif
        d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
        d_zibasestep = r_zistepy + r_zistepx * ubasestep;

        d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
                ((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
                r_affinetridesc.skinwidth;
#if id386
        d_sfracextrastep = ((r_sstepy+r_sstepx*d_countextrastep) & 0xFFFF)<<16;
        d_tfracextrastep = ((r_tstepy+r_tstepx*d_countextrastep) & 0xFFFF)<<16;
#else
        d_sfracextrastep = (r_sstepy+r_sstepx*d_countextrastep) & 0xFFFF;
        d_tfracextrastep = (r_tstepy+r_tstepx*d_countextrastep) & 0xFFFF;
#endif
        d_lightextrastep = d_lightbasestep + working_lstepx;
        d_ziextrastep = d_zibasestep + r_zistepx;

        D_PolysetScanLeftEdge (height);
    }

// scan out the top (and possibly only) part of the right edge, updating the
// count field
    d_pedgespanpackage = a_spans;

    D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
                          prightbottom[0], prightbottom[1]);
    d_aspancount = 0;
    d_countextrastep = ubasestep + 1;
    originalcount = a_spans[initialrightheight].count;
    a_spans[initialrightheight].count = -999999; // mark end of the spanpackages
    D_PolysetDrawSpans8 (a_spans);

// scan out the bottom part of the right edge, if it exists
    if (pedgetable->numrightedges == 2)
    {
        int             height;
        spanpackage_t   *pstart;

        pstart = a_spans + initialrightheight;
        pstart->count = originalcount;

        d_aspancount = prightbottom[0] - prighttop[0];

        prighttop = prightbottom;
        prightbottom = pedgetable->prightedgevert2;

        height = prightbottom[1] - prighttop[1];

        D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
                              prightbottom[0], prightbottom[1]);

        d_countextrastep = ubasestep + 1;
        a_spans[initialrightheight + height].count = -999999;
                                            // mark end of the spanpackages
        D_PolysetDrawSpans8 (pstart);
    }
}

static void D_DrawNonSubdiv(void)
/*
Definition at line 266 of file d_polyse.c.

References ALIAS_ONSEAM, D_PolysetSetEdgeTable(), D_RasterizeAliasPolySmooth(), d_xdenom, mtriangle_s::facesfront, finalvert_s::flags, affinetridesc_t::numtriangles, affinetridesc_t::pfinalverts, affinetridesc_t::ptriangles, r_affinetridesc, r_p0, r_p1, r_p2, affinetridesc_t::seamfixupX16, finalvert_s::v, and mtriangle_s::vertindex.

Referenced by D_PolysetDraw().
*/
{
    mtriangle_t     *ptri;
    finalvert_t     *pfv, *index0, *index1, *index2;
    int             i;
    int             lnumtriangles;

    pfv = r_affinetridesc.pfinalverts;
    ptri = r_affinetridesc.ptriangles;
    lnumtriangles = r_affinetridesc.numtriangles;

    for (i=0 ; i<lnumtriangles ; i++, ptri++)
    {
        index0 = pfv + ptri->vertindex[0];
        index1 = pfv + ptri->vertindex[1];
        index2 = pfv + ptri->vertindex[2];

        d_xdenom = (index0->v[1]-index1->v[1]) *
                (index0->v[0]-index2->v[0]) -
                (index0->v[0]-index1->v[0])*(index0->v[1]-index2->v[1]);

        if (d_xdenom >= 0)
        {
            continue;
        }

        r_p0[0] = index0->v[0];     // u
        r_p0[1] = index0->v[1];     // v
        r_p0[2] = index0->v[2];     // s
        r_p0[3] = index0->v[3];     // t
        r_p0[4] = index0->v[4];     // light
        r_p0[5] = index0->v[5];     // iz

        r_p1[0] = index1->v[0];
        r_p1[1] = index1->v[1];
        r_p1[2] = index1->v[2];
        r_p1[3] = index1->v[3];
        r_p1[4] = index1->v[4];
        r_p1[5] = index1->v[5];

        r_p2[0] = index2->v[0];
        r_p2[1] = index2->v[1];
        r_p2[2] = index2->v[2];
        r_p2[3] = index2->v[3];
        r_p2[4] = index2->v[4];
        r_p2[5] = index2->v[5];

        if (!ptri->facesfront)
        {
            if (index0->flags & ALIAS_ONSEAM)
                r_p0[2] += r_affinetridesc.seamfixupX16;
            if (index1->flags & ALIAS_ONSEAM)
                r_p1[2] += r_affinetridesc.seamfixupX16;
            if (index2->flags & ALIAS_ONSEAM)
                r_p2[2] += r_affinetridesc.seamfixupX16;
        }

        D_PolysetSetEdgeTable ();
        D_RasterizeAliasPolySmooth ();
    }
}

void D_PolysetDraw(void)
/*
Definition at line 132 of file d_polyse.c.

References CACHE_SIZE, D_DrawNonSubdiv(), D_DrawSubdiv(), DPS_MAXSPANS, affinetridesc_t::drawtype, and r_affinetridesc.

Referenced by R_AliasClipTriangle(), R_AliasPreparePoints(), and R_AliasPrepareUnclippedPoints().
*/
{
    spanpackage_t   spans[DPS_MAXSPANS + 1 +
            ((CACHE_SIZE - 1) / sizeof(spanpackage_t)) + 1];
                        // one extra because of cache line pretouching

    a_spans = (spanpackage_t *)
            (((long)&spans[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

    if (r_affinetridesc.drawtype)
    {
        D_DrawSubdiv ();
    }
    else
    {
        D_DrawNonSubdiv ();
    }
}

int main()
{
	mtriangle_t tris[10];
	finalvert_t verts[30];
	int i;
	for (i = 0; i < sizeof(tris)/sizeof(tris[0]); i++) {
		tris[i].facesfront = 1;
		tris[i].vertindex[0] = i + 0;
		tris[i].vertindex[1] = i + 1;
		tris[i].vertindex[2] = i + 2;
	}
	for (i = 0; i < sizeof(verts)/sizeof(verts[0]); i++) {
		verts[i].v[0] = i;
		verts[i].v[1] = i;
		verts[i].v[2] = 0;
		verts[i].v[3] = 0;
		verts[i].v[4] = 0;
		verts[i].v[5] = 20;
		verts[i].flags = 0;
		verts[i].reserved = 0;
	}
	r_affinetridesc.pfinalverts = verts;
	r_affinetridesc.ptriangles = tris;
	r_affinetridesc.numtriangles = 10;
	D_PolysetDraw();
	return 0;
}

