#include "my-malloc.c"
#include <string.h>

#define ALLOCATIONS 200

int main(int argc, char *argv[])
{
    for (int a = 0; a < ALLOCATIONS; a++) {
        if (malloc(100) == NULL) {
            return -1;
        }
    }
}
