#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define MIN_ALLOCATION 1000

struct metadata Head;
void *Heap_Top_Addr;

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
  return mdp->data_addr + mdp->data_len;
}

// TODO: account for byte alignment
int size_available(struct metadata *mdp1, struct metadata *mdp2) {
  return (char *)mdp2 - data_end_addr(mdp1);
}

struct metadata *get_metadata(void *ptr) {
  struct metadata *metadata_p = &Head;
  while (metadata_p->data_addr != ptr && metadata_p != NULL) {
    metadata_p = metadata_p->next;
  }
  return metadata_p;
}

void *my_malloc(size_t size) {
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
    if (sbrk(allocation) == -1) {
      perror("malloc");
      return NULL;
    }

    Heap_Top_Addr = sbrk(0);

    first_run_flag = 0;
  }

  // traverse list until suitable spot found
  struct metadata *metadata_p = &Head;
  while (metadata_p->next != NULL && size_available(metadata_p, metadata_p->next) < size + sizeof(struct metadata)) {
    metadata_p = metadata_p->next;
  }


  // check to see if we have enough space
  int space_needed = size + sizeof(struct metadata) + 16;
  if ((char *)Heap_Top_Addr - (space_needed + data_end_addr(metadata_p)) <= 0) {
    int allocation = MIN_ALLOCATION > space_needed ? MIN_ALLOCATION : space_needed;
    if (sbrk(allocation) == -1) {
      perror("malloc");
      return NULL;
    }
    Heap_Top_Addr = sbrk(0);
  }

  // address of the new metadata
  void *metadata_addr;
  metadata_addr = metadata_p->data_addr + metadata_p->data_len;

  struct metadata md;
  // address of the data for this metadata
  md.data_addr = ((unsigned long)metadata_addr + sizeof(struct metadata)) + 
    ((unsigned long)(metadata_addr + sizeof(struct metadata)) % 16);
  md.data_len = size;
  md.prev = metadata_p;
  metadata_p->next = metadata_addr;

  memcpy(metadata_addr, &md, sizeof(struct metadata));

  return md.data_addr;
}

// TODO: maybe align metadata somehow so we can find it consistenly without looping
void my_free(void *ptr) {
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

void *my_calloc(size_t nmemb, size_t size) {
  return 0;
}

void *my_realloc(void *ptr, size_t size) {
  return 0;
}
