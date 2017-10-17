/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/qnx/misc.c
 * @brief       Generic misc. functions for QNX4
 * @version     -
 * @author      Nickols_K
 * @date        2003
 * @author      Andrew Golovnia
 * @since       2006
 * @note        Development, fixes and improvements
**/
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "biewlib/biewlib.h"

int __FASTCALL__ __inputRawInfo(char *head, char *text)
{
    return -1;
}

#ifdef __GNUC__
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#define LONG_LONG_MAX	9223372036854775807LL
#define LONG_LONG_MIN	(-LONG_LONG_MAX-1)

#define ULONG_LONG_MAX	(2ULL * LONG_LONG_MAX + 1)

/* constants used in Solaris */
#define LLONG_MIN       LONG_LONG_MIN
#define LLONG_MAX       LONG_LONG_MAX
#define ULLONG_MAX      ULONG_LONG_MAX

#define unconst(__v, __t) __extension__ ({union { const __t __cp; __t __p; } __q; __q.__cp = __v; __q.__p;})

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

/*
 * Convert a string to an unsigned long long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long long
strtoull(const char *nptr, char **endptr, int base)
{
  const char *s = nptr;
  unsigned long long acc;
  int c;
  unsigned long long cutoff;
  int neg = 0, any, cutlim;

  /*
   * See strtol for comments as to the logic used.
   */
  do {
    c = *s++;
  } while (isspace(c));
  if (c == '-')
  {
    neg = 1;
    c = *s++;
  }
  else if (c == '+')
    c = *s++;
  if ((base == 0 || base == 16) &&
      c == '0' && (*s == 'x' || *s == 'X'))
  {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;
  cutoff = (unsigned long long)ULLONG_MAX / base;
  cutlim = (unsigned long long)ULLONG_MAX % base;
  for (acc = 0, any = 0;; c = *s++)
  {
    if (isdigit(c))
      c -= '0';
    else if (isalpha(c))
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else
      break;
    if (c >= base)
      break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0)
  {
    acc = ULLONG_MAX;
    errno = ERANGE;
  }
  else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = any ? unconst(s, char *) - 1 : unconst(nptr, char *);
  return acc;
}

long long
strtoll(const char *nptr, char **endptr, int base)
{
  const char *s = nptr;
  unsigned long long acc;
  int c;
  unsigned long long cutoff;
  int neg = 0, any, cutlim;

  /*
   * See strtol for comments as to the logic used.
   */
  do {
    c = *s++;
  } while (isspace(c));
  if (c == '-')
  {
    neg = 1;
    c = *s++;
  }
  else if (c == '+')
    c = *s++;
  if ((base == 0 || base == 16) &&
      c == '0' && (*s == 'x' || *s == 'X'))
  {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;

/* to prevent overflow, we take max-1 and add 1 after division */
  cutoff = neg ? -(LLONG_MIN+1) : LLONG_MAX-1;
  cutlim = cutoff % base;
  cutoff /= base;
  if (++cutlim == base)
  {
    cutlim = 0;
    cutoff++;
  }
  for (acc = 0, any = 0;; c = *s++)
  {
    if (isdigit(c))
      c -= '0';
    else if (isalpha(c))
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else
      break;
    if (c >= base)
      break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else
    {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0)
  {
    acc = neg ? LLONG_MIN : LLONG_MAX;
    errno = ERANGE;
  }
  else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = any ? unconst(s, char *) - 1 : unconst(nptr, char *);
  return acc;
}
#endif
