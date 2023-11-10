#include "userprog/syscall.h"
#include <stdint.h>
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "list.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/checked_user_mem.h"
#include "userprog/process.h"

#define WRITE_BUFSIZE 128
#define READ_BUFSIZE 128

static void syscall_handler (struct intr_frame *);
static int syscall_halt (void *) NO_RETURN;
static int syscall_exit (void *) NO_RETURN;
static int syscall_exec (void *);
static int syscall_wait (void *);
static int syscall_create (void *);
static int syscall_remove (void *);
static int syscall_open (void *);
static int syscall_filesize (void *);
static int syscall_read (void *);
static int syscall_write (void *);
static int syscall_seek (void *);
static int syscall_tell (void *);
static int syscall_close (void *);

void *syscall_user_esp;

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall\n");
  syscall_user_esp = NULL;
}

#define pop_arg(TYPE, OUT, SP)                                                 \
  ({                                                                           \
    TYPE *arg_ptr;                                                             \
    void *dst;                                                                 \
    arg_ptr = (TYPE *)(SP);                                                    \
    dst = checked_memcpy_from_user (&OUT, arg_ptr, sizeof (TYPE));             \
    if (dst == NULL)                                                           \
      process_trigger_exit (-1);                                               \
    arg_ptr++;                                                                 \
    SP = arg_ptr;                                                              \
  })

/* Handle system call. */
static void
syscall_handler (struct intr_frame *f)
{
  int syscall_id;
  int (*syscall_table[13]) (void *) = {
    syscall_halt,   syscall_exit,   syscall_exec, syscall_wait,
    syscall_create, syscall_remove, syscall_open, syscall_filesize,
    syscall_read,   syscall_write,  syscall_seek, syscall_tell,
    syscall_close,
  };
  void *sp = f->esp;
  syscall_user_esp = f->esp;

  pop_arg (int, syscall_id, sp);

  f->eax = syscall_table[syscall_id](sp);
  syscall_user_esp = NULL;
}

/* System call handler for `HALT`. */
static int
syscall_halt (void *sp UNUSED)
{
  shutdown_power_off ();
}

/* System call handler for `EXIT`. */
static int
syscall_exit (void *sp)
{
  int status;

  pop_arg (int, status, sp);
  process_trigger_exit (status);
}

/* System call handler for `EXEC`. */
static int
syscall_exec (void *sp)
{
  const uint8_t *file;
  char *copied;
  tid_t child_pid;
  int res;
  struct process_context *child_ctx;

  pop_arg (const uint8_t *, file, sp);

  copied = palloc_get_page (PAL_ZERO);
  if (copied == NULL)
    return TID_ERROR;

  res = checked_strlcpy_from_user (copied, (void *)file, PGSIZE);
  if (res == -1)
    {
      palloc_free_page (copied);
      process_trigger_exit (-1);
    }

  child_pid = process_execute (copied);
  palloc_free_page (copied);

  child_ctx = process_child_ctx_by_pid (child_pid);
  if (child_ctx == NULL)
    return TID_ERROR;

  sema_down (&child_ctx->load_sema);
  if (!child_ctx->load_success)
    {
      process_cleanup_ctx (child_ctx);
      return TID_ERROR;
    }
  return child_pid;
}

/* System call handler for `WAIT`. */
static int
syscall_wait (void *sp)
{
  int pid; /* pid_t is typedef'd as int. */
  int status_code;

  pop_arg (int, pid, sp);
  status_code = process_wait (pid);
  return status_code;
}

/* System call handler for `CREATE`. */
static int
syscall_create (void *sp)
{
  const char *file;
  unsigned initial_size;
  char *copied_filename;
  int res;
  bool success;

  pop_arg (const char *, file, sp);
  pop_arg (unsigned, initial_size, sp);

  copied_filename = palloc_get_page (0);
  if (copied_filename == NULL)
    return false;

  res = checked_strlcpy_from_user (copied_filename, file, PGSIZE);
  if (res == -1)
    {
      palloc_free_page (copied_filename);
      process_trigger_exit (-1);
    }
  if (res == 0)
    {
      palloc_free_page (copied_filename);
      return false;
    }

  thread_acquire_fs_lock ();
  success = filesys_create (copied_filename, initial_size);
  thread_release_fs_lock ();

  palloc_free_page (copied_filename);
  return success;
}

/* System call handler for `REMOVE`. */
static int
syscall_remove (void *sp)
{
  const char *file;
  char *copied_filename;
  int res;
  bool success;

  pop_arg (const char *, file, sp);

  copied_filename = palloc_get_page (0);
  if (copied_filename == NULL)
    return false;

  res = checked_strlcpy_from_user (copied_filename, file, PGSIZE);
  if (res == -1)
    {
      palloc_free_page (copied_filename);
      process_trigger_exit (-1);
    }

  thread_acquire_fs_lock ();
  success = filesys_remove (copied_filename);
  thread_release_fs_lock ();

  palloc_free_page (copied_filename);
  return success;
}

/* System call handler for `OPEN`. */
static int
syscall_open (void *sp)
{
  const char *file;
  char *copied_filename;
  int res;
  struct fd_context *fd_ctx;

  pop_arg (const char *, file, sp);

  copied_filename = palloc_get_page (0);
  if (copied_filename == NULL)
    return -1;

  res = checked_strlcpy_from_user (copied_filename, file, PGSIZE);
  if (res == -1)
    {
      palloc_free_page (copied_filename);
      process_trigger_exit (-1);
    }

  fd_ctx = process_create_fd_ctx ();
  if (fd_ctx == NULL)
    {
      palloc_free_page (copied_filename);
      return -1;
    }

  thread_acquire_fs_lock ();
  fd_ctx->file = filesys_open (copied_filename);
  thread_release_fs_lock ();

  palloc_free_page (copied_filename);
  if (fd_ctx->file == NULL)
    return -1;

  return fd_ctx->fd;
}

