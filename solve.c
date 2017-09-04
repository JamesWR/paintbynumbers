#include "stdinclude.h"

void draw_window(long windownumber);
void showerror(char *windowname, char *message);
void solve(long windownumber);
struct solution *genpatterns(int n, int *list, long long vecon, long long vecoff);
void printsolution(int nx, int ny, int *solution, char *message);
unsigned long long binom(int r, int s, int n);

/* solve */

int insert = 1;		/* generate database inserts for solutions */

struct solution{
	struct solution *next;
	long long solution;
};

/* top level solver */
void solve(long windownumber)
{
	struct rec *rp = &windows[windownumber].rec;
	int flag = 1;
	int x, y;
	struct solution *sp, *nsp, *lsp;
	struct solution **xvalues, **yvalues;
	int *xflags, *yflags;
	unsigned long long *xweights, *yweights;
	long long vecon, vecoff;
	int *xvec, *yvec;
	int cnt;
	int tcnt;
	unsigned long long mcnt;
	int mind;
	struct tms tms;
	double clocktick = (double)sysconf(_SC_CLK_TCK);
	long long baset, deltat;
	/* XX x & y vectors of row/column with changes */

	times(&tms);
	baset = tms.tms_utime;

	/* create solution possibilities vectors */
	if ( (xvalues = (struct solution **)malloc(rp->x * sizeof(struct solution *))) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return;
	}
	for (x = 0; x < rp->x; x++)
		xvalues[x] = (struct solution *)-1;
	if ( (yvalues = (struct solution **)malloc(rp->y * sizeof(struct solution *))) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return;
	}
	for (y = 0; y < rp->y; y++)
		yvalues[y] = (struct solution *)-1;

	/* initialize flags */
	if ( (xflags = (int *)malloc(rp->x * sizeof(xflags[0]))) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return;
	}
	for (x = 0; x < rp->x; x++){
		xflags[x] = 0;
	}
	if ( (yflags = (int *)malloc(rp->y * sizeof(yflags[0]))) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return;
	}
	for (y = 0; y < rp->y; y++){
		yflags[y] = 0;
	}

	/* calculate weights */
	if ( (xweights = (long long *)malloc(rp->x * sizeof(xweights[0]))) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return;
	}
	mcnt = 0;
	for (x = 0; x < rp->x; x++) {
		cnt = 0;
		for (y = 0; rp->xlist[x][y]; y++)
			cnt += rp->xlist[x][y];
		xweights[x] = binom(rp->y, cnt, y);
		if (mcnt == 0 || xweights[x] < mcnt) {
			mcnt = xweights[x];
			mind = x;
		}
	}
	xflags[mind] = 1;
	xweights[mind] = 0;
	if ( (yweights = (long long *)malloc(rp->y * sizeof(yweights[0]))) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return;
	}
	mcnt = 0;
	for (y = 0; y < rp->y; y++) {
		cnt = 0;
		for (x = 0; rp->ylist[y][x]; x++)
			cnt += rp->ylist[y][x];
		yweights[y] = binom(rp->x, cnt, x);
		if (mcnt == 0 || yweights[y] < mcnt) {
			mcnt = yweights[y];
			mind = y;
		}
	}
	yflags[mind] = 1;
	yweights[mind] = 0;

	/* initialize solution */
	if ((rp->solution = (int *)malloc(rp->x * rp->y * sizeof(rp->solution[0]))) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return;
	}
	for (x = 0; x < rp->x; x++)
	for (y = 0; y < rp->y; y++)
		rp->solution[x + rp->x * y] = 0;

	if ( (xvec = (int *)malloc(rp->x * sizeof(xvec[0]))) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return;
	}
	if ( (yvec = (int *)malloc(rp->y * sizeof(yvec[0]))) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return;
	}

	while (flag) {
		flag = 0;
		/* scan for solution values */
		for (x = 0; x < rp->x; x++) {
			/* XX xflags and change */
			if (xflags[x] <= 0)
				continue;
			if (xvalues[x] == (struct solution *)-1) {
				/* need to generate patterns */
				vecon = 0;
				vecoff = 0;
				for (y = 0; y < rp->y; y++) {
					if (rp->solution[x + rp->x * y] > 0)
						vecon |= (long long)1 << y;
					else if (rp->solution[x + rp->x * y] < 0)
						vecoff |= (long long)1 << y;
				}
				xvalues[x] = genpatterns(rp->y, rp->xlist[x], vecon, vecoff);
			}
			sp = xvalues[x];
			if (sp == NULL) {
				showerror(windows[windownumber].filename, "No solutions");
				fprintf(stderr, "No solutions\n");
				goto DONE;
			}
			cnt = 0;
			vecon = 0x7FFFFFFFFFFFFFFF;
			vecoff = 0;
			for ( ; sp; sp = sp->next) {
				cnt++;
				vecon &= sp->solution;
				vecoff |= sp->solution;
			}
			for (y = 0; y < rp->y; y++) {
				if (rp->solution[x + rp->x * y] == 0) {
					if (vecon & ((long long)1 << y)) {
						/* XX if change, set flag */
						rp->solution[x + rp->x * y] = 1;
						flag = 1;
					}
					if ( (vecoff & ((long long)1 << y)) == 0) {
						/* XX if change, set flag */
						rp->solution[x + rp->x * y] = -1;
						flag = 1;
					}
				}
			}
		}
		for (y = 0; y < rp->y; y++) {
			/* XX yflags and change */
			if (yflags[y] <= 0)
				continue;
			if (yvalues[y] == (struct solution *)-1) {
				/* need to generate patterns */
				vecon = 0;
				vecoff = 0;
				for (x = 0; x < rp->x; x++) {
					if (rp->solution[x + rp->x * y] > 0)
						vecon |= (long long)1 << x;
					else if (rp->solution[x + rp->x * y] < 0)
						vecoff |= (long long)1 << x;
				}
				yvalues[y] = genpatterns(rp->x, rp->ylist[y], vecon, vecoff);
			}
			sp = yvalues[y];
			if (sp == NULL) {
				showerror(windows[windownumber].filename, "No solutions");
				fprintf(stderr, "No solutions\n");
				goto DONE;
			}
			cnt = 0;
			vecon = 0x7FFFFFFFFFFFFFFF;
			vecoff = 0;
			for ( ; sp; sp = sp->next) {
				cnt++;
				vecon &= sp->solution;
				vecoff |= sp->solution;
			}
			for (x = 0; x < rp->x; x++) {
				if (rp->solution[x + rp->x * y] == 0) {
					if (vecon & ((long long)1 << x)) {
						/* XX if change, set flag */
						rp->solution[x + rp->x * y] = 1;
						flag = 1;
					}
					if ( (vecoff & ((long long)1 << x)) == 0) {
						/* XX if change, set flag */
						rp->solution[x + rp->x * y] = -1;
						flag = 1;
					}
				}
			}
		}
		/* scan to eliminate invalid solutions */
		for (x = 0; x < rp->x; x++) {
			/* XX xflags and change */
			if (xflags[x] <= 0)
				continue;
			/* generate masks */
			vecon = 0;
			vecoff = 0;
			for (y = 0; y < rp->y; y++) {
				if (rp->solution[x + rp->x * y] > 0)
					vecon |= (long long)1 << y;
				else if (rp->solution[x + rp->x * y] < 0)
					vecoff |= (long long)1 << y;
			}
			lsp = (struct solution *)&xvalues[x];
			cnt = 0;
			for (sp = xvalues[x]; sp; sp = nsp) {
				nsp = sp->next;
				if ( ((sp->solution & vecon) != vecon) || ((sp->solution & vecoff) != 0)) {
					lsp->next = nsp;
					free(sp);
					flag = 1;
				} else {
					cnt++;
					lsp = sp;
				}
			}
		}
		for (y = 0; y < rp->y; y++) {
			/* XX xflags and change */
			if (yflags[y] <= 0)
				continue;
			/* generate masks */
			vecon = 0;
			vecoff = 0;
			for (x = 0; x < rp->x; x++) {
				if (rp->solution[x + rp->x * y] > 0)
					vecon |= (long long)1 << x;
				else if (rp->solution[x + rp->x * y] < 0)
					vecoff |= (long long)1 << x;
			}
			lsp = (struct solution *)&yvalues[y];
			cnt = 0;
			for (sp = yvalues[y]; sp; sp = nsp) {
				nsp = sp->next;
				if ( (sp->solution & vecon) != vecon || (sp->solution & vecoff) != 0) {
					lsp->next = nsp;
					free(sp);
					flag = 1;
				} else {
					cnt++;
					lsp = sp;
				}
			}
		}
/* printsolution(rp->x, rp->y, rp->solution, ""); */
		if (flag == 0) {
			mcnt = -1;
			mind = -1;
			for (x = 0; x < rp->x; x++)
				if (xweights[x] > 0 && xweights[x] < mcnt) {
					mcnt = xweights[x];
					mind = x;
				}
			if (mind >= 0) {
				xflags[mind] = 1;
				xweights[mind] = 0;
				flag = 1;
			}
			mcnt = -1;
			mind = -1;
			for (y = 0; y < rp->y; y++)
				if (yweights[y] > 0 && yweights[y] < mcnt) {
					mcnt = yweights[y];
					mind = y;
				}
			if (mind >= 0) {
				yflags[mind] = 1;
				yweights[mind] = 0;
				flag = 1;
			}
		}
#ifdef SHOWASYOUGO
		{
			int hx, hy;
			int tx, ty;
			int boxsize;
			int width, height;

			hx = (rp->x + 1) / 2;
			hy = (rp->y + 1) / 2;
			tx = hx + rp->x;
			ty = hy + rp->y;
			boxsize = screenwidth / tx;
			if ( (screenheight / ty) < boxsize)
				boxsize = screenheight / ty;
			if (boxsize > BOXMAX)
				boxsize = BOXMAX;
			width = tx * boxsize;
			height = ty * boxsize;
			draw_window(windownumber);
			gdk_draw_drawable(windows[windownumber].widget->window, windows[windownumber].widget->style->fg_gc[gtk_widget_get_state(windows[windownumber].widget)], windows[windownumber].pixmap, 0, 0, 0, 0, width, height);
		}
#endif /* SHOWASYOUGO */
	}
DONE:
	tcnt = 0;
	for (x = 0; x < rp->x; x++) {
		if (xvalues[x] != (struct solution *)-1)
			for (sp = xvalues[x], cnt = 0; sp; sp = nsp, cnt++) {
				nsp = sp->next;
				free(sp);
			}
			if (cnt > 1)
				tcnt++;
	}
	free(xvalues);
	for (y = 0; y < rp->y; y++) {
		if (yvalues[y] != (struct solution *)-1)
			for (sp = yvalues[y], cnt = 0; sp; sp = nsp, cnt++) {
				nsp = sp->next;
				free(sp);
			}
			if (cnt > 1)
				tcnt++;
	}
	free(yvalues);
	free(xweights);
	free(yweights);
	free(xflags);
	free(yflags);
	free(xvec);
	free(yvec);

	if (tcnt)
		showerror(windows[windownumber].filename, "Unresolved solution");
		
	times(&tms);
	deltat = tms.tms_utime - baset;
	if (insert && tcnt == 0) {
		/* INSERT */
		char *t, *tt;

		t = windows[windownumber].filename;
		for (tt = t; *tt; tt++)
			if (*tt == '/')
				t = tt + 1;
		fprintf(stderr, "INSERT INTO t_pbn VALUES (0, '%s', 'PBN', '%dx%d', '%.2f', '', '', '', 'pending', '');\n", t, rp->x, rp->y, deltat / clocktick);
	} else {
		/* simple printout */
		fprintf(stderr, "%s: TIME: %.2f\n", windows[windownumber].filename, deltat / clocktick);
	}
	draw_window(windownumber);
}

