#include "vm/swap.h"

#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "vm/frame.h"

#include <bitmap.h>
#include <debug.h>
#include <list.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

static bool swap_present;
static struct lock swap_lock;
static struct list active_frames;
static struct block *swap_block_dev;
static struct bitmap *swap_block_map;

/* Initialize swap manager. */
void
swap_init (void)
{
  block_sector_t swap_size;

  lock_init (&swap_lock);
  list_init (&active_frames);
  swap_present = false;

  swap_block_dev = block_get_role (BLOCK_SWAP);
  if (swap_block_dev == NULL)
    return;
  swap_present = true;

  swap_size = block_size (swap_block_dev);
  swap_block_map = bitmap_create (swap_size);
}

/* Register frame to swap manager. */
void
swap_register_frame (struct frame *frame)
{
  lock_acquire (&swap_lock);

  list_push_back (&active_frames, &frame->global_elem);

  lock_release (&swap_lock);
}

/* Unregister frame from swap manager. */
void
swap_unregister_frame (struct frame *frame)
{
  lock_acquire (&swap_lock);

  list_remove (&frame->global_elem);

  lock_release (&swap_lock);
}

/* Find victim frame. */
struct frame *
swap_find_victim (void)
{
  struct list_elem *el;

  ASSERT (swap_present);

  lock_acquire (&swap_lock);

  if (list_empty (&active_frames))
    return NULL;

  // TODO: implement smarter policy
  el = list_back (&active_frames);

  lock_release (&swap_lock);

  return list_entry (el, struct frame, global_elem);
}

/* Write frame to swap space. */
void
swap_write_frame (struct frame *frame)
{
  size_t sector;
  int i;

  ASSERT (swap_present);

  lock_acquire (&swap_lock);

  sector = bitmap_scan_and_flip (swap_block_map, 0, SECTORS_PER_PAGE, false);
  ASSERT (sector != BITMAP_ERROR);
  frame->swap_sector = sector;

  for (i = 0; i < SECTORS_PER_PAGE; i++)
    block_write (swap_block_dev, sector + i,
                 (uint8_t *)frame->kpage + i * BLOCK_SECTOR_SIZE);

  lock_release (&swap_lock);
}

/* Read frame from swap space. */
void
swap_read_frame (struct frame *frame)
{
  int i;

  ASSERT (swap_present);

  lock_acquire (&swap_lock);

  ASSERT (
      bitmap_count (swap_block_map, frame->swap_sector, SECTORS_PER_PAGE, true)
      == SECTORS_PER_PAGE);
  for (i = 0; i < SECTORS_PER_PAGE; i++)
    block_read (swap_block_dev, frame->swap_sector + i,
                (uint8_t *)frame->kpage + i * BLOCK_SECTOR_SIZE);

  lock_release (&swap_lock);
}

/* Free frame from swap space. */
void
swap_free_frame (struct frame *frame)
{
  lock_acquire (&swap_lock);

  if (swap_present)
    bitmap_set_multiple (swap_block_map, frame->swap_sector, SECTORS_PER_PAGE,
                         false);
  frame->swap_sector = -1;

  lock_release (&swap_lock);
}
