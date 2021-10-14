#include "my-malloc.c"
#include <string.h>

int main(int argc, char *argv[])
{
  void * ptr1 = my_malloc(10);
  int x = 100000;
  void * ptr2 = my_malloc(x);
  memset(ptr2, 0, x);
}
