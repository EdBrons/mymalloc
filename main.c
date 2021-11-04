#include "my-malloc.c"
#include <string.h>

#define ALLOCATIONS 200

int main(int argc, char *argv[])
{
    void *ptrs[ALLOCATIONS];
    for (int i = 0; i < ALLOCATIONS; i++) {
        ptrs[i] = malloc(100 * i);
        memset(ptrs[i], 0, 100 *  i);
        char *s;
        sprintf(s, "%x\n", ptrs[i]);
        write(1, s, strlen(s));
    }

    for (int i = ALLOCATIONS-1; i >= 0; i--) {
        free(ptrs[i]);
    }
}
