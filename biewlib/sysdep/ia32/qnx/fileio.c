#ifdef __QNX4__
#define ftruncate(a,b) chsize(a,b)
#endif
#include "biewlib/sysdep/generic/posix/fileio.c"