/* print solution */

void printsolution(int nx, int ny, int *solution, char *message)
{
	int x, y;

	fprintf(stderr, "SOLUTION %s\n", message);
	for (y = 0; y < ny; y++) {
		for (x = 0; x < nx; x++) {
			if (solution[x + nx * y] > 0)
				fprintf(stderr, "X");
			else if (solution[x + nx * y] < 0)
				fprintf(stderr, " ");
			else
				fprintf(stderr, ".");
		}
		fprintf(stderr, "\n");
	}
}

/* generate the currently valid patterns for a single row or column */

struct solution *genpatterns(int n, int *list, long long vecon, long long vecoff)
{
	int nlist;
	int *plist;
	int *result;
	long long vec;
	struct solution *rv = NULL, *trv;
	int i, j, k;
int cntall, cntval;

	/* size list */
	for (nlist = 0; list[nlist]; nlist++) ;
	if (nlist == 0) {
		if ( (rv = (struct solution *)malloc(sizeof(struct solution))) == NULL) {
			fprintf(stderr, "Malloc failed in genpatterns\n");
			return(NULL);
		}
		rv->next = NULL;
		rv->solution = 0;
		return(rv);
	}

	/* memory allocation */
	if ( (plist = (int *)malloc(nlist * sizeof(plist[0]))) == NULL) {
		fprintf(stderr, "Malloc failed in genpatterns\n");
		return(NULL);
	}
	if ( (result = (int *)malloc(n * sizeof(result[0]))) == NULL) {
		fprintf(stderr, "Malloc failed in genpatterns\n");
		return(NULL);
	}

