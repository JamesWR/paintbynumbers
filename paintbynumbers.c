#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <gtk/gtk.h>

/* @(#) paintbynumbers.c 1.26@(#) */

int insert = 1;		/* generate database inserts for solutions */

/* DATA AND DEFINITION AREA (.h) */

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

#define COMMENTSLENGTH	64
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

#define NWINDOWS	500
struct windows {
	GtkWidget *widget;
	GtkWidget *drawing_area;
	GdkPixmap *pixmap;
	struct rec rec;
	int modified;
	char *filename;
}  windows[NWINDOWS];

struct solution{
	struct solution *next;
	long long solution;
};

#define MENU_FILE_NEW		1000 + 0
#define MENU_FILE_OPEN		1000 + 1
#define MENU_FILE_CLOSE		1000 + 2
#define MENU_FILE_SAVE		1000 + 3
#define MENU_FILE_SAVEAS	1000 + 4
#define MENU_FILE_PRINT		1000 + 5
#define MENU_FILE_PRINTALL	1000 + 6

#define MENU_EDIT_UNDO		2000 + 0
#define MENU_EDIT_CUT		2000 + 1
#define MENU_EDIT_COPY		2000 + 2
#define MENU_EDIT_PASTE		2000 + 3
#define MENU_EDIT_CLEAR		2000 + 4

#define MENU_MODE_SETUP		3000 + 1
#define MENU_MODE_VALIDATE	3000 + 3
#define MENU_MODE_SOLVE		3000 + 4
#define MENU_MODE_SOLVEALL	3000 + 5

/* ROUTINE PROTOTYPES */
static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);
static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static gboolean button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data);
static void destroy(long windownumber, guint action, GtkWidget *widget);
static void menuitem_response(long windownumber, guint action, GtkWidget *widget);
GtkWidget *createmenus(long windownumber);
static long new_window(int width, int height, char *title);
static void file_open(long windownumber);
static void file_save(long windownumber);
static void file_saveas(long windownumber);
static int open_by_file(char *name);
static void draw_window(long windownumber);
int file_new();
void savewindow(long windownumber);
void showerror(char *windowname, char *message);
int sumscheck(long windownumber);
void solve(long windownumber);
void file_print(long windownumber);
static void draw_page(GtkPrintOperation *operation, GtkPrintContext *context, int page_nr, long windownumber);
char *ncvt(char *str, int *v);
int solveinit(long windownumber);
struct solution *genpatterns(int n, int *list, long long vecon, long long vecoff);
void printsolution(int nx, int ny, int *solution, char *message);
unsigned long long binom(int r, int s, int n);
void dbformat(char *str, char *ostr);

/* MAIN */

int main(int argc, char *argv[])
{
	long windownumber;
	int i;
	int flag1 = 0;
	int flag2 = 0;
	GtkWidget *widget;
	GtkWidget *box;
	GtkWidget *draw;
	GdkScreen *screen;

	/* init */
	gtk_init(&argc, &argv);

	/* get screen size */
	widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	screen = gtk_window_get_screen(GTK_WINDOW(widget));
	screenwidth = gdk_screen_get_width(GDK_SCREEN(screen));
	screenheight = gdk_screen_get_height(GDK_SCREEN(screen)) - MAINWINDOWMENUBARHEIGHTS;
fprintf(stderr, "screenwidth=%d screenheight=%d\n", screenwidth, screenheight);
	gtk_widget_destroy(widget);

	for (windownumber = 0; windownumber < NWINDOWS; windownumber++) {
		windows[windownumber].widget = NULL;
	}

	for (i = 1; i < argc; i++) {
		flag1 = 1;
		if (open_by_file(argv[i]) >= 0)
			flag2++;
	}
	if (flag1 == 0) {
		if (file_new() < 0)
			return(0);
	} else if (flag2 == 0)
		return(0);

	gtk_main();

	return(0);
}

/* ROUTINES AREA */

/* delete (close window) event */
static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data )
{
	long windownumber = (long)data;
	int count = 0;

	if (windows[windownumber].modified)
		savewindow(windownumber);
	windows[windownumber].widget = NULL;

	count = 0;
	for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
		if (windows[windownumber].widget)
			count++;
	if (count == 0)
		gtk_main_quit();

	return(FALSE);
}

/* expose window event */

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	long windownumber = (long)data;

	gdk_draw_drawable(widget->window, widget->style->fg_gc[gtk_widget_get_state(widget)], windows[windownumber].pixmap, event->area.x, event->area.y, event->area.x, event->area.y, event->area.width, event->area.height);
	return(FALSE);
}

/* configure event (create pixmap for drawing window */

static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	long windownumber = (long)data;

	/* create pixmap and clear it */
	if (windows[windownumber].pixmap)
		g_object_unref(windows[windownumber].pixmap);
	windows[windownumber].pixmap = gdk_pixmap_new(widget->window, widget->allocation.width, widget->allocation.height, -1);
	gdk_draw_rectangle(windows[windownumber].pixmap, widget->style->white_gc, TRUE, 0, 0, widget->allocation.width, widget->allocation.height);

	return(TRUE);
}

/* key press event in drawing window */

static gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	long windownumber = (long)data;
	struct rec *rp;
	int kv;

	rp = &windows[windownumber].rec;
	if (rp->mode == MENU_MODE_SETUP) {
		/* GDK_MOD2_MASK is Numeric Lock */
		if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK | GDK_SUPER_MASK | GDK_HYPER_MASK | GDK_META_MASK))
			return(FALSE);
		kv = event->keyval & 0x7F;
		if (kv >= (int)'0' && kv <= (int)'9') {
			if (rp->xy == 0)
				if (rp->newsum)
					rp->xlist[rp->index1][rp->index2] = kv - (int)'0';
				else
					rp->xlist[rp->index1][rp->index2] = (rp->xlist[rp->index1][rp->index2] % 10) * 10 + kv - (int)'0';
			else
				if (rp->newsum)
					rp->ylist[rp->index1][rp->index2] = kv - (int)'0';
				else
					rp->ylist[rp->index1][rp->index2] = (rp->ylist[rp->index1][rp->index2] % 10) * 10 + kv - (int)'0';
			rp->newsum = 0;
			windows[windownumber].modified = 1;
			draw_window(windownumber);
			return(FALSE);
		} else if (kv == 0x09) {
			/* return or enter, step to next number in this unit */
			rp->index2++;
			if (rp->xy == 0) {
				if (rp->index2 >= (rp->y + 1) / 2) {
					rp->index2 = 0;
					rp->index1++;
					if (rp->index1 >= rp->x) {
						rp->xy = 1;
						rp->index1 = 0;
						rp->index2 = 0;
					}
				}
			} else {
				if (rp->index2 >= (rp->x + 1) / 2) {
					rp->index2 = 0;
					rp->index1++;
					if (rp->index1 >= rp->y) {
						rp->xy = 0;
						rp->index1 = 0;
						rp->index2 = 0;
					}
				}
			}
			rp->newsum = 1;
			draw_window(windownumber);
		} else if (kv == 0x0A || kv == 0x0D) {
			/* tab, step to next unit */
			rp->index2 = 0;
			rp->index1++;
			if (rp->xy == 0) {
				if (rp->index1 >= rp->x) {
					rp->xy = 1;
					rp->index1 = 0;
					rp->index2 = 0;
				}
			} else {
				if (rp->index1 >= rp->y) {
					rp->xy = 0;
					rp->index1 = 0;
					rp->index2 = 0;
				}
			}
			rp->newsum = 1;
			draw_window(windownumber);
		}
	}
	return(FALSE);
}

/* mouse button press event in drawing window */

static gboolean button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	long windownumber = (long)data;
	int x, y;
	int tx, ty;
	int hx, hy;
	int boxsize;
	struct rec *rp;

	rp = &windows[windownumber].rec;

	/* calculate box size */
	hx = (rp->x + 1) / 2;
	hy = (rp->y + 1) / 2;
	tx = hx + rp->x;
	ty = hy + rp->y;
	boxsize = screenwidth / tx;
	if ( (screenheight / ty) < boxsize)
		boxsize = screenheight / ty;
	if (boxsize > BOXMAX)
		boxsize = BOXMAX;

	/* get coords of box */
	x = event->x / boxsize;
	y = event->y / boxsize;
	if (event->button == 1 && windows[windownumber].pixmap != NULL)
		if (rp->mode == MENU_MODE_SETUP) {
			if ( (x >= hx && x < tx) && (y >= 0 && y < hy)) {
				rp->xy = 0;
				rp->index1 = x - hx;
				rp->index2 = y;
				rp->newsum = 1;
				draw_window(windownumber);
			} else if ( (x >= 0 && x < hx) && (y >= hy && y < ty)) {
				rp->xy = 1;
				rp->index1 = y - hy;
				rp->index2 = x;
				rp->newsum = 1;
				draw_window(windownumber);
			}
		}
	return(TRUE);
}

/* destroy (save altered windows and exit) */

static void destroy(long windownumber, guint action, GtkWidget *widget)
{
	/* check for save windows */
	for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
		if (windows[windownumber].widget && windows[windownumber].modified)
			savewindow(windownumber);

	gtk_main_quit();
}

/* menu events */

static void menuitem_response(long windownumber, guint action, GtkWidget *widget)
{
	int count;
	GtkWidget *topwidget = windows[windownumber].widget;
	int x, y;
	struct rec *rp;

	rp = &windows[windownumber].rec;
	switch(action) {
		case MENU_FILE_NEW:
			(void)file_new();
			break;
		case MENU_FILE_OPEN:
			file_open(windownumber);
			break;
		case MENU_FILE_CLOSE:
			if (windows[windownumber].modified)
				savewindow(windownumber);
			gtk_widget_destroy(topwidget);
			windows[windownumber].widget = NULL;
			count = 0;
			for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
				if (windows[windownumber].widget)
					count++;
			if (count == 0)
				gtk_main_quit();
			break;
		case MENU_FILE_SAVE:
			file_save(windownumber);
			break;
		case MENU_FILE_SAVEAS:
			file_saveas(windownumber);
			break;
		case MENU_FILE_PRINT:
			file_print(windownumber);
			break;
		case MENU_FILE_PRINTALL:
			file_print(-1);
			break;

		case MENU_EDIT_UNDO:
			fprintf(stderr, "UNDO NOT IMPLEMENTED\n");
			break;
		case MENU_EDIT_CUT:
			fprintf(stderr, "CUT NOT IMPLEMENTED\n");
			break;
		case MENU_EDIT_COPY:
			fprintf(stderr, "COPY NOT IMPLEMENTED\n");
			break;
		case MENU_EDIT_PASTE:
			fprintf(stderr, "PASTE NOT IMPLEMENTED\n");
			break;
		case MENU_EDIT_CLEAR:
			fprintf(stderr, "CLEAR NOT IMPLEMENTED\n");
			break;

			break;
		case MENU_MODE_SETUP:
			rp->mode = MENU_MODE_SETUP;
			rp->xy = 0;
			rp->index1 = 0;
			rp->index2 = 0;
			rp->newsum = 1;
		DONE:	draw_window(windownumber);
			break;
		case MENU_MODE_VALIDATE:
			if (sumscheck(windownumber)) {
				rp->mode = MENU_MODE_SETUP;
				break;
			}
			rp->mode = MENU_MODE_VALIDATE;
			draw_window(windownumber);
			break;
		case MENU_MODE_SOLVE:
			if (sumscheck(windownumber)) {
				rp->mode = MENU_MODE_SETUP;
				draw_window(windownumber);
			} else
				solve(windownumber);
			draw_window(windownumber);
			break;
		case MENU_MODE_SOLVEALL:
			for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
				if (windows[windownumber].widget) {
					if (sumscheck(windownumber)) {
						rp->mode = MENU_MODE_SETUP;
					} else
						solve(windownumber);
					draw_window(windownumber);
				}
			break;
	}
}

