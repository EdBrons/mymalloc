#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#define MIN_ALLOCATION 10000

struct metadata Head; /*Keep track of the head of the linked list for all memory in use */
struct metadata Tail; /* Keep track of the end of allocated memory for the linked list*/
void *Heap_Top_Addr;  /* Keep track of the top of the heap (the boundary of used and unused memory) */

/* Code for debugging the data strucure (correct alignment and allocation of memory) 
   It takes a memory address pointer and prints the address */
void print_address(void *p) {
    char buf[100];
    sprintf(buf, "%p\n", p);
    write(1, buf, strlen(buf));
}


struct metadata {
    void *data_addr; /* address of the data */
    size_t data_len; /* length of the data */
    struct metadata *prev;
    struct metadata *next;
};

 /*This function takes a pointer to the head of a metadata and 
   returns its last address after adding the length of its data*/


char* data_end_addr(struct metadata *mdp) {
    return (char*)mdp->data_addr + mdp->data_len;
}


 /*This function takes a pointer to an address as input and returns
  a pointer to an address that is 16-bit aligned  */

void *align(void * ptr) {
    return (void*)((char *)ptr + (16 - ((unsigned long)ptr % 16)));
}


/* This function takes two adjacent metadata in the linked list and calculate the
   space that is free between them  */
int size_available(struct metadata *mdp1, struct metadata *mdp2) {
    return (char *)mdp2 - data_end_addr(mdp1);
}
/* This function takes a pointer to an address as input, searches
   for that pointer in the linked list and returns the address of the 
    metadata stored at that pointer if found, it returns NULL if the address
    is not found in the linked list  */

struct metadata *get_metadata(void *ptr) {
    struct metadata *metadata_p = &Head;
    while (metadata_p != NULL) { // traverse the linked list till we find the address
        if (metadata_p->data_addr == ptr) {
            return metadata_p;
        }
        metadata_p = metadata_p->next;
    }
    return NULL;
}

/* This function takes the size of memory the user needs as input, and 
 returns a chunk of memory that can be partition in the form of a linked 
 list for the user to use. Malloc provides a minum allocation size of 10000
 bytes if the user asks for less than that.  */
void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    // keep track of head
    static int first_run_flag = 1;

    // if head not created, create it
    if (first_run_flag) {
        void *heap_bottom_addr = sbrk(0);

        Head.data_addr = heap_bottom_addr;
        Head.data_len = 0;
        Head.prev = NULL;
        Head.next = NULL;

        /* if the input size is less than 10000 bytes, allocate
         10000 bytes  */
        intptr_t allocation = MIN_ALLOCATION > size ? MIN_ALLOCATION : size;
        allocation += 2 * sizeof(struct metadata);

        if ((ssize_t)sbrk(allocation) < 0) {
            return NULL;
        }

        Heap_Top_Addr = sbrk(0); /*update the top of the heap */

        first_run_flag = 0; // change to 0 if head of linked list is created
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

        if ((ssize_t)sbrk(allocation) < 0) { // use sbrk to a grab more memory (allocation)
            perror("malloc");

        // if sbrk returns -1, we ran out of memory
        if ((ssize_t)sbrk(allocation) < 0) {

            return NULL;
        }
        Heap_Top_Addr = sbrk(0); /*update the top of the heap */
    }

    // address of the new metadata
    void *metadata_addr;
    metadata_addr = align((char*)metadata_p->data_addr + metadata_p->data_len);

    // address of the data for this metadata
    struct metadata md;
    // align the address 16 bit alignment 
    md.data_addr = align((char *)metadata_addr + sizeof(struct metadata));
    md.data_len = size;
    // add the new metadata to the linkedlist 
    md.prev = metadata_p;
    md.next = metadata_p->next;
    if (metadata_p->next != NULL) {
        metadata_p->next->prev = metadata_addr;
    }
    metadata_p->next = metadata_addr;

    memcpy(metadata_addr, &md, sizeof(struct metadata));

    return md.data_addr;
}

 /* This function takes a pointer to an address and removes the address from the 
    linked list to make it available for storage of other things when needed */
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
/* This function allocates memory for an array of nmemb elements of size bytes each
    and returns a pointer to the allocated memory.*/ 
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
/* This function changes the size of the memory block pointed to by ptr to size bytes.
   The contents will be unchanged in the range from the start of the region up  to  the
   minimum  of  the  old  and new sizes.  
   If the new size is larger than the old size, the added memory will not be initialized.*/
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
