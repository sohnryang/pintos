#include "vm/frame.h"

#include "debug.h"
#include "threads/malloc.h"

#include <list.h>
#include <stdbool.h>

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
