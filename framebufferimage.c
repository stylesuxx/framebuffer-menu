#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "framebufferimage.h"

struct imageData *getMeta(const char *imagePath)
{
  FILE *image;
  image = fopen(imagePath, "r");
  imageData *data;
  data = (imageData *) malloc(sizeof(imageData));

  unsigned char format[3] = {'\0'};
  unsigned char fileSize[4];
  unsigned char offset[4];
  unsigned char width[4];
  unsigned char height[4];

  unsigned int fileSizeInBytes = 0;
  unsigned int fileSizeReal = 0;
  unsigned int bytesRead;

  strcpy(data->path, imagePath);

  // Check for right bitmap format
  fread(format, 2, 1, image);
  if(strcmp(format, BITMAP_FORMAT) != 0) {
    fclose(image);
    return NULL;
  }

  // Check the filesize indicated by the header with the files real size
  fread(fileSize, 4, 1, image);
  fileSizeInBytes = littleEndianToInt(fileSize, 4);

  fseek(image, 0, SEEK_END);
  fileSizeReal =  ftell(image);

  if(fileSizeInBytes != fileSizeReal) {
    fclose(image);
    return NULL;
  }

  // Get the offset at which the actual image data starts
  fseek(image, 10, SEEK_SET);
  fread(offset, 4, 1, image);
  data->offset = littleEndianToInt(offset, 4);

  // Get width & height
  fseek(image, 18, SEEK_SET);
  fread(width, 4, 1, image);
  fread(height, 4, 1, image);
  data->width = littleEndianToInt(width, 4);
  data->height = littleEndianToInt(height, 4);

  // Check the images depth
  data->bpp = ((fileSizeInBytes - data->offset) / (data->height * data->width)) * 8;
  if(data->bpp != 32) {
  	fclose(image);
  	return NULL;
  }

  fclose(image);
  
  return data;
}