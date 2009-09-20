/*
  This file contains most common definitions to make MMX stuff
  easy portable between different kinds of mmx clones
  Written By Nickols_K <nickols_k@mail.ru>
*/

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
#if !defined( HAVE_MMX2 ) && defined( HAVE_3DNOW )
#define HAVE_K6_2PLUS
#endif

#undef MMREG_SIZE
#if defined(HAVE_SSE2) || defined (HAVE_SSE)
#define MMREG_SIZE 16
#else
#define MMREG_SIZE 8
#endif

#undef PREFETCH
#undef PREFETCHW
#undef PAVGB
#ifdef HAVE_3DNOW
#define PREFETCH  "prefetch"
#define PREFETCHW "prefetchw" 
#define PAVGB	  "pavgusb"
#elif defined ( HAVE_MMX2 )
#define PREFETCH "prefetchnta"
#define PREFETCHW "prefetcht0" 
#define PAVGB	  "pavgb"
#else
#define PREFETCH "/nop"
#define PREFETCHW "/nop" 
#endif

#undef EMMS
#ifdef HAVE_3DNOW
/* On K6 femms is faster of emms. On K7 femms is directly mapped on emms. */
#define EMMS     "femms"
#else
#define EMMS     "emms"
#endif

#undef MOVNTQ
#undef SFENCE
#ifdef HAVE_MMX2
#define MOVNTQ "movntq"
#define SFENCE "sfence"
#else
#define MOVNTQ "movq"
#define SFENCE "/nop"
#endif
