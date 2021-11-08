#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#define MIN_ALLOCATION 10000

struct metadata Head;
void *Heap_Top_Addr;

struct metadata {
    void *data_addr; /* address of the data */
    size_t data_len; /* length of the data */
    struct metadata *prev;
    struct metadata *next;
};

// gets the end of the data segment
char* data_end_addr(struct metadata *mdp) {
    return (char*)mdp->data_addr + mdp->data_len;
}

// returns the ptr passed aligned to 16 bytes.
void *align(void * ptr) {
    return (void*)((char *)ptr + (16 - ((unsigned long)ptr % 16)));
}

// returns the size available for a allocation between two metadata chunks
int size_available(struct metadata *mdp1, struct metadata *mdp2) {
    return (char *)mdp2 - data_end_addr(mdp1);
}

// gets the metadata struct corresponding to ptr from the linked list
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
    if (size == 0) {
        return NULL;
    }

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

        if ((ssize_t)sbrk(allocation) < 0) {
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
        // if sbrk returns -1, we ran out of memory
        if ((ssize_t)sbrk(allocation) < 0) {
            return NULL;
        }
        Heap_Top_Addr = sbrk(0);
    }

    // address of the new metadata
    void *metadata_addr;
    metadata_addr = align((char*)metadata_p->data_addr + metadata_p->data_len);

    // address of the data for this metadata
    struct metadata md;
    md.data_addr = align((char *)metadata_addr + sizeof(struct metadata));
    md.data_len = size;

    md.prev = metadata_p;
    md.next = metadata_p->next;
    if (metadata_p->next != NULL) {
        metadata_p->next->prev = metadata_addr;
    }
    metadata_p->next = metadata_addr;

    memcpy(metadata_addr, &md, sizeof(struct metadata));

    return md.data_addr;
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    struct metadata *mdp = (struct metadata *)((char* )ptr - 0x30);

    // update linked list
    if (mdp->next != NULL) {
        mdp->next->prev = mdp->prev;
    }
    mdp->prev->next = mdp->next;
}

void *calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return NULL;
    }

    // check for overflow
    if (INT_MAX / nmemb < size) {
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

    if (mdp == NULL) {
        return NULL;
    }
    else if (size == 0) {
        free(ptr);
        return NULL;
    }

    // realloc the ptr
    void * new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    // check for bounds
    memcpy(new_ptr, ptr, mdp->data_len > size ? size : mdp->data_len);
    free(ptr);

    return new_ptr;
}

size_t malloc_usable_size(void *ptr) {
    struct metadata *mdp = get_metadata(ptr);
    if (mdp == NULL) {
        return 0;
    }
    return mdp->data_len;
}
