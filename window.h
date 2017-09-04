/* definitions for the paint by numbers information */
#define MAINWINDOWMENUBARHEIGHTS	105
#define BOXMAX	20
#define MAXX	60
#define MAXY	60
#define DEFAULTX	20
#define DEFAULTY	20
#define PRINTBORDERSIZE	0.25	/* 0.5 inch */
#define PRINTBOXMAX	0.25	/* 0.75 inch */
#define PRINTFONTSCALE	0.6

gint screenwidth, screenheight;

/* structure for holding puzzle data, one per window */

struct rec {
	int x;		/* x dimension */
	int y;		/* y dimension */
	int **xlist;
	int **ylist;
	int *solution;
	int xy;	/* 0 is doing x entry, 1 is doing y entry */
	int index1;	/* which x or y entry */
	int index2;	/* index within that entry */
	int newsum;	/* 1 if just starting this entry */
	int mode;	/* mode */
} rec;

/* structure for windows */

#define NWINDOWS	500
struct windows {
	GtkWidget *widget;
	GtkWidget *drawing_area;
	GdkPixmap *pixmap;
	struct rec rec;
	int modified;
	char *filename;
}  windows[NWINDOWS];

