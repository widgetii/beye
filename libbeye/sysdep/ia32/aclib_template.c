/*
  aclib - advanced C library ;)
  This file contains functions which improve and expand standard C-library
*/

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

If you have questions please contact with me: nickols_k@mail.ru.
*/

/* 3dnow memcpy support from kernel 2.4.2
   by Pontscho/fresh!mindworkz */


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

#include "biewlib/sysdep/ia32/mmx_defs.h"
#undef MMREG_SIZE
#ifdef HAVE_SSE
#define MMREG_SIZE 16
#else
#define MMREG_SIZE 64 //8
#endif
#undef MIN_LEN
#ifdef HAVE_MMX1
#define MIN_LEN 0x800  /* 2K blocks */
#else
#define MIN_LEN 0x40  /* 64-byte blocks */
#endif


static void * RENAME(fast_memcpy)(void * to, const void * from, size_t len)
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
	if(i) {
	// Align destination at BLOCK_SIZE boundary
	for(; i>0;i--)
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
		from+=64;
		to+=64;
	}
	}
#ifdef HAVE_MMX2
		/* since movntq is weakly-ordered, a "sfence"
		 * is needed to become ordered again. */
		__asm__ __volatile__ ("sfence":::"memory");
#endif
		/* enables to use FPU */
		__asm__ __volatile__ (EMMS:::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if(len) small_memcpy(to, from, len);
	return retval;
}

#define small_memset(to,val,n)\
{\
register unsigned long int dummy;\
__asm__ __volatile__(\
	"rep; stosb"\
	:"=&D"(to), "=&c"(dummy)\
	:"0" (to), "1" (n), "a"((char)val)\
	:"memory");\
}

/* Fast memory set. See comments for fast_memcpy */
static void * RENAME(fast_memset)(void * to, int val, size_t len)
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
		to+=128;
	}
#ifdef HAVE_MMX2
		/* since movntq is weakly-ordered, a "sfence"
		 * is needed to become ordered again. */
		__asm__ __volatile__ ("sfence":::"memory");
#endif
		/* enables to use FPU */
		__asm__ __volatile__ (EMMS:::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if(len) small_memset(to, val, len);
	return retval;
}

#ifdef REGMM_SIZE
#undef REGMM_SIZE
#endif
#define REGMM_SIZE 8 /* In the future it can be safety replaced with 16 for SSE2 */
static void __FASTCALL__ RENAME(InterleaveBuffers)(tUInt32 limit,
				    void *destbuffer,
				    const void *evenbuffer, 
				    const void *oddbuffer)
{
#ifdef HAVE_MMX
  register char *destbuffptr;
  register const char *oddptr, *evenptr;
  register tUInt32 freq;
  destbuffptr = (char *)destbuffer;
  evenptr = (const char *)evenbuffer;
  oddptr = (const char *)oddbuffer;
  freq = 0;
  if(limit>REGMM_SIZE*4-1)
  {
      register tUInt32 delta, nlimit, step;
      step = REGMM_SIZE*2;
      /* Try to align buffers on boundary of REGMM_SIZE */
      delta = ((tUInt32)evenptr)&(REGMM_SIZE-1);
      if(delta) delta=REGMM_SIZE-delta;
      nlimit=(limit-delta)/step;
      freq=delta+(nlimit*step);
      while(delta)
      {
	*destbuffptr++ = *evenptr++;
	*destbuffptr++ = *oddptr++;
	delta--;
      }
      /* Perform MMX optimized interleaving */
      while(nlimit)
      {
	 /* Interleave mmx and cpu instructions */
	 __asm __volatile("movq	(%0), %%mm0\n\t"
	       "movq	8(%0), %%mm2\n\t"
	       ::"r"(evenptr):"memory");
	 evenptr+=step;
	 __asm __volatile("movq	%%mm0, %%mm1\n\t"
	       "movq	%%mm2, %%mm3\n\t"
	       "punpckhbw (%0), %%mm0\n\t"	       
	       "punpckhbw 8(%0), %%mm2\n\t"
	      ::"r"(oddptr):"memory");
	 nlimit--;
	 __asm __volatile("punpcklbw (%0), %%mm1\n\t"
	       "punpcklbw 8(%0), %%mm3\n\t"
	       ::"r"(oddptr):"memory");
	 oddptr+=step;
	 __asm __volatile("movq	%%mm0, 8(%0)\n\t"
	       "movq	%%mm1, (%0)\n\t"
	       "movq	%%mm2, 24(%0)\n\t"
	       "movq	%%mm3, 16(%0)"
	      ::"r"(destbuffptr):"memory");
	 destbuffptr+=step*2;
      }
      __asm __volatile("emms":::"memory");
  }
  /* If tail exists then finish it */
  while(freq<limit)
  {
    *destbuffptr++ = *evenptr++;
    *destbuffptr++ = *oddptr++;
    freq++;
  }
#else
{
  register size_t freq;
  for(freq=0;freq<(size_t)limit;freq++)
  {
    ((char *)destbuffer)[freq+freq] = ((char *)evenbuffer)[freq];
    ((char *)destbuffer)[freq+freq+1] = ((char *)oddbuffer)[freq];
  }
}
#endif
}

