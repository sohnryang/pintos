#include "checked_user_mem.h"

#include <stdbool.h>
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

static bool
is_valid_user_ptr (const void *uaddr)
{
#ifdef VM
  struct thread *cur;

  cur = thread_current ();
  return pagedir_is_user (cur->pagedir, pg_round_down (uaddr));
#else
  return uaddr < PHYS_BASE;
#endif
}

static bool
is_contained_in_user (const void *uaddr, size_t n)
{
  return is_valid_user_ptr (uaddr)
         && is_valid_user_ptr ((const uint8_t *)uaddr + n - 1);
}

int
checked_copy_byte_from_user (const uint8_t *usrc)
{
  int res;

  if (!is_valid_user_ptr (usrc))
    return -1;

#ifdef VM
  res = *usrc;
#else
  asm ("movl $1f, %0\n\t"
       "movzbl %1, %0\n\t"
       "1:"
       : "=&a"(res)
       : "m"(*usrc));
#endif

  return res;
}

bool
checked_copy_byte_to_user (uint8_t *udst, uint8_t byte)
{
#ifndef VM
  int error_code;
#endif

  if (!is_valid_user_ptr (udst))
    return false;

#ifdef VM
  *udst = byte;
  return true;
#else
  asm ("movl $1f, %0\n\t"
       "movb %b2, %1\n\t"
       "1:"
       : "=&a"(error_code), "=m"(*udst)
       : "q"(byte));
  return error_code != -1;
#endif
}

/* Copy `n` bytes from `usrc` to `dst`. `usrc` must be pointing to userspace
   memory. Returns `dst` on success, and NULL on failure. */
void *
checked_memcpy_from_user (void *dst, const void *usrc, size_t n)
{
  uint8_t *dst_byte = dst;
  const uint8_t *src_byte = usrc;
  int byte;

  if (!is_contained_in_user (usrc, n))
    return NULL;

  for (size_t i = 0; i < n; i++)
    {
      byte = checked_copy_byte_from_user (src_byte);
      if (byte == -1)
        return NULL;
      *dst_byte = byte;
      dst_byte++;
      src_byte++;
    }
  return dst;
}

/* Copy `n` bytes from `src` to `udst`. `udst` must be pointing to userspace
   memory. Returns `udst` on success, and NULL on failure. */
void *
checked_memcpy_to_user (void *udst, const void *src, size_t n)
{
  uint8_t *dst_byte = udst;
  const uint8_t *src_byte = src;
  bool success;

  if (!is_contained_in_user (udst, n))
    return NULL;

  for (size_t i = 0; i < n; i++)
    {
      success = checked_copy_byte_to_user (dst_byte, *src_byte);
      if (!success)
        return NULL;
      dst_byte++;
      src_byte++;
    }
  return udst;
}

/* Calculate length of the given `string`. Returns -1 on error.
   Note: this function works on strings whose lengths are bounded by maximum
   value of 32-bit integer. */
int
checked_strlen (const char *string)
{
  const char *p;
  int byte;

  for (p = string;
       (byte = checked_copy_byte_from_user ((const void *)p)) != '\0'; p++)
    {
      if (byte == -1)
        return -1;
      continue;
    }
  return p - string;
}

/* Copy NULL terminated string from `usrc` to `dst`. Returns copied length on
   success, and -1 on failure. Copies at most `size` bytes.
   Note: this function works on strings whose lengths are bounded by maximum
   value of 32-bit integer. */
int
checked_strlcpy_from_user (char *dst, const char *usrc, size_t size)
{
  int src_len;
  void *res;

  ASSERT (dst != NULL);

  src_len = checked_strlen (usrc);
  if (src_len == -1)
    return -1;
  if (size > 0)
    {
      size_t dst_len = size - 1;
      if ((size_t)src_len < dst_len)
        dst_len = src_len;
      res = checked_memcpy_from_user (dst, usrc, dst_len);

      if (res == NULL)
        return 0;
      dst[dst_len] = '\0';
    }
  return src_len;
}
