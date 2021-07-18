#include <stdio.h>

main()
{
  int i, j;
  for (i = 0; i < 16; i++)
    for (j = 0; j < 1024; j++)
      printf("%1X", i);
}
