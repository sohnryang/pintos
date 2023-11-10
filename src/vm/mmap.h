#ifndef VM_MMAP_H
#define VM_MMAP_H

#include "filesys/file.h"
#include "filesys/off_t.h"
#include "vm/frame.h"

#include <hash.h>
#include <list.h>
#include <stdbool.h>
#include <stdint.h>

/* Struct describing memory mapped object. */
struct mmap_info
{
  void *upage;       /* User page the file is mapped to. */
  struct file *file; /* Pointer to mapped file. Set to NULL if the mapping is
                        anonymous. */

  bool writable;        /* Whether the mapping is writable. */
  off_t offset;         /* Offset of mapped file. */
  uint32_t mapped_size; /* Size of mapped data. */

  struct list_elem elem; /* Element for linked list in frame. */
  struct frame *frame;   /* Pointer to frame object. */

  struct hash_elem map_elem; /* Element for mapping table. */
};

hash_hash_func mmap_info_hash;
hash_less_func mmap_info_less;
hash_action_func mmap_info_destruct;

void mmap_init_anonymous (struct mmap_info *, void *, bool);
void mmap_init_file_map (struct mmap_info *, void *, struct file *, bool, off_t,
                         uint32_t);

#endif
