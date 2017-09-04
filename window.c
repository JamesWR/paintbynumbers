#include "stdinclude.h"

gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);
gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
gboolean button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data);
GtkWidget *createmenus(long windownumber);
long new_window(int width, int height, char *title);
void file_open(long windownumber);
void file_save(long windownumber);
void file_saveas(long windownumber);
int open_by_file(char *name);
void draw_window(long windownumber);
int file_new();
void showerror(char *windowname, char *message);
char *ncvt(char *str, int *v);

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

long new_window(int width, int height, char *title)
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

void file_open(long windownumber)
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

void file_save(long windownumber)
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

void file_saveas(long windownumber)
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

int open_by_file(char *name)
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

/* input is string, output is int of next integer and pointer to rest of string */
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
