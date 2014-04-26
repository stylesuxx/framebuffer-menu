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

void draw(const char *imagePath);
void drawBorder();
void usage();
int mygetch();

char *frameBuffer = 0;
int width = 0, height = 0;
int menuWidth = 0, menuHeight = 0;
int paddingHorizontal = 0, paddingVertical = 0;
int depth = 32;

int main(int argc, char* argv[])
{
  DIR *imageFolder;
  char *imagePath;
  int selection = 0, frameBufferFD = 0, images = 0;
  struct fb_var_screeninfo vinfo;
  struct fb_fix_screeninfo finfo;
  long int screensize, dx, dy;

  if(argc < 4)
    usage();

  system("setterm -cursor off");
  system("stty -echo");

  menuWidth = strtol(argv[1], 0, 10);
  menuHeight = strtol(argv[2], 0, 10);
  imagePath = argv[3];

  const char *imagePaths[5];
  char str[5][255];  
  struct dirent **namelist;
  int i,n;
  n = scandir(imagePath, &namelist, 0, alphasort);
  if (n < 0) {
    printf("Could not read image directory.\n");
    return 1;
  }
  else {
    for (i = 0; i < n; i++) {
      if(strstr(namelist[i]->d_name, ".bmp") == NULL) continue;

      strcpy(str[images], imagePath);
      strcat(str[images], "/");
      strcat(str[images], namelist[i]->d_name);
      imagePaths[images] = str[images];

      images++;

      free(namelist[i]);
    }
  }
  free(namelist);

  if(images == 0) {
    printf("Error: no images found.\n");
    return 1;
  }

  frameBufferFD = open("/dev/fb0", O_RDWR);
  if(!frameBufferFD) {
    printf("Error: cannot open framebuffer device.\n");
    return 1;
  }

  if (ioctl(frameBufferFD, FBIOGET_VSCREENINFO, &vinfo))
    printf("Error reading variable information.\n");

  if(ioctl(frameBufferFD, FBIOGET_FSCREENINFO, &finfo))
    printf("Error reading fixed information.\n");

  screensize = finfo.smem_len;
  depth = vinfo.bits_per_pixel;
  width = finfo.line_length / (depth/8);
  height = (screensize / (depth/8)) / width;

  dx = width - menuWidth;
  dy = height - menuHeight;

  if(dx > 0) paddingVertical = dx / 2;
  if(dy > 0) paddingHorizontal = dy / 2;

  frameBuffer = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, frameBufferFD, 0);

  drawBorder();
  draw(imagePaths[0]);

  bool escape = false;
  for(;;) {
    unsigned char keyPress = mygetch();
    if(keyPress == 'B' && escape) {
      selection++;
      if(selection >= images)
        selection = images-1;

      draw(imagePaths[selection]);
    }

    if(keyPress == 'A' && escape) {
      selection--;
      if(selection < 0)
        selection = 0;

      draw(imagePaths[selection]);
    }

    if(keyPress == '[') escape = true;
    else escape = false;

    if(keyPress == '\n')
      break;
  }

  close(frameBufferFD);
  munmap(frameBuffer, screensize);
  
  system("setterm -cursor on");
  system("stty echo");
  system("clear");
  
  printf("%d\n", selection + 1);

  return 0;
}

void drawBorder()
{
  int color = 0x00;

  int offsetBottom = width * (paddingHorizontal + menuHeight) * (depth/8);
  memset(frameBuffer, color, width * paddingHorizontal * (depth/8));
  memset(frameBuffer + offsetBottom, color, width * paddingHorizontal * (depth/8));
  
  int i;
  for(i = 0; i < height; i++) {
    int offsetLeft  = width * i * (depth/8);
    int offsetRight = offsetLeft + (paddingVertical + menuWidth) * (depth/8);
    memset(frameBuffer + offsetLeft,  color, paddingVertical * (depth/8));
    memset(frameBuffer + offsetRight, color, paddingVertical * (depth/8));
  }
}

void draw(const char *imagePath)
{
  FILE *image;
  int offset = 0;
  unsigned char offsets[4];
  unsigned char *pixelBuffer = malloc(depth/8);
  unsigned char *revertBuffer = malloc(depth/8);

  image = fopen(imagePath, "r+");

  fseek(image, 10, SEEK_SET);
  fread(offsets, 4, 1, image);

  offset |= offsets[0];

  fseek(image, offset, SEEK_SET);

  int i, off;
  for(i = menuHeight-1; i >= 0; i--) {
    off  = width * paddingHorizontal * (depth/8);
    off += width * (depth/8) * i;
    off += paddingVertical * (depth/8);

    int k;
    for(k = 0; k < menuWidth; k++) {
      fread(pixelBuffer, sizeof(char), (depth/8), image);

      int j;
      for(j = 1; j <= (depth/8); j++)
        revertBuffer[j-1] = pixelBuffer[j];
      revertBuffer[(depth/8)] = pixelBuffer[0];

      memcpy(frameBuffer + off + (depth/8) * k, revertBuffer, (depth/8));
    }
  }

  fclose(image);
  free(pixelBuffer);
  free(revertBuffer);
}

void usage()
{
  printf("Usage: ./fbmenu WIDTH HEIGHT IMAGE_FOLDER\n");
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