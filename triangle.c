#include "triangle.h"
static int triangle_core(int *lp1, int *lp2, int *lp3);

static void triangle_split(int *lp1, int *lp2, int *lp3,
				       unsigned char *d_pcolormap,
				       unsigned char *d_viewbuffer,
				       short * const *zspantable,
				       unsigned char **skintable,
				       const int *d_scantable, int ld)
{
	int new[6];
	int z;
	short *zbuf;
	int *temp;
// split this edge
	while(1) {
		temp = lp1;
		lp1 = lp3;
		if (ld) {
			lp2 = temp;
			lp3 = new;
		} else {
			temp = lp2;
			lp2 = new;
			lp3 = temp;
		}
		new[0] = (lp1[0] + lp2[0]) >> 1;
		new[1] = (lp1[1] + lp2[1]) >> 1;
		new[2] = (lp1[2] + lp2[2]) >> 1;
		new[3] = (lp1[3] + lp2[3]) >> 1;
		new[5] = (lp1[5] + lp2[5]) >> 1;
		if (!triangle_core(lp1, lp2, lp3))
			break;
		if (lp2[1] > lp1[1])
			goto nodraw;
		if ((lp2[1] == lp1[1]) && (lp2[0] < lp1[0]))
			goto nodraw;

		z = new[5] >> 16;
		zbuf = zspantable[new[1]] + new[0];
		if (z >= *zbuf) {
			int pix;

			*zbuf = z;
			pix = d_pcolormap[skintable[new[3] >> 16][new[2] >> 16]];
			d_viewbuffer[d_scantable[new[1]] + new[0]] = pix;
		}
nodraw:
		ld ^= ld;
	}
}
static int triangle_core(int *lp1, int *lp2, int *lp3)
{
	int d;
	int *temp;
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
	if (d < -1 || d > 1) {
 split3:
		temp = lp1;
		lp1 = lp3;
		lp3 = lp2;
		lp2 = temp;

		goto split;
	}

	return 0;			// entire tri is filled

 split2:
	temp = lp1;
	lp1 = lp2;
	lp2 = lp3;
	lp3 = temp;

 split:
	return 1;
}
void D_PolysetRecursiveTriangle(int *lp1, int *lp2, int *lp3,
				       unsigned char *d_pcolormap,
				       unsigned char *d_viewbuffer,
				       short * const *zspantable,
				       unsigned char **skintable,
				       const int *d_scantable)
/*
Definition at line 334 of file d_polyse.c.

References d_pcolormap, d_scantable, d_viewbuffer, skintable, and zspantable.

Referenced by D_DrawSubdiv().
*/
{

	triangle_split(lp1, lp2, lp3, d_pcolormap, d_viewbuffer,
			zspantable, skintable, d_scantable, 0);
}

