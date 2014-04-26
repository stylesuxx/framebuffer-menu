#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <dirent.h>
#include <termios.h>
#include <stdbool.h>

static const char BITMAP_FORMAT[] = "BM";

typedef struct IMAGE{
  char path[255];
  int width, height;
  int bpp;
  int dataOffset;
  bool isValid;
} IMAGE;

IMAGE build(const char *imagePath);
void draw(IMAGE image);
void cleanUp();
unsigned int getIntFromBytes(unsigned char bytes[4]);
int mygetch();
void usage();

int frameBufferFD = 0;
char *frameBuffer = 0;
int width = 0, height = 0, screensize = 0;
int depth = 32;

int main(int argc, char* argv[])
{
  DIR *imageFolder;
  char *imagePath;
  int selection = 0, images = 0;
  struct fb_var_screeninfo vinfo;
  struct fb_fix_screeninfo finfo;
  long int dx, dy;
  struct IMAGE menuImages[5];
  bool escape = false;

  if(argc < 2)
    usage();

  system("setterm -cursor off");
  system("stty -echo");

  imagePath = argv[1];
  
  char str[5][255];  
  struct dirent **namelist;
  int i,n;
  n = scandir(imagePath, &namelist, 0, alphasort);
  if (n < 0) {
    cleanUp();
    printf("Could not read image directory.\n");
    return 1;
  }
  else {
    for (i = 0; i < n; i++) {
      if(strstr(namelist[i]->d_name, ".bmp") == NULL) continue;

      strcpy(str[images], imagePath);
      strcat(str[images], "/");
      strcat(str[images], namelist[i]->d_name);

      // Build an image from the current path and add it as menuImage if image is processable.
      IMAGE currentImage = build(str[images]);
      if(currentImage.isValid) {
        menuImages[images] = currentImage;
        images++;  
      }

      free(namelist[i]);
    }
  }
  free(namelist);

  if(images == 0) {
    cleanUp();
    printf("Error: no images found.\n");
    return 1;
  }

  frameBufferFD = open("/dev/fb0", O_RDWR);
  if(!frameBufferFD) {
    cleanUp();
    printf("Error: cannot open framebuffer device.\n");
    return 1;
  }

  if (ioctl(frameBufferFD, FBIOGET_VSCREENINFO, &vinfo)) {
    cleanUp();
    printf("Error reading variable information.\n");
    return 1;
  }

  if(ioctl(frameBufferFD, FBIOGET_FSCREENINFO, &finfo)) {
    cleanUp();
    printf("Error reading fixed information.\n");
    return 1;
  }

  screensize = finfo.smem_len;
  depth = vinfo.bits_per_pixel;
  width = finfo.line_length / (depth/8);
  height = (screensize / (depth/8)) / width;
  frameBuffer = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, frameBufferFD, 0);

  draw(menuImages[0]);
  for(;;) {
    unsigned char keyPress = mygetch();
    if(keyPress == 'B' && escape) {
      selection++;
      if(selection >= images)
        selection = images-1;

      draw(menuImages[selection]);
    }

    if(keyPress == 'A' && escape) {
      selection--;
      if(selection < 0)
        selection = 0;

      draw(menuImages[selection]);
    }

    if(keyPress == '[') escape = true;
    else escape = false;

    if(keyPress == '\n')
      break;
  }

  cleanUp();
  
  printf("%d\n", selection + 1);

  return 0;
}

void cleanUp()
{
  close(frameBufferFD);
  munmap(frameBuffer, screensize);
  
  system("setterm -cursor on");
  system("stty echo");
  system("clear");
}

struct IMAGE build(const char *imagePath)
{
  FILE *image;
  struct IMAGE data;
  unsigned char format[3] = {'\0'};
  unsigned char fileSize[4];
  unsigned char offset[4];
  unsigned char width[4];
  unsigned char height[4];
  unsigned int fileSizeInBytes = 0;
  unsigned int fileSizeReal = 0;
  