/* menu data */

static GtkItemFactoryEntry menu_items[] = {
	{"/_File", NULL, NULL, 0, "<Branch>"},
	{"/_File/_New", "<control>N", menuitem_response, MENU_FILE_NEW, "<StockItem>", GTK_STOCK_NEW},
	{"/_File/_Open", "<control>O", menuitem_response, MENU_FILE_OPEN, "<StockItem>", GTK_STOCK_OPEN},
	{"/_File/_Close", "<control>W", menuitem_response, MENU_FILE_CLOSE, "<Item>"},
	{"/_File/_Save", "<control>S", menuitem_response, MENU_FILE_SAVE, "<StockItem>", GTK_STOCK_SAVE},
	{"/_File/Save_As", NULL, menuitem_response, MENU_FILE_SAVEAS, "<Item>"},
	{"/_File/sep1", NULL, NULL, 0, "<Separator>"},
	{"/_File/_Print", "<control>P", menuitem_response, MENU_FILE_PRINT, "<Item>"},
	{"/_File/P_rintAll", "<shift><control>P", menuitem_response, MENU_FILE_PRINTALL, "<Item>"},
	{"/_File/sep2", NULL, NULL, 0, "<Separator>"},
	{"/_File/_Quit", "<CTRL>Q", destroy, 0, "<StockItem>", GTK_STOCK_QUIT},

	{"/_Edit", NULL, NULL, 0, "<Branch>"},
	{"/_Edit/_Undo", "<control>Z", menuitem_response, MENU_EDIT_UNDO, "<StockItem>", GTK_STOCK_UNDO},
	{"/_Edit/_Cut", "<control>X", menuitem_response, MENU_EDIT_CUT, "<StockItem>", GTK_STOCK_CUT},
	{"/_Edit/_Copy", "<control>C", menuitem_response, MENU_EDIT_COPY, "<StockItem>", GTK_STOCK_COPY},
	{"/_Edit/_Paste", "<control>V", menuitem_response, MENU_EDIT_PASTE, "<StockItem>", GTK_STOCK_PASTE},
	{"/_Edit/_Clear", NULL, menuitem_response, MENU_EDIT_CLEAR, "<StockItem>", GTK_STOCK_CLEAR},

	{"/_Mode", NULL, NULL, 0, "<Branch>"},
	{"/_Mode/_Setup", "<control>2", menuitem_response, MENU_MODE_SETUP, "<Item>"},
	{"/_Mode/Validate", "<control>4", menuitem_response, MENU_MODE_VALIDATE, "<Item>"},
	{"/_Mode/Solve", "<control>5", menuitem_response, MENU_MODE_SOLVE, "<Item>"},
	{"/_Mode/SolveAll", "<control>6", menuitem_response, MENU_MODE_SOLVEALL, "<Item>"},

	{"/_Help", NULL, NULL, 0, "<Branch>"},
};

/* create menus */

GtkWidget *createmenus(long windownumber)
{
	GtkWidget *widget = windows[windownumber].widget;
	gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;

	/* Make accelerator group */
	accel_group = gtk_accel_group_new();

	/* Make an ItemFactory */
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

	/* Generate menu items */
	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, (gpointer)windownumber);

	/* Attach the new accelerator group to the widget */
	gtk_window_add_accel_group(GTK_WINDOW(widget), accel_group);

	/* return the menu bar */
	return(gtk_item_factory_get_widget(item_factory, "<main>"));
}

/* menu File New response - get dimensions for new window and create it */

