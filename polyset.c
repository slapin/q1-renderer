#include <stddef.h>
#include <math.h>
#include "triangle.h"
#include "polyset.h"
typedef struct {
	int isflattop;
	int numleftedges;
	int *pleftedgevert0;
	int *pleftedgevert1;
	int *pleftedgevert2;
	int numrightedges;
	int *prightedgevert0;
	int *prightedgevert1;
	int *prightedgevert2;
} edgetable;
typedef struct {
	void *pdest;
	short *pz;
	int count;
	unsigned char *ptex;
	int sfrac, tfrac, light, zi;
} spanpackage_t;
struct r_state {
	int ubasestep;
	int d_aspancount;
	int erroradjustup;
	int erroradjustdown;
	int errorterm;
	int d_countextrastep;
	int a_ststepxwhole;
	int a_sstepxfrac;
	int a_tstepxfrac;
	int d_light;
	int d_lightbasestep;
	int d_lightextrastep;
	int d_pdestbasestep;
	int d_pdestextrastep;
	short *d_pz;
	int d_pzbasestep;
	int d_pzextrastep;
	int d_sfrac;
	int d_sfracbasestep;
	int d_sfracextrastep;
	int d_tfrac;
	int d_tfracbasestep;
	int d_tfracextrastep;
	int d_zi;
	int d_zibasestep;
	int d_ziextrastep;
	int r_lstepx;
	int r_lstepy;
	int r_sstepx, r_sstepy;
	int r_tstepx, r_tstepy;
	int r_zistepx, r_zistepy;
	spanpackage_t *d_pedgespanpackage;
	spanpackage_t *a_spans;
	edgetable *pedgetable;
};

static int r_p0[6], r_p1[6], r_p2[6];

#define PARANOID
static void FloorDivMod(double numer, double denom, int *quotient, int *rem)
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
		Sys_Error("FloorDivMod: bad denominator %d", denom);
