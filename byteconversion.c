#include "byteconversion.h"

unsigned int littleEndianToInt(unsigned char bytes[])
{
  unsigned int result = 0;

  int i;
  for(i = 0; i < sizeof(bytes); i++)
    result |= bytes[i]<<(8*i);

  return result;
}