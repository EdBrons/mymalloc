#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define MIN_ALLOCATION 1000

struct metadata {
  // addr of the data segment
  void *data_addr;
  // len of the data segment
  size_t data_len;

  // addr of prev metadata
  void *prev;
  // addr of the next metadata
  void *next;

  // allocated segment length
  size_t segment_len;
};

// TODO: account for byte alignment
int size_available(struct metadata *mdp1, struct metadata *mdp2) {
  return (char *)mdp2 - (char *)mdp1 - sizeof(struct metadata);
}

void *my_malloc(size_t size) {
  static int first_run_flag = 1;
  static struct metadata head;

  // if head not created, create it
  if (first_run_flag) {
    void *heap_bottom_addr = sbrk(0);

    head.data_addr = heap_bottom_addr;
    head.data_len = 0;
    head.prev = NULL;
    head.next = NULL;

    intptr_t allocation = MIN_ALLOCATION > size ? MIN_ALLOCATION : size;
    allocation += 2 * sizeof(struct metadata);

    void *heap_top_addr;
    // TODO: rework this error checking because perror might not work
    if (sbrk(allocation) == -1) {
      perror("malloc");
      return NULL;
    }

    heap_top_addr = sbrk(0);
    head.segment_len = heap_top_addr - heap_bottom_addr;

    first_run_flag = 0;
  }

  // traverse list until suitable spot found
  struct metadata *metadata_p = &head;
  while ((metadata_p == &head && metadata_p->next != NULL) || size_available(metadata_p, metadata_p->next) < size + sizeof(struct metadata))
    metadata_p = metadata_p->next;

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
  md.segment_len = 0;

  memcpy(metadata_addr, &md, sizeof(struct metadata));

  return md.data_addr;
}