static void __FASTCALL__ RENAME(CharsToShorts)(tUInt32 limit,
					     void *destbuffer,
					     const void *evenbuffer)
{
#ifdef HAVE_MMX
  register char *destbuffptr;
  register const char *evenptr;
  register tUInt32 freq;
  destbuffptr = (char *)destbuffer;
  evenptr = (const char *)evenbuffer;
  freq = 0;
  if(limit>REGMM_SIZE*4-1)
  {
      register tUInt32 delta, nlimit, step;
      step = REGMM_SIZE*2;
      /* Try to align buffer on boundary of REGMM_SIZE */
      delta = ((tUInt32)evenptr)&(REGMM_SIZE-1);
      if(delta) delta=REGMM_SIZE-delta;
      nlimit=(limit-delta)/step;
      freq=delta+(nlimit*step);
      while(delta)
      {
	*destbuffptr++ = *evenptr++;
	*destbuffptr++ = 0;
	delta--;
      }
      /* Perform MMX optimized loop */
      __asm __volatile("pxor	%%mm7, %%mm7":::"memory");
      while(nlimit)
      {
	 /* Interleave mmx and cpu instructions */
	 __asm __volatile("movq	(%0),%%mm0\n\t"
	       "movq	8(%0),%%mm2"
	       ::"r"(evenptr):"memory");
	 evenptr+=step;
	 __asm __volatile("movq	%%mm0, %%mm1\n\t"
	       "movq	%%mm2, %%mm3\n\t"
	       "punpckhbw %%mm7, %%mm0\n\t"
	       "punpckhbw %%mm7, %%mm2"
	      :::"memory");
	 nlimit--;
	 __asm __volatile("punpcklbw %%mm7, %%mm1\n\t"
	       "punpcklbw %%mm7, %%mm3"
	       :::"memory");
	 __asm __volatile("movq	%%mm0, 8(%0)\n\t"
	       "movq	%%mm1, (%0)\n\t"
	       "movq	%%mm2, 24(%0)\n\t"
	       "movq	%%mm3, 16(%0)"
	       ::"r"(destbuffptr):"memory");
	 destbuffptr+=step*2;
      }
      __asm __volatile("emms":::"memory");
  }
  /* If tail exists then finish it */
  while(freq<limit)
  {
    *destbuffptr++ = *evenptr++;
    *destbuffptr++ = 0;
    freq++;
  }
#else
{
  register size_t freq;
  for(freq=0;freq<(size_t)limit;freq++)
  {
    ((char *)destbuffer)[freq+freq] = ((char *)evenbuffer)[freq];
    ((char *)destbuffer)[freq+freq+1] = 0;
  }
}
#endif
}

static void __FASTCALL__ RENAME(ShortsToChars)(tUInt32 limit,
				     void * destbuffer, const void * srcbuffer)
{
#ifdef HAVE_MMX
  register char *destbuffptr;
  register const char *srcptr;
  register tUInt32 freq;
  destbuffptr = (char *)destbuffer;
  srcptr = (const char *)srcbuffer;
  freq = 0;
  if(limit>REGMM_SIZE*4-1)
  {
      tUInt32 delta, nlimit, step;
      step = REGMM_SIZE*2;
      /* Try to align buffers on boundary of REGMM_SIZE */
      delta=((tUInt32)destbuffptr)&(REGMM_SIZE-1);
      if(delta) delta=REGMM_SIZE-delta;
      nlimit=(limit-delta)/step;
      freq=delta+(nlimit*step);
      while(delta)
      {
	*destbuffptr++ = *srcptr;
	srcptr+=2;
	delta--;
      }
      /* Perform MMX optimized loop */
      while(nlimit)
      {
	 /* Interleave mmx and cpu instructions */
	 __asm __volatile("movq	(%0), %%mm0\n\t"
	       "movq	8(%0), %%mm1\n\t"
	       ::"r"(srcptr):"memory");
	 nlimit--;
	 __asm __volatile("packuswb (%0), %%mm0\n\t"
	       "packuswb 8(%0), %%mm1"
	       ::"g"(&srcptr[REGMM_SIZE]):"memory");
	 srcptr+=step*2;
	 __asm __volatile("movq	%%mm0, (%0)\n\t"
	       "movq	%%mm1, 8(%0)\n\t"
	       ::"r"(destbuffptr):"memory");
	 destbuffptr+=step;
      }
      __asm __volatile("emms":::"memory");
  }
  /* If tail exists then finish it */
  while(freq<limit)
  {
    *destbuffptr++ = *srcptr;
    srcptr+=2;
    freq++;
  }
#else
{
  register size_t freq;
  for(freq=0;freq<(size_t)limit;freq++)
  {
    ((char *)destbuffer)[freq] = ((char *)srcbuffer)[freq+freq];
  }
}
#endif
}
