#include "swiped.h"

// Fallback routine only recognizes the format of the locale.
// It is the same as the output format.
// But the caller needs to know the day of the week.
time_t lde_getdate(char *p)
{
  char buf[100];
  struct tm t;
  if (strftime(buf, sizeof(buf), "%c", &t)) {
    return mktime(&t);
  }
  return 0;
}