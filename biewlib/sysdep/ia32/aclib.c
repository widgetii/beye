/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia32/aclib.c
 * @brief       This file contains functions which are expand are improve
 *              standard C library ones.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 * @remark      I used such form of this file because of build-in assembler
 *              allow write calling convention independed code. In addition,
 *              GNU C compiler is ported under multiple OS's. If somebody will
 *              port it under ABC-xyz platform, then more easy find compiler
 *              with build-in assembler, instead rewriting of makefile with .s
 *              (or .asm) extensions for choosen development system.
 *
 * @author      Nick Kurshev
 * @since       2001
 * @note        Development, fixes and improvements
**/
#include <stddef.h>
#include <string.h>
#include "biewlib/sysdep/ia32/fastcopy.h"
#include "biewlib/sysdep/ia32/stdint.h"
#include "biewlib/sysdep/ia32/_inlines.h"
#define BLOCK_SIZE 4096
#define CONFUSION_FACTOR 0
//Feel free to fine-tune the above 2, it might be possible to get some speedup with them :)

//#define STATISTICS

#ifdef	CAN_COMPILE_X86_GAS
#define CAN_COMPILE_X86_ASM
#endif

//Note: we have MMX, MMX2, 3DNOW version there is no 3DNOW+MMX2 one
//Plain C versions
//#if !defined (HAVE_MMX) || defined (RUNTIME_CPUDETECT)
//#define COMPILE_C
//#endif
#define USE_MMX		0x00000001UL
#define USE_MMX2	0x00000002UL
#define USE_3DNOW	0x00000004UL
#define USE_3DNOW2	0x00000008UL
#define USE_SSE		0x00000010UL
#define USE_SSE2	0x00000020UL

#ifdef CAN_COMPILE_X86_ASM
static int _mmx_inited=0;
static unsigned __mmx_caps=0;
static inline void do_cpuid(unsigned int ax, unsigned int *p)
{
	int a, c;

	__asm__ __volatile__ (
			  /* See if CPUID instruction is supported ... */
			  /* ... Get copies of EFLAGS into eax and ecx */
			  "pushf\n\t"
			  "popl %0\n\t"
			  "movl %0, %1\n\t"
			  
			  /* ... Toggle the ID bit in one copy and store */
			  /*     to the EFLAGS reg */
			  "xorl $0x200000, %0\n\t"
			  "push %0\n\t"
			  "popf\n\t"
			  
			  /* ... Get the (hopefully modified) EFLAGS */
			  "pushf\n\t"
			  "popl %0\n\t"
			  : "=a" (a), "=c" (c)
			  :
			  : "cc" 
			  );
    if(a!=c)
    __asm __volatile
	("movl %%ebx, %%esi\n\t"
	 "cpuid\n\t"
	 "xchgl %%ebx, %%esi"
	 : "=a" (p[0]), "=S" (p[1]), 
	   "=c" (p[2]), "=d" (p[3])
	 : "0" (ax));
    else p[0]=p[1]=p[2]=p[3]=ax=0;
}

static void GetCpuCaps( void )
{
	unsigned int regs[4];
	unsigned int regs2[4];
	_mmx_inited=1;
	__mmx_caps = 0;
	do_cpuid(0x00000000, regs); /* get _max_ cpuid level and vendor name*/
	if (regs[0]>=0x00000001)
	{
		do_cpuid(0x00000001, regs2);
		/* general feature flags: */
		if((regs2[3] & (1 << 23 )) >> 23) __mmx_caps |= USE_MMX;
		if((regs2[3] & (1 << 25 )) >> 25) __mmx_caps |= USE_SSE;
		if((regs2[3] & (1 << 26 )) >> 26) __mmx_caps |= USE_SSE2;
		if(__mmx_caps & USE_SSE) __mmx_caps |= USE_MMX2; /* SSE cpus supports mmxext too */
	}
	do_cpuid(0x80000000, regs);
	if (regs[0]>=0x80000001) {
		do_cpuid(0x80000001, regs2);
		if((regs2[3] & (1 << 23 )) >> 23) __mmx_caps |= USE_MMX;
		if((regs2[3] & (1 << 22 )) >> 22) __mmx_caps |= USE_MMX2;
		if((regs2[3] & (1 << 31 )) >> 31) __mmx_caps |= USE_3DNOW;
		if((regs2[3] & (1 << 30 )) >> 30) __mmx_caps |= USE_3DNOW2;
	}
}
#else
static int _mmx_inited=1;
static unsigned __mmx_caps=0;
static void GetCpuCaps( void ) {}
#endif


#ifdef CAN_COMPILE_X86_ASM

#undef HAVE_MMX
#undef HAVE_MMX2
#undef HAVE_3DNOW
#undef HAVE_SSE
#define RENAME(a) a ## _C
#include "biewlib/sysdep/ia32/aclib_template.c"

//MMX versions
#undef RENAME
#define HAVE_MMX
#undef HAVE_MMX2
#undef HAVE_3DNOW
#define RENAME(a) a ## _MMX
#include "biewlib/sysdep/ia32/aclib_template.c"

