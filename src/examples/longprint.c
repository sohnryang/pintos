#include <syscall.h>

int
main (void)
{
  char arr[2048];
  int i, n;
  for (n = 0; n < 10; n++)
    {
      for (i = 0; i < 2046; i++)
        arr[i] = '0' + n;
      arr[2046] = '\n';
      arr[2047] = '\0';
      write (1, arr, 2047);
    }
  return 0;
}
