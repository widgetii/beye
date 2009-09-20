#ifndef __BIEW_FASTMEMCPY
#define __BIEW_FASTMEMCPY 1

#ifndef	__DISABLE_ASM
#include <stddef.h>
#include <string.h>

extern void * (*fast_memcpy_ptr)(void * to, const void * from, size_t len);
#ifdef memcpy
#undef memcpy
#endif
#define memcpy(a,b,c) (*fast_memcpy_ptr)(a,b,c)
extern void * (*fast_memset_ptr)(void * to, int filler, size_t len);
#ifdef memset
#undef memset
#endif
#define memset(a,b,c) (*fast_memset_ptr)(a,b,c)

#endif	/* __DISABLE_ASM */

#endif	/* __BIEW_FASTMEMCPY */