  // Set the path to the image  
  strcpy(data.path, imagePath);

  image = fopen(imagePath, "r+");
  
  // Check for right bitmap format
  fread(format, 2, 1, image);
  if(strcmp(format, BITMAP_FORMAT) != 0) {
    printf("Not the correct format.\n");
    data.isValid = false;
  }

  // Check the filesize indicated by the header with the files real size
  fread(fileSize, 4, 1, image);
  fileSizeInBytes = getIntFromBytes(fileSize);

  fseek(image, 0, SEEK_END);
  fileSizeReal =  ftell(image);

  if(fileSizeInBytes != fileSizeReal) {
    printf("Image file corrupted, file size does not match.\n");
    data.isValid = false;
  }

  // Get the offset at which the actual image data starts
  fseek(image, 10, SEEK_SET);
  fread(offset, 4, 1, image);
  data.dataOffset = getIntFromBytes(offset);

  // Get width & height
  fseek(image, 18, SEEK_SET);
  fread(width, 4, 1, image);
  fread(height, 4, 1, image);
  data.width = getIntFromBytes(width);
  data.height = getIntFromBytes(height);

  data.bpp = 32;

  fclose(image);
  return data;
}

unsigned int getIntFromBytes(unsigned char bytes[4])
{
  unsigned int result = 0;
  result |= bytes[0];
  result |= bytes[1]<<8;
  result |= bytes[2]<<16;
  result |= bytes[3]<<24;

  return result;
}

void draw(IMAGE meta)
{
  FILE *image;
  unsigned char *pixelBuffer = malloc(meta.bpp/8);
  unsigned char *revertBuffer = malloc(meta.bpp/8);
  unsigned int paddingHorizontal, paddingVertical;
  unsigned int dx, dy;
  unsigned int offsetBottom, offsetLeft, offsetRight;
  int i;
  int color = 0x00;

  // Calculate the offsets for a centered menu
  dx = width - meta.width;
  dy = height - meta.height;

  if(dx > 0) paddingVertical = dx / 2;
  if(dy > 0) paddingHorizontal = dy / 2;

  // Draw the border
  offsetBottom = width * (paddingHorizontal + meta.height) * (meta.bpp/8);
  memset(frameBuffer, color, width * paddingHorizontal * (meta.bpp/8));
  memset(frameBuffer + offsetBottom, color, width * paddingHorizontal * (meta.bpp/8));
  
  for(i = 0; i < height; i++) {
    offsetLeft  = width * i * (meta.bpp/8);
    offsetRight = offsetLeft + (paddingVertical + meta.width) * (meta.bpp/8);
    memset(frameBuffer + offsetLeft,  color, paddingVertical * (meta.bpp/8));
    memset(frameBuffer + offsetRight, color, paddingVertical * (meta.bpp/8));
  }

  // Draw the menu item
  image = fopen(meta.path, "r+");
  fseek(image, meta.dataOffset, SEEK_SET);

  int off;
  for(i = meta.height-1; i >= 0; i--) {
    off  = width * paddingHorizontal * (meta.bpp/8);
    off += width * (meta.bpp/8) * i;
    off += paddingVertical * (meta.bpp/8);

    int k;
    for(k = 0; k < meta.width; k++) {
      fread(pixelBuffer, sizeof(char), (meta.bpp/8), image);

      int j;
      for(j = 1; j <= (meta.bpp/8); j++)
        revertBuffer[j-1] = pixelBuffer[j];
      revertBuffer[(depth/8)] = pixelBuffer[0];

      memcpy(frameBuffer + off + (meta.bpp/8) * k, revertBuffer, (meta.bpp/8));
    }
  }

  fclose(image);
  free(pixelBuffer);
  free(revertBuffer);
}

void usage()
{
  printf("Usage: ./fbmenu IMAGE_FOLDER\n");
  exit(1);
}

int mygetch()
{
  struct termios oldt, newt;
  int ch;
  tcgetattr(STDIN_FILENO, &oldt);

  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  
  return ch;
}