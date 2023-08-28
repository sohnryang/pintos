#ifndef USERPROG_CHECKED_USER_MEM_H
#define USERPROG_CHECKED_USER_MEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int checked_copy_byte_from_user (const uint8_t *);
bool checked_copy_byte_to_user (uint8_t *, uint8_t);

void *checked_memcpy_from_user (void *, const void *, size_t);
void *checked_memcpy_to_user (void *, const void *, size_t);
int checked_strlen (const char *);
int checked_strlcpy_from_user (char *, const char *, size_t);

#endif
