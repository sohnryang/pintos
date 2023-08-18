#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
static int syscall_halt (void *);
static int syscall_exit (void *);
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
  printf ("SYS_HALT()\n");
  return 0;
}

/* System call handler for `EXIT`. */
static int
syscall_exit (void *sp)
{
  int status;

  pop_arg (int, status, sp);
  printf ("SYS_EXIT(%d)\n", status);
  return 0;
}

/* System call handler for `EXEC`. */
static int
syscall_exec (void *sp)
{
  const char *file;

  pop_arg (const char *, file, sp);
  printf ("SYS_EXEC(%s)\n", file);
  return 0;
}

/* System call handler for `WAIT`. */
static int
syscall_wait (void *sp)
{
  int pid; // pid_t is typedef'd as int

  pop_arg (int, pid, sp);
  printf ("SYS_WAIT(%d)\n", pid);
  return 0;
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
