#include "vm/fault.h"

#include "lib/debug.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "vm/vmm.h"

#include <hash.h>
#include <stdbool.h>
#include <stdlib.h>

/* Handle page faults caused by non-present page access. */
bool
fault_handle_not_present (void *fault_addr)
{
  void *kpage, *upage;
  struct frame *frame;

  kpage = palloc_get_page (PAL_USER);
  if (kpage == NULL) // TODO: implement swapping
    return false;

  upage = pg_round_down (fault_addr);
  frame = vmm_lookup_frame (upage);
  if (frame == NULL)
    return false;
  if (!vmm_deserialize_frame (frame, kpage))
    return false;

  return true;
}
