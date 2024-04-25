#include <semaphore.h> //sem_t
#include <sys/stat.h> //S_IRWSU
#include <fcntl.h> //O_CREAT
#include <syslog.h>
#include <stdint.h>

#define COMMAP_SIZE 0x2000
#define SIZE_SEMNAME 22

/*
  create_sem = COMMAP_SIZE, &commap_id, 0, 0
  open_sem = COMMAP_SIZE, NULL, 1, 2323 <-shared_id
 */
extern void *portable_shmat(size_t size, int *ret_id, int mode, int mode_id);


sem_t *exec_sem;
sem_t *iteration_sem;

typedef struct _communication_map_t
{
  uint64_t crash_found;
  uint64_t payload_len;
  char sem_name[SIZE_SEMNAME + 4];
  char payload[];

} communication_map_t;

struct Fstate {
   communication_map_t *commap;
};






