#include "vm/mmap.h"

#include "debug.h"
#include "threads/malloc.h"

#include <hash.h>
#include <list.h>
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

  info = hash_entry (el, struct mmap_info, map_elem);
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
  info->offset = 0;
  info->mapped_size = 0;
}

/* Initialize `mmap_info` object for file-backed mapping. */
void
mmap_init_file_map (struct mmap_info *info, void *upage, struct file *file,
                    bool writable, off_t offset, uint32_t size)
{
  info->upage = upage;
  info->file = file;
  info->writable = writable;
  info->offset = offset;
  info->mapped_size = size;
}
