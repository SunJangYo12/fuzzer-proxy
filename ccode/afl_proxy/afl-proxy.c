#include "config.h"
#include "types.h"
#include "fuzzer.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

__thread u32 __afl_map_size = MAP_SIZE;

void send_forkserver_error(int error)
{
  u32 status;
  if (!error || error > 0xffff) return;
  status = (FS_OPT_ERROR | FS_OPT_SET_ERROR(error));
  if (write(FORKSRV_FD + 1, (char *)&status, 4) != 4) return;

}



void write_commap(u32 shm_id, int commap_id, int sem_exec_id, int sem_iteration_id)
{
   char path_buf[0x100];
   char *pwd = getcwd(NULL, 0);

   sprintf(path_buf, "%s/.mmap_id", pwd);

   FILE *pf = fopen(path_buf, "w");

   syslog(LOG_INFO, "[*] write %s", path_buf);

   if (pf != NULL)
   {
      fprintf(pf, "%d %d %d %d\n", shm_id, commap_id, sem_exec_id, sem_iteration_id);
   }
   fclose(pf);
}


void __afl_map_shm(struct Fstate *fstate)
{
  char *id_str = getenv(SHM_ENV_VAR);

  if (id_str)
  {
    u32 shm_id = atoi(id_str);
    int *ptr_id = portable_shmat(COMMAP_SIZE, NULL, 1, shm_id);

    syslog(LOG_INFO, "[*] AFL shm_id: %d %p", shm_id, ptr_id);

    int commap_id = -1;
    fstate->commap = portable_shmat(COMMAP_SIZE, &commap_id, 0, 0);

    if (commap_id == -1) {
       syslog(LOG_INFO, "[!] failed create commap_id: %p", (void*)fstate->commap);
       exit(1);
    }
    memset((void *)fstate->commap, '\0', COMMAP_SIZE);

    syslog(LOG_INFO, "[*] commap_id: %d", commap_id);

    strncpy((char *)fstate->commap->sem_name, id_str, SIZE_SEMNAME);
    fstate->commap->sem_name[SIZE_SEMNAME] = 0;

    fstate->commap->crash_found = 23;

    int sem_exec_id = -1;
    int sem_iteration_id = -1;


    exec_sem = portable_shmat(sizeof(sem_t), &sem_exec_id, 0, 0);
    iteration_sem = portable_shmat(sizeof(sem_t), &sem_iteration_id, 0, 0);

    syslog(LOG_INFO, "[*] address_id: %p %p", exec_sem, iteration_sem);
    syslog(LOG_INFO, "[*] semku_id: %d %d", sem_exec_id, sem_iteration_id);

    sem_init(exec_sem, 1, 0);
    sem_init(iteration_sem, 1, 0);

    write_commap(shm_id, commap_id, sem_exec_id, sem_iteration_id);
  }
  else {
    printf("[!] AFL shm_id not found\n");
    exit(1);
  }
}


static void __afl_start_forkserver(void) {

  u8  tmp[4] = {0, 0, 0, 0};
  u32 status = 0;

  if (__afl_map_size <= FS_OPT_MAX_MAPSIZE)
    status |= (FS_OPT_SET_MAPSIZE(__afl_map_size) | FS_OPT_MAPSIZE);

  if (status)
    status |= (FS_OPT_ENABLED);

  memcpy(tmp, &status, 4);

  // Phone home and tell the parent that we're OK.

  if (write(FORKSRV_FD + 1, tmp, 4) != 4) return;

}


static uint32_t __afl_next_testcase(uint8_t *buf, uint32_t max_len) {
    int32_t status = 0;
    int32_t res = 1;

    // Wait for parent by reading from the pipe. Abort if read fails.
    if (read(FORKSRV_FD, &status, 4) != 4) return 0;

    // afl only writes the test case to stdout when the cmdline does not contain "@@"
    status = read(0, buf, max_len);

    // Report that we are starting the target
    if (write(FORKSRV_FD + 1, &res, 4) != 4) return 0;

    return status;
}


static void __afl_end_testcase(int status)
{
  //int status = 0xffffff;

  if (write(FORKSRV_FD + 1, &status, 4) != 4) exit(1);
}


int main()
{
  u8  buf[1024];
  s32 len;

  struct Fstate fstate;

  __afl_map_size = MAP_SIZE;  // default is 65536
  __afl_map_shm(&fstate);
  __afl_start_forkserver();


  fstate.commap->crash_found = 0;

  sem_wait(exec_sem);

  int crash_count = 0;
  int ret_status;

  while ((len = __afl_next_testcase(buf, sizeof(buf))) >= 0)
  {
    if (len > 4)
    {
      memcpy(fstate.commap->payload, buf, len);
      fstate.commap->payload[len] = 0x00;
      fstate.commap->payload_len = len;

      if (fstate.commap->crash_found == 1)
      {
         ret_status = SIGSEGV;

         fstate.commap->crash_found = 0;
         syslog(LOG_INFO, "[*] write crash data: %d %s", crash_count, buf);
         crash_count += 1;
      }
      else {
         ret_status = 0;
      }
      sem_wait(iteration_sem);

    }

     __afl_end_testcase(ret_status);

  }

  return 0;

}