int file_new()
{
	int x = DEFAULTX, y = DEFAULTY;
	int hx, hy;
	int tx, ty;
	int boxsize;
	int i;
	int it;
	gchar *str;
	gchar *xstr, *ystr, *p;
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *stock;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *xentry;
	GtkWidget *yentry;
	long windownumber;
	struct rec *rp;
	gint dialogresponse;

	/* get dimensions */
	for (windownumber = 0; windownumber < NWINDOWS; windownumber++) {
		if (windows[windownumber].widget == (GtkWidget *)0) {
			break;
		}
	}
	str = g_strdup_printf("Window %ld", windownumber + 1);
	dialog = gtk_dialog_new_with_buttons(str, NULL, GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, "Cancel", GTK_RESPONSE_CANCEL, NULL);
	g_free(str);

	/* set default response */
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 0);

	table = gtk_table_new(2, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);

	label = gtk_label_new("X");
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	xentry = gtk_entry_new();
	str = g_strdup_printf("%d", DEFAULTX);
	gtk_entry_set_text(GTK_ENTRY(xentry), str);
	g_free(str);
	gtk_table_attach_defaults(GTK_TABLE(table), xentry, 1, 2, 0, 1);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), xentry);

	label = gtk_label_new("Y");
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	yentry = gtk_entry_new();
	str = g_strdup_printf("%d", DEFAULTY);
	gtk_entry_set_text(GTK_ENTRY(yentry), str);
	g_free(str);
	gtk_table_attach_defaults(GTK_TABLE(table), yentry, 1, 2, 1, 2);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), yentry);

	gtk_widget_show_all(dialog);

	dialogresponse = gtk_dialog_run(GTK_DIALOG(dialog));
	if (dialogresponse == GTK_RESPONSE_OK) {
		xstr = gtk_entry_get_text(GTK_ENTRY(xentry));	/* ERROR */
		p = ncvt(xstr, &tx);
		ystr = gtk_entry_get_text(GTK_ENTRY(yentry));	/* ERROR */
		p = ncvt(ystr, &ty);
		if (tx >= 2 && tx <= MAXX && ty >= 2 && ty <= MAXY) {
			x = tx;
			y = ty;
		} else {
			showerror(NULL, "Dimensions too large");
			dialogresponse = GTK_RESPONSE_CANCEL;
		}
	}

	gtk_widget_destroy(dialog);

	if (dialogresponse != GTK_RESPONSE_OK)
		return(-1);

	/* calculate box size */
	hx = (x + 1) / 2;
	tx = x + hx;
	boxsize = screenwidth / tx;
	hy = (y + 1) / 2;
	ty = y + hy;
	if ( (screenheight / ty) < boxsize)
		boxsize = screenheight / ty;
	if (boxsize > BOXMAX)
		boxsize = BOXMAX;

	/* create new window */
	windownumber = new_window(tx * boxsize, ty * boxsize, NULL);
	/* set up initial grid */
	rp = &windows[windownumber].rec;
	rp->x = x;
	rp->y = y;
	if ( (rp->xlist = (int **)malloc(sizeof(rp->xlist[0]) * x)) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return(-1);
	}
	for (i = 0; i < x; i++) {
		if ( (rp->xlist[i] = (int *)malloc(sizeof(rp->xlist[0][0]) * (hy + 1))) == NULL) {
			showerror(windows[windownumber].filename, "malloc failure");
			return(-1);
		}
		for (it = 0; it <= hy; it++)
			rp->xlist[i][it] = 0;
	}
	if ( (rp->ylist = (int **)malloc(sizeof(rp->ylist[0]) * y)) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return(-1);
	}
	for (i = 0; i < y; i++) {
		if ( (rp->ylist[i] = (int *)malloc(sizeof(rp->ylist[0][0]) * (hx + 1))) == NULL) {
			showerror(windows[windownumber].filename, "malloc failure");
			return(-1);
		}
		for (it = 0; it <= hx; it++)
			rp->ylist[i][it] = 0;
	}
	rp->mode = MENU_MODE_SETUP;
	rp->xy = 0;
	rp->index1 = 0;
	rp->index2 = 0;
	rp->newsum = 1;

	/* draw content */
	draw_window(windownumber);
	windows[windownumber].filename = NULL;
	windows[windownumber].modified = 0;
	return(windownumber);
}

/* basic new window routine */

static long new_window(int width, int height, char *title)
{
	GtkWidget *menuwindow;
	GtkWidget *button;
	GtkWidget *windowbox;
	GtkWidget *windowboxlabel;
	GtkWidget *mainwindow;
	struct rec *rp;
	long windownumber;
	char ltitle[16];

	for (windownumber = 0; windownumber < NWINDOWS; windownumber++) {
		if (windows[windownumber].widget == (GtkWidget *)0) {
			break;
		}
	}
	if (windownumber >= NWINDOWS) {
		showerror(NULL, "Too many windows");
		return;
	}

	/* top widget */
	windows[windownumber].widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable(GTK_WINDOW(windows[windownumber].widget), FALSE);
	rp = &windows[windownumber].rec;
	rp->mode = MENU_MODE_SETUP;
	windows[windownumber].modified = 0;
	/* title */
	if (title == NULL) {
		sprintf(ltitle, "Window %ld", windownumber + 1);
		title = ltitle;
	}
	gtk_window_set_title(GTK_WINDOW(windows[windownumber].widget), title);
	/* callback on delete-event */
	g_signal_connect(windows[windownumber].widget, "delete_event", G_CALLBACK(delete_event), (gpointer)windownumber);

	/* window box */
	windowbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(windows[windownumber].widget), windowbox);
	gtk_widget_show(windowbox);

	/* create menus */
	menuwindow = createmenus(windownumber);
	/* pack in box */
	gtk_box_pack_start(GTK_BOX(windowbox), menuwindow, FALSE, FALSE, 0);
	gtk_widget_show(menuwindow);

	/* create drawing area */
	windows[windownumber].drawing_area = gtk_drawing_area_new();
	/* set size */
	gtk_widget_set_size_request(windows[windownumber].drawing_area, width, height);
	/* pack in box */
	gtk_box_pack_start(GTK_BOX(windowbox), windows[windownumber].drawing_area, TRUE, TRUE, 0);
	/* display */
	gtk_widget_show(windows[windownumber].drawing_area);

	/* signals to handle backing pixmap */
	g_signal_connect(windows[windownumber].drawing_area, "expose_event", G_CALLBACK(expose_event), (gpointer)windownumber);
	g_signal_connect(windows[windownumber].drawing_area, "configure_event", G_CALLBACK(configure_event), (gpointer)windownumber);

	/* signals to handle events */
	g_signal_connect(windows[windownumber].drawing_area, "button_press_event", G_CALLBACK(button_press_event), (gpointer)windownumber);
	g_signal_connect(windows[windownumber].widget, "key_press_event", G_CALLBACK(key_press_event), (gpointer)windownumber);
	gtk_widget_set_events(windows[windownumber].drawing_area, GDK_EXPOSURE_MASK
		| GDK_LEAVE_NOTIFY_MASK
		| GDK_KEY_PRESS_MASK
		| GDK_BUTTON_PRESS_MASK);
	/* display */
	gtk_widget_show(windows[windownumber].widget);

	/* clear filename */
	windows[windownumber].filename = NULL;
	windows[windownumber].modified = 0;

	return(windownumber);
}

