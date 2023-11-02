#ifndef VM_FRAME
#define VM_FRAME

#include <hash.h>
#include <list.h>
#include <stdbool.h>

/* Object mapped to page frame. */
struct mapping
{
  void *upage; /* Virtual address to user page. */

  struct list_elem elem; /* Element for linked list. */
};

/* Physical frame. */
struct frame
{
  void *paddr; /* Physical address of frame. */

  struct list mappings;  /* List of mappings. */
  struct hash_elem elem; /* Element for hash table. */
};

hash_hash_func frame_hash;
hash_less_func frame_less;
hash_action_func frame_destruct;

void frame_init (struct frame *, void *);
bool frame_add_upage_mapping (struct frame *, void *);

#endif