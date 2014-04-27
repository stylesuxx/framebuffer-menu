#ifndef FRAMEBUFFERIMAGE_H
#define FRAMEBUFFERIMAGE_H

static const char BITMAP_FORMAT[] = "BM";

typedef struct imageData {
  char path[255];
  int width, height;
  int bpp;
  int offset;
  bool isValid;
} imageData;

imageData build(const char *imagePath);
imageData *getMeta(const char *imagePath);

#endif