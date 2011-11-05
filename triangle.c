#include "triangle.h"
void D_PolysetRecursiveTriangle(int *lp1, int *lp2, int *lp3,
				       unsigned char *d_pcolormap,
				       unsigned char *d_viewbuffer,
				       const short **zspantable,
				       unsigned char **skintable,
				       const int *d_scantable)
/*
Definition at line 334 of file d_polyse.c.

References d_pcolormap, d_scantable, d_viewbuffer, skintable, and zspantable.

Referenced by D_DrawSubdiv().
*/
{
	int *temp;
	int d;
	int new[6];
	int z;
	short *zbuf;

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

	return;			// entire tri is filled

 split2:
	temp = lp1;
	lp1 = lp2;
	lp2 = lp3;
	lp3 = temp;

 split:
// split this edge
#if 0
	printf("%s:%d lp1[1] + lp2[1] = %d\n", __func__, __LINE__,
	       lp1[1] + lp2[1]);
#endif
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

	z = new[5] >> 16;
	zbuf = zspantable[new[1]] + new[0];
	if (z >= *zbuf) {
		int pix;

		*zbuf = z;
		pix = d_pcolormap[skintable[new[3] >> 16][new[2] >> 16]];
#if 0
		printf("%p = %d\n", &d_viewbuffer[d_scantable[new[1]] + new[0]],
		       pix);
#endif
		d_viewbuffer[d_scantable[new[1]] + new[0]] = pix;
#if 0
		printf("plot %d %d %d = %d\n", new[1], d_scantable[new[1]],
		       new[0], pix);
#endif
	}

 nodraw:
// recursively continue
	D_PolysetRecursiveTriangle(lp3, lp1, new, d_pcolormap, d_viewbuffer, zspantable, skintable, d_scantable);
	D_PolysetRecursiveTriangle(lp3, new, lp2, d_pcolormap, d_viewbuffer, zspantable, skintable, d_scantable);
}

