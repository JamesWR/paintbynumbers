CC = gcc
COPTS = `pkg-config --cflags --libs gtk+-2.0`

paintbynumbers:	paintbynumbers.c
	$(CC) paintbynumbers.c -o paintbynumbers $(COPTS)

onerow:	onerow.c
	$(CC) onerow.c -o onerow $(COPTS)

