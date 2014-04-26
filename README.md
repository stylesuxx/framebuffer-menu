# Linux framebuffer menu / basic framebuffer image viewer

A simple menu for the framebuffer. You pass a folder with images, one for each highlighted menu item.
The user may now navigate through the menu items with cursor up and down.

When he presses enter his final choice is printed - this is the number of the chosen image.
The images are read alphabetically from the provided directory and have to be 32bit (X8 R8 G8 B8) BMP's. When you save the file with gimp as 32bit bmp you should be good to go.

The images are centered on the screen and a black border is drawn around them if the image size is smaller than the resolution.

The images may have different sizes, although for a menu each will have the same size with a different menu point highlighted.

At the current state the framebuffer menu only supports 32Bit Images and 32Bit framebuffers.
In future it will support lower frame buffer depths, downsampling a 32 Bit BMP to 24, 16 and 8 Bit.

The reason for the images to be 32Bit is that we do not need to care about the 4Byte alignment, which we will have automatically if each pixel is 4Bytes. We can than continuesly read the image data without calculating the alignment buffers on the line ends.