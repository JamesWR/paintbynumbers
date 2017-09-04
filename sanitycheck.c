#include "stdinclude.h"

void draw_window(long windownumber);
void showerror(char *windowname, char *message);
int sumscheck(long windownumber);

/* in going from sums entry mode to validate mode, check that sums are valid for each run length and that horizontal and vertical sums total match */

int sumscheck(long windownumber)
{
	int x, y;
	int ix, iy;
	int *sp;
	struct rec *rp;
	char m[64];
	int hsum, vsum;

	rp = &windows[windownumber].rec;
	x = rp->x;
	y = rp->y;

	/* check x vectors */
	for (ix = 0; ix < x; ix++) {
		vsum = 0;
		for (iy = 0; rp->xlist[ix][iy]; iy++)
			vsum += rp->xlist[ix][iy];
		if (vsum + iy - 1 > y) {
			rp->xy = 0;
			rp->index1 = ix;
			rp->index2 = 0;
			rp->newsum = 1;
			showerror(windows[windownumber].filename, "X vector too large");
			draw_window(windownumber);
			return(-1);
		}
	}

	/* check y vectors */
	for (iy = 0; iy < y; iy++) {
		hsum = 0;
		for (ix = 0; rp->ylist[iy][ix]; ix++)
			hsum += rp->ylist[iy][ix];
		if (hsum + ix - 1 > x) {
			rp->xy = 1;
			rp->index1 = iy;
			rp->index2 = 0;
			rp->newsum = 1;
			showerror(windows[windownumber].filename, "Y vector too large");
			draw_window(windownumber);
			return(-1);
		}
	}

	/* check matching sums */
	hsum = 0;
	for (ix = 0; ix < x; ix++)
		for (sp = rp->xlist[ix]; *sp; sp++)
			hsum += *sp;
	vsum = 0;
	for (ix = 0; ix < y; ix++)
		for (sp = rp->ylist[ix]; *sp; sp++)
			vsum += *sp;
	if (hsum != vsum) {
		showerror(windows[windownumber].filename, "horizontal and vertical sum mismatch");
		return(-1);
	}
	return(0);
}