#endif

	if (numer >= 0.0) {
		x = floor(numer / denom);
		q = (int)x;
		r = (int)floor(numer - (x * denom));
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

typedef int fixed8_t;
typedef struct {
	int quotient;
	int remainder;
} adivtab_t;
static const adivtab_t adivtab[32 * 32] = {
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
	{0, -10}, {0, -10}, {0, -10}, {0, -10}, {0, -10}, {1, 0}, {1, -1}, {1,
									    -2},
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
	{-1, 4}, {-1, 5}, {-1, 6}, {-1, 7}, {-1, 8}, {-1, 9}, {-1, 10}, {-1,
									 11},
// numerator = -4
	{0, -4}, {0, -4}, {0, -4}, {0, -4}, {0, -4}, {0, -4}, {0, -4}, {0, -4},
	{0, -4}, {0, -4}, {0, -4}, {1, 0}, {1, -1}, {2, 0}, {4, 0}, {0, 0},
	{-4, 0}, {-2, 0}, {-2, 2}, {-1, 0}, {-1, 1}, {-1, 2}, {-1, 3}, {-1, 4},
	{-1, 5}, {-1, 6}, {-1, 7}, {-1, 8}, {-1, 9}, {-1, 10}, {-1, 11}, {-1,
									  12},
// numerator = -3
	{0, -3}, {0, -3}, {0, -3}, {0, -3}, {0, -3}, {0, -3}, {0, -3}, {0, -3},
	{0, -3}, {0, -3}, {0, -3}, {0, -3}, {1, 0}, {1, -1}, {3, 0}, {0, 0},
	{-3, 0}, {-2, 1}, {-1, 0}, {-1, 1}, {-1, 2}, {-1, 3}, {-1, 4}, {-1, 5},
	{-1, 6}, {-1, 7}, {-1, 8}, {-1, 9}, {-1, 10}, {-1, 11}, {-1, 12}, {-1,
									   13},
// numerator = -2
	{0, -2}, {0, -2}, {0, -2}, {0, -2}, {0, -2}, {0, -2}, {0, -2}, {0, -2},
	{0, -2}, {0, -2}, {0, -2}, {0, -2}, {0, -2}, {1, 0}, {2, 0}, {0, 0},
	{-2, 0}, {-1, 0}, {-1, 1}, {-1, 2}, {-1, 3}, {-1, 4}, {-1, 5}, {-1, 6},
	{-1, 7}, {-1, 8}, {-1, 9}, {-1, 10}, {-1, 11}, {-1, 12}, {-1, 13}, {-1,
									    14},
// numerator = -1
	{0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1},
	{0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {1, 0}, {0, 0},
	{-1, 0}, {-1, 1}, {-1, 2}, {-1, 3}, {-1, 4}, {-1, 5}, {-1, 6}, {-1, 7},
	{-1, 8}, {-1, 9}, {-1, 10}, {-1, 11}, {-1, 12}, {-1, 13}, {-1, 14}, {-1,
									     15},
// numerator = 0
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
// numerator = 1
	{-1, -14}, {-1, -13}, {-1, -12}, {-1, -11}, {-1, -10}, {-1, -9}, {-1,
									  -8},
	{-1, -7}, {-1, -6}, {-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1},
	{-1, 0}, {0, 0}, {1, 0}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
// numerator = 2
	{-1, -13}, {-1, -12}, {-1, -11}, {-1, -10}, {-1, -9}, {-1, -8}, {-1,
									 -7},
	{-1, -6}, {-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1}, {-1, 0},
	{-2, 0}, {0, 0}, {2, 0}, {1, 0}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
	{0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
// numerator = 3
	{-1, -12}, {-1, -11}, {-1, -10}, {-1, -9}, {-1, -8}, {-1, -7}, {-1, -6},
	{-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -1},
	{-3, 0},
	{0, 0}, {3, 0}, {1, 1}, {1, 0}, {0, 3}, {0, 3}, {0, 3}, {0, 3}, {0, 3},
	{0, 3}, {0, 3}, {0, 3}, {0, 3}, {0, 3}, {0, 3}, {0, 3}, {0, 3},
// numerator = 4
	{-1, -11}, {-1, -10}, {-1, -9}, {-1, -8}, {-1, -7}, {-1, -6}, {-1, -5},
	{-1, -4},
	{-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -2}, {-2, 0}, {-4, 0}, {0,
									    0},
	{4, 0},
	{2, 0}, {1, 1}, {1, 0}, {0, 4}, {0, 4}, {0, 4}, {0, 4}, {0, 4}, {0, 4},
	{0, 4},
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
	{-1, -8}, {-1, -7}, {-1, -6}, {-1, -5}, {-1, -4}, {-1, -3}, {-1, -2},
	{-1, -1},
	{-1, 0}, {-2, -5}, {-2, -3}, {-2, -1}, {-3, -2}, {-4, -1}, {-7, 0}, {0,
									     0},
	{7, 0}, {3, 1}, {2, 1}, {1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 7}, {0, 7},
	{0, 7}, {0, 7}, {0, 7}, {0, 7}, {0, 7}, {0, 7}, {0, 7},
// numerator = 8
	{-1, -7}, {-1, -6}, {-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1},
	{-1, 0},
	{-2, -6}, {-2, -4}, {-2, -2}, {-2, 0}, {-3, -1}, {-4, 0}, {-8, 0}, {0,
									    0},
	{8, 0},
	{4, 0}, {2, 2}, {2, 0}, {1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 8}, {0, 8},
	{0, 8},
	{0, 8}, {0, 8}, {0, 8}, {0, 8}, {0, 8},
// numerator = 9
	{-1, -6}, {-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1}, {-1, 0},
	{-2, -7},
	{-2, -5}, {-2, -3}, {-2, -1}, {-3, -3}, {-3, 0}, {-5, -1}, {-9, 0}, {0,
									     0},
	{9, 0},
	{4, 1}, {3, 0}, {2, 1}, {1, 4}, {1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 9},
	{0, 9},
	{0, 9}, {0, 9}, {0, 9}, {0, 9}, {0, 9},
// numerator = 10
	{-1, -5}, {-1, -4}, {-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -8},
	{-2, -6}, {-2, -4}, {-2, -2}, {-2, 0}, {-3, -2}, {-4, -2}, {-5, 0},
	{-10, 0}, {0, 0}, {10, 0}, {5, 0}, {3, 1}, {2, 2}, {2, 0}, {1, 4}, {1,
									    3},
	{1, 2}, {1, 1}, {1, 0}, {0, 10}, {0, 10}, {0, 10}, {0, 10}, {0, 10}, {0,
									      10},
// numerator = 11
	{-1, -4}, {-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -9}, {-2, -7},
	{-2, -5}, {-2, -3}, {-2, -1}, {-3, -4}, {-3, -1}, {-4, -1}, {-6, -1},
	{-11, 0}, {0, 0}, {11, 0}, {5, 1}, {3, 2}, {2, 3}, {2, 1}, {1, 5},
	{1, 4}, {1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 11}, {0, 11}, {0, 11},
	{0, 11}, {0, 11},
// numerator = 12
	{-1, -3}, {-1, -2}, {-1, -1}, {-1, 0}, {-2, -10}, {-2, -8}, {-2, -6},
	{-2, -4}, {-2, -2}, {-2, 0}, {-3, -3}, {-3, 0}, {-4, 0}, {-6, 0}, {-12,
									   0},
	{0, 0}, {12, 0}, {6, 0}, {4, 0}, {3, 0}, {2, 2}, {2, 0}, {1, 5}, {1, 4},
	{1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 12}, {0, 12}, {0, 12}, {0, 12},
// numerator = 13
	{-1, -2}, {-1, -1}, {-1, 0}, {-2, -11}, {-2, -9}, {-2, -7}, {-2, -5},
	{-2, -3},
	{-2, -1}, {-3, -5}, {-3, -2}, {-4, -3}, {-5, -2}, {-7, -1}, {-13, 0},
	{0, 0},
	{13, 0}, {6, 1}, {4, 1}, {3, 1}, {2, 3}, {2, 1}, {1, 6}, {1, 5}, {1, 4},
	{1, 3},
	{1, 2}, {1, 1}, {1, 0}, {0, 13}, {0, 13}, {0, 13},
// numerator = 14
	{-1, -1}, {-1, 0}, {-2, -12}, {-2, -10}, {-2, -8}, {-2, -6}, {-2, -4},
	{-2, -2},
	{-2, 0}, {-3, -4}, {-3, -1}, {-4, -2}, {-5, -1}, {-7, 0}, {-14, 0}, {0,
									     0},
	{14, 0},
	{7, 0}, {4, 2}, {3, 2}, {2, 4}, {2, 2}, {2, 0}, {1, 6}, {1, 5}, {1, 4},
	{1, 3},
	{1, 2}, {1, 1}, {1, 0}, {0, 14}, {0, 14},
// numerator = 15
	{-1, 0}, {-2, -13}, {-2, -11}, {-2, -9}, {-2, -7}, {-2, -5}, {-2, -3},
	{-2, -1},
	{-3, -6}, {-3, -3}, {-3, 0}, {-4, -1}, {-5, 0}, {-8, -1}, {-15, 0}, {0,
									     0},
	{15, 0}, {7, 1}, {5, 0}, {3, 3}, {3, 0}, {2, 3}, {2, 1}, {1, 7}, {1, 6},
	{1, 5},
	{1, 4}, {1, 3}, {1, 2}, {1, 1}, {1, 0}, {0, 15},
// numerator = 16
	{-2, -14}, {-2, -12}, {-2, -10}, {-2, -8}, {-2, -6}, {-2, -4}, {-2, -2},
	{-2, 0},
	{-3, -5}, {-3, -2}, {-4, -4}, {-4, 0}, {-6, -2}, {-8, 0}, {-16, 0}, {0,
									     0},
	{16, 0}, {8, 0}, {5, 1}, {4, 0}, {3, 1}, {2, 4}, {2, 2}, {2, 0}, {1, 7},
	{1, 6}, {1, 5}, {1, 4}, {1, 3}, {1, 2}, {1, 1}, {1, 0},
};


static edgetable edgetables[12] = {
	{0, 1, r_p0, r_p2, NULL, 2, r_p0, r_p1, r_p2},
	{0, 2, r_p1, r_p0, r_p2, 1, r_p1, r_p2, NULL},
	{1, 1, r_p0, r_p2, NULL, 1, r_p1, r_p2, NULL},
	{0, 1, r_p1, r_p0, NULL, 2, r_p1, r_p2, r_p0},
	{0, 2, r_p0, r_p2, r_p1, 1, r_p0, r_p1, NULL},
	{0, 1, r_p2, r_p1, NULL, 1, r_p2, r_p0, NULL},
	{0, 1, r_p2, r_p1, NULL, 2, r_p2, r_p0, r_p1},
	{0, 2, r_p2, r_p1, r_p0, 1, r_p2, r_p0, NULL},
	{0, 1, r_p1, r_p0, NULL, 1, r_p1, r_p2, NULL},
	{1, 1, r_p2, r_p1, NULL, 1, r_p0, r_p1, NULL},
	{1, 1, r_p1, r_p0, NULL, 1, r_p2, r_p0, NULL},
	{0, 1, r_p0, r_p2, NULL, 1, r_p0, r_p1, NULL},
};

static void D_PolysetDrawSpans8(spanpackage_t * pspanpackage,
				unsigned char *d_viewbuffer, struct r_state *r,
				unsigned char *acolormap, int skinwidth)
/*
Definition at line 634 of file d_polyse.c.

References a_sstepxfrac, a_ststepxwhole, a_tstepxfrac, acolormap, spanpackage_t::count, d_aspancount, d_countextrastep, erroradjustdown, erroradjustup, errorterm, spanpackage_t::light, spanpackage_t::pdest, spanpackage_t::ptex, spanpackage_t::pz, r_affinetridesc, r_lstepx, r_zistepx, spanpackage_t::sfrac, affinetridesc_t::skinwidth, spanpackage_t::tfrac, ubasestep, and spanpackage_t::zi.

Referenced by D_RasterizeAliasPolySmooth().
*/
{
	int lcount;
	unsigned char *lpdest;
	unsigned char *lptex;
	int lsfrac, ltfrac;
	int llight;
	int lzi;
	short *lpz;

	do {
		lcount = r->d_aspancount - pspanpackage->count;

		r->errorterm += r->erroradjustup;
		if (r->errorterm >= 0) {
			r->d_aspancount += r->d_countextrastep;
			r->errorterm -= r->erroradjustdown;
		} else {
			r->d_aspancount += r->ubasestep;
		}

		if (lcount) {
			lpdest = pspanpackage->pdest;
			lptex = pspanpackage->ptex;
			lpz = pspanpackage->pz;
			lsfrac = pspanpackage->sfrac;
			ltfrac = pspanpackage->tfrac;
			llight = pspanpackage->light;
			lzi = pspanpackage->zi;

			do {
				if ((lzi >> 16) >= *lpz) {
					*lpdest =
					    acolormap[*lptex +
						      (llight & 0xFF00)];
// gel mapping                  *lpdest = gelmap[*lpdest];
					*lpz = lzi >> 16;
#if 0
					printf("plot %d %d = %d\n", (lpdest -
								     d_viewbuffer)
					       % WIDTH,
					       (lpdest - d_viewbuffer) / WIDTH,
					       acolormap[*lptex +
							 (llight & 0xFF00)]);
#endif
				}
				lpdest++;
				lzi += r->r_zistepx;
				lpz++;
				llight += r->r_lstepx;
				lptex += r->a_ststepxwhole;
				lsfrac += r->a_sstepxfrac;
				lptex += lsfrac >> 16;
				lsfrac &= 0xFFFF;
				ltfrac += r->a_tstepxfrac;
				if (ltfrac & 0x10000) {
					lptex += skinwidth;
					ltfrac &= 0xFFFF;
				}
			} while (--lcount);
		}

		pspanpackage++;
	} while (pspanpackage->count != -999999);
}

static void D_PolysetCalcGradients(int skinwidth, struct r_state *r,
				   int d_xdenom)
/*
Definition at line 553 of file d_polyse.c.

References a_sstepxfrac, a_ststepxwhole, a_tstepxfrac, d_xdenom, int, r_lstepx, r_lstepy, r_p0, r_p1, r_p2, r_sstepx, r_sstepy, r_tstepx, r_tstepy, r_zistepx, and r_zistepy.

Referenced by D_RasterizeAliasPolySmooth().

*/
{
	float xstepdenominv, ystepdenominv, t0, t1;
	float p01_minus_p21, p11_minus_p21, p00_minus_p20, p10_minus_p20;

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
	r->r_lstepx = (int)
	    ceil((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
	r->r_lstepy = (int)
	    ceil((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

	t0 = r_p0[2] - r_p2[2];
	t1 = r_p1[2] - r_p2[2];
	r->r_sstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
			    xstepdenominv);
	r->r_sstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
			    ystepdenominv);

	t0 = r_p0[3] - r_p2[3];
	t1 = r_p1[3] - r_p2[3];
	r->r_tstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
			    xstepdenominv);
	r->r_tstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
			    ystepdenominv);

	t0 = r_p0[5] - r_p2[5];
	t1 = r_p1[5] - r_p2[5];
	r->r_zistepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
			     xstepdenominv);
	r->r_zistepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
			     ystepdenominv);

#if id386
	r->a_sstepxfrac = r->r_sstepx << 16;
	r->a_tstepxfrac = r->r_tstepx << 16;
#else
	r->a_sstepxfrac = r->r_sstepx & 0xFFFF;
	r->a_tstepxfrac = r->r_tstepx & 0xFFFF;
#endif

	r->a_ststepxwhole =
	    skinwidth * (r->r_tstepx >> 16) + (r->r_sstepx >> 16);
}


static void D_PolysetSetEdgeTable(struct r_state *r)
/*
Definition at line 956 of file d_polyse.c.

References r_p0, r_p1, and r_p2.

Referenced by D_DrawNonSubdiv().
*/
{
	int edgetableindex;

	edgetableindex = 0;	// assume the vertices are already in
	//  top to bottom order

//
// determine which edges are right & left, and the order in which
// to rasterize them
//
	if (r_p0[1] >= r_p1[1]) {
		if (r_p0[1] == r_p1[1]) {
			if (r_p0[1] < r_p2[1])
				r->pedgetable = &edgetables[2];
			else
				r->pedgetable = &edgetables[5];

			return;
		} else {
			edgetableindex = 1;
		}
	}

	if (r_p0[1] == r_p2[1]) {
		if (edgetableindex)
			r->pedgetable = &edgetables[8];
		else
			r->pedgetable = &edgetables[9];

		return;
	} else if (r_p1[1] == r_p2[1]) {
		if (edgetableindex)
			r->pedgetable = &edgetables[10];
		else
			r->pedgetable = &edgetables[11];

		return;
	}

	if (r_p0[1] > r_p2[1])
		edgetableindex += 2;

	if (r_p1[1] > r_p2[1])
		edgetableindex += 4;

	r->pedgetable = &edgetables[edgetableindex];
}

static void D_PolysetScanLeftEdge(int height, unsigned char *d_pdest, unsigned char *d_ptex,
				  int d_ptexbasestep, int d_ptexextrastep,
				  struct r_state *r, int skinwidth)
/*
Definition at line 443 of file d_polyse.c.

References spanpackage_t::count, d_aspancount, d_countextrastep, d_light, d_lightbasestep, d_lightextrastep, d_pdest, d_pdestbasestep, d_pdestextrastep, d_ptex, d_ptexbasestep, d_ptexextrastep, d_pz, d_pzbasestep, d_pzextrastep, d_sfrac, d_sfracbasestep, d_sfracextrastep, d_tfrac, d_tfracbasestep, d_tfracextrastep, d_zi, d_zibasestep, d_ziextrastep, erroradjustdown, erroradjustup, errorterm, spanpackage_t::light, spanpackage_t::pdest, spanpackage_t::ptex, spanpackage_t::pz, r_affinetridesc, spanpackage_t::sfrac, affinetridesc_t::skinwidth, spanpackage_t::tfrac, ubasestep, and spanpackage_t::zi.

Referenced by D_RasterizeAliasPolySmooth().
*/
{
#if 0
	printf("plot steps base %d extra %d\n", d_pdestbasestep,
	       d_pdestextrastep);
#endif

	do {
		r->d_pedgespanpackage->pdest = d_pdest;
		r->d_pedgespanpackage->pz = r->d_pz;
		r->d_pedgespanpackage->count = r->d_aspancount;
		r->d_pedgespanpackage->ptex = d_ptex;

		r->d_pedgespanpackage->sfrac = r->d_sfrac;
		r->d_pedgespanpackage->tfrac = r->d_tfrac;

		// FIXME: need to clamp l, s, t, at both ends?
		r->d_pedgespanpackage->light = r->d_light;
		r->d_pedgespanpackage->zi = r->d_zi;

		r->d_pedgespanpackage++;

		r->errorterm += r->erroradjustup;
		if (r->errorterm >= 0) {
			d_pdest += r->d_pdestextrastep;
			r->d_pz += r->d_pzextrastep;
			r->d_aspancount += r->d_countextrastep;
			d_ptex += d_ptexextrastep;
			r->d_sfrac += r->d_sfracextrastep;
			d_ptex += r->d_sfrac >> 16;

			r->d_sfrac &= 0xFFFF;
			r->d_tfrac += r->d_tfracextrastep;
			if (r->d_tfrac & 0x10000) {
				d_ptex += skinwidth;
				r->d_tfrac &= 0xFFFF;
			}
			r->d_light += r->d_lightextrastep;
			r->d_zi += r->d_ziextrastep;
			r->errorterm -= r->erroradjustdown;
		} else {
			d_pdest += r->d_pdestbasestep;
			r->d_pz += r->d_pzbasestep;
			r->d_aspancount += r->ubasestep;
			d_ptex += d_ptexbasestep;
			r->d_sfrac += r->d_sfracbasestep;
			d_ptex += r->d_sfrac >> 16;
			r->d_sfrac &= 0xFFFF;
			r->d_tfrac += r->d_tfracbasestep;
			if (r->d_tfrac & 0x10000) {
				d_ptex += skinwidth;
				r->d_tfrac &= 0xFFFF;
			}
			r->d_light += r->d_lightbasestep;
			r->d_zi += r->d_zibasestep;
		}
	} while (--height);
}

static void D_PolysetSetUpForLineScan(fixed8_t startvertu, fixed8_t startvertv,
				      fixed8_t endvertu, fixed8_t endvertv,
				      struct r_state *r)
/*
Definition at line 512 of file d_polyse.c.

References erroradjustdown, erroradjustup, errorterm, FloorDivMod(), adivtab_t::quotient, adivtab_t::remainder, and ubasestep.

Referenced by D_RasterizeAliasPolySmooth().
*/
{
	double dm, dn;
	int tm, tn;
	const adivtab_t *ptemp;

// TODO: implement x86 version

	r->errorterm = -1;

	tm = endvertu - startvertu;
	tn = endvertv - startvertv;

	if (((tm <= 16) && (tm >= -15)) && ((tn <= 16) && (tn >= -15))) {
		ptemp = &adivtab[((tm + 15) << 5) + (tn + 15)];
		r->ubasestep = ptemp->quotient;
		r->erroradjustup = ptemp->remainder;
		r->erroradjustdown = tn;
	} else {
		dm = (double)tm;
		dn = (double)tn;

		FloorDivMod(dm, dn, &r->ubasestep, &r->erroradjustup);

		r->erroradjustdown = dn;
	}
}

static void D_RasterizeAliasPolySmooth(unsigned char * d_viewbuffer,
				       short *d_pzbuffer, unsigned char * acolormap,
				       struct r_state *r, unsigned int d_zwidth,
				       int screenwidth, int d_xdenom, void *pskin,
				       int skinwidth)
/*
Definition at line 742 of file d_polyse.c.

References a_spans, spanpackage_t::count, d_aspancount, d_countextrastep, d_light, d_lightbasestep, d_lightextrastep, d_pdest, d_pdestbasestep, d_pdestextrastep, D_PolysetCalcGradients(), D_PolysetDrawSpans8(), D_PolysetScanLeftEdge(), D_PolysetSetUpForLineScan(), d_ptex, d_ptexbasestep, d_ptexextrastep, d_pz, d_pzbasestep, d_pzbuffer, d_pzextrastep, d_sfrac, d_sfracbasestep, d_sfracextrastep, d_tfrac, d_tfracbasestep, d_tfracextrastep, d_viewbuffer, d_zi, d_zibasestep, d_ziextrastep, d_zwidth, edgetable::numleftedges, edgetable::numrightedges, edgetable::pleftedgevert0, edgetable::pleftedgevert1, edgetable::pleftedgevert2, edgetable::prightedgevert0, edgetable::prightedgevert1, edgetable::prightedgevert2, affinetridesc_t::pskin, r_affinetridesc, r_lstepx, r_lstepy, r_sstepx, r_sstepy, r_tstepx, r_tstepy, r_zistepx, r_zistepy, screenwidth, affinetridesc_t::skinwidth, ubasestep, and ystart.

Referenced by D_DrawNonSubdiv().
*/
{
	int ystart;
	unsigned char *d_pdest;
	unsigned char *d_ptex;
	int initialleftheight, initialrightheight;
	int *plefttop, *prighttop, *pleftbottom, *prightbottom;
	int working_lstepx, originalcount;
	int d_ptexbasestep;
	int d_ptexextrastep;

	plefttop = r->pedgetable->pleftedgevert0;
	prighttop = r->pedgetable->prightedgevert0;

	pleftbottom = r->pedgetable->pleftedgevert1;
	prightbottom = r->pedgetable->prightedgevert1;

	initialleftheight = pleftbottom[1] - plefttop[1];
	initialrightheight = prightbottom[1] - prighttop[1];

//
// set the s, t, and light gradients, which are consistent across the triangle
// because being a triangle, things are affine
//
	D_PolysetCalcGradients(skinwidth, r, d_xdenom);

#if 0
	printf("%s:%d: plot left height = %d right height = %d\n", __func__,
	       __LINE__, initialleftheight, initialrightheight);
#endif
//
// rasterize the polygon
//

//
// scan out the top (and possibly only) part of the left edge
//
	D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
				  pleftbottom[0], pleftbottom[1], r);

	r->d_pedgespanpackage = r->a_spans;

	ystart = plefttop[1];
	r->d_aspancount = plefttop[0] - prighttop[0];

	d_ptex = (unsigned char *) pskin + (plefttop[2] >> 16) +
	    (plefttop[3] >> 16) * skinwidth;
#if id386
	r->d_sfrac = (plefttop[2] & 0xFFFF) << 16;
	r->d_tfrac = (plefttop[3] & 0xFFFF) << 16;
	r->d_pzbasestep = (d_zwidth + r->ubasestep) << 1;
	r->d_pzextrastep = r->d_pzbasestep + 2;
#else
	r->d_sfrac = plefttop[2] & 0xFFFF;
	r->d_tfrac = plefttop[3] & 0xFFFF;
	r->d_pzbasestep = d_zwidth + r->ubasestep;
	r->d_pzextrastep = r->d_pzbasestep + 1;
#endif
	r->d_light = plefttop[4];
	r->d_zi = plefttop[5];

	r->d_pdestbasestep = screenwidth + r->ubasestep;
	r->d_pdestextrastep = r->d_pdestbasestep + 1;
	d_pdest = (unsigned char *) d_viewbuffer + ystart * screenwidth + plefttop[0];
	r->d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

// TODO: can reuse partial expressions here

// for negative steps in x along left edge, bias toward overflow rather than
// underflow (sort of turning the floor () we did in the gradient calcs into
// ceil (), but plus a little bit)
	if (r->ubasestep < 0)
		working_lstepx = r->r_lstepx - 1;
	else
		working_lstepx = r->r_lstepx;

	r->d_countextrastep = r->ubasestep + 1;
	d_ptexbasestep = ((r->r_sstepy + r->r_sstepx * r->ubasestep) >> 16) +
	    ((r->r_tstepy + r->r_tstepx * r->ubasestep) >> 16) *
	    skinwidth;
#if id386
	r->d_sfracbasestep = (r->r_sstepy + r->r_sstepx * r->ubasestep) << 16;
	r->d_tfracbasestep = (r->r_tstepy + r->r_tstepx * r->ubasestep) << 16;
#else
	r->d_sfracbasestep =
	    (r->r_sstepy + r->r_sstepx * r->ubasestep) & 0xFFFF;
	r->d_tfracbasestep =
	    (r->r_tstepy + r->r_tstepx * r->ubasestep) & 0xFFFF;
#endif
	r->d_lightbasestep = r->r_lstepy + working_lstepx * r->ubasestep;
	r->d_zibasestep = r->r_zistepy + r->r_zistepx * r->ubasestep;

	d_ptexextrastep =
	    ((r->r_sstepy + r->r_sstepx * r->d_countextrastep) >> 16) +
	    ((r->r_tstepy +
	      r->r_tstepx * r->d_countextrastep) >> 16) *
	    skinwidth;
#if id386
	r->d_sfracextrastep =
	    (r->r_sstepy + r->r_sstepx * r->d_countextrastep) << 16;
	r->d_tfracextrastep =
	    (r->r_tstepy + r->r_tstepx * r->d_countextrastep) << 16;
#else
	r->d_sfracextrastep =
	    (r->r_sstepy + r->r_sstepx * r->d_countextrastep) & 0xFFFF;
	r->d_tfracextrastep =
	    (r->r_tstepy + r->r_tstepx * r->d_countextrastep) & 0xFFFF;
#endif
	r->d_lightextrastep = r->d_lightbasestep + working_lstepx;
	r->d_ziextrastep = r->d_zibasestep + r->r_zistepx;

	D_PolysetScanLeftEdge(initialleftheight, d_pdest, d_ptex,
			      d_ptexbasestep, d_ptexextrastep, r, skinwidth);

//
// scan out the bottom part of the left edge, if it exists
//
	if (r->pedgetable->numleftedges == 2) {
		int height;

		plefttop = pleftbottom;
		pleftbottom = r->pedgetable->pleftedgevert2;

		D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
					  pleftbottom[0], pleftbottom[1], r);

		height = pleftbottom[1] - plefttop[1];

// TODO: make this a function; modularize this function in general

		ystart = plefttop[1];
		r->d_aspancount = plefttop[0] - prighttop[0];
		d_ptex = (unsigned char *) pskin + (plefttop[2] >> 16) +
		    (plefttop[3] >> 16) * skinwidth;
		r->d_sfrac = 0;
		r->d_tfrac = 0;
		r->d_light = plefttop[4];
		r->d_zi = plefttop[5];

		r->d_pdestbasestep = screenwidth + r->ubasestep;
		r->d_pdestextrastep = r->d_pdestbasestep + 1;
		d_pdest =
		    (unsigned char *) d_viewbuffer + ystart * screenwidth + plefttop[0];
#if id386
		r->d_pzbasestep = (d_zwidth + r->ubasestep) << 1;
		r->d_pzextrastep = r->d_pzbasestep + 2;
#else
		r->d_pzbasestep = d_zwidth + r->ubasestep;
		r->d_pzextrastep = r->d_pzbasestep + 1;
#endif
		r->d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

		if (r->ubasestep < 0)
			working_lstepx = r->r_lstepx - 1;
		else
			working_lstepx = r->r_lstepx;

		r->d_countextrastep = r->ubasestep + 1;
		d_ptexbasestep =
		    ((r->r_sstepy + r->r_sstepx * r->ubasestep) >> 16) +
		    ((r->r_tstepy +
		      r->r_tstepx * r->ubasestep) >> 16) *
		    skinwidth;
#if id386
		r->d_sfracbasestep =
		    (r->r_sstepy + r->r_sstepx * r->ubasestep) << 16;
		r->d_tfracbasestep =
		    (r->r_tstepy + r->r_tstepx * r->ubasestep) << 16;
#else
		r->d_sfracbasestep =
		    (r->r_sstepy + r->r_sstepx * r->ubasestep) & 0xFFFF;
		r->d_tfracbasestep =
		    (r->r_tstepy + r->r_tstepx * r->ubasestep) & 0xFFFF;
#endif
		r->d_lightbasestep =
		    r->r_lstepy + working_lstepx * r->ubasestep;
		r->d_zibasestep = r->r_zistepy + r->r_zistepx * r->ubasestep;

		d_ptexextrastep =
		    ((r->r_sstepy + r->r_sstepx * r->d_countextrastep) >> 16) +
		    ((r->r_tstepy +
		      r->r_tstepx * r->d_countextrastep) >> 16) *
		    skinwidth;
#if id386
		r->d_sfracextrastep =
		    ((r->r_sstepy +
		      r->r_sstepx * r->d_countextrastep) & 0xFFFF) << 16;
		r->d_tfracextrastep =
		    ((r->r_tstepy +
		      r->r_tstepx * r->d_countextrastep) & 0xFFFF) << 16;
#else
		r->d_sfracextrastep =
		    (r->r_sstepy + r->r_sstepx * r->d_countextrastep) & 0xFFFF;
		r->d_tfracextrastep =
		    (r->r_tstepy + r->r_tstepx * r->d_countextrastep) & 0xFFFF;
#endif
		r->d_lightextrastep = r->d_lightbasestep + working_lstepx;
		r->d_ziextrastep = r->d_zibasestep + r->r_zistepx;

		D_PolysetScanLeftEdge(height, d_pdest, d_ptex, d_ptexbasestep,
				      d_ptexextrastep, r, skinwidth);
	}
