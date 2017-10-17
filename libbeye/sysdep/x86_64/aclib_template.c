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

/* for small memory blocks (<256 bytes) this version is faster */
#define small_memcpy(to,from,n)\
{\
register unsigned long int siz;\
register unsigned long int dummy;\
    siz=n&0x7;  n>>=3;\
    if(siz)\
__asm__ __volatile__(\
	"rep; movsb"\
	:"=&D"(to), "=&S"(from), "=&c"(dummy)\
	:"0" (to), "1" (from),"2" (siz)\
	: "memory","cc");\
    if(n)\
__asm__ __volatile__(\
	"rep; movsq"\
	:"=&D"(to), "=&S"(from), "=&c"(dummy)\
	:"0" (to), "1" (from),"2" (n)\
	: "memory","cc");\
}


#define MMREG_SIZE 16ULL
#define MIN_LEN 257ULL
#define CL_SIZE 256ULL /*always align on 256 byte boundary */

static inline void * RENAME(fast_memcpy)(void * to, const void * from, size_t len)
{
	void *retval;
	const unsigned char *cfrom=from;
	unsigned char *tto=to;
	size_t i=0;
	retval = to;
	if(!len) return retval;
        /* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
		"prefetcht0 (%0)\n"
		"prefetcht0 64(%0)\n"
		"prefetcht0 128(%0)\n"
		"prefetcht0 192(%0)\n"
		:: "r" (cfrom));
        if(len >= MIN_LEN)
	{
	  register unsigned long int delta;
          /* Align destinition to cache-line size -boundary */
          delta = ((unsigned long long int)tto)&(CL_SIZE-1ULL);
          if(delta)
	  {
	    delta=CL_SIZE-delta;
	    len -=delta;
	    small_memcpy(tto, cfrom, delta);
	  }
	  i = len>>8; /* len/256 */
	  len=len-(i<<8);
	}
	if(i) {
        /*
           This algorithm is top effective when the code consequently
           reads and writes blocks which have size of cache line.
           Size of cache line is processor-dependent.
           It will, however, be a minimum of 32 bytes on any processors.
           It would be better to have a number of instructions which
           perform reading and writing to be multiple to a number of
           processor's decoders, but it's not always possible.
        */
	if(((unsigned long long)cfrom) & 15)
	/* if SRC is misaligned */
	for(; i>0; i--)
	{
		__asm__ __volatile__ (
		"prefetcht0 256(%0)\n"
		"prefetcht0 320(%0)\n"
		"movdqu (%0), %%xmm0\n"
		"movdqu 16(%0), %%xmm1\n"
		"movdqu 32(%0), %%xmm2\n"
		"movdqu 48(%0), %%xmm3\n"
		"movdqu 64(%0), %%xmm4\n"
		"movdqu 80(%0), %%xmm5\n"
		"movdqu 96(%0), %%xmm6\n"
		"movdqu 112(%0), %%xmm7\n"
		"prefetcht0 384(%0)\n"
		"prefetcht0 448(%0)\n"
		"movdqu 128(%0), %%xmm8\n"
		"movdqu 144(%0), %%xmm9\n"
		"movdqu 160(%0), %%xmm10\n"
		"movdqu 176(%0), %%xmm11\n"
		"movdqu 192(%0), %%xmm12\n"
		"movdqu 208(%0), %%xmm13\n"
		"movdqu 224(%0), %%xmm14\n"
		"movdqu 240(%0), %%xmm15\n"
		"movntdq %%xmm0, (%1)\n"
		"movntdq %%xmm1, 16(%1)\n"
		"movntdq %%xmm2, 32(%1)\n"
		"movntdq %%xmm3, 48(%1)\n"
		"movntdq %%xmm4, 64(%1)\n"
		"movntdq %%xmm5, 80(%1)\n"
		"movntdq %%xmm6, 96(%1)\n"
		"movntdq %%xmm7, 112(%1)\n"
		"movntdq %%xmm8, 128(%1)\n"
		"movntdq %%xmm9, 144(%1)\n"
		"movntdq %%xmm10, 160(%1)\n"
		"movntdq %%xmm11, 176(%1)\n"
		"movntdq %%xmm12, 192(%1)\n"
		"movntdq %%xmm13, 208(%1)\n"
		"movntdq %%xmm14, 224(%1)\n"
		"movntdq %%xmm15, 240(%1)\n"
		:: "r" (cfrom), "r" (tto):
		"memory"
		,"xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15"
		);
		cfrom+=256ULL;
		tto+=256ULL;
	}
	else
	/*
	   Only if SRC is aligned on 16-byte boundary.
	   It allows to use movdqa instead of movdqu, which required data
	   to be aligned or a general-protection exception (#GP) is generated.
	*/
	for(; i>0; i--)
	{
		__asm__ __volatile__ (
		"prefetcht0 256(%0)\n"
		"prefetcht0 320(%0)\n"
		"movdqa (%0), %%xmm0\n"
		"movdqa 16(%0), %%xmm1\n"
		"movdqa 32(%0), %%xmm2\n"
		"movdqa 48(%0), %%xmm3\n"
		"movdqa 64(%0), %%xmm4\n"
		"movdqa 80(%0), %%xmm5\n"
		"movdqa 96(%0), %%xmm6\n"
		"movdqa 112(%0), %%xmm7\n"
		"prefetcht0 384(%0)\n"
		"prefetcht0 448(%0)\n"
		"movdqa 128(%0), %%xmm8\n"
		"movdqa 144(%0), %%xmm9\n"
		"movdqa 160(%0), %%xmm10\n"
		"movdqa 176(%0), %%xmm11\n"
		"movdqa 192(%0), %%xmm12\n"
		"movdqa 208(%0), %%xmm13\n"
		"movdqa 224(%0), %%xmm14\n"
		"movdqa 240(%0), %%xmm15\n"
		"movntdq %%xmm0, (%1)\n"
		"movntdq %%xmm1, 16(%1)\n"
		"movntdq %%xmm2, 32(%1)\n"
		"movntdq %%xmm3, 48(%1)\n"
		"movntdq %%xmm4, 64(%1)\n"
		"movntdq %%xmm5, 80(%1)\n"
		"movntdq %%xmm6, 96(%1)\n"
		"movntdq %%xmm7, 112(%1)\n"
		"movntdq %%xmm8, 128(%1)\n"
		"movntdq %%xmm9, 144(%1)\n"
		"movntdq %%xmm10, 160(%1)\n"
		"movntdq %%xmm11, 176(%1)\n"
		"movntdq %%xmm12, 192(%1)\n"
		"movntdq %%xmm13, 208(%1)\n"
		"movntdq %%xmm14, 224(%1)\n"
		"movntdq %%xmm15, 240(%1)\n"
		:: "r" (cfrom), "r" (tto):
		"memory"
		,"xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15"
		);
		cfrom+=256ULL;
		tto+=256ULL;
	  }
	__asm__ __volatile__ ("sfence":::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if(len) small_memcpy(tto, cfrom, len);
	return retval;
}

#define small_memset(to,val,n)\
{\
register unsigned long int dummy;\
    if(n)\
__asm__ __volatile__(\
	"rep; stosb"\
	:"=&D"(to), "=&c"(dummy)\
	:"0" (to), "1" (n), "a" ((char)val)\
	: "memory","cc");\
}

#define XMMREG_SIZE 16
/* Fast memory set. See comments for fast_memcpy */
static void * RENAME(fast_memset)(void * to, int val, size_t len)
{
	void *retval;
	size_t i;
	unsigned char mm_reg[XMMREG_SIZE], *pmm_reg;
	unsigned char *tto=to;

	retval = tto;
	if(len >= MIN_LEN)
	{
	  register unsigned long int delta;
	  delta = ((unsigned long long int)tto)&(XMMREG_SIZE-1);
	  if(delta)
	  {
	    delta=XMMREG_SIZE-delta;
	    len -= delta;
	    small_memset(tto, val, delta);
	  }
	  i = len >> 7; /* len/128 */
	  len&=127;
	  pmm_reg = mm_reg;
	  small_memset(pmm_reg,val,sizeof(mm_reg));
	__asm__ __volatile__(
		"movdqa (%0), %%xmm0\n"
		:: "r"(mm_reg):"memory");
	for(; i>0; i--)
	{
		__asm__ __volatile__ (
		"movntdq %%xmm0, (%0)\n"
		"movntdq %%xmm0, 16(%0)\n"
		"movntdq %%xmm0, 32(%0)\n"
		"movntdq %%xmm0, 48(%0)\n"
		"movntdq %%xmm0, 64(%0)\n"
		"movntdq %%xmm0, 80(%0)\n"
		"movntdq %%xmm0, 96(%0)\n"
		"movntdq %%xmm0, 112(%0)\n"
		:: "r" (tto) : "memory");
		tto+=128ULL;
	}
	__asm__ __volatile__ ("sfence":::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if(len) small_memset(tto, val, len);
	return retval;
}

#ifdef REGMM_SIZE
#undef REGMM_SIZE
#endif
#define REGMM_SIZE 16
static void __FASTCALL__ RENAME(InterleaveBuffers)(tUInt32 limit,
				    void *destbuffer,
				    const void *evenbuffer,
				    const void *oddbuffer)
{
  register char *destbuffptr;
  register const char *oddptr, *evenptr;
  register tUInt32 freq;
  destbuffptr = (char *)destbuffer;
  evenptr = (const char *)evenbuffer;
  oddptr = (const char *)oddbuffer;
  freq = 0;
  if(limit>REGMM_SIZE*4-1)
  {
      register tUInt64 delta, nlimit, step;
      step = REGMM_SIZE*2;
      /* Try to align buffers on boundary of REGMM_SIZE */
      delta = ((tUInt64)evenptr)&(REGMM_SIZE-1);
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
	 __asm __volatile("movdqa	(%0), %%xmm0\n\t"
	       ::"r"(evenptr):"memory");
	 evenptr+=step;
	 __asm __volatile("movdqa	%%xmm0, %%xmm1\n\t"
	       "punpckhbw (%0), %%xmm0\n\t"	       
	      ::"r"(oddptr):"memory");
	 nlimit--;
	 __asm __volatile("punpcklbw (%0), %%xmm1\n\t"
	       ::"r"(oddptr):"memory");
	 oddptr+=step;
	 __asm __volatile("movdqu	%%xmm0, (%0)\n\t"
	       "movdqu	%%xmm2, 16(%0)\n\t"
	      ::"r"(destbuffptr):"memory");
	 destbuffptr+=step*2;
      }
  }
  /* If tail exists then finish it */
  while(freq<limit)
  {
    *destbuffptr++ = *evenptr++;
    *destbuffptr++ = *oddptr++;
    freq++;
  }
}

static void __FASTCALL__ RENAME(CharsToShorts)(tUInt32 limit,
					     void *destbuffer,
					     const void *evenbuffer)
{
  register char *destbuffptr;
  register const char *evenptr;
  register tUInt32 freq;
  destbuffptr = (char *)destbuffer;
  evenptr = (const char *)evenbuffer;
  freq = 0;
  if(limit>REGMM_SIZE*4-1)
  {
      register tUInt64 delta, nlimit, step;
      step = REGMM_SIZE*2;
      /* Try to align buffer on boundary of REGMM_SIZE */
      delta = ((tUInt64)evenptr)&(REGMM_SIZE-1);
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
      __asm __volatile("pxor	%%xmm7, %%xmm7":::"memory");
      while(nlimit)
      {
	 /* Interleave mmx and cpu instructions */
	 __asm __volatile("movdqa	(%0),%%xmm0\n\t"
	       ::"r"(evenptr):"memory");
	 evenptr+=step;
	 __asm __volatile("movdqa	%%xmm0, %%xmm1\n\t"
	       "punpckhbw %%xmm7, %%xmm0\n\t"
	      :::"memory");
	 nlimit--;
	 __asm __volatile(
	       "punpcklbw %%xmm7, %%xmm1\n\t"
	       :::"memory");
	 __asm __volatile("movdqu	%%xmm0, (%0)\n\t"
	       "movdqu	%%xmm1, 16(%0)\n\t"
	       ::"r"(destbuffptr):"memory");
	 destbuffptr+=step*2;
      }
  }
  /* If tail exists then finish it */
  while(freq<limit)
  {
    *destbuffptr++ = *evenptr++;
    *destbuffptr++ = 0;
    freq++;
  }
}

static void __FASTCALL__ RENAME(ShortsToChars)(tUInt32 limit,
				     void * destbuffer, const void * srcbuffer)
{
  register char *destbuffptr;
  register const char *srcptr;
  register tUInt32 freq;
  destbuffptr = (char *)destbuffer;
  srcptr = (const char *)srcbuffer;
  freq = 0;
  if(limit>REGMM_SIZE*4-1)
  {
      tUInt64 delta, nlimit, step;
      step = REGMM_SIZE*2;
      /* Try to align buffers on boundary of REGMM_SIZE */
      delta=((tUInt64)destbuffptr)&(REGMM_SIZE-1);
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
	 __asm __volatile("movdqu	(%0), %%xmm0\n\t"
	       ::"r"(srcptr):"memory");
	 nlimit--;
	 __asm __volatile("packuswb (%0), %%xmm0\n\t"
	       ::"g"(&srcptr[REGMM_SIZE]):"memory");
	 srcptr+=step*2;
	 __asm __volatile("movdqa	%%xmm0, (%0)\n\t"
	       ::"r"(destbuffptr):"memory");
	 destbuffptr+=step;
      }
  }
  /* If tail exists then finish it */
  while(freq<limit)
  {
    *destbuffptr++ = *srcptr;
    srcptr+=2;
    freq++;
  }
}
