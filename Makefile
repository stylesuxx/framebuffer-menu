fbmenu: fbmenu.c
	cc fbmenu.c getch.c -o fbmenu

PHONY: clean
clean:
	rm -rf fbmenu *.o
