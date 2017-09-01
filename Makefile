CC = gcc
COPTS = `pkg-config --cflags --libs gtk+-2.0` -I /usr/include/gtk-2.0
OBJECTS = main.c db.c graphics.c sanitycheck.c solve.c window.c

npaintbynumbers: stdinclude.h $(OBJECTS)
	$(CC) $(OBJECTS) -o npaintbynumbers $(COPTS)

stdinclude.h: menus.h solve.h window.h

paintbynumbers:	paintbynumbers.c
	$(CC) paintbynumbers.c -o paintbynumbers $(COPTS)

onerow:	onerow.c
	$(CC) onerow.c -o onerow $(COPTS)

