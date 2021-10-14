#include "my-malloc.c"
#include <string.h>

int main(int argc, char *argv[])
{
  void * ptr = my_malloc(10);
  memset(ptr, 0, 10);
}
