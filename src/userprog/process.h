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

struct process_context *process_child_ctx_by_pid (tid_t);
struct fd_context *process_get_fd_ctx (int);

#endif /* userprog/process.h */
