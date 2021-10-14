#include "my-malloc.c"
#include <string.h>

int main(int argc, char *argv[])
{
  void * ptr1 = my_malloc(10);
  memset(ptr1, 0, 10);
  void * ptr2 = my_malloc(10);
  memset(ptr2, 0, 10);
  void * ptr3 = my_malloc(10);
  memset(ptr3, 0, 10);
}
