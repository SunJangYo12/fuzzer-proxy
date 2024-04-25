#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 256

uint8_t afl_ptr[10];
void (*crashMe)() = (uint64_t)0xdeadbeef;


void fuzzMe(const uint8_t *buffer, uint64_t length)
{
  printf("fuzzMe: %s\n", buffer);

    /* Quarksl4bfuzzMe! */
  if (buffer[0] == 'Q')
    if (buffer[1] == 'u')
     if (buffer[2] == 'a')
         crashMe();
}


void writeFile()
{
    FILE* imgFile = fopen("x", "a");
    if(imgFile == NULL){
        printf("no image\n");
    }



    srand((unsigned int)time(NULL));
    int acak = rand();
    fprintf(imgFile, "zzz: %d\n", acak);

    fclose(imgFile);
}

extern void loop_read()
{
   //char line[128];
   while(1)
   {
/*      FILE *f = fopen("/tmp/file1", "r");

      if (!f) {
          printf("no such file!\n");
          break;
      }

      if (!fgets(line, sizeof(line), f))
      {
          printf("error readin!\n");
          break;
      }

      memcpy(afl_ptr, "xxxx", 10);


      printf("content: %s", line);

      fclose(f);*/
      sleep(1);

      //fuzzMe("Q", 2);
   }
}

void fuzz_one_input(const uint8_t *buf, int len) {
   fuzzMe(buf, len);
}

int main(int argc, char **argv)
{

//  ssize_t rlength = fread((void*) buffer, 1, BUFFER_SIZE, stdin);
//  if (rlength == -1)
//    return errno;

//   uint8_t buffer[BUFFER_SIZE];
//   memcpy(buffer, argv[1], BUFFER_SIZE);

   printf("main func PID: %d\n", getpid());

   loop_read();


//   writeFile();

   //fuzz_one_input(buffer, BUFFER_SIZE);

  return 0;
}
