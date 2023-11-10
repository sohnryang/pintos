#ifndef VM_VMM_H
#define VM_VMM_H

#include "filesys/file.h"
#include "filesys/off_t.h"
#include "vm/mmap.h"

#include <stdbool.h>
#include <stdint.h>

bool vmm_init (void);
void vmm_destroy (void);

bool vmm_map_to_new_frame (struct mmap_info *);
bool vmm_create_anonymous (void *, bool);
bool vmm_create_file_map (void *, struct file *, bool, off_t, uint32_t);

#endif
