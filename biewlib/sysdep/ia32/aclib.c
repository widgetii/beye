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

/*
 * This version of memcpy speedups memory copying up to 5 times
 * on modern cpus (K7, P3). But on K6-2+ cpus it has effect about 25%.
 *
 * This file was born in MPLAYER project. (http://mplayer.sourceforge.net)
 * Special thanks to Arp�d GereOffy (A'rpi/ESP-team) <arpi@thot.banki.hu>
 * for improvements and testing, Felix Buenemann <Felix.Buenemann@gmx.de>
 * for testing and Zolt�n Ponekker (Pontscho/Fresh!) <pontscho@makacs.poliod.hu>
 * for 3dNow! support which was ignored by me from begin. 
*/

/*
 This part of code was taken by me from Linux-2.4.3 and slightly modified
for MMX, MMX2, SSE instruction set. I have done it since linux uses page aligned
blocks but mplayer uses weakly ordered data and original sources can not
speedup them. Only using PREFETCHNTA and MOVNTQ together have effect!

>From IA-32 Intel Architecture Software Developer's Manual Volume 1,

Order Number 245470:
"10.4.6. Cacheability Control, Prefetch, and Memory Ordering Instructions"

Data referenced by a program can be temporal (data will be used again) or
non-temporal (data will be referenced once and not reused in the immediate
future). To make efficient use of the processor's caches, it is generally
desirable to cache temporal data and not cache non-temporal data. Overloading
the processor's caches with non-temporal data is sometimes referred to as
"polluting the caches".
The non-temporal data is written to memory with Write-Combining semantics.

The PREFETCHh instructions permits a program to load data into the processor
at a suggested cache level, so that it is closer to the processors load and
store unit when it is needed. If the data is already present in a level of
the cache hierarchy that is closer to the processor, the PREFETCHh instruction
will not result in any data movement.
But we should you PREFETCHNTA: Non-temporal data fetch data into location
close to the processor, minimizing cache pollution.

The MOVNTQ (store quadword using non-temporal hint) instruction stores
packed integer data from an MMX register to memory, using a non-temporal hint.
The MOVNTPS (store packed single-precision floating-point values using
non-temporal hint) instruction stores packed floating-point data from an
XMM register to memory, using a non-temporal hint.

The SFENCE (Store Fence) instruction controls write ordering by creating a
fence for memory store operations. This instruction guarantees that the results
of every store instruction that precedes the store fence in program order is
globally visible before any store instruction that follows the fence. The
SFENCE instruction provides an efficient way of ensuring ordering between
procedures that produce weakly-ordered data and procedures that consume that
data.

If you have questions please contact with me: Nick Kurshev: nickols_k@mail.ru.
*/

// 3dnow memcpy support from kernel 2.4.2
//  by Pontscho/fresh!mindworkz

#if !defined(__DISABLE_ASM) && (defined( HAVE_MMX2 ) || defined( HAVE_3DNOW ) || defined( HAVE_MMX ))

#undef HAVE_MMX1
#if defined(HAVE_MMX) && !defined(HAVE_MMX2) && !defined(HAVE_3DNOW) && !defined(HAVE_SSE)
/*  means: mmx v.1. Note: Since we added alignment of destinition it speedups
    of memory copying on PentMMX, Celeron-1 and P2 upto 12% versus
    standard (non MMX-optimized) version.
    Note: on K6-2+ it speedups memory copying upto 25% and
          on K7 and P3 about 500% (5 times). */
#define HAVE_MMX1
#endif


#undef HAVE_K6_2PLUS
#if !defined( HAVE_MMX2) && defined( HAVE_3DNOW)
#define HAVE_K6_2PLUS
#endif

#ifndef HAVE_SSE2
/*
   P3 processor has only one SSE decoder so can execute only 1 sse insn per
   cpu clock, but it has 3 mmx decoders (include load/store unit)
   and executes 3 mmx insns per cpu clock.
   P4 processor has some chances, but after reading:
   http://www.emulators.com/pentium4.htm
   I have doubts. Anyway SSE2 version of this code can be written better.
*/
#undef HAVE_SSE
#endif

/* for small memory blocks (<256 bytes) this version is faster */
#define small_memcpy(to,from,n)\
{\
register unsigned long int dummy;\
__asm__ __volatile__(\
	"rep; movsb"\
	:"=&D"(to), "=&S"(from), "=&c"(dummy)\
/* It's most portable way to notify compiler */\
/* that edi, esi and ecx are clobbered in asm block. */\
/* Thanks to A'rpi for hint!!! */\
        :"0" (to), "1" (from),"2" (n)\
	: "memory");\
}

