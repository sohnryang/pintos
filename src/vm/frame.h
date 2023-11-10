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
  struct hash_elem elem; /* Element for hash table. */
};

hash_hash_func frame_hash;
hash_less_func frame_less;
hash_action_func frame_destruct;

void frame_init (struct frame *, void *);
bool frame_add_upage_mapping (struct frame *, void *);

#endif