	/* initialization */
	j = 0;
	for (i = 0; list[i]; i++) {
		plist[i] = j;
		j += list[i] + 1;
	}

cntall = 0;
cntval = 0;
	while (1) {
		/* generate this pattern */
		j = 0;
		for (i = 0; i < nlist; i++) {
			while (j < plist[i])
				result[j++] = 0;
			for (k = 0; k < list[i]; k++)
				result[j++] = 1;
		}
		while (j < n)
			result[j++] = 0;

		/* generate mask */
		vec = 0;
		for (j = 0; j < n; j++)
			if (result[j] > 0)
				vec |= (long long)1 << j;
cntall++;
		/* save if mask succeeds vector match */
		if ( ((vec & vecon) == vecon) && ((vec & vecoff) == 0)) {
			/* stash pattern in output vector */
			if ( (trv = (struct solution *)malloc(sizeof(struct solution))) == NULL) {
				fprintf(stderr, "Malloc failed in genpatterns\n");
				return(NULL);
			}
			trv->next = rv;
			rv = trv;
			trv->solution = vec;
cntval++;
		}

		/* move to next pattern */
		for (i = nlist - 1; i >= 0; --i) {
			k = plist[i];
			for (j = i; j < nlist; j++)
				k += list[j] + 1;
			if (k <= n) {
				plist[i]++;
				break;
			}
		}
		if (i < 0)
			break;
		for (i = i + 1; i < nlist; i++) {
			plist[i] = plist[i-1] + list[i-1] + 1;
		}
		if (plist[nlist-1] + list[nlist-1] > n)
			break;
	}

	free(plist);
	free(result);
	return(rv);
}

/* calculate maximum number of possible solutions */

/* THIS ROUTINE HAS WRAPAROUND PROBLEMS */
unsigned long long binom(int r, int s, int n)
{
	int t = r - s + 1;
	long long result = 1;
	int i;

	if (n == 0)
		return(result);
	for (i = t - n + 1; i <= t; i++)
		result *= i;
	for (i = 2; i <= n; i++)
		result /= i;
	if (result == 0)	/* in case wrap went to zero */
		result = 0xFFFFFFFFFFFFFFFF;
	return(result);
}

