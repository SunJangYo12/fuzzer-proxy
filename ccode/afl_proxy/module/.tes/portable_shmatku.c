#include <stdio.h>

extern void *portable_shmat(size_t size, int *ret_id, int mode, int mode_id);

int main()
{

   int commap_id = -1;
   portable_shmat(0x2000, &commap_id, 0, 0);

   printf("zzzz: %d\n", commap_id);
   printf("zzzz: %p\n", portable_shmat(0x2000, NULL, 1, commap_id));
}