// scan out the top (and possibly only) part of the right edge, updating the
// count field
	r->d_pedgespanpackage = r->a_spans;

	D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
				  prightbottom[0], prightbottom[1], r);
	r->d_aspancount = 0;
	r->d_countextrastep = r->ubasestep + 1;
	originalcount = r->a_spans[initialrightheight].count;
	r->a_spans[initialrightheight].count = -999999;	// mark end of the spanpackages
	D_PolysetDrawSpans8(r->a_spans, d_viewbuffer, r, acolormap,
			    skinwidth);

#if 0
	printf("%s:%d\n", __func__, __LINE__);
#endif
// scan out the bottom part of the right edge, if it exists
	if (r->pedgetable->numrightedges == 2) {
		int height;
		spanpackage_t *pstart;

		pstart = r->a_spans + initialrightheight;
		pstart->count = originalcount;

		r->d_aspancount = prightbottom[0] - prighttop[0];

		prighttop = prightbottom;
		prightbottom = r->pedgetable->prightedgevert2;

		height = prightbottom[1] - prighttop[1];

		D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
					  prightbottom[0], prightbottom[1], r);

		r->d_countextrastep = r->ubasestep + 1;
		r->a_spans[initialrightheight + height].count = -999999;
		// mark end of the spanpackages
