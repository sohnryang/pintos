#include "vm/frame.h"

#include "debug.h"
#include "threads/malloc.h"

#include <list.h>
#include <stdbool.h>

/* Initialize `frame` as page frame at `paddr`. */
void
frame_init (struct frame *frame)
{
  frame->kpage = NULL;
  frame->is_stub = true;
  frame->is_swapped_out = false;
  list_init (&frame->mappings);
}
