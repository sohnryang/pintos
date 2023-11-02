#include "vm/fault.h"

#include "lib/debug.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"

#include <hash.h>
#include <stdbool.h>
#include <stdlib.h>

/* Handle page faults caused by non-present page access. */
bool
fault_handle_not_present (void *fault_addr)
{
  void *kpage, *upage;
  struct frame *frame;
  struct thread *cur;
  bool writable;

  cur = thread_current ();

  kpage = palloc_get_page (PAL_USER);
  if (kpage == NULL) // TODO: implement swapping
    return false;

  frame = malloc (sizeof (struct frame));
  if (frame == NULL) // TODO: implement swapping
    return false;

  frame_init (frame, kpage);

  upage = pg_round_down (fault_addr);
  frame_add_upage_mapping (frame, upage);
  hash_insert (&cur->frames, &frame->elem);

  writable = pagedir_is_writable (cur->pagedir, upage);
  pagedir_set_page (cur->pagedir, upage, kpage, writable);

  return true;
}
