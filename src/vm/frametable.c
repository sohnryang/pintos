#include "vm/frametable.h"

#include "threads/thread.h"
#include "vm/frame.h"

#include <hash.h>

/* Initialize frame table of current process. Returns true on success, false on
   failure. */
bool
frametable_init (void)
{
  struct thread *cur;

  cur = thread_current ();

  return hash_init (&cur->frames, frame_hash, frame_less, NULL);
}

/* Destroy frame table of current process. */
void
frametable_destroy (void)
{
  struct thread *cur;

  cur = thread_current ();

  hash_destroy (&cur->frames, frame_destruct);
}