#if 0 /* TODO: better detection of gas possibilities */
//MMX2 versions
#undef RENAME
#define HAVE_MMX
#define HAVE_MMX2
#undef HAVE_3DNOW
#define RENAME(a) a ## _MMX2
#include "biewlib/sysdep/ia32/aclib_template.c"

//3DNOW versions
#undef RENAME
#define HAVE_MMX
#undef HAVE_MMX2
#define HAVE_3DNOW
#define RENAME(a) a ## _3DNow
#include "biewlib/sysdep/ia32/aclib_template.c"
#endif

#endif // CAN_COMPILE_X86_ASM

static void * init_fast_memcpy(void * to, const void * from, size_t len)
{
#ifdef CAN_COMPILE_X86_ASM
	/* ordered per speed fastest first */
	if(!_mmx_inited) GetCpuCaps();
//	if(__mmx_caps & USE_MMX2)	fast_memcpy_ptr = fast_memcpy_MMX2;
//	else if(__mmx_caps & USE_3DNOW)	fast_memcpy_ptr = fast_memcpy_3DNow;
	else if(__mmx_caps & USE_MMX)	fast_memcpy_ptr = fast_memcpy_MMX;
	else
#endif
	fast_memcpy_ptr = memcpy;
	return (*fast_memcpy_ptr)(to,from,len);	
}
void *(*fast_memcpy_ptr)(void * to, const void * from, size_t len) = init_fast_memcpy;

static void * init_fast_memset(void * to, int filler, size_t len)
{
#ifdef CAN_COMPILE_X86_ASM
	/* ordered per speed fastest first */
	if(!_mmx_inited) GetCpuCaps();
//	if(__mmx_caps & USE_MMX2)	fast_memset_ptr = fast_memset_MMX2;
//	else if(__mmx_caps & USE_3DNOW)	fast_memset_ptr = fast_memset_3DNow;
	else if(__mmx_caps & USE_MMX)	fast_memset_ptr = fast_memset_MMX;
	else
#endif
	fast_memset_ptr = memset;
	return (*fast_memset_ptr)(to,filler,len);	
}
void *(*fast_memset_ptr)(void * to, int filler, size_t len) = init_fast_memset;

static void __FASTCALL__ init_InterleaveBuffers(tUInt32 limit,
				    void *destbuffer,
				    const void *evenbuffer, 
				    const void *oddbuffer)
{
#ifdef CAN_COMPILE_X86_ASM
	/* ordered per speed fastest first */
	if(!_mmx_inited) GetCpuCaps();
//	if(__mmx_caps & USE_MMX2)	InterleaveBuffers_ptr = InterleaveBuffers_MMX2;
//	else if(__mmx_caps & USE_3DNOW)	InterleaveBuffers_ptr = InterleaveBuffers_3DNow;
	else if(__mmx_caps & USE_MMX)	InterleaveBuffers_ptr = InterleaveBuffers_MMX;
	else
#endif
	InterleaveBuffers_ptr = InterleaveBuffers_C;
	(*InterleaveBuffers_ptr)(limit,destbuffer,evenbuffer,oddbuffer);	
}

void __FASTCALL__ (*InterleaveBuffers_ptr)(tUInt32 limit,
				    void *destbuffer,
				    const void *evenbuffer, 
				    const void *oddbuffer) = init_InterleaveBuffers;

static void __FASTCALL__ init_CharsToShorts(tUInt32 limit,
					     void *destbuffer,
					     const void *evenbuffer)
{
#ifdef CAN_COMPILE_X86_ASM
	/* ordered per speed fastest first */
	if(!_mmx_inited) GetCpuCaps();
//	if(__mmx_caps & USE_MMX2)	CharsToShorts_ptr = CharsToShorts_MMX2;
//	else if(__mmx_caps & USE_3DNOW)	CharsToShorts_ptr = CharsToShorts_3DNow;
	else if(__mmx_caps & USE_MMX)	CharsToShorts_ptr = CharsToShorts_MMX;
	else
#endif
	CharsToShorts_ptr = CharsToShorts_C;
	(*CharsToShorts_ptr)(limit,destbuffer,evenbuffer);	
}

void __FASTCALL__ (*CharsToShorts_ptr)(tUInt32 limit,
					     void *destbuffer,
					     const void *evenbuffer) = init_CharsToShorts;

static void __FASTCALL__ init_ShortsToChars(tUInt32 limit,
					     void *destbuffer,
					     const void *evenbuffer)
{
#ifdef CAN_COMPILE_X86_ASM
	/* ordered per speed fastest first */
	if(!_mmx_inited) GetCpuCaps();
//	if(__mmx_caps & USE_MMX2)	ShortsToChars_ptr = ShortsToChars_MMX2;
//	else if(__mmx_caps & USE_3DNOW)	ShortsToChars_ptr = ShortsToChars_3DNow;
	else if(__mmx_caps & USE_MMX)	ShortsToChars_ptr = ShortsToChars_MMX;
	else
#endif
	ShortsToChars_ptr = ShortsToChars_C;
	(*ShortsToChars_ptr)(limit,destbuffer,evenbuffer);	
}

void __FASTCALL__ (*ShortsToChars_ptr)(tUInt32 limit,
					     void *destbuffer,
					     const void *evenbuffer) = init_ShortsToChars;
