#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "vm/frame.h"

void swap_init (void);

void swap_register_frame (struct frame *);
void swap_unregister_frame (struct frame *);

struct frame *swap_find_victim (void);
void swap_write_frame (struct frame *);
void swap_read_frame (struct frame *);
void swap_free_frame (struct frame *);

#endif
