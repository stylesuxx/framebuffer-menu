#include "byteconversion.h"

unsigned int littleEndianToInt(unsigned char bytes[])
{
  unsigned int result = 0;
  unsigned int length = sizeof(bytes);
  
  int i;
  for(i= 0; i < length; i++)
    result |= bytes[i]<<(8*i);

  return result;
}