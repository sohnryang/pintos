#include "vm/vmm.h"

#include "stddef.h"
#include "threads/thread.h"
#include "vm/mmap.h"

#include <hash.h>
#include <list.h>
#include <stdbool.h>
#include <string.h>

/* Initialize virtual memory manager. */
bool
vmm_init (void)
{
  struct thread *cur;

  cur = thread_current ();

  list_init (&cur->frames);
  return hash_init (&cur->mmaps, mmap_info_hash, mmap_info_less, NULL);
}

/* Destroy VMM related data structures. */
void
vmm_destroy (void)
{
  struct thread *cur;

  cur = thread_current ();

  hash_destroy (&cur->mmaps, mmap_info_destruct);
}
