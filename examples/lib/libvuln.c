#include <stdint.h>
#include <stdio.h>

void (*crashMe)() = (uint64_t)0xdeadbeef;

void fuzzMe(const uint8_t *buffer, uint64_t length)
{
  printf("lib fuzzMe: %s\n", buffer);

  if (buffer[0] == 'Q')
    if (buffer[1] == 'u')
     if (buffer[2] == 'a')
         crashMe();
}

