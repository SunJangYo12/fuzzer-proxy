#include <stdio.h>
#include "../android_ashmem/shm.h"
#include <sys/stat.h> //S_IRWSU
#include <fcntl.h> //O_CREAT

int main()
{

   int shm_id = shmget(IPC_PRIVATE, 0x2000, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

   printf("zzz: %d\n", shm_id);
   printf("zzz: %p\n", shmat(shm_id, NULL, 0));
}
