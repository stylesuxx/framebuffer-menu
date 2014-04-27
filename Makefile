fbmenu: fbmenu.c
	cc fbmenu.c getch.c byteconversion.c framebufferimage.c -o fbmenu

PHONY: clean
clean:
	rm -rf fbmenu *.o
