#ifndef __BIEW_FASTMEMCPY
#define __BIEW_FASTMEMCPY 1

#ifndef	__DISABLE_ASM

#if defined( HAVE_MMX2 ) || defined( HAVE_3DNOW ) || defined( HAVE_MMX )
#include <stddef.h>

extern void * fast_memcpy(void * to, const void * from, size_t len);
#ifdef memcpy
#undef memcpy
#endif
#define memcpy(a,b,c) fast_memcpy(a,b,c)
extern void * fast_memset(void * to, int val, size_t len);
#ifdef memset
#undef memset
#endif
#define memset(a,b,c) fast_memset(a,b,c)

#endif

#endif	/* __DISABLE_ASM */

#endif	/* __BIEW_FASTMEMCPY */
