#include "stdinclude.h"

gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);
gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
gboolean button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data);
void destroy(long windownumber, guint action, GtkWidget *widget);
void menuitem_response(long windownumber, guint action, GtkWidget *widget);
GtkWidget *createmenus(long windownumber);
void file_open(long windownumber);
void file_save(long windownumber);
void file_saveas(long windownumber);
void draw_window(long windownumber);
int file_new();
void savewindow(long windownumber);
void showerror(char *windowname, char *message);
int sumscheck(long windownumber);
void solve(long windownumber);
void file_print(long windownumber);
void draw_page(GtkPrintOperation *operation, GtkPrintContext *context, int page_nr, long windownumber);
void dbformat(char *str, char *ostr);

/* delete (close window) event */

gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data )
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

gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	long windownumber = (long)data;

	gdk_draw_drawable(widget->window, widget->style->fg_gc[gtk_widget_get_state(widget)], windows[windownumber].pixmap, event->area.x, event->area.y, event->area.x, event->area.y, event->area.width, event->area.height);
	return(FALSE);
}

/* configure event (create pixmap for drawing window */

gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
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
/* valid only in SETUP MODE */

gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
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
/* valid only in SETUP MODE */

gboolean button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
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

void destroy(long windownumber, guint action, GtkWidget *widget)
{
	/* check for save windows */
	for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
		if (windows[windownumber].widget && windows[windownumber].modified)
			savewindow(windownumber);

	gtk_main_quit();
}

/* menu events */

void menuitem_response(long windownumber, guint action, GtkWidget *widget)
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

GtkItemFactoryEntry menu_items[] = {
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

/* draw puzzle in window */

void draw_window(long windownumber)
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

GtkPrintSettings *printsettings = NULL;

/* master print routine */

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

/* print it: draw puzzle in print window */

void draw_page(GtkPrintOperation *operation, GtkPrintContext *printcontext, int page_nr, long windownumber)
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