/* File Open handler */

static void file_open(long windownumber)
{
	GtkWidget *dialog;
	char *path;
	char *p;
	char *file = 0;
	long newwindownumber;

	dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(windows[windownumber].widget), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	if (windows[windownumber].filename) {
		path = strdup(windows[windownumber].filename);
		for (p = path; *p; p++)
			if (*p == '/')
				file = p;
		if (file) {
			*file = 0;
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
		}
		free(path);
	}
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		windows[windownumber].filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		newwindownumber =  open_by_file(windows[windownumber].filename);
		gtk_window_set_transient_for(GTK_WINDOW(windows[newwindownumber].widget), GTK_WINDOW(windows[windownumber].widget));
	}
	gtk_widget_destroy(dialog);
}

/* File Save handler */

static void file_save(long windownumber)
{
	FILE *fp;
	struct rec *rp;
	int x, y;
	int i;
	int *sp;

	if (windows[windownumber].filename == NULL)
		file_saveas(windownumber);

	if ((fp = fopen(windows[windownumber].filename, "w")) == NULL) {
		showerror(windows[windownumber].filename, "Cannot create");
		return;
	}

	rp = &windows[windownumber].rec;

	/* file type */
	fprintf(fp, "PBN\n");

	/* write x and y */
	fprintf(fp, "%d %d\n", rp->x, rp->y);

	/* write mode */
	fprintf(fp, "%d\n", rp->mode);

	/* write runs */
	for (x = 0; x < rp->x; x++) {
		for (sp = rp->xlist[x]; *sp; sp++)
			fprintf(fp, "%d ", *sp);
		fprintf(fp, "\n");
	}
	for (y = 0; y < rp->y; y++) {
		for (sp = rp->ylist[y]; *sp; sp++)
			fprintf(fp, "%d ", *sp);
		fprintf(fp, "\n");
	}

	windows[windownumber].modified = 0;
	fclose(fp);
}

/* File SaveAs handler (also invoked if new not yet saved) */

static void file_saveas(long windownumber)
{
	GtkWidget *dialog;
	char *str;
	char ltitle[16];

	dialog = gtk_file_chooser_dialog_new("Save File", GTK_WINDOW(windows[windownumber].widget), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	if (windows[windownumber].filename == NULL) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), ".");
		sprintf(ltitle, "Window %ld", windownumber + 1);
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), ltitle);
	} else
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), windows[windownumber].filename);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		windows[windownumber].filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		file_save(windownumber);
	}

	gtk_widget_destroy(dialog);
}

/* open existing file and extract data */

static int open_by_file(char *name)
{
	FILE *fp;
	int x, y;
	int hx, hy;
	int tx, ty;
	int boxsize;
	int windownumber = 0;
	int i;
	int ip;
	struct rec *rp;
	char *p, *sname;
	char buffer[2048];

	/* open file */
	if ((fp = fopen(name, "r")) == NULL) {
		showerror(name, "Cannot open");
		return(-1);
	}

	/* check file type */
	if ( !(p = fgets(buffer, sizeof(buffer), fp)) || strcmp(buffer, "PBN\n")) {
		showerror(name, "Wrong file type");
		fclose(fp);
		return(-1);
	}

	/* read x and y */
	if ( !(p = fgets(buffer, sizeof(buffer), fp)) ) {
		showerror(name, "Read failure");
		fclose(fp);
		return(-1);
	}
	p = ncvt(p, &x);
	p = ncvt(p, &y);
	if (*p != '\n' || x > MAXX || y > MAXY) {
		showerror(name, "Bad format");
		fclose(fp);
		return(-1);
	}

	/* calculate box size */
	hx = (x + 1) / 2;
	tx = x + hx;
	boxsize = screenwidth / tx;
	hy = (y + 1) / 2;
	ty = y + hy;
	if ( (screenheight / ty) < boxsize)
		boxsize = screenheight / ty;
	if (boxsize > BOXMAX)
		boxsize = BOXMAX;

	/* create new window */
	sname = name;
	for (p = name; *p; p++) {
		if (*p == '/')
			sname = p+1;
	}
	windownumber = new_window(tx * boxsize, ty * boxsize, sname);

	/* set up initial grid */
	rp = &windows[windownumber].rec;
	rp->x = x;
	rp->y = y;

	/* get mode */
	if ( !(p = fgets(buffer, sizeof(buffer), fp)) ) {
		showerror(name, "Read failure");
		fclose(fp);
		return(-1);
	}
	p = ncvt(p, &rp->mode);

	if ( (rp->xlist = (int **)malloc(sizeof(rp->xlist[0]) * x)) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return(-1);
	}
	for (i = 0; i < x; i++) {
		if ( (rp->xlist[i] = (int *)malloc(sizeof(rp->xlist[0][0]) * (hy + 1))) == NULL) {
			showerror(windows[windownumber].filename, "malloc failure");
			return(-1);
		}
		if ( !(p = fgets(buffer, sizeof(buffer), fp)) ) {
			showerror(name, "Read failure");
			fclose(fp);
			return(-1);
		}
		p = buffer;
		for (ip = 0; ip < hy; ip++) {
			p = ncvt(p, &rp->xlist[i][ip]);
			if (rp->xlist[i][ip] == 0)
				break;
		}
		rp->xlist[i][ip] = 0;
	}
	if ( (rp->ylist = (int **)malloc(sizeof(rp->ylist[0]) * y)) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return(-1);
	}
	for (i = 0; i < y; i++) {
		if ( (rp->ylist[i] = (int *)malloc(sizeof(rp->ylist[0][0]) * (hx + 1))) == NULL) {
			showerror(windows[windownumber].filename, "malloc failure");
			return(-1);
		}
		if ( !(p = fgets(buffer, sizeof(buffer), fp)) ) {
			showerror(name, "Read failure");
			fclose(fp);
			return(-1);
		}
		p = buffer;
		for (ip = 0; ip < hx; ip++) {
			p = ncvt(p, &rp->ylist[i][ip]);
			if (rp->ylist[i][ip] == 0)
				break;
		}
		rp->ylist[i][ip] = 0;
	}
	rp->xy = 0;
	rp->index1 = 0;
	rp->index2 = 0;
	rp->newsum = 1;

	/* bring to top */
	gtk_window_present(GTK_WINDOW(windows[windownumber].widget));

	/* draw content */
	draw_window(windownumber);

	/* set filename */
	windows[windownumber].filename = strdup(name);

	return(windownumber);
}

