#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <hash.h>
#include <list.h>
#include <stdbool.h>

/* Physical frame. */
struct frame
{
  void *kpage; /* Address to a page from user pool. */

  bool is_stub;        /* Is this frame a stub frame? */
  bool is_swapped_out; /* Is this frame swapped out? */

  struct list mappings;  /* List of mappings. */
  struct list_elem elem; /* Element for frame table. */
};

void frame_init (struct frame *);

#endif
