#include "stdinclude.h"

int open_by_file(char *name);
int file_new();

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

