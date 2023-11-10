#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <hash.h>
#include <list.h>
#include <stdbool.h>

/* Physical frame. */
struct frame
{
  void *kpage; /* Address to a page from user pool. */

  struct list mappings;  /* List of mappings. */
  struct list_elem elem; /* Element for frame table. */
};

void frame_init (struct frame *);

void frame_init (struct frame *, void *);
bool frame_add_upage_mapping (struct frame *, void *);

#endif
