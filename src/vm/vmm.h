#ifndef VM_VMM_H
#define VM_VMM_H

#include "filesys/file.h"
#include "filesys/off_t.h"
#include "user/syscall.h"
#include "vm/frame.h"
#include "vm/mmap.h"

#include <stdbool.h>
#include <stdint.h>

bool vmm_init (void);
void vmm_destroy (void);

bool vmm_map_to_new_frame (struct mmap_info *);
struct mmap_info *vmm_create_anonymous (void *, bool);
struct mmap_info *vmm_create_file_map (void *, struct file *, bool, bool, off_t,
                                       uint32_t);
void vmm_remove_mapping (struct mmap_info *);

struct frame *vmm_lookup_frame (void *);
bool vmm_activate_frame (struct frame *, void *);
void vmm_deactivate_frame (struct frame *);

bool vmm_handle_not_present (void *);
bool vmm_grow_stack (void *, void *);

mapid_t vmm_get_free_mapid (void);
struct mmap_user_block *vmm_get_mmap_user_block (mapid_t);
bool vmm_setup_user_block (struct mmap_user_block *, void *);
void vmm_cleanup_user_block (struct mmap_user_block *);

#endif
