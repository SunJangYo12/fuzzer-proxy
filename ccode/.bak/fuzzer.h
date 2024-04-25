#include <semaphore.h> //sem_t
#include <sys/stat.h> //S_IRWSU
#include <fcntl.h> //O_CREAT
#include <syslog.h>

#define SIZE_SEMNAME 22

typedef struct _communication_map_t
{
  uint64_t state_flag;
  uint64_t payload_len;
  char sem_name[SIZE_SEMNAME + 4];
  char payload[];

} communication_map_t;

struct Fstate {
   /*afl*/
   char *afl_ptr;
   char *commap_id;
   char *id_afl;
   communication_map_t *commap;
   sem_t *iteration_sem;
   sem_t *exec_sem;
};