/* for small memory blocks (<256 bytes) this version is faster */
#define small_memset(to,val,n)\
{\
register unsigned long int dummy;\
__asm__ __volatile__(\
	"rep; stosb"\
	:"=&D"(to), "=&c"(dummy)\
        :"0" (to), "1" (n), "a"((char)val)\
	:"memory");\
}

#ifdef HAVE_SSE
#define MMREG_SIZE 16
#else
#define MMREG_SIZE 8
#endif

/* Small defines (for readability only) ;) */
#ifdef HAVE_K6_2PLUS
#define PREFETCH "prefetch"
/* On K6 femms is faster of emms. On K7 femms is directly mapped on emms. */
#define EMMS     "femms"
#else
#define PREFETCH "prefetchnta"
#define EMMS     "emms"
#endif

#ifdef HAVE_MMX2
#define MOVNTQ "movntq"
#else
#define MOVNTQ "movq"
#endif

#ifdef HAVE_MMX1
#define MIN_LEN 0x800  /* 2K blocks */
#else
#define MIN_LEN 0x40  /* 64-byte blocks */
#endif

void * fast_memcpy(void * to, const void * from, size_t len)
{
	void *retval;
	size_t i;
  	retval = to;
#ifndef HAVE_MMX1
        /* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
	        PREFETCH" (%0)\n"
	        PREFETCH" 64(%0)\n"
	        PREFETCH" 128(%0)\n"
        	PREFETCH" 192(%0)\n"
        	PREFETCH" 256(%0)\n"
		: : "r" (from) );
#endif
        if(len >= MIN_LEN)
	{
	  register unsigned long int delta;
          /* Align destinition to MMREG_SIZE -boundary */
          delta = ((unsigned long int)to)&(MMREG_SIZE-1);
          if(delta)
	  {
	    delta=MMREG_SIZE-delta;
	    len -= delta;
	    small_memcpy(to, from, delta);
	  }
	  i = len >> 6; /* len/64 */
	  len&=63;
        /*
           This algorithm is top effective when the code consequently
           reads and writes blocks which have size of cache line.
           Size of cache line is processor-dependent.
           It will, however, be a minimum of 32 bytes on any processors.
           It would be better to have a number of instructions which
           perform reading and writing to be multiple to a number of
           processor's decoders, but it's not always possible.
        */
#ifdef HAVE_SSE /* Only P3 (may be Cyrix3) */
	if(((unsigned long)from) & 15)
	/* if SRC is misaligned */
	for(; i>0; i--)
	{
		__asm__ __volatile__ (
		PREFETCH" 320(%0)\n"
		"movups (%0), %%xmm0\n"
		"movups 16(%0), %%xmm1\n"
		"movups 32(%0), %%xmm2\n"
		"movups 48(%0), %%xmm3\n"
		"movntps %%xmm0, (%1)\n"
		"movntps %%xmm1, 16(%1)\n"
		"movntps %%xmm2, 32(%1)\n"
		"movntps %%xmm3, 48(%1)\n"
		:: "r" (from), "r" (to) : "memory");
		((const unsigned char *)from)+=64;
		((unsigned char *)to)+=64;
	}
	else 
	/*
	   Only if SRC is aligned on 16-byte boundary.
	   It allows to use movaps instead of movups, which required data
	   to be aligned or a general-protection exception (#GP) is generated.
	*/
	for(; i>0; i--)
	{
		__asm__ __volatile__ (
		PREFETCH" 320(%0)\n"
		"movaps (%0), %%xmm0\n"
		"movaps 16(%0), %%xmm1\n"
		"movaps 32(%0), %%xmm2\n"
		"movaps 48(%0), %%xmm3\n"
		"movntps %%xmm0, (%1)\n"
		"movntps %%xmm1, 16(%1)\n"
		"movntps %%xmm2, 32(%1)\n"
		"movntps %%xmm3, 48(%1)\n"
		:: "r" (from), "r" (to) : "memory");
		((const unsigned char *)from)+=64;
		((unsigned char *)to)+=64;
	}
#else
	for(; i>0; i--)
	{
		__asm__ __volatile__ (
#ifndef HAVE_MMX1
        	PREFETCH" 320(%0)\n"
#endif
		"movq (%0), %%mm0\n"
		"movq 8(%0), %%mm1\n"
		"movq 16(%0), %%mm2\n"
		"movq 24(%0), %%mm3\n"
		"movq 32(%0), %%mm4\n"
		"movq 40(%0), %%mm5\n"
		"movq 48(%0), %%mm6\n"
		"movq 56(%0), %%mm7\n"
		MOVNTQ" %%mm0, (%1)\n"
		MOVNTQ" %%mm1, 8(%1)\n"
		MOVNTQ" %%mm2, 16(%1)\n"
		MOVNTQ" %%mm3, 24(%1)\n"
		MOVNTQ" %%mm4, 32(%1)\n"
		MOVNTQ" %%mm5, 40(%1)\n"
		MOVNTQ" %%mm6, 48(%1)\n"
		MOVNTQ" %%mm7, 56(%1)\n"
		:: "r" (from), "r" (to) : "memory");
		((const unsigned char *)from)+=64;
		((unsigned char *)to)+=64;
	}
#endif /* Have SSE */
#ifdef HAVE_MMX2
                /* since movntq is weakly-ordered, a "sfence"
		 * is needed to become ordered again. */
		__asm__ __volatile__ ("sfence":::"memory");
#endif
#ifndef HAVE_SSE		
		/* enables to use FPU */
		__asm__ __volatile__ (EMMS:::"memory");
#endif		
	}
	/*
	 *	Now do the tail of the block
	 */
	if(len) small_memcpy(to, from, len);
	return retval;
}

