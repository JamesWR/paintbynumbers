/* function prototypes */

char *ncvt(char *str, int *v);
gboolean button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);
gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data);
GtkWidget *createmenus(long windownumber);
int file_new();
int open_by_file(char *name);
int sumscheck(long windownumber);
long new_window(int width, int height, char *title);
struct solution *genpatterns(int n, int *list, long long vecon, long long vecoff);
unsigned long long binom(int r, int s, int n);
void dbformat(char *str, char *ostr);
void destroy(long windownumber, guint action, GtkWidget *widget);
void draw_page(GtkPrintOperation *operation, GtkPrintContext *context, int page_nr, long windownumber);
void draw_window(long windownumber);
void file_open(long windownumber);
void file_print(long windownumber);
void file_saveas(long windownumber);
void file_save(long windownumber);
void menuitem_response(long windownumber, guint action, GtkWidget *widget);
void printsolution(int nx, int ny, int *solution, char *message);
void savewindow(long windownumber);
void showerror(char *windowname, char *message);
void solve(long windownumber);
