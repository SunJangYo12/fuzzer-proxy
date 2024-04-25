#ifdef __ANDROID__
  #include "android-ashmem.h"
#endif
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

#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>

#define COMMAP_SIZE 0x2000

u8 *__afl_area_ptr;
__thread u32 __afl_map_size = MAP_SIZE;


/* Error reporting to forkserver controller */

void send_forkserver_error(int error) {

  u32 status;
  if (!error || error > 0xffff) return;
  status = (FS_OPT_ERROR | FS_OPT_SET_ERROR(error));
  if (write(FORKSRV_FD + 1, (char *)&status, 4) != 4) return;

}



sem_t *open_sem(char *type, char *nprefix)
{
   char sem_name[64];
   sem_t *ret_sem = NULL;

   bzero(sem_name, 64);

   snprintf(sem_name, 64, "%s-%s", nprefix, type);
   sem_unlink(sem_name);

   ret_sem = sem_open(sem_name, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO, 0);
   if (ret_sem == SEM_FAILED)
   {
       syslog(LOG_INFO, "[*] sem_open (%s) failed errno=%s\n", type, strerror(errno));
       return NULL;
   }
   return ret_sem;
}

void afl_map_shm(struct Fstate *fstate)
{
  char *id_str = getenv(SHM_ENV_VAR);
  char *ptr;

  if (id_str)
  {
    syslog(LOG_INFO, "[*] use mmap...");

    const char    *shm_file_path = id_str;
    int            shm_fd = -1;
    unsigned char *shm_base = NULL;

    shm_fd = shm_open(shm_file_path, O_RDWR, 0600);

    if (shm_fd == -1) {
        syslog(LOG_INFO, "[!] shm_open() failed\n");
        send_forkserver_error(FS_ERROR_SHM_OPEN);
    }
    shm_base = mmap(0, __afl_map_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (shm_base == MAP_FAILED) {
        close(shm_fd);
        shm_fd = -1;

        syslog(LOG_INFO, "[!] mmap() failed\n");
        send_forkserver_error(FS_ERROR_MMAP);
    }

    __afl_area_ptr = shm_base;

    /* create comunication map to js*/
    int commap_id = shmget(IPC_PRIVATE, COMMAP_SIZE, IPC_CREAT | IPC_EXCL | 0644);


    char *shm_id = malloc(128);
    snprintf(shm_id, 128, "%d", commap_id);

    fstate->commap_id = shm_id;
    fstate->id_afl = id_str;
    syslog(LOG_INFO, "[*] commap_id = %s", fstate->commap_id);


    fstate->commap = shmat(commap_id, NULL, 0);
    bzero((void *)fstate->commap, COMMAP_SIZE);


    strncpy((char *)fstate->commap->sem_name, id_str, SIZE_SEMNAME);
    fstate->commap->sem_name[SIZE_SEMNAME] = 0;


    fstate->exec_sem = open_sem("exec", id_str);
    fstate->iteration_sem = open_sem("iter", id_str);

    if (fstate->exec_sem == NULL || fstate->iteration_sem == NULL) {
        syslog(LOG_INFO, "[!] exec_sem is NULL");
    }

    if (__afl_area_ptr == (void *)-1)
    {
      syslog(LOG_INFO, "[!] shmat failed");
      send_forkserver_error(FS_ERROR_SHMAT);
      exit(1);
    }

    syslog(LOG_INFO, "[*] afl_area_ptr = %p", __afl_area_ptr);
    fstate->afl_ptr = __afl_area_ptr;

    /* Write something into the bitmap so that the parent doesn't give up */
    __afl_area_ptr[0] = 132;
  }
}


static u32 __afl_next_testcase(u8 *buf, u32 max_len)
{
  s32 status, res = 0xffffff;
  if (read(FORKSRV_FD, &status, 4) != 4) return 0;

  /* we have a testcase - read it */
  status = read(0, buf, max_len);

  /* report that we are starting the target */
  if (write(FORKSRV_FD + 1, &res, 4) != 4) return 0;

  return status;
}

static void __afl_start_forkserver(void)
{
  u8  tmp[4] = {0, 0, 0, 0};
  u32 status = 0;

  if (__afl_map_size <= FS_OPT_MAX_MAPSIZE)
    status |= (FS_OPT_SET_MAPSIZE(__afl_map_size) | FS_OPT_MAPSIZE);
  if (status) status |= (FS_OPT_ENABLED);
  memcpy(tmp, &status, 4);

  if (write(FORKSRV_FD + 1, tmp, 4) != 4) return;
}
static void __afl_end_testcase(void)
{
  int status = 0xffffff;
  if (write(FORKSRV_FD + 1, &status, 4) != 4) exit(1);
}


int main()
{
  uint8_t buf[1024];
  int len;
  struct Fstate fstate;

  __afl_map_size = MAP_SIZE;  // default is 65536

  afl_map_shm(&fstate);

  __afl_start_forkserver();

  while ((len = __afl_next_testcase(buf, sizeof(buf))) > 0)
  {
    if (len > 4)
    {

      if (buf[0] == 0xff)
        __afl_area_ptr[1] = 1;
      else if (buf[0] == 0xf1)
        __afl_area_ptr[2] = 1;
      else
        __afl_area_ptr[3] = 1;



      memcpy(fstate.commap->payload, buf, len);
      fstate.commap->payload[len] = 0x00;
      fstate.commap->payload_len = len;

      if (sem_post(fstate.exec_sem) == -1) {
          syslog(LOG_INFO, "[!] error exec posting in semaphore = %s", strerror(errno));
      }

      if (sem_wait(fstate.iteration_sem) == -1) {
          syslog(LOG_INFO, "[!] error iteration waiting in semaphore = %s", strerror(errno));
      }
    }

    __afl_end_testcase();
  }
}