/* System call handler for `FILESIZE`. */
static int
syscall_filesize (void *sp)
{
  int fd;
  struct fd_context *fd_ctx;

  pop_arg (int, fd, sp);

  off_t res;

  fd_ctx = process_get_fd_ctx (fd);
  if (fd_ctx == NULL)
    process_trigger_exit (-1);
  if (fd_ctx->file == NULL)
    process_trigger_exit (-1);

  thread_acquire_fs_lock ();
  res = file_length (fd_ctx->file);
  thread_release_fs_lock ();

  return res;
}

/* System call handler for `READ`. */
static int
syscall_read (void *sp)
{
  int fd;
  void *buffer;
  unsigned length;

  pop_arg (int, fd, sp);
  pop_arg (void *, buffer, sp);
  pop_arg (unsigned, length, sp);

  struct fd_context *fd_ctx;
  char copied_buf[READ_BUFSIZE];
  uint8_t key_in;
  bool success;
  unsigned read_len, copy_len;
  off_t file_read_len;
  void *dst;

  fd_ctx = process_get_fd_ctx (fd);
  if (fd_ctx == NULL)
    process_trigger_exit (-1);

  if (fd_ctx->screen_out)
    process_trigger_exit (-1);
  else if (fd_ctx->keyboard_in)
    {
      for (read_len = 0; read_len < length; read_len++)
        {
          key_in = input_getc ();
          success = checked_copy_byte_to_user (buffer + read_len, key_in);
          if (!success)
            process_trigger_exit (-1);
        }
      return length;
    }

  if (fd_ctx->file == NULL)
    process_trigger_exit (-1);

  file_read_len = 0;

  thread_acquire_fs_lock ();
  for (read_len = 0; read_len < length; read_len += READ_BUFSIZE)
    {
      copy_len
          = length - read_len < READ_BUFSIZE ? length - read_len : READ_BUFSIZE;
      file_read_len += file_read (fd_ctx->file, copied_buf, copy_len);
      dst = checked_memcpy_to_user (buffer + read_len, copied_buf, copy_len);
      if (dst == NULL)
        {
          thread_release_fs_lock ();
          process_trigger_exit (-1);
        }
    }
  thread_release_fs_lock ();

  return file_read_len;
}

/* System call handler for `WRITE`. */
static int
syscall_write (void *sp)
{
  int fd;
  void *buffer;
  unsigned length;

  pop_arg (int, fd, sp);
  pop_arg (void *, buffer, sp);
  pop_arg (unsigned, length, sp);

  struct fd_context *fd_ctx;
  char copied_buf[WRITE_BUFSIZE];
  unsigned written, copy_len;
  void *dst;

  fd_ctx = process_get_fd_ctx (fd);
  if (fd_ctx == NULL)
    process_trigger_exit (-1);

  if (fd_ctx->keyboard_in)
    process_trigger_exit (-1);
  else if (fd_ctx->screen_out)
    {
      for (written = 0; written < length; written += WRITE_BUFSIZE)
        {
          copy_len = length - written < WRITE_BUFSIZE ? length - written
                                                      : WRITE_BUFSIZE;
          dst = checked_memcpy_from_user (copied_buf, buffer + written,
                                          copy_len);
          if (dst == NULL)
            process_trigger_exit (-1);
          putbuf (copied_buf, copy_len);
        }
      return length;
    }

  if (fd_ctx->file == NULL)
    process_trigger_exit (-1);

  off_t file_written = 0;

  thread_acquire_fs_lock ();
  for (written = 0; written < length; written += WRITE_BUFSIZE)
    {
      copy_len
          = length - written < WRITE_BUFSIZE ? length - written : WRITE_BUFSIZE;
      dst = checked_memcpy_from_user (copied_buf, buffer + written, copy_len);
      if (dst == NULL)
        {
          thread_release_fs_lock ();
          process_trigger_exit (-1);
        }
      file_written += file_write (fd_ctx->file, copied_buf, copy_len);
    }
  thread_release_fs_lock ();
  return file_written;
}

/* System call handler for `SEEK`. */
static int
syscall_seek (void *sp)
{
  int fd;
  unsigned position;
  struct fd_context *fd_ctx;

  pop_arg (int, fd, sp);
  pop_arg (unsigned, position, sp);

  fd_ctx = process_get_fd_ctx (fd);
  if (fd_ctx == NULL || fd_ctx->file == NULL)
    process_trigger_exit (-1);

  thread_acquire_fs_lock ();
  file_seek (fd_ctx->file, position);
  thread_release_fs_lock ();

  return 0;
}

/* System call handler for `TELL`. */
static int
syscall_tell (void *sp)
{
  int fd;
  struct fd_context *fd_ctx;

  pop_arg (int, fd, sp);

  fd_ctx = process_get_fd_ctx (fd);
  if (fd_ctx == NULL || fd_ctx->file == NULL)
    process_trigger_exit (-1);

  thread_acquire_fs_lock ();
  file_tell (fd_ctx->file);
  thread_release_fs_lock ();

  return 0;
}

/* System call handler for `CLOSE`. */
static int
syscall_close (void *sp)
{
  int fd;
  struct fd_context *fd_ctx;

  pop_arg (int, fd, sp);

  fd_ctx = process_get_fd_ctx (fd);
  if (fd_ctx == NULL)
    return -1;

  thread_acquire_fs_lock ();
  file_close (fd_ctx->file);
  thread_release_fs_lock ();

  process_remove_fd_ctx (fd_ctx);
  return 0;
}
