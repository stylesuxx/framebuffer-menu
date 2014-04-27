#include <stdio.h>

#include "byteconversion.h"

unsigned int littleEndianToInt(unsigned char bytes[], int length)
{
  unsigned int result = 0;

  int i;
  for(i = 0; i < length; i++)
    result |= bytes[i]<<(8*i);

  return result;
}