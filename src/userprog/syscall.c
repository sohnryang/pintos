#include "userprog/syscall.h"
#include <stdint.h>
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/shutdown.h"
#include "list.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"

static void do_exit (int) NO_RETURN;
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

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall\n");
}

#define pop_arg(TYPE, OUT, SP) \
  ({                           \
    TYPE *arg_ptr;             \
    arg_ptr = (TYPE *)(SP);    \
    OUT = *arg_ptr;            \
    arg_ptr++;                 \
    SP = arg_ptr;              \
  })

static void
do_exit (int status)
{
  struct thread *cur;
  cur = thread_current ();
  cur->process_ctx->exit_code = status;
  sema_up (&cur->process_ctx->exit_sema);
  thread_exit ();
}

/* Handle system call. */
static void
syscall_handler (struct intr_frame *f)
{
  int syscall_id, *syscall_id_ptr;
  int (*syscall_table[13]) (void *) = {
    syscall_halt,
    syscall_exit,
    syscall_exec,
    syscall_wait,
    syscall_create,
    syscall_remove,
    syscall_open,
    syscall_filesize,
    syscall_read,
    syscall_write,
    syscall_seek,
    syscall_tell,
    syscall_close,
  };
  void *sp = f->esp;

  syscall_id_ptr = sp;
  syscall_id = *syscall_id_ptr;
  syscall_id_ptr++;
  sp = syscall_id_ptr;

  f->eax = syscall_table[syscall_id](sp);
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
  do_exit (status);
}

/* System call handler for `EXEC`. */
static int
syscall_exec (void *sp)
{
  const uint8_t *file;
  char *copied, *copied_current;
  int byte;
  tid_t child_pid;
  struct process_context *child_ctx;

  pop_arg (const uint8_t *, file, sp);

  copied = palloc_get_page (PAL_ZERO);
  copied_current = copied;
  do
    {
      /* Check if we are copying from userspace. */
      if ((void *)file >= PHYS_BASE)
        goto fail_during_copy;

      /* Copy a byte from userspace and check its validity. */
      byte = copy_byte_from_user (file);
      if (byte == -1)
        goto fail_during_copy;

      /* Copy the byte to the buffer. */
      *copied_current = (char)byte;
      file++;
      copied_current++;
    }
  while (byte && (copied_current - copied) < PGSIZE);

  /* Check if the copied string is properly NULL terminated. */
  if ((copied_current - copied) == PGSIZE && byte != 0)
    goto fail_during_copy;

  child_pid = process_execute (copied);
  palloc_free_page (copied);

  child_ctx = child_ctx_by_pid (child_pid);

  sema_down (&child_ctx->load_sema);
  if (!child_ctx->load_success)
    {
      process_cleanup_ctx (child_ctx);
      return TID_ERROR;
    }
  return child_pid;

fail_during_copy:
  palloc_free_page (copied);
  return TID_ERROR;
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

  pop_arg (const char *, file, sp);
  pop_arg (unsigned, initial_size, sp);
  printf ("SYS_CREATE(%s, %u)\n", file, initial_size);
  return 0;
}

/* System call handler for `REMOVE`. */
static int
syscall_remove (void *sp)
{
  const char *file;

  pop_arg (const char *, file, sp);
  printf ("SYS_REMOVE(%s)\n", file);
  return 0;
}

/* System call handler for `OPEN`. */
static int
syscall_open (void *sp)
{
  const char *file;

  pop_arg (const char *, file, sp);
  printf ("SYS_OPEN(%s)\n", file);
  return 0;
}

/* System call handler for `FILESIZE`. */
static int
syscall_filesize (void *sp)
{
  int fd;

  pop_arg (int, fd, sp);
  printf ("SYS_FILESIZE(%d)\n", fd);
  return 0;
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
  printf ("SYS_READ(%d, %p, %u)\n", fd, buffer, length);
  return 0;
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
  printf ("SYS_WRITE(%d, %p, %u)\n", fd, buffer, length);
  return 0;
}

/* System call handler for `SEEK`. */
static int
syscall_seek (void *sp)
{
  int fd;
  unsigned position;

  pop_arg (int, fd, sp);
  pop_arg (unsigned, position, sp);
  printf ("SYS_SEEK(%d, %u)\n", fd, position);
  return 0;
}

/* System call handler for `TELL`. */
static int
syscall_tell (void *sp)
{
  int fd;

  pop_arg (int, fd, sp);
  printf ("SYS_TELL(%d)\n", fd);
  return 0;
}

/* System call handler for `CLOSE`. */
static int
syscall_close (void *sp)
{
  int fd;

  pop_arg (int, fd, sp);
  printf ("SYS_CLOSE(%d)\n", fd);
  return 0;
}