char *ncvt(char *s, int *v)
{
	int rv = 0;

	while (*s >= '0' && *s <= '9') {
		rv = rv * 10 + *s - '0';
		s++;
	}
	while (*s == ' ') s++;
	*v = rv;
	return(s);
}

/* draw it */

static void draw_window(long windownumber)
{
	int x, y;
	int ix, iy;
	int hx, hy;
	int tx, ty;
	int boxsize;
	struct rec *rp;
	gchar *str;
	PangoLayout *layout;
	PangoContext *context;
	PangoFontDescription *desc;
	int t;
	int mod;

	rp = &windows[windownumber].rec;

	/* calculate box size */
	hx = (rp->x + 1) / 2;
	hy = (rp->y + 1) / 2;
	tx = hx + rp->x;
	ty = hy + rp->y;
	boxsize = screenwidth / tx;
	if ( (screenheight / ty) < boxsize)
		boxsize = screenheight / ty;
	if (boxsize > BOXMAX)
		boxsize = BOXMAX;

	/* clear drawing area */
	gdk_draw_rectangle(windows[windownumber].pixmap, windows[windownumber].widget->style->white_gc, TRUE, 0, 0, windows[windownumber].widget->allocation.width, windows[windownumber].widget->allocation.height);

	/* PangoLanguage *pango_language_get_default(); */
	/* PangoFontMetrics *pango_font_get_metrics(PangoFont *font, PangoLanguage *language); */
	/* int pango_font_metrics_get_approximate_digit_width(PangoFontMetrics *metrics); */

	context = gdk_pango_context_get();
	layout = pango_layout_new(context);
	t = (9 * boxsize + BOXMAX/2) / BOXMAX;
	str = g_strdup_printf("sans %d", t);
	desc = pango_font_description_from_string(str);
	g_free(str);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
	pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

	/* highlight current entry slot, if any */
	if (rp->mode == MENU_MODE_SETUP) {
		if (rp->xy == 0) {
			gdk_draw_rectangle(windows[windownumber].pixmap, windows[windownumber].widget->style->dark_gc[0], TRUE, (hx + rp->index1) * boxsize, rp->index2 * boxsize, boxsize, boxsize);
		} else {
			gdk_draw_rectangle(windows[windownumber].pixmap, windows[windownumber].widget->style->dark_gc[0], TRUE, rp->index2 * boxsize, (hy + rp->index1) * boxsize, boxsize, boxsize);
		}
	}

	/* display solution if any */
	if (rp->solution) {
		for (x = 0; x < rp->x; x++)
		for (y = 0; y < rp->y; y++) {
			if (rp->solution[x + rp->x * y] > 0) {
				gdk_draw_rectangle(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, TRUE, (x + hx) * boxsize, (y + hy) * boxsize, boxsize, boxsize);
			} else if (rp->solution[x + rp->x * y] < 0) {
				gdk_draw_rectangle(windows[windownumber].pixmap, windows[windownumber].widget->style->white_gc, TRUE, (x + hx) * boxsize, (y + hy) * boxsize, boxsize, boxsize);
			} else {
				gdk_draw_rectangle(windows[windownumber].pixmap, windows[windownumber].widget->style->dark_gc[0], TRUE, (x + hx) * boxsize, (y + hy) * boxsize, boxsize, boxsize);
			}
		}
	}

	/* vertical lines */
	mod = (tx - rp->x) % 5;
	for (x = tx - rp->x; x <= tx; x++) {
		gdk_draw_line(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, x * boxsize, 0, x * boxsize, ty * boxsize);
		if ( (x % 5) == mod)
			gdk_draw_line(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, x * boxsize - 1, 0, x * boxsize - 1, ty * boxsize);
	}

	/* horizontal lines */
	mod = (ty - rp->y) % 5;
	for (y = ty - rp->y; y <= ty; y++) {
		gdk_draw_line(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, 0, y * boxsize, tx * boxsize, y * boxsize);
		if ( (y % 5) == mod)
			gdk_draw_line(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, 0, y * boxsize - 1, tx * boxsize, y * boxsize - 1);
	}

	/* top numbers */
	for (x = 0; x < rp->x; x++) {
		for (ix = 0; ix < hy; ix++) {
			if (rp->xlist[x][ix]) {
				str = g_strdup_printf("%d", rp->xlist[x][ix]);
				pango_layout_set_text(layout, str, -1);
				g_free(str);
				gdk_draw_layout(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, (x + hx) * boxsize, ix * boxsize, layout);
			} else {
				break;
			}
		}
	}

	/* left numbers */
	for (y = 0; y < rp->y; y++) {
		for (iy = 0; iy < hx; iy++) {
			if (rp->ylist[y][iy]) {
				str = g_strdup_printf("%d", rp->ylist[y][iy]);
				pango_layout_set_text(layout, str, -1);
				g_free(str);
				gdk_draw_layout(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, iy * boxsize, (y + hy) * boxsize, layout);
			} else {
				break;
			}
		}
	}

	/* flag entire area for redraw */
	gtk_widget_queue_draw_area(windows[windownumber].widget, 0, 0, windows[windownumber].widget->allocation.width, windows[windownumber].widget->allocation.height);
}

