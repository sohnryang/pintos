#include "vm/vmm.h"

#include "filesys/file.h"
#include "stddef.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "vm/mmap.h"

#include <hash.h>
#include <list.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Add a non-mapped user page `upage` to page table. */
static bool
install_page_stub (void *upage, bool writable)
{
  struct thread *cur = thread_current ();

  return (pagedir_get_page (cur->pagedir, upage) == NULL
          && pagedir_set_page_stub (cur->pagedir, upage, writable));
}

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

/* Create a new frame and map `info` to it. */
bool
vmm_map_to_new_frame (struct mmap_info *info)
{
  struct frame *frame;
  struct thread *cur;

  cur = thread_current ();
  if (hash_find (&cur->mmaps, &info->map_elem))
    return false;

  frame = malloc (sizeof (struct frame));
  if (frame == NULL)
    {
      free (info);
      return false;
    }
  frame_init (frame);

  list_push_back (&frame->mappings, &info->elem);
  info->frame = frame;
  list_push_back (&cur->frames, &frame->elem);
  hash_insert (&cur->mmaps, &info->map_elem);

  return install_page_stub (info->upage, info->writable);
}

/* Create an anonymous mapping for `upage`. */
bool
vmm_create_anonymous (void *upage, bool writable)
{
  struct mmap_info *info;

  ASSERT (pg_ofs (upage) == 0);

  info = malloc (sizeof (struct mmap_info));
  if (info == NULL)
    return false;
  mmap_init_anonymous (info, upage, writable);

  if (!vmm_map_to_new_frame (info))
    {
      free (info);
      return false;
    }
  return true;
}

/* Create a file mapping for `file` to `upage`. */
bool
vmm_create_file_map (void *upage, struct file *file, bool writable,
                     off_t offset, uint32_t size)
{
  struct mmap_info *info;

  ASSERT (pg_ofs (upage) == 0);

  info = malloc (sizeof (struct mmap_info));
  if (info == NULL)
    return false;
  mmap_init_file_map (info, upage, file, writable, offset, size);

  if (!vmm_map_to_new_frame (info))
    {
      free (info);
      return false;
    }
  return true;
}

/* Find page frame corresponding to `upage`. */
struct frame *
vmm_lookup_frame (void *upage)
{
  struct mmap_info info;
  struct hash_elem *el;
  struct thread *cur;

  info.upage = upage;

  cur = thread_current ();
  el = hash_find (&cur->mmaps, &info.map_elem);

  return el != NULL ? hash_entry (el, struct mmap_info, map_elem)->frame : NULL;
}
