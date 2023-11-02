#include "vm/frame.h"

#include "debug.h"
#include "threads/malloc.h"

#include <hash.h>
#include <list.h>
#include <stdbool.h>

/* Hash function for frame. */
unsigned
frame_hash (const struct hash_elem *el, void *aux UNUSED)
{
  const struct frame *frame;

  frame = hash_entry (el, struct frame, elem);
  return hash_bytes (&frame->paddr, sizeof (frame->paddr));
}

/* Hash function for frame. */
bool
frame_less (const struct hash_elem *a, const struct hash_elem *b,
            void *aux UNUSED)
{
  const struct frame *frame_a, *frame_b;

  frame_a = hash_entry (a, struct frame, elem);
  frame_b = hash_entry (b, struct frame, elem);
  return frame_a->paddr < frame_b->paddr;
}

/* Destructor for frame. */
void
frame_destruct (struct hash_elem *el, void *aux UNUSED)
{
  struct frame *frame;

  frame = hash_entry (el, struct frame, elem);
  free (frame);
}

/* Initialize `frame` as page frame at `paddr`. */
void
frame_init (struct frame *frame, void *paddr)
{
  frame->paddr = paddr;
  list_init (&frame->mappings);
}

/* Add a mapping for user page to specified frame. Returns true if success,
   false on failure. */
bool
frame_add_upage_mapping (struct frame *frame, void *upage)
{
  struct mapping *mapping;

  mapping = malloc (sizeof (struct mapping));
  if (mapping == NULL)
    return false;

  mapping->upage = upage;
  list_push_back (&frame->mappings, &mapping->elem);

  return true;
}