/* ask if you want to save an altered window */

void savewindow(long windownumber)
{
	GtkWidget *dialog;
	GtkResponseType result;
	char lmessage[2048];

	if (windows[windownumber].filename)
		sprintf(lmessage, "%s has changed, save?", windows[windownumber].filename);
	else
		sprintf(lmessage, "Window %ld has changed, save?", windownumber + 1);
	dialog = gtk_message_dialog_new(GTK_WINDOW(windows[windownumber].widget), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, lmessage);	/* ERROR */
	gtk_window_set_title(GTK_WINDOW(dialog), "Save?");
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if (result == GTK_RESPONSE_YES || result == GTK_RESPONSE_ACCEPT)
		if (windows[windownumber].filename)
			file_save(windownumber);
		else
			file_saveas(windownumber);
}

/* error dialog handler */

void showerror(char *windowname, char *message)
{
	GtkWidget *dialog;
	char *lmessage;

fprintf(stderr, "%s: %s\n", windowname, message);
	if (windowname != NULL) {
		lmessage = (char *)malloc(strlen(windowname) + strlen(message) + 4);
		sprintf(lmessage, "%s: %s", windowname, message);
	} else {
		lmessage = (char *)malloc(strlen(message) + 4);
		sprintf(lmessage, "%s", message);
	}
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, lmessage);	/* ERROR */
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	free(lmessage);
}

static GtkPrintSettings *printsettings = NULL;

void file_print(long windownumber)
{
	GtkPrintOperation *print;
	GtkPrintOperationResult res;
	int npages;

	if (windownumber < 0) {
		npages = 0;
		for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
			if (windows[windownumber].widget)
				npages++;
		windownumber = -1;
	} else {
		npages = 1;
	}
	print = gtk_print_operation_new();
	if (printsettings != NULL)
		gtk_print_operation_set_print_settings(print, printsettings);
	/* next two should probably be in a begin_print callback */
	gtk_print_operation_set_n_pages(print, npages);
	gtk_print_operation_set_unit(print, GTK_UNIT_MM);
	g_signal_connect(print, "draw_page", G_CALLBACK(draw_page), (gpointer)windownumber);
	res = gtk_print_operation_run(print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, NULL, NULL);
	if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		if (printsettings != NULL)
			g_object_unref(printsettings);
		printsettings = g_object_ref(gtk_print_operation_get_print_settings(print));
	}
	g_object_unref(print);
}

/* print it */