#if 0
		printf("%s:%d\n", __func__, __LINE__);
#endif
		D_PolysetDrawSpans8(pstart, d_viewbuffer, r, acolormap,
				    skinwidth);
	}
}

static void D_DrawNonSubdiv(unsigned char * d_viewbuffer, short *d_pzbuffer,
			    unsigned char * acolormap, int d_zwidth,
			    int screenwidth, int *d_xdenom, int skinwidth, struct r_finalmesh *fm)
/*
Definition at line 266 of file d_polyse.c.

References ALIAS_ONSEAM, D_PolysetSetEdgeTable(), D_RasterizeAliasPolySmooth(), d_xdenom, mtriangle_s::facesfront, finalvert_s::flags, affinetridesc_t::numtriangles, affinetridesc_t::pfinalverts, affinetridesc_t::ptriangles, r_affinetridesc, r_p0, r_p1, r_p2, affinetridesc_t::seamfixupX16, finalvert_s::v, and mtriangle_s::vertindex.

Referenced by D_PolysetDraw().
*/
{
	mtriangle_t *ptri;
	finalvert_t *pfv, *index0, *index1, *index2;
	int i;
	int lnumtriangles;
	spanpackage_t spans[DPS_MAXSPANS + 1 +
			    ((CACHE_SIZE - 1) / sizeof(spanpackage_t)) + 1];
	struct r_state r;

	r.a_spans = (spanpackage_t *)
	    (((long)&spans[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

	pfv = fm->finalverts;
	ptri = fm->triangles;
	lnumtriangles = fm->ntriangles;

	for (i = 0; i < lnumtriangles; i++, ptri++) {
		index0 = pfv + ptri->vertindex[0];
		index1 = pfv + ptri->vertindex[1];
		index2 = pfv + ptri->vertindex[2];

		*d_xdenom = (index0->v[1] - index1->v[1]) *
		    (index0->v[0] - index2->v[0]) -
		    (index0->v[0] - index1->v[0]) * (index0->v[1] -
						     index2->v[1]);

#if 0
		printf("%s:%d d_xdenom = %d, %d, %d, %d, %d, %d, %d\n",
		       __func__, __LINE__, d_xdenom, index0->v[0], index0->v[1],
		       index1->v[0], index1->v[1], index2->v[0], index2->v[1]);
#endif
		if (*d_xdenom >= 0) {
			continue;
		}

		r_p0[0] = index0->v[0];	// u
		r_p0[1] = index0->v[1];	// v
		r_p0[2] = index0->v[2];	// s
		r_p0[3] = index0->v[3];	// t
		r_p0[4] = index0->v[4];	// light
		r_p0[5] = index0->v[5];	// iz

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

		if (!ptri->facesfront) {
			if (index0->flags & ALIAS_ONSEAM)
				r_p0[2] += fm->seamfixupX16;
			if (index1->flags & ALIAS_ONSEAM)
				r_p1[2] += fm->seamfixupX16;
			if (index2->flags & ALIAS_ONSEAM)
				r_p2[2] += fm->seamfixupX16;
		}

		D_PolysetSetEdgeTable(&r);
#if 0
		printf("%s:%d running D_RasterizeAliasPolySmooth()\n", __func__,
		       __LINE__);
#endif
		D_RasterizeAliasPolySmooth(d_viewbuffer, d_pzbuffer, acolormap,
					   &r, d_zwidth, screenwidth, *d_xdenom,
					   fm->pskin, skinwidth);
	}
}

static void D_DrawSubdiv(unsigned char * d_viewbuffer, unsigned char * acolormap, short * const *zspantable, unsigned char **skintable, int *d_scantable, struct r_finalmesh *fm)
/*
Definition at line 190 of file d_polyse.c.

References acolormap, ALIAS_ONSEAM, d_pcolormap, D_PolysetRecursiveTriangle(), finalvert_s::flags, affinetridesc_t::numtriangles, affinetridesc_t::pfinalverts, affinetridesc_t::ptriangles, r_affinetridesc, affinetridesc_t::seamfixupX16, finalvert_s::v, and mtriangle_s::vertindex.

Referenced by D_PolysetDraw().
*/
{
	mtriangle_t *ptri;
	finalvert_t *pfv, *index0, *index1, *index2;
	int i;
	int lnumtriangles;
	unsigned char *d_pcolormap;

	pfv = fm->finalverts;
	ptri = fm->triangles;
	lnumtriangles = fm->ntriangles;


#ifdef FTE_PEXT_TRANS
#ifdef GLQUAKE
	if (transbackfac) {
		if (r_pixbytes == 4)
			drawfnc = D_PolysetRecursiveTriangle32Trans;
		else if (r_pixbytes == 2)
			drawfnc = D_PolysetRecursiveTriangle16C;
		else		//if (r_pixbytes == 1)
			drawfnc = D_PolysetRecursiveTriangleTrans;
	} else
#endif
#endif

		for (i = 0; i < lnumtriangles; i++) {
			index0 = pfv + ptri[i].vertindex[0];
			index1 = pfv + ptri[i].vertindex[1];
			index2 = pfv + ptri[i].vertindex[2];

			if (((index0->v[1] - index1->v[1]) *
			     (index0->v[0] - index2->v[0]) -
			     (index0->v[0] - index1->v[0]) *
			     (index0->v[1] - index2->v[1])) >= 0) {
				continue;
			}

			d_pcolormap = &acolormap[index0->v[4] & 0xFF00];

			if (ptri[i].facesfront) {
				D_PolysetRecursiveTriangle(index0->v, index1->v,
							   index2->v,
							   d_pcolormap,
							   d_viewbuffer,
							   zspantable, skintable, d_scantable);
			} else {
				int s0, s1, s2;

				s0 = index0->v[2];
				s1 = index1->v[2];
				s2 = index2->v[2];

				if (index0->flags & ALIAS_ONSEAM)
					index0->v[2] +=
					    fm->seamfixupX16;
				if (index1->flags & ALIAS_ONSEAM)
					index1->v[2] +=
					    fm->seamfixupX16;
				if (index2->flags & ALIAS_ONSEAM)
					index2->v[2] +=
					    fm->seamfixupX16;

				D_PolysetRecursiveTriangle(index0->v, index1->v,
							   index2->v,
							   d_pcolormap,
							   d_viewbuffer,
							   zspantable, skintable, d_scantable);

				index0->v[2] = s0;
				index1->v[2] = s1;
				index2->v[2] = s2;
			}
		}
}

void D_PolysetDraw(const struct r_view *v,
			  unsigned char * acolormap,
			  unsigned char **skintable,
			  int skinwidth, struct r_finalmesh *fm, int drawtype)
/*
Definition at line 132 of file d_polyse.c.

References CACHE_SIZE, D_DrawNonSubdiv(), D_DrawSubdiv(), DPS_MAXSPANS, affinetridesc_t::drawtype, and r_affinetridesc.

Referenced by R_AliasClipTriangle(), R_AliasPreparePoints(), and R_AliasPrepareUnclippedPoints().
*/
{
	int d_xdenom;
	// one extra because of cache line pretouching

	if (drawtype)
		D_DrawSubdiv(v->viewbuffer, acolormap, v->zspantable, skintable, v->d_scantable, fm);
	else
		D_DrawNonSubdiv(v->viewbuffer, v->zbuffer, acolormap,
				v->zwidth, v->screenwidth, &d_xdenom, skinwidth, fm);
}

