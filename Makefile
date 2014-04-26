fbmenu: fbmenu.c
	cc fbmenu.c -o fbmenu

PHONY: clean
clean:
	rm -rf fbmenu *.o
