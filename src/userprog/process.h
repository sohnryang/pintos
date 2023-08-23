#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include <stdbool.h>
#include <stdint.h>

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void process_cleanup_ctx (struct process_context *);

struct process_context *child_ctx_by_pid (tid_t pid);

int copy_byte_from_user (const uint8_t *);
bool copy_byte_to_user (uint8_t *, uint8_t);

#endif /* userprog/process.h */
