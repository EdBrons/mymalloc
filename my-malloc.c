#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#define MIN_ALLOCATION 10000

struct metadata Head;
struct metadata Tail;
void *Heap_Top_Addr;

void print_address(void *p) {
    char buf[100];
    sprintf(buf, "%p\n", p);
    write(1, buf, strlen(buf));
}

struct metadata {
    // addr of the data segment
    void *data_addr;
    // len of the data segment
    size_t data_len;

    // addr of prev metadata
    struct metadata *prev;
    // addr of the next metadata
    struct metadata *next;
};

char* data_end_addr(struct metadata *mdp) {
    return (char*)mdp->data_addr + mdp->data_len;
}

void *align(void * ptr) {
    // return (void *)(((unsigned long)ptr & 0xFFFFFFFFFFF0) + 16);
    return (void*)((intptr_t)ptr + (16 - ((intptr_t)ptr % 16)));
}

// TODO: account for byte alignment
int size_available(struct metadata *mdp1, struct metadata *mdp2) {
    return (char *)mdp2 - data_end_addr(mdp1);
}

struct metadata *get_metadata(void *ptr) {
    struct metadata *metadata_p = &Head;
    while (metadata_p != NULL) {
        if (metadata_p->data_addr == ptr) {
            return metadata_p;
        }
        metadata_p = metadata_p->next;
    }
    return NULL;
}

void *malloc(size_t size) {
    static int first_run_flag = 1;

    // if head not created, create it
    if (first_run_flag) {
        void *heap_bottom_addr = sbrk(0);

        Head.data_addr = heap_bottom_addr;
        Head.data_len = 0;
        Head.prev = NULL;
        Head.next = NULL;

        intptr_t allocation = MIN_ALLOCATION > size ? MIN_ALLOCATION : size;
        allocation += 2 * sizeof(struct metadata);

        // TODO: rework this error checking because perror might not work
        if ((ssize_t)sbrk(allocation) < 0) {
            perror("malloc");
            return NULL;
        }

        Heap_Top_Addr = sbrk(0);

        first_run_flag = 0;
    }

    // traverse list until suitable spot found
    struct metadata *metadata_p = &Head;
    while (metadata_p->next != NULL && size_available(metadata_p, metadata_p->next) < size + sizeof(struct metadata) + 32) {
        metadata_p = metadata_p->next;
    }


    // check to see if we have enough space
    int space_needed = size + sizeof(struct metadata) + 32;
    if ((char *)Heap_Top_Addr - (space_needed + data_end_addr(metadata_p)) <= 0) {
        int allocation = MIN_ALLOCATION > space_needed ? MIN_ALLOCATION : space_needed;
        if ((ssize_t)sbrk(allocation) < 0) {
            perror("malloc");
            return NULL;
        }
        Heap_Top_Addr = sbrk(0);
    }

    // address of the new metadata
    void *metadata_addr;
    metadata_addr = align(metadata_p->data_addr + metadata_p->data_len);

    // address of the data for this metadata
    struct metadata md;
    md.data_addr = align((unsigned long)metadata_addr + sizeof(struct metadata));
    md.data_len = size;

    md.prev = metadata_p;
    md.next = metadata_p->next;
    if (metadata_p->next != NULL) {
        metadata_p->next->prev = metadata_addr;
    }
    metadata_p->next = metadata_addr;

    memcpy(metadata_addr, &md, sizeof(struct metadata));

    if ((intptr_t)md.data_addr % 16 != 0) {
        // print_address(md.data_addr);
    }

    return md.data_addr;
}

// TODO: maybe align metadata somehow so we can find it consistenly without looping
void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    struct metadata *mdp = get_metadata(ptr);
    if (mdp == NULL) {
        return;
    }

    // update linked list
    if (mdp->next != NULL) {
        mdp->next->prev = mdp->prev;
    }
    mdp->prev->next = mdp->next;
}

void *calloc(size_t nmemb, size_t size) {
    // check for overflow
    if (INT_MAX / nmemb < size) {
        // TODO: change out perror
        fprintf(stderr, "calloc: integer overflow\n");
        return NULL;
    }

    void * ptr = malloc(nmemb * size);
    memset(ptr, 0, nmemb * size);
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }

    struct metadata *mdp = get_metadata(ptr);

    if (mdp == NULL && size == 0) {
        return NULL;
    }
    else if (mdp == NULL && size != 0) {
        return malloc(size);
    }
    else if (mdp != NULL && size == 0) {
        free(ptr);
        return NULL;
    }

    // TODO: this implemtation is slighly inefficent
    void * new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    memcpy(new_ptr, ptr, mdp->data_len);
    free(ptr);

    return new_ptr;
}
