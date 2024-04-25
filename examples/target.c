#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

extern void fuzzMe(const uint8_t *buffer, uint64_t length);

void loop_read()
{
   char line[128];
   while(1)
   {
      FILE *f = fopen("/tmp/file1", "r");

      if (!f) {
          printf("no such file!\n");
          break;
      }

      if (!fgets(line, sizeof(line), f))
      {
          printf("error readin!\n");
          break;
      }

      printf("content: %s", line);

      fclose(f);
      sleep(1);
   }
}

int main(int argc, char **argv)
{
   fuzzMe("tes", 5);

   printf("main func PID: %d\n", getpid());

   loop_read();

   return 0;
}
