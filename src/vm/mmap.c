#include "vm/mmap.h"

#include "debug.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"

#include <hash.h>
#include <list.h>
#include <stdbool.h>
#include <stdint.h>

/* Hash function for `mmap_info`. */
unsigned
mmap_info_hash (const struct hash_elem *el, void *aux UNUSED)
{
  const struct mmap_info *info;

  info = hash_entry (el, struct mmap_info, map_elem);
  return hash_bytes (&info->upage, sizeof (info->upage));
}

/* Less function for `mmap_info`. */
bool
mmap_info_less (const struct hash_elem *a, const struct hash_elem *b,
                void *aux UNUSED)
{
  const struct mmap_info *info_a, *info_b;

  info_a = hash_entry (a, struct mmap_info, map_elem);
  info_b = hash_entry (b, struct mmap_info, map_elem);
  return info_a->upage < info_b->upage;
}

/* Destructor for `mmap_info`. */
void
mmap_info_destruct (struct hash_elem *el, void *aux UNUSED)
{
  struct mmap_info *info;
  struct thread *cur;

  cur = thread_current ();
  info = hash_entry (el, struct mmap_info, map_elem);

  pagedir_clear_page (cur->pagedir, info->upage);
  list_remove (&info->elem);
  free (info);
}

/* Initialize `mmap_info` object for anonymous mapping. */
void
mmap_init_anonymous (struct mmap_info *info, void *upage, bool writable)
{
  info->upage = upage;
  info->file = NULL;
  info->writable = writable;
  info->exe_mapping = false;
  info->offset = 0;
  info->mapped_size = 0;
}

/* Initialize `mmap_info` object for file-backed mapping. */
void
mmap_init_file_map (struct mmap_info *info, void *upage, struct file *file,
                    bool writable, bool exe_mapping, off_t offset,
                    uint32_t size)
{
  info->upage = upage;
  info->file = file;
  info->writable = writable;
  info->exe_mapping = exe_mapping;
  info->offset = offset;
  info->mapped_size = size;
}

/* Initialize `mmap_user_block` object. */
void
mmap_init_user_block (struct mmap_user_block *block, mapid_t id,
                      struct file *file)
{
  block->id = id;
  block->file = file;
  list_init (&block->chunks);
}

bool
mmap_user_block_compare_id (const struct list_elem *a,
                            const struct list_elem *b, void *aux UNUSED)
{
  const struct mmap_user_block *block_a, *block_b;
  block_a = list_entry (a, struct mmap_user_block, elem);
  block_b = list_entry (b, struct mmap_user_block, elem);
  return block_a->id < block_b->id;
}
