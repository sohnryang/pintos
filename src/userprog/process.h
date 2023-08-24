#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void process_cleanup_ctx (struct process_context *);
void process_trigger_exit (int) NO_RETURN;

struct process_context *child_ctx_by_pid (tid_t pid);

bool is_valid_user_ptr (const void *);
bool is_contained_in_user (const void *, size_t);

int copy_byte_from_user (const uint8_t *);
bool copy_byte_to_user (uint8_t *, uint8_t);

void *memcpy_from_user (void *, const void *, size_t);
void *memcpy_to_user (void *, const void *, size_t);

#endif /* userprog/process.h */
