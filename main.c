#include "my-malloc.c"
#include <string.h>

int main(int argc, char *argv[])
{
  void * ptr = my_malloc(100);
  my_realloc(ptr, 1000);
  my_free(ptr);
}