/* Fast memory set. See comments for fast_memcpy */
void * fast_memset(void * to, int val, size_t len)
{
	void *retval;
	size_t i;
	unsigned char mm_reg[MMREG_SIZE], *pmm_reg;

  	retval = to;
        if(len >= MIN_LEN)
	{
	  register unsigned long int delta;
          delta = ((unsigned long int)to)&(MMREG_SIZE-1);
          if(delta)
	  {
	    delta=MMREG_SIZE-delta;
	    len -= delta;
	    small_memset(to, val, delta);
	  }
	  i = len >> 7; /* len/128 */
	  len&=127;
	  pmm_reg = mm_reg;
	  small_memset(pmm_reg,val,sizeof(mm_reg));
#ifdef HAVE_SSE /* Only P3 (may be Cyrix3) */
	__asm__ __volatile__(
		"movups (%0), %%xmm0\n"
		:: "r"(mm_reg):"memory");
	for(; i>0; i--)
	{
		__asm__ __volatile__ (
		"movntps %%xmm0, (%0)\n"
		"movntps %%xmm0, 16(%0)\n"
		"movntps %%xmm0, 32(%0)\n"
		"movntps %%xmm0, 48(%0)\n"
		"movntps %%xmm0, 64(%0)\n"
		"movntps %%xmm0, 80(%0)\n"
		"movntps %%xmm0, 96(%0)\n"
		"movntps %%xmm0, 112(%0)\n"
		:: "r" (to) : "memory");
		((unsigned char *)to)+=128;
	}
#else
	__asm__ __volatile__(
		"movq (%0), %%mm0\n"
		:: "r"(mm_reg):"memory");
	for(; i>0; i--)
	{
		__asm__ __volatile__ (
		MOVNTQ" %%mm0, (%0)\n"
		MOVNTQ" %%mm0, 8(%0)\n"
		MOVNTQ" %%mm0, 16(%0)\n"
		MOVNTQ" %%mm0, 24(%0)\n"
		MOVNTQ" %%mm0, 32(%0)\n"
		MOVNTQ" %%mm0, 40(%0)\n"
		MOVNTQ" %%mm0, 48(%0)\n"
		MOVNTQ" %%mm0, 56(%0)\n"
		MOVNTQ" %%mm0, 64(%0)\n"
		MOVNTQ" %%mm0, 72(%0)\n"
		MOVNTQ" %%mm0, 80(%0)\n"
		MOVNTQ" %%mm0, 88(%0)\n"
		MOVNTQ" %%mm0, 96(%0)\n"
		MOVNTQ" %%mm0, 104(%0)\n"
		MOVNTQ" %%mm0, 112(%0)\n"
		MOVNTQ" %%mm0, 120(%0)\n"
		:: "r" (to) : "memory");
		((unsigned char *)to)+=128;
	}
#endif /* Have SSE */
#ifdef HAVE_MMX2
                /* since movntq is weakly-ordered, a "sfence"
		 * is needed to become ordered again. */
		__asm__ __volatile__ ("sfence":::"memory");
#endif
#ifndef HAVE_SSE		
		/* enables to use FPU */
		__asm__ __volatile__ (EMMS:::"memory");
#endif		
	}
	/*
	 *	Now do the tail of the block
	 */
	if(len) small_memset(to, val, len);
	return retval;
}

#endif /* #if defined( HAVE_MMX2 ) || defined( HAVE_3DNOW ) || defined( HAVE_MMX ) */