static void draw_page(GtkPrintOperation *operation, GtkPrintContext *printcontext, int page_nr, long windownumber)
{
	int x, y;
	int ix, iy;
	int tx, ty;
	int hx, hy;
	int mod;
	struct rec *rp;
	cairo_t *cr;
	gchar *str;
	PangoLayout *layout;
	PangoContext *pangocontext;
	PangoFontDescription *desc;
	gdouble printboxsize;
	gdouble cwidth, cheight;
	gdouble printborder;
	gdouble printboxmax;
	gdouble printtitleheight;
	char *t, *ts;

	if (windownumber < 0) {
		x = 0;
		for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
			if (windows[windownumber].widget)
				if (x++ == page_nr)
					break;
		if (windownumber >= NWINDOWS)
			return;
	}

	rp = &windows[windownumber].rec;

	/* get page size */
	cr = gtk_print_context_get_cairo_context(printcontext);
	cwidth = gtk_print_context_get_width(printcontext);
	cheight = gtk_print_context_get_height(printcontext);
	if (cwidth < cheight) {
		printborder = cwidth / 8.5 * PRINTBORDERSIZE;
		printboxmax = cwidth / 8.5 * PRINTBOXMAX;
	} else {
		printborder = cheight / 8.5 * PRINTBORDERSIZE;
		printboxmax = cheight / 8.5 * PRINTBOXMAX;
	}
	printtitleheight = printborder;

	cairo_set_line_width(cr, 0.2);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

	/* calculate box size */
	hx = 0;
	for (x = 0; x < rp->y; x++) {
		for (ix = 0; rp->ylist[x][ix]; ix++) ;
		if (ix > hx)
			hx = ix;
	}
	hy = 0;
	for (y = 0; y < rp->x; y++) {
		for (iy = 0; rp->xlist[y][iy]; iy++) ;
		if (iy > hy)
			hy = iy;
	}
	tx = hx + rp->x;
	ty = hy + rp->y;
	printboxsize = (cwidth - printborder * 2) / tx;
	if ( (cheight - printtitleheight - printborder * 2) / ty < printboxsize)
		printboxsize = (cheight - printtitleheight - printborder * 2) / ty;
	if (printboxsize > printboxmax)
		printboxsize = printboxmax;

	pangocontext = gdk_pango_context_get();
	layout = pango_layout_new(pangocontext);
	str = g_strdup_printf("sans %.1f", printboxsize * PRINTFONTSCALE);
	desc = pango_font_description_from_string(str);
	g_free(str);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	/* title */
	pango_layout_set_width(layout, -1);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
	ts = windows[windownumber].filename;
	for (t = windows[windownumber].filename; *t; t++)
		if (*t == '/')
			ts = t + 1;
	/* START DATABASE */
	{
		FILE *fp;
		char str[1024];
		char ostr[1024];

		snprintf(str, sizeof(str), "echo 'SELECT * FROM t_pbn WHERE pbn_name=\"%s\";' | mysql --user=root --password=bar.mysql home", ts);
		if (fp = popen(str, "r")) {
			fgets(str, sizeof(str), fp);
			if (fgets(str, sizeof(str), fp)) {
				dbformat(str, ostr);
				pango_layout_set_text(layout, ostr, -1);
				cairo_move_to(cr, printborder + (cwidth - 2 * printborder) * 3 / 4, printborder);
				pango_cairo_show_layout(cr, layout);
			}
			pclose(fp);
		}
	}
	/* END DATABASE */
	pango_layout_set_text(layout, ts, -1);
	cairo_move_to(cr, printborder, printborder);
	pango_cairo_show_layout(cr, layout);

	pango_layout_set_width(layout, -1);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

#ifdef GRAYBARS
	/* gray bars */
	if (rp->y > rp->x) {
		/* gray vertical bars */
		for (x = hx; x <= tx; x++) {
			if (x % 2 && x != tx) {
				cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
				cairo_rectangle(cr, printborder + x * printboxsize, printborder + printtitleheight, printboxsize, ty * printboxsize);
				cairo_fill(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			}
		}
	} else {
		/* gray horizontal bars */
		for (y = hy; y <= ty; y++) {
			if (y % 2 && y != ty) {
				cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
				cairo_rectangle(cr, printborder, printborder + printtitleheight + y * printboxsize, tx * printboxsize, printboxsize);
				cairo_fill(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			}
		}
	}
#endif /* GRAYBARS */

	/* display solution if any */
	if (rp->solution) {
		for (x = 0; x < rp->x; x++)
		for (y = 0; y < rp->y; y++) {
			if (rp->solution[x + rp->x * y] > 0) {
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_rectangle(cr, printborder + (x + hx) * printboxsize, printborder + printtitleheight + (y + hy) * printboxsize, printboxsize, printboxsize);
				cairo_fill(cr);
			} else if (rp->solution[x + rp->x * y] < 0) {
				cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
				cairo_rectangle(cr, printborder + (x + hx) * printboxsize, printborder + printtitleheight + (y + hy) * printboxsize, printboxsize, printboxsize);
				cairo_fill(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			} else {
				cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
				cairo_rectangle(cr, printborder + (x + hx) * printboxsize, printborder + printtitleheight + (y + hy) * printboxsize, printboxsize, printboxsize);
				cairo_fill(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			}
		}
	}

	/* vertical lines */
	mod = (tx - rp->x) % 5;
	for (x = hx; x <= tx; x++) {
		if ( (x % 5) == mod) {
			cairo_set_line_width(cr, 0.6);
		} else {
			cairo_set_line_width(cr, 0.2);
		}
		cairo_move_to(cr, printborder + x * printboxsize, printborder + printtitleheight);
		cairo_line_to(cr, printborder + x * printboxsize, printborder + printtitleheight + ty * printboxsize);
		cairo_stroke(cr);
	}

	/* horizontal lines */
	mod = (ty - rp->y) % 5;
	for (y = hy; y <= ty; y++) {
		if ( (y % 5) == mod) {
			cairo_set_line_width(cr, 0.6);
		} else {
			cairo_set_line_width(cr, 0.2);
		}
		cairo_move_to(cr, printborder, printborder + printtitleheight + y * printboxsize);
		cairo_line_to(cr, printborder + tx * printboxsize, printborder + printtitleheight + y * printboxsize);
		cairo_stroke(cr);
	}

	/* top numbers */
	for (x = 0; x < rp->x; x++) {
		for (ix = 0; ix < hy; ix++) {
			if (rp->xlist[x][ix]) {
				pango_layout_set_width(layout, (int)printboxsize);
				pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
				str = g_strdup_printf("%d", rp->xlist[x][ix]);
				pango_layout_set_text(layout, str, -1);
				g_free(str);
				cairo_move_to(cr, printborder + (x + hx) * printboxsize + printboxsize / 2, printborder + printtitleheight + ix * printboxsize);
				pango_cairo_show_layout(cr, layout);
			} else {
				break;
			}
		}
	}

	/* left numbers */
	for (y = 0; y < rp->y; y++) {
		for (iy = 0; iy < hx; iy++) {
			if (rp->ylist[y][iy]) {
				pango_layout_set_width(layout, (int)printboxsize);
				pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
				str = g_strdup_printf("%d", rp->ylist[y][iy]);
				pango_layout_set_text(layout, str, -1);
				g_free(str);
				cairo_move_to(cr, printborder + iy * printboxsize + printboxsize / 2, printborder + printtitleheight + (y + hy) * printboxsize);
				pango_cairo_show_layout(cr, layout);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_set_line_width(cr, 0.2);
				cairo_move_to(cr, printborder + (iy+1) * printboxsize, printborder + printtitleheight + (y + hy) * printboxsize);
				cairo_line_to(cr, printborder + (iy+1) * printboxsize, printborder + printtitleheight + (y + hy + 1) * printboxsize);
				cairo_stroke(cr);
			} else {
				break;
			}
		}
	}
}

void dbformat(char *str, char *ostr)
{
	char *pbn_id;
	char *pbn_name;
	char *pbn_style;
	char *pbn_size;
	char *pbn_ctime;
	char *pbn_mtime;
	char *pbn_iterations;
	char *pbn_notes;
	char *pbn_pending;
	char *istr = str;

	/* remove newline */
	while (*str && *str != '\n') str++;
	if (*str) *str++ = 0;
	str = istr;

	pbn_id = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_name = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_style = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_size = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_ctime = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_mtime = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_iterations = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_notes = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_pending = str;
	sprintf(ostr, "%s %s %s %s", pbn_size, pbn_ctime, pbn_mtime, pbn_notes);
}

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

/* solve */
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
